idf_component_register(SRCS "fsm.c"
                    REQUIRES driver freertos esp_timer
                    spi_flash 
                    pad_section 
                    i2s 
                    playback_mode 
                    effects 
                    joystick 
                    adc1
                    lcd
                    sd_reader
                    recorder
                    INCLUDE_DIRS "include")