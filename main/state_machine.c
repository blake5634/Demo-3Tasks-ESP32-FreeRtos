#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include "driver/gpio.h"

#include "timer_Photo.h"
#include "state_machine.h"

#define TAG "State Machine Task: "

void state_machine_init(){
    //init_photonics();
    ESP_LOGI(TAG, "photonics pinouts have been set (via State Machine init.)");

    /*
    //   1) set END_PAUSE_INPUT to input
    //
    //Configure the input pin to trigger transition out of pause state
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << END_PAUSE_INPUT),  // Bitmask of pins
        .mode = GPIO_MODE_INPUT,                    // Set as input
        .pull_up_en = GPIO_PULLUP_ENABLE,           // Disable pull-up
        .pull_down_en = GPIO_PULLDOWN_DISABLE,      // Disable pull-down
        .intr_type = GPIO_INTR_DISABLE              // Disable interrupts
    };
    // send the config to hardware
    gpio_config(&io_conf);
    */
    // Reset to clean state first
    gpio_reset_pin(END_PAUSE_INPUT);

    // Configure step-by-step (this is what worked in the test)
    gpio_set_direction(END_PAUSE_INPUT, GPIO_MODE_INPUT);
    gpio_input_enable(END_PAUSE_INPUT);    // ← ADD THIS LINE!
    gpio_set_pull_mode(END_PAUSE_INPUT, GPIO_PULLUP_ONLY);

    // Small delay to let it stabilize
    vTaskDelay(pdMS_TO_TICKS(10));

    ESP_LOGI(TAG, "End Pause Pin Configured");



    // set up GPIO
    ESP_LOGI(TAG, "State Machine input pin has been set (via State Machine init.)");

    return;
}

/*
// State Machine Task  states
typedef enum {
    SM_State_Paused,
    SM_State_Acquiring,
    SM_State_Uploading
} SM_state_t;
*/

static SM_state_t state=SM_State_Paused;


void state_machine_task(void *pvParameters){
    static uint16_t databuff[PHOTO_DATA_BUF_SIZE];

    while (1)
    {
        int bit=1;
        ESP_LOGI(TAG, "StateMachine woke up.");
        bit = gpio_get_level(END_PAUSE_INPUT);
        ESP_LOGI(TAG, "current button input... (%d)",bit);
        switch(state) {
            case SM_State_Paused:{
            int j=0;
                ESP_LOGI(TAG, "*****SM_State_Paused");
                // wait for user input via pushbutton
                while ((bit = gpio_get_level(END_PAUSE_INPUT)) == FLOATING_PIN){
                    vTaskDelay(pdMS_TO_TICKS(10));
                    if ((j++)%100 == 0)
                        ESP_LOGI(TAG, "waiting for button input... (%d)",bit);
                    }
                state = SM_State_Acquiring;
                break;
            }
            case SM_State_Acquiring:{
                ESP_LOGI(TAG, "*****SM_State_Acquiring");
                vTaskDelay(pdMS_TO_TICKS(500));
                state = SM_State_Uploading;
                break;
            }
            case SM_State_Uploading:{
                ESP_LOGI(TAG, "*****SM_State_Uploading");
                vTaskDelay(pdMS_TO_TICKS(2000));
                state = SM_State_Paused;
                break;
            }
        }// end switch cases
    }
}

