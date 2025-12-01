#include "i2s_driver.h"
#include <stdint.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "sdkconfig.h"

#define BUFF_SIZE 2048

static i2s_chan_handle_t out_chan;

static void i2s_example_write_task(void *args)
{
    uint8_t *w_buf = (uint8_t *)calloc(1, BUFF_SIZE);
    assert(w_buf); // Check if w_buf allocation success

    /* fills the buffer with fake data */
    for (int i = 0; i < BUFF_SIZE; i += 8) {
        w_buf[i]     = 0x12;
        w_buf[i + 1] = 0x34;
        w_buf[i + 2] = 0x56;
        w_buf[i + 3] = 0x78;
        w_buf[i + 4] = 0x9A;
        w_buf[i + 5] = 0xBC;
        w_buf[i + 6] = 0xDE;
        w_buf[i + 7] = 0xF0;
    }

    size_t w_bytes = BUFF_SIZE;

    /* Preloads the channel with data, this is only for test purposes */
    while (w_bytes == BUFF_SIZE) {
        /* load the target buffer repeatedly, until all the DMA buffers are preloaded */
        ESP_ERROR_CHECK(i2s_channel_preload_data(out_chan, w_buf, BUFF_SIZE, &w_bytes));
    }

    /* Enable the output channel */
    ESP_ERROR_CHECK(i2s_channel_enable(out_chan));
    while (1) {
        /* Write i2s data */
        if (i2s_channel_write(out_chan, w_buf, BUFF_SIZE, &w_bytes, portMAX_DELAY) == ESP_OK) {
            printf("Write Task: i2s write %d bytes\n", w_bytes); // this print is just for testing, blocking I/O should be avoided at all cost
        } else {
            printf("Write Task: i2s write failed\n");
        }
        //tells the rest of the program it has 10 ms to do whatever it wants before the task starts reading the i2s buffer again
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    free(w_buf);
    vTaskDelete(NULL);
}

void i2s_driver_init(){
    i2s_chan_config_t out_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&out_chan_cfg, &out_chan, NULL));

    i2s_std_config_t out_port_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(16000), //clock works at 16kHz
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO), // output data resolution is 32 bits per sample in stereo
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
