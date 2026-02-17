#ifndef EXAMPLE_H_
#define EXAMPLE_H_

#include "mixer.h"

typedef struct metronome {
    bool state; /* metronome on or off */
    float bpm; /* Beats Per Minute */
    int8_t subdivisions; /* How many mentronome clicks in a beat */
    float samples_per_subdivision; /* How many samples are supposed to be played before the next metronome click*/
    bool playback_enabled; /* whether the metronome "click" should play or not */
    int16_t playback_ptr; /* track the playback of the metronome click */
    const unsigned char *raw_data; /* the actual metronome audio */
    wav_header_t header; /* needed to determine when to stop playback */
} metronome;

void init_metronome();

void set_metronome_state(bool new_state);

bool get_metronome_state();

void set_metronome_bpm(float new_bpm);

float get_metronome_bpm();

void set_metronome_subdiv(int new_subdiv);

int8_t get_metronome_subdiv();

void set_metronome_tick();

void set_metronome_playback(bool new_state);

bool get_metronome_playback();

float get_samples_per_subdiv();

bool get_metronome_playback();

int16_t advance_metronome_audio();

bool is_metronome_tick();

void reset_mtrn();

void advance_metronome_ptr();

#endif
