#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_mac.h"
#include "driver/gpio.h"
#include "pad_section.h"
#define BLINK_GPIO 2

void app_main(void)
{
    // xTaskCreate(&simpleTask, "simple task", 2048, NULL, 5, NULL);
    // printf("Aspetto...");
    pad_section_init();
}