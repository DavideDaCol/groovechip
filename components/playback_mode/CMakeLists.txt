idf_component_register(
    SRCS "playback_mode.c"
    INCLUDE_DIRS "include"
    REQUIRES driver mixer pad_section freertos esp_timer
)