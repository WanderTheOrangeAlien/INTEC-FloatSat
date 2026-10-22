#ifndef PROJECT_DEFS_H
#define PROJECT_DEFS_H

#include "driver/uart.h"
#include "soc/uart_pins.h"

#define MAIN_UART_RX_BUFFER_SIZE    1024
#define MAIN_UART_TX_BUFFER_SIZE    1024
#define MAIN_UART_PORT_NUM          UART_NUM_2
#define MAIN_UART_PIN_RX            4                      
#define MAIN_UART_PIN_TX            5                      
#define MAIN_UART_BAUDRATE          115200
#define MAIN_UART_QUEUE_SIZE        10

#endif