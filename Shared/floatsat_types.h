#ifndef INC_FLOATSAT_TYPES_H
#define INC_FLOATSAT_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "floatsat_defs.h"
#include "fs_commands.h"



typedef struct Vec3_t {
    float x,y,z;
}Vec3_t;

typedef struct Vec3_16_t {
    int16_t x,y,z;
}Vec3_16_t;

typedef struct Quaternion_t {
    float a,b,c,d;
}Quaternion_t;

typedef struct IMU_data_raw_t {
    Vec3_16_t accel;
    Vec3_16_t gyro;
    Vec3_16_t mag;
    uint8_t is_valid;
}IMU_data_raw_t;

typedef struct IMU_data_t {
    Vec3_t accel;
    Vec3_t gyro;
    Vec3_t mag;
}IMU_data_t;

typedef uint64_t timestamp_t;

#define IMU_MAG_OFFSET  ((Vec3_t){-0.0218f * 1e3f, 0.4711f * 1e3f, 0.0f})
#define IMU_MAG_SCALE   ((Vec3_t){0.98395f, 1.01290f, 0.0f})

/* ----------------- Control ----------------- */
#define CONTROL_MIN_PARAMS      3
#define CONTROL_MAX_PARAMS      4


/// Configuration parameters for the control system. These are changed by the Core in response to commands.
typedef struct control_cascade_params_t {
    float k_theta;
    float k_p;
    float k_i;
}control_cascade_params_t;

typedef struct __attribute__((packed)) control_data_t {
    float       angle;
    float       angle_ref;
    float       w;
    float       w_ref;
    float       pwm_out;
    float       rw_speed;
}control_data_t;

typedef struct control_config_t {
    float                       target_angle;
    control_cascade_params_t    params;
    uint8_t                     control_type;
    bool                        control_active;
    uint8_t                     debug_id;
}control_config_t;



typedef struct floatsat_args_set_control_t {
    uint8_t control_type;   // see @control_type
    float control_params[CONTROL_MAX_PARAMS];
}floatsat_args_set_control_t;

#define CMD_MSG_SYNC                  0xCAFE

typedef struct floatsat_msg_header_t {
    uint16_t sync;
    uint16_t len;
    uint16_t crc;
}floatsat_msg_header_t;



#endif