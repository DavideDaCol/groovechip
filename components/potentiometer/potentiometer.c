#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"
#include "esp_mac.h"
#include "freertos/queue.h"
#include "include/potentiometer.h" 

#define POT_CHANNEL ADC_CHANNEL_5  // GPIO 37
#define POT_READ_INTERVAL_MS 100   // reading interval in ms

QueueHandle_t pot_queue;
int pot_value = 0;  // Valore grezzo (0-4095 con 12 bit)

void potentiometer_init() {
    
    // configure the ADC channel
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,  // 12 bit (0-4095)
        .atten = ADC_ATTEN_DB_12,          // Range 0V - 3.3V
    };
    
    adc_oneshot_config_channel(adc1_handle, POT_CHANNEL, &config);
    
    // creating the queue
    pot_queue = xQueueCreate(10, sizeof(int));

    // create the task
    xTaskCreate(potentiometer_task, "pot_task", 2048, NULL, 5, NULL);
}

int potentiometer_read_raw() {
    int raw_value;
    adc_oneshot_read(adc1_handle, POT_CHANNEL, &raw_value);
    return raw_value;
}

// converts the raw value to (0-100)
int potentiometer_read_percent() {
    int raw = potentiometer_read_raw();
    return (raw * 100) / 4095;
}

// Converti il valore grezzo in voltaggio (0.0 - 3.3V)
float potentiometer_read_voltage() {
    int raw = potentiometer_read_raw();
    return (raw * 3.3f) / 4095.0f;
}

// main task
void potentiometer_task(void *args) {
    // the default value is set to 0 (it is not printed or sent to the queue)
    int last_pot_value = 0;
    while(1) {
        // reading the value
        pot_value = potentiometer_read_raw();
        int percent = (pot_value * 100) / 4095;
        float voltage = (pot_value * 3.3f) / 4095.0f;
        int diff = pot_value - last_pot_value;
        int diff_percent = (diff * 100) /4095;
        
        // ignore the repetitive events
        if (pot_value != last_pot_value){
            printf("Pot: %d (raw) | %d%% | %.2fV\n", pot_value, percent, voltage);
            xQueueSend(pot_queue, &diff_percent, 0); // parameters -> queue name, message, tick to wait to send the message
            last_pot_value = pot_value;
        }
        
        vTaskDelay(pdMS_TO_TICKS(POT_READ_INTERVAL_MS));
    }
}