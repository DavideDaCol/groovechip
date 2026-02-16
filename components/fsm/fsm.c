#include "fsm.h"
#include "sd_reader.h"
#include "esp_log.h"
#include <math.h>

QueueHandle_t fsm_queue = NULL;

menu_types curr_menu = GEN_MENU;

uint8_t pressed_button = NOT_DEFINED;

pb_mode_t mode = ONESHOT;

static bool screen_has_to_change = false;

void get_sample_load_second_line(char*);
void get_volume_second_line(char*);
void get_gen_menu_second_line(char* out);
void get_btn_menu_or_btn_effects_second_line(char* out);
void get_gen_settings_second_line(char* out);
void get_btn_settings_second_line(char* out);
void get_metronome_second_line(char* out);
void get_bitcrusher_second_line(char* out);
void get_distortion_second_line(char* out);
void get_pitch_second_line(char* out);
void get_chopping_second_line(char* out);

const char* TAG_FSM = "FSM";

static int last_pot_value = 2000;

char *sample_names_bank[SAMPLE_NUM];

#pragma region GENERAL MENU
/***********************************
GENERAL MENU 
***********************************/
opt_interactions_t gen_handlers[] = {
    {
        .first_line = "Settings",
        .second_line = get_gen_menu_second_line,
        .js_right_action = goto_gen_settings,
        .pt_action = sink
    }, 
    {
        .first_line = "Effects",
        .second_line = get_gen_menu_second_line,
        .js_right_action = goto_gen_effects,
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
        .second_line = get_btn_menu_or_btn_effects_second_line,
        .js_right_action = goto_btn_settings,
        .pt_action = sink
    },
    {
        .first_line = "Effects",
        .second_line = get_btn_menu_or_btn_effects_second_line,
        .js_right_action = goto_btn_effects,
        .pt_action = sink
    }, 
    {
        .first_line = "Select sample",
        .second_line = get_btn_menu_or_btn_effects_second_line,
        .js_right_action = goto_sample_load,
        .pt_action = sink
    }, 
    {
        .first_line = "Save changes",
        .second_line = get_btn_menu_or_btn_effects_second_line,
        .js_right_action = save, //TODO
        .pt_action = sink
    },
    {
        .first_line = "Chopping",
        .second_line = get_btn_menu_or_btn_effects_second_line,
        .js_right_action = goto_chopping,
        .pt_action = sink
    }
};

menu_t btn_menu = {
    .curr_index = 0,
    .max_size = BTN_MENU_NUM_OPT,
    .opt_handlers = btn_handlers
};

/***********************************/
#pragma endregion

#pragma region SETTINGS MENU
/***********************************
SETTINGS MENU (structure is the same whether it's related to a specific button or is general) 
***********************************/
opt_interactions_t gen_settings_handlers[] = {
    {
        .first_line = "Volume",
        .second_line = get_volume_second_line,
        .js_right_action = sink,
        .pt_action = change_master_vol,
    },
    {
        .first_line = "Metronome",
        .second_line = get_gen_settings_second_line,
        .js_right_action = goto_metronome,
        .pt_action = sink
    }
};

menu_t gen_settings = {
    .curr_index = 0,
    .max_size = GEN_SETTINGS_NUM_OPT,
    .opt_handlers = gen_settings_handlers
};

// Button settings
opt_interactions_t btn_settings_handlers[] = {
    {
        .first_line = "Mode",
        .second_line = get_btn_settings_second_line,
        .js_right_action = sink,
        .pt_action = rotate_mode,
    },
    {
        .first_line = "Volume",
        .second_line = get_volume_second_line,
        .js_right_action = sink, 
        .pt_action = change_vol,
    }
};

menu_t btn_settings = {
    .curr_index = 0,
    .max_size = BTN_SETTINGS_NUM_OPT,
    .opt_handlers = btn_settings_handlers
};
/***********************************/
#pragma endregion

#pragma region EFFECTS MENU
/***********************************
GEN EFFECTS MENU (structure is the same whether it's related to a specific button or is general) 
***********************************/
opt_interactions_t btn_eff_handlers[] = {
    {
        .first_line = "Bitcrusher",
        .second_line = get_btn_menu_or_btn_effects_second_line,
        .js_right_action = goto_bitcrusher,
        .pt_action = sink,
    },
    {
        .first_line = "Pitch",
        .second_line = get_btn_menu_or_btn_effects_second_line,
        .js_right_action = goto_pitch,
        .pt_action = sink,
    },
    {
        .first_line = "Distortion",
        .second_line = get_btn_menu_or_btn_effects_second_line,
        .js_right_action = goto_distortion,
        .pt_action = sink,
    }
};

menu_t btn_effects = {
    .curr_index = 0,
    .max_size = BTN_EFFECTS_NUM_OPT,
    .opt_handlers = btn_eff_handlers
};

opt_interactions_t gen_eff_handlers[] = {
    {
        .first_line = "Bitcrusher",
        .second_line = get_gen_menu_second_line,
        .js_right_action = goto_bitcrusher,
        .pt_action = sink,
    },
    {
        .first_line = "Distortion",
        .second_line = get_gen_menu_second_line,
        .js_right_action = goto_distortion,
        .pt_action = sink,
    }
};

menu_t gen_effects = {
    .curr_index = 0,
    .max_size = GEN_EFFECTS_NUM_OPT,
    .opt_handlers = gen_eff_handlers
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
        .second_line = get_bitcrusher_second_line,
        .js_right_action = sink,
        .pt_action = change_bit_crusher,
    },
    {
        .first_line = "Bit depth: ",
        .second_line = get_bitcrusher_second_line,
        .js_right_action = sink, 
        .pt_action = change_bit_depth,
    },
    {
        .first_line = "Downsample: ",
        .second_line = get_bitcrusher_second_line,
        .js_right_action = sink,
        .pt_action = change_downsample,
    }
};

menu_t bit_crusher_menu = {
    .curr_index = 0,
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
        .second_line = get_pitch_second_line,
        .js_right_action = sink,
        .pt_action = change_pitch,
    }
};

menu_t pitch_menu = {
    .curr_index = 0,
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
        .second_line = get_distortion_second_line,
        .js_right_action = sink,
        .pt_action = change_distortion,
    },
    {
        .first_line = "Gain: ",
        .second_line = get_distortion_second_line,
        .js_right_action = sink, 
        .pt_action = change_distortion_gain,
    },
    {
        .first_line = "Threshold: ",
        .second_line = get_distortion_second_line,
        .js_right_action = sink,
        .pt_action = change_distortion_threshold,
    }
};

menu_t distortion_menu = {
    .curr_index = 0,
    .max_size = DISTORTION_NUM_OPT,
    .opt_handlers = distortion_handlers
};
/**********************************************
CHOPPING MENU
***********************************************/
opt_interactions_t chopping_handlers[] = {
    {
        .first_line = "Start: ",
        .second_line = get_chopping_second_line,
        .js_right_action = sink,
        .pt_action = change_chopping_start,
    },
    {
        .first_line = "End: ",
        .second_line = get_chopping_second_line,
        .js_right_action = sink,
        .pt_action = change_chopping_end, //TODO
    }
};
void change_chopping_start();
void change_chopping_end();
menu_t chopping_menu = {
    .curr_index = 0,
    .max_size = CHOPPING_NUM_OPT,
    .opt_handlers = chopping_handlers
};

/************************************* */

/**********************************************
METRONOME MENU
***********************************************/
opt_interactions_t metronome_handlers[] = {
    {
        .first_line = "Metronome: ",
        .second_line = get_metronome_second_line,
        .js_right_action = sink,
        .pt_action = change_metronome,
    },
    {
        .first_line = "Bpm: ",
        .second_line = get_metronome_second_line,
        .js_right_action = sink,
        .pt_action = change_metronome_bpm,
    },
};

menu_t metronome_menu = {
    .curr_index = 0,
    .max_size = METRONOME_NUM_OPT,
    .opt_handlers = metronome_handlers
};

/************************************* */

#pragma endregion

//Menu collection, essential for the navigation
menu_t* menu_navigation[] = {
    &gen_menu,
    &btn_menu,
    &gen_settings,
    &btn_settings,
    &gen_effects,
    &btn_effects,
    &metronome_menu,
    &bit_crusher_menu,
    &pitch_menu,
    &distortion_menu,
    NULL,
    &chopping_menu,
};


// menu state stack
static menu_pair_t menu_stack[10];
static int stack_index = 0;

void menu_push(menu_types menu, int index){
    menu_pair_t state = {
        .menu = menu,
        .index = index
    };

    menu_stack[stack_index++] = state;
}
menu_pair_t menu_pop(){
    if (stack_index <= 0){
        ESP_LOGI(TAG_FSM, "Info: empty stack");
        menu_pair_t pair = {
            .menu = GEN_MENU,
            .index = 0
        };
        return pair;
    } 
    else return menu_stack[--stack_index];
}

void clear_stack(){
    stack_index = 0;
}

void get_gen_menu_second_line(char* out){
    sprintf(out, "General");
}
void get_btn_menu_or_btn_effects_second_line(char* out){
    uint8_t bank_index = get_sample_bank_index(pressed_button);
    uint8_t pad_num = get_pad_num(pressed_button);

    sprintf(out, "Pad %u", pad_num); //button is presset
}
void get_gen_settings_second_line(char* out){
    sprintf(out, "General");
}
void get_btn_settings_second_line(char* out){
    sprintf(out, " "); //reset string
    uint8_t bank_index = get_sample_bank_index(pressed_button);
    if(bank_index == NOT_DEFINED) return;

    pb_mode_t curr_mode = get_playback_mode(bank_index);
    get_mode_stringify(curr_mode, out);
}
void get_metronome_second_line(char* out){
    switch (menu_navigation[curr_menu] -> curr_index){
        case ENABLE_MTRN:
            if(get_metronome_state()){
                sprintf(out, "On");
            }
            else {
                sprintf(out, "Off");
            }
            break;
        case BPM:
            int mtrn_bpm = get_metronome_bpm();
            sprintf(out, "%d", mtrn_bpm);
            break;
        default:
            break;
        }
}
void get_bitcrusher_second_line(char* out){
    sprintf(out, " "); //reset string
    uint8_t bank_index = get_sample_bank_index(pressed_button);
    uint8_t value;
    switch (menu_navigation[curr_menu]->curr_index){
    // retrieve current value set res
    case ENABLED_BC:
        if((pressed_button != NOT_DEFINED && get_bit_crusher_state(bank_index))
        || (pressed_button == NOT_DEFINED && get_master_bit_crusher_enable())){
            sprintf(out, "On");
        }
        else {
            sprintf(out, "Off");
        }
        break;
    case BIT_DEPTH:
        if (pressed_button != NOT_DEFINED){
            value = get_bit_crusher_bit_depth(bank_index);
        } else {
            value = get_bit_crusher_bit_depth_master_buffer();
        }
        printf("BD: %u", value);
        sprintf(out, "%u", value);
        break;
    case DOWNSAMPLE:
        if (pressed_button != NOT_DEFINED){
            value = get_bit_crusher_downsample(bank_index);
        } else {
            value = get_bit_crusher_downsample_master_buffer();
        }
        printf("DS: %u", value);
        sprintf(out, "%u", value);
        break;
    default:
        break;
    }
}
void get_distortion_second_line(char* out){
    sprintf(out, " "); //reset string
    uint8_t bank_index = get_sample_bank_index(pressed_button);

    if(bank_index == NOT_DEFINED) return;

    switch (menu_navigation[curr_menu]->curr_index){
    case ENABLED_D:
        if((pressed_button != NOT_DEFINED && get_distortion_state(bank_index))
        || (pressed_button == NOT_DEFINED && get_master_distortion_enable())){
            sprintf(out, "On");
        }
        else {
            sprintf(out, "Off");
        }
        break;

    case GAIN:
        float gain = get_distortion_gain_master_buffer();
        if (pressed_button != NOT_DEFINED){
            gain = get_distortion_gain(bank_index);
        }
        sprintf(out, "%.2f", gain);
        break;
    case THRESHOLD:
        int16_t threshold = get_distortion_threshold_master_buffer();
        if (pressed_button != NOT_DEFINED){
            get_distortion_threshold(bank_index);
        }
        sprintf(out, "%d", threshold);
        break;
    default:
        break;
    }
}
void get_pitch_second_line(char* out){
    uint8_t bank_index = get_sample_bank_index(pressed_button);

    float factor = get_pitch_factor(bank_index);
    sprintf(out, "%.2f", factor);
}

void get_chopping_second_line(char* out){
    sprintf(out, " "); //reset string
    uint8_t bank_index = get_sample_bank_index(pressed_button);

    if(bank_index == NOT_DEFINED) return;
    switch (menu_navigation[curr_menu]->curr_index){
    case START:
        uint32_t start_ptr = (float)get_sample_start_ptr(bank_index) / GRVCHP_SAMPLE_FREQ * 1000;
        sprintf(out, "%ld", start_ptr);
        break;
    case END:
        uint32_t end_ptr = (float)get_sample_end_ptr(bank_index) / GRVCHP_SAMPLE_FREQ * 1000;
        sprintf(out, "%ld", end_ptr);
        break;
    default:
        break;
    }
}

void get_sample_load_second_line(char* out)
{
    uint8_t index = menu_navigation[SAMPLE_LOAD]->curr_index;

    if(index < sample_names_size)
    {
        snprintf(out, 16, "%s", sample_names[index]);
    }
}

void get_volume_second_line(char* out) {
    float volume = get_master_volume();
    if (pressed_button != NOT_DEFINED){
        volume = get_volume(get_sample_bank_index(pressed_button));
    }

    //float volume = get_volume(get_sample_bank_index(pressed_button));
    //5 units of volume = 4 strays
    volume *= 80;

    int i = 0;
    while (volume >= 5) {
        out[i] = BLACK_BOX;
        volume -=5;
        i++;
    }
    out[i] = round(volume) - 1;
    out[i + 1] = '\0';
}


menu_t sample_load_menu = {};
opt_interactions_t* sample_load_actions = NULL;

#pragma region MAIN FSM
//FSM implementation function
void main_fsm_task(void *pvParameters) {

    for (int i = 0; i < sample_names_size; i++){
        ESP_LOGI(TAG_FSM, "sample name %i is %s", i, sample_names[i]);
    }
    
    sample_load_actions = (opt_interactions_t *)malloc(sample_names_size * sizeof(opt_interactions_t));
    //for every sample, create the menu options
    for(int i = 0; i < sample_names_size; i++){
        // char title[17] = "";
        // snprintf(title, sizeof(sample_names[i]), "%s", sample_names[i]);

        ESP_LOGI(TAG_FSM, "created menu entry for %s", sample_names[i]);
        sample_load_actions[i].js_right_action = sample_load;
        sprintf(sample_load_actions[i].first_line,"Sample list:");
        sample_load_actions[i].second_line = get_sample_load_second_line;
        sample_load_actions[i].pt_action = sink;
    }
    // create the menu for the samples in the SD card
    sample_load_menu.curr_index = -1;
    sample_load_menu.max_size = sample_names_size;
    sample_load_menu.opt_handlers = sample_load_actions;

    // TODO make this nicer
    menu_navigation[SAMPLE_LOAD] = &sample_load_menu;

    while(1) {
        fsm_queue_msg_t msg;
        if (xQueueReceive(fsm_queue, &msg, portMAX_DELAY) == pdFALSE){
            ESP_LOGE(TAG_FSM, "Error: unable to read the fsm_queue");
            continue;
        }
        int payload = msg.payload;
        switch (msg.source)
        {
        case JOYSTICK:
            joystick_handler(payload);  
            break;
        case POTENTIOMETER:
            potentiometer_handler(payload);
            last_pot_value = payload;
            break;
        case PAD:
            set_button_pressed(payload);
            break;
        default:
            break;
        }
        if(screen_has_to_change){
            char* line1 = (menu_navigation[curr_menu]->opt_handlers[menu_navigation[curr_menu]->curr_index]).first_line;
            char line2[17] = "";
            
            (menu_navigation[curr_menu]->opt_handlers[menu_navigation[curr_menu]->curr_index]).second_line(line2);
            print_double(line1, line2);
            screen_has_to_change = false;
        }
    }
}
#pragma endregion

#pragma region MENU NAVIGATION
//Joystick handler implementation 
void joystick_handler(joystick_dir_t in_dir) {
    printf("CURRENT STATE -> %d, %d\n", curr_menu, menu_navigation[curr_menu] -> curr_index);
    switch (in_dir){
        case LEFT: js_left_handler(); break;
        case DOWN: js_down_handler(); break;
        case RIGHT: js_right_handler(); break;
        case UP: js_up_handler(); break;
        case PRESS: recorder_fsm(); break;
        default: sink(); return;
    }
    printf("%s\n", (menu_navigation[curr_menu]->opt_handlers[menu_navigation[curr_menu]->curr_index]).first_line);

    screen_has_to_change = true;
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
void goto_gen_settings() {
    gen_settings.curr_index = 0;
    curr_menu = GEN_SETTINGS;
}

// Atomic function that sets the current menu and the current index
void goto_btn_settings(){
    btn_settings.curr_index = 0;
    curr_menu = BTN_SETTINGS;
}

// Atomic function that sets the current menu and the current index
void goto_btn_effects() {
    btn_effects.curr_index = 0;
    curr_menu = BTN_EFFECTS;
}

// Atomic function that sets the current menu and the current index
void goto_gen_effects(){
    btn_effects.curr_index = 0;
    curr_menu = GEN_EFFECTS;
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

void goto_chopping(){
    chopping_menu.curr_index = 0;
    curr_menu = CHOPPING;
}

void goto_metronome(){
    metronome_menu.curr_index = 0;
    curr_menu = METRONOME;
}

void sample_load() {
    int sample_idx = get_pad_num(pressed_button) - 1;
    
    int index = menu_navigation[curr_menu] -> curr_index;
    ESP_LOGW(TAG_FSM, "pressed_button is %i, sample path is %s", pressed_button, sample_names[index]);
    esp_err_t res = ld_sample(sample_idx, sample_names[index], &sample_bank[sample_idx]);
    if(res == ESP_OK){
        map_pad_to_sample(pressed_button, sample_idx);
        sample_names_bank[sample_idx] = sample_names[index];

        // HACK: push btn_menu state, so if js_left is triggered, it go to btn menu
        menu_pop();
        menu_push(BTN_MENU, 0);
    }
}

// Function that calls the correct handler based on the current menu
void js_right_handler() {
    int index = menu_navigation[curr_menu] -> curr_index;
    if(menu_navigation[curr_menu]->opt_handlers[index].js_right_action != sink 
        && menu_navigation[curr_menu]->opt_handlers[index].js_right_action != sample_load
    ){
        // change the state only if the current menu can actually go in a submenu. This prevents to push the same state multiple times        
        menu_push(curr_menu, index);
    }
    menu_navigation[curr_menu] -> opt_handlers[index].js_right_action();

}

// Function that sets the current menu the the previoous one
void js_left_handler() {
    menu_pair_t state = menu_pop();
    curr_menu = state.menu;
    menu_navigation[curr_menu]->curr_index = state.index;

    if(curr_menu == GEN_MENU)
        pressed_button = NOT_DEFINED;
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
        clear_stack();
        pressed_button = pad_id;
        btn_menu.curr_index = 0;

        if(get_sample_bank_index(pad_id) != NOT_DEFINED)
            curr_menu = BTN_MENU; // normal menu
        else {
            // curr_menu = BTN_MENU_NO_SAMPLE; //in this case the user can only laod a sample
            sample_load_menu.curr_index = 0;
            curr_menu = SAMPLE_LOAD;
        }

        screen_has_to_change = true;
    }
}

// Sink function
void sink() {
    
    return;
}

// sink function, but it also free a stack position. This is needed when we don't want to save the previous state (if we go in the same menu multiple times)
void sink_free_prev(){
    menu_pop();
}

void save() {
    st_sample(get_sample_bank_index(pressed_button), sample_names_bank[get_sample_bank_index(pressed_button)]);
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
    float raw_vol = (float)pot_value * VOLUME_NORMALIZER_VALUE;
    float stepped_vol = round(raw_vol / VOLUME_SCALE_VALUE) * VOLUME_SCALE_VALUE;
    
    
    uint8_t idx = get_sample_bank_index(pressed_button);
    screen_has_to_change = (stepped_vol != get_volume(idx)); 

    set_volume(idx, stepped_vol);
}

// Function that changes the master volume
void change_master_vol(int pot_value){
    float raw_vol = (float)pot_value * VOLUME_NORMALIZER_VALUE;
    float stepped_vol = round(raw_vol / VOLUME_SCALE_VALUE) * VOLUME_SCALE_VALUE;

    screen_has_to_change = (stepped_vol != get_master_volume()); 
    set_master_buffer_volume(stepped_vol);
}

// Function that changes the pitch
void change_pitch(int pot_value){
    if (pressed_button == NOT_DEFINED) return;

    float new_pitch_factor = round(pot_value * PITCH_NORMALIZER_VALUE / PITCH_SCALE_VALUE) * PITCH_SCALE_VALUE;

    uint8_t idx = get_sample_bank_index(pressed_button);
    screen_has_to_change = (new_pitch_factor != get_pitch_factor(idx));

    set_pitch_factor(idx, new_pitch_factor);
}

void change_bit_crusher(int pot_value){
    if (pressed_button == NOT_DEFINED){
        change_master_bit_crusher(pot_value);
    } else {
        change_sample_bit_crusher(pot_value);
    }
}

// Function that changes the bit crusher enable
void change_sample_bit_crusher(int pot_value){

    uint8_t idx = get_sample_bank_index(pressed_button);
    bool new_state = pot_value > 50;
    screen_has_to_change = get_bit_crusher_state(idx) != new_state;

    set_bit_crusher(idx, new_state);
}

// Function that changes the master bit crusher enable
void change_master_bit_crusher(int pot_value){

    bool new_state = pot_value > 50;
    screen_has_to_change = get_master_bit_crusher_enable() != new_state;

    set_master_bit_crusher_enable(new_state);
}

void change_distortion(int pot_value){
    if (pressed_button == NOT_DEFINED){
        change_master_distortion(pot_value);
    } else {
        change_sample_distortion(pot_value);
    }
}

// Function that changes the distortion enable
void change_sample_distortion(int pot_value){

    uint8_t idx = get_sample_bank_index(pressed_button);
    bool new_state = pot_value > 50;
    screen_has_to_change = get_distortion_state(idx) != new_state;

    set_distortion(idx, new_state);
}

// Function that changes the master distortion enable
void change_master_distortion(int pot_value){

    bool new_state = pot_value > 50;
    screen_has_to_change = get_master_distortion_enable() != new_state;

    set_master_distortion_enable(new_state);
}

// Function that rotates the mode based on the enum in playback_mode.h
void rotate_mode(int pot_value){

    uint8_t idx = get_sample_bank_index(pressed_button);
    pb_mode_t new_mode = next_mode(pot_value);
    screen_has_to_change = get_playback_mode(idx) != new_mode;

    set_playback_mode(idx, new_mode);
}

// Function that gets the next mode
pb_mode_t next_mode(int pot_value){
    // return (mode_t)(((int)curr_mode + next + MODE_NUM_OPT)%MODE_NUM_OPT);
    if(pot_value <= 25)
        return HOLD;
    if(pot_value <= 50)
        return ONESHOT;
    if(pot_value <= 75)
        return LOOP;
    if(pot_value <= 100)
        return ONESHOT_LOOP;
    return HOLD;
}

void change_bit_depth(int pot_value){
    if (pressed_button == NOT_DEFINED){
        change_master_bit_depth(pot_value);
    } else {
        change_sample_bit_depth(pot_value);
    }
}

// Function that changes the bit depth
void change_sample_bit_depth(int pot_value){
    int idx = get_sample_bank_index(pressed_button);
    uint8_t new_bit_depth = 1 + round(((float)pot_value * 15.0) / 100.0);

    screen_has_to_change = get_bit_crusher_bit_depth(idx) != new_bit_depth;

    set_bit_crusher_bit_depth(idx, new_bit_depth);
}

// Function that changes the master bit depth
void change_master_bit_depth(int pot_value){
    uint8_t new_bit_depth = 1 + round(((float)pot_value * 15.0) / 100.0);

    screen_has_to_change = get_bit_crusher_bit_depth_master_buffer() != new_bit_depth;

    set_bit_crusher_bit_depth_master_buffer(new_bit_depth);
}

void change_downsample(int pot_value){
    if (pressed_button == NOT_DEFINED){
        change_master_downsample(pot_value);
    } else {
        change_sample_downsample(pot_value);
    }
}

// Function that changes the downsample
void change_sample_downsample(int pot_value){

    uint8_t idx = get_sample_bank_index(pressed_button);
    uint8_t new_downsample = 1 + round(((float)pot_value * 9.0) / 100.0);

    screen_has_to_change = get_bit_crusher_downsample(idx) != new_downsample;

    set_bit_crusher_downsample(idx, new_downsample);
}

// Function that changes the master downsample
void change_master_downsample(int pot_value){
    uint8_t new_downsample = 1 + round(((float)pot_value * 9.0) / 100.0);
    screen_has_to_change = get_bit_crusher_downsample_master_buffer() != new_downsample;

    set_bit_crusher_downsample_master_buffer(new_downsample);
}

void change_distortion_gain(int pot_value){
    if (pressed_button == NOT_DEFINED){
        change_master_distortion_gain(pot_value);
    } else {
        change_sample_distortion_gain(pot_value);
    }
}

// Function that changes the distortion gain
void change_sample_distortion_gain(int pot_value){

    uint8_t idx = get_sample_bank_index(pressed_button);
    float new_gain = round(pot_value * VOLUME_NORMALIZER_VALUE / VOLUME_SCALE_VALUE) * VOLUME_SCALE_VALUE * 10;

    screen_has_to_change = get_distortion_gain(idx) != new_gain;

    set_distortion_gain(idx, new_gain);
}

// Function that changes the master distortion gain
void change_master_distortion_gain(int pot_value){
    float new_gain = round(pot_value * VOLUME_NORMALIZER_VALUE / VOLUME_SCALE_VALUE) * VOLUME_SCALE_VALUE * 10;

    screen_has_to_change = get_distortion_gain_master_buffer() != new_gain;

    set_distortion_gain_master_buffer(new_gain);
}

void change_distortion_threshold(int pot_value){
    if (pressed_button == NOT_DEFINED){
        change_master_distortion_threshold(pot_value);
    } else {
        change_sample_distortion_threshold(pot_value);
    }
}

// Function that changes the distortion threshold
void change_sample_distortion_threshold(int pot_value){

    uint8_t idx = get_sample_bank_index(pressed_button);
    int16_t new_threshold = (int16_t)((pot_value * THRESHOLD_NORMALIZER_VALUE)/THRESHOLD_SCALE_VALUE)*THRESHOLD_SCALE_VALUE;

    screen_has_to_change = get_distortion_threshold(idx) != new_threshold;

    set_distortion_threshold(idx, new_threshold);
}

// Function that changes the master distortion threshold
void change_master_distortion_threshold(int pot_value){
    int16_t new_threshold = (int16_t)((pot_value * THRESHOLD_NORMALIZER_VALUE)/THRESHOLD_SCALE_VALUE)*THRESHOLD_SCALE_VALUE;

    screen_has_to_change = get_distortion_threshold_master_buffer() != new_threshold;

    set_distortion_threshold_master_buffer(new_threshold);
}

// Function that changes the metronome enable value
void change_metronome(int pot_value){

    bool new_state = pot_value > 50;
    screen_has_to_change = get_metronome_state() != new_state;

    set_metronome_state(new_state);
}

// Function that changes the metronome bpm value
void change_metronome_bpm(int pot_value){
    float new_bpm = (round((float)pot_value * METRONOME_NORMALIZER / METRONOME_SCALE_VALUE) * METRONOME_SCALE_VALUE) + BASE_METRONOME_VALUE;
    screen_has_to_change = get_metronome_bpm() != new_bpm;

    set_metronome_bpm(new_bpm);
}

void change_chopping_start(int pot_value){
    const uint8_t idx = get_sample_bank_index(pressed_button);
    uint32_t new_start = pot_value * (((float)get_sample_total_frames(idx)) / 100.0);
    printf("New start: %ld\nPot value: %d\n", new_start, pot_value);

    if(get_sample_start_ptr(idx) != new_start){
        screen_has_to_change = set_sample_start_ptr(idx, (float)new_start);
    }
}
void change_chopping_end(int pot_value){
    const uint8_t idx = get_sample_bank_index(pressed_button);
    uint32_t new_end = pot_value * (((float)get_sample_total_frames(idx)) / 100.0);
    printf("New end: %ld\nPot value: %d\n", new_end, pot_value);

    if(get_sample_end_ptr(idx) != new_end){
        screen_has_to_change = set_sample_end_ptr(idx, (float)new_end);
    }

}

#pragma endregion

void fsm_init(){
    // init queue
    fsm_queue = xQueueCreate(30, sizeof(fsm_queue_msg_t));
    
    // draw initial display
    char* line1 = (menu_navigation[curr_menu]->opt_handlers[menu_navigation[curr_menu]->curr_index]).first_line;
    char line2[17] = "";
    
    (menu_navigation[curr_menu]->opt_handlers[menu_navigation[curr_menu]->curr_index]).second_line(line2);
    print_double(line1, line2);

    // create task
    xTaskCreate(main_fsm_task, "fsm_task", 4096, NULL, 5, NULL);
}

#pragma region SINGLE QUEUE FUNCTIONS

void send_message_to_fsm_queue(message_source_t source, int payload){
    fsm_queue_msg_t msg = {
        .source = source,
        .payload = payload
    };
    if (xQueueSend(fsm_queue, &msg, 0) == pdFALSE){
        ESP_LOGE(TAG_FSM, "Error: unable to send the message to the fsm_queue");
    }
}

void IRAM_ATTR send_message_to_fsm_queue_from_ISR(message_source_t source, int payload){
    fsm_queue_msg_t msg = {
        .source = source,
        .payload = payload
    };
    if (xQueueSendFromISR(fsm_queue, &msg, NULL) == pdFALSE){
        ESP_LOGE(TAG_FSM, "Error: unable to send the message to the fsm_queue");
    }
}

void set_last_pot_value(int pot_value){
    last_pot_value = pot_value;
}

#pragma endregion