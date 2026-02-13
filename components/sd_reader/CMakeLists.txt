idf_component_register(
    SRCS "sd_reader.c"
    INCLUDE_DIRS "include"
    REQUIRES 
        mixer
    PRIV_REQUIRES 
        json
        effects
        spi
        sdmmc
        fatfs
        esp_psram
)
