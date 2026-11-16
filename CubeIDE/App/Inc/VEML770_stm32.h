#ifndef VEML770_STM32_H
#define VEML770_STM32_H

/*
    Some notes on the sensor, as per the application note here https://www.vishay.com/docs/84323/designingveml7700.pdf

    - The integration time has a 30% tolerance. This should be accounted for in the firmware
    - Gain factors x1 and x2 should not be used above 100 lux because the sensor becomes non-linear
    - Before any register configuration, always enable shutdown (Bit 0 of ALS_CONF_0)

*/



#include "floatsat_types.h"
#include "floatsat_error.h"
#include "floatsat_err_log.h"

#include "floatsat_i2c.h"

#define MASK_2BITS  0x03U
#define MASK_3BITS  0x07U
#define MASK_4BITS  0x0FU
#define MASK_8BITS  0xFFU

#define VEML7700_ADDRESS                0x10
#define VEML7700_TIMEOUT_MS             100U
#define VEML7700_INIT_ATTEMPTS          5U

// ALS means Ambient Light Sensor

#define VEML7700_REG_ALS_CONF_0         0x00 // Configuration register
#define VEML7700_REG_ALS_WH             0x01 // ALS High Window Thereshold
#define VEML7700_REG_ALS_WL             0x02 // ALS Low Window Thereshold
#define VEML7700_REG_ALS_PS             0x03 // Power saving
#define VEML7700_REG_ALS                0x04 // ALS measurement
#define VEML7700_REG_WHITE              0x05 // White light sensor measurement
#define VEML7700_REG_ALS_INT            0x06 // ALS INT trigger even
#define VEML7700_REG_ID                 0x07 // Device ID

/* ************************************************************************** */
/* ***************************** Register fields **************************** */
/* ************************************************************************** */

/* =============================== ALS_CONF_0 =============================== */

// Sensor Gain
#define VEML7700_MASK_ALS_GAIN          (MASK_2BITS << 11) 
#define VEML7700_ALS_GAIN_X1            (0x00 << 11)    
#define VEML7700_ALS_GAIN_X2            (0x01 << 11)    
#define VEML7700_ALS_GAIN_X1_8          (0x02 << 11) // x1/8    
#define VEML7700_ALS_GAIN_X1_4          (0x03 << 11) // x1/4

// Integration time
#define VEML7700_MASK_ALS_IT            (MASK_4BITS << 6)
#define VEML7700_ALS_IT_100MS           (0x00 << 6)
#define VEML7700_ALS_IT_200MS           (0x01 << 6)
#define VEML7700_ALS_IT_400MS           (0x02 << 6)
#define VEML7700_ALS_IT_800MS           (0x03 << 6)
#define VEML7700_ALS_IT_50MS            (0x08 << 6)
#define VEML7700_ALS_IT_25MS            (0x0C << 6)

// Persistence protect number setting
#define VEML7700_MASK_ALS_PERS          (MASK_2BITS << 4)
#define VEML7700_ALS_PERS_1             (0x00 << 4)
#define VEML7700_ALS_PERS_2             (0x01 << 4)
#define VEML7700_ALS_PERS_4             (0x02 << 4)
#define VEML7700_ALS_PERS_8             (0x03 << 4)

// Interrupt enable
#define VEML7700_ALS_INT_EN             (0x01 << 1)

// ALS Shutdown
#define VEML7700_ALS_SHUTDOWN           (0x01 << 0)

/* =============================== ALS_PS =============================== */
// Power saving mode
#define VEML7700_MASK_PSM               (MASK_2BITS << 1)
#define VEML7700_PSM_1                  (0x00 << 1)
#define VEML7700_PSM_2                  (0x01 << 1)
#define VEML7700_PSM_3                  (0x02 << 1)
#define VEML7700_PSM_4                  (0x03 << 1)

// Power saving enable
#define VEML7700_PSM_EN                 (0x01 << 0)

/* =============================== ALS_INT =============================== */
#define VEML7700_INT_TH_LOW             (0x01 << 15)
#define VEML7700_INT_TH_HIGH            (0x01 << 14)

/* =============================== ID =============================== */
#define VEML7700_MASK_SLAVE_ADDR        (MASK_8BITS << 8)



#define VEML7700_MASK_DEV_ID            (MASK_8BITS << 0)



typedef struct VEML7700_handle_t {
    i2c_info_t          i2c;

    uint16_t            gain;
    uint16_t            int_time; // Integration time
    uint16_t            persistence;

}VEML7700_handle_t;


floatsat_err_t VEML7700_Init(VEML7700_handle_t *handle);
floatsat_err_t VEML7700_ReadLight(VEML7700_handle_t *handle, float *value);

floatsat_err_t VEML7700_TurnOn(VEML7700_handle_t *handle);
floatsat_err_t VEML7700_TurnOff(VEML7700_handle_t *handle);
floatsat_err_t VEML7700_SetGain(VEML7700_handle_t *handle, uint16_t gain);
floatsat_err_t VEML7700_IntTime(VEML7700_handle_t *handle, uint16_t int_time);




#endif