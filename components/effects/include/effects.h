#ifndef EFFECTS_H_
#define EFFECTS_H_
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#define BIT_DEPTH_DEFAULT 16
#define DOWNSAMPLE_DEFAULT 10

//pitch struct
typedef struct{
    float pitch_factor;
} pitch_params_t;

//bit crusher struct
typedef struct {
    bool enabled;
    uint8_t bit_depth;       // 1-16
    uint8_t downsample; // 1-10
    float last_bc_sample;
    int bc_counter;
} bitcrusher_params_t;

//effects container
typedef struct{
    pitch_params_t pitch;
    bitcrusher_params_t bitcrusher;
} effects_t;


effects_t* get_sample_effect(uint8_t sample_id);
//=========================PITCH============================
void init_pitch(uint8_t sample_id);

void set_pitch_factor(uint8_t sample_id, float pitch_factor);

float get_pitch_factor(uint8_t sample_id);

// inline void get_sample_interpolated(sample_t *smp, int16_t *out_L, int16_t *out_R, uint32_t total_frames) {
//==========================================================

//=========================BIT CRUSHER============================

void init_bit_crusher(uint8_t sample_id);

void set_bit_crusher_bit_depth(uint8_t sample_id, uint8_t bit_depth);

void set_bit_crusher_downsample(uint8_t sample_id, uint8_t downsample_value);
//================================================================
void effects_init();



#endif