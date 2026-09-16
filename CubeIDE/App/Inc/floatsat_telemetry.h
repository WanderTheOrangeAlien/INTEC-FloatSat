#ifndef FLOATSAT_TELEMETRY_H
#define FLOATSAT_TELEMETRY_H

#include <stdbool.h>

#include "stm32f4xx_hal.h"
#include "floatsat_error.h"
#include "floatsat_types.h"


#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

#define TELEMETRY_QUEUE_SIZE        8U
#define TELEMETRY_UART_TIMEOUT_MS   2U

#define TELEMETRY_INFO_BUFFER_SIZE    1024U

#define PACKET_TYPE_FAST            0   // Only the orientation vector
#define PACKET_TYPE_EXTENDED        1   // Orientation plus all sensors and subsystem health
#define PACKET_TYPE_INFO_HEADER     2
#define PACKET_TYPE_INFO_CHUNK      3

#define PACKET_MASK_FAST            (1U << 0U)
#define PACKET_MASK_SLOW            (1U << 1U)
#define PACKET_MASK_INFO            (1U << 2U)

#define INFO_STATE_START            0U /* New info has been written. Construct the header and divide in chunks */
#define INFO_STATE_TX               1U /* Transmitting. The info buffer is read and chunks are constructed and sent */

#define INFO_CHUNK_DATA_SIZE        32U
#define INFO_CHUNK_SIZE             (offsetof(info_packet_chunk_t, data) + INFO_CHUNK_DATA_SIZE)
#define INFO_CHUNK_TIME_MARGIN_US   100U        // Microseconds of margin to leave before task switch is performed
#define INFO_CHUNK_RX_TIME_US       (80U*2U)    // Estimated time needed for sending a chunk.   


typedef struct telemetry_packet_fast_t {
    Vec3_t orientation;
    float rw_speed;
}telemetry_packet_fast_t;

typedef struct telemetry_packet_slow_t {
    float temp;
    float v_batt;
    // TODO: Add Subsystem health indicators

}telemetry_packet_slow_t;

typedef struct __attribute__((packed)) telemetry_packet_t {
    uint8_t type;                 // Thsi is written by the telemetry task based on the packet type flags
    timestamp_t timestamp;
      
    telemetry_packet_fast_t fast; // Orientaiton and reaction wheel speed
    
    telemetry_packet_slow_t slow; // Other Onboard sensors and health indicators 

}telemetry_packet_t;

typedef struct info_packet_header_t {
    uint8_t type;                // To tell apart from telemetry packets
    timestamp_t timestamp;
    uint8_t nchunks;
    uint8_t last_chunk_size;
    uint16_t crc;

}info_packet_header_t;

typedef struct info_packet_chunk_t {
    uint8_t type;
    uint8_t chunk_number;
    uint8_t *data; // When transmiting, will only read INFO_CHUNK_DATA_SIZE bytes from this pointer

}info_packet_chunk_t;

typedef struct telemetry_handle_t {
    UART_HandleTypeDef *uart;
    
    QueueHandle_t queue;
    SemaphoreHandle_t info_semphr;      // Mutex for the infor buffer

    TaskHandle_t task_handle;

    volatile uint8_t packet_type_mask;    // Bit mask indicating which types of packets must be sent.
    volatile telemetry_packet_t *current_packet;
    volatile bool tx_busy;

    uint8_t info_state;                 // State for the info transmission state machine
    uint8_t nchunks;
    uint8_t last_chunk_size;
    uint8_t chunk_index;
    // uint32_t info_buffer_index;

    uint64_t task_start_time_us;
    uint64_t task_end_time_us;

    uint8_t *info_bufer;                // Buffer acquired with `Telemetry_TakeInfoBuffer`
    volatile bool info_buffer_not_empty;

}telemetry_handle_t;


floatsat_err_t Telemetry_Init(telemetry_handle_t *handle);

floatsat_err_t Telemetry_AddFast(telemetry_handle_t *handle, telemetry_packet_fast_t *packet);
floatsat_err_t Telemetry_AddSlow(telemetry_handle_t *handle, telemetry_packet_slow_t *packet);
// floatsat_err_t Telemetry_AddInfo(telemetry_handle_t *handle, )

floatsat_err_t Telemetry_GiveInfoBuffer(telemetry_handle_t *handle, uint8_t **ptr);
floatsat_err_t Telemetry_TakeInfoBuffer(telemetry_handle_t *handle, uint8_t **out_ptr);


#endif