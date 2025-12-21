idf_component_register(
    SRCS "joystick.c"
    INCLUDE_DIRS "."
    # questo serve per linkare moduli da compilare prima di questo
    PRIV_REQUIRES 
        freertos
        driver
        esp_adc
)