#ifndef PHOTONIC_TASK_H //prevent double includes
#define PHOTONIC_TASK_H

#include "esp_log.h"
#include "unistd.h"

// declarations
esp_err_t init_photonics(void);
void photonic_task(void*);
int collect_PD_ADC(int);

// pin assignments for photonics

// LED output
#define PIN_EXCIT_DRIVE   GPIO_NUM_0   //GPIO-00, module pin 4
// ADC parameters:
#define TPT_PIN_ADC_PD    GPIO_NUM_2  //GPIO-2, module pin 6
#define ADC_CHANNEL       ADC_CHANNEL_2   // ADC channel for GPIO2
#define TPT_ADC_ATTEN     ADC_ATTEN_DB_12  // 0-3.1V range (adjust as needed)

// Excitation parameters
#define SQUARE_WAVE_HALF_MS  250  // ms
#define N_CYCLES             10  //number of cycles before result

// Detection parameters
#define N_AD_PER_HALF    5   // how many ADC samples per 1/2 cycle

#endif  // prevent double includes
