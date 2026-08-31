#ifndef INC_FLOATSAT_INIT_H
#define INC_FLOATSAT_INIT_H

#include "stm32f4xx_hal.h"

#include "floatsat_error.h"

#include "LSM9DS1_stm32.h"
#include "madgwick_filter.h"


// Structure to enclose all peripheral handles. This is required to cleanly
// pass all the handles from main.c to floatsat_init.c
typedef struct floatsat_periph_t {
    I2C_HandleTypeDef       *imu_i2c;

}floatsat_periph_t;


floatsat_err_t FloatSat_Init(floatsat_periph_t *peripherals);


#endif
