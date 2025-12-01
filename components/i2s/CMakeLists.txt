idf_component_register(
    SRCS "i2s_driver.c"
    INCLUDE_DIRS "include"
    # questo serve per linkare moduli da compilare prima di questo
    REQUIRES driver
)
