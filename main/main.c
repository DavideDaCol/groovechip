#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h" // Add this
#include "lcd.h"

static const char *TAG = "APP";

void app_main(void)
{
    lcd_driver_init();

    while(1) {
        print_single("Hello World!   ");

        vTaskDelay(pdTICKS_TO_MS(4000));
        
        print_double("Come           ", "Stai           ");

        vTaskDelay(pdTICKS_TO_MS(4000));
    }
    


}