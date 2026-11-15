idf_component_register(SRCS "uart_module.c"
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "../../../Shared"
                    REQUIRES
                        esp_driver_uart
                        esp_driver_gpio)
