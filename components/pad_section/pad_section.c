#include "pad_section.h"
#include "mixer.h"
#include "playback_mode.h"
#include "esp_log.h"
static const char* TAG = "PadSection";

// pad config
// pad_settings_t pads_config[GPIO_NUM_MAX];

// debounce + detecting press/release
static volatile uint64_t last_isr_time[GPIO_NUM_MAX] = {0};
static volatile int last_level[GPIO_NUM_MAX];

//  pads interrupt handler
static void IRAM_ATTR gpio_isr_handler(void *arg){
	uint32_t pad_id = (uint32_t)arg;

	// debounce
	uint64_t current_time = esp_timer_get_time();

	if (current_time - last_isr_time[pad_id] < 50000)
	{
		// ignore
		return;
	}

	// check on level
	int level = gpio_get_level(pad_id);

	if (level == last_level[pad_id])
	{
		// ignore
		return;
	}

    ESP_EARLY_LOGI(TAG, "ISR pad %lu level %d\n", pad_id, level);


	// update time and level

	last_isr_time[pad_id] = current_time;
	last_level[pad_id] = level;

	// check on level to determinate the event type

	enum evt_type_t event_type = level == 0 ? EVT_PRESS : EVT_RELEASE;


	// send the event on the sample task
	send_pad_event(pad_id, event_type);
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