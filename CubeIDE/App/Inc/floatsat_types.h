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

#define CMD_ID_SET_CONTROL      0x10

#define CMD_ID_INFO_CONTROL     0x50
#define CMD_ID_INFO_MISSION     0x51
#define CMD_ID_INFO_PHOTO       0x52


typedef struct floatsat_cmd_t {
    uint8_t cmd_id;
    void *params;   /* Command parameters. This is a pointer to different 
                     * types of structs. The type will depend on the cmd_id.
                     * IMPORTANT: This is dynamically allocated. The command
                     * executor must free this pointer after usage!
                     */
}floatsat_cmd_t;

/* @control_type */
#define CONTROL_TYPE_NO_CHANGE      0
#define CONTROL_TYPE_PID            1
#define CONTROL_TYPE_LQR            2

#define MIN_CONTROL_PARAMS      3
#define MAX_CONTROL_PARAMS      4
typedef struct floatsat_args_set_control_t {
    uint8_t control_type;   // see @control_type
    double control_params[MAX_CONTROL_PARAMS];
}floatsat_args_set_control_t;


#endif