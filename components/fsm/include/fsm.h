#ifndef FSM_H
#define FSM_H

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
#include "lcd.h"

#define GEN_MENU_NUM_OPT 2
#define BTN_MENU_NUM_OPT 4
#define SETTINGS_NUM_OPT 2
#define EFFECTS_NUM_OPT 3
#define MODE_NUM_OPT 3
#define BITCRUSHER_NUM_OPT 3
#define PITCH_NUM_OPT 1
#define DISTORTION_NUM_OPT 2

void main_fsm(QueueSetHandle_t in_set);
void joystick_handler(JoystickDir in_dir);
void goto_settings();
void goto_effects();
void goto_selection();
void goto_bitcrusher();
void goto_pitch();
void goto_distortion();
void menu_move(int* index, int max_opt, int direction);
void js_right_handler();
void js_left_handler();
void js_up_handler();
void js_down_handler();
void sink();
void set_button_pressed(int pad_id);
void set_new_pot_value (int* old_val);
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
pb_mode_t next_mode(int next, pb_mode_t curr_mode);

//Enum that describes every type of menu we have in our project
typedef enum {
    GEN_MENU,
    BTN_MENU,
    SETTINGS,
    EFFECTS,
    BITCRUSHER,
    PITCH,
    DISTORTION
} menu_types;

//Menu that we are currently navigating
extern menu_types curr_menu;

//Records the last pressed button, which is useful to know the button to apply the changes to 
extern int8_t pressed_button;

//Records the last mode set (or the default one if never changed)
extern pb_mode_t mode;

//Actual functions to perform when an input is received through joystick or potentiometer
typedef void (*action_t) (void); 
typedef void (*pt_action_t) (int); 

//Interaction with a single voice of the menu
typedef struct {
    char print[16];         //String to print on screen for each voice of the menu
    action_t js_right_action; //Function to execute for each voice when a right shift of the joystick is detected
    pt_action_t pt_action;       //Function to execute for each voice when a potentiometer action_t is detected 
} opt_interactions_t;

//Datatype that represent a single menu (between the one enlisted above)
typedef struct {
    int curr_index;                     //Current position in the menu
    int max_size;                       //Actual size of the menu
    opt_interactions_t* opt_handlers;   //Array of handlers for each voice
} menu_t;

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

#endif