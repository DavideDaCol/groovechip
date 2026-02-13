#ifndef POTENTIOMETER_H
#define POTENTIOMETER_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"
#include "esp_mac.h"
#include "freertos/queue.h"
#include "joystick.h"
#include "adc1.h"

extern int pot_value;

#define POT_CHANNEL ADC_CHANNEL_0
#define POT_READ_INTERVAL_MS 50        
#define THRESHOLD_PERCENT 4            
#define FILTER_SAMPLES 16

void potentiometer_init();

int potentiometer_read_filtered();

void potentiometer_task(void *args);

#endif