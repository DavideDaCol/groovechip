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
#include "sd_reader.h"

void app_main(void)
{
    // xTaskCreate(&simpleTask, "simple task", 2048, NULL, 5, NULL);
    // printf("Aspetto...");
    lcd_driver_init();

    char out[] = {BLACK_BOX, BLACK_BOX, BLACK_BOX, BLACK_BOX, BLACK_BOX, 3, '\0'};

    while(1) {

        print_double("Ciao a tutti", out);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }

}