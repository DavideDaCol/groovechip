#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_mac.h"
#include "driver/gpio.h"
#include "i2s_driver.h"
#include "mixer.h"
#include "playback_mode.h"

#include "pad_section.h"
#define BLINK_GPIO 2

void app_main(void)
{
    // xTaskCreate(&simpleTask, "simple task", 2048, NULL, 5, NULL);
    // printf("Aspetto...");
    playback_mode_init();
    pad_section_init();
    i2s_chan_handle_t master = i2s_driver_init();
    create_mixer(master);
}