#ifndef RECORDER_H_
#define RECORDER_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "fsm.h"
#include "lcd.h"

#define RECORD_SAMPLE_RATE 16000
#define RECORD_MAX_DURATION_SEC 3  // TODO establish the max record duration (10 sec at the moment)
#define RECORD_MAX_FRAMES (RECORD_SAMPLE_RATE * RECORD_MAX_DURATION_SEC)
#define RECORD_BUFFER_SIZE (RECORD_MAX_FRAMES * 2)  // Stereo

// Recorder states // TODO replace with the real fsm or not?
typedef enum {
    REC_IDLE,              // Idle state
    REC_WAITING_PAD,       // Waiting for the user to choose the pad
    REC_RECORDING,         // Currently recording
} recorder_state_t;

// Recorder struct
typedef struct {
    recorder_state_t state;
    
    // Pointer to the recorder buffer
    int16_t *buffer;
    size_t buffer_capacity;  // Max capacity of the buffer
    size_t buffer_used;      // How much of the buffer is used
    
    // Target pad for the new sample
    int target_bank_index;
    
    // Timestamp // TODO remove when is no more needed for debugging
    uint32_t start_time_ms;
    uint32_t duration_ms;
    
} recorder_t;

extern recorder_t g_recorder;

void recorder_init(void);
void recorder_start_pad_selection(void);
void recorder_select_pad(int bank_index);
void recorder_start_recording(void);
void recorder_stop_recording(void);
void recorder_cancel(void);
bool is_flash_ptr(void *p);
// bool recorder_save_to_sd(const char *filename); // TODO use the SD defined function

void recorder_capture_frame(int16_t left, int16_t right);

recorder_state_t recorder_get_state(void);
bool recorder_is_recording(void);
float recorder_get_duration_sec(void);

void recorder_fsm();

#endif