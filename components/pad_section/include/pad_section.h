
/*******************************************************************************
 *                                 PAD SECTION                                 *
 *           This module implements the pad section of the project.            *
 * To test this section, assemble the pad module by connecting each switch pin *
 *       to the corresponding GPIO pin as specified later in this file.        *
 *  Call the `pad_section_init` function to configure the GPIO pins and start  *
 *               the interrupt handling and the associated task.               *
 *               To change the pad mode of a single pad, use the               *
 *       `set_pad_mode(int pad_id, const sample_mode_t *mode)` function,       *
 *   passing one of the predefined modes from the `SAMPLE_MODES[]` constant    *
 *       using the `HOLD`, `LOOP`, `ONESHOT`, or `ONESHOT_LOOP` macros.        *
 *******************************************************************************/

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
//enum for sample event type
enum evt_type_t {
	EVT_PRESS,
	EVT_RELEASE,
	EVT_FINISH
};

typedef struct {
	uint32_t pad_id;
	enum evt_type_t event_type;
} pad_queue_msg_t;

// pads queue
extern QueueHandle_t pads_evt_queue;

// pad mode section
// action function type
typedef void (*event_handler)(int pad_id);

typedef struct {
    event_handler on_press;
    event_handler on_release;  
    event_handler on_finish;   // not sure about this
} sample_mode_t;


// ACTIONS: this might be moved to the sample lib
void action_start_sample(int);
void action_start_or_stop_sample(int);
void action_stop_sample(int);
void action_restart_sample(int);
void action_ignore(int);

// this is to pick the mode

#define HOLD 0
#define ONESHOT 1
#define LOOP 2
#define ONESHOT_LOOP 3
extern const sample_mode_t* SAMPLE_MODES[];

// exposed function to select the pad mode
void set_pad_mode(int, const sample_mode_t*);


void pad_section_init();

#endif
