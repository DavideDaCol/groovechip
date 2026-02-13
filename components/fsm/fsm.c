#include "fsm.h"
#include "sd_reader.h"
#include <math.h>

menu_types curr_menu = GEN_MENU;

uint8_t pressed_button = NOT_DEFINED;

mode_t mode = ONESHOT;

void get_second_line(char *);

const char* TAG_FSM = "FSM";

#pragma region GENERAL MENU
/***********************************
GENERAL MENU 
***********************************/
opt_interactions_t gen_handlers[] = {
    {
        .first_line = "Settings",
        .second_line = get_second_line,
        .js_right_action = goto_settings,
        .pt_action = sink
    }, 
    {
        .first_line = "Effects",
        .second_line = get_second_line,
        .js_right_action = goto_effects,
        .pt_action = sink
    }
};

menu_t gen_menu = {
    .curr_index = 0,
    .max_size = GEN_MENU_NUM_OPT,
    .opt_handlers = gen_handlers
};
/***********************************/
#pragma endregion

#pragma region BUTTON SPECIFIC MENU
/***********************************
BUTTON SPECIFIC MENU 
***********************************/
opt_interactions_t btn_handlers[] = {
    {
        .first_line = "Settings",
        .second_line = get_second_line,
        .js_right_action = goto_settings,
        .pt_action = sink
    },
    {
        .first_line = "Effects",
        .second_line = get_second_line,
        .js_right_action = goto_effects,
        .pt_action = sink
    }, 
    {
        .first_line = "Select sample",
        .second_line = get_second_line,
        .js_right_action = sink, //TODO
        .pt_action = sink
    }, 
    {
        .first_line = "Save changes",
        .second_line = get_second_line,
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
#pragma endregion

#pragma region SETTINGS MENU
/***********************************
SETTINGS MENU (structure is the same whether it's related to a specific button or is general) 
***********************************/
opt_interactions_t set_handlers[] = {
    {
        .first_line = "Mode",
        .second_line = get_second_line,
        .js_right_action = sink,
        .pt_action = rotate_mode,
    },
    {
        .first_line = "Volume",
        .second_line = get_second_line,
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
#pragma endregion

#pragma region EFFECTS MENU
/***********************************
EFFECTS MENU (structure is the same whether it's related to a specific button or is general) 
***********************************/
opt_interactions_t eff_handlers[] = {
    {
        .first_line = "Bitcrusher",
        .second_line = get_second_line,
        .js_right_action = goto_bitcrusher,
        .pt_action = sink,
    },
    {
        .first_line = "Pitch",
        .second_line = get_second_line,
        .js_right_action = goto_pitch,
        .pt_action = sink,
    },
    {
        .first_line = "Distortion",
        .second_line = get_second_line,
        .js_right_action = goto_distortion,
        .pt_action = sink,
    }
};

menu_t effects = {
    .curr_index = -1,
    .max_size = EFFECTS_NUM_OPT,
    .opt_handlers = eff_handlers
};
/***********************************/
#pragma endregion

#pragma region BITCRUSHER MENU
/***********************************
BITCRUSHER MENU (structure is the same whether it's related to a specific button or is general) 
***********************************/

opt_interactions_t bit_crusher_handlers[] = {
    {
        .first_line = "Bitcrusher: ",
        .second_line = get_second_line,
        .js_right_action = sink,
        .pt_action = toggle_bit_crusher_menu,
    },
    {
        .first_line = "Bit depth: ",
        .second_line = get_second_line,
        .js_right_action = sink, 
        .pt_action = change_bit_depth,
    },
    {
        .first_line = "Downsample: ",
        .second_line = get_second_line,
        .js_right_action = sink,
        .pt_action = change_downsample,
    }
};

menu_t bit_crusher_menu = {
    .curr_index = -1,
    .max_size = BITCRUSHER_NUM_OPT,
    .opt_handlers = bit_crusher_handlers
};
/***********************************/
#pragma endregion

#pragma region PITCH MENU
/***********************************
PITCH MENU (structure is the same whether it's related to a specific button or is general) 
***********************************/

opt_interactions_t pitch_handlers[] = {
    {
        .first_line = "Pitch: ",
        .second_line = get_second_line,
        .js_right_action = sink,
        .pt_action = change_pitch,
    }
};

menu_t pitch_menu = {
    .curr_index = -1,
    .max_size = PITCH_NUM_OPT,
    .opt_handlers = pitch_handlers
};
/***********************************/
#pragma endregion

#pragma region DISTORTION MENU
/***********************************
DISTORTION MENU (structure is the same whether it's related to a specific button or is general) 
***********************************/

opt_interactions_t distortion_handlers[] = {
    {
        .first_line = "Distortion: ",
        .second_line = get_second_line,
        .js_right_action = sink,
        .pt_action = toggle_distortion_menu,
    },
    {
        .first_line = "Gain: ",
        .second_line = get_second_line,
        .js_right_action = sink, 
        .pt_action = change_distortion_gain,
    },
    {
        .first_line = "Threshold: ",
        .second_line = get_second_line,
        .js_right_action = sink,
        .pt_action = change_distortion_threshold,
    }
};

menu_t distortion_menu = {
    .curr_index = -1,
    .max_size = DISTORTION_NUM_OPT,
    .opt_handlers = distortion_handlers
};
/***********************************/
#pragma endregion

//Menu collection, essential for the navigation
menu_t* menu_navigation[] = {
    &gen_menu,
    &btn_menu,
    &settings,
    &effects,
    &bit_crusher_menu,
    &pitch_menu,
    &distortion_menu,
    NULL //TODO questa deve corrispondere al menu di sample load a runtime
};


void get_second_line(char* out){
    uint8_t bank_index = get_sample_bank_index(pressed_button);
    printf("PressedButton: %u\n", pressed_button);
    printf("Bank index: %u\n", bank_index);
    switch (curr_menu)
    {
    case BITCRUSHER:
        if(bank_index == NOT_DEFINED) break; // if there is no associated sample_id, exit
        uint8_t value;
        switch (menu_navigation[curr_menu]->curr_index)
        {
        // retrieve current value set res
        case ENABLED_BC:
            if(get_bit_crusher_state(bank_index)){
                sprintf(out, "On");
            }
            else sprintf(out, "Off");
            break;

        case BIT_DEPTH:
            value = get_bit_crusher_bit_depth(bank_index);
            printf("BD: %u", value);
            sprintf(out, "%u", value);
            break;

        case DOWNSAMPLE:
            value = get_bit_crusher_downsample(bank_index);
            printf("DS: %u", value);
            sprintf(out, "%u", value);
            break;
            
        default:
            break;
        }
        break;
    case PITCH:
        break;
    case DISTORTION:
        break;
    case SETTINGS:
        break;
    
    default:
        break;
    }
}

menu_t sample_load_menu = {};
opt_interactions_t* sample_load_actions = NULL;

#pragma region MAIN FSM
//FSM implementation function
void main_fsm(QueueSetHandle_t in_set) {

    sample_load_actions = (opt_interactions_t *)malloc(sample_names_size * sizeof(opt_interactions_t));
    //for every sample, create the menu options
    for(int i = 0; i < sample_names_size; i++){
        sample_load_actions[i].js_right_action = sink;
        sprintf(sample_load_actions[i].print,sample_names[i]);
        sample_load_actions[i].pt_action = sink;
    }
    // create the menu for the samples in the SD card
    sample_load_menu.curr_index = -1;
    sample_load_menu.max_size = sample_names_size;
    sample_load_menu.opt_handlers = sample_load_actions;

    // TODO make this nicer
    menu_navigation[7] = &sample_load_menu;

    while(1) {
        QueueSetMemberHandle_t curr_io_queue = xQueueSelectFromSet(in_set, pdMS_TO_TICKS(50)); //TODO: determine the period
        bool is_changed = false;
        if(curr_io_queue == NULL) continue;

        if (curr_io_queue == joystick_queue) {
            JoystickDir curr_js;
            if(xQueueReceive(curr_io_queue, &curr_js, 0) == pdFALSE){
                ESP_LOGE(TAG_FSM, "Error: unable to read the joystick_queue");
                continue;
            }
            joystick_handler(curr_js);  
            if(curr_js != CENTER){
                is_changed = true;
            }

        } else if (curr_io_queue == pad_queue) {
            pad_queue_msg_t curr_pad;
            if(xQueueReceive(curr_io_queue, &curr_pad, 0) == pdFALSE){
                ESP_LOGE(TAG_FSM, "Error: unable to read the pad_queue");
                continue;
            }

            set_button_pressed(curr_pad.pad_id);

            printf("CurrPadId: %d\n", curr_pad.pad_id);
            
            // char* line1 = (menu_navigation[curr_menu]->opt_handlers[menu_navigation[curr_menu]->curr_index]).first_line;
            // char line2[17] = "";
                
            // (menu_navigation[curr_menu]->opt_handlers[menu_navigation[curr_menu]->curr_index]).second_line(line2);
            // print_double(line1, line2);
            is_changed = true;

        } else if (curr_io_queue == pot_queue){
            int diff_percent_pot_value;
            if(xQueueReceive(curr_io_queue, &diff_percent_pot_value, 0) == pdFALSE){
                ESP_LOGE(TAG_FSM, "Error: unable to read the pad_queue");
                continue;
            }
            printf("diff_percent: %d\n", diff_percent_pot_value);
            potentiometer_handler(diff_percent_pot_value);
            is_changed = true;
        } 

        if(is_changed){
            char* line1 = (menu_navigation[curr_menu]->opt_handlers[menu_navigation[curr_menu]->curr_index]).first_line;
            char line2[17] = "";
            
            (menu_navigation[curr_menu]->opt_handlers[menu_navigation[curr_menu]->curr_index]).second_line(line2);
            print_double(line1, line2);
            is_changed = false;
        }
    }
}
#pragma endregion

#pragma region MENU NAVIGATION
//Joystick handler implementation 
void joystick_handler(JoystickDir in_dir) {
    switch (in_dir){
        case LEFT: js_left_handler(); break;
        case DOWN: js_down_handler(); break;
        case RIGHT: js_right_handler(); break;
        case UP: js_up_handler(); break;
        default: sink(); return;
    }
    printf("%s\n", (menu_navigation[curr_menu]->opt_handlers[menu_navigation[curr_menu]->curr_index]).first_line);

}

//Atomic function to navigate the menu
void menu_move(int* index, int max_opt, int direction) {
    // direction: 1 for down, -1 for up
    *index = (*index + direction + max_opt) % max_opt;
}

void goto_sample_load() {
    sample_load_menu.curr_index = 0;
    curr_menu = SAMPLE_LOAD;
}

// Atomic function that sets the current menu and the current index
void goto_settings() {
    settings.curr_index = 0;
    curr_menu = SETTINGS;
}

// Atomic function that sets the current menu and the current index
void goto_effects() {
    effects.curr_index = 0;
    curr_menu = EFFECTS;
}

// Atomic function that sets the current menu and the current index
void goto_bitcrusher() {
    bit_crusher_menu.curr_index = 0;
    curr_menu = BITCRUSHER;
}

// Atomic function that sets the current menu and the current index
void goto_pitch() {
    pitch_menu.curr_index = 0;
    curr_menu = PITCH;
}

// Atomic function that sets the current menu and the current index
void goto_distortion() {
    distortion_menu.curr_index = 0;
    curr_menu = DISTORTION;
}

void sample_load() {
    int index = menu_navigation[curr_menu] -> curr_index;
    ld_sample(pressed_button, sample_names[index], &sample_bank[pressed_button]);
}

// Function that calls the correct handler based on the current menu
void js_right_handler() {
    int index = menu_navigation[curr_menu] -> curr_index;
    menu_navigation[curr_menu] -> opt_handlers[index].js_right_action();
}

// Function that sets the current menu the the previoous one
void js_left_handler() {
    if (pressed_button != NOT_DEFINED) {
        if (curr_menu == BTN_MENU) {
            curr_menu = GEN_MENU;
            pressed_button = NOT_DEFINED;
        } else 
            curr_menu = BTN_MENU;
    } else 
        curr_menu = GEN_MENU;
}

// Function that lets the menu navigation up
void js_up_handler() {
    menu_move(
        &(menu_navigation[curr_menu] -> curr_index), 
        menu_navigation[curr_menu] -> max_size, 
        -1
    );
}

// Function that lets the menu navigation down
void js_down_handler() {
    menu_move(
        &(menu_navigation[curr_menu] -> curr_index), 
        menu_navigation[curr_menu] -> max_size, 
        1
    );
}

//Function to call when a button bound to a sample is pressed
void set_button_pressed(int pad_id) {
    if(pad_id != pressed_button){
        pressed_button = pad_id;
        btn_menu.curr_index = 0;
        curr_menu = BTN_MENU;
    }
}

// Sing function
void sink() {
    return;
}

#pragma endregion


#pragma region POTENTIOMETER HANDLING

// Function that handles the potentiometer value by calling the handler based on the current menu
void potentiometer_handler(int diff_percent_pot_value){
    int curr_index = menu_navigation[curr_menu] -> curr_index;
    (menu_navigation[curr_menu] -> opt_handlers[curr_index]).pt_action(diff_percent_pot_value);
}

// Function that changes the volume
void change_vol(int pot_value) {
    if (pressed_button == NOT_DEFINED) {
        for (uint8_t i = 0; i < SAMPLE_NUM; i++)
            set_volume(i, (float)pot_value * VOLUME_NORMALIZER_VALUE);
    } else
        set_volume(get_sample_bank_index(pressed_button), (float)pot_value * VOLUME_NORMALIZER_VALUE);
}

// Function that changes the pitch
void change_pitch(int pot_value){
    if (pressed_button == NOT_DEFINED) {
        for (uint8_t i = 0; i < SAMPLE_NUM; i++) {
            float sample_i_pitch_factor = get_pitch_factor(i);
            set_pitch_factor(i, sample_i_pitch_factor + (float)pot_value * PITCH_NORMALIZER_VALUE);
        }
    } else {
        float sample_i_pitch_factor = get_pitch_factor(get_sample_bank_index(pressed_button));
        set_pitch_factor(get_sample_bank_index(pressed_button), sample_i_pitch_factor + (float)pot_value * PITCH_NORMALIZER_VALUE);
    }
}

// Function that toggles the bit crusher
void toggle_bit_crusher_menu(int pot_value){
    bool bit_crusher = false;
    if (pot_value >= 0){
        bit_crusher = true;
    }
    if (pressed_button == NOT_DEFINED) {
        for (uint8_t i = 0; i < SAMPLE_NUM; i++){
            toggle_bit_crusher(i, bit_crusher);
        }
    } else {
        toggle_bit_crusher(get_sample_bank_index(pressed_button), bit_crusher);
    }
}

// Function that toggles the distortion 
void toggle_distortion_menu(int pot_value){
    bool distortion = false;
    if (pot_value >= 0){
        distortion = true;
    }
    if (pressed_button == NOT_DEFINED) {
        for (uint8_t i = 0; i < SAMPLE_NUM; i++){
            toggle_distortion(i, distortion);
        }
    } else {
        toggle_distortion(get_sample_bank_index(pressed_button), distortion);
    }
}

// Function that rotates the mode based on the enum in playback_mode.h
void rotate_mode(int pot_value){
    int next = -1;
    if (pot_value > 0){
        next = 1;
    }
    if (pressed_button == NOT_DEFINED) {
        for (uint8_t i = 0; i < SAMPLE_NUM; i++){
            set_playback_mode(i, next_mode(next, mode));
        }
    } else {
        set_playback_mode(get_sample_bank_index(pressed_button), next_mode(next, get_playback_mode(get_sample_bank_index(pressed_button))));
    }
}

// Function that gets the next mode
mode_t next_mode(int next, mode_t curr_mode){
    return (mode_t)((curr_mode + next + MODE_NUM_OPT)%MODE_NUM_OPT);
}

// Function that changes the bit depth
void change_bit_depth(int pot_value){
    int8_t bit_depth_changing = -1;
    if (pot_value > 0){
        bit_depth_changing = 1;
    }
    if (pressed_button == NOT_DEFINED){
        for (uint8_t i = 0; i < SAMPLE_NUM; i++){
            uint8_t curr_bit_depth = get_bit_crusher_bit_depth(i);
            uint8_t new_bit_depth = (curr_bit_depth + bit_depth_changing + BIT_DEPTH_MAX) % BIT_DEPTH_MAX;
            set_bit_crusher_bit_depth(i, new_bit_depth);
        }
    } else {
        uint8_t curr_bit_depth = get_bit_crusher_bit_depth(get_sample_bank_index(pressed_button));
        uint8_t new_bit_depth = (curr_bit_depth + bit_depth_changing + BIT_DEPTH_MAX) % BIT_DEPTH_MAX;
            set_bit_crusher_bit_depth(get_sample_bank_index(pressed_button), new_bit_depth);
    }
}

// Function that changes the downsample
void change_downsample(int pot_value){
    int8_t downsample_changing = -1;
    if (pot_value > 0){
        downsample_changing = 1;
    }
    if (pressed_button == NOT_DEFINED){
        for (uint8_t i = 0; i < SAMPLE_NUM; i++){
            uint8_t curr_downsample = get_bit_crusher_downsample(i);
            uint8_t new_downsample = (curr_downsample + downsample_changing + DOWNSAMPLE_MAX) % DOWNSAMPLE_MAX;
            set_bit_crusher_downsample(i, new_downsample);
        }
    } else {
        uint8_t curr_downsample = get_bit_crusher_downsample(get_sample_bank_index(pressed_button));
        uint8_t new_downsample = (curr_downsample + downsample_changing + DOWNSAMPLE_MAX) % DOWNSAMPLE_MAX;
        set_bit_crusher_downsample(get_sample_bank_index(pressed_button), new_downsample);
    }
}

// Function that changes the distortion gain
void change_distortion_gain(int pot_value){
    float gain_changing = (float)(pot_value * VOLUME_NORMALIZER_VALUE);
    if (pressed_button == NOT_DEFINED){
        for (uint8_t i = 0; i < SAMPLE_NUM; i++){
            float curr_gain = get_distortion_gain(i);
            float new_gain = fmodf(curr_gain + gain_changing + DISTORTION_GAIN_MAX, DISTORTION_GAIN_MAX);
            set_distortion_gain(i, new_gain);
        }
    } else {
        float curr_gain = get_distortion_gain(get_sample_bank_index(pressed_button));
        float new_gain = fmodf(curr_gain + gain_changing + DISTORTION_GAIN_MAX, DISTORTION_GAIN_MAX);
        set_distortion_gain(get_sample_bank_index(pressed_button), new_gain);
    }
}

// Function that changes the distortion threshold
void change_distortion_threshold(int pot_value){
    int16_t threshold_changing = (int16_t)(pot_value * THRESHOLD_NORMALIZER_VALUE);
    if (pressed_button == NOT_DEFINED){
        for (uint8_t i = 0; i < SAMPLE_NUM; i++){
            int16_t curr_threshold = get_distortion_threshold(i);
            int16_t new_threshold = (curr_threshold + threshold_changing + DISTORTION_THRESHOLD_MAX) % DISTORTION_THRESHOLD_MAX;
            set_distortion_threshold(i, new_threshold);
        }
    } else {
        int16_t curr_threshold = get_distortion_threshold(get_sample_bank_index(pressed_button));
        int16_t new_threshold = (curr_threshold + threshold_changing + DISTORTION_THRESHOLD_MAX) % DISTORTION_THRESHOLD_MAX;
        set_distortion_threshold(get_sample_bank_index(pressed_button), new_threshold);
    }
}

#pragma endregion