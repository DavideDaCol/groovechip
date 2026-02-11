#include "fsm.h"

menu_types curr_menu;

int8_t pressed_button = -1;

mode_t mode = ONESHOT;

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
        .pt_action = rotate_mode,
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
        .pt_action = toggle_bit_crusher_menu,
    },
    {
        .print = "Pitch: ",
        .js_right_action = sink, 
        .pt_action = change_pitch,
    },
    {
        .print = "Distortion: ",
        .js_right_action = sink,
        .pt_action = toggle_distortion_menu,
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

void potentiometer_handler(int diff_percent_pot_value){
    int curr_index = menu_navigation[curr_menu] -> curr_index;
    (menu_navigation[curr_menu] -> opt_handlers[curr_index]).pt_action(diff_percent_pot_value);
}

void change_vol(int pot_value) {
    if (pressed_button < 0) {
        for (uint8_t i = 0; i < SAMPLE_NUM; i++)
            set_volume(i, (float)pot_value * VOLUME_NORMALIZER_VALUE);
    } else
        set_volume((uint8_t)pressed_button, (float)pot_value * VOLUME_NORMALIZER_VALUE);
}

void change_pitch(int pot_value){
    if (pressed_button < 0) {
        for (uint8_t i = 0; i < SAMPLE_NUM; i++) {
            float sample_i_pitch_factor = get_pitch_factor(i);
            set_pitch_factor(i, sample_i_pitch_factor + (float)pot_value * PITCH_NORMALIZER_VALUE);
        }
    } else {
        float sample_i_pitch_factor = get_pitch_factor(pressed_button);
        set_pitch_factor(pressed_button, sample_i_pitch_factor + (float)pot_value * PITCH_NORMALIZER_VALUE);
    }
}

void toggle_bit_crusher_menu(int pot_value){
    bool bit_crusher = false;
    if (pot_value >= 0){
        bit_crusher = true;
    }
    if (pressed_button < 0) {
        for (uint8_t i = 0; i < SAMPLE_NUM; i++){
            toggle_bit_crusher(i, bit_crusher);
        }
    } else {
        toggle_bit_crusher(pressed_button, bit_crusher);
    }
}

void toggle_distortion_menu(int pot_value){
    bool distortion = false;
    if (pot_value >= 0){
        distortion = true;
    }
    if (pressed_button < 0) {
        for (uint8_t i = 0; i < SAMPLE_NUM; i++){
            toggle_distortion(i, distortion);
        }
    } else {
        toggle_distortion(pressed_button, distortion);
    }
}

void rotate_mode(int pot_value){
    int next = -1;
    if (pot_value > 0){
        next = 1;
    }
    if (pressed_button < 0) {
        for (uint8_t i = 0; i < SAMPLE_NUM; i++){
            set_playback_mode(i, next_mode(next, mode));
        }
    } else {
        set_playback_mode(pressed_button, next_mode(next, get_playback_mode(pressed_button)));
    }
}

mode_t next_mode(int next, mode_t curr_mode){
    return (mode_t)((curr_mode + next + 3)%3);
}