#ifndef FLOATSAT_TELEMETRY_H
#define FLOATSAT_TELEMETRY_H

#include "stm32f4xx_hal.h"
#include "floatsat_error.h"
#include "floatsat_types.h"

#include "FreeRTOS.h"
#include "queue.h"

#define TELEMETRY_QUEUE_SIZE        8
#define TELEMETRY_UART_TIMEOUT_MS   2

#define PACKET_TYPE_ORIENTATION     0   // Only the orientation vector
#define PACKET_TYPE_EXTENDED        1   // Orientation plus all sensors and subsystem health

typedef struct telemetry_handle_t {
    UART_HandleTypeDef *uart;
    QueueHandle_t queue;

    TaskHandle_t task_handle;

    Vec3_t  *g_orientation;     // Orientation provided by the control task
                                // Must ensure atomic access

}telemetry_handle_t;


typedef struct __attribute__((packed)) telemetry_packet_t {
    uint8_t type;
    timestamp_t timestamp;
    Vec3_t orientation;
    float temp;
    float v_batt;
    // TODO: Add Subsystem health indicators
}telemetry_packet_t;

floatsat_err_t Telemetry_Init(telemetry_handle_t *handle);


#endif