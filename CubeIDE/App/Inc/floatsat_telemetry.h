#ifndef FLOATSAT_TELEMETRY_H
#define FLOATSAT_TELEMETRY_H

#include <stdbool.h>

#include "stm32f4xx_hal.h"
#include "floatsat_err_log.h"
#include "floatsat_types.h"

#include "fs_packets.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

#define TELEMETRY_QUEUE_SIZE        8U
#define TELEMETRY_UART_TIMEOUT_MS   2U

#define TELEMETRY_INFO_BUFFER_SIZE    1024U

#define TELEMETRY_RUNTIME_MS       5U

#define PACKET_TYPE_FAST            0   // Only the orientation vector
#define PACKET_TYPE_EXTENDED        1   // Orientation plus all sensors and subsystem health
#define PACKET_TYPE_INFO_HEADER     2
#define PACKET_TYPE_INFO_CHUNK      3
#define PACKET_TYPE_DEBUG           4


#define PACKET_MASK_FAST            (1U << 0U)
#define PACKET_MASK_SLOW            (1U << 1U)
#define PACKET_MASK_INFO            (1U << 2U)
#define PACKET_MASK_DEBUG           (1U << 3U)

#define INFO_STATE_START            0U /* New info has been written. Construct the header and divide in chunks */
#define INFO_STATE_TX               1U /* Transmitting. The info buffer is read and chunks are constructed and sent */

#define INFO_CHUNK_DATA_SIZE        32U
#define INFO_CHUNK_SIZE             (offsetof(info_packet_chunk_t, data) + INFO_CHUNK_DATA_SIZE)
#define INFO_CHUNK_TIME_MARGIN_US   100U        // Microseconds of margin to leave before task switch is performed
#define INFO_CHUNK_RX_TIME_US       (680U)    // Estimated time needed for sending a chunk.   

#define DEBUG_PACKET_ID_IMU         0x00
#define DEBUG_PACKET_ID_CONTROL     0x01

typedef struct telemetry_handle_t {
    UART_HandleTypeDef *uart;
    
    QueueHandle_t queue;
    SemaphoreHandle_t info_semphr;      // Mutex for the infor buffer

    TaskHandle_t task_handle;

    volatile uint8_t packet_type_mask;    // Bit mask indicating which types of packets must be sent.
    telemetry_packet_t current_packet;
    telemetry_packet_debug_t debug_packet;
    volatile bool tx_busy;

    uint8_t info_state;                 // State for the info transmission state machine
    uint8_t nchunks;
    uint8_t last_chunk_size;
    uint8_t chunk_index;
    // uint32_t info_buffer_index;

    uint64_t task_start_time_us;
    uint64_t current_deadline_us;       // The telemetry task has a fixed runtime of TELEMETRY_RUNTIME_MS milliseconds

    uint8_t *info_buffer;                // Buffer acquired with `Telemetry_TakeInfoBuffer`
    volatile bool info_buffer_not_empty;

}telemetry_handle_t;


void Task_TelemetryFn(void *args);

floatsat_err_t Telemetry_Init(telemetry_handle_t *handle);

floatsat_err_t Telemetry_AddFast(telemetry_handle_t *handle, telemetry_packet_fast_t *packet);
floatsat_err_t Telemetry_AddSlow(telemetry_handle_t *handle, telemetry_packet_slow_t *packet);
floatsat_err_t Telemetry_AddDebug(telemetry_handle_t *handle, telemetry_packet_debug_t *packet);

floatsat_err_t Telemetry_GiveInfoBuffer(telemetry_handle_t *handle, uint8_t **ptr);
floatsat_err_t Telemetry_TakeInfoBuffer(telemetry_handle_t *handle, uint8_t **out_ptr);


#endif