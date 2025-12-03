#ifndef PAD_SECTION_
#define PAD_SECTION_

#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"

// pins
#define GPIO_BUTTON_1     22
#define GPIO_BUTTON_2     23
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_BUTTON_1) | (1ULL<<GPIO_BUTTON_2))

// pads queue
static QueueHandle_t btn_evt_queue = NULL;

// debounce + detecting press/release
static volatile uint64_t last_isr_time[GPIO_NUM_MAX] = {0};
static volatile int last_level[GPIO_NUM_MAX] = {1};

typedef struct {

  uint32_t gpio_num;
  char payload[32];

}btn_queue_msg;

void init();

#endif
