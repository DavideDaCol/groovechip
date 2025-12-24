#include "i2s_driver.h"
#include <stdint.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "sdkconfig.h"

i2s_chan_handle_t i2s_driver_init(){

    i2s_chan_handle_t out_channel; // master channel for the output

    i2s_chan_config_t out_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&out_chan_cfg, &out_channel, NULL));

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
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(out_channel, &out_port_cfg));

    return out_channel;
}
