idf_component_register(
    SRCS effects.c
    INCLUDE_DIRS "include"
    REQUIRES driver freertos mixer playback_mode
)
