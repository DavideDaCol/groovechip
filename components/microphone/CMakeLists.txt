idf_component_register(
    SRCS "microphone.c"
    INCLUDE_DIRS "include"
    # questo serve per linkare moduli da compilare prima di questo
    REQUIRES 
        i2s
        mixer
)