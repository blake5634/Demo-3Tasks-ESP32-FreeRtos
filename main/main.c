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
#include "esp_chip_info.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_freertos_hooks.h"


//
//  Blake's demo of multiple FreeRTOS tasks    With CPU Load output bit
//            (Jul 26)
//


/*------------------------------------------------------------*/
/* Macros */




/////////// BH
// Local function prototypes:

void handle_error(char* );  // log an error to console and freeze


//FREE-RTOS tasks defined here:
static void PerfBit_SetBusy(void*);
static void PerfBit_SetFree(void*);
static void cpu_load_task_1k(void*);
static void cpu_load_task_10k(void*);
static void cpu_load_task_100k(void*);
static void hello_task(void *arg);
// hook from the IDLE task
//void vApplicationIdleHook( void );  //OLD Style
bool my_idle_callback(void);

// for the tasks
unsigned long squareTheInts(unsigned long);


#define TAG  "TPT-perfBit_testing.c"

//   BH defines
#define DEFAULT_STACK  4096
#define BUSY_GPIO       10    //
#define TEST_GPIO        9    //


#define TASK_PRIO_MAX     configMAX_PRIORITIES - 1

#define TASK_PRIO_2       configMAX_PRIORITIES - 2

#define TASK_PRIO_5       1  // lowest you can have (just above official Idle tas)

// static int working_flag = 0;   // each task needs to set this to 1 while working


void handle_error(char* msg){
#define TAGe  "TASK ERROR: "
    while(1){  // freeze the system (sort of)
        ESP_LOGI(TAGe, "%s", msg);
        vTaskDelay(2000/portTICK_PERIOD_MS);
    }
}

/*
 *   Non task approach.  Each task sets the BUSY bit just
 *     after starting its loop, and clears it just
 *         before  vTaskDelay (reset by idle callback),
 *
 */
void Set_Busy(void){
        gpio_set_level(BUSY_GPIO, 1);
}
void Clear_Busy(void){
        gpio_set_level(BUSY_GPIO, 0);
}



/*
 *   Set Perf Bit task (priority PRIOR_HIGHEST)
 */

static void PerfBit_SetBusy(void*)
{   int x = 1;
    while(1) {
        x = ~x;
        gpio_set_level(BUSY_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(7));
    }
}

/*
 *   Set Perf Bit task (priority PRIOR_LOWEST)
 */

static void PerfBit_SetFree(void*)
{
    while(1) {
        gpio_set_level(BUSY_GPIO, 0);  // will be pre-empted!
        vTaskDelay(pdMS_TO_TICKS(4));
        }
}

/*
 *   OLD STYLE
void vApplicationIdleHook( void ){  // same intent as PerfBit_SetFree
    gpio_set_level(BUSY_GPIO, 0);
}
*/

/*
 * New style
 */
bool my_idle_callback(void) {
    // gpio_set_level(BUSY_GPIO, 0);  // clear if idle
    return true; // Call once per tick
}

/*
 *
 *    Set up three CPU user tasks
 *
 */


static void  cpu_load_task_1k(void*){
    while(1){

    gpio_set_level(TEST_GPIO, 1);  // flag the cycle for scope
    squareTheInts(100);
    gpio_set_level(TEST_GPIO, 0);

    Set_Busy();
    squareTheInts(500);   //
    Clear_Busy();

    // vTaskDelay(pdMS_TO_TICKS(30));
    // vTaskDelay(1);
    }
}

static void  cpu_load_task_10k(void*){
    while(1){
    Set_Busy();
    squareTheInts(20*1000); // uSec
    Clear_Busy();
    // vTaskDelay(1);
    }
}

static void  cpu_load_task_100k(void*){
    while(1){
    Set_Busy();
    squareTheInts(10*1000);
    Clear_Busy();
    // vTaskDelay(1);
    }
}

static void hello_task(void *arg)
{

    int i=0;
    while(1) {
        Clear_Busy();
        vTaskDelay(2000/portTICK_PERIOD_MS);
        Set_Busy();
        printf("\n\n\n");
        i++;
        printf("Hello world! (task rep: %d) \n", i);
        printf("\n\n\n");
    }
}


/*
 *  Compute for N micro sec
 */
unsigned long squareTheInts(unsigned long N){
    // gpio_set_level(TEST_GPIO, 1);
    volatile float data[200]={0.0};
    for (int i=0;i<N*6;i++){  // N in micro sec
        data[i%200] = (float)i * (float)i;
    }
    // gpio_set_level(TEST_GPIO, 0);
    return (unsigned long) N;
}

void app_main(void)
{

    ESP_LOGI(TAG, "configMAX_PRIORITIES = %d\n", configMAX_PRIORITIES);

    // this will be called when there is nothing to do.
    esp_register_freertos_idle_hook(my_idle_callback);

    /*
     * init gpio
     */

    gpio_reset_pin(BUSY_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BUSY_GPIO, GPIO_MODE_OUTPUT);
    gpio_reset_pin(TEST_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(TEST_GPIO, GPIO_MODE_OUTPUT);


    /***********************************************************************
     *
     * Start up the Free-RTOS Tasks
     */

    ESP_LOGI(TAG, "\n\n      Starting task(s)...\n\n");

    int Core = 0;

//    xTaskCreatePinnedToCore(PerfBit_SetBusy, "Set Busy Bit",   DEFAULT_STACK, NULL, TASK_PRIO_MAX, NULL, Core);
    // xTaskCreatePinnedToCore(PerfBit_SetFree, "Clear Busy Bit", DEFAULT_STACK, NULL, TASK_PRIO_5,   NULL, Core);

    xTaskCreatePinnedToCore(cpu_load_task_1k, "CPU load 1k",   DEFAULT_STACK, NULL, 5, NULL, Core);

    xTaskCreatePinnedToCore(cpu_load_task_10k, "CPU load 10k", DEFAULT_STACK, NULL, 4, NULL, Core);

    xTaskCreatePinnedToCore(cpu_load_task_100k, "CPU load 100k", DEFAULT_STACK, NULL, 3, NULL, Core);

    // xTaskCreatePinnedToCore(hello_task, "Hello World", DEFAULT_STACK, NULL, 5, NULL, Core);

    ESP_LOGI(TAG, "\n\n      tasks STARTED...\n\n");

}
