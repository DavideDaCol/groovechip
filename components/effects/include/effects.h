#ifndef EFFECTS_H_
#define EFFECTS_H_
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#define BIT_DEPTH_MAX 16
#define DOWNSAMPLE_MAX 10
#define DOWNSAMPLE_MIN 1
#define DISTORTION_GAIN_MAX 1.0
#define DISTORTION_THRESHOLD_MAX 32000

#define VOLUME_NORMALIZER_VALUE 0.01f // TODO lo ho impostato a caso
#define PITCH_NORMALIZER_VALUE 0.03f // TODO lo ho impostato a caso
#define THRESHOLD_NORMALIZER_VALUE 320
#define DOWNSAMPLE_NORMALIZER_VALUE 0.1f
#define BIT_DEPTH_NORMALIZER_VALUE 0.16f

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
    int16_t last_frame;
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

 extern effects_t master_buffer_effects;

effects_t* get_sample_effect(uint8_t bank_index);
effects_t* get_master_buffer_effects();

#pragma region PITCH
//=========================PITCH============================
/*
@brief initializes the pitch of the sample mapped to the bank_index
@param bank_index bank index of the sample we want to initialize the pitch of
*/
void init_pitch(uint8_t bank_index);

/*
@brief
@param
*/
void set_pitch_factor(uint8_t bank_index, float pitch_factor);

/*
@brief
@param
*/
float get_pitch_factor(uint8_t bank_index);
//==========================================================
#pragma endregion
#pragma region BIT CRUSHER
//=========================BIT CRUSHER============================
/*
@brief
@param
*/
void init_bit_crusher(uint8_t bank_index);

/*
@brief
@param
*/
void set_bit_crusher(uint8_t bank_index, bool state);

/*
@brief
@param
*/
void set_master_bit_crusher_enable(bool state);

/*
@brief
@param
*/
bool get_bit_crusher_state(uint8_t bank_index);

/*
@brief
@param
*/
bool get_master_bit_crusher_enable();

/*
@brief
@param
*/
void set_bit_crusher_bit_depth(uint8_t bank_index, uint8_t bit_depth);

/*
@brief
@param
*/
void set_bit_crusher_bit_depth_master_buffer(uint8_t bit_depth);

/*
@brief
@param
*/
uint8_t get_bit_crusher_bit_depth(uint8_t bank_index);

/*
@brief
@param
*/
uint8_t get_bit_crusher_bit_depth_master_buffer();

/*
@brief
@param
*/
void set_bit_crusher_downsample(uint8_t bank_index, uint8_t downsample_value);

/*
@brief
@param
*/
void set_bit_crusher_downsample_master_buffer(uint8_t downsample_value);

/*
@brief
@param
*/
uint8_t get_bit_crusher_downsample(uint8_t bank_index);

/*
@brief
@param
*/
uint8_t get_bit_crusher_downsample_master_buffer();

//================================================================
#pragma endregion
#pragma region DISTORTION
//=========================DISTORTION=============================
/*
@brief
@param
*/
void init_distortion(uint8_t bank_index);

/*
@brief
@param
*/
void set_distortion(uint8_t bank_index, bool state);

/*
@brief
@param
*/
void set_master_distortion_enable(bool state);

/*
@brief
@param
*/
bool get_distortion_state(uint8_t bank_index);

/*
@brief
@param
*/
bool get_master_distortion_enable();

/*
@brief
@param
*/
void set_distortion_gain(uint8_t bank_index, float gain);

/*
@brief
@param
*/
void set_distortion_gain_master_buffer(float gain);

/*
@brief
@param
*/
float get_distortion_gain(uint8_t bank_index);

/*
@brief
@param
*/
float get_distortion_gain_master_buffer();

/*
@brief
@param
*/
void set_distortion_threshold(uint8_t bank_index, int16_t threshold_value);

/*
@brief
@param
*/
void set_distortion_threshold_master_buffer(int16_t threshold_value);

/*
@brief
@param
*/
int16_t get_distortion_threshold(uint8_t bank_index);

/*
@brief
@param
*/
int16_t get_distortion_threshold_master_buffer();

//================================================================
/*
@brief
@param
*/
void effects_init();

/*
@brief
@param
*/
void smp_effects_init(int in_bank_index);

#pragma endregion


#endif