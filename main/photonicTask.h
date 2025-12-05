#ifndef PHOTONIC_TASK_H //prevent double includes
#define PHOTONIC_TASK_H
#endif

#include "esp_log.h"
#include "unistd.h"

// declarationss
esp_err_t init_photonics(void);
void photonic_task(void*);


// pin assignments for photonics
#define PIN_EXCIT_DRIVE   GPIO_NUM_0   //GPIO-00, module pin 4
#define PIN_ADC_PD        GPIO_NUM_12  //GPIO-12, module pin 6

// Excitation parameters
#define SQUARE_WAVE_HALF_MS  50  // ms
#define N_CYCLES             10  //number of cycles before result

// Detection parameters
#define N_AD_PER_HALF    5   // how many ADC samples per 1/2 cycle

