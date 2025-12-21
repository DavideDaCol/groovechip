#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_mac.h"
#include "joystick.h"


#define ADC_CHANNEL_X ADC_CHANNEL_6
#define ADC_CHANNEL_Y ADC_CHANNEL_7
#define GPIO_SW GPIO_NUM_32

#define THRESH_LOW 1200
#define THRESH_UP 2800

#define DEBOUNCE_MS  30

joystick_t my_joystick = { 0, 0, 1};
QueueHandle_t joystick_queue;
adc_oneshot_unit_handle_t adc1_handle;

void joystick_init() {
    // initialize ADC unit 1
    adc_oneshot_unit_init_cfg_t init_config = {.unit_id = ADC_UNIT_1};
    adc_oneshot_new_unit(&init_config, &adc1_handle);

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

    // creating the queue
    joystick_queue = xQueueCreate(10, sizeof(joystick_dir_t)); // don't know how many messages the queue should be able to contain at a time, i put 10 as a starting point (can be changed)

    // creating the task
    xTaskCreate(joystick_task, "joystick_task", 2048, NULL, 5, NULL); // not sure about these parameters (expecially the stack depth), but for now should work
}

void joystick_get_raw(){
    adc_oneshot_read(adc1_handle, ADC_CHANNEL_X, &my_joystick.x);
    adc_oneshot_read(adc1_handle, ADC_CHANNEL_Y, &my_joystick.y);
    my_joystick.sw = gpio_get_level(GPIO_SW);

    // TODO maybe normalize the raw values? -> not needed if we only use the joystick to move in the menu (only up/down/left/right)
}

// converts the voltage to a direction
joystick_dir_t joystick_get_direction(){
    if (my_joystick.sw == 0){
        return PRESS;
    }
    if (my_joystick.x > THRESH_UP){
        return LEFT;
    }
    if (my_joystick.x < THRESH_LOW){
        return RIGHT;
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
    joystick_dir_t last_dir = CENTER;

    // polling
    while(1){
        joystick_get_raw();
        joystick_dir_t new_dir = joystick_get_direction();

        // ignore the repetitive events
        if (new_dir != last_dir){
            xQueueSend(joystick_queue, &new_dir, 0); // parameters -> queue name, message, tick to wait to send the message
            last_dir = new_dir;
        }

        vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS)); // 30ms should be a good trade off between pollong at a good ratio and not wasting CPU cycles (polling 33Hz)
    }
}