/*
 * sketch from AI for timer code for sq wave generation and ADC
 *
 *   Claude.ai    29-Dec-25

 Key Points

IRAM_ATTR: The ISR must be in IRAM for deterministic timing
Auto-reload: Enables continuous periodic interrupts
Timer divider: Reduces the 80 MHz APB clock to 1 MHz (easier calculations)
Independence: Hardware timer runs separately from FreeRTOS scheduler
ISR duration: Keep your ISR short - just toggle the GPIO

 */

//  Compute timer config
// For 200 Hz square wave
#define SQUARE_WAVE_FREQ_HZ  200
#define TIMER_DIVIDER        80      // Hardware timer clock divider
#define TIMER_SCALE          (TIMER_BASE_CLK / TIMER_DIVIDER)  // Convert to seconds
// TIMER_BASE_CLK is 80 MHz for ESP32

// Calculate alarm value for half period (toggle interval)
#define TIMER_INTERVAL_US    (1000000 / (SQUARE_WAVE_FREQ_HZ * 2))  // 2500 µs
#define TIMER_ALARM_VALUE    (TIMER_INTERVAL_US * TIMER_SCALE / 1000000)

// Hardware defines
#define OUTPUT_GPIO          GPIO_NUM_2  // Choose your GPIO pin
#define TIMER_GROUP          TIMER_GROUP_0
#define TIMER_IDX            TIMER_0

// ISR is static for fast (in-ram) execution
static bool IRAM_ATTR timer_isr_callback(void *args)
{
    // Toggle the GPIO
    static uint8_t level = 0;
    gpio_set_level(OUTPUT_GPIO, level);
    level = !level;

    // Return whether we need to yield to a higher priority task
    return false;
}

//  Init the GPIO
void init_gpio(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << OUTPUT_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(OUTPUT_GPIO, 0);
}

// Init the timer
void init_timer(void)
{
    // Timer configuration
    timer_config_t config = {
        .divider = TIMER_DIVIDER,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .auto_reload = TIMER_AUTORELOAD_EN,
    };

    // Initialize timer
    timer_init(TIMER_GROUP, TIMER_IDX, &config);

    // Set timer counter value to 0
    timer_set_counter_value(TIMER_GROUP, TIMER_IDX, 0);

    // Set alarm value (when to trigger interrupt)
    timer_set_alarm_value(TIMER_GROUP, TIMER_IDX, TIMER_ALARM_VALUE);

    // Enable timer interrupt
    timer_enable_intr(TIMER_GROUP, TIMER_IDX);

    // Register ISR callback
    timer_isr_callback_add(TIMER_GROUP, TIMER_IDX, timer_isr_callback, NULL, 0);

    // Start timer
    timer_start(TIMER_GROUP, TIMER_IDX);
}


//    Demo App
void app_main(void)
{
    // Initialize GPIO
    init_gpio();

    // Initialize and start timer
    init_timer();

    printf("200 Hz square wave generator started on GPIO %d\n", OUTPUT_GPIO);

    // Your main application continues here
    // The timer ISR runs independently
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));  // Your 10ms tick is unaffected
        // Do other work...
    }
}



/////////////////////////////////////////////////////////////////////
//
// modified code with state machine for A/D sample aquisition
//
#include "driver/timer.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define OUTPUT_GPIO          GPIO_NUM_2
#define TIMER_GROUP          TIMER_GROUP_0
#define TIMER_IDX            TIMER_0
#define TIMER_DIVIDER        80       // 80MHz / 80 = 1MHz (1µs resolution)
#define TIMER_SCALE          1000000  // 1 MHz = 1µs per tick

// Timing definitions (in microseconds)
#define PHASE_DURATION_US    2500     // 2.5 ms per phase
#define SAMPLE_DELAY_US      1150     // Wait 1.15ms before starting samples
#define INTER_SAMPLE_US      100      // 100µs between samples

// State machine states
typedef enum {
    STATE_GPIO_TOGGLE,     // Toggle GPIO and wait for middle of phase
    STATE_SAMPLE_1,        // Take first sample
    STATE_SAMPLE_2,        // Take second sample
    STATE_SAMPLE_3,        // Take third sample
} timer_state_t;

// Global variables
static volatile timer_state_t current_state = STATE_GPIO_TOGGLE;
static volatile uint8_t gpio_level = 0;
static volatile uint32_t sample_count = 0;

// Your A/D reading function (declare this)
extern uint16_t read_adc(void);  // Replace with your actual function

// Storage for samples (double buffer for each phase)
#define SAMPLES_PER_PHASE 3
static volatile uint16_t samples_positive[SAMPLES_PER_PHASE];
static volatile uint16_t samples_zero[SAMPLES_PER_PHASE];
static volatile uint8_t sample_index = 0;


// Timer ISR with state machine
static bool IRAM_ATTR timer_isr_callback(void *args)
{
    uint64_t next_alarm;

    switch(current_state) {
        case STATE_GPIO_TOGGLE:
            // Toggle GPIO
            gpio_set_level(OUTPUT_GPIO, gpio_level);
            gpio_level = !gpio_level;

            // Reset sample index
            sample_index = 0;

            // Wait until middle of this phase to start sampling
            next_alarm = SAMPLE_DELAY_US;
            current_state = STATE_SAMPLE_1;
            break;

        case STATE_SAMPLE_1:
            // Take first A/D sample
            if (gpio_level == 1) {
                samples_positive[0] = read_adc();
            } else {
                samples_zero[0] = read_adc();
            }

            // Schedule next sample
            next_alarm = INTER_SAMPLE_US;
            current_state = STATE_SAMPLE_2;
            break;

        case STATE_SAMPLE_2:
            // Take second A/D sample
            if (gpio_level == 1) {
                samples_positive[1] = read_adc();
            } else {
                samples_zero[1] = read_adc();
            }

            // Schedule next sample
            next_alarm = INTER_SAMPLE_US;
            current_state = STATE_SAMPLE_3;
            break;

        case STATE_SAMPLE_3:
            // Take third A/D sample
            if (gpio_level == 1) {
                samples_positive[2] = read_adc();
            } else {
                samples_zero[2] = read_adc();
            }

            sample_count++;

            // Calculate time remaining in this phase
            // We've used: SAMPLE_DELAY_US + 2*INTER_SAMPLE_US + ~33µs for samples
            // Remaining: PHASE_DURATION_US - (SAMPLE_DELAY_US + 2*INTER_SAMPLE_US + 50)
            next_alarm = PHASE_DURATION_US - SAMPLE_DELAY_US - 2*INTER_SAMPLE_US - 50;
            current_state = STATE_GPIO_TOGGLE;
            break;
    }

    // Set next alarm (relative to current time)
    timer_set_alarm_value(TIMER_GROUP, TIMER_IDX, next_alarm);
    timer_set_counter_value(TIMER_GROUP, TIMER_IDX, 0);

    return false;
}


void init_gpio(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << OUTPUT_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(OUTPUT_GPIO, 0);
}


void init_timer(void)
{
    timer_config_t config = {
        .divider = TIMER_DIVIDER,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .auto_reload = TIMER_AUTORELOAD_DIS,  // Manual reload for variable timing
    };

    timer_init(TIMER_GROUP, TIMER_IDX, &config);
    timer_set_counter_value(TIMER_GROUP, TIMER_IDX, 0);

    // First alarm: toggle GPIO immediately
    timer_set_alarm_value(TIMER_GROUP, TIMER_IDX, 10);  // Start quickly

    timer_enable_intr(TIMER_GROUP, TIMER_IDX);
    timer_isr_callback_add(TIMER_GROUP, TIMER_IDX, timer_isr_callback, NULL, 0);
    timer_start(TIMER_GROUP, TIMER_IDX);
}


// Optional: Function to retrieve samples from main code
void get_latest_samples(uint16_t *pos_samples, uint16_t *zero_samples)
{
    taskENTER_CRITICAL();
    for (int i = 0; i < SAMPLES_PER_PHASE; i++) {
        pos_samples[i] = samples_positive[i];
        zero_samples[i] = samples_zero[i];
    }
    taskEXIT_CRITICAL();
}


void app_main(void)
{
    init_gpio();
    init_timer();

    printf("200 Hz square wave with A/D sampling started\n");

    uint16_t pos[3], zero[3];

    while(1) {
        vTaskDelay(pdMS_TO_TICKS(100));

        // Get latest samples
        get_latest_samples(pos, zero);

        printf("Sample #%lu - Positive phase: %d, %d, %d | Zero phase: %d, %d, %d\n",
               (unsigned long)sample_count,
               pos[0], pos[1], pos[2],
               zero[0], zero[1], zero[2]);
    }
}
/*
## Key Design Decisions

1. **State Machine**: Four states handle GPIO toggle and three sequential samples
2. **Dynamic Alarm**: Each state sets the next alarm value based on what needs to happen next
3. **Auto-reload OFF**: We manually set each alarm to get precise timing control
4. **Sample Storage**: Separate arrays for positive and zero phases
5. **Critical Sections**: Protect sample reading in main code

## Timing Breakdown Per Cycle (5 ms total)

t=0.00ms:  GPIO HIGH, State=STATE_GPIO_TOGGLE
t=1.15ms:  Sample 1 (positive phase)
t=1.25ms:  Sample 2 (positive phase)
t=1.35ms:  Sample 3 (positive phase)
t=2.50ms:  GPIO LOW, State=STATE_GPIO_TOGGLE
t=3.65ms:  Sample 1 (zero phase)
t=3.75ms:  Sample 2 (zero phase)
t=3.85ms:  Sample 3 (zero phase)
t=5.00ms:  Cycle repeats...
*/




