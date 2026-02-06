#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_mac.h"
#include "driver/gpio.h"
#include "i2s_driver.h"
#include "mixer.h"
#include "effects.h"
#include "playback_mode.h"

#include "pad_section.h"

void app_main(void)
{
    // xTaskCreate(&simpleTask, "simple task", 2048, NULL, 5, NULL);
    // printf("Aspetto...");
    pad_section_init();
    playback_mode_init();
    effects_init();

    i2s_chan_handle_t master = i2s_driver_init();
    create_mixer(master);
}