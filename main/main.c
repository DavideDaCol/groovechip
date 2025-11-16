#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_mac.h"
#include "driver/gpio.h"
#include <driver/i2s_std.h>
#include "esp_adc/adc_continuous.h"

#define BLINK_GPIO 2
#define BIT_FIDELITY 16
#define SAMPLE_RATE 44100

////config per l'output via i2s
//i2s_std_config_t is2conf = {
//    .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE), //imposta il clock al sample rate di registrazione
//    //info sulla philips mode: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/i2s.html
//    .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(BIT_FIDELITY,I2S_SLOT_MODE_STEREO),//usa la philips mode a 16 bit in stereo
//    .gpio_cfg = {
//
//    }
//};

adc_continuous_handle_t mic_adc = NULL;
adc_continuous_handle_cfg_t adc_config = {
    .max_store_buf_size = 1024,
    .conv_frame_size = 256,
    .flags = {
        .flush_pool = 1
    }
};

void simpleTask(void *parameters){
    int counter = 0;
    while(true){
        printf("esp32 is working, second %i\n", counter);
        counter += 1;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    //crea nuovo handler adc, viene piazzato dentro mic_adc
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &mic_adc));
    xTaskCreate(&simpleTask, "simple task", 2048, NULL, 5, NULL);
}