#ifndef INC_FLOATSAT_TYPES_H
#define INC_FLOATSAT_TYPES_H

#include <stdint.h>

#include "floatsat_defs.h"
#include "test_defs.h"

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

/* ----------------- Control ----------------- */
#define CONTROL_MIN_PARAMS      3
#define CONTROL_MAX_PARAMS      4


/// Configuration parameters for the control system. These are changed by the Core in response to commands.
typedef struct control_config_t {
    float   params[CONTROL_MAX_PARAMS];
    float   target_angle;
    uint8_t control_type;

}control_config_t;


#pragma region Telemecommand

/* ========================================================================== */
/* ============================= Telemecommand ============================== */
/* ========================================================================== */


#define CMD_ID_CONTROL_INFO     0x10
#define CMD_ID_CONTROL_SET      0x11
#define CMD_ID_CONTROL_ANGLE    0x12

#define CMD_ID_MISSION_INFO     0x20
#define CMD_ID_MISSION_SET      0x21

#define CMD_ID_PHOTO_INFO       0x30
#define CMD_ID_PHOTO_ADD       0x31
#define CMD_ID_PHOTO_RESET       0x32


typedef struct floatsat_cmd_t {
    uint8_t cmd_id;
    void *params;   /* Command parameters. This is a pointer to different 
                     * types of structs. The type will depend on the cmd_id.
                     * IMPORTANT: This is dynamically allocated. The command
                     * executor must free this pointer after usage!
                     */
}floatsat_cmd_t;

//@mission_id
#define MISSION_ID_PLAYGROUND       0
#define MISSION_ID_FOLLOW_SUN       1
#define MISSION_ID_PHOTO            2


/* @control_type */
#define CONTROL_TYPE_NO_CHANGE      0
#define CONTROL_TYPE_PID            1
#define CONTROL_TYPE_LQR            2


typedef struct floatsat_args_set_control_t {
    uint8_t control_type;   // see @control_type
    float control_params[CONTROL_MAX_PARAMS];
}floatsat_args_set_control_t;

typedef struct floatsat_time_t {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
}floatsat_time_t;

typedef struct photo_info_t {
    float angle;
    floatsat_time_t time;
    uint32_t duration;

}photo_info_t;

#pragma endregion


#endif