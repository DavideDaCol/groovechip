#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_mac.h"
#include "driver/gpio.h"
#include "pad_section.h"
#define BLINK_GPIO 2

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
    // xTaskCreate(&simpleTask, "simple task", 2048, NULL, 5, NULL);
    printf("Aspetto...");
    init();
}