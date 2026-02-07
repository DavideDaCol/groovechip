#ifndef MIXER_H_
#define MIXER_H_

#include "i2s_driver.h"
#include <stdint.h>
#include "esp_log.h"
#include "pad_section.h"
#include "playback_mode.h"


// Size of the wav header, must be stripped before playing
#define WAV_HDR_SIZE 44

// Size of the buffer to dump in the i2s driver every cycle
#define BUFF_SIZE 256

// Maximum number of available samples
#define SAMPLE_NUM 8

#pragma region TYPES

// Type used to store the metadata of a WAV file

typedef struct wav_header_t
    {
      char riff_section_id[4];      // literally the letters "RIFF"
      uint32_t size;                // (size of entire file) - 8
      char riff_format[4];          // literally "WAVE"
         
      char format_id[4];            // letters "fmt"
      uint32_t format_size;         // (size of format section) - 8
      uint16_t fmt_id;              // 1=uncompressed PCM, any other number isn't WAV
      uint16_t num_channels;        // 1=mono,2=stereo
      uint32_t sample_rate;         // 44100, 16000, 8000 etc.
      uint32_t byte_rate;           // sample_rate * channels * (bits_per_sample/8)
      uint16_t block_align;         // channels * (bits_per_sample/8)
      uint16_t bits_per_sample;     // 8,16,24 or 32
    
      char data_id[4];              // literally the letters "data"
      uint32_t data_size;           // Size of the data that follows
    } wav_header_t;

/**
 * @brief Keeps track of currently playing samples
 *
 * Binary bit mask used to track the now playing status of 
 * each sample. must be set via bit shift operations
 */
typedef uint8_t sample_bitmask; 


/**
 * @brief Sample metadata struct
 *
 * Used to keep together the raw pointer to the sample bytes
 * and other necessary metadata to aid for playback. 
 */
typedef struct sample_t
{
    const unsigned char *raw_data; /** raw sample bytes */
    wav_header_t header; /** contains sample metadata like size and bit rate */
    float playback_ptr; /** progress indicator for the sample */
    // playback_mode_t playback_mode; /** sample play type: ONESHOT, LOOP, etc... */
    int sample_id;
    bool playback_finished;
} sample_t;

typedef struct metronome {
    bool state; /* metronome on or off */
    float bpm; /* Beats Per Minute */
    int8_t subdivisions; /* How many mentronome clicks in a beat */
    float samples_per_subdivision; /* How many samples are supposed to be played before the next metronome click*/
    bool playback_enabled; /* whether the metronome "click" should play or not */
} metronome;

// all samples that can be played
extern sample_t sample_bank[SAMPLE_NUM];

#pragma endregion

//Sample actions
void action_start_sample(int);
void action_start_or_stop_sample(int);
void action_stop_sample(int);
void action_restart_sample(int);
void action_ignore(int);

//metronome actions
void init_metronome();
void toggle_metronome_state(bool);
void set_metronome_bpm(float);
void set_metronome_subdiv(int);
void set_metronome_tick();
void toggle_metronome_playback(bool);

void create_mixer(i2s_chan_handle_t channel);

#endif
