#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_mac.h"
#include "include/joystick.h"
#include "fsm.h"


#define ADC_CHANNEL_X ADC_CHANNEL_6
#define ADC_CHANNEL_Y ADC_CHANNEL_7
#define GPIO_SW GPIO_NUM_27

#define THRESH_LOW 800
#define THRESH_UP 3200

#define DEBOUNCE_MS  30

Joystick my_joystick = { 0, 0, 1};

void joystick_init() {

    // configuring the adc for X, Y
    adc_oneshot_chan_cfg_t config = { // TODO .atten = ADC_ATTEN_DB_11 is deprecated -> need to change it
        .bitwidth = ADC_BITWIDTH_DEFAULT, // default is 12 bits -> 12 bits to represent analog input voltage -> not necessary but the "noise" makes less impact
        .atten = ADC_ATTEN_DB_12,        // 12DB makes the measurement range from 0V to 3.3V -> not sure it's necessary to navigate the menu
    };
    
    // channel 6 = gpio34, channel 7 = gpio35
    adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_6, &config);
    adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_7, &config);

    // configuring gpio for SW
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_SW), // this mask is needed not to ignore the GPIO 32 (if a bit in the mask is 0, it is ignored, so we set the 32nd bit to 1)
        .mode = GPIO_MODE_INPUT, // set to input mode
        .pull_up_en = GPIO_PULLUP_ENABLE // if pushed, it's 0, otherwise it's 1 
    };
    gpio_config(&io_conf);

    // creating the task
    xTaskCreate(joystick_task, "joystick_task", 2048, NULL, 3, NULL); // not sure about these parameters (expecially the stack depth), but for now should work
}

void joystick_get_raw(){
    adc_oneshot_read(adc1_handle, ADC_CHANNEL_X, &my_joystick.x);
    adc_oneshot_read(adc1_handle, ADC_CHANNEL_Y, &my_joystick.y);
    my_joystick.sw = gpio_get_level(GPIO_SW);

    // TODO maybe normalize the raw values? -> not needed if we only use the joystick to move in the menu (only up/down/left/right)
}

// converts the voltage to a direction
JoystickDir joystick_get_direction(){
    if (my_joystick.sw == 0){
        return PRESS;
    }
    if (my_joystick.x > THRESH_UP){
        return RIGHT;
    }
    if (my_joystick.x < THRESH_LOW){
        return LEFT;
    }
    if (my_joystick.y > THRESH_UP){
        return UP;
    }
    if (my_joystick.y < THRESH_LOW){
        return DOWN;
    }
    return CENTER;
}

// main task
void joystick_task(void *args){
    // set the "default" value for the last joystick direction
    JoystickDir last_dir = CENTER;

    // polling
    while(1){
        joystick_get_raw();
        JoystickDir new_dir = joystick_get_direction();

        // ignore the repetitive events
        if (new_dir != last_dir){
            send_message_to_fsm_queue(JOYSTICK, new_dir); 
            last_dir = new_dir;
        }

        vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS)); // 30ms should be a good trade off between pollong at a good ratio and not wasting CPU cycles (polling 33Hz)
    }
}