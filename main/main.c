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

#define SET_LENGTH 3

QueueSetHandle_t connection_init();

void app_main(void)
{
    sd_reader_init();

    for (int i = 0; i < sample_names_size; i++) {
        printf("%s\n", sample_names[i]);
    }
}

//Setup function
//It prevents polling and eventual prioritization of the different items 
QueueSetHandle_t connection_init() {
    //Creating a queue set, useful for handling multiple queues simultaneously
    QueueSetHandle_t out_set = xQueueCreateSet(SET_LENGTH);

    //Adding the different queues that handle different I/O peripherals
    xQueueAddToSet((QueueSetMemberHandle_t) joystick_queue, out_set);
    xQueueAddToSet((QueueSetMemberHandle_t) pot_queue, out_set);
    xQueueAddToSet((QueueSetMemberHandle_t) pad_queue, out_set);
    return out_set;
}