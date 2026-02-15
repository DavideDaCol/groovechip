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

// Max volume
#define VOLUME_THRESHOLD_UP 1.0f
#define MASTER_VOLUME_THRESHOLD_UP 2.0f

// Max metronome bpm
#define MAX_METRONOME_BPM 210.0f
#define MIN_MOTRONOME_BPM 40.0f
#define BASE_METRONOME_VALUE 40.0f

#define METRONOME_NORMALIZER 1.7f
#define METRONOME_SCALE_VALUE 5.0f

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
    unsigned char *raw_data; /** raw sample bytes */
    wav_header_t header; /** contains sample metadata like size and bit rate */
    uint32_t total_frames; /* frame number (data size / 4)*/
    float playback_ptr; /** progress indicator for the sample */
    float start_ptr; /* the playback_ptr get initialized to this value every time the sample play */
    uint32_t end_ptr; /* limit the sample duration */
    // playback_mode_t playback_mode; /** sample play type: ONESHOT, LOOP, etc... */
    int bank_index;
    bool playback_finished;
    float volume;

} sample_t;

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

// all samples that can be played
extern sample_t* sample_bank[SAMPLE_NUM];

#pragma endregion

//Sample actions
void action_start_sample(int);
void action_start_or_stop_sample(int);
void action_stop_sample(int);
void action_restart_sample(int);
void action_ignore(int);
//chopping
void set_sample_end_ptr(uint8_t, uint32_t);
void set_sample_start_ptr(uint8_t, float);
uint32_t get_sample_end_ptr(uint8_t bank_index);
uint32_t get_sample_start_ptr(uint8_t bank_index);
uint32_t get_sample_total_frames(uint8_t bank_index);

// volume
void set_volume(uint8_t, float);
float get_volume(uint8_t);

extern float volume_master_buffer;
void set_master_buffer_volume(float);
float get_master_volume();

//metronome actions
void init_metronome();
void set_metronome_state(bool);
void set_metronome_bpm(float);
void set_metronome_subdiv(int);
void set_metronome_tick();
void set_metronome_playback(bool);
bool get_metronome_playback();
bool get_metronome_state();
float get_metronome_bpm();

void create_mixer(i2s_chan_handle_t channel);

#endif
