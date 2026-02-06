#include "microphone.h"

sample_t record_sample_task(void *args) {

    i2s_chan_handle_t in_channel = (i2s_chan_handle_t)args[0];
    assert(in_channel);

    uint32_t pb_mode = (uint32_t)args[1];
    assert(pb_mode);

    int gpio_sample = (int)args[2];
    assert(gpio_sample);

    interrupt_mask(gpio_sample);

    unsigned char sample_rec[BUFF_SIZE * 2 * sizeof(int16_t)];

    while(1){
        // TODO mask the other GPIO 
        if (xQueueReceive(pads_evt_queue, &evt, portMAX_DELAY)){
            
        }
    }
    
    
    
    sample_t out_sample = {
        .header = {
            .riff_section_id[4] = "RIFF";      // literally the letters "RIFF"
            .riff_format[4] = "WAVE";          // literally "WAVE"
                
            .format_id[4] = "fmt ";            // letters "fmt"
            .format_size ;         // (size of format section) - 8
            .fmt_id = 1;              // 1=uncompressed PCM, any other number isn't WAV
            .num_channels = 2;        // 1=mono,2=stereo
            .sample_rate = 15000;         // 44100, 16000, 8000 etc.
            .byte_rate = 60000;           // sample_rate * channels * (bits_per_sample/8)
            .block_align = 4;         // channels * (bits_per_sample/8)
            .bits_per_samplen = 16;     // 8,16,24 or 32
            
            .data_id[4] "data";              // literally the letters "data"
            .data_size;           // Size of the data that follows
            },
        .playback_mode = pb_mode,
        .pad_id = gpio_sample,
        .sample_id = get_id(), // TODO
    }

    interrupt_enable();
}

// masks every gpio interrupt besides the pad_id
void interrupt_mask(int pad_id){
    const uint8_t button_pins[8] = {
        GPIO_BUTTON_1,
        GPIO_BUTTON_2,
        GPIO_BUTTON_3,
        GPIO_BUTTON_4,
        GPIO_BUTTON_5,
        GPIO_BUTTON_6,
        GPIO_BUTTON_7,
        GPIO_BUTTON_8
    };

    for(int i = 0; i < 8; i++){
        gpio_intr_disable(button_pins[i]);
    }
    gpio_intr_enable(pad_id);
}

// enables every gpio interrupt
void interrupt_enable(){
    const uint8_t button_pins[8] = {
        GPIO_BUTTON_1,
        GPIO_BUTTON_2,
        GPIO_BUTTON_3,
        GPIO_BUTTON_4,
        GPIO_BUTTON_5,
        GPIO_BUTTON_6,
        GPIO_BUTTON_7,
        GPIO_BUTTON_8
    };

    for(int i = 0; i < 8; i++){
        gpio_intr_enable(button_pins[i]);
    }
}