#include "playback_mode.h"
#include "mixer.h"
#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"

const char* TAG_PM = "PlaybackMode";

#pragma region PAYBACK MODES

// init queue
QueueHandle_t playback_evt_queue = NULL;

const playback_mode_t MODE_HOLD = {
	.mode 		= HOLD,
    .on_press   = action_start_sample,
    .on_release = action_stop_sample,
    .on_finish  = action_stop_sample
};

const playback_mode_t MODE_LOOP = {
	.mode 		= LOOP,
    .on_press   = action_start_sample,
    .on_release = action_stop_sample,
    .on_finish  = action_restart_sample
};

const playback_mode_t MODE_ONESHOT = {
	.mode 		= ONESHOT,
    .on_press   = action_restart_sample,
    .on_release = action_ignore,  
    .on_finish  = action_stop_sample
};

const playback_mode_t MODE_ONESHOT_LOOP = {
	.mode		= ONESHOT_LOOP,
    .on_press   = action_start_or_stop_sample,
    .on_release = action_ignore,
    .on_finish  = action_restart_sample
};

const playback_mode_t* PLAYBACK_MODES[] = {
    &MODE_HOLD,
	&MODE_ONESHOT,
    &MODE_LOOP,
    &MODE_ONESHOT_LOOP
};

static const playback_mode_t* samples_config[SAMPLE_NUM];
static int pad_to_sample_map[GPIO_NUM_MAX];

uint8_t get_sample_bank_index(uint8_t pad_id){
	if(pad_id >= 0 && pad_id < GPIO_NUM_MAX){
		return pad_to_sample_map[pad_id];
	}
	else return NOT_DEFINED;
}

mode_t get_playback_mode(uint8_t bank_index){
	if(bank_index < SAMPLE_NUM){
		return samples_config[bank_index]->mode;
	}
	else return UNSET;
}

// exposed function to set the mode
void set_playback_mode(uint8_t bank_index, mode_t playback_mode){
	if (bank_index < SAMPLE_NUM){
		switch (playback_mode)
		{
		case HOLD:
			samples_config[bank_index] = &MODE_HOLD;
			break;
		case ONESHOT:
			samples_config[bank_index] = &MODE_ONESHOT;
			break;
		case LOOP:
			samples_config[bank_index] = &MODE_LOOP;
			break;
		case ONESHOT_LOOP:
			samples_config[bank_index] = &MODE_ONESHOT_LOOP;
			break;
		default:
			break;
		}
	}
}

// exposed function map pad_id to bank_index
void map_pad_to_sample(uint8_t pad_id, uint8_t bank_index){
	if (bank_index != NOT_DEFINED)
		pad_to_sample_map[pad_id] = bank_index;
}

// send messages into playback_evt_queue from the pad section
void IRAM_ATTR send_pad_event(uint8_t pad_id, enum evt_type_t type) {
    playback_msg_t msg;
    msg.source = SRC_PAD_SECTION;
    msg.event_type = type;
    msg.payload.pad_id = pad_id;
    
    xQueueSendFromISR(playback_evt_queue, &msg, NULL);
}

// send messages into playback_evt_queue from the mixer section
void IRAM_ATTR send_mixer_event(uint8_t bank_index, enum evt_type_t type) {
    playback_msg_t msg;
    msg.source = SRC_MIXER;
    msg.event_type = type;
    msg.payload.bank_index = bank_index;

	xQueueSendFromISR(playback_evt_queue, &msg, NULL);
}
void sample_task(void *pvParameter){
	playback_msg_t queue_msg;

	for (;;)
	{
		if (xQueueReceive(playback_evt_queue, &queue_msg, portMAX_DELAY) == pdPASS)
		{
			uint8_t bank_index = NOT_DEFINED;

			// retrieve bank_index based on the source
			switch (queue_msg.source)
			{
			case SRC_PAD_SECTION:
				//the message came from PAD_SECTION, get the associated bank_index
				if (queue_msg.payload.pad_id < GPIO_NUM_MAX) {
                    bank_index = pad_to_sample_map[queue_msg.payload.pad_id];
                }
				break;
			case SRC_MIXER:
				// the message came from the mixer, so we already have the bank_index
                bank_index = queue_msg.payload.bank_index;
				break;
			default:
				break;
			}

            if(bank_index == NOT_DEFINED){
                // there is no associated sample to this pad
                ESP_LOGW(TAG_PM,"sample id was NOT defined");
                fflush(stdout);
                continue;
            }

            fflush(stdout);

			switch (queue_msg.event_type)
			{
			case EVT_PRESS:
				samples_config[bank_index]->on_press(bank_index);
				break;
			case EVT_RELEASE:
				samples_config[bank_index]->on_release(bank_index);
				break;
			case EVT_FINISH:
				samples_config[bank_index]->on_finish(bank_index);
				break;
			default:
				break;
			}
		}
	}
}

void playback_mode_init(){
    // set default sample mode (HOLD)
	for(int i = 0; i < SAMPLE_NUM; i++){
		set_playback_mode(i, HOLD);
	}

	// init pad to sample to NOT_DEFINED
	for(int i = 0; i  < GPIO_NUM_MAX; i++){
		map_pad_to_sample(i, NOT_DEFINED);
	}

	// init queue
	playback_evt_queue = xQueueCreate(10, sizeof(playback_msg_t));
	
    xTaskCreate(sample_task, "sample_task", 2048, NULL, 5, NULL);
    printf("[Sample] Ready\n");
}


#pragma endregion