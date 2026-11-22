idf_component_register(SRCS 
                        "uart_module.c"
                        "uart_module2.c"
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "../../../Shared"
                    REQUIRES
                        esp_driver_uart
                        esp_driver_gpio
                        debug_buffer)
