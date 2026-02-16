#ifndef RECORDER_H_
#define RECORDER_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "fsm.h"
#include "lcd.h"

// sample rate in kHz
#define RECORD_SAMPLE_RATE 16000

// max duration of a sample in seconds
#define RECORD_MAX_DURATION_SEC 5

// max number of frames that can be sampled
#define RECORD_MAX_FRAMES (RECORD_SAMPLE_RATE * RECORD_MAX_DURATION_SEC)

// size of the new sample buffer (differs between stereo and mono)
#define RECORD_BUFFER_SIZE (RECORD_MAX_FRAMES * 2)  // 2 for stereo

// recorder states
typedef enum {
    REC_IDLE,              // idle state = not in the recorder fsm (before/after recording)
    REC_WAITING_PAD,       // waiting for the user to choose the pad
    REC_RECORDING,         // currently recording
} recorder_state_t;

// recorder struct
typedef struct {
    recorder_state_t state;     // state of the fsm
    
    int16_t *buffer;            // pointer to the recorder buffer
    size_t buffer_capacity;     // max capacity of the buffer
    size_t buffer_used;         // size of the buffer is used
    
    int target_bank_index;      // target bank index for the new sample
    
    uint32_t start_time_ms;     // timestamp of the start (for debug purpose)
    uint32_t duration_ms;       // duration of the sample (for debug purpose)
    
} recorder_t;

// actual recorder
extern recorder_t g_recorder;

/*
@brief function that initializes the recorder struct.
*/
void recorder_init(void);

/*
@brief function that switches the state of the recorder to REC_WAITING_PAD.
*/
void recorder_start_pad_selection(void);

/*
@brief function that sets the bank index of the recorder based on the gpio, and calls recorder_start_recording().
@param gpio the gpio of the button pressed.
*/
void recorder_select_pad(int gpio);

/*
@brief function that sets the recording state to REC_RECORDING, 
resets the useful recording parameters and sets the timestamp.
*/
void recorder_start_recording(void);

/*
@brief 
function that stops the recording of the actual record buffer by 
setting the recording state to IDLE, resetting all the useful recording
parameters and saving the recorded sample in PSRAM 
(freeing the previous sample PSRAM space if necessary).
*/
void recorder_stop_recording(void);

/*
@brief function that resets all the recording struct parameters.
*/
void recorder_cancel(void);

/*
@brief function that checks if a pointer points to esp32-e flash memory.
@param p pointer whose memory location has to be checked. 
*/
bool is_flash_ptr(void *p);

/*
@brief function that sets the new frame of the recording buffer using the
buffer used parameter of the recording struct.
@param frame frame from the master buffer that has to be copied into the
recording buffer.
*/
void recorder_capture_frame(int16_t left, int16_t right);

/*
@brief getter function for the recording state.
*/
recorder_state_t recorder_get_state(void);

/*
@brief function that tells if the recorder is recording.
*/
bool recorder_is_recording(void);

/*
@brief function that returns the duration in seconds of the sampel recorded.
*/
float recorder_get_duration_sec(void);

/*
@brief function that acts as a small final state machine for the recorder mode,
it handles all messages from the fsm_queue in charge from the main fsm and
manages the flow of the changing recording state.
*/
void recorder_fsm(void);

#endif