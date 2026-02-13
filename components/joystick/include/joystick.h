#ifndef JOYSTICK_H
#define JOYSTICK_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "adc1.h"

typedef enum {
    CENTER,
    UP,
    DOWN,
    LEFT,
    RIGHT,
    PRESS
} joystick_dir_t;

typedef struct {
    int x;
    int y;
    int sw;   // 0 = pressed, 1 = released
} joystick_t;

extern joystick_t my_joystick;

void joystick_init(void);

void joystick_get_raw(void);

void joystick_task(void *args);

#endif // JOYSTICK_H
