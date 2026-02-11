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

#define SET_LENGTH 3
#define GEN_MENU_NUM_OPT 2
#define BTN_MENU_NUM_OPT 4
#define SETTINGS_NUM_OPT 2
#define EFFECTS_NUM_OPT 3

QueueSetHandle_t connection_init();
void main_fsm(QueueSetHandle_t in_set);
void joystick_handler(JoystickDir in_dir);
void goto_settings();
void goto_effects();
void goto_selection();
void menu_move(int* index, int max_opt, int direction);
void js_right_handler();
void js_left_handler();
void js_up_handler();
void js_down_handler();
void sink();
void set_button_pressed(int pad_id);
void set_new_pot_value (int* old_val);
void save();
void change_vol();
void change_btn_vol(int pad_id);

//Enum that describes every type of menu we have in our project
typedef enum {
    GEN_MENU,
    BTN_MENU,
    SETTINGS,
    EFFECTS,
} menu_types;

//Menu that we are currently navigating
menu_types curr_menu;

//Records the last pressed button, which is useful to know the button to apply the changes to 
int8_t pressed_button = -1;

//Actual function to perform when an input is received
typedef void (*action) (void); 

//Interaction with a single voice of the menu
typedef struct {
    char print[16];         //String to print on screen for each voice of the menu
    action js_right_action; //Function to execute for each voice when a right shift of the joystick is detected
    action pt_action;       //Function to execute for each voice when a potentiometer action is detected 
} opt_interactions_t;

//Datatype that represent a single menu (between the one enlisted above)
typedef struct {
    int curr_index;                     //Current position in the menu
    int max_size;                       //Actual size of the menu
    opt_interactions_t* opt_handlers;   //Array of handlers for each voice
} menu_t;

/***********************************
GENERAL MENU 
***********************************/
opt_interactions_t gen_handlers[] = {
    {
        .print = "Settings",
        .js_right_action = goto_settings,
        .pt_action = sink
    }, 
    {
        .print = "Effects",
        .js_right_action = goto_effects,
        .pt_action = sink
    }
};

menu_t gen_menu = {
    .curr_index = -1,
    .max_size = GEN_MENU_NUM_OPT,
    .opt_handlers = gen_handlers
};
/***********************************/

/***********************************
BUTTON SPECIFIC MENU 
***********************************/
opt_interactions_t btn_handlers[] = {
    {
        .print = "Settings",
        .js_right_action = goto_settings,
        .pt_action = sink
    },
    {
        .print = "Effects",
        .js_right_action = goto_effects,
        .pt_action = sink
    }, 
    {
        .print = "Select sample",
        .js_right_action = sink, //TODO
        .pt_action = sink
    }, 
    {
        .print = "Save changes",
        .js_right_action = sink, //TODO
        .pt_action = sink
    }
};

menu_t btn_menu = {
    .curr_index = -1,
    .max_size = BTN_MENU_NUM_OPT,
    .opt_handlers = btn_handlers
};
/***********************************/

/***********************************
SETTINGS MENU (structure is the same whether it's related to a specific button or is general) 
***********************************/
opt_interactions_t set_handlers[] = {
    {
        .print = "Mode",
        .js_right_action = sink,
        .pt_action = sink, //TODO
    },
    {
        .print = "Volume",
        .js_right_action = sink, 
        .pt_action = change_vol,
    }
};

menu_t settings = {
    .curr_index = -1,
    .max_size = SETTINGS_NUM_OPT,
    .opt_handlers = set_handlers
};
/***********************************/

/***********************************
EFFECTS MENU (structure is the same whether it's related to a specific button or is general) 
***********************************/
opt_interactions_t eff_handlers[] = {
    {
        .print = "Bitcrusher: ",
        .js_right_action = sink,
        .pt_action = sink, //TODO
    },
    {
        .print = "Pitch: ",
        .js_right_action = sink, 
        .pt_action = sink, //TODO
    },
    {
        .print = "Distortion: ",
        .js_right_action = sink,
        .pt_action = sink, //TODO
    }
};

menu_t effects = {
    .curr_index = -1,
    .max_size = EFFECTS_NUM_OPT,
    .opt_handlers = eff_handlers
};
/***********************************/

//Menu collection, essential for the navigation
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
    adc1_init();
    pad_section_init();
    playback_mode_init();
    effects_init();
    potentiometer_init();
    joystick_init();

    i2s_chan_handle_t master = i2s_driver_init();
    create_mixer(master);

    QueueSetHandle_t io_queue_set = connection_init();

    main_fsm(io_queue_set);
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


//FSM implementation function
void main_fsm(QueueSetHandle_t in_set) {
    while(1) {
        //
        QueueSetMemberHandle_t curr_io_queue = xQueueSelectFromSet(in_set, pdMS_TO_TICKS(50)); //TODO: determine the period
        printf("%d\n", curr_menu);
        
        if (curr_io_queue == joystick_queue) {

            JoystickDir curr_js;
            xQueueReceive(curr_io_queue, &curr_js, 0);
            joystick_handler(curr_js);  

        } else if (curr_io_queue == pad_queue) {
            pad_queue_msg_t curr_pad;
            xQueueReceive(curr_io_queue, &curr_pad, 0);
            set_button_pressed(curr_pad.pad_id);

        } else if (curr_io_queue == pot_queue){
            int diff_percent_pot_value;
            xQueueReceive(curr_io_queue, &diff_percent_pot_value, 0);
            
        } 

    }
}

//Joystick handler implementation 
void joystick_handler(JoystickDir in_dir) {
    switch (in_dir){
        case LEFT: js_left_handler(); break;
        case DOWN: js_down_handler(); break;
        case RIGHT: js_right_handler(); break;
        case UP: js_up_handler(); break;
        default: sink();
    }
}

//Atomic function to navigate the menu
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
    menu_navigation[curr_menu] -> opt_handlers[index].js_right_action();
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

//Function to call when a button bound to a sample is pressed
void set_button_pressed(int pad_id) {
    pressed_button = pad_id;
    curr_menu = BTN_MENU;
}

void sink() {
    return;
}

void goto_selection() {
    //TODO
    return;
}

void change_vol(float volume_to_add) {
    if (pressed_button < 0) {
        for (uint8_t i = 0; i < SAMPLE_NUM; i++)
            set_volume(i, volume_to_add);
    } else
        change_btn_vol((uint8_t)pressed_button);
}