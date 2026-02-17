#include "metronome.h"
#include "metronome_sound.h"
#include <stdio.h>

#pragma region METRONOME

const char* TAG_MTRN = "Metronome";

//metronome object
static metronome mtrn;

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

void set_metronome_state(bool new_state){
    mtrn.state = new_state;
}

bool get_metronome_state(){
    return mtrn.state;
}

void set_metronome_bpm(float new_bpm){
    mtrn.bpm = new_bpm;
    if (mtrn.bpm > MAX_METRONOME_BPM){
        mtrn.bpm = MAX_METRONOME_BPM;
    }
    if (mtrn.bpm <= MIN_MOTRONOME_BPM){
        mtrn.bpm = MIN_MOTRONOME_BPM;
    }
    set_metronome_tick();
}

float get_metronome_bpm(){
    return mtrn.bpm;
}

void set_metronome_subdiv(int new_subdiv){
    mtrn.subdivisions = new_subdiv;
    set_metronome_tick();
}

int8_t get_metronome_subdiv(){
    return mtrn.subdivisions;
}

void set_metronome_tick(){
    float new_sample_per_subdiv = GRVCHP_SAMPLE_FREQ * ((60.0 / mtrn.bpm) / mtrn.subdivisions ); 
    ESP_LOGI(TAG_MTRN, "samples per subdivision: %f", new_sample_per_subdiv);
    mtrn.samples_per_subdivision = new_sample_per_subdiv;
}

float get_samples_per_subdiv(){
    return mtrn.samples_per_subdivision;
}

void set_metronome_playback(bool new_state){
    mtrn.playback_enabled = new_state;
}

bool get_metronome_playback(){
    return mtrn.playback_enabled;
}

int16_t advance_metronome_audio(){
    return *(int16_t*)(mtrn.raw_data + mtrn.playback_ptr);
}

bool is_metronome_tick(){
    return mtrn.playback_ptr >= mtrn.header.data_size;
}

void reset_mtrn(){
    mtrn.playback_ptr = 0;
}

void advance_metronome_ptr(){
    mtrn.playback_ptr += 2;
}

#pragma endregion

