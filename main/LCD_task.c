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

// void LCD_16x2_task(void*);
// void LCD_16x2_init(void) ;
void LCD_task(void*); 

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

void LCD_reset(int lcd_addr) {
    int lcd = lcd_addr;
    if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
        printf("Starting: 0x%x\n", lcd);
        
        lcd_init(lcd);             // Initialize the LCD
        usleep(d100ms);

        lcd_clear(lcd);            // Clear the LCD screen
        usleep(d100ms);

 
        usleep(d100ms);
        xSemaphoreGive(i2cMutex);
        usleep(d100ms);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void LCD_task1(void *lcd_addr) {
    int i=0;   // iteration counter
    char numst[20];  // place to hold string to print
    
    int lcd = (uint32_t)lcd_addr;

    char buffer[20];
    lcd_put_cursor(lcd, 0, 0);   // Set cursor position to   row, column
    sprintf(buffer, "Works: 0x%x\n", lcd);
    lcd_send_string(lcd, buffer);

    while (1) {
        if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
            /* We were able to obtain the semaphore and can now access the
//                 shared resource. */
            lcd_put_cursor(lcd, 1, 0);

            sprintf(numst, "N: %04d", i++);

            lcd_send_string(lcd, numst);     // Display the count (numst)
            //usleep(3*d100ms);

            /* We have finished accessing the shared resource. Release the
            semaphore. */
            xSemaphoreGive(i2cMutex);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

void LCD_task2(void *lcd_addr) {
    int lcd = (uint32_t)lcd_addr;
    char buffer[20];
    
    sprintf(buffer, "LCD 0x%x works!", lcd);

    uint8_t flip = 0;

    while (1) {
        if(xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
            lcd_clear(lcd);
            lcd_put_cursor(lcd, flip, 0);
            
            lcd_send_string(lcd, buffer);
        }
        xSemaphoreGive(i2cMutex);
        flip = flip ^ 1;
        vTaskDelay(pdMS_TO_TICKS(1000)); 
    }
}

#elif   CONFIG_DRIVE_LCD_NO
#define TAG  "NO LCD_16x2_task"

char lcd_LOG_message[] = "CONFIG:DRIVE_LCD:  NO_LCD ";

void LCD_16x2_task(void*){
    while(1){
        ESP_LOGI(TAG, "placeholder: LCD_16x2_task()");
        vTaskDelay(2000/portTICK_PERIOD_MS);
    }
}


void LCD_16x2_init(void) {
    ESP_LOGI(TAG, "placeholder: LCD_16x2_init()");
}
#else
#error "unsupported LCD config"
#endif
