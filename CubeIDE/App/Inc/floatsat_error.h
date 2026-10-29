/**
  ******************************************************************************
  * @file    LSM9DS1.c
  * @author  Miguel Urena
  * @brief   Library for communication with the LSM9DS1 IMU.
  *
  ******************************************************************************
**/
#ifndef INC_FLOATSAT_ERROR_H
#define INC_FLOATSAT_ERROR_H

typedef enum floatsat_err_t {

    ERR_OK                      =   0x000,
    
    ERR_INVALID_ARG             =   0x001,
    ERR_OUT_OF_MEM              =   0x002,
    ERR_DEVICE_NOT_FOUND        =   0x003,
    ERR_DEV_I2C_FAIL            =   0x004,
    ERR_INVALID_REG             =   0x005,
    ERR_INVALID_STATE           =   0x006,
    ERR_BUFFER_OVERFLOW         =   0x007,
    
    ERR_UART_TX_FAIL            =   0x100,

    ERR_RESOURCE_BUSY           =   0x200,
    ERR_RESOURCE_NOT_AVAILABLE  =   0X201, 

    ERR_TELEM_MISSED_FAST       =   0x400,    // Fast telemetry was not added before other types


    ERR_TOO_MANY_ARGS           =   0x500,
    ERR_INVALID_CMD             =   0x501,
    ERR_CMD_NOT_FOUND           =   0x502,

    ERR_INVALID_TIME            =   0x600,

    ERR_HAL_BASE                =   0x800,
    ERR_HAL_ERROR               =   0x801,
    ERR_HAL_BUSY                =   0x802,
    ERR_HAL_TIMEOUT             =   0x803,

    ERR_CORE_MAX_PHOTOS         =   0xA00,

    ERR_NOT_IMPLEMENTED         =   0xF00,

}floatsat_err_t;


#define HAL_ERR_TO_FLOATSAT_ERR(err) ( (err == (floatsat_err_t)HAL_OK) ? ERR_OK : err + ERR_HAL_BASE)

#endif