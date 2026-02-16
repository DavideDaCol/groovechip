idf_component_register(SRCS "fsm.c"
                    REQUIRES driver freertos esp_timer
                    playback_mode 
                    joystick 
                    sd_reader
                    PRIV_REQUIRES
                    spi_flash 
                    pad_section 
                    i2s 
                    effects 
                    adc1
                    lcd
                    recorder
                    INCLUDE_DIRS "include")