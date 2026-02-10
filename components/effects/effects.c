#include "effects.h"
#include "mixer.h"

static effects_t sample_effects[SAMPLE_NUM];

effects_t* get_sample_effect(uint8_t bank_index){
    if(bank_index < SAMPLE_NUM)
        return &sample_effects[bank_index];
    else return NULL;
}
#pragma region PITCH
//=========================PITCH============================
void init_pitch(uint8_t bank_index){
    if(bank_index < SAMPLE_NUM){
        sample_effects[bank_index].pitch.pitch_factor = 1.0;
    }
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
        sample_effects[bank_index].bitcrusher.downsample = DOWNSAMPLE_MAX;
        sample_effects[bank_index].bitcrusher.counter = 0;
        sample_effects[bank_index].bitcrusher.last_L = 0.0;
        sample_effects[bank_index].bitcrusher.last_R = 0.0;
    }
}

void toggle_bit_crusher(uint8_t bank_index, bool state){
    sample_effects[bank_index].bitcrusher.enabled = state;
}

void set_bit_crusher_bit_depth(uint8_t bank_index, uint8_t bit_depth){
    if(bank_index < SAMPLE_NUM && bit_depth >= 1 && bit_depth <= BIT_DEPTH_MAX){
        sample_effects[bank_index].bitcrusher.bit_depth = bit_depth;
        
        //reset counter values
        sample_effects[bank_index].bitcrusher.counter = 0;
        sample_effects[bank_index].bitcrusher.last_L = 0.0;
        sample_effects[bank_index].bitcrusher.last_R = 0.0;
    }
}

void set_bit_crusher_downsample(uint8_t bank_index, uint8_t downsample_value){
    if(bank_index < SAMPLE_NUM && downsample_value >= 1 && downsample_value <= DOWNSAMPLE_MAX){
        sample_effects[bank_index].bitcrusher.downsample = downsample_value;

        //reset counter values
        sample_effects[bank_index].bitcrusher.counter = 0;
        sample_effects[bank_index].bitcrusher.last_L = 0.0;
        sample_effects[bank_index].bitcrusher.last_R = 0.0;
    }
}
//================================================================
#pragma endregion

#pragma region DISTORTION
//=========================DISTORTION=============================
void init_distortion(uint8_t bank_index){
    if(bank_index < SAMPLE_NUM){
        sample_effects[bank_index].distortion.enabled = false;
        sample_effects[bank_index].distortion.gain = DISTORTION_GAIN_MIX;
        sample_effects[bank_index].distortion.threshold = DISTORTION_THRESHOLD_MAX;
    }
}

void toggle_distortion(uint8_t bank_index, bool state){
    if(bank_index < SAMPLE_NUM){
        sample_effects[bank_index].distortion.enabled = state;
    }
}

void set_distortion_gain(uint8_t bank_index, float gain){
    if(bank_index < SAMPLE_NUM){
        sample_effects[bank_index].distortion.gain = gain;
    }
}

void set_distortion_threshold(uint8_t bank_index, int16_t threshold_value){
    if(bank_index < SAMPLE_NUM){
        sample_effects[bank_index].distortion.threshold = threshold_value;
    }
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

