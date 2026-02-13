idf_component_register(
    SRCS "lcd.c"
    INCLUDE_DIRS "include"
    REQUIRES 
        esp_lcd
        esp_driver_i2c
)
