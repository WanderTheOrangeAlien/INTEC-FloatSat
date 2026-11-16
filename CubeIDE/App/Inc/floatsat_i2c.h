#ifndef FLOATSAT_I2C_H
#define FLOATSAT_I2C_H

#include "stm32f4xx_hal.h"
#include "floatsat_error.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "floatsat_utils.h"

typedef struct i2c_info_t {
    I2C_HandleTypeDef   *periph;
    GPIO_TypeDef        *gpio_port;
    uint16_t            sda_pin_mask;
    uint16_t            scl_pin_mask;
    void                (*init_fn)(void);
}i2c_info_t;

typedef struct fs_i2c_handle_t {
    I2C_HandleTypeDef   *i2c;
    SemaphoreHandle_t   mutex;    
}fs_i2c_handle_t;

floatsat_err_t I2C_BusRecover(i2c_info_t *i2c);

#endif