/* University of Washington TPT-Finder Project
 * Blake Hannaford,  July 25
 *
 * Derived from:
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
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


//
//  Blake's demo of multiple FreeRTOS tasks
//            (Jul 25)
//


// #include "basic_freertos_smp_usage.h"

// #pragma once

/*------------------------------------------------------------*/
/* Macros */
#define PROMPT_STR CONFIG_IDF_TARGET
#define TASK_PRIO_3         3
#define TASK_PRIO_2         2
#define COMP_LOOP_PERIOD    5000
#define SEM_CREATE_ERR_STR                "semaphore creation failed"
#define QUEUE_CREATE_ERR_STR              "queue creation failed"

int comp_creating_task_entry_func(int argc, char **argv);
int comp_queue_entry_func(int argc, char **argv);
int comp_lock_entry_func(int argc, char **argv);
int comp_task_notification_entry_func(int argc, char **argv);
int comp_batch_proc_example_entry_func(int argc, char **argv);


/////////// BH
// Local function protos:

//FREE-RTOS tasks:
static void LED_task(void*);
static void hello_task(void *arg);
static void LCD_16x2_task(void*);

//  local functions:
static void configure_led(void);
static void setLedFromState(void);
void LCD_16x2_init(void) ;
void handle_error(char* );

#define TAG  "BH_Demo-3Tasks"

//   BH defines
#define DEFAULT_STACK  4096
#define BLINK_PERIOD    300 //ms
#define BLINK_GPIO 8




void handle_error(char* msg){
#define TAGe  "TASK ERROR: "
    while(1){  // freeze the system (sort of)
        ESP_LOGI(TAGe, "%s", msg);
        vTaskDelay(2000/portTICK_PERIOD_MS);
    }
}

extern char lcd_LOG_message[];

// blinker state
static uint8_t s_led_state = 0;

// i2c mutex:  this is used to make sure only one task can transact on i2c at a time.
SemaphoreHandle_t i2cMutex = NULL;


/*
 *   LED blink task
 */

static void LED_task(void*)
{
    while (1) {
        ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
        setLedFromState();
        /* Toggle the LED state */
        s_led_state = !s_led_state;
        vTaskDelay(BLINK_PERIOD / portTICK_PERIOD_MS);
        }
}

/*********************************************************************
 *   Initialize LED task differently depending on hardware
 *
 *   Following code block (from "blink" example) is overkill for
 *     Waveshare ESP32c6-zero for which
 *        we should configure:   Blink LED type:  "LED strip"
 *        and
 *                               LED strip backend peripheral "RMT"
 */

#ifdef CONFIG_BLINK_LED_STRIP

static led_strip_handle_t led_strip;

static void setLedFromState(void)
{
    /* If the addressable LED is enabled */
    if (s_led_state) {
        /* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
        led_strip_set_pixel(led_strip, 0, 16, 16, 16);
        /* Refresh the strip to send data */
        led_strip_refresh(led_strip);
    } else {
        /* Set all LED off to clear all pixels */
        led_strip_clear(led_strip);
    }
}

static void configure_led(void)
{
    ESP_LOGI(TAG, "Configure pins to blink ADDRESSABLE LED!");
    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = BLINK_GPIO,
        .max_leds = 1, // at least one LED on board
    };
#if CONFIG_BLINK_LED_STRIP_BACKEND_RMT
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
#elif CONFIG_BLINK_LED_STRIP_BACKEND_SPI
    led_strip_spi_config_t spi_config = {
        .spi_bus = SPI2_HOST,
        .flags.with_dma = true,
    };
    ESP_ERROR_CHECK(led_strip_new_spi_device(&strip_config, &spi_config, &led_strip));
#else
#error "unsupported LED strip backend"
#endif
    /* Set all LED off to clear all pixels */
    led_strip_clear(led_strip);
}

#elif CONFIG_BLINK_LED_GPIO   // if this were configured to use GPIO (not what waveshare ESP32C5-Zero uses)

static void setLedFromState(void)
{
    /* Set the GPIO level according to the state (LOW or HIGH)*/
    gpio_set_level(BLINK_GPIO, s_led_state);
}

static void configure_led(void)
{
    ESP_LOGI(TAG, "Configure pins to blink GPIO LED!");
    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

#else
#error "unsupported LED type"
#endif

/*
 *
 *    End of "blink code block"
 */


static void hello_task(void *arg)
{

    int i=0;
    while(1) {
        vTaskDelay(1000/portTICK_PERIOD_MS);
        printf("\n\n\n");
        i++;
        printf("Hello world! (task rep: %d) \n", i);
        printf("\n\n\n");
    }
}

void app_main(void)
{
    //   Set up i2c for all tasks
    i2cMutex = xSemaphoreCreateMutex();
    ESP_LOGI(TAG, "mutex created");
    i2c_master_init();  // now separate from lcd_init()
    ESP_LOGI(TAG, "i2c master is inited");
    // initialize the LCD device via serial backpack
    LCD_16x2_init();
    ESP_LOGI(TAG, "LCD device init ");


    if (i2cMutex != NULL){
        if( xSemaphoreTake( i2cMutex, ( TickType_t ) 10 ) == pdTRUE )
        {
            /* We were able to obtain the semaphore and can now access the
               shared resource. */

            //   Configure LCD display via i2c
            ESP_LOGI(TAG, "%s", lcd_LOG_message );
            // ESP_LOGI(TAG, "got here..." );
            LCD_16x2_init();
            ESP_LOGI(TAG, " LCD display is set up" );

            /* We have finished accessing the shared resource. Release the
               semaphore. */
            xSemaphoreGive( i2cMutex );
        }
        else handle_error("lcd_init Mutex Timeout");
    }
    else handle_error("i2c Mutex FAIL");


    //  Configure the LED GPIO
    configure_led();   // defined above for two configs
    ESP_LOGI(TAG, " LED has been configured.");

    ESP_LOGI(TAG, "\n\n      Starting task(s)...\n\n");

    xTaskCreatePinnedToCore(hello_task, "Hello World Task", DEFAULT_STACK, NULL, TASK_PRIO_3, NULL, tskNO_AFFINITY);
    ESP_LOGI(TAG, "Hello world (serial) task created");

    xTaskCreatePinnedToCore(LED_task, "LED Task", DEFAULT_STACK, NULL, TASK_PRIO_2, NULL, tskNO_AFFINITY);
    ESP_LOGI(TAG, "LED task created");

    xTaskCreatePinnedToCore(LCD_16x2_task, "LCD 16x2 Task", DEFAULT_STACK, NULL, TASK_PRIO_2, NULL, tskNO_AFFINITY);
    ESP_LOGI(TAG, "LCD task created");

}
