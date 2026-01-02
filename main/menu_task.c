#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "MENU";
#define INPUT_BUFFER_SIZE 64

void menu_task(void *pvParameters)
{
    char input_buffer[INPUT_BUFFER_SIZE];
    char choice;

    while (1)
    {
        // Display menu
        printf("\n================================\n");
        printf("        SYSTEM MENU\n");
        printf("================================\n");
        printf("1. View System Status\n");
        printf("2. Configure Parameters\n");
        printf("3. Run Diagnostics\n");
        printf("\nEnter your choice (1-3): ");
        fflush(stdout);

        // Read string input until Enter
        if (fgets(input_buffer, INPUT_BUFFER_SIZE, stdin) != NULL)
        {
            // Remove trailing newline if present
            size_t len = strlen(input_buffer);
            if (len > 0 && input_buffer[len-1] == '\n')
            {
                input_buffer[len-1] = '\0';
            }

            // Check if input is a single character
            if (strlen(input_buffer) == 1)
            {
                choice = input_buffer[0];

                // Process selection
                switch(choice)
                {
                    case '1':
                        printf("\n[Status] System running normally\n");
                        break;

                    case '2':
                        printf("\n[Config] Configuration menu selected\n");
                        break;

                    case '3':
                        printf("\n[Diag] Running diagnostics...\n");
                        break;

                    default:
                        printf("\nInvalid choice. Please enter 1-3.\n");
                        break;
                }
            }
            else if (strlen(input_buffer) == 0)
            {
                printf("\nNo input received. Please enter 1-3.\n");
            }
            else
            {
                printf("\nInvalid input. Please enter a single digit (1-3).\n");
            }
        }

        // Small delay to prevent console flooding
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


