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

#define VOLUME_NORMALIZER_VALUE 0.01f 
#define PITCH_NORMALIZER_VALUE 0.03f 
#define THRESHOLD_NORMALIZER_VALUE 320
#define DOWNSAMPLE_NORMALIZER_VALUE 0.1f
#define BIT_DEPTH_NORMALIZER_VALUE 0.16f
#define PITCH_SCALE_VALUE 0.25f
#define THRESHOLD_SCALE_VALUE 1000
#define VOLUME_SCALE_VALUE 0.05f

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
@brief initializes the pitch of the sample mapped to the bank_index.
@param bank_index bank index of the sample we want to initialize the pitch of.
*/
void init_pitch(uint8_t bank_index);

/*
@brief pitch factor's setter based on the bank index.
@param bank_index bank index of the sample we want to set the pitch factor of.
*/
void set_pitch_factor(uint8_t bank_index, float pitch_factor);

/*
@brief pitch factor's getter based on the bank index.
@param bank_index bank index of the sample we want to get the pitch factor of .
*/
float get_pitch_factor(uint8_t bank_index);
//==========================================================
#pragma endregion
#pragma region BIT CRUSHER
//=========================BIT CRUSHER============================
/*
@brief bit crusher's initializer.
@param bank_index bank index of the sample we want to initialize the bit crusher of.
*/
void init_bit_crusher(uint8_t bank_index);

/*
@brief bit crusher's state setter.
@param bank_index bank index of the sample we want to change the bit crusher's state of.
@param state state (on/off) we want to set the bit crusher to
*/
void set_bit_crusher(uint8_t bank_index, bool state);

/*
@brief master bit crusher's setter.
@param state state (on/off) we want to set the bit crusher to.
*/
void set_master_bit_crusher_enable(bool state);

/*
@brief bit crusher's getter.
@param bank_index bank index of the sample we want to get the bit crusher's state of.
*/
bool get_bit_crusher_state(uint8_t bank_index);

/*
@brief master bit crusher's state getter.
*/
bool get_master_bit_crusher_enable();

/*
@brief bit depth's setter.
@param bank_index bank index of the sample we want to set the pitch factor of.
@param bit_depth value of the bit depth we want to change to.
*/
void set_bit_crusher_bit_depth(uint8_t bank_index, uint8_t bit_depth);

/*
@brief master bit crusher bit depth's setter.
@param bit_depth value of the bit depth we want to change to.
*/
void set_bit_crusher_bit_depth_master_buffer(uint8_t bit_depth);

/*
@brief bit crusher bit depth's getter.
@param bank_index bank index of the sample we want to get the bit crusher's bit depth of.
*/
uint8_t get_bit_crusher_bit_depth(uint8_t bank_index);

/*
@brief master bit crusher bit depth's getter.
*/
uint8_t get_bit_crusher_bit_depth_master_buffer();

/*
@brief bit crusher downsample's setter.
@param bank_index bank index of the sample we want to change the downsample's of.
*/
void set_bit_crusher_downsample(uint8_t bank_index, uint8_t downsample_value);

/*
@brief master bit crusher downsample's setter.
@param bank_index bank index of the sample we want to set the pitch factor of.
@param bit_depth value of the downsample we want to change to.
*/
void set_bit_crusher_downsample_master_buffer(uint8_t downsample_value);

/*
@brief bit crusher downsample's getter.
@param bank_index bank index of the sample we want to get the downsample's bit depth of.
*/
uint8_t get_bit_crusher_downsample(uint8_t bank_index);

/*
@brief master bit crusher downsample's getter.
*/
uint8_t get_bit_crusher_downsample_master_buffer();

//================================================================
#pragma endregion
#pragma region DISTORTION
//=========================DISTORTION=============================
/*
@brief distortion's initializer.
@param bank_index bank index of the sample we want to initialize the distortion of.
*/
void init_distortion(uint8_t bank_index);

/*
@brief distortion's state setter.
@param bank_index bank index of the sample we want to change the distortion's state of.
@param state state (on/off) we want to set the distortion to.
*/
void set_distortion(uint8_t bank_index, bool state);

/*
@brief master distortion's setter.
@param state state (on/off) we want to set the distortion to.
*/
void set_master_distortion_enable(bool state);

/*
@brief distortion's getter.
@param bank_index bank index of the sample we want to get the distortion's state of.
*/
bool get_distortion_state(uint8_t bank_index);

/*
@brief master distortion's getter.
*/
bool get_master_distortion_enable();

/*
@brief gain's setter.
@param bank_index bank index of the sample we want to change the gain of.
@param gain value we want to set the gain to.
*/
void set_distortion_gain(uint8_t bank_index, float gain);

/*
@brief master gain's setter.
@param gain value we want to set the gain to.
*/
void set_distortion_gain_master_buffer(float gain);

/*
@brief gain's getter.
@param bank_index bank index of the sample we want to get the gain of.
*/
float get_distortion_gain(uint8_t bank_index);

/*
@brief master gain's getter.
*/
float get_distortion_gain_master_buffer();

/*
@brief threshold's setter.
@param bank_index bank index of the sample we want to change the threshold of.
@param threshold_value value we want to set the threshold to.
*/
void set_distortion_threshold(uint8_t bank_index, int16_t threshold_value);

/*
@brief threshold's setter.
@param threshold_value value we want to set the gain to.
*/
void set_distortion_threshold_master_buffer(int16_t threshold_value);

/*
@brief threshold's getter.
@param bank_index bank index of the sample we want to get the threshold of.
*/
int16_t get_distortion_threshold(uint8_t bank_index);

/*
@brief master threshold's getter.
*/
int16_t get_distortion_threshold_master_buffer();

//================================================================
/*
@brief effects' initializer.
*/
void effects_init();

/*
@brief effect's initializer.
@param in_bank_index bank index of the sample we want to initialize the effects of
*/
void smp_effects_init(int in_bank_index);

#pragma endregion


#endif