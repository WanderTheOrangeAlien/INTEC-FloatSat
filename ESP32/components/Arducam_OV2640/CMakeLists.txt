idf_component_register(SRCS 
                        "Arducam_OV2640.c"
                        "OV2640_init_seq.c"
                    INCLUDE_DIRS "include"
                    REQUIRES
                        esp_driver_spi
                        esp_driver_i2c)