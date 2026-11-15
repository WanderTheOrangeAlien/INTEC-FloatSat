#ifndef FS_PACKETS_H
#define FS_PACKETS_H

#include "external_cmd.h"
#include "floatsat_types.h"

#define DO_NOTHING() __asm volatile ("nop")

#define PACKET_TYPE_FAST            0x00   // Only the orientation vector
#define PACKET_TYPE_EXTENDED        0x01   // Orientation plus all sensors and subsystem health
#define PACKET_TYPE_INFO_HEADER     0x02
#define PACKET_TYPE_INFO_CHUNK      0x03
#define PACKET_TYPE_DEBUG           0x04
#define PACKET_TYPE_CMD             0x05


#define DEBUG_PACKET_ID_NONE        0x00
#define DEBUG_PACKET_ID_IMU         0x01
#define DEBUG_PACKET_ID_CONTROL     0x02
#define DEBUG_PACKET_ID_RTC         0x03


typedef struct __attribute__((packed)) telemetry_packet_fast_t {
    Vec3_t orientation;
    float rw_speed;
}telemetry_packet_fast_t;



typedef struct __attribute__((packed)) telemetry_packet_slow_t {
    float temp;
    float v_batt;
    // TODO: Add Subsystem health indicators

}telemetry_packet_slow_t;

typedef struct __attribute__((packed)) telemetry_packet_debug_t {
    uint8_t     type;
    uint8_t     debug_id;
    timestamp_t timestamp;
    union {
        IMU_data_t      imu_data;
        control_data_t  control_data;
        floatsat_time_t rtc_time;
    };
    
}telemetry_packet_debug_t;

typedef struct __attribute__((packed)) telemetry_packet_cmd_t {
    uint8_t type;
    esp_cmd_t cmd;
}telemetry_packet_cmd_t;

typedef struct __attribute__((packed)) telemetry_packet_t {
    uint8_t type;                 // Thsi is written by the telemetry task based on the packet type flags
    timestamp_t timestamp;
      
    telemetry_packet_fast_t fast; // Orientaiton and reaction wheel speed
    
    telemetry_packet_slow_t slow; // Other Onboard sensors and health indicators
}telemetry_packet_t;

typedef struct __attribute__((packed)) info_packet_header_t {
    uint8_t type;                // To tell apart from telemetry packets
    timestamp_t timestamp;
    uint8_t nchunks;
    uint8_t last_chunk_size;
    uint16_t crc;
}info_packet_header_t;

typedef struct __attribute__((packed)) info_packet_chunk_t {
    uint8_t type;
    uint8_t chunk_number;
    uint8_t *data; // When transmiting, will only read INFO_CHUNK_DATA_SIZE bytes from this pointer

}info_packet_chunk_t;


#endif