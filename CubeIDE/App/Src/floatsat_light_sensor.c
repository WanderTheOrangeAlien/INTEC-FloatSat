#include <string.h>

#include "floatsat_light_sensor.h"

floatsat_err_t LightSensor_Init(light_sensor_handle_t *handle)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }

    if(!handle->multiplexer 
        || !handle->sensors[0] || !handle->sensors[1] || !handle->sensors[2])
    {
        return ERR_INVALID_ARG;
    }
    floatsat_err_t ret;

    for (size_t i = 0; i < FLOATSAT_LIGHT_SENSOR_AMOUNT; i++)
    {
        GOTO_ON_ERR(VEML7700_Init(handle->sensors[i]), err, ret);
    }
    GOTO_ON_ERR(TCA9548_Init(handle->multiplexer), err, ret);
    

    return ERR_OK;
err:
    return ret;

}

floatsat_err_t LightSensor_ReadAll(light_sensor_handle_t *handle, float out[FLOATSAT_LIGHT_SENSOR_AMOUNT])
{
    if(!handle || !out){
        return ERR_INVALID_ARG;
    }
    floatsat_err_t ret;

    for (uint8_t i = 0U; i < FLOATSAT_LIGHT_SENSOR_AMOUNT; i++)
    {
        GOTO_ON_ERR(TCA9548_SelectChannel(handle->multiplexer, i), err, ret);
        HAL_Delay(1);
        GOTO_ON_ERR(VEML7700_ReadLight(handle->sensors[i], &out[i]), err, ret);
    }
    

    return ERR_OK;
err:
    TCA9548_SelectChannel(handle->multiplexer, 0);
    memset(out, 0, FLOATSAT_LIGHT_SENSOR_AMOUNT * sizeof(float));
    return ret;
}
