#ifndef FLOATSAT_TELECOMMAND_H
#define FLOATSAT_TELECOMMAND_H

#include <stdbool.h>

#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "floatsat_types.h"
#include "floatsat_error.h"
#include "floatsat_log.h"
#include "telecommand_parser.h"

#define MSG_RX_STATE_WAIT_HEADER      0
#define MSG_RX_STATE_WAIT_MSG         1

#define CMD_MSG_SYNC                  0xCAFE

#define CMD_DMA_BUFFER_LEN            1024
#define CMD_MSG_BUFFER_LEN            CMD_DMA_BUFFER_LEN

typedef struct floatsat_msg_header_t {
    uint16_t sync;
    uint16_t len;
    uint16_t crc;
}floatsat_msg_header_t;


typedef struct cmd_manager_handle_t {
    UART_HandleTypeDef *uart;
    TaskHandle_t task_handle;
    QueueHandle_t core_cmd_queue;

    parser_ctx_t *parser_ctx;

    volatile uint8_t *dma_buffer;            // Buffer written by the UART DMA
    volatile uint8_t *msg_buffer;            // Buffer message is copied to at the end of UART RX

    volatile uint8_t state;
    volatile uint16_t xfer_len; 
    volatile uint16_t xfer_crc;
    volatile bool msg_buffer_empty;




}cmd_manager_handle_t;

floatsat_err_t CmdManager_Init(cmd_manager_handle_t *handle);


#endif