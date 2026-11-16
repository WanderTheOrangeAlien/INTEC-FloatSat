#ifndef FLOATSAT_LIGHT_SENSOR_H
#define FLOATSAT_LIGHT_SENSOR_H

// ALS = Ambient 

#include "VEML770_stm32.h"
#include "TCA9548_stm32.h"

#define FLOATSAT_LIGHT_SENSOR_AMOUNT    3

#define LIGHT_SENSOR_IDX_LEFT       0
#define LIGHT_SENSOR_IDX_CENTER     1
#define LIGHT_SENSOR_IDX_RIGHT      2

typedef struct light_sensor_handle_t {
    VEML7700_handle_t   *sensors[3];
    TCA9548_handle_t    *multiplexer;
} light_sensor_handle_t;

floatsat_err_t LightSensor_Init(light_sensor_handle_t *handle);

floatsat_err_t LightSensor_ReadAll(light_sensor_handle_t *handle, float out[FLOATSAT_LIGHT_SENSOR_AMOUNT]);



#endif