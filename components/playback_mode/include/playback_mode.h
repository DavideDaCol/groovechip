#ifndef PLAYBACK_MODE_H_
#define PLAYBACK_MODE_H_
#define NOT_DEFINED GPIO_NUM_MAX

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#pragma region PLAYBACK EVENT QUEUE 

//type of press/release/finish handlers
typedef void (*event_handler)(int pad_id);

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

/*
@brief send pad message to the playback queue, setting the source to `SRC_PAD_SECTION`.
@param pad_id pad_id the pad_id of the pad that send the message.
@param event_type specify te `event_type` (`EVT_PRESS`/`RELEASE`)
*/
void send_pad_event(uint8_t pad_id, enum evt_type_t event_type);

/*
@brief send pad message to the playback queue, setting the source to `SRC_MIXER`.
@param pad_id bank_index the bank_index of the sample that send the message.
@param event_type specify te `event_type` (`EVT_FINISH`)
*/
void send_mixer_event(uint8_t bank_index, enum evt_type_t event_type);

void map_pad_to_sample(uint8_t pad_id, uint8_t bank_index);
uint8_t get_sample_bank_index(uint8_t pad_id);

#pragma endregion

#pragma region PLAYBACK MODE

// available playback modes
typedef enum{
	HOLD,
	ONESHOT,
	LOOP,
	ONESHOT_LOOP,
	UNSET
} pb_mode_t;

// playback mode structure
typedef struct {
	pb_mode_t mode;
    event_handler on_press;
    event_handler on_release;  
    event_handler on_finish;
} playback_mode_t;

/*
@brief get the string associated to a mode.
@param mode the playback mode.
@param out output string.
*/
void get_mode_stringify(pb_mode_t mode, char* out);

/*
@brief get playback mode of a sample.
@param bank_index index of the sample.
*/
pb_mode_t get_playback_mode(uint8_t bank_index);

/*
@brief set the playback mode of a sample.
@param bank_index index of the sample.
@param mode new mode of the sample.
*/
void set_playback_mode(uint8_t bank_index, const pb_mode_t mode);

#pragma endregion

/*
@brief playback mode component init function.
*/
void playback_mode_init();
#endif