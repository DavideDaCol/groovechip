idf_component_register(
    SRCS "potentiometer.c"
    INCLUDE_DIRS "include"
    # questo serve per linkare moduli da compilare prima di questo
    REQUIRES 
        freertos
        driver
        esp_adc
        joystick
        adc1
)