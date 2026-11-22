#ifndef OV2640_INIT_SEQ_H
#define OV2640_INIT_SEQ_H

#include <stdint.h>

typedef struct sensor_reg_t {
    uint8_t     addr;
    uint8_t     val;
}sensor_reg_t;

extern const sensor_reg_t OV2640_JPEG_INIT_SEQ[];
extern const sensor_reg_t OV2640_YUV422[];
extern const sensor_reg_t OV2640_JPEG[]; 


// Resolutions
extern const sensor_reg_t OV2640_160x120_JPEG[];
extern const sensor_reg_t OV2640_176x144_JPEG[];
extern const sensor_reg_t OV2640_320x240_JPEG[];
extern const sensor_reg_t OV2640_352x288_JPEG[];
extern const sensor_reg_t OV2640_640x480_JPEG[];
extern const sensor_reg_t OV2640_800x600_JPEG[];
extern const sensor_reg_t OV2640_1024x768_JPEG[];
extern const sensor_reg_t OV2640_1280x1024_JPEG[];
extern const sensor_reg_t OV2640_1600x1200_JPEG[];

extern const sensor_reg_t *OV2640_RESOLUTIONS[];


#endif