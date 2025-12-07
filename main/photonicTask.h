#ifndef PHOTONIC_TASK_H //prevent double includes
#define PHOTONIC_TASK_H

#include "esp_log.h"
#include "unistd.h"

// declarations
esp_err_t init_photonics(void);  // initialize photonic_task
void photonic_task(void*);     // generate Ex signal and collect data
unsigned long int collect_PD_ADC(int);       // get an ADC reading from the PD amp.
unsigned long int photonic_test(void);       // test method for ADC

// pin assignments for photonics

// LED output
#define PIN_EXCIT_DRIVE   GPIO_NUM_0   //GPIO-00, module pin 4

// ADC parameters:

// The options for attenuation
// ADC_ATTEN_DB_0      // 0 dB attenuation, range: 0 - ~750 mV
// ADC_ATTEN_DB_2_5    // 2.5 dB attenuation, range: 0 - ~1050 mV
// ADC_ATTEN_DB_6      // 6 dB attenuation, range: 0 - ~1300 mV
// ADC_ATTEN_DB_12     // 12 dB attenuation, range: 0 - ~3100 mV

#define TPT_PIN_ADC_PD    GPIO_NUM_2  //GPIO-2, module pin 6
#define ADC_CHANNEL       ADC_CHANNEL_2   // ADC channel for GPIO2
#define TPT_ADC_ATTEN     ADC_ATTEN_DB_12  // 0-3.1V range (adjust as needed)

// Excitation parameters
#define SQUARE_WAVE_HALF_MS  50 // ms
#define N_CYCLES             3  //number of cycles before result

// Detection parameters
#define N_AD_PER_HALF         2  // how many ADC samples per 1/2 cycle

#endif  // prevent double includes
