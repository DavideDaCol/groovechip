#include "include/adc1.h"

adc_oneshot_unit_handle_t adc1_handle;

void adc1_init(){
    // initialize ADC unit 1
    adc_oneshot_unit_init_cfg_t init_config = {.unit_id = ADC_UNIT_1};
    adc_oneshot_new_unit(&init_config, &adc1_handle);
}