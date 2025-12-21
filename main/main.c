#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_mac.h"
#include "driver/gpio.h"
#include "joystick.h"

#define BLINK_GPIO 2

void app_main(void)
{
    joystick_init();
    joystick_dir_t dir;
    while(1){
        if (xQueueReceive(joystick_queue, &dir, portMAX_DELAY)){ // portMAX_DELAY tells to block forever until something happens
            switch (dir) {
                case UP:    printf("UP\n"); break;
                case DOWN:  printf("DOWN\n"); break;
                case LEFT:  printf("LEFT\n"); break;
                case RIGHT: printf("RIGHT\n"); break;
                case PRESS: printf("PRESS\n"); break;
                case CENTER: printf("CENTER\n"); break;
                default: break;
            }
        }
    }
}