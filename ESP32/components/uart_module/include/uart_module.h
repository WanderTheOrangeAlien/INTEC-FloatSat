#ifndef UART_MODULE_H
#define UART_MODULE_H

#include "freertos/FreeRTOS.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_log.h"

#define UART_TASK_STACK_DEPTH       2048
#define UART_TASK_PRIORITY          2

/* @UART_RX_STATE */
#define UART_RX_STATE_WAIT_HEADER   0
#define UART_RX_STATE_WAIT_BODY     1

#define PACKET_TYPE_FAST            0   // Only the orientation vector
#define PACKET_TYPE_EXTENDED        1   // Orientation plus all sensors and subsystem health
#define PACKET_TYPE_INFO_HEADER     2
#define PACKET_TYPE_INFO_CHUNK      3

#define INFO_CHUNK_DATA_SIZE        32U


typedef uint64_t timestamp_t;

/* From FloatSat project */
typedef struct __attribute__((packed)) info_packet_header_t {
    uint8_t type;                // To tell apart from telemetry packets
    timestamp_t timestamp;
    uint8_t nchunks;
    uint8_t last_chunk_size;
    uint16_t crc;

}info_packet_header_t;



typedef void (*on_rx_cplt_cb_t)(uint8_t *buffer, size_t len);

typedef struct uart_module_config_t {
    uart_port_t     uart_port_num;
    size_t          rx_buff_size;
    size_t          tx_buff_size;
    uint8_t         uart_queue_size;

    uint8_t         pin_rx;
    uint8_t         pin_tx;
    uart_config_t   uart_config;
    
    on_rx_cplt_cb_t on_rx_complete;
    size_t          info_buffer_len;    // Floatsat info messages are buffered in this buffer before sending them completely
}uart_module_config_t;

typedef struct uart_module_handle_t {
    uart_module_config_t    config;

    TaskHandle_t            task_handle;
    QueueHandle_t           uart_queue;

    uint8_t                 *rx_buffer;
    size_t                  rx_idx;
    uint8_t                 rx_state; // See @UART_RX_STATE
    uint8_t                 current_packet_type;
    size_t                  current_packet_bytes;

    uint8_t                 *info_buffer;
    uint8_t                 info_chunks_to_wait;
    uint8_t                 info_chunk_idx;
    uint8_t                 last_chunk_size; 
}uart_module_handle_t;

esp_err_t UART_Init(uart_module_handle_t *handle);


#endif