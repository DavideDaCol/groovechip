idf_component_register(SRCS "fsm.c"
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
                        lcd
                        sd_reader
                    INCLUDE_DIRS "include")