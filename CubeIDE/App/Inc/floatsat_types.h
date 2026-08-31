#ifndef INC_FLOATSAT_TYPES_H
#define INC_FLOATSAT_TYPES_H

#include <stdint.h>

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

#endif