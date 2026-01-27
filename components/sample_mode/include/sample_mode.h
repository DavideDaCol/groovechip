#ifndef SAMPLE_MODE_H_
#define SAMPLE_MODE_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// pad mode section
// action function type
typedef void (*event_handler)(int pad_id);

// sample event queue
extern QueueHandle_t sample_evt_queue;

//enum for sample event type
enum evt_type_t {
	EVT_PRESS,
	EVT_RELEASE,
	EVT_FINISH
};

void send_event(uint8_t sample_id, enum evt_type_t);

typedef struct {
    event_handler on_press;
    event_handler on_release;  
    event_handler on_finish;
} sample_mode_t;

typedef struct {
	uint8_t sample_id;
	enum evt_type_t event_type;
} sample_queue_msg_t;

// this is to pick the mode

#define HOLD 0
#define ONESHOT 1
#define LOOP 2
#define ONESHOT_LOOP 3
extern const sample_mode_t* SAMPLE_MODES[];

// exposed function to select the pad mode
void set_sample_mode(int, const sample_mode_t*);
void sample_mode_init();
#endif