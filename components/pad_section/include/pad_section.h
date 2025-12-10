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

//enum for sample event type
enum evt_type {
	EVT_PRESS,
	EVT_RELEASE,
	EVT_FINISH
};

typedef struct {
	uint32_t pad_id;
	enum evt_type event_type;
} pad_queue_msg;

// pads queue
extern QueueHandle_t pads_evt_queue;

// pad mode section
// action function type
typedef void (*event_handler)(int pad_id);

typedef struct {
    event_handler on_press;
    event_handler on_release;  
    event_handler on_finish;   // not sure about this
} sample_mode;


// ACTIONS: this might be moved to the sample lib
// these are just placeholders functions.
void action_start_sample(int);
void action_stop_sample(int);
void action_restart_sample(int);
void action_ignore(int);

extern const sample_mode MODE_HOLD;
extern const sample_mode MODE_LOOP;
extern const sample_mode MODE_ONESHOT;
extern const sample_mode MODE_ONESHOT_LOOP;

// this is to pick the mode
extern const sample_mode* SAMPLE_MODES[];

void init();

#endif
