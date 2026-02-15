/*
********************************* PAD SECTION ***********************************
*   This module implements the pad section of the project.                      *
*   To test this section, assemble the pad module by connecting each switch pin *
*   to the corresponding GPIO pin as specified later in this file.              *
*   Call the `pad_section_init` function to configure the GPIO pins and start   *
*   the interrupt handling and the associated task.                             *
*   To change the pad mode of a single pad, use the                             *
*   `set_pad_mode(int pad_id, const playback_mode_t *mode)` function,             *
*   passing one of the predefined modes from the `PLAYBACK_MODES[]` constant      *
*   using the `HOLD`, `LOOP`, `ONESHOT`, or `ONESHOT_LOOP` macros.              *
*********************************************************************************
*/
#ifndef PAD_SECTION_
#define PAD_SECTION_

#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#define DEBOUNCE_MS 30000

// pins
#define GPIO_BUTTON_1     4
#define GPIO_BUTTON_2     12
#define GPIO_BUTTON_3     13
#define GPIO_BUTTON_4     14
#define GPIO_BUTTON_5     15
#define GPIO_BUTTON_6     2
#define GPIO_BUTTON_7     39
#define GPIO_BUTTON_8     32
#define PAD_NUM 8

#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_BUTTON_1) | \
                             (1ULL<<GPIO_BUTTON_2) | \
                             (1ULL<<GPIO_BUTTON_3) | \
                             (1ULL<<GPIO_BUTTON_4) | \
                             (1ULL<<GPIO_BUTTON_5) | \
                             (1ULL<<GPIO_BUTTON_6) | \
                             (1ULL<<GPIO_BUTTON_7) | \
                             (1ULL<<GPIO_BUTTON_8))

#define PAD_NUM_NOT_DEFINED GPIO_NUM_MAX


// #define NOT_DEFINED SAMPLE_NUM + 1 // to specify when a pad isn't associated with any sample

// typedef struct{
//     uint32_t bank_index; // id of the associated sample
// } pad_settings_t;


uint8_t get_pad_num(int gpio_num);

void pad_section_init();

#endif
