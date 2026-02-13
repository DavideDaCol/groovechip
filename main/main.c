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
#include "joystick.h"
#include "potentiometer.h"
#include "adc1.h"
#include "fsm.h"
#include "lcd.h"

void app_main(void)
{
    // xTaskCreate(&simpleTask, "simple task", 2048, NULL, 5, NULL);
    // printf("Aspetto...");
    lcd_driver_init();
    adc1_init();
    fsm_init();
    pad_section_init();
    playback_mode_init();
    effects_init();
    potentiometer_init();
    joystick_init();

    i2s_chan_handle_t master = i2s_driver_init();
    create_mixer(master);

}