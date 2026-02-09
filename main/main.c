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

#define SET_LENGTH 3
#define GEN_MENU_NUM_OPT 2
#define BTN_MENU_NUM_OPT 3
#define SETTINGS_NUM_OPT 2
#define EFFECTS_NUM_OPT 3

QueueSetHandle_t connection_init();
void main_fsm(QueueSetHandle_t in_set);
void joystick_handler(JoystickDir in_dir);
void goto_settings();
void goto_effects();
void goto_selection();
void menu_move(int* index, int max_opt, int direction);
void sink();

typedef enum {
    GEN_MENU,
    BTN_MENU,
    SETTINGS,
    EFFECTS,
} menu_types;

menu_types curr_menu;
int8_t pressed_button = -1;
int pot_diff = 0;

typedef void (*action) (void); 

typedef struct {
    int curr_index;
    int max_size;
    action *js_right_actions;
    action *pt_actions; 
    char** prints;
} menu_t;

action empty_opt2[] = {sink, sink};
action empty_opt3[] = {sink, sink, sink};

/***********************************/
action gen_opt[] = {
    goto_settings,
    goto_effects
};

char gen_prints[][16] = {
    "Settings",
    "Effects"
};

menu_t gen_menu = {
    .curr_index = -1,
    .max_size = GEN_MENU_NUM_OPT,
    .js_right_actions = gen_opt,
    .prints = gen_prints
};
/***********************************/

/***********************************/
action btn_opt[] = {
    goto_settings,
    goto_effects,
    goto_selection
};

char btn_prints[][16] = {
    "Settings",
    "Effects",
    "Select sample"
};

menu_t btn_menu = {
    .curr_index = -1,
    .max_size = BTN_MENU_NUM_OPT,
    .js_right_actions = btn_opt,
    .pot_actions = 
    .prints = btn_prints
};
/***********************************/

/***********************************/
action set_opt[] = {

};

char set_prints[][16] = {
    "Mode: ",
    "Volume: "
};

menu_t settings = {
    .curr_index = -1,
    .max_size = SETTINGS_NUM_OPT,
    .js_right_actions = empty_opt2
};
/***********************************/

/***********************************/
action eff_opt[] = {

};

char eff_prints[][16] = {
    "Bitcrusher: ",
    "Pitch: ",
    "Distortion"
};

menu_t effects = {
    .curr_index = -1,
    .max_size = EFFECTS_NUM_OPT,
    .js_right_actions = empty_opt3
};
/***********************************/

menu_t* menu_navigation[] = {
    &gen_menu,
    &btn_menu,
    &settings,
    &effects,
};



void app_main(void)
{
    // xTaskCreate(&simpleTask, "simple task", 2048, NULL, 5, NULL);
    // printf("Aspetto...");
    pad_section_init();
    playback_mode_init();
    effects_init();

    i2s_chan_handle_t master = i2s_driver_init();
    create_mixer(master);

    QueueSetHandle_t io_queue_set = NULL;
    connection_init(io_queue_set);

}

QueueSetHandle_t connection_init() {
    //Creating a queue set
    QueueSetHandle_t out_set = xQueueCreateSet(SET_LENGTH);

    //Adding the different queues that handle different I/O peripherals
    xQueueAddToSet((QueueSetMemberHandle_t) joystick_queue, out_set);
    xQueueAddToSet((QueueSetMemberHandle_t) playback_evt_queue, out_set);
    xQueueAddToSet((QueueSetMemberHandle_t) pot_queue, out_set);
    return out_set;
}


void main_fsm(QueueSetHandle_t in_set) {
    while(1) {
        //
        QueueSetMemberHandle_t curr_io_queue = xQueueSelectFromSet(in_set, pdMS_TO_TICKS(10)); //TODO: determine the period
        
        if (curr_io_queue == joystick_queue) {

            JoystickDir curr_js;
            xQueueReceive(curr_io_queue, &curr_js, 0);
            joystick_handler(curr_js);  

        } else if (curr_io_queue == playback_evt_queue) {


        } else {
        } 

    }
}

void joystick_handler(JoystickDir in_dir) {
    switch (in_dir){
        case LEFT: js_left_handler(); break;
        case DOWN: js_down_handler(); break;
        case RIGHT: js_right_handler(); break;
        case UP: js_up_handler(); break;
        default: sink();
    }
}

void menu_move(int* index, int max_opt, int direction) {
    // direction: 1 for down, -1 for up
    *index = (*index + direction + max_opt) % max_opt;
}

void goto_settings() {
    settings.curr_index = 0;
    curr_menu = SETTINGS;
}

void goto_effects() {
    effects.curr_index = 0;
    curr_menu = EFFECTS;
}

void js_right_handler() {
    int index = menu_navigation[curr_menu] -> curr_index;
    menu_navigation[curr_menu] -> js_right_actions[index]();
}

void js_left_handler() {
    if (pressed_button != -1) {
        if (curr_menu == BTN_MENU) {
            curr_menu = GEN_MENU;
            pressed_button = -1;
        } else 
            curr_menu = BTN_MENU;
    } else 
        curr_menu = GEN_MENU;
}

void js_up_handler() {
    menu_move(
        &(menu_navigation[curr_menu] -> curr_index), 
        menu_navigation[curr_menu] -> max_size, 
        -1
    );
}

void js_down_handler() {
    menu_move(
        &(menu_navigation[curr_menu] -> curr_index), 
        menu_navigation[curr_menu] -> max_size, 
        1
    );
}

void set_button_pressed(int pad_id) {
    pressed_button = pad_id;
    curr_menu = BTN_MENU;
}

void sink() {
    return;
}

void set_new_pot_value (int* old_val) {
    *old_val += pot_diff;
}

void goto_selection() {
    //TODO
    return;
}