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
        print_single("Hello World!   \0");

        vTaskDelay(pdMS_TO_TICKS(4000));
        
        print_double("Come           \0", "Stai           \0");

        vTaskDelay(pdMS_TO_TICKS(4000));
    }
    


}