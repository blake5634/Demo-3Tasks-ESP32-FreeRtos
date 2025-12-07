
#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
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

#include "photonicTask.h"

#define TAG "TPT Photonics Task: "

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
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,  // 12-bit for ESP32-C6
        .atten = TPT_ADC_ATTEN
    };
    adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_2, &config);
    ESP_LOGI(TAG, "ADC Configured");

    // Doc ref:
    //   connect ADC  https://docs.espressif.com/projects/esp-idf/en/release-v4.4/esp32/api-reference/peripherals/adc.html

    return statusCode;
    }


void photonic_task(void*) {
    int flag = 1;
    unsigned long int on_total = 0;
    unsigned long int off_total = 0;

    int64_t timeused = 0;
    int64_t pulseStart = 0;
    int i = 0;  // Added missing semicolon
    while(1) {
        i=0;
        off_total= 0;
        on_total = 0;
        while(i++ < 2*N_CYCLES) {
            timeused = 0;  // Added missing semicolon
            flag = flag ^ 1; // Toggle the flag
            pulseStart = esp_timer_get_time(); // microsec - Added missing semicolon
            if (flag) { // Excitation-ON 1/2 cycle
                // ESP_LOGI(TAG, "Start Excit. half-cycle");
                // Set excitation LED ON
                gpio_set_level(PIN_EXCIT_DRIVE, 1);
                // Wait 1/2 way through the 1/2 cycle
                vTaskDelay(pdMS_TO_TICKS(SQUARE_WAVE_HALF_MS >> 1));
                on_total += collect_PD_ADC(N_AD_PER_HALF);
            }
            else { // Excitation-OFF 1/2 cycle
                // ESP_LOGI(TAG, "Start OFF half-cycle");
                // Set excitation LED OFF
                gpio_set_level(PIN_EXCIT_DRIVE, 0);
                // Wait 1/2 way through the 1/2 cycle
                vTaskDelay(pdMS_TO_TICKS(SQUARE_WAVE_HALF_MS >> 1));
                off_total += collect_PD_ADC(N_AD_PER_HALF);
            }
            timeused = esp_timer_get_time() - pulseStart;  // uSec
            // Finish off the period with more accurate timing, allowing for
            // ADC cycles etc.
            vTaskDelay(pdMS_TO_TICKS(SQUARE_WAVE_HALF_MS - (timeused/ 1000) ) );
            }

        // Compute the two averages and do something with it.
        int npts = N_AD_PER_HALF * N_CYCLES;
        long int onAvg = on_total/npts;
        long int offAvg = off_total/npts;

        ESP_LOGI(TAG,"onAvg: %ld offAvg: %ld",onAvg,offAvg);
        ESP_LOGI(TAG, "Completed a cycle ... pausing");
        vTaskDelay(pdMS_TO_TICKS(500)); //pause before new cycle
        ESP_LOGI(TAG, "Starting a new cycle");

        }
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
