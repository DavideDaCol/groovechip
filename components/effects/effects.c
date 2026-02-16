#include "effects.h"
#include "mixer.h"

// effects associated to each sample in the bank
static effects_t sample_effects[SAMPLE_NUM];

// effects applied to the master
effects_t master_buffer_effects;

effects_t* get_sample_effect(uint8_t bank_index){
    if(bank_index < SAMPLE_NUM)
        return &sample_effects[bank_index];
    else return NULL;
}

effects_t* get_master_buffer_effects(){
    return &master_buffer_effects;
}

#pragma region PITCH
//=========================PITCH============================
void init_pitch(uint8_t bank_index){
    if(bank_index < SAMPLE_NUM){
        sample_effects[bank_index].pitch.pitch_factor = 1.0;
    }
    master_buffer_effects.pitch.pitch_factor = 1.0;
}

void set_pitch_factor(uint8_t bank_index, float pitch_factor){
    if(bank_index < SAMPLE_NUM){
        sample_effects[bank_index].pitch.pitch_factor = pitch_factor;
    }
}

float get_pitch_factor(uint8_t bank_index){
    if(bank_index < SAMPLE_NUM){
        return sample_effects[bank_index].pitch.pitch_factor;
    }
    else return 1.0; //default value
}
//==========================================================
#pragma endregion
#pragma region BIT CRUSHER
//=========================BIT CRUSHER============================

void init_bit_crusher(uint8_t bank_index){
    if(bank_index < SAMPLE_NUM){
        sample_effects[bank_index].bitcrusher.enabled = false;
        sample_effects[bank_index].bitcrusher.bit_depth = BIT_DEPTH_MAX;
        sample_effects[bank_index].bitcrusher.downsample = DOWNSAMPLE_MIN;
        sample_effects[bank_index].bitcrusher.counter = 0;
        sample_effects[bank_index].bitcrusher.last_frame = 0.0;
    }
    master_buffer_effects.bitcrusher.enabled = false;
    master_buffer_effects.bitcrusher.bit_depth = BIT_DEPTH_MAX;
    master_buffer_effects.bitcrusher.downsample = DOWNSAMPLE_MIN;
    master_buffer_effects.bitcrusher.counter = 0;
    master_buffer_effects.bitcrusher.last_frame = 0.0;
}

void set_bit_crusher(uint8_t bank_index, bool state){
    sample_effects[bank_index].bitcrusher.enabled = state;
}

void set_master_bit_crusher_enable(bool state){
    master_buffer_effects.bitcrusher.enabled = state;
}

bool get_bit_crusher_state(uint8_t bank_index){
    return sample_effects[bank_index].bitcrusher.enabled;
}

bool get_master_bit_crusher_enable(){
    return master_buffer_effects.bitcrusher.enabled;
}

void set_bit_crusher_bit_depth(uint8_t bank_index, uint8_t bit_depth){
    if(bank_index < SAMPLE_NUM && bit_depth >= 1 && bit_depth <= BIT_DEPTH_MAX){
        sample_effects[bank_index].bitcrusher.bit_depth = bit_depth;
        
        //reset counter values
        sample_effects[bank_index].bitcrusher.counter = 0;
        sample_effects[bank_index].bitcrusher.last_frame = 0.0;
    }
}

void set_bit_crusher_bit_depth_master_buffer(uint8_t bit_depth){
    master_buffer_effects.bitcrusher.bit_depth = bit_depth;
    
    //reset counter values
    master_buffer_effects.bitcrusher.counter = 0;
    master_buffer_effects.bitcrusher.last_frame = 0.0;
}

uint8_t get_bit_crusher_bit_depth(uint8_t bank_index){
    return sample_effects[bank_index].bitcrusher.bit_depth;
}

uint8_t get_bit_crusher_bit_depth_master_buffer(){
    return master_buffer_effects.bitcrusher.bit_depth;
}

void set_bit_crusher_downsample(uint8_t bank_index, uint8_t downsample_value){
    if(bank_index < SAMPLE_NUM && downsample_value >= 1 && downsample_value <= DOWNSAMPLE_MAX){
        sample_effects[bank_index].bitcrusher.downsample = downsample_value;

        //reset counter values
        sample_effects[bank_index].bitcrusher.counter = 0;
        sample_effects[bank_index].bitcrusher.last_frame = 0.0;
    }
}

void set_bit_crusher_downsample_master_buffer(uint8_t downsample_value){
        master_buffer_effects.bitcrusher.downsample = downsample_value;

        //reset counter values
        master_buffer_effects.bitcrusher.counter = 0;
        master_buffer_effects.bitcrusher.last_frame = 0.0;
}

uint8_t get_bit_crusher_downsample(uint8_t bank_index){
    return sample_effects[bank_index].bitcrusher.downsample;
}

uint8_t get_bit_crusher_downsample_master_buffer(){
    return master_buffer_effects.bitcrusher.downsample;
}

//================================================================
#pragma endregion

#pragma region DISTORTION
//=========================DISTORTION=============================
void init_distortion(uint8_t bank_index){
    if(bank_index < SAMPLE_NUM){
        sample_effects[bank_index].distortion.enabled = false;
        sample_effects[bank_index].distortion.gain = DISTORTION_GAIN_MAX;
        sample_effects[bank_index].distortion.threshold = DISTORTION_THRESHOLD_MAX;
    }
    master_buffer_effects.distortion.enabled = false;
    master_buffer_effects.distortion.gain = DISTORTION_GAIN_MAX;
    master_buffer_effects.distortion.threshold = DISTORTION_THRESHOLD_MAX;
}

void set_distortion(uint8_t bank_index, bool state){
    if(bank_index < SAMPLE_NUM){
        sample_effects[bank_index].distortion.enabled = state;
    }
}

void set_master_distortion_enable(bool state){
    master_buffer_effects.distortion.enabled = state;
}

bool get_distortion_state(uint8_t bank_index){
    return sample_effects[bank_index].distortion.enabled;
}

bool get_master_distortion_enable(){
    return master_buffer_effects.distortion.enabled;
}

void set_distortion_gain(uint8_t bank_index, float gain){
    if(bank_index < SAMPLE_NUM){
        sample_effects[bank_index].distortion.gain = gain;
    }
}

void set_distortion_gain_master_buffer(float gain){
    master_buffer_effects.distortion.gain = gain;
}

float get_distortion_gain(uint8_t bank_index){
    return sample_effects[bank_index].distortion.gain;
}

float get_distortion_gain_master_buffer(){
    return master_buffer_effects.distortion.gain;
}

void set_distortion_threshold(uint8_t bank_index, int16_t threshold_value){
    if(bank_index < SAMPLE_NUM){
        sample_effects[bank_index].distortion.threshold = threshold_value;
    }
}

void set_distortion_threshold_master_buffer(int16_t threshold_value){
    master_buffer_effects.distortion.threshold = threshold_value;
}

int16_t get_distortion_threshold(uint8_t bank_index){
    return sample_effects[bank_index].distortion.threshold;
}

int16_t get_distortion_threshold_master_buffer(){
    return master_buffer_effects.distortion.threshold;
}

//================================================================
#pragma endregion

void effects_init(){
    //init effects to default values
    for(uint8_t i = 0; i < SAMPLE_NUM; i++){
        init_pitch(i);
        init_bit_crusher(i);
        init_distortion(i);
    }
}

void smp_effects_init(int in_bank_index){
    init_pitch(in_bank_index);
    init_bit_crusher(in_bank_index);
    init_distortion(in_bank_index);
}