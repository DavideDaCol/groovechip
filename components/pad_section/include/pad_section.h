/*
********************************* PAD SECTION ***********************************
*   This module implements the pad section of the project.                      *
*   To test this section, assemble the pad module by connecting each switch pin *
*   to the corresponding GPIO pin as specified later in this file.              *
*   Call the `pad_section_init` function to configure the GPIO pins and start   *
*   the interrupt handling and the associated task.                             *
*   To change the pad mode of a single pad, use the                             *
*   `set_pad_mode(int pad_id, const sample_mode_t *mode)` function,             *
*   passing one of the predefined modes from the `SAMPLE_MODES[]` constant      *
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

// pins
#define GPIO_BUTTON_1     19
#define GPIO_BUTTON_2     21
#define GPIO_BUTTON_3     22
#define GPIO_BUTTON_4     23
#define GPIO_BUTTON_5     14
#define GPIO_BUTTON_6     27
#define GPIO_BUTTON_7     26
#define GPIO_BUTTON_8     25

#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_BUTTON_1) | \
                             (1ULL<<GPIO_BUTTON_2) | \
                             (1ULL<<GPIO_BUTTON_3) | \
                             (1ULL<<GPIO_BUTTON_4) | \
                             (1ULL<<GPIO_BUTTON_5) | \
                             (1ULL<<GPIO_BUTTON_6) | \
                             (1ULL<<GPIO_BUTTON_7) | \
                             (1ULL<<GPIO_BUTTON_8))


#define NOT_DEFINED SAMPLE_NUM + 1 // to specify when a pad isn't associated with any sample

typedef struct{
    uint32_t sample_id; // id of the associated sample
} pad_settings_t;

extern pad_settings_t pads_config[GPIO_NUM_MAX];

void pad_section_init();

#endif
