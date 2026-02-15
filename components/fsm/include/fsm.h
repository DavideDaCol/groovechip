#ifndef FSM_H
#define FSM_H

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_mac.h"
#include "driver/gpio.h"
#include "effects.h"
#include "mixer.h"
#include "playback_mode.h"
#include "pad_section.h"
#include "joystick.h"
#include "adc1.h"
#include "lcd.h"

#define MENU_NUM 8
#define GEN_MENU_NUM_OPT 2
#define BTN_MENU_NUM_OPT 5
#define GEN_SETTINGS_NUM_OPT 2
#define BTN_SETTINGS_NUM_OPT 2
#define EFFECTS_NUM_OPT 3
#define MODE_NUM_OPT 4
#define BITCRUSHER_NUM_OPT 3
#define PITCH_NUM_OPT 1
#define DISTORTION_NUM_OPT 3
#define CHOPPING_NUM_OPT 2
#define PITCH_SCALE_VALUE 0.25f
#define THRESHOLD_SCALE_VALUE 1000
#define VOLUME_SCALE_VALUE 0.05f

typedef enum {
    JOYSTICK,
    POTENTIOMETER,
    PAD,
} message_source_t;

typedef struct {
    message_source_t source;
    int payload;
} fsm_queue_msg_t;

void main_fsm_task(void *pvParameters);
void joystick_handler(joystick_dir_t in_dir);
void goto_gen_settings();
void goto_effects();
void goto_btn_settings();
void goto_bitcrusher();
void goto_pitch();
void goto_distortion();
void goto_sample_load();
void goto_chopping();
void goto_metronome();
void menu_move(int* index, int max_opt, int direction);
void js_right_handler();
void js_left_handler();
void js_up_handler();
void js_down_handler();
void sink();
void set_button_pressed(int pad_id);
void save();
void potentiometer_handler(int diff_percent_pot_value);
void change_vol(int pot_value);
void toggle_distortion_menu(int pot_value);
void toggle_bit_crusher_menu(int pot_value);
void change_pitch(int pot_value);
void rotate_mode(int pot_value);
void change_bit_depth(int pot_value);
void change_downsample(int pot_value);
void change_distortion_gain(int pot_value);
void change_distortion_threshold(int pot_value);
pb_mode_t next_mode(int pot_value);
void sample_load();
void send_message_to_fsm_queue(message_source_t source, int payload);
void send_message_to_fsm_queue_from_ISR(message_source_t source, int payload);
void fsm_init();
void set_last_pot_value(int pot_value);


//Enum that describes every type of menu we have in our project
typedef enum {
    GEN_MENU,
    BTN_MENU,
    GEN_SETTINGS,
    BTN_SETTINGS,
    EFFECTS,
    BITCRUSHER,
    PITCH,
    DISTORTION,
    SAMPLE_LOAD,
    CHOPPING
} menu_types;


typedef enum{
    ENABLED_BC,
    BIT_DEPTH,
    DOWNSAMPLE
} bitcrusher_menu_t;

typedef enum{
    ENABLED_D,
    GAIN,
    THRESHOLD
} distortion_menu_t;

typedef enum{
    MODE,
    VOLUME
} settings_menu_t;

typedef enum{
    START,
    END
} chopping_menu_t;

//Menu that we are currently navigating
extern menu_types curr_menu;

//Records the last pressed button, which is useful to know the button to apply the changes to 
extern uint8_t pressed_button;

//Records the last mode set (or the default one if never changed)
extern pb_mode_t mode;

//Actual functions to perform when an input is received through joystick or potentiometer
typedef void (*action_t) (void); 
typedef void (*pt_action_t) (int); 
typedef void (*second_line_t) (char*);

//Interaction with a single voice of the menu
typedef struct {
    char first_line[16];         //String to first_line on screen for each voice of the menu
    second_line_t second_line;
    action_t js_right_action; //Function to execute for each voice when a right shift of the joystick is detected
    pt_action_t pt_action;       //Function to execute for each voice when a potentiometer action_t is detected 
} opt_interactions_t;

//Datatype that represent a single menu (between the one enlisted above)
typedef struct {
    int curr_index;                     //Current position in the menu
    int max_size;                       //Actual size of the menu
    opt_interactions_t* opt_handlers;   //Array of handlers for each voice
} menu_t;

typedef struct{
    menu_types menu;
    uint8_t index;
} menu_pair_t;

// stack for navigate the menu
void menu_push(menu_types menu, int index);
menu_pair_t menu_pop();
void clear_stack();

extern opt_interactions_t gen_handlers[];
extern menu_t gen_menu;

extern opt_interactions_t btn_handlers[];
extern menu_t btn_menu;

extern opt_interactions_t set_handlers[];
extern menu_t settings;

extern opt_interactions_t eff_handlers[];
extern menu_t effects;

extern opt_interactions_t bit_crusher_handlers[];
extern menu_t bit_crusher_menu;

extern opt_interactions_t pitch_handlers[];
extern menu_t pitch_menu;

extern opt_interactions_t distortion_handlers[];
extern menu_t distortion_menu;

extern menu_t* menu_navigation[];
// extern const menu_pair_t PREV_MENU_GENERAL[];
// extern const menu_pair_t PREV_MENU_PAD[];

extern char* sample_names_bank[SAMPLE_NUM];

#endif