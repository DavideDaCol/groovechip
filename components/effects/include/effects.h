#ifndef EFFECTS_H_
#define EFFECTS_H_
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#define BIT_DEPTH_MAX 16
#define DOWNSAMPLE_MAX 10
#define DISTORTION_GAIN_MIX 1.0
#define DISTORTION_THRESHOLD_MAX 32000

//pitch struct
typedef struct{
    float pitch_factor;
} pitch_params_t;

//bit crusher struct
typedef struct {
    //parameters
    bool enabled;
    uint8_t bit_depth;       // 1-16
    uint8_t downsample; // 1-10
    int counter;

    //internal state
    int16_t last_L;
    int16_t last_R; 
} bitcrusher_params_t;

//distortion
typedef struct{
    bool enabled;
    float gain;
    int16_t threshold;
}distortion_params_t;

//effects container
typedef struct{
    pitch_params_t pitch;
    bitcrusher_params_t bitcrusher;
    distortion_params_t distortion;
} effects_t;


effects_t* get_sample_effect(uint8_t bank_index);

#pragma region PITCH
//=========================PITCH============================
void init_pitch(uint8_t bank_index);

void set_pitch_factor(uint8_t bank_index, float pitch_factor);

float get_pitch_factor(uint8_t bank_index);
//==========================================================
#pragma endregion
#pragma region BIT CRUSHER
//=========================BIT CRUSHER============================

void init_bit_crusher(uint8_t bank_index);

void toggle_bit_crusher(uint8_t bank_index, bool state);

bool get_bit_crusher_state(uint8_t bank_index);

void set_bit_crusher_bit_depth(uint8_t bank_index, uint8_t bit_depth);

void set_bit_crusher_downsample(uint8_t bank_index, uint8_t downsample_value);

//================================================================
#pragma endregion
#pragma region DISTORTION
//=========================DISTORTION=============================
void init_distortion(uint8_t bank_index);

void toggle_distortion(uint8_t bank_index, bool state);

bool get_distortion_state(uint8_t bank_index);

void set_distortion_gain(uint8_t bank_index, float gain);

void set_distortion_threshold(uint8_t bank_index, int16_t threshold_value);

//================================================================
void effects_init();

#pragma endregion


#endif