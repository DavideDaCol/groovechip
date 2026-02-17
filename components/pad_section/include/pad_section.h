#ifndef PAD_SECTION_
#define PAD_SECTION_

#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#define DEBOUNCE_MS_PAD 30000

// pin associated to buttons
#define GPIO_BUTTON_1 (uint8_t)14
#define GPIO_BUTTON_2 (uint8_t)12
#define GPIO_BUTTON_3 (uint8_t)2
#define GPIO_BUTTON_4 (uint8_t)4
#define GPIO_BUTTON_5 (uint8_t)32
#define GPIO_BUTTON_6 (uint8_t)13
#define GPIO_BUTTON_7 (uint8_t)15
#define GPIO_BUTTON_8 (uint8_t)27
#define PAD_NUM 8

// bitmask to set buttons
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_BUTTON_1) | \
                             (1ULL<<GPIO_BUTTON_2) | \
                             (1ULL<<GPIO_BUTTON_3) | \
                             (1ULL<<GPIO_BUTTON_4) | \
                             (1ULL<<GPIO_BUTTON_5) | \
                             (1ULL<<GPIO_BUTTON_6) | \
                             (1ULL<<GPIO_BUTTON_7) | \
                             (1ULL<<GPIO_BUTTON_8))

#define PAD_NUM_NOT_DEFINED GPIO_NUM_MAX

/*
@brief get the pad_number (1-8) from its gpio number.
@pad gpio_num gpio number of the pad. 
*/
uint8_t get_pad_num(int gpio_num);

void pad_section_init();

#endif
