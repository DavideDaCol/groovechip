idf_component_register(
    SRCS "sd_reader.c"
    INCLUDE_DIRS "include"
    REQUIRES 
        sdmmc
        spi
        fatfs
)
