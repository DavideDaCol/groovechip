#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"
#include "esp_mac.h"
#include "freertos/queue.h"
#include "include/potentiometer.h" 
#include "esp_log.h"
#include <stdlib.h>
#include "fsm.h"

#define POT_CHANNEL ADC_CHANNEL_0  // GPIO 36
#define POT_READ_INTERVAL_MS 20   // reading interval in ms

const char* TAG_POT = "Potentiometer";
int pot_value = 0;  // Valore grezzo (0-4095 con 12 bit)

void potentiometer_init() {
    
    // configure the ADC channel
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,  // 12 bit (0-4095)
        .atten = ADC_ATTEN_DB_12,          // Range 0V - 3.3V
    };
    
    adc_oneshot_config_channel(adc1_handle, POT_CHANNEL, &config);

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

int potentiometer_read_filtered() {
    int sum = 0;
    for(int i = 0; i < 8; i++) {
        int raw;
        adc_oneshot_read(adc1_handle, POT_CHANNEL, &raw);
        sum += raw;
    }
    return sum / 8;
}

// main task
void potentiometer_task(void *args) {
    // the default value is set to 0 (it is not printed or sent to the queue)
    int last_pot_value = potentiometer_read_filtered();
    int last_diff_percentage = (last_pot_value * 100) / 4095;
    while(1) {
        // reading the value
        pot_value = potentiometer_read_filtered();
        int percent = (pot_value * 100) / 4095;
        float voltage = (pot_value * 3.3f) / 4095.0f;
        int diff = pot_value - last_pot_value;
        int diff_percent = (diff * 100) /4095;
        
        // ignore the repetitive events
        if (abs(diff_percent - last_diff_percentage) > 0){
            printf("Pot: %d (raw) | %d%% | %.2fV\n", pot_value, percent, voltage);
            send_message_to_fsm_queue(POTENTIOMETER, diff_percent);
            last_pot_value = pot_value;
            last_diff_percentage = diff_percent;
        }
        
        vTaskDelay(pdMS_TO_TICKS(POT_READ_INTERVAL_MS));
    }
}