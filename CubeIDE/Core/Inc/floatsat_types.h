#ifndef INC_FLOATSAT_TYPES_H
#define INC_FLOATSAT_TYPES_H

#include <stdint.h>

typedef struct Vec3_t {
    float x,y,z;
}Vec3_t;

typedef struct Vec3_16_t {
    int16_t x,y,z;
}Vec3_16_t;

typedef struct Quaternion_t {
    float a,b,c,d;
}Quaternion_t;

#endif