#include "recorder.h"
#include "mixer.h"
#include "pad_section.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

const char* TAG_REC = "REC";

recorder_t g_recorder = {0};

void recorder_init(void) {
    g_recorder.buffer_capacity = RECORD_BUFFER_SIZE;
    g_recorder.buffer_used = 0;
    g_recorder.state = REC_IDLE;
    g_recorder.target_bank_index = -1;
    
    ESP_LOGI(TAG_REC, "Recorder initialized (buffer: %zu frames, %.1f sec max)",
           RECORD_BUFFER_SIZE / 2,
           (float)RECORD_MAX_DURATION_SEC);
}

void recorder_start_pad_selection(void) {
    if (g_recorder.state == REC_RECORDING) {
        ESP_LOGE(TAG_REC, "Already recording...");
        return;
    }
    
    print_single("Select a pad...");
    g_recorder.state = REC_WAITING_PAD;
    ESP_LOGI(TAG_REC, "Click the pad in which you wanna save the new sample...");
}

void recorder_select_pad(int gpio) {
    if (g_recorder.state != REC_WAITING_PAD) {
        return;
    }
    
    // map pad to sample
    g_recorder.target_bank_index = get_sample_bank_index(gpio);
    if (g_recorder.target_bank_index == NOT_DEFINED){
        g_recorder.target_bank_index = get_pad_num(gpio) - 1;
        map_pad_to_sample(gpio, g_recorder.target_bank_index);
    }
    
    ESP_LOGI(TAG_REC, "Sample slot: %d", g_recorder.target_bank_index);

    // start recording
    recorder_start_recording();
}

void recorder_start_recording(void) {
    if (g_recorder.state != REC_WAITING_PAD && g_recorder.state != REC_IDLE) {
        ESP_LOGE(TAG_REC, "State not valid to start recording");
        return;
    }
    
    // Allocate space for buffer and reset fields
    g_recorder.buffer = heap_caps_malloc(RECORD_BUFFER_SIZE * sizeof(int16_t), MALLOC_CAP_SPIRAM);
    if (!g_recorder.buffer) {
        ESP_LOGE(TAG_REC, "Cannot allocate record buffer");
        return;
    }

    g_recorder.buffer_capacity = RECORD_BUFFER_SIZE;
    g_recorder.buffer_used = 0;
    g_recorder.start_time_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    print_single("Recording...");
    g_recorder.state = REC_RECORDING;
    
    ESP_LOGI(TAG_REC, "Recording started on sample %d", g_recorder.target_bank_index);
}

void recorder_stop_recording(void) {
    if (g_recorder.state != REC_RECORDING) {
        printf("Error: No active recording\n");
        return;
    }
    
    g_recorder.duration_ms = (xTaskGetTickCount() * portTICK_PERIOD_MS) - g_recorder.start_time_ms;
    g_recorder.state = REC_IDLE;
    
    uint32_t frames_recorded = g_recorder.buffer_used / 2;
    float duration_sec = (float)frames_recorded / RECORD_SAMPLE_RATE;
    
    ESP_LOGI(TAG_REC, "Recording stopped: %.2f seconds (%lu frames)", duration_sec, frames_recorded);
    
    // Put the sample in sample_bank
    if (g_recorder.target_bank_index >= 0 && g_recorder.target_bank_index < SAMPLE_NUM) {
        sample_t *target = sample_bank[g_recorder.target_bank_index];

        if (target == NULL){
            ESP_LOGI(TAG_REC, "Creating new sample_t for bank %d", g_recorder.target_bank_index);
            target = heap_caps_malloc(sizeof(sample_t), MALLOC_CAP_SPIRAM);
            if (target == NULL) {
                ESP_LOGE(TAG_REC, "Cannot allocate sample_t structure");
                if (g_recorder.buffer) {
                    heap_caps_free(g_recorder.buffer);
                    g_recorder.buffer = NULL;
                }
                return;
            }

            // memset(target, 0, sizeof(sample_t));
            sample_bank[g_recorder.target_bank_index] = target;
            target->bank_index = g_recorder.target_bank_index;
            target->volume = 0.1f;
        }

        // If there is already a sample bound to that bank_index 
        // and if it's not in flash memory then free it
        if (target->raw_data != NULL) {
            printf("%hhn\n", target -> raw_data);
            heap_caps_free((void *)(target->raw_data));
        }
        
        size_t bytes = g_recorder.buffer_used * sizeof(int16_t);
        int16_t *final_buffer = heap_caps_realloc(g_recorder.buffer, bytes, MALLOC_CAP_SPIRAM);
        
        if (final_buffer) {
            target -> raw_data = (unsigned char *)final_buffer;
        } else {
            target -> raw_data = (unsigned char *)g_recorder.buffer;
        }

        ESP_LOGI(TAG_REC, "Buffer used: %d", g_recorder.buffer_used);
        
        // Manually create the WAV header
        target->header.sample_rate = RECORD_SAMPLE_RATE;
        target->header.num_channels = 2;
        target->header.bits_per_sample = 16;
        target->header.data_size = g_recorder.buffer_used * sizeof(int16_t);
        
        // Reset playback state
        target->total_frames = g_recorder.buffer_used / 2;
        target->start_ptr = 0.0f;
        target->end_ptr = (float)target->total_frames - 1.0f;
        target->playback_ptr = 0.0f;
        target->playback_finished = false;

        // Reset the fields in the recording struct and free memory
        g_recorder.buffer = NULL;
        g_recorder.buffer_used = 0;
        
        ESP_LOGI(TAG_REC, "Sample %d updated.", g_recorder.target_bank_index);
    } else {
        ESP_LOGE(TAG_REC, "Wrong bank index in record mode: %d.", g_recorder.target_bank_index);
    }
}

void recorder_cancel(void) {
    g_recorder.state = REC_IDLE;
    g_recorder.buffer_used = 0;
    g_recorder.target_bank_index = -1;
    ESP_LOGI(TAG_REC, "Recording canceled.");
}

void recorder_capture_frame(int16_t left, int16_t right) {
    if (g_recorder.state != REC_RECORDING) {
        return;
    }
    
    // Check free space
    if (g_recorder.buffer_used + 2 > g_recorder.buffer_capacity) {
        ESP_LOGE(TAG_REC, "Buffer full!");
        fsm_queue_msg_t msg = {
            .payload = PRESS,
            .source = JOYSTICK
        };
        xQueueSend(fsm_queue, &msg, 0);
        return;
    }
    
    // Save the stereo frame
    g_recorder.buffer[g_recorder.buffer_used++] = left;
    g_recorder.buffer[g_recorder.buffer_used++] = right;
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
    } else if (g_recorder.state == REC_IDLE) {
        return (float)g_recorder.duration_ms / 1000.0f;
    }
    return 0.0f;
}

bool is_flash_ptr(void *p) {
    return ((uintptr_t)p) >= 0x3F400000; // ESP32 FLASH region
}

void recorder_fsm(){
    // start recording
    ESP_LOGI(TAG_REC, "Recording state (expected 0 = IDLE/NOT STARTED): %d", g_recorder.state);

    if (g_recorder.state != REC_IDLE){
        ESP_LOGE(TAG_REC, "Trying to record while not in REC_IDLE state.");
        return;
    }
    recorder_start_pad_selection();

    ESP_LOGI(TAG_REC, "Recording state (expected 1 = WAITING FOR PAD): %d", g_recorder.state);

    // wait for signal by the pressed button (which is the chosen pad)
    fsm_queue_msg_t msg;
    while(xQueueReceive(fsm_queue, &msg, portMAX_DELAY) && msg.source != PAD);
    recorder_select_pad(msg.payload);

    ESP_LOGI(TAG_REC, "Recording state (expected 2 = RECORDING): %d", g_recorder.state);

    // recording...

    // wait for signal by the recording button to stop recording

    while(xQueueReceive(fsm_queue, &msg, portMAX_DELAY) && msg.source != PAD && msg.payload != PRESS);
    if (g_recorder.state != REC_RECORDING){
        ESP_LOGE(TAG_REC, "Trying to stop recording while not in REC_RECORDING state.");
        return;
    }
    recorder_stop_recording();
    ESP_LOGI(TAG_REC, "Recording state (expected 0 = IDLE/FINISHED): %d", g_recorder.state);
}