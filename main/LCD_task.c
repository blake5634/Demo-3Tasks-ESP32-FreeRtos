/* University of Washington TPT-Finder Project
 * Blake Hannaford (Jul 25)
 */

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
#include "led_strip.h"
#include "esp_system.h"
#include "i2c_lcd.h"
#include "unistd.h"

extern SemaphoreHandle_t i2cMutex;
extern void handle_error(char* msg);

/*************************   LCD code
 *
 *
 */
#define d100ms   100000

/*
 *  use idf.py menuconfig
 *
 *   (select "BH_demo_Configuration2")
 */
#ifdef  CONFIG_DRIVE_LCD_YES
#define TAG  "DRIVE LCD_16x2_task"

char lcd_LOG_message[] = "CONFIG:DRIVE_LCD:  YES_LCD, We will be using LCD via i2c";

esp_err_t  LCD_16x2_init() {
    lcd_init();             // Initialize the LCD
    usleep(d100ms);

    lcd_clear();            // Clear the LCD screen
    usleep(d100ms);

    lcd_put_cursor(0, 0);   // Set cursor position to   row, column
    usleep(d100ms);
    lcd_send_string("lcd_init()2 ...");
    usleep(d100ms);
    return((esp_err_t) 0);   // no error checks are implemented yet
}

void LCD_16x2_task(void*){
    int i=0;   // iteration counter
    char numst[20];  // place to hold string to print
    while (1){
        ESP_LOGI(TAG,"  LCD TASK IS ACTIVE");
        if (i2cMutex != NULL){
            if( xSemaphoreTake( i2cMutex, ( TickType_t ) 10 ) == pdTRUE )
            {
                /* We were able to obtain the semaphore and can now access the
                shared resource. */

                lcd_put_cursor(1, 0);

                sprintf(numst, "N: %04d",i++);
                lcd_send_string(numst);     // Display the count (numst)
                //usleep(3*d100ms);

                /* We have finished accessing the shared resource. Release the
                semaphore. */
                xSemaphoreGive( i2cMutex );
            }
            else handle_error("lcd_init Mutex Timeout");
        }
        else handle_error("i2c Mutex FAIL");

        ESP_LOGI(TAG, "LCD update sent");
        vTaskDelay(1000/portTICK_PERIOD_MS);
        }


     }

#elif   CONFIG_DRIVE_LCD_NO
#define TAG  "NO LCD_16x2_task"

char lcd_LOG_message[] = "CONFIG:DRIVE_LCD:  NO_LCD ";

void LCD_16x2_task(void*){
    while(1){
        ESP_LOGI(TAG, "LCD not configed: placeholder: LCD_16x2_task()");
        vTaskDelay(2000/portTICK_PERIOD_MS);
    }
}


esp_err_t LCD_16x2_init() {
    ESP_LOGI(TAG, "LCD not configed: placeholder: LCD_16x2_init()");
    return(esp_err_t 1);  // conf for no LCD is not an error but we can ID it
}
#else
#error "unsupported LCD config"
#endif
