#ifndef FSM_H
#define FSM_H

#pragma region INCLUDE SECTION

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_mac.h"
#include "driver/gpio.h"
#include "mixer.h"
#include "playback_mode.h"
#include "joystick.h"

#pragma endregion

#pragma region DEFINE REGION

// number of menu
#define MENU_NUM 8

// number of options in general menu
#define GEN_MENU_NUM_OPT 2

// number of options in button menu
#define BTN_MENU_NUM_OPT 5

// number of options in general settings
#define GEN_SETTINGS_NUM_OPT 2

// number of options in button settings
#define BTN_SETTINGS_NUM_OPT 2

// number of options in general effects
#define GEN_EFFECTS_NUM_OPT 2

// number of options in button effects
#define BTN_EFFECTS_NUM_OPT 3

// number of mode options
#define MODE_NUM_OPT 4

// number of bitcrusher options
#define BITCRUSHER_NUM_OPT 3

// number of pitch options
#define PITCH_NUM_OPT 1

// number of distortion options
#define DISTORTION_NUM_OPT 3

// number of chopping options
#define CHOPPING_NUM_OPT 2

// number of metronome options
#define METRONOME_NUM_OPT 2

#pragma endregion

#pragma region STRUCT/EXTERN REGION

// enum that contains the possible sources of the message
typedef enum {
    JOYSTICK,
    POTENTIOMETER,
    PAD,
} message_source_t;

// fsm_queue message
typedef struct {
    message_source_t source;
    int payload;
} fsm_queue_msg_t;

extern QueueHandle_t fsm_queue;

// enum that describes every type of menu we have in our project
typedef enum {
    GEN_MENU,
    BTN_MENU,
    GEN_SETTINGS,
    BTN_SETTINGS,
    GEN_EFFECTS,
    BTN_EFFECTS,
    METRONOME,
    BITCRUSHER,
    PITCH,
    DISTORTION,
    SAMPLE_LOAD,
    CHOPPING
} menu_types;

// enum that describes the bitcrusher menu options
typedef enum{
    ENABLED_BC,
    BIT_DEPTH,
    DOWNSAMPLE
} bitcrusher_menu_t;

// enum that describes the distortion menu options
typedef enum{
    ENABLED_D,
    GAIN,
    THRESHOLD
} distortion_menu_t;

// enum that describes the button settings menu options
typedef enum{
    MODE,
    SAMPLE_VOLUME
} btn_settings_menu_t;

// enum that describes the general settings menu options
typedef enum{
    GEN_VOLUME,
    METRONOME_MENU
} gen_settings_menu_t;

// enum that describes the metronome menu options
typedef enum {
    ENABLE_MTRN,
    BPM,
} metronome_menu_t;

// enum that describes the chopping menu options
typedef enum{
    START,
    END
} chopping_menu_t;

// menu that we are currently navigating
extern menu_types curr_menu;

// records the last pressed button, which is useful to know the button to apply the changes to 
extern uint8_t pressed_button;

// records the last mode set (or the default one if never changed)
extern pb_mode_t mode;

// actual functions to perform when an input is received through joystick or potentiometer
typedef void (*action_t) (void); 
typedef void (*pt_action_t) (int); 
typedef void (*second_line_t) (char*);

// interaction with a single voice of the menu
typedef struct {
    char first_line[16];        // string to first_line on screen for each voice of the menu
    second_line_t second_line;  // function that dynamically retreives the second line of the screen base on the menu
    action_t js_right_action;   // function to execute for each voice when a right shift of the joystick is detected
    pt_action_t pt_action;      // function to execute for each voice when a potentiometer action_t is detected 
} opt_interactions_t;

// datatype that represent a single menu (between the one enlisted above)
typedef struct {
    int curr_index;                     // current position in the menu
    int max_size;                       // actual size of the menu
    opt_interactions_t* opt_handlers;   // array of handlers for each voice
} menu_t;

// struct used to manage the menu stack
typedef struct{
    menu_types menu;
    uint8_t index;
} menu_pair_t;

// stack for navigate the menu
void menu_push(menu_types menu, int index);
menu_pair_t menu_pop();
void clear_stack();

// array containing the actions of the general menu
extern opt_interactions_t gen_handlers[];
extern menu_t gen_menu;

// array containing the actions of the button menu
extern opt_interactions_t btn_handlers[];
extern menu_t btn_menu;

// array containing the actions of the settings menu
extern opt_interactions_t set_handlers[];
extern menu_t settings;

// array containing the actions of the button effects menu
extern opt_interactions_t btn_eff_handlers[];
extern menu_t btn_effects;

// array containing the actions of the bitcrusher menu
extern opt_interactions_t bit_crusher_handlers[];
extern menu_t bit_crusher_menu;

// array containing the actions of the pitch menu
extern opt_interactions_t pitch_handlers[];
extern menu_t pitch_menu;

// array containing the actions of the distortion menu
extern opt_interactions_t distortion_handlers[];
extern menu_t distortion_menu;

// contains all the possible menu
extern menu_t* menu_navigation[];


#pragma endregion

#pragma region FUNCTION REGION

/*
@brief main final state machine task that manages the flow of all the program
by receiving messages from other peripherals and handling them.
@param pvParameters parameter of all the FreeRTOS tasks.
*/
void main_fsm_task(void *pvParameters);

/*
@brief function that handles the joystinck direction 
by calling a goto function / changing the menu and the index.
@param in_dir joystick direction.
*/
void joystick_handler(joystick_dir_t in_dir);

/*
@brief helper function that switches to button settings menu.
*/
void goto_btn_settings();

/*
@brief helper function that switches to general settings menu.
*/
void goto_gen_settings();

/*
@brief helper function that switches to button effects menu.
*/
void goto_btn_effects();

/*
@brief helper function that switches to general effects menu.
*/
void goto_gen_effects();

/*
@brief helper function that switches to bitcrusher menu.
*/
void goto_bitcrusher();

/*
@brief helper function that switches to pitch menu.
*/
void goto_pitch();

/*
@brief helper function that switches to distortion menu.
*/
void goto_distortion();

/*
@brief helper function that switches to sample load menu.
*/
void goto_sample_load();

/*
@brief helper function that switches to chopping menu.
*/
void goto_chopping();

/*
@brief helper function that switches to metronome menu.
*/
void goto_metronome();

/*
@brief function that handles the up and down index in menus.
@param index pointer to the current menu index.
@param max_opt max number of options of the current menu.
@param direction up or down direction.
*/
void menu_move(int* index, int max_opt, int direction);

/*
@brief function that calls the correct goto function 
*/
void js_right_handler();

/*
@brief function that calls the stack pop to retreive the previous menu.
*/
void js_left_handler();

/*
@brief function that calls the menu index move function to go up.
*/
void js_up_handler();

/*
@brief function that calls the menu index move function to go down.
*/
void js_down_handler();

/*
@brief sink function that do nothing.
*/
void sink();

/*
@brief function that sets the curren pressed button.
@param gpio gpio of the pressed button.
*/
void set_button_pressed(int gpio);

/*
@brief save the sample in SD.
*/
void save();

/*
@biref function that handles the potentiometer message 
by calling the current menu relative action.
@param pot_value value of the potentiometer (percentage)
*/
void potentiometer_handler(int pot_value);

/*
@brief function that changes the volume of the selected sample
by calling the corresponding function in mixer.
@param pot_value value of the potentiometer corresponding to
the percentage of the volume.
*/
void change_vol(int pot_value);

/*
@brief function that changes the volume of the master buffer
by calling the corresponding function in mixer.
@param pot_value value of the potentiometer corresponding to
the percentage of the volume.
*/
void change_master_vol(int pot_value);

/*
@brief function that changes the sample/master distortion state based
on the pressed button by calling the correct helper function.
@param pot_value value of the potentiometer.
*/
void change_distortion(int pot_value);

/*
@brief helper function that changes the distortion state
of the sample by calling the corresponding function in effects.
@param pot_value value of the potentiometer.
*/
void change_sample_distortion(int pot_value);

/*
@brief helper function that changes the distortion state
of the master buffer by calling the corresponding function in effects.
@param pot_value value of the potentiometer.
*/
void change_master_distortion(int pot_value);

/*
@brief function that changes the sample/master bit crusher state based
on the pressed button by calling the correct helper function.
@param pot_value value of the potentiometer.
*/
void change_bit_crusher(int pot_value);

/*
@brief helper function that changes the bit crusher state
of the sample by calling the corresponding function in effects.
@param pot_value value of the potentiometer.
*/
void change_sample_bit_crusher(int pot_value);

/*
@brief helper function that changes the bit crusher state
of the master buffer by calling the corresponding function in effects.
@param pot_value value of the potentiometer.
*/
void change_master_bit_crusher(int pot_value);

/*
@brief helper function that changes the pitch value of the sample by 
calling the corresponding function in effects based on the potentiometer value.
@param pot_value value of the potentiometer.
*/
void change_pitch(int pot_value);

/*
@brief function that sets the mode of the pressed button based on the
potentiometer value.
@param pot_value value of the potentiometer.
*/
void rotate_mode(int pot_value);

/*
@brief function that changes the sample/master bit depth based
on the pressed button by calling the correct helper function.
@param pot_value value of the potentiometer.
*/
void change_bit_depth(int pot_value);

/*
@brief function that changes the sample bit depth based
on the pressed button by calling the correct helper function.
@param pot_value value of the potentiometer.
*/
void change_sample_bit_depth(int pot_value);

/*
@brief function that changes the master bit depth
by calling the correct helper function.
@param pot_value value of the potentiometer.
*/
void change_master_bit_depth(int pot_value);

/*
@brief function that changes the sample/master downsample value based
on the pressed button by calling the correct helper function.
@param pot_value value of the potentiometer.
*/
void change_downsample(int pot_value);

/*
@brief function that changes the sample downsample value based
on the pressed button by calling the correct helper function.
@param pot_value value of the potentiometer.
*/
void change_sample_downsample(int pot_value);

/*
@brief function that changes the master downsample value 
by calling the correct helper function.
@param pot_value value of the potentiometer.
*/
void change_master_downsample(int pot_value);

/*
@brief function that changes the sample/master distortion gain 
value based on the pressed button by calling the correct helper function.
@param pot_value value of the potentiometer.
*/
void change_distortion_gain(int pot_value);

/*
@brief function that changes the sample distortion gain 
value based on the pressed button by calling the correct helper function.
@param pot_value value of the potentiometer.
*/
void change_sample_distortion_gain(int pot_value);

/*
@brief function that changes the master distortion gain 
value by calling the correct helper function.
@param pot_value value of the potentiometer.
*/
void change_master_distortion_gain(int pot_value);

/*
@brief function that changes the sample/master threshold value based
on the pressed button by calling the correct helper function.
@param pot_value value of the potentiometer.
*/
void change_distortion_threshold(int pot_value);

/*
@brief function that changes the sample threshold value based
on the pressed button by calling the correct helper function.
@param pot_value value of the potentiometer.
*/
void change_sample_distortion_threshold(int pot_value);

/*
@brief function that changes the master threshold value 
by calling the correct helper function.
@param pot_value value of the potentiometer.
*/
void change_master_distortion_threshold(int pot_value);

/*
@brief function that changes the metronome state
by calling the correct helper function.
@param pot_value value of the potentiometer.
*/
void change_metronome(int pot_value);

/*
@brief function that changes the metronome bpm
by calling the correct helper function.
@param pot_value value of the potentiometer.
*/
void change_metronome_bpm(int pot_value);

/*
@brief function that changes the chopping start value based
on the pressed button by calling the correct helper function.
@param pot_value value of the potentiometer.
*/
void change_chopping_start(int pot_value);

/*
@brief function that changes the chopping end value based
on the pressed button by calling the correct helper function.
@param pot_value value of the potentiometer.
*/
void change_chopping_end(int pot_value);

/*
@brief helper function that based on the potentiometer value
returns a specific mode.
@param pot_value value of the potentiometer.
*/
pb_mode_t next_mode(int pot_value);

/*
@brief function that loads the sample based on the index.
*/
void sample_load();

/*
@brief function that sends a message to the fsm_queue.
@param source source of the message.
@param payload payload of the message (its meaning depends on the source).
*/
void send_message_to_fsm_queue(message_source_t source, int payload);

/*
@brief function that sends a message to the fsm_queue from an ISR.
@param source source of the message.
@param payload payload of the message (its meaning depends on the source).
*/
void send_message_to_fsm_queue_from_ISR(message_source_t source, int payload);

/*
@brief function that sets the global last pot value to the one sent.
@param pot_value value of the potentiometer.
*/
void set_last_pot_value(int pot_value);

/*
@brief function that initializes the curr_menu, creates the fsm_queue and
creates the main fsm task.
*/
void fsm_init();

#pragma endregion

#endif