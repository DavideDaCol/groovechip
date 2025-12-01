idf_component_register(
    SRCS "example.c"
    INCLUDE_DIRS "include"
    # questo serve per linkare moduli da compilare prima di questo
    # REQUIRES dependency
)
