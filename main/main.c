#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lcd.h"

void app_main(void)
{
    // 1. Initialize the hardware
    lcd_driver_init();

    // 2. Initial Clear (Good practice to start fresh)
    LCD_clearScreen();
    
    while (1) {
        // --- Screen 1 ---
        LCD_clearScreen();
        LCD_setCursor(0x0, 0x0);       // Col 0, Row 0
        LCD_writeStr("GrooveChip");
        
        

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}