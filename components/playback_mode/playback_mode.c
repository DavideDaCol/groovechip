#include "playback_mode.h"
#include "mixer.h"
#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"

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

mode_t get_playback_mode(uint8_t sample_id){
	if(sample_id < SAMPLE_NUM){
		return samples_config[sample_id]->mode;
	}
	else return UNSET;
}

// exposed function to set the mode
void set_playback_mode(uint8_t sample_id, mode_t playback_mode){
	if (sample_id < SAMPLE_NUM){
		switch (playback_mode)
		{
		case HOLD:
			samples_config[sample_id] = &MODE_HOLD;
			break;
		case ONESHOT:
			samples_config[sample_id] = &MODE_ONESHOT;
			break;
		case LOOP:
			samples_config[sample_id] = &MODE_LOOP;
			break;
		case ONESHOT_LOOP:
			samples_config[sample_id] = &MODE_ONESHOT_LOOP;
			break;
		default:
			break;
		}
	}
}

// exposed function map pad_id to sample_id
void map_pad_to_sample(uint8_t pad_id, uint8_t sample_id){
	if (sample_id != NOT_DEFINED)
		pad_to_sample_map[pad_id] = sample_id;
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
void IRAM_ATTR send_mixer_event(uint8_t sample_id, enum evt_type_t type) {
    playback_msg_t msg;
    msg.source = SRC_MIXER;
    msg.event_type = type;
    msg.payload.sample_id = sample_id;

	xQueueSendFromISR(playback_evt_queue, &msg, NULL);
}
void sample_task(void *pvParameter){
	playback_msg_t queue_msg;

	for (;;)
	{
		if (xQueueReceive(playback_evt_queue, &queue_msg, portMAX_DELAY) == pdPASS)
		{
			uint8_t sample_id = NOT_DEFINED;

			// retrieve sample_id based on the source
			switch (queue_msg.source)
			{
			case SRC_PAD_SECTION:
				//the message came from PAD_SECTION, get the associated sample_id
				if (queue_msg.payload.pad_id < GPIO_NUM_MAX) {
                    sample_id = pad_to_sample_map[queue_msg.payload.pad_id];
                }
				break;
			case SRC_MIXER:
				// the message came from the mixer, so we already have the sample_id
                sample_id = queue_msg.payload.sample_id;
				break;
			default:
				break;
			}

            if(sample_id == NOT_DEFINED){
                // there is no associated sample to this pad
                printf("sample id was NOT defined\n");
                fflush(stdout);
                continue;
            }

            fflush(stdout);

			switch (queue_msg.event_type)
			{
			case EVT_PRESS:
				samples_config[sample_id]->on_press(sample_id);
				break;
			case EVT_RELEASE:
				samples_config[sample_id]->on_release(sample_id);
				break;
			case EVT_FINISH:
				samples_config[sample_id]->on_finish(sample_id);
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