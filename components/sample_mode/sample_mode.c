#include "sample_mode.h"
#include "mixer.h"
#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#pragma region sample_modes_config

// init queue
QueueHandle_t sample_evt_queue = NULL;

const sample_mode_t MODE_HOLD = {
    .on_press   = action_start_sample,
    .on_release = action_stop_sample,
    .on_finish  = action_stop_sample
};

const sample_mode_t MODE_LOOP = {
    .on_press   = action_start_sample,
    .on_release = action_stop_sample,
    .on_finish  = action_restart_sample
};

const sample_mode_t MODE_ONESHOT = {
    .on_press   = action_restart_sample,
    .on_release = action_ignore,  
    .on_finish  = action_stop_sample
};

const sample_mode_t MODE_ONESHOT_LOOP = {
    .on_press   = action_start_or_stop_sample,
    .on_release = action_ignore,
    .on_finish  = action_restart_sample
};

const sample_mode_t* SAMPLE_MODES[] = {
    &MODE_HOLD,
	&MODE_ONESHOT,
    &MODE_LOOP,
    &MODE_ONESHOT_LOOP
};

const sample_mode_t* samples_config[SAMPLE_NUM];

// exposed function to set the mode
void set_sample_mode(int sample_id, const sample_mode_t* mode){
	samples_config[sample_id] = mode;
}

void send_event(uint8_t sample_id, enum evt_type_t event_type){
	sample_queue_msg_t queue_msg;

	queue_msg.event_type = event_type;
	queue_msg.sample_id = sample_id;

	xQueueSendFromISR(sample_evt_queue, &queue_msg, NULL);
}
void sample_task(void *pvParameter){
	sample_queue_msg_t queue_msg;

	for (;;)
	{
		if (xQueueReceive(sample_evt_queue, &queue_msg, portMAX_DELAY) == pdPASS)
		{
            // retrieve sample_id from pad settings
			uint8_t sample_id = queue_msg.sample_id;

            if(sample_id == NOT_DEFINED){
                // there is no associated sample to this pad
                printf("sample id was NOT defined\n");
                fflush(stdout);
                continue;
            }

            printf("sample id is set to %d\n", sample_id);
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
		// give to the processor the possibility of switching task
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}

void sample_mode_init(){
    // set default sample mode (HOLD)
	for(int i = 0; i < GPIO_NUM_MAX; i++){
		set_sample_mode(i, SAMPLE_MODES[HOLD]);
	}

	// init queue
	sample_evt_queue = xQueueCreate(10, sizeof(sample_queue_msg_t));

    xTaskCreate(sample_task, "sample_task", 2048, NULL, 5, NULL);
    printf("[Sample] Ready\n");
}


#pragma endregion