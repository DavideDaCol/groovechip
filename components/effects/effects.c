#include "effects.h"
#include "mixer.h"

static effects_t sample_effects[SAMPLE_NUM];

effects_t* get_sample_effect(uint8_t sample_id){
    if(sample_id < SAMPLE_NUM)
        return &sample_effects[sample_id];
    else return NULL;
}
//=========================PITCH============================
void init_pitch(uint8_t sample_id){
    if(sample_id < SAMPLE_NUM){
        sample_effects[sample_id].pitch.pitch_factor = 1.0;
    }
}

void set_pitch_factor(uint8_t sample_id, float pitch_factor){
    if(sample_id < SAMPLE_NUM){
        sample_effects[sample_id].pitch.pitch_factor = pitch_factor;
    }
}

float get_pitch_factor(uint8_t sample_id){
    if(sample_id < SAMPLE_NUM){
        return sample_effects[sample_id].pitch.pitch_factor;
    }
    else return 1.0; //default value
}
//==========================================================

//=========================BIT CRUSHER============================

void init_bit_crusher(uint8_t sample_id){
    if(sample_id < SAMPLE_NUM){
        sample_effects[sample_id].bitcrusher.enabled = false;
        sample_effects[sample_id].bitcrusher.bit_depth = BIT_DEPTH_DEFAULT;
        sample_effects[sample_id].bitcrusher.downsample = DOWNSAMPLE_DEFAULT;
        sample_effects[sample_id].bitcrusher.bc_counter = 0;
        sample_effects[sample_id].bitcrusher.last_bc_sample = 0.0;
    }
}

void set_bit_crusher_bit_depth(uint8_t sample_id, uint8_t bit_depth){
    if(sample_id < SAMPLE_NUM && bit_depth >= 1 && bit_depth <= BIT_DEPTH_DEFAULT){
        sample_effects[sample_id].bitcrusher.bit_depth = bit_depth;
        
        //reset counter values
        sample_effects[sample_id].bitcrusher.bc_counter = 0;
        sample_effects[sample_id].bitcrusher.last_bc_sample = 0.0;
    }
}

void set_bit_crusher_downsample(uint8_t sample_id, uint8_t downsample_value){
    if(sample_id < SAMPLE_NUM && downsample_value >= 1 && downsample_value <= DOWNSAMPLE_DEFAULT){
        sample_effects[sample_id].bitcrusher.downsample = downsample_value;

        //reset counter values
        sample_effects[sample_id].bitcrusher.bc_counter = 0;
        sample_effects[sample_id].bitcrusher.last_bc_sample = 0.0;
    }
}
//================================================================
void effects_init(){
    //init effects to default values
    for(uint8_t i = 0; i < SAMPLE_NUM; i++){
        init_pitch(i);
        init_bit_crusher(i);
    }
}
