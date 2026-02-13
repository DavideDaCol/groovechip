idf_component_register(
    SRCS "joystick.c"
    INCLUDE_DIRS "include"
    # questo serve per linkare moduli da compilare prima di questo
    REQUIRES
        freertos
        driver
        esp_adc
        adc1
        fsm
        mixer
)