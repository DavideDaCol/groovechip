#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_mac.h"
#include "driver/gpio.h"
#include "pad_section.h"
#include "potentiometer.h"
#define BLINK_GPIO 2

void app_main(void) {
    potentiometer_init();
    int pot_value;
    while(1){
        if (xQueueReceive(pot_queue, &pot_value, portMAX_DELAY)){ // portMAX_DELAY tells to block forever until something happens
            // this receives from the potentiometer queue -> pot_value is the raw value (0-4095) but it can be 
            // changed to be the percentage or the voltage (those functions are already implemented in potentiometer.c)
        }
    }

}