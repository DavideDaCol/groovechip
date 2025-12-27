#include "mixer.h"
#include <stdint.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "sdkconfig.h"
#include "dummy_wav.h"

//currently active samples
sample_bitmask now_playing;

// all samples that can be played
sample_t sample_bank[SAMPLE_NUM];

#pragma region SAMPLE_ACTION

// these are just placeholders functions.
void action_start_or_stop_sample(int sample_id){
	//either stop or play the sample
    now_playing ^= (1 << sample_id);
    //reset the playback pointer if the sample was stopped
    if ((now_playing & (1 << sample_id)) != 0 ){
        sample_bank[sample_id].playback_ptr = 0;
    }
}

void action_start_sample(int sample_id){
	//add the sample from the nowplaying bitmask
    now_playing |= (1 << sample_id);
}

void action_stop_sample(int sample_id){
	//remove the sample from the nowplaying bitmask
    now_playing &= ~(1 << sample_id);
    //reset the playback pointer
    sample_bank[sample_id].playback_ptr = 0;
}

void action_restart_sample(int sample_id){
	//reset the playback pointer
    sample_bank[sample_id].playback_ptr = 0;
}

void action_ignore(int pad_id){
	// nothing
}

#pragma endregion

static void mixer_task_wip(void *args)
{
    i2s_chan_handle_t out_channel = (i2s_chan_handle_t)args;
    assert(out_channel);

    //initialize the sample bank
    for (int j = 0; j < SAMPLE_NUM; j++){
        sample_bank[j].sample_id = j;
        sample_bank[j].pad_id = 19;
        sample_bank[j].playback_ptr = 0;
    }

    //initialize the first sample for testing purposes
    memcpy(&sample_bank[0].header, WavData, WAV_HDR_SIZE);
    sample_bank[0].raw_data = WavData + WAV_HDR_SIZE;
    sample_bank[0].pad_id = 23;
    sample_bank[0].playback_mode.on_finish = action_stop_sample;
    sample_bank[0].playback_mode.on_release = action_ignore;
    sample_bank[0].playback_mode.on_press = action_restart_sample;

    size_t w_bytes = BUFF_SIZE;

    // create a i2s DMA buffer to write samples without stressing the CPU.
    // Has to be double the chosen size to allow stereo playback
    int16_t *master_buf = malloc(BUFF_SIZE * 2 * sizeof(int16_t));
    assert(master_buf);

    ESP_ERROR_CHECK(i2s_channel_enable(out_channel));

    // quick and dirty volume control. 
    // WARNING: the stereo out is usually very loud, this value shoud stay below 0.1
    float volume = 0.1f; 

     while (1) {
        for (int i = 0; i < BUFF_SIZE; i++) {

            //fill the buffer with 0 in case no samples are playing
            master_buf[i * 2] = 0x00;
            master_buf[i * 2 + 1] = 0x00;

            //look at all playing samples
            for (int j = 0; j < SAMPLE_NUM; j++){

                //check play status via bit masking
                if ((now_playing & (1 << j)) != 0 ){

                    // in WAV files, left and right samples are sequential
                    int16_t left  = *(int16_t*)(sample_bank[j].raw_data + sample_bank[j].playback_ptr);
                    int16_t right = *(int16_t*)(sample_bank[j].raw_data + sample_bank[j].playback_ptr + 2);

                    left  *= volume;
                    right *= volume;

                    // writes the WAV data to the buffer post volume adjustment
                    master_buf[i * 2] += left;
                    master_buf[i * 2 + 1] += right;

                    sample_bank[j].playback_ptr += 4;

                    // case: playback pointer has reached EOF
                    if (sample_bank[j].playback_ptr >= sample_bank[j].header.data_size) {
                        sample_bank[j].playback_mode.on_finish(sample_bank[j].sample_id);
                    }
                }
            }
        }

        // Write full buffer (1024 bytes)
        ESP_ERROR_CHECK(i2s_channel_write(out_channel, master_buf,
                                          BUFF_SIZE * 2 * sizeof(int16_t),
                                          &w_bytes,
                                          portMAX_DELAY));
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    free(master_buf);
    vTaskDelete(NULL);
}

void create_mixer(i2s_chan_handle_t channel){
    xTaskCreate(&mixer_task_wip, "Mixer task", 2048, (void*)channel, 5, NULL);
}