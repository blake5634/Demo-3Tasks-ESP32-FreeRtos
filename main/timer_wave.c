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



