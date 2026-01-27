#include "pad_section.h"
#include "mixer.h"
#include "sample_mode.h"

#pragma region SAMPLE_MODES

#pragma endregion


// pad config
pad_settings_t pads_config[GPIO_NUM_MAX];

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

	enum evt_type_t event_type;

	if (level == 0)
	{
		// pressed
		event_type = EVT_PRESS;
	}
	else
	{
		// released
		event_type = EVT_RELEASE;
	}

	// send the event on the sample task
	uint8_t sample_id = pads_config[gpio_num].sample_id;
	send_event(sample_id, event_type);
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