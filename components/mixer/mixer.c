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

// all samples that can be played
sample_t sample_bank[SAMPLE_NUM];

//currently active samples
sample_bitmask now_playing;

static void mixer_task_wip(void *args)
{
    i2s_chan_handle_t out_channel = (i2s_chan_handle_t)args;
    assert(out_channel);

    //create new sample and copy its data to it

    memcpy(&sample_bank[0].header, WavData, WAV_HDR_SIZE);
    sample_bank[0].raw_data = WavData + WAV_HDR_SIZE;

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

            // in WAV files, left and right samples are sequential
            int16_t left1  = *(int16_t*)(sample_bank[0].raw_data + sample_bank[0].playback_ptr);
            int16_t right1 = *(int16_t*)(sample_bank[0].raw_data + sample_bank[0].playback_ptr + 2);

            left1  *= volume;
            right1 *= volume;

            // writes the WAV data to the buffer post volume adjustment
            master_buf[i * 2] = left1;
            master_buf[i * 2 + 1] = right1;

            sample_bank[0].playback_ptr += 4;

            // loop the audio file
            if (sample_bank[0].playback_ptr >= sample_bank[0].header.data_size) {
                sample_bank[0].playback_ptr = 0;
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