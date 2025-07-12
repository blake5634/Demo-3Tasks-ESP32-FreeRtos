# MultiTasking ESP 32 tasks using IDF and FreeRTOS with i2c etc.


 The goal of this demo is to set up a structure of FreeRTOS tasks
 which can be scaled to more serious tasks.
We also wish to protect the i2c driver with a mutex.

This should be able to run on any ESP32 supported by IDF.  The LCD display is
the popular 16x2 display with i2c ``Backpack''.   Our ESP32 board is Waveshare's ESP32c6 Zero.
Lots of info on and an IDF project for
connecting the display is [[HERE].](https://github.com/blake5634/esp32_IDF_i2c_16x2_LCD).

Learning goals of this demo include

- How to set up tasks in FreeRTOS to run using it's non-preemptive scheduler.

- How to use the idf.py menuconfig to define options just before compile time

- How to use a Mutex to guard a resource (in this case i2c) so that only one task will
use it at a time.


The three tasks   are:

**Tasks**

1. Blink the on-board LED at a constant rate. Support different ESP32 boards in terms of an LED hooked to a GPIO *OR*
using the LED_strip approach (such as RGB LEDs).

2. Increment a counter displayed on a 16x2 LCD connected via i2c.

3.  Print "Hello World" periodically on the IDF "monitor" console.

These simple tasks are orchestrated by appropriate FreeRTOS structure and API calls.

- As typical of FreeRTOS tasks, each task has the structure:

```C
   static void taskExample(void *arg){
            while(1){
                //
                // do some work
                //
                vtaskDelay(PERIOD)
                }
            }

```

In this code, the hello_task (main.c line 190) is the cleanest example of this structure.


**Configuration**

- The custom configuration menu is defined in the file `Kconfig.projbuild` ([[GUIDE]](https://medium.com/@bhautik.markeye/esp-idf-configuration-how-to-add-custom-configuration-in-project-config-728f81b8d0d8) to customizing the configuration menu.)

- Options must be / can be configured using IDF's menu system.   To invoke the configuration menu,

    > >get-idf
    > >idf.py menuconfig

    Choose option  `BH_Demo_Configuration2`

    - Specifically

        - Which type of LED does the board have, LED-via GPIO, or LED-via the ``LED_strip'' protocol.

        - For LED_strip, there are two protocols and our hardware (ESP32C6-zero from Waveshare) needs
        to be set to  'backend peripheral "RMT"' (the option for some other boards is "SPI")

        - The cycle time delay for the XXXXXXXXXXx task can be set

        - GPIO pin for the LED can be set (8 in our board)

        - You might not have an i2c 16x2 LCD module connected.   You can turn the LCD module "off" in menuconfig
        and the code will change the LCD task to only simulate the device with log messages.   This way
        you can test this code with a bare board.


**Resource mutual exclusion**

A Mutex Semaphore is a FreeRTOS implementation which can be used by cooperating tasks to ``check out'' the i2c
interface driver so that only one task attempts to do i2c transactions at a time.  The LCD_16x2_task()
(see file LCD_task.c) uses
the Mutex as follows:

1.  The LCD_16x2_task "takes" the Mutex:
```C
if( xSemaphoreTake( i2cMutex, ( TickType_t ) 10 ) == pdTRUE ) {
```

2.  The task works with the display via i2c_lcd calls like  lcd_put_cursor(1, 0);   lcd_send_string("text"); etc.

3.  The task "releases" the Mutex when done to allow others access:
```C
xSemaphoreGive( i2cMutex );
```

