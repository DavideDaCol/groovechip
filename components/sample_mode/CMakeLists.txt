idf_component_register(
    SRCS "sample_mode.c"
    INCLUDE_DIRS "include"
    REQUIRES driver mixer pad_section freertos esp_timer
)