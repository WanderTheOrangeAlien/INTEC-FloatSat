#ifndef FLOATSAT_I2C_H
#define FLOATSAT_I2C_H

#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "semphr.h"

typedef struct fs_i2c_handle_t {
    I2C_HandleTypeDef   *i2c;
    SemaphoreHandle_t   mutex;    
}fs_i2c_handle_t;

#endif