#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_mac.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include <driver/i2s_std.h>
#include "esp_adc/adc_continuous.h"

#define BLINK_GPIO 2
#define BIT_FIDELITY 16
#define SAMPLE_RATE 44100

static TaskHandle_t main_handler;
static adc_channel_t channel[2] = {ADC_CHANNEL_6, ADC_CHANNEL_7};

////config per l'output via i2s
//i2s_std_config_t is2conf = {
//    .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE), //imposta il clock al sample rate di registrazione
//    //info sulla philips mode: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/i2s.html
//    .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(BIT_FIDELITY,I2S_SLOT_MODE_STEREO),//usa la philips mode a 16 bit in stereo
//    .gpio_cfg = {
//
//    }
//};

void simpleRecTask(void *parameters){
    
}

//la funzione si attiva quando il buffer dell'ADC è pieno, dicendo al main di elaborare quello che è stato raccolto
static bool IRAM_ATTR full_mic_buffer_ISR(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *evtdata, void *userdata){
    
    BaseType_t stop_conversion = pdFALSE;
    
    //notifica il driver ADC che è stato raggiunto il giusto numero di conversioni
    vTaskNotifyGiveFromISR(main_handler,&stop_conversion);
    return (stop_conversion == pdTRUE);
}

static void adc_setup(adc_channel_t *channel, uint8_t channel_number, adc_continuous_handle_t *adc_handle){
    
    //configura il driver dell'adc
    adc_continuous_handle_t mic_adc = NULL;
    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = 1024,
        .conv_frame_size = 256,
        .flags = {
            .flush_pool = 1
        }
    };
    //crea nuovo handler adc, viene piazzato dentro mic_adc
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &mic_adc));

    //configura i parametri di sampling dell'ADC
    adc_continuous_config_t sampling_cfg = {
        .sample_freq_hz = 44100,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE1
    };

    //setup di ogni canale ADC
    adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {0};
    sampling_cfg.pattern_num = channel_number;
    for (int i = 0; i < channel_number; i++){
        adc_pattern[i].atten = ADC_ATTEN_DB_11; //attenuatore del segnale audio, aiuta con il noise floor
        adc_pattern[i].channel = channel[i] & 0x7; //TODO: capire a cosa serve il 0x7 (sta nella reference espressif)
        adc_pattern[i].unit = ADC_UNIT_1; //"unit: the ADC that the IO is subordinate to"
        adc_pattern[i].bit_width = SOC_ADC_DIGI_MAX_BITWIDTH; //granularità del sample finale
    }
    sampling_cfg.adc_pattern = adc_pattern;
    ESP_ERROR_CHECK(adc_continuous_config(mic_adc,&sampling_cfg));

    *adc_handle = mic_adc;

}

void app_main(void)
{
    //inizializza vettore per i sample del microfono
    esp_err_t rec_status;
    uint32_t ret_num = 0;
    uint8_t mic_samples[256] = {0};
    memset(mic_samples,0xcc,256);
    uint32_t global_cnt = 0;
    
    //salva handler della task principale per notificare il programma che il buffer ADC è pieno (via ISR)
    main_handler = xTaskGetCurrentTaskHandle();
    
    //inizializza parametri ADC
    adc_continuous_handle_t adc_driver = NULL;
    adc_setup(channel, sizeof(channel) / sizeof(adc_channel_t), &adc_driver);

    //imposta callback per gestire il momento in cui il buffer si riempie
    adc_continuous_evt_cbs_t full_buffer_cbs = {
        .on_conv_done = full_mic_buffer_ISR,
    };
    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(adc_driver,&full_buffer_cbs,NULL));

    //fa finalmente partire la registrazione dei sample mediante il driver ADC
    ESP_ERROR_CHECK(adc_continuous_start(adc_driver));

    ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
    while(1){
        //registra dal pin del microfono dentro a result
        rec_status = adc_continuous_read(adc_driver, mic_samples, 256, &ret_num, 0);
        if (rec_status == ESP_OK) {
            for(int i = 0; i < ret_num; i += SOC_ADC_DIGI_RESULT_BYTES){
                adc_digi_output_data_t *rec_output = (adc_digi_output_data_t *)&mic_samples[i];
                uint32_t channel_num = rec_output->type1.channel;
                uint32_t raw_data = rec_output->type1.data;

                //stampa il dato registrato solo se è di un canale attivo
                if(channel_num < SOC_ADC_CHANNEL_NUM(ADC_UNIT_1)){
                    //ESP_LOGI("REC", "%u", raw_data);
                    printf("%lu\t%lu\r\n", global_cnt, raw_data);
                    global_cnt++;
                }
            }
            vTaskDelay(1);
        }
    }

    //xTaskCreate(&simpleRecTask, "simple ADC recording task", 2048, NULL, 5, NULL);
    ESP_ERROR_CHECK(adc_continuous_stop(adc_driver));
    ESP_ERROR_CHECK(adc_continuous_deinit(adc_driver));
}