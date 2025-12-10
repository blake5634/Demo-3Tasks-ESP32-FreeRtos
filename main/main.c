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
#include "LCD_task.h"
#include "unistd.h"
#include "photonicTask.h"


//
//  Blake's demo of multiple FreeRTOS tasks
//            (Jul 25)
//
//     Adapted to drive the TPT-Finder photonics LED/Photodiode board
//            (Nov/Dec 25)
//


/*------------------------------------------------------------*/
/* Macros */
#define PROMPT_STR CONFIG_IDF_TARGET
#define TASK_PRIO_3         3
#define TASK_PRIO_2         2
#define TASK_PRIO_1         1
#define COMP_LOOP_PERIOD    5000
#define SEM_CREATE_ERR_STR      "semaphore creation failed"
#define QUEUE_CREATE_ERR_STR    "queue creation failed"

//
//  Choose tasks which will be run
//
#define TASK_ON              1
#define TASK_OFF             0

#define LED_TASK            TASK_ON
#define LCD_TASK            TASK_ON
#define PHOTONIC_TASK       TASK_OFF
#define HELLO_WORLD_TASK    TASK_ON
#define IDLE_MON_TASK       TASK_OFF
#define PHOTONICS_TEST      TASK_OFF


// LED Task related functions (in this file)
static void configure_led(void);
static void setLedFromState(void);


/////////// BH
// Local function prototypes:

void handle_error(char* );  // log an error to console and freeze


//FREE-RTOS tasks defined here:
// blinker state
static uint8_t s_led_state = 0;
static void LED_task(void*);
static void hello_task(void *arg);
// defined in LCD_task.h
// void LCD_task1(void *lcd_addr);
// void LCD_task2(void *lcd_addr);  // we're only using one in TPT-finder



#define TAG  "TPT-Photonics"

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

// i2c mutex:  this is used to make sure only one task can transact on i2c at a time.
SemaphoreHandle_t i2cMutex = NULL;


/*
 *   LED blink task
 */

static void LED_task(void*)
{
    while (1) {
        // ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
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
 *
 *   TODO:  move this to a separate .c file for LED hardware setup.
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
// RMT is the required config for WaveShare ESP32-C6
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



// if this were configured to use GPIO (NOT what waveshare ESP32C6-Zero uses)
#elif CONFIG_BLINK_LED_GPIO

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
 ***************************************************************/

long int idle_run_counter = 0;


static void hello_task(void *arg)
{
    static long int prev_tick_cnt = 0;
    int i=0;
    while(1) {
        vTaskDelay(2000/portTICK_PERIOD_MS);
        printf("\n\n\n");
        i++;
        long int T = xTaskGetTickCount();  // current tick count
        long int deltaT  = T - prev_tick_cnt;
        printf("Hello world! (task rep: %d) (idle: %ld/%ld)\n", i,idle_run_counter,deltaT);
        prev_tick_cnt = T;
        idle_run_counter = 0; // reset every 2 sec
        printf("\n\n\n");
    }
}


/*
 *  Task to count and display idle ticks
 *
 *
 */
static void idle_mon(void *arg){
    while(1){
        idle_run_counter++;
    }
}

void app_main(void)
{
    /*
     * Hardware and Software setups and inits
     */

    //   Set up i2c for all tasks
    i2cMutex = xSemaphoreCreateMutex();
    ESP_LOGI(TAG, "mutex created");
    i2c_master_init();  // now separate from lcd_init()
    ESP_LOGI(TAG, "i2c master is inited");
    
    if (LCD_TASK == TASK_ON) {
        // initialize LCD hardware
        LCD_reset(SLAVE_ADDRESS1_LCD);
        // LCD_reset(SLAVE_ADDRESS2_LCD);
        ESP_LOGI(TAG, "LCD device init completed ");
    }

    // config hardware GPIO pins for on-board LED (board-specific)
    if (LED_TASK == TASK_ON) {
        configure_led();   // defined above for two configs
        ESP_LOGI(TAG, "on-board LED hardware has been configured.");
    }

    if (PHOTONIC_TASK == TASK_ON || PHOTONICS_TEST==TASK_ON) {
        // setup for photonics board interface.
        init_photonics();
        ESP_LOGI(TAG, "photonics pinouts have been set");
    }


    /***********************************************************************
     *
     * Start up the Free-RTOS Tasks
     */
    void* argptr = NULL;  // use for task arguments

    ESP_LOGI(TAG, "\n\n      Starting task(s)...\n\n");

    if (HELLO_WORLD_TASK==TASK_ON) {
    /*
     *   HELLO WORLD on serial console
     */
        xTaskCreatePinnedToCore(hello_task, "Hello World Task", DEFAULT_STACK, NULL, TASK_PRIO_3, NULL, tskNO_AFFINITY);
        ESP_LOGI(TAG, "Hello world (serial) task created");
        }

    if (LED_TASK==TASK_ON) {
        /*
        * Flash the onboard LED
        */
        xTaskCreatePinnedToCore(LED_task, "LED Task", DEFAULT_STACK, NULL, TASK_PRIO_2, NULL, tskNO_AFFINITY);
        ESP_LOGI(TAG, "LED task created");
        }

    if (PHOTONIC_TASK==TASK_ON) {
        //
        // Generate 100Hz cycle and coordinate OFF time  ON time and
        //        ADC readings
        //
        argptr = NULL;
        xTaskCreatePinnedToCore(photonic_task, "Photonics Task", DEFAULT_STACK, argptr, TASK_PRIO_3, NULL, tskNO_AFFINITY);
        ESP_LOGI(TAG, "Photonics task created");
        }

    if (LCD_TASK==TASK_ON) {
        //
        // Display messages on the LCD
        //
        uint8_t lcd_address1 = SLAVE_ADDRESS1_LCD;
        argptr = &lcd_address1;
        // uint8_t lcd_address2 = SLAVE_ADDRESS2_LCD;
        xTaskCreatePinnedToCore(LCD_task1, "LCD Task", DEFAULT_STACK, argptr, TASK_PRIO_2, NULL, tskNO_AFFINITY);
        // xTaskCreatePinnedToCore(LCD_task2, "LCD 16x2 Task", DEFAULT_STACK, (void*)lcd_address2, TASK_PRIO_2, NULL, tskNO_AFFINITY);
        ESP_LOGI(TAG, "LCD task created");
        }

    if(IDLE_MON_TASK==TASK_ON){
        argptr = NULL;
        xTaskCreatePinnedToCore(idle_mon, "Idle Monitor", DEFAULT_STACK, argptr, TASK_PRIO_1, NULL, tskNO_AFFINITY);
        }

    vTaskStartScheduler();

    if(PHOTONICS_TEST==TASK_ON){
        while(1){
                int testval = photonic_test();
                ESP_LOGI(TAG, "A/D Test value: %d", testval);
                vTaskDelay(pdMS_TO_TICKS(1000));
                }
        }
}
