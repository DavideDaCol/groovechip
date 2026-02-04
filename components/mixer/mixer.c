#include "mixer.h"
#include <stdint.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "sdkconfig.h"
#include "kick.h"
#include "snare.h"
#include "playback_mode.h"

//currently active samples
sample_bitmask now_playing;

// all samples that can be played
sample_t sample_bank[SAMPLE_NUM];

#pragma region SAMPLE_ACTION

// these are just placeholders functions.
void action_start_or_stop_sample(int sample_id){
    printf("play/pause event was triggered");
	//either stop or play the sample
    now_playing ^= (1 << sample_id);
    //reset the playback pointer if the sample was stopped
    if ((now_playing & (1 << sample_id)) != 0 ){
        sample_bank[sample_id].playback_ptr = 0;
    }
}

void action_start_sample(int sample_id){
    printf("play event was triggered");
	//add the sample from the nowplaying bitmask
    now_playing |= (1 << sample_id);
}

void action_stop_sample(int sample_id){
    printf("pause event was triggered");
	//remove the sample from the nowplaying bitmask
    now_playing &= ~(1 << sample_id);
    //reset the playback pointer
    sample_bank[sample_id].playback_ptr = 0;
    //set the playing state to "not done" (for future iterations)
    sample_bank[sample_id].playback_finished = false;
}

void action_restart_sample(int sample_id){
    printf("restart event was triggered");
    //add the sample to the nowplaying bitmask
    now_playing |= (1 << sample_id);
	//reset the playback pointer
    sample_bank[sample_id].playback_ptr = 0;
}

void action_ignore(int pad_id){
	// nothing
}

#pragma endregion

void print_wav_header(const wav_header_t *h)
{
    if (!h) {
        printf("WAV header is NULL\n");
        return;
    }

    printf("=== WAV HEADER ===\n");

    printf("RIFF Section ID   : %.4s\n", h->riff_section_id);
    printf("File Size         : %lu bytes\n", h->size);
    printf("RIFF Format       : %.4s\n", h->riff_format);

    printf("Format ID         : %.4s\n", h->format_id);
    printf("Format Size       : %lu\n", h->format_size);
    printf("Audio Format      : %u (%s)\n",
           h->fmt_id,
           (h->fmt_id == 1) ? "PCM" : "Non-PCM");
    printf("Channels          : %u\n", h->num_channels);
    printf("Sample Rate       : %lu Hz\n", h->sample_rate);
    printf("Byte Rate         : %lu\n", h->byte_rate);
    printf("Block Align       : %u\n", h->block_align);
    printf("Bits Per Sample   : %u\n", h->bits_per_sample);

    printf("Data ID           : %.4s\n", h->data_id);
    printf("Data Size         : %lu bytes\n", h->data_size);

    printf("===================\n");
}

static void mixer_task_wip(void *args)
{
    i2s_chan_handle_t out_channel = (i2s_chan_handle_t)args;
    assert(out_channel);

    //initialize the sample bank
    for (int j = 0; j < SAMPLE_NUM; j++){
        sample_bank[j].sample_id = j;
        sample_bank[j].playback_ptr = 0;
    }

    //initialize the first sample for testing purposes
    memcpy(&sample_bank[0].header, snare_clean_wav, WAV_HDR_SIZE);
    sample_bank[0].raw_data = snare_clean_wav + WAV_HDR_SIZE;
    sample_bank[0].playback_finished = false;
    // sample 0 will be triggered by GPIO 23
    map_pad_to_sample(23, 0);
    //need to set the handler to something to avoid crashes!
    //it will actually be set correctly by the sample component
    // sample_bank[0].playback_mode.on_finish = action_stop_sample;
    set_playback_mode(0, PLAYBACK_MODES[HOLD]);
    //debug function
    print_wav_header(&sample_bank[0].header);

    //initialize the second sample for testing purposes
    memcpy(&sample_bank[1].header, kick_clean_wav, WAV_HDR_SIZE);
    sample_bank[1].raw_data = kick_clean_wav + WAV_HDR_SIZE;
    sample_bank[1].playback_finished = false;
    // sample 1 will be triggered by GPIO 19
    map_pad_to_sample(19, 1);
    // sample_bank[1].playback_mode.on_finish = action_stop_sample;
    set_playback_mode(0, PLAYBACK_MODES[HOLD]);

    print_wav_header(&sample_bank[1].header);

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
                        if (!sample_bank[j].playback_finished) {
                            //flag the sample as "done playing"
                            sample_bank[j].playback_finished = true;
                            //stop the sample
                            send_mixer_event(j, EVT_FINISH);
                        }
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