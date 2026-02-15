#include "pad_section.h"
#include "mixer.h"
#include "playback_mode.h"
#include "esp_log.h"
#include "fsm.h"
static const char* TAG = "PadSection";
static uint8_t map_gpio_to_pad_num[GPIO_NUM_MAX] = {0};

// pad config
// pad_settings_t pads_config[GPIO_NUM_MAX];

// debounce + detecting press/release
static volatile uint64_t last_isr_time[GPIO_NUM_MAX] = {0};
static volatile int last_level[GPIO_NUM_MAX];

uint8_t get_pad_num(int gpio_num){
	if (gpio_num < GPIO_NUM_MAX)
		return map_gpio_to_pad_num[gpio_num];
	else return PAD_NUM_NOT_DEFINED;
}

//  pads interrupt handler
static void IRAM_ATTR gpio_isr_handler(void *arg){
	uint32_t pad_id = (uint32_t)arg;

	// debounce
	uint64_t current_time = esp_timer_get_time();

	if (current_time - last_isr_time[pad_id] < DEBOUNCE_MS)
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

	if (event_type == EVT_PRESS){
		send_message_to_fsm_queue_from_ISR(PAD, pad_id);
	}

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

	const uint8_t BUTTONS[PAD_NUM] = {
		GPIO_BUTTON_1,
		GPIO_BUTTON_2,
		GPIO_BUTTON_3,
		GPIO_BUTTON_4,
		GPIO_BUTTON_5,
		GPIO_BUTTON_6,
		GPIO_BUTTON_7,
		GPIO_BUTTON_8,
	};

	// init map to not mapped
	for(int i = 0; i < GPIO_NUM_MAX; i++){
		map_gpio_to_pad_num[i] = PAD_NUM_NOT_DEFINED;
	}
	
	ESP_ERROR_CHECK(gpio_install_isr_service(0));

	
	for(int i = 0; i < PAD_NUM; i++){
		gpio_isr_handler_add(BUTTONS[i], gpio_isr_handler, (void *)BUTTONS[i]); // ISR installation
		map_gpio_to_pad_num[BUTTONS[i]] = i + 1; // map gpio to pad num
        ESP_LOGI(TAG, "mapped GPIO %i to pad number %i", BUTTONS[i], map_gpio_to_pad_num[BUTTONS[i]]);
	}

	ESP_LOGI(TAG, "[Pad Section] Ready\n");
}