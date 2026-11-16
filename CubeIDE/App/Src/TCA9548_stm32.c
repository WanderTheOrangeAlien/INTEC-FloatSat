#include "TCA9548_stm32.h"


floatsat_err_t TCA9548_Init(TCA9548_handle_t *handle)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }

    if( !handle->i2c.gpio_port || !handle->i2c.periph || !handle->i2c.init_fn 
        || !(handle->dev_address >= 0x70 && handle->dev_address <= 0x77) ){
        return ERR_INVALID_ARG;
    }

    floatsat_err_t ret;
    for (size_t i = 0; i < TCA9548_INIT_ATTEMPTS; i++)
    {
        ret = TCA9548_SelectChannel(handle, 0U);
        if(ret == ERR_OK){
            break;
        }else{
            I2C_BusRecover(&handle->i2c);
        }
    }

    if(ret != ERR_OK){
        return ERR_DEV_I2C_FAIL;
    }

    return ERR_OK;
}

floatsat_err_t TCA9548_SelectChannel(TCA9548_handle_t *handle, uint8_t channel)
{
    if(!handle || channel >= 8U){
        return ERR_INVALID_ARG;
    }

    handle->current_channel = channel;
    uint8_t dummy = 0x00;

    HAL_StatusTypeDef status;
    status = HAL_I2C_Mem_Write( handle->i2c.periph, handle->dev_address << 1, 
                                (1 << channel), 1U, 
                                &dummy, 0U, 
                                TCA9548_TIMEOUT_MS);

    if(status != HAL_OK){
        return ERR_DEV_I2C_FAIL;
    }

    return ERR_OK;
}
