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

#include "floatsat_log.h"

typedef enum floatsat_err_t {

    ERR_OK                  =   0x000,
    
    ERR_INVALID_ARG         =   0x001,
    ERR_OUT_OF_MEM          =   0x002,
    ERR_DEVICE_NOT_FOUND    =   0x003,
    ERR_DEV_I2C_FAIL        =   0x004,
    ERR_INVALID_REG         =   0x005,
    ERR_INVALID_STATE       =   0x006,
    
    ERR_UART_TX_FAIL        =   0x100,

    ERR_TOO_MANY_ARGS       =   0x500,
    ERR_INVALID_CMD         =   0x501

}floatsat_err_t;


#define GOTO_ON_ERR(x,goto_label,ret_val) do{ \
  ret_val = (x);                              \
  if(ret_val != ERR_OK){                      \
    goto goto_label;                          \
  }                                           \
}while(0)

#define GOTO_ON_ERR_LOG(x, goto_label, ret_val, tag, format, ...) do{ \
  ret_val = (x);                              \
  if(ret_val != ERR_OK){                      \
    LOGE(tag, format, ##__VA_ARGS__);         \
    goto goto_label;                          \
  }                                           \
}while(0)

#endif