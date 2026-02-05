idf_component_register(
    SRCS mixer.c
    INCLUDE_DIRS "include"
    REQUIRES driver pad_section i2s playback_mode freertos
)
