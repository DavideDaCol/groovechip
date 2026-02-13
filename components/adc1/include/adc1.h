#ifndef ADC_1
#define ADC_1

#include "esp_adc/adc_oneshot.h"

extern adc_oneshot_unit_handle_t adc1_handle;

void adc1_init();

#endif