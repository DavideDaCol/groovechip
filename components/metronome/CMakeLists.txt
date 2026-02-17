idf_component_register(
    SRCS "metronome.c"
    INCLUDE_DIRS "include"
    REQUIRES driver freertos mixer 
)
