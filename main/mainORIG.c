/*
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
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_log.h"
#include "led_strip.h"
#include "esp_system.h"


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

// Local function protos:
  //FREE-RTOS tasks:
static void LED_task(void*);
static void hello_task(void *arg);

//  local functions:
static void set_led(void);
static void  configure_led(void);


#define TAG  "BH_Demo_2_Tasks"

//   BH defines
#define DEFAULT_STACK  4096
#define BLINK_PERIOD    200 //ms
#define BLINK_GPIO 8
// blinker state
static uint8_t s_led_state = 0;


static void LED_task(void*)
{
    while (1) {
        ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
        set_led();
        /* Toggle the LED state */
        s_led_state = !s_led_state;
        vTaskDelay(BLINK_PERIOD / portTICK_PERIOD_MS);
        }
}

static void set_led(void)
{
    /* Set the GPIO level according to the state (LOW or HIGH)*/
    gpio_set_level(BLINK_GPIO, s_led_state);
}

static void configure_led(void)
{
    // ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}


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

    //  Configure the LED GPIO
    configure_led();

    /* Print chip information */
    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    printf("\n\n\nThis is %s chip with %d CPU core(s), %s%s%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
           (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
           (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    printf("silicon revision v%d.%d, ", major_rev, minor_rev);
    if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        printf("Get flash size failed");
        return;
    }

    printf("%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());

    printf("\n\n      Starting task(s)...\n\n");
    fflush(stdout);

    xTaskCreatePinnedToCore(hello_task, "Hello World Task", DEFAULT_STACK, NULL, TASK_PRIO_3, NULL, tskNO_AFFINITY);

    xTaskCreatePinnedToCore(LED_task, "LED Task", DEFAULT_STACK, NULL, TASK_PRIO_2, NULL, tskNO_AFFINITY);

}
