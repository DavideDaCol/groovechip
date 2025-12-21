#include "i2s_driver.h"
#include <stdint.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "sdkconfig.h"
#include "dummy_wav.h"
#include "dummy_wav_2.h"

#define BUFF_SIZE 256

static i2s_chan_handle_t out_chan; // master channel for the output
unsigned const char* wav_data_ptr; // points to the pure wav file
uint32_t wav_data_idx = 0; //keeps track of how far ahead the playback is
unsigned const char* yeah_data_ptr; // points to the pure wav file
uint32_t yeah_data_idx = 0; //keeps track of how far ahead the playback is

// a WAV file is made of a header with info + pure audio samples
// this struct holds that information to ensure everything is set up
// correctly and garbage isn't being loaded into the esp32

typedef struct wav_header_t
    {
      //   RIFF Section    
      char riff_section_id[4];      // literally the letters "RIFF"
      uint32_t size;                // (size of entire file) - 8
      char riff_format[4];          // literally "WAVE" (for our usecase)
      
      //   Format Section    
      char format_id[4];            // letters "fmt"
      uint32_t format_size;         // (size of format section) - 8
      uint16_t fmt_id;              // 1=uncompressed PCM, any other number isn't WAV
      uint16_t num_channels;        // 1=mono,2=stereo
      uint32_t sample_rate;         // 44100, 16000, 8000 etc.
      uint32_t byte_rate;           // sample_rate * channels * (bits_per_sample/8)
      uint16_t block_align;         // channels * (bits_per_sample/8)
      uint16_t bits_per_sample;     // 8,16,24 or 32
    
      // Data Section
      char data_id[4];              // literally the letters "data"
      uint32_t data_size;           // Size of the data that follows
    }WavHeader;


static void i2s_example_write_task(void *args)
{
    //size of the WAV header, it's always the same size in every WAV file
    const uint32_t header_size = 44;

    WavHeader wh1, wh2;

    //copy the WAV header into the struct so we can analyze it
    memcpy(&wh1, WavData, header_size);
    memcpy(&wh2, yeahData, header_size);

    //set the WAV pointer to start at the beginning of the audio samples
    wav_data_ptr = WavData + header_size;
    yeah_data_ptr = yeahData + header_size;

    size_t w_bytes = BUFF_SIZE;

    // create a i2s DMA buffer to write samples without stressing the CPU.
    // Has to be double the chosen size to allow stereo playback
    int16_t *i2s_buf = malloc(BUFF_SIZE * 2 * sizeof(int16_t));
    assert(i2s_buf);

    ESP_ERROR_CHECK(i2s_channel_enable(out_chan));

    // quick and dirty volume control. 
    // WARNING: the stereo out is usually very loud, this value shoud stay below 0.1
    float volume = 0.05f; 

     while (1) {
        for (int i = 0; i < BUFF_SIZE; i++) {

            int16_t left2;
            int16_t right2;

            // in WAV files, left and right samples are sequential
            int16_t left1  = *(int16_t*)(wav_data_ptr + wav_data_idx);
            int16_t right1 = *(int16_t*)(wav_data_ptr + wav_data_idx + 2);
            
            if (yeah_data_idx < wh1.data_size){
                left2  = *(int16_t*)(yeah_data_ptr + yeah_data_idx);
                right2 = *(int16_t*)(yeah_data_ptr + yeah_data_idx + 2);
            
            } else {
                left2 = 0x00;
                right2 = 0x00;
            }

            left1  *= volume;
            right1 *= volume;
            left2  *= volume;
            right2 *= volume;

            // writes the WAV data to the buffer post volume adjustment
            i2s_buf[i * 2] = left1 + left2;
            i2s_buf[i * 2 + 1] = right1 + right2;

            wav_data_idx += 4;
            yeah_data_idx += 4;

            // loop the audio file
            if (wav_data_idx >= wh1.data_size) {
                wav_data_idx = 0;
                yeah_data_idx = 0;
            }
        }

        // Write full buffer (1024 bytes)
        ESP_ERROR_CHECK(i2s_channel_write(out_chan, i2s_buf,
                                          BUFF_SIZE * 2 * sizeof(int16_t),
                                          &w_bytes,
                                          portMAX_DELAY));
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    free(i2s_buf);
    vTaskDelete(NULL);
}

void i2s_driver_init(){
    i2s_chan_config_t out_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&out_chan_cfg, &out_chan, NULL));

    i2s_std_config_t out_port_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(16000), //ideally should match the WAV files
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO), // data resolution is 16 bits per sample, stereo
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,    // some codecs may require a master clock signal
            .bclk = GRVCHP_OUT_BCLK,
            .ws   = GRVCHP_OUT_WS,
            .dout = GRVCHP_OUT_DOUT,
            .din  = GRVCHP_OUT_DIN,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv   = false,
            },
        },
    };

    //init the i2s channel with the given configs
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(out_chan, &out_port_cfg));

    xTaskCreate(i2s_example_write_task, "i2s_example_write_task", 4096, NULL, 5, NULL);
}
