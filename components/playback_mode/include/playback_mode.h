#ifndef PLAYBACK_MODE_H_
#define PLAYBACK_MODE_H_
#define NOT_DEFINED GPIO_NUM_MAX

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#pragma region PLAYBACK EVENT QUEUE 
// pad mode section
// action function type
typedef void (*event_handler)(int pad_id);

// sample event queue
extern QueueHandle_t playback_evt_queue;

// the playback event can be sent from pad_section or from the mixer (ON_FINISH event).
// This enum distinguish this two cases
typedef enum{
	SRC_PAD_SECTION,
	SRC_MIXER
} event_source_t;

// enum for sample event type
enum evt_type_t {
	EVT_PRESS,
	EVT_RELEASE,
	EVT_FINISH
};

// associate the pad_id to the event_type 
typedef struct {
	event_source_t source;
	enum evt_type_t event_type;

	union{
		uint8_t pad_id;
		uint8_t bank_index;
	} payload;

} playback_msg_t;

void send_pad_event(uint8_t pad_id, enum evt_type_t event_type);
void send_mixer_event(uint8_t bank_index, enum evt_type_t event_type);

void map_pad_to_sample(uint8_t pad_id, uint8_t bank_index);
uint8_t get_sample_bank_index(uint8_t pad_id);

#pragma endregion

#pragma region PLAYBACK MODE
typedef enum{
	HOLD,
	ONESHOT,
	LOOP,
	ONESHOT_LOOP,
	UNSET
} pb_mode_t;

typedef struct {
	pb_mode_t mode;
    event_handler on_press;
    event_handler on_release;  
    event_handler on_finish;
} playback_mode_t;


void get_mode_stringify(pb_mode_t, char*);

pb_mode_t get_playback_mode(uint8_t);

// exposed function to select the pad mode
void set_playback_mode(uint8_t, const pb_mode_t);

#pragma endregion

void playback_mode_init();
#endif