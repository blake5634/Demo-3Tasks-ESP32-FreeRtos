
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

#include "photonicTask.h"



void init_photonics(void) {
    int x = 5;
    int y = 2;
    return x+y;
    //   1) set PIN_EXCIT_DRIVE to voltage output

    //   2) set PIN_PIN_ADC_PD to input to the ADC
    //   connect ADC  https://docs.espressif.com/projects/esp-idf/en/release-v4.4/esp32/api-reference/peripherals/adc.html
}


void photonic_task()  {
    int flag = 1
    long int on_total = 0
    long int off_total = 0

    int64_t timeused = 0
    int64_t waveStart = 0
    int i = 0
    while(i++ <= N_CYCLES){
        timeused = 0
        waveStart = esp_timer_get_time() // microsec
        flag = flag ^ 1; // toggle the flag
        if (flag){ // Excitation 1/2 cycle
            // set excitation LED ON
            // TODO set output bit HERE
            // wait 1/2 way through the 1/2 cycle
            vTaskDelay(pdMS_TO_TICKS(SQUARE_WAVE_HALF_MS>>1));
            on_total += collect_PD_ADC(N_AD_PER_HALF);
        }
        else{      // OFF 1/2 cycle
            // set excitation LED OFF
            // TODo clear output bit here
            // wait 1/2 way through the 1/2 cycle
            vTaskDelay(pdMS_TO_TICKS(SQUARE_WAVE_HALF_MS>>1));
            on_total += collect_PD_ADC(N_AD_PER_HALF);
            timeused = esp_timer_get_time() - waveStart;
        }

        // finish off the period with more accurate timing, allowing for
        //   ADC cycles etc.
        vTaskDelay(pdMS_TO_TICKS(2*SQUARE_WAVE_HALF_MS - timeused/1000));
        }

    // compute the two averages and do something with it.

}

int collect_PD_ADC(int n){

}
