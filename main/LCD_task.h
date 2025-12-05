/*
 * includes and defines for use of LCD_task.c
 *     BH Dec 25
 */
#include "esp_system.h"
#include "i2c_lcd.h"

#define SLAVE_ADDRESS1_LCD 0x27
#define SLAVE_ADDRESS2_LCD 0x26

void LCD_task1(void* argptr);
void LCD_task2(void* argptr);  // we're only using one in TPT-finder

esp_err_t LCD_16x2_init();

void LCD_16x2_task(void*);  // void* required by FreeRTOS

void LCD_reset(uint8_t);

