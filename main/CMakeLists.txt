idf_component_register(SRCS "main.c"
                    REQUIRES driver freertos esp_timer
                    PRIV_REQUIRES spi_flash pad_section potentiometer
                    INCLUDE_DIRS ".")