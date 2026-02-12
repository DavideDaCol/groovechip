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

#define POT_THRESHOLD 20

extern QueueHandle_t pot_queue;
extern int pot_value;

void potentiometer_init();

int potentiometer_read_raw();

int potentiometer_read_percent();

float potentiometer_read_voltage();

void potentiometer_task(void *args);

#endif