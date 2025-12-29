#include "pad_section.h"
#include "mixer.h"

#pragma region SAMPLE_MODES

#pragma endregion


// pad config
pad_settings_t pads_config[GPIO_NUM_MAX];

// init queue
QueueHandle_t pads_evt_queue = NULL;

// debounce + detecting press/release
static volatile uint64_t last_isr_time[GPIO_NUM_MAX] = {0};
static volatile int last_level[GPIO_NUM_MAX];

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

	pad_queue_msg_t queue_msg;

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

void pad_section_init(){
	// GPIO config
	gpio_config_t io_conf = {};
	io_conf.intr_type = GPIO_INTR_ANYEDGE; // on press and release
	io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pull_up_en = 1;
	io_conf.pull_down_en = 0;
	gpio_config(&io_conf);


	for(int i = 0; i < GPIO_NUM_MAX; i++){
		pad_settings_t init_settings;
		init_settings.sample_id = 0;

		pads_config[i] = init_settings;
	}

	// init last level to 1, so it doesn't ignore the first press
	for(int i = 0; i < GPIO_NUM_MAX; i++){
		last_level[i] = 1;
	}

	// init queue
	pads_evt_queue = xQueueCreate(10, sizeof(pad_queue_msg_t));

	// ISR installation
	ESP_ERROR_CHECK(gpio_install_isr_service(0));
	gpio_isr_handler_add(GPIO_BUTTON_1, gpio_isr_handler, (void *)GPIO_BUTTON_1);
	gpio_isr_handler_add(GPIO_BUTTON_2, gpio_isr_handler, (void *)GPIO_BUTTON_2);
	gpio_isr_handler_add(GPIO_BUTTON_3, gpio_isr_handler, (void *)GPIO_BUTTON_3);
	gpio_isr_handler_add(GPIO_BUTTON_4, gpio_isr_handler, (void *)GPIO_BUTTON_4);
	gpio_isr_handler_add(GPIO_BUTTON_5, gpio_isr_handler, (void *)GPIO_BUTTON_5);
	gpio_isr_handler_add(GPIO_BUTTON_6, gpio_isr_handler, (void *)GPIO_BUTTON_6);
	gpio_isr_handler_add(GPIO_BUTTON_7, gpio_isr_handler, (void *)GPIO_BUTTON_7);
	gpio_isr_handler_add(GPIO_BUTTON_8, gpio_isr_handler, (void *)GPIO_BUTTON_8);
	

	printf("[Pad Section] Ready\n");
}