#include "pad_section.h"

// Gestore dell'interrupt (ISR)
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
  uint32_t gpio_num = (uint32_t) arg;

  //debounce
  uint64_t current_time = esp_timer_get_time();

  if (current_time - last_isr_time[gpio_num] < 50000) 
  {
    //ignore
    return;
  }

  //check on level
  int level = gpio_get_level(gpio_num);

  if(level == last_level[gpio_num]){
    //ignore
    return;
  }


  //update time and level

  last_isr_time[gpio_num] = current_time;
  last_level[gpio_num] = level;

  //check on level

  btn_queue_msg queue_msg;

  if(level == 0){
    //pressed
    sprintf(queue_msg.payload, "%s", "pressed");
  }
  else{
    //released
    sprintf(queue_msg.payload, "%s", "released");
  }
  queue_msg.gpio_num = gpio_num;
  

  xQueueSendFromISR(btn_evt_queue, &queue_msg, NULL);
}

void button_task(void *pvParameter)
{
  btn_queue_msg queue_msg;

  for( ;; ) {
    if(xQueueReceive(btn_evt_queue, &queue_msg, portMAX_DELAY) == pdPASS){
      //play the sound
      printf("%s da %d\n", queue_msg.payload,(int)queue_msg.gpio_num);

    }
    // give to the processor the possibility of switching task
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }

}

void init(){
  // GPIO config
  gpio_config_t io_conf = {};
  io_conf.intr_type = GPIO_INTR_ANYEDGE; //on press and release
  io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pull_up_en = 1;
  io_conf.pull_down_en = 0;
  gpio_config(&io_conf);

  // init queue
  btn_evt_queue = xQueueCreate(10, sizeof(btn_queue_msg));

  // ISR installation
  ESP_ERROR_CHECK(gpio_install_isr_service(0));
  gpio_isr_handler_add(GPIO_BUTTON_1, gpio_isr_handler, (void*) GPIO_BUTTON_1);
  gpio_isr_handler_add(GPIO_BUTTON_2, gpio_isr_handler, (void*) GPIO_BUTTON_2);

  printf("Sistema pronto. Premi i pulsanti.\n");
  
  xTaskCreate(button_task, "btn_task", 2048, NULL, 5, NULL);
}