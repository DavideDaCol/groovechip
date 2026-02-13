idf_component_register(
    SRCS "adc1.c"
    INCLUDE_DIRS "include"
    # questo serve per linkare moduli da compilare prima di questo
    PRIV_REQUIRES 
        freertos
        driver
        esp_adc
)