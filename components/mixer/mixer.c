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
#include "metronome.h"
#include "playback_mode.h"
#include "effects.h"
#include "esp_log.h"
#include "recorder.h"

static const char* TAG = "Mixer";

//currently active samples
sample_bitmask now_playing;

// all samples that can be played
sample_t* sample_bank[SAMPLE_NUM];

//metronome object
static metronome mtrn;

#pragma region SAMPLE_ACTION

void action_start_or_stop_sample(int bank_index){
    printf("play/pause event was triggered from %i\n", bank_index);
    if(sample_bank[bank_index] != NULL){
        //either stop or play the sample
        now_playing ^= (1 << bank_index);
        //reset the playback pointer if the sample was stopped
        if (sample_bank[bank_index]->playback_finished){
            sample_bank[bank_index]->playback_ptr = sample_bank[bank_index]->start_ptr;
            sample_bank[bank_index]->playback_finished = false;
        }
    } else {
        ESP_LOGW(TAG, "sample %i is set to NULL!", bank_index);
    }
}

void action_start_sample(int bank_index){
    printf("play event was triggered from %i\n", bank_index);
    if(sample_bank[bank_index] != NULL){
	    //add the sample from the nowplaying bitmask
        now_playing |= (1 << bank_index);
    } else {
        ESP_LOGW(TAG, "sample %i is set to NULL!", bank_index);
    }
}

void action_stop_sample(int bank_index){
    printf("pause event was triggered from %i\n", bank_index);
    if(sample_bank[bank_index] != NULL){
        //remove the sample from the nowplaying bitmask
        now_playing &= ~(1 << bank_index);
        //reset the playback pointer to the start value
        sample_bank[bank_index]->playback_ptr = sample_bank[bank_index]->start_ptr;
        //set the playing state to "not finished" (for future iterations)
        sample_bank[bank_index]->playback_finished = false;
    } else {
        ESP_LOGW(TAG, "sample %i is set to NULL!", bank_index);
    }
}

void action_restart_sample(int bank_index){
    printf("restart event was triggered from %i\n", bank_index);
    if(sample_bank[bank_index] != NULL){
        //add the sample to the nowplaying bitmask
        now_playing |= (1 << bank_index);
        //reset the playback pointer to the start value
        sample_bank[bank_index]->playback_ptr = sample_bank[bank_index]->start_ptr;
        //set the playing state to "not finished" (for future iterations)
        sample_bank[bank_index]->playback_finished = false;
    } else {
        ESP_LOGW(TAG, "sample %i is set to NULL!", bank_index);
    }
}

void action_ignore(int pad_id){
	// nothing
}

#pragma endregion

static inline void get_sample_interpolated(sample_t *smp, int16_t *out_L, int16_t *out_R, uint32_t total_frames) {
    float pos = smp->playback_ptr;

    //first frame
    int frame_a = (int)pos;    
    float frac = pos - frame_a; 

    //second frame 
    int frame_b = frame_a + 1;
    
    //loop handling
    if (frame_b >= total_frames) {
        pb_mode_t playback_mode = get_playback_mode(smp->bank_index);
        if (playback_mode == LOOP || playback_mode == ONESHOT_LOOP) {
            frame_b = 0; //return to first sample
        } else {
            frame_b = frame_a; //stay in the same sample
        }
    }

    // raw data
    int16_t *raw_data = (int16_t*)smp->raw_data;

    //handle stereo audio

    //left channel
    float la = raw_data[frame_a * 2];
    float lb = raw_data[frame_b * 2];
    *out_L = la * (1.0f - frac) + lb * frac;


    //right channel
    float ra = raw_data[frame_a * 2 + 1];
    float rb = raw_data[frame_b * 2 + 1];
    *out_R = ra * (1.0f - frac) + rb * frac;
}

static inline void apply_bitcrusher(bitcrusher_params_t* bc, int16_t *out_L, int16_t *out_R) {

    if(!bc->enabled) return; //exit if the effect is not enabled

    // DOWNSAMPLING (reduce sample_rate)
    bc->counter++;

    // if the counter is less than the downsample value, repeat the same value as before
    if (bc->counter < bc->downsample) {
        *out_L = bc->last_L;
        *out_R = bc->last_R;
        return;
    }

    // update the sample
    bc->counter = 0;

    // BIT CRUSHING (reduce "resolution")
    if (bc->bit_depth < 16) {
        // how much bit do we need to "cut"
        int shift_amount = 16 - bc->bit_depth;

        // left shift in order to set to zero least significant bits, and right shift to bring the other bits back
        *out_L = (*out_L >> shift_amount) << shift_amount;
        *out_R = (*out_R >> shift_amount) << shift_amount;
    }

    // update last sample
    bc->last_L = *out_L;
    bc->last_R = *out_R;
}

static inline void apply_distortion(distortion_params_t* dst_params, int16_t *out_L, int16_t *out_R){

    if(dst_params == NULL) return;
    if(!dst_params->enabled) return;

    //calculate gain
    int32_t temp_L = *out_L * dst_params->gain;
    int32_t temp_R = *out_R * dst_params->gain;

    temp_L = (int16_t)temp_L;
    temp_R = (int16_t)temp_R;

    //calculate threshold
    int16_t threshold = dst_params->threshold;

    //left channel
    if(temp_L > threshold){
        temp_L = threshold;
    }
    else if(temp_L < -threshold){
        temp_L = -threshold; // anche verso il basso? Da capire
    }

    //right channel
    if(temp_R > threshold){
        temp_R = threshold;
    }
    else if(temp_R < -threshold){
        temp_R = -threshold;
    }   

    *out_L = temp_L;
    *out_R = temp_R;

} 
#pragma region VOLUME

void set_volume(uint8_t bank_index, float new_volume){
    sample_bank[bank_index]->volume = new_volume;
    if (sample_bank[bank_index]->volume > VOLUME_THRESHOLD_UP) {
        sample_bank[bank_index]->volume = VOLUME_THRESHOLD_UP;
    }
    if (sample_bank[bank_index]->volume < 0) {
        sample_bank[bank_index]->volume = 0;
    }
}

float get_volume(uint8_t bank_index){
    if(bank_index < SAMPLE_NUM){
        return sample_bank[bank_index]->volume;
    } 
    else return 0.0;
}

#pragma endregion

#pragma region METRONOME

void init_metronome(){
    mtrn.state = false;
    mtrn.bpm = 120.;
    mtrn.subdivisions = 1;
    mtrn.playback_enabled = false;
    set_metronome_tick();

    mtrn.playback_ptr = 0;
    memcpy(&mtrn.header, metronome_clean_wav, WAV_HDR_SIZE);
    mtrn.raw_data = metronome_clean_wav + WAV_HDR_SIZE;
}

void toggle_metronome_state(bool new_state){
    mtrn.state = new_state;
}
void set_metronome_bpm(float new_bpm){
    mtrn.bpm = new_bpm;
    set_metronome_tick();
}
void set_metronome_subdiv(int new_subdiv){
    mtrn.subdivisions = new_subdiv;
    set_metronome_tick();
}
void set_metronome_tick(){
    float new_sample_per_subdiv = GRVCHP_SAMPLE_FREQ * ((60.0 / mtrn.bpm) / mtrn.subdivisions ); 
    ESP_LOGI(TAG, "samples per subdivision: %f", new_sample_per_subdiv);
    mtrn.samples_per_subdivision = new_sample_per_subdiv;
}

void toggle_metronome_playback(bool new_state){
    mtrn.playback_enabled = new_state;
}

#pragma endregion

#pragma region SAMPLE CHOPPING
void set_sample_end_ptr(uint8_t bank_index, uint32_t new_end_ptr){
    sample_t *smp = sample_bank[bank_index];
    if(new_end_ptr > smp->start_ptr && new_end_ptr < smp->total_frames){
        smp->end_ptr = new_end_ptr; // TODO move this out of here. We before changing this parameter, the sample must be stopped.
    }
}
void set_sample_start_ptr(uint8_t bank_index, float new_start_ptr){
    sample_t *smp = sample_bank[bank_index];
    if(new_start_ptr >= 0.0 && new_start_ptr < smp->end_ptr){
        smp->start_ptr = new_start_ptr;
        smp->playback_ptr = new_start_ptr; // TODO move this out of here. We before changing this parameter, the sample must be stopped.
    }
}
#pragma endregion

void print_wav_header(const wav_header_t *h)
{
    if (h == NULL) {
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

esp_err_t ld_internal_sample(int bank_index, const uint8_t* wav_data, const char* debug_name) {
    // 1. Allocate the struct in PSRAM
    //sample_bank[bank_index] = heap_caps_malloc(sizeof(sample_t), MALLOC_CAP_SPIRAM);
    sample_bank[bank_index] = malloc(sizeof(sample_t));
    if (sample_bank[bank_index] == NULL) return ESP_ERR_NO_MEM;

    sample_t* smp = sample_bank[bank_index];

    // 2. Copy the header from the static array
    memcpy(&smp->header, wav_data, sizeof(wav_header_t));

    // 3. Allocate memory for the raw data and copy it
    //smp->raw_data = heap_caps_malloc(smp->header.data_size, MALLOC_CAP_SPIRAM);
    smp->raw_data = malloc(smp->header.data_size);
    if (smp->raw_data == NULL) {
        heap_caps_free(smp);
        sample_bank[bank_index] = NULL;
        return ESP_ERR_NO_MEM;
    }
    
    // Copy the audio part (skipping the header)
    memcpy(smp->raw_data, wav_data + sizeof(wav_header_t), smp->header.data_size);

    // 4. Set the metadata
    smp->bank_index = bank_index;
    smp->total_frames = smp->header.data_size / 4;
    smp->start_ptr = 0.0f;
    smp->end_ptr = (float)smp->total_frames - 1.0f;
    smp->playback_ptr = 0.0f;
    smp->playback_finished = false;
    smp->volume = 0.1f;

    ESP_LOGI("Mixer", "Loaded internal sample: %s into bank %d", debug_name, bank_index);
    return ESP_OK;
}

static void mixer_task_wip(void *args)
{
    i2s_chan_handle_t out_channel = (i2s_chan_handle_t)args;
    assert(out_channel);

    //initialize the sample bank
    
    //initialize the metronome
    init_metronome();

    //initialize the recording struct
    recorder_init();

    //test metronome functions (equivalent to 120bpm)
    set_metronome_bpm(60.0);
    set_metronome_subdiv(2);

    size_t w_bytes = BUFF_SIZE;

    // create a i2s DMA buffer to write samples without stressing the CPU.
    // Has to be double the chosen size to allow stereo playback
    int16_t *master_buf = malloc(BUFF_SIZE * 2 * sizeof(int16_t));
    assert(master_buf);

    ESP_ERROR_CHECK(i2s_channel_enable(out_channel));

    // for the metronome: counts how many samples have been played since the last tick
    int16_t sample_lookahead = 0;

     while (1) {
        for (int i = 0; i < BUFF_SIZE; i++) {

            sample_lookahead += 1;

            if (sample_lookahead >= mtrn.samples_per_subdivision) {
                // unlock_metronome
                toggle_metronome_playback(true);
                //reset the metronome audio, in case the sample is too long for each tick
                mtrn.playback_ptr = 0;
                sample_lookahead = 0;
            }

            //fill the buffer with 0 in case no samples are playing
            master_buf[i * 2] = 0x00;
            master_buf[i * 2 + 1] = 0x00;

            //look at all playing samples
            for (int j = 0; j < SAMPLE_NUM; j++){

                //check play status via bit masking
                if (sample_bank[j] != NULL && (now_playing & (1 << j)) != 0  && !sample_bank[j]->playback_finished){

                    // in WAV files, left and right samples are sequential
                    int16_t left, right;
                    
                    // stereo interpolated samples
                    get_sample_interpolated(sample_bank[j], &left, &right, sample_bank[j]->total_frames);
                    
                    //volume adjustment
                    left *= sample_bank[j]->volume;
                    right *= sample_bank[j]->volume;

                    //apply distortion
                    distortion_params_t *dst_params = &get_sample_effect(j)->distortion;
                    apply_distortion(dst_params, &left, &right);

                    //apply bit crushing
                    bitcrusher_params_t *bc_params = &get_sample_effect(j)->bitcrusher;
                    apply_bitcrusher(bc_params, &left, &right);

                    // writes the WAV data to the buffer post volume adjustment
                    master_buf[i * 2] += left;
                    master_buf[i * 2 + 1] += right;

                    // add the pitch factor to the pointer
                    sample_bank[j]->playback_ptr += get_pitch_factor(j);

                    // case: playback pointer has reached EOF or the end_ptr
                    if (sample_bank[j]->playback_ptr > sample_bank[j]->end_ptr || sample_bank[j]->playback_ptr >= sample_bank[j]->total_frames) {
                        if (!sample_bank[j]->playback_finished) {
                            //flag the sample as "done playing"
                            sample_bank[j]->playback_finished = true;
                            //stop the sample
                            send_mixer_event(j, EVT_FINISH);
                        }
                    }
                    
                }
            }

            // capture the master frame for the recorded sample
            if (recorder_is_recording()){
                recorder_capture_frame(master_buf[i * 2], master_buf[i * 2 + 1]);
            }

            if (mtrn.playback_enabled) {
                // if the metronome is playing, but the sound has finished
                if (mtrn.playback_ptr >= mtrn.header.data_size) {
                    //lock the metronome again
                    toggle_metronome_playback(false);
                    mtrn.playback_ptr = 0;
                } else {
                    // otherwise, keep on playing!
                    int16_t left_mtrn  = *(int16_t*)(mtrn.raw_data + mtrn.playback_ptr);
                    int16_t right_mtrn = *(int16_t*)(mtrn.raw_data + mtrn.playback_ptr + 2);

                    master_buf[i * 2] += left_mtrn * 0.1;
                    master_buf[i * 2 + 1] += right_mtrn * 0.1;

                    mtrn.playback_ptr += 4;
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

    for (int i = 0; i < SAMPLE_NUM; i++) {
        sample_bank[i] = NULL;
    }

    xTaskCreate(&mixer_task_wip, "Mixer task", 8192, (void*)channel, 5, NULL);
}

