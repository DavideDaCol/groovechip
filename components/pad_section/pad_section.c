#include "pad_section.h"

#pragma region SAMPLE_MODES
const sample_mode MODE_HOLD = {
    .on_press   = action_start_sample,
    .on_release = action_stop_sample,
    .on_finish  = action_stop_sample
};

const sample_mode MODE_LOOP = {
    .on_press   = action_start_sample,
    .on_release = action_stop_sample,
    .on_finish  = action_restart_sample
};

const sample_mode MODE_ONESHOT = {
    .on_press   = action_start_sample,
    .on_release = action_ignore,  
    .on_finish  = action_stop_sample
};

const sample_mode MODE_ONESHOT_LOOP = {
    .on_press   = action_start_or_stop_sample,
    .on_release = action_ignore,
    .on_finish  = action_restart_sample
};

const sample_mode* SAMPLE_MODES[] = {
    &MODE_HOLD,
    &MODE_LOOP,
    &MODE_ONESHOT,
    &MODE_ONESHOT_LOOP
};

#pragma endregion

#pragma region SAMPLE_ACTION
//pad sample mode config
const sample_mode* pads_config[GPIO_NUM_MAX];

void action_start_or_stop_sample(int pad_id){
	printf("Pad %d: Start/Stop playback", pad_id);
}

void action_start_sample(int pad_id){
	printf("Pad %d: Start Playback\n", pad_id);
}

void action_stop_sample(int pad_id){
	printf("Pad %d: Stop Playback\n", pad_id);
}

void action_restart_sample(int pad_id){
	printf("Pad %d: Rewind & Restart\n", pad_id);
}

void action_ignore(int pad_id){
	// nothing
}

#pragma endregion

// init queue
QueueHandle_t pads_evt_queue = NULL;

// debounce + detecting press/release
static volatile uint64_t last_isr_time[GPIO_NUM_MAX] = {0};
static volatile int last_level[GPIO_NUM_MAX] = {1};

//  pads interrupt handler
static void IRAM_ATTR gpio_isr_handler(void *arg){
	uint32_t gpio_num = (uint32_t)arg;

	// debounce
	uint64_t current_time = esp_timer_get_time();

	if (current_time - last_isr_time[gpio_num] < 50000)
	{
		// ignore
		return;
	}

	// check on level
	int level = gpio_get_level(gpio_num);

	if (level == last_level[gpio_num])
	{
		// ignore
		return;
	}

	// update time and level

	last_isr_time[gpio_num] = current_time;
	last_level[gpio_num] = level;

	// check on level

	pad_queue_msg queue_msg;

	if (level == 0)
	{
		// pressed
		queue_msg.event_type = EVT_PRESS;
	}
	else
	{
		// released
		queue_msg.event_type = EVT_RELEASE;
	}
	queue_msg.pad_id = gpio_num;

	xQueueSendFromISR(pads_evt_queue, &queue_msg, NULL);
}

#pragma endregion

void sample_task(void *pvParameter){
	pad_queue_msg queue_msg;

	for (;;)
	{
		if (xQueueReceive(pads_evt_queue, &queue_msg, portMAX_DELAY) == pdPASS)
		{
			uint32_t pad_id = queue_msg.pad_id;
			// checks on actions
			switch (queue_msg.event_type)
			{
			case EVT_PRESS:
				pads_config[pad_id]->on_press(pad_id);
				break;
			case EVT_RELEASE:
				pads_config[pad_id]->on_release(pad_id);
				break;
			case EVT_FINISH:
				pads_config[pad_id]->on_finish(pad_id);
				break;
			default:
				break;
			}
		}
	// give to the processor the possibility of switching task
    vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}

void init(){
	// GPIO config
	gpio_config_t io_conf = {};
	io_conf.intr_type = GPIO_INTR_ANYEDGE; // on press and release
	io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pull_up_en = 1;
	io_conf.pull_down_en = 0;
	gpio_config(&io_conf);

	// det default pad mode (HOLD)
	for(int i = 0; i < GPIO_NUM_MAX; i++){
		pads_config[i] = &MODE_HOLD;
	}

	// init queue
	pads_evt_queue = xQueueCreate(10, sizeof(pad_queue_msg));

	// ISR installation
	ESP_ERROR_CHECK(gpio_install_isr_service(0));
	gpio_isr_handler_add(GPIO_BUTTON_1, gpio_isr_handler, (void *)GPIO_BUTTON_1);
	gpio_isr_handler_add(GPIO_BUTTON_2, gpio_isr_handler, (void *)GPIO_BUTTON_2);

	printf("[Pad Section] Ready\n");

	xTaskCreate(sample_task, "sample_task", 2048, NULL, 5, NULL);
}