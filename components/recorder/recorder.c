#include "recorder.h"
#include "mixer.h"
#include "pad_section.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "fsm.h"
#include "lcd.h"
#include "sd_reader.h"
#include "effects.h"

const char* TAG_REC = "REC";

recorder_t g_recorder = {0};

int record_number = 0;

void add_to_sample_names(char *new_name);

void recorder_init(void) {
    // set all the parameters
    g_recorder.buffer_capacity = RECORD_BUFFER_SIZE;
    g_recorder.buffer_used = 0;
    g_recorder.state = REC_IDLE;
    g_recorder.target_bank_index = -1;
    
    // logging action
    ESP_LOGI(TAG_REC, "Recorder initialized (buffer: %zu frames, %.1f sec max)",
           RECORD_BUFFER_SIZE / 2,
           (float)RECORD_MAX_DURATION_SEC);
}

void recorder_start_pad_selection(void) {
    // logging action + skip if in wrong state
    if (g_recorder.state == REC_RECORDING) {
        ESP_LOGE(TAG_REC, "Already recording...");
        return;
    }
    
    // print on lcd screen
    // print_single("Select a pad...");
    printf("-----------------------\nSelect a pad\n-----------------------");
    

    // change state
    g_recorder.state = REC_WAITING_PAD;

    // logging action
    ESP_LOGI(TAG_REC, "Click the pad in which you wanna save the new sample...");
}

void recorder_select_pad(int gpio) {
    // logging action + skip if in wrong state
    if (g_recorder.state != REC_WAITING_PAD) {
        ESP_LOGE(TAG_REC, "Tryng to select pad while not in REC_WAITING_PAD");
        return;
    }
    
    // map pad to sample
    g_recorder.target_bank_index = get_sample_bank_index(gpio);
    if (g_recorder.target_bank_index == NOT_DEFINED){
        g_recorder.target_bank_index = get_pad_num(gpio) - 1;
        map_pad_to_sample(gpio, g_recorder.target_bank_index);
    }
    
    // logging action
    ESP_LOGI(TAG_REC, "Sample slot: %d", g_recorder.target_bank_index);

    // start recording
    recorder_start_recording();
}

void recorder_start_recording(void) {
    // logging action + skip if in wrong state
    if (g_recorder.state != REC_WAITING_PAD && g_recorder.state != REC_IDLE) {
        ESP_LOGE(TAG_REC, "State not valid to start recording");
        return;
    }
    
    // allocate space for buffer
    g_recorder.buffer = heap_caps_calloc(1, RECORD_BUFFER_SIZE * sizeof(int16_t), MALLOC_CAP_SPIRAM);

    // logging action + don't start recording if there is no space in PSRAM
    if (!g_recorder.buffer) {
        ESP_LOGE(TAG_REC, "Cannot allocate record buffer");
        return;
    }
    
    // reset fields
    g_recorder.buffer_capacity = RECORD_BUFFER_SIZE;
    g_recorder.buffer_used = 0;
    g_recorder.start_time_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    // print on lcd screen
    // print_single("Recording...");
    printf("-----------------------\nRecording\n------------------------");
    
    
    // change state
    g_recorder.state = REC_RECORDING;
    
    // logging action
    ESP_LOGI(TAG_REC, "Recording started on sample %d", g_recorder.target_bank_index);
}

void recorder_stop_recording(void) {
    // logging action + skip if in wrong state
    if (g_recorder.state != REC_RECORDING) {
        ESP_LOGE(TAG_REC, "Trying to stop a recording that did't start.");
        return;
    }
    
    // change state
    g_recorder.state = REC_IDLE;
    
    // sets some fields
    uint32_t frames_recorded = g_recorder.buffer_used;
    g_recorder.duration_ms = (xTaskGetTickCount() * portTICK_PERIOD_MS) - g_recorder.start_time_ms;
    float duration_sec = (float)frames_recorded / RECORD_SAMPLE_RATE;
    
    // logging action
    ESP_LOGI(TAG_REC, "Recording stopped: %.2f seconds (%lu frames)", duration_sec, frames_recorded);
    
    // put the sample in sample_bank if possible, otherwise log error
    if (g_recorder.target_bank_index >= 0 && g_recorder.target_bank_index < SAMPLE_NUM) {
        sample_t *target = sample_bank[g_recorder.target_bank_index];

        // if not in memory allocate space
        if (target == NULL){

            //logging action
            ESP_LOGI(TAG_REC, "Creating new sample_t for bank %d", g_recorder.target_bank_index);

            target = heap_caps_calloc(1, sizeof(sample_t), MALLOC_CAP_SPIRAM);

            // logging action + free memory + return
            if (target == NULL) {
                ESP_LOGE(TAG_REC, "Cannot allocate sample_t structure");
                if (g_recorder.buffer) {
                    heap_caps_free(g_recorder.buffer);
                    g_recorder.buffer = NULL;
                }
                return;
            }

            // set some default values of the sample_t
            sample_bank[g_recorder.target_bank_index] = target;
        }

        // if there is already a sample bound to that bank_index 
        // and if it's not in flash memory then free it
        if (target->raw_data != NULL) {

            //logging action
            ESP_LOGE(TAG_REC, "Raw data is not NULL");
            
            // free memory
            heap_caps_free((void *)(target->raw_data));
        }
        
        // allocate space for the new sample in PSRAM
        size_t bytes = g_recorder.buffer_used * sizeof(int16_t);
        int16_t *final_buffer = heap_caps_realloc(g_recorder.buffer, bytes, MALLOC_CAP_SPIRAM);
        
        // if final buffer is allocated associate it to the target raw data
        // otherwise it means that there is no free space, so associates 
        // directly the g_recorder.buffer to the target raw data
        if (final_buffer) {
            target -> raw_data = (unsigned char *)final_buffer;
        } else {
            printf("2\n");
            target -> raw_data = (unsigned char *)g_recorder.buffer;
        }

        heap_caps_free(sample_names_bank[g_recorder.target_bank_index]);

        // sets sample name
        sample_names_bank[g_recorder.target_bank_index] = heap_caps_calloc(1, MAX_SIZE, MALLOC_CAP_SPIRAM);
        sprintf(sample_names_bank[g_recorder.target_bank_index], "new_rec%d", record_number++);
        add_to_sample_names(sample_names_bank[g_recorder.target_bank_index]);

        // logging action
        ESP_LOGI(TAG_REC, "Buffer used: %d", g_recorder.buffer_used);


        
        // reset the fields in the recording struct and free memory
        g_recorder.buffer = NULL;
        g_recorder.buffer_used = 0;
        
        
        // logging action
        ESP_LOGI(TAG_REC, "Sample %d updated.", g_recorder.target_bank_index);
    } else {

        // logging action
        ESP_LOGE(TAG_REC, "Wrong bank index in record mode: %d.", g_recorder.target_bank_index);
    }
}

void recorder_cancel(void) {
    // reset all the fields of the recording struct
    g_recorder.state = REC_IDLE;
    g_recorder.buffer_used = 0;
    g_recorder.target_bank_index = -1;

    // logging action
    ESP_LOGI(TAG_REC, "Recording canceled.");
}

void recorder_capture_frame(int16_t sample) {
    // logging action + skip if in wrong state
    if (g_recorder.state != REC_RECORDING) {
        ESP_LOGE(TAG_REC, "Trying to capture frame while not recording.");
        return;
    }
    
    // check free space and stop recording if the buffer is full
    if (g_recorder.buffer_used + 1 > g_recorder.buffer_capacity) {
        ESP_LOGE(TAG_REC, "Buffer full!");
        fsm_queue_msg_t msg = {
            .payload = PRESS,
            .source = JOYSTICK
        };
        recorder_stop_recording();
        xQueueSend(fsm_queue, &msg, 0);
        return;
    }
    
    g_recorder.buffer[g_recorder.buffer_used++] = sample;
}

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

void recorder_fsm(){
    // start recording

    // logging action
    ESP_LOGI(TAG_REC, "Recording state (expected 0 = IDLE/NOT STARTED): %d", g_recorder.state);

    // logging action + skip if in wrong state
    if (g_recorder.state != REC_IDLE){
        ESP_LOGE(TAG_REC, "Trying to record while not in REC_IDLE state.");
        return;
    }

    // start pad selection
    recorder_start_pad_selection();

    // logging action
    ESP_LOGI(TAG_REC, "Recording state (expected 1 = WAITING FOR PAD): %d", g_recorder.state);

    // wait for signal by the pressed button (which is the chosen pad)
    fsm_queue_msg_t msg;
    while(xQueueReceive(fsm_queue, &msg, portMAX_DELAY) && msg.source != PAD);

    // select pad
    recorder_select_pad(msg.payload);

    // logging action
    ESP_LOGI(TAG_REC, "Recording state (expected 2 = RECORDING): %d", g_recorder.state);

    // recording...

    // wait for signal by the recording button to stop recording
    while(xQueueReceive(fsm_queue, &msg, portMAX_DELAY) && msg.source != JOYSTICK && msg.payload != PRESS);

    // stop recording
    if (g_recorder.state != REC_IDLE){
        recorder_stop_recording();
    }

    // logging action
    ESP_LOGI(TAG_REC, "Recording state (expected 0 = IDLE/FINISHED): %d", g_recorder.state);
}

void add_to_sample_names(char *new_name) {
    sample_names = heap_caps_realloc(sample_names, (++sample_names_size)*sizeof(char*), MALLOC_CAP_SPIRAM);
    sample_names[sample_names_size - 1] = new_name;

    for (int i = 0; i < sample_names_size; i++) {
        printf("%s\n", sample_names[i]);
    }
}