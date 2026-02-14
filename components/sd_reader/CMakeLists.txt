idf_component_register(
    SRCS "sd_reader.c"
    INCLUDE_DIRS "include"
    REQUIRES 
        mixer
        spi
        json
        effects
        sdmmc
        fatfs
        esp_psram
)
