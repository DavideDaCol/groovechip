#include "recorder.h"
#include "mixer.h"
#include "pad_section.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

recorder_t g_recorder = {0};

void recorder_init(void) {
    g_recorder.buffer_capacity = RECORD_BUFFER_SIZE;
    g_recorder.buffer_used = 0;
    g_recorder.state = REC_IDLE;
    g_recorder.target_pad = -1;
    g_recorder.target_sample_id = -1;
    
    printf("[DEBUG PRINT] Recorder initialized (buffer: %zu frames, %.1f sec max)\n",
           RECORD_BUFFER_SIZE,
           (float)RECORD_MAX_DURATION_SEC);
}

// TODO serve solo per essere sicuri del flow di azioni (se tutto funziona bene si può togliere)
void recorder_start_pad_selection(void) {
    if (g_recorder.state == REC_RECORDING) {
        printf("Error: Already recording...\n");
        return;
    }
    
    g_recorder.state = REC_WAITING_PAD;
    printf("[DEBUG PRINT] Click the pad in which you wanna save the new sample...\n");
}

void recorder_select_pad(int pad_id, int bank_index) {
    // TODO rimuovi se hai tolto lo step precedente (sostituisci con state != IDLE)
    if (g_recorder.state != REC_WAITING_PAD) {
        return;
    }
    
    g_recorder.target_pad = pad_id;
    g_recorder.target_sample_id = bank_index;
    
    printf("[DEBUG PRINT] Sample slot: %d\n", bank_index);
    
    // Start recording
    recorder_start_recording();
}

void recorder_start_recording(void) {
    if (g_recorder.state != REC_WAITING_PAD && g_recorder.state != REC_IDLE) {
        printf("Error: state not valid to start recording\n");
        return;
    }
    
    // Allocate space for buffer and reset fields
    g_recorder.buffer = malloc(RECORD_BUFFER_SIZE * sizeof(int16_t));
    if (!g_recorder.buffer) {
        printf("ERROR: cannot allocate record buffer\n");
        return;
    }

    g_recorder.buffer_capacity = RECORD_BUFFER_SIZE;
    g_recorder.buffer_used = 0;
    g_recorder.start_time_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    g_recorder.state = REC_RECORDING;
    
    printf("[DEBUG PRINT] Recording started on pad %d (sample %d)\n",
           g_recorder.target_pad,
           g_recorder.target_sample_id);
}

void recorder_stop_recording(void) {
    if (g_recorder.state != REC_RECORDING) {
        printf("Error: No active recording\n");
        return;
    }
    
    g_recorder.duration_ms = (xTaskGetTickCount() * portTICK_PERIOD_MS) - g_recorder.start_time_ms;
    g_recorder.state = REC_RECORDED;
    
    uint32_t frames_recorded = g_recorder.buffer_used;
    float duration_sec = (float)frames_recorded / RECORD_SAMPLE_RATE;
    
    printf("[DEBUG PRINT] Recording stopped: %.2f seconds (%lu frames)\n", duration_sec, frames_recorded);
    
    // Put the sample in sample_bank
    if (g_recorder.target_sample_id >= 0 && g_recorder.target_sample_id < SAMPLE_NUM) {
        sample_t *target = sample_bank[g_recorder.target_sample_id];

        // If there is already a sample bound to that bank_index 
        // and if it's not in flash memory then free it
        if (target->raw_data && !is_flash_ptr((void *)(target->raw_data))) {
            free((void *)(target->raw_data));
        }
        
        size_t bytes = g_recorder.buffer_used * sizeof(int16_t);
        int16_t *final_buffer = realloc(g_recorder.buffer, bytes);

        if (final_buffer) {
            target -> raw_data = (unsigned char *)final_buffer;
        } else {
            target -> raw_data = (unsigned char *)g_recorder.buffer;
        }

        // Manually create the WAV header
        target->header.sample_rate = RECORD_SAMPLE_RATE;
        target->header.num_channels = 1;
        target->header.bits_per_sample = 16;
        target->header.data_size = g_recorder.buffer_used * sizeof(int16_t);
        
        // Reset playback state
        target->playback_ptr = 0;
        target->playback_finished = false;

        // Reset the fields in the recording struct and free memory
        g_recorder.buffer = NULL;
        g_recorder.buffer_used = 0;
        
        printf("[DEBUG PRINT] Sample %d updated on pad %d.\n", g_recorder.target_sample_id, g_recorder.target_pad);
    }
}

void recorder_cancel(void) {
    g_recorder.state = REC_IDLE;
    g_recorder.buffer_used = 0;
    g_recorder.target_pad = -1;
    g_recorder.target_sample_id = -1;
    printf("[DEBUG PRINT] Recording canceled\n");
}

void recorder_capture_frame(int16_t sample) {
    if (g_recorder.state != REC_RECORDING) {
        return;
    }
    
    // Check free space
    if (g_recorder.buffer_used + 1 > g_recorder.buffer_capacity) {
        printf("Error: Buffer full!\n");
        recorder_stop_recording();
        return;
    }
    
    // Save the frame
    g_recorder.buffer[g_recorder.buffer_used++] = sample;
}

// Getters

recorder_state_t recorder_get_state(void) {
    return g_recorder.state;
}

bool recorder_is_recording(void) {
    return g_recorder.state == REC_RECORDING;
}

float recorder_get_duration_sec(void) {
    if (g_recorder.state == REC_RECORDING) {
        uint32_t elapsed_ms = (xTaskGetTickCount() * portTICK_PERIOD_MS) - g_recorder.start_time_ms;
        return (float)elapsed_ms / 1000.0f;
    } else if (g_recorder.state == REC_RECORDED) {
        return (float)g_recorder.duration_ms / 1000.0f;
    }
    return 0.0f;
}

bool is_flash_ptr(void *p) {
    return ((uintptr_t)p) >= 0x3F400000; // ESP32 FLASH region
}

// TODO maybe in pad_section or in mixer
bool rec_mode = false;

void on_rec_button_press() {
    if (recorder_is_recording()) {
        recorder_stop_recording();
        rec_mode = false;
        printf("[DEBUG PRINT] Rec mode OFF");
    } else {
        rec_mode = !rec_mode;
        if (rec_mode) {
            recorder_start_pad_selection();
            printf("[DEBUG PRINT] Rec mode ON\n");
        }
    }
}

void on_pad_press(int gpio_num) {
    int bank_index = -1;
    
    // TODO non so come sono collegati bank_index e gpio_num
    switch(gpio_num) {
        case GPIO_BUTTON_1: bank_index = 0; break;
        case GPIO_BUTTON_2: bank_index = 1; break;
        case GPIO_BUTTON_3: bank_index = 2; break;
        case GPIO_BUTTON_4: bank_index = 3; break;
        case GPIO_BUTTON_5: bank_index = 4; break;
        case GPIO_BUTTON_6: bank_index = 5; break;
        case GPIO_BUTTON_7: bank_index = 6; break;
        case GPIO_BUTTON_8: bank_index = 7; break;
        default: return;
    }
    
    // if rec mode is on but recorder is not recording then start recording
    if (rec_mode && !recorder_is_recording()) {
        recorder_select_pad(gpio_num, bank_index);
        return;
    }
    
    // otherwise 
    action_start_or_stop_sample(bank_index);
}