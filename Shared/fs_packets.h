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


#define INFO_CHUNK_DATA_SIZE        32U

typedef struct __attribute__((packed)) telemetry_packet_fast_t {
    Vec3_t orientation;
    float rw_speed;
    uint16_t end_marker;
}telemetry_packet_fast_t;



typedef struct __attribute__((packed)) telemetry_packet_slow_t {
    float temp;
    float v_batt;
    uint16_t end_marker;
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
    uint16_t end_marker;
    
}telemetry_packet_debug_t;

typedef struct __attribute__((packed)) telemetry_packet_cmd_t {
    uint8_t type;
    esp_cmd_t cmd;
    uint16_t end_marker;

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
    uint16_t end_marker;
}info_packet_header_t;

typedef struct __attribute__((packed)) info_packet_chunk_t {
    uint8_t type;
    uint8_t chunk_number;
    uint8_t data[INFO_CHUNK_DATA_SIZE]; // When transmiting, will only read INFO_CHUNK_DATA_SIZE bytes from this pointer
    uint16_t end_marker;

}info_packet_chunk_t;


#define PACKET_LEN_FAST             (offsetof(telemetry_packet_t, fast) + sizeof(telemetry_packet_fast_t))
#define PACKET_LEN_EXTENDED         (offsetof(telemetry_packet_t, slow) + sizeof(telemetry_packet_slow_t) )
#define PACKET_LEN_INFO_HEADER      (sizeof(info_packet_header_t))
#define PACKET_LEN_INFO_CHUNK       (sizeof(info_packet_chunk_t))
#define PACKET_LEN_DEBUG            (sizeof(telemetry_packet_debug_t))
#define PACKET_LEN_CMD              (sizeof(telemetry_packet_cmd_t))

#define PACKET_END_MARKER_LEN       2U
#define PACKET_END_MARKER_VAL       ((uint16_t)0xCAFE)

#define PACKET_LEN_ARR() { \
    [PACKET_TYPE_FAST]          PACKET_LEN_FAST,       \
    [PACKET_TYPE_EXTENDED]      PACKET_LEN_EXTENDED,   \
    [PACKET_TYPE_INFO_HEADER]   PACKET_LEN_INFO_HEADER,\
    [PACKET_TYPE_INFO_CHUNK]    PACKET_LEN_INFO_CHUNK, \
    [PACKET_TYPE_DEBUG]         PACKET_LEN_DEBUG,      \
    [PACKET_TYPE_CMD]           PACKET_LEN_CMD         \
    }
#endif