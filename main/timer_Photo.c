
#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gptimer.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_log.h"
#include "esp_system.h"
#include "unistd.h"
#include "esp_timer.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#include "timer_Photo.h"

#define TAG "TPT Timer/Photonics Task: "

/*   29-Dec-2025  BH
 *   New approach: use timer-driven ISR to get control of wave frequency
 *   and sampling times.
 */

// Forward declaration
static bool timer_isr_callback(gptimer_handle_t timer,
                               const gptimer_alarm_event_data_t *edata,
                               void *user_ctx);


// State machine states
typedef enum {
    STATE_GPIO_TOGGLE,
    STATE_SAMPLE_1,
    STATE_SAMPLE_2,
    STATE_SAMPLE_3,
} timer_state_t;

// Globals
static gptimer_handle_t gptimer = NULL;
static volatile timer_state_t current_state = STATE_GPIO_TOGGLE;
static volatile uint8_t gpio_level = 0;
static volatile uint32_t sample_count = 0;


#define SAMPLES_PER_PHASE 3
static volatile uint16_t samples_positive[SAMPLES_PER_PHASE];
static volatile uint16_t samples_zero[SAMPLES_PER_PHASE];

// Your ADC function
extern uint16_t read_adc(void);

// ADC handle
adc_oneshot_unit_handle_t adc1_handle;

esp_err_t init_photonics(void) {
    esp_err_t statusCode = 0; // 0== normal
    //
    //   1) set PIN_EXCIT_DRIVE to voltage output
    //
    //Configure the Excitation LED pin
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_EXCIT_DRIVE),  // Bitmask of pins
        .mode = GPIO_MODE_OUTPUT,                    // Set as output
        .pull_up_en = GPIO_PULLUP_DISABLE,          // Disable pull-up
        .pull_down_en = GPIO_PULLDOWN_DISABLE,      // Disable pull-down
        .intr_type = GPIO_INTR_DISABLE              // Disable interrupts
    };
    // send the config to hardware
    gpio_config(&io_conf);
    ESP_LOGI(TAG, "Excitation Pin Configured");

    //
    //   2) set TPT_PIN_ADC_PD to input to the ADC
    //

    // Configure ADC
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    adc_oneshot_new_unit(&init_config, &adc1_handle);

    // Configure the channel
    adc_oneshot_chan_cfg_t AD_chan_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,  // 12-bit for ESP32-C6
        .atten = TPT_ADC_ATTEN
    };
    adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_2, &AD_chan_config);
    ESP_LOGI(TAG, "ADC Configured");

    // Doc ref:
    //   connect ADC  https://docs.espressif.com/projects/esp-idf/en/release-v4.4/esp32/api-reference/peripherals/adc.html

    // Initialize the Timer
    //     Claide.ai helped

     // Timer configuration - NO DIVIDER in v5.x!
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = TIMER_RESOLUTION_HZ,  // 1MHz resolution
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

    // Register callback
    gptimer_event_callbacks_t cbs = {
        .on_alarm = timer_isr_callback
        };

    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, NULL));

    // Enable timer
    ESP_ERROR_CHECK(gptimer_enable(gptimer));

    // Set first alarm to start quickly
    gptimer_alarm_config_t alarm_config = {
        .alarm_count = 2500,  // 100µs
        .flags.auto_reload_on_alarm = true,
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));

    // Start timer
    ESP_ERROR_CHECK(gptimer_start(gptimer));



    return statusCode;
    }

// ISR is static for fast (in-ram) execution
//     Claide.ai helped
static bool IRAM_ATTR timer_isr_callback(gptimer_handle_t timer,
                                         const gptimer_alarm_event_data_t *edata,
                                         void *user_ctx)  {

    uint64_t next_alarm_count=1000;

    switch(current_state) {
        case STATE_GPIO_TOGGLE:
            // Toggle GPIO
            gpio_set_level(OUTPUT_GPIO, gpio_level);
            gpio_level = !gpio_level;

            // Schedule first sample in middle of phase
            next_alarm_count = edata->alarm_value + SAMPLE_DELAY_US;
            current_state = STATE_SAMPLE_1;
            break;

        case STATE_SAMPLE_1:
            // Take first A/D sample
            if (gpio_level == 1) {
                samples_positive[0] = read_adc();
            } else {
                samples_zero[0] = read_adc();
            }

            next_alarm_count = edata->alarm_value + INTER_SAMPLE_US;
            current_state = STATE_SAMPLE_2;
            break;

        case STATE_SAMPLE_2:
            // Take second A/D sample
            if (gpio_level == 1) {
                samples_positive[1] = read_adc();
            } else {
                samples_zero[1] = read_adc();
            }

            next_alarm_count = edata->alarm_value + INTER_SAMPLE_US;
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

            // Calculate remaining time until next GPIO toggle
            next_alarm_count = edata->alarm_value +
                              (PHASE_DURATION_US - SAMPLE_DELAY_US - 2*INTER_SAMPLE_US - 50);
            current_state = STATE_GPIO_TOGGLE;
            break;
    }

    // Set next alarm
    gptimer_alarm_config_t alarm_config = {
        .alarm_count = next_alarm_count,
        .flags.auto_reload_on_alarm = false,
    };
    gptimer_set_alarm_action(timer, &alarm_config);


    /*
     *    // Toggle the GPIO to drive the LED driver wave
    static uint8_t level = 0;
    gpio_set_level(PIN_EXCIT_DRIVE, level);
    level = !level;
    sample_count++;
    // Return whether we need to yield to a higher priority task


    next_alarm_count = 1000;
    // Set next alarm
    gptimer_alarm_config_t alarm_config = {
        .alarm_count = next_alarm_count,
        .flags.auto_reload_on_alarm = false,
    };
    gptimer_set_alarm_action(timer, &alarm_config);
    */

    return false;
   }

void photonic_task(void*) {
    /*
    int flag = 1;
    unsigned long int on_total = 0;
    unsigned long int off_total = 0;

    int64_t timeused = 0;
    int64_t pulseStart = 0;
    int i = 0;  // Added missing semicolon */
    int cycleCnt = 0;

    while(1) {
        cycleCnt++;
        ESP_LOGI(TAG, "photonic task is doing nothing: %d/%d",cycleCnt, (int)sample_count);
        vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }


uint16_t read_adc(void){
    for (int i=0;i<10000;i++) {int x = 5; x++;}
    return 477;
    }
//
//    Collect one test A/D sample
//
unsigned long int photonic_test(void) {
    return collect_PD_ADC(1);
    }

unsigned long int collect_PD_ADC(int n) {
    int adc_raw;
    int total=0;
    for (int i=0;i<n;i++) {
        // Read raw ADC value (0-4095 for 12-bit) n times.
        adc_oneshot_read(adc1_handle, ADC_CHANNEL_2, &adc_raw);
        total += adc_raw;
        }
    return total; // return value.
    }
