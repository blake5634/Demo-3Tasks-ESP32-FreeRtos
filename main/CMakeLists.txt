
idf_component_register(SRCS main.c i2c_lcd.c LCD_task.c photonicTask.c
                    PRIV_REQUIRES driver spi_flash esp_driver_gpio esp_timer
                    INCLUDE_DIRS ".")
