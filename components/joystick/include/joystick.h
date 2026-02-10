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
} JoystickDir;

typedef struct {
    int x;
    int y;
    int sw;   // 0 = pressed, 1 = released
} Joystick;

extern Joystick my_joystick;

extern QueueHandle_t joystick_queue;

void joystick_init(void);

void joystick_get_raw(void);

void joystick_task(void *args);

#endif // JOYSTICK_H
