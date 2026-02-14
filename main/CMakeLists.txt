idf_component_register(SRCS "main.c"
                    REQUIRES driver freertos esp_timer
                    PRIV_REQUIRES 
                        spi_flash 
                        pad_section 
                        i2s 
                        mixer 
                        playback_mode 
                        effects 
                        joystick 
                        potentiometer
                        adc1
                        fsm
                        lcd
                        sd_reader
                        json
                        sd_reader
                    INCLUDE_DIRS ".")
