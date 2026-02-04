idf_component_register(
    SRCS "pad_section.c"
    INCLUDE_DIRS "include"
    REQUIRES driver mixer freertos esp_timer iot_button
)
