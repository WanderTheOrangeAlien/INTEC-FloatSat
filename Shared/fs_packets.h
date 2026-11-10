#ifndef FS_PACKETS_H
#define FS_PACKETS_H

#define DO_NOTHING() __asm volatile ("nop")

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
    };
    
}telemetry_packet_debug_t;

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