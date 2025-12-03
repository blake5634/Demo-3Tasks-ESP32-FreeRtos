#include "esp_log.h"
#include "unistd.h"

// pin assignments for photonics
#define PIN_EXCIT_DRIVE   0
#define PIN_ADC_PD        0

// Excitation parameters
#define SQUARE_WAVE_HALF_MS  50  // ms
#define N_CYCLES          10  //number of cycles before result

// Detection parameters
#define N_AD_PER_HALF    5   // how many ADC samples per 1/2 cycle

