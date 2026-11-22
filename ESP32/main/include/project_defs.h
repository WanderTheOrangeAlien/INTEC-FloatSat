#ifndef PROJECT_DEFS_H
#define PROJECT_DEFS_H

#include "driver/uart.h"
#include "soc/uart_pins.h"
#include "soc/spi_pins.h"

#define MAIN_UART_RX_BUFFER_SIZE    1024
#define MAIN_UART_TX_BUFFER_SIZE    1024
#define MAIN_UART_INFO_BUFFER_SIZE  4096
#define MAIN_UART_PORT_NUM          UART_NUM_2
#define MAIN_UART_PIN_RX            4                      
#define MAIN_UART_PIN_TX            5                      
#define MAIN_UART_BAUDRATE          500000
#define MAIN_UART_QUEUE_SIZE        10


#define CAMERA_CHUNK_SIZE           1024U
#define CAMERA_TASK_STACK_SIZE      2048U
#define CAMERA_TASK_PRIO            4U
#define CAMERA_READ_MAX_LOOP_ITERS  150U
#define CAMERA_CHUNK_TIMEOUT_MS     2000U

#define CAMERA_I2C_CLK_FREQ     (100U * 1000U)
#define CAMERA_I2C_PORT         I2C_NUM_0
#define CAMERA_I2C_PIN_SDA      21U
#define CAMERA_I2C_PIN_SCL      22U
#define CAMERA_I2C_TIMEOUT_MS   200      

#define CAMERA_SPI_PIN_MISO     SPI2_IOMUX_PIN_NUM_MISO     
#define CAMERA_SPI_PIN_MOSI     SPI2_IOMUX_PIN_NUM_MOSI
#define CAMERA_SPI_PIN_SCLK     SPI2_IOMUX_PIN_NUM_CLK
#define CAMERA_SPI_PIN_CS       SPI2_IOMUX_PIN_NUM_CS
#define CAMERA_SPI_QUEUE_SIZE   5U
#define CAMERA_SPI_HOST         SPI2_HOST

#endif