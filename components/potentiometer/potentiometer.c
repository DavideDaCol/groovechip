#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"
#include "esp_mac.h"
#include "freertos/queue.h"
#include "include/potentiometer.h" 
#include "esp_log.h"
#include <stdlib.h>
#include <math.h>
#include "fsm.h"

const char* TAG_POT = "Potentiometer";
int pot_value = 0;

void potentiometer_init() {
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    
    adc_oneshot_config_channel(adc1_handle, POT_CHANNEL, &config);

    set_last_pot_value(round(potentiometer_read_filtered() * 100 / 4096));

    xTaskCreate(potentiometer_task, "pot_task", 4096, NULL, 3, NULL);
}

int potentiometer_read_filtered() {
    int sum = 0;
    int valid_readings = 0;
    
    for(int i = 0; i < FILTER_SAMPLES; i++) {
        int raw;
        esp_err_t err = adc_oneshot_read(adc1_handle, POT_CHANNEL, &raw);
        
        if (err == ESP_OK && raw >= 0 && raw <= 4095) {
            sum += raw;
            valid_readings++;
        } else {
            ESP_LOGW(TAG_POT, "Invalid ADC reading: %d (err: %d)", raw, err);
        }
        
        // delay between samples
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    
    if (valid_readings < FILTER_SAMPLES / 2) {
        ESP_LOGE(TAG_POT, "Too many invalid readings (%d/%d)", 
                 FILTER_SAMPLES - valid_readings, FILTER_SAMPLES);
        return -1;  // error
    }
    
    return sum / valid_readings;
}

bool reaching_max(float voltage, int last_percent){
    if (voltage >= 3.3f && last_percent < 100){
        return true;
    }
    return false;
}

bool reaching_min(float voltage, int last_percent){
    if (voltage <= 0.0f && last_percent > 0){
        return true;
    }
    return false;
}

void potentiometer_task(void *args) {
    // initial value
    pot_value = potentiometer_read_filtered();
    if (pot_value < 0) {
        pot_value = 2048;  // set a mid value
    }
    
    int last_percentage = (pot_value * 100) / 4095;
    
    ESP_LOGI(TAG_POT, "Initialized at %d%% (%d raw)", last_percentage, pot_value);
    
    while(1) {
        int new_pot_value = potentiometer_read_filtered();
        
        // skip if it's an invalid value
        if (new_pot_value < 0) {
            vTaskDelay(pdMS_TO_TICKS(POT_READ_INTERVAL_MS));
            continue;
        }
        
        if (new_pot_value > 4095) {
            ESP_LOGW(TAG_POT, "Out of range value: %d", new_pot_value);
            vTaskDelay(pdMS_TO_TICKS(POT_READ_INTERVAL_MS));
            continue;
        }
        
        pot_value = new_pot_value;
        
        // compute percentage
        int percent = round((pot_value * 100) / 4095);
        float voltage = (pot_value * 3.3f) / 4095.0f;

        int diff = abs(percent - last_percentage);
        
        // send message to the queue
        if (diff >= THRESHOLD_PERCENT || reaching_max(voltage, last_percentage) || reaching_min(voltage, last_percentage)) {
            
            ESP_LOGI(TAG_POT, "Changed: %d%% (raw: %d, %.2fV, diff: %+d%%)", 
                     percent, pot_value, voltage, percent - last_percentage);
            
            send_message_to_fsm_queue(POTENTIOMETER, percent);
            last_percentage = percent;
        }
        
        vTaskDelay(pdMS_TO_TICKS(POT_READ_INTERVAL_MS));
    }
}