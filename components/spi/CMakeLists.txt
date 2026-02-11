idf_component_register(
    SRCS "spi_driver.c"
    INCLUDE_DIRS "include"
    REQUIRES 
        sdmmc
        driver
    PRIV_REQUIRES
        esp_driver_spi
        esp_driver_sdspi
)