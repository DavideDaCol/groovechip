#include "mixer.h"
#include <stdint.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "sdkconfig.h"
#include "metronome.h"
#include "playback_mode.h"
#include "effects.h"
#include "esp_log.h"
#include "recorder.h"
#include "fsm.h"
#include "sd_reader.h"

static const char* TAG = "Mixer";

TaskHandle_t fsm_task_handler;

//currently active samples
sample_bitmask now_playing;

// all samples that can be played
sample_t* sample_bank[SAMPLE_NUM];

// volume of master buffer
float volume = 0.5f;

#pragma region Apply fuctions
/*
@brief apply the bit crusher effect to the audio bufer.
@param bc bit crusher parameters.
@param out audio buffer where the effect will be applied.
*/
static inline void apply_bitcrusher_mono(bitcrusher_params_t* bc, int16_t *out) {

    if(!bc->enabled) return; //exit if the effect is not enabled

    // DOWNSAMPLING (reduce sample_rate)
    bc->counter++;

    // if the counter is less than the downsample value, repeat the same value as before
    if (bc->counter < bc->downsample) {
        *out = bc->last_frame;
        return;
    }

    // update the sample
    bc->counter = 0;

    // BIT CRUSHING (reduce "resolution")
    if (bc->bit_depth < 16) {
        // how much bit do we need to "cut"
        int shift_amount = 16 - bc->bit_depth;

        // left shift in order to set to zero least significant bits, and right shift to bring the other bits back
        *out = (*out >> shift_amount) << shift_amount;
    }

    // update last sample
    bc->last_frame = *out;
}

/*
@brief apply the distrotion effect to the audio bufer.
@param dst_params distortion parameters.
@param out audio buffer where the effect will be applied.
*/
static inline void apply_distortion_mono(distortion_params_t* dst_params, int16_t *out){

    if(dst_params == NULL) return;
    if(!dst_params->enabled) return;

    //calculate gain
    int32_t temp = *out * dst_params->gain;

    temp = (int16_t)temp;

    //calculate threshold
    int16_t threshold = dst_params->threshold;

    if(temp > threshold){
        temp = threshold;
    }
    else if(temp < -threshold){
        temp = -threshold;
    }

    *out = temp;
} 

#pragma endregion

#pragma region SAMPLE_ACTION

void action_start_or_stop_sample(int bank_index){

    if (g_recorder.state == REC_WAITING_PAD){
        printf("AOAOAOAOOAOAOA\n");
        return;
    }

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

    if (g_recorder.state == REC_WAITING_PAD){
        printf("AOAOAOAOOAOAOA\n");
        return;
    }

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


static inline void get_sample_interpolated_mono(sample_t *smp, int16_t *out, uint32_t total_frames) {
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

    //interpolation
    float la = raw_data[frame_a];
    float lb = raw_data[frame_b];
    *out = la * (1.0f - frac) + lb * frac;

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

void set_master_buffer_volume(float new_volume){
    volume = new_volume;
    if (volume > MASTER_VOLUME_THRESHOLD_UP) {
        volume = MASTER_VOLUME_THRESHOLD_UP;
    }
    if (volume < 0) {
        volume = 0;
    }
}

float get_master_volume(){
    return volume;
}

#pragma endregion

#pragma region SAMPLE CHOPPING

static uint8_t chopping_precision = 1;

uint32_t get_sample_end_ptr(uint8_t bank_index){
    return (uint32_t)sample_bank[bank_index]->end_ptr;
}

uint32_t get_sample_start_ptr(uint8_t bank_index){
    return sample_bank[bank_index]->start_ptr;
}

bool set_sample_end_ptr(uint8_t bank_index, uint32_t new_end_ptr){
    sample_t *smp = sample_bank[bank_index];
    if(new_end_ptr > smp->start_ptr && new_end_ptr < smp->total_frames){
        smp->end_ptr = new_end_ptr; 
        return true;
    }
    else return false;
}
bool set_sample_start_ptr(uint8_t bank_index, float new_start_ptr){
    sample_t *smp = sample_bank[bank_index];
    if(new_start_ptr >= 0.0 && new_start_ptr < smp->end_ptr){
        smp->start_ptr = new_start_ptr;
        return true;
    }
    else return false;
}
uint32_t get_sample_total_frames(uint8_t bank_index){
    return sample_bank[bank_index]->total_frames;
}


uint8_t get_chopping_precision(){
    return chopping_precision;
}
void set_chopping_precision(uint8_t precision){
    chopping_precision = precision;
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

esp_err_t ld_sample_debug(int bank_index, const uint8_t* wav_data, const char* debug_name) {
    // 1. Allocate the struct in PSRAM
    sample_bank[bank_index] = malloc(sizeof(sample_t));
    if (sample_bank[bank_index] == NULL) return ESP_ERR_NO_MEM;

    sample_t* smp = sample_bank[bank_index];

    // 2. Copy the header from the static array
    memcpy(&smp->header, wav_data, sizeof(wav_header_t));

    // 3. Allocate memory for the raw data and copy it
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
    smp->total_frames = smp->header.data_size / 2;
    smp->start_ptr = 0.0f;
    smp->end_ptr = (float)smp->total_frames - 1.0f;
    smp->playback_ptr = 0.0f;
    smp->playback_finished = false;
    smp->volume = 0.1f;

    ESP_LOGI("Mixer", "Loaded internal sample: %s into bank %d", debug_name, bank_index);
    return ESP_OK;
}

static void mixer_task(void *args)
{
    i2s_chan_handle_t out_channel = (i2s_chan_handle_t)args;
    assert(out_channel);
    
    //initialize the metronome
    init_metronome();

    //initialize the recording struct
    recorder_init();

    size_t w_bytes = BUFF_SIZE;

    // create a i2s DMA buffer to write samples without stressing the CPU.
    int16_t *master_buf = malloc(BUFF_SIZE * sizeof(int16_t));
    assert(master_buf);

    ESP_ERROR_CHECK(i2s_channel_enable(out_channel));

    // for the metronome: counts how many samples have been played since the last tick
    int16_t sample_lookahead = 0;

    while (1) {
        for (int i = 0; i < BUFF_SIZE; i++) {

            sample_lookahead += 1;

            if (sample_lookahead >= get_samples_per_subdiv()) {
                // unlock_metronome
                set_metronome_playback(true);
                //reset the metronome audio, in case the sample is too long for each tick
                reset_mtrn();
                sample_lookahead = 0;
            }

            //fill the buffer with 0 in case no samples are playing
            master_buf[i] = 0x00;

            //look at all playing samples
            for (int j = 0; j < SAMPLE_NUM; j++){

                //check play status via bit masking
                if (sample_bank[j] != NULL && (now_playing & (1 << j)) != 0  && !sample_bank[j]->playback_finished){

                    //single audio sample as contained in the WAV file
                    int16_t sample_to_play;
                    
                    get_sample_interpolated_mono(sample_bank[j], &sample_to_play, sample_bank[j]->total_frames);
                    
                    //volume adjustment
                    sample_to_play *= sample_bank[j]->volume;

                    //apply distortion
                    distortion_params_t *dst_params = &get_sample_effect(j)->distortion;
                    apply_distortion_mono(dst_params, &sample_to_play);

                    //apply bit crushing
                    bitcrusher_params_t *bc_params = &get_sample_effect(j)->bitcrusher;
                    apply_bitcrusher_mono(bc_params, &sample_to_play);

                    // writes the WAV data to the buffer post volume adjustment and effects pipeline
                    master_buf[i] += sample_to_play;

                    // add the pitch factor to the pointer
                    sample_bank[j]->playback_ptr += get_pitch_factor(j);
                    //printf("PLAYBACK PTR: %f\nEND_PTR: %ld\n", sample_bank[j]->playback_ptr, sample_bank[j]->end_ptr);

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

            // apply volume to master buffer
            master_buf[i] *= (volume * 2);

            // capture the master frame for the recorded sample
            if (recorder_is_recording()){
                recorder_capture_frame(master_buf[i]);
            }

            if (get_metronome_playback() && get_metronome_state()) {
                // if the metronome is playing, but the sound has finished
                if (is_metronome_tick()) {
                    //lock the metronome again
                    set_metronome_playback(false);
                    reset_mtrn();
                } else {
                    // otherwise, keep on playing!
                    int16_t mtrn_audio  = advance_metronome_audio();

                    master_buf[i] += mtrn_audio * 0.1;

                    advance_metronome_ptr();
                }
                
            }
            
        }
        

        // Write full buffer (1024 bytes)
        ESP_ERROR_CHECK(i2s_channel_write(out_channel, master_buf,
                                          BUFF_SIZE * sizeof(int16_t),
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

    xTaskCreate(&mixer_task, "Mixer task", 8192, (void*)channel, 5, NULL);
}

void sample_init (sample_t* in_sample, int size, int bank_index) {
    sample_names_bank[bank_index] = NULL;
    
    // manually fill the WAV header
    memcpy(in_sample->header.riff_section_id, "RIFF", 4);
    in_sample->header.size = sizeof(wav_header_t) - 8 + size; 
    memcpy(in_sample->header.riff_format, "WAVE", 4);
    
    memcpy(in_sample->header.format_id, "fmt ", 4); 
    in_sample->header.format_size = 16;
    in_sample->header.fmt_id = 1;
    in_sample->header.num_channels = 1;
    in_sample->header.sample_rate = GRVCHP_SAMPLE_FREQ;
    
    in_sample->header.block_align = 2; 
    in_sample->header.byte_rate = in_sample->header.block_align * GRVCHP_SAMPLE_FREQ;
    in_sample->header.bits_per_sample = 16;
    
    memcpy(in_sample->header.data_id, "data", 4);
    in_sample->header.data_size = size;
    
    in_sample->total_frames = size / sizeof(uint16_t); 
    in_sample->start_ptr = 0.0f;
    in_sample->end_ptr = (float)in_sample->total_frames - 1.0f;
    in_sample->playback_ptr = 0.0f;
    in_sample->playback_finished = false;
    in_sample->volume = 1.0f;

    in_sample->bank_index = bank_index;

    // initializing the effects
    smp_effects_init(bank_index);
}
