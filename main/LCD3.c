/* Claude.ai:   14-Jan-26
 *
 *
 * This is not working code but has pretty good fragments...
 */

// 1. Define a message structure
typedef struct {
    uint8_t row;
    uint8_t col;
    char text[20];  // Adjust size as needed
} lcd_message_t;

// 2. Create queue handle (global or in main)
QueueHandle_t lcdQueue = NULL;

// 3. In your app_main() or initialization function:
void app_main(void) {
    // Create queue that can hold 5 messages
    lcdQueue = xQueueCreate(5, sizeof(lcd_message_t));

    if (lcdQueue == NULL) {
        ESP_LOGE(LCD_tasks_TAG, "Failed to create LCD queue");
        return;
    }

    // Create your LCD task...
    xTaskCreate(lcd_task, "LCD_Task", 4096, NULL, 5, NULL);

    // Create other tasks...
}

// 4. Modified LCD task (receiver):
void lcd_task(void *pvParameters) {
    lcd_message_t msg;

    while (1) {
        // Wait for message (blocks until message arrives)
        if (xQueueReceive(lcdQueue, &msg, portMAX_DELAY) == pdTRUE) {

            if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
                lcd_put_cursor(lcda, msg.row, msg.col);
                lcd_send_string(lcda, msg.text);
                xSemaphoreGive(i2cMutex);
            }
        }
    }
}

// 5. From any other task - send message to LCD:
void some_other_task(void *pvParameters) {
    lcd_message_t msg;
    int counter = 0;

    while (1) {
        // Prepare message
        msg.row = 1;
        msg.col = 0;
        snprintf(msg.text, sizeof(msg.text), "N: %04d", counter++);

        // Send to LCD task (non-blocking with timeout)
        if (xQueueSend(lcdQueue, &msg, pdMS_TO_TICKS(100)) != pdTRUE) {
            ESP_LOGW("TASK", "LCD queue full!");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
