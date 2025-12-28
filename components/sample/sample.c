#include "sample.h"
#include "mixer.h"
#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#pragma region sample_modes_config

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
    .on_press   = action_start_sample,
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

void sample_task(void *pvParameter){
	pad_queue_msg_t queue_msg;

	for (;;)
	{
		if (xQueueReceive(pads_evt_queue, &queue_msg, portMAX_DELAY) == pdPASS)
		{
            // retrieve sample_id from pad settings
			pad_settings_t settings = pads_config[queue_msg.pad_id];
			uint32_t sample_id = settings.sample_id;

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

void sample_init(){
    // set default sample mode (HOLD)
	for(int i = 0; i < GPIO_NUM_MAX; i++){
		set_sample_mode(i, SAMPLE_MODES[HOLD]);
	}

    xTaskCreate(sample_task, "sample_task", 2048, NULL, 5, NULL);
    printf("[Sample] Ready\n");
}

#pragma endregion