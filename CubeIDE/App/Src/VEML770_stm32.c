#include "VEML770_stm32.h"


static floatsat_err_t VEML7700_InitRegs(VEML7700_handle_t *handle);

static floatsat_err_t VEML7700_WriteField(VEML7700_handle_t *handle, uint8_t reg, 
                                          uint16_t field_mask, uint16_t field_val);
static floatsat_err_t VEML7700_WriteReg(VEML7700_handle_t *handle, uint8_t reg, uint16_t val);
static floatsat_err_t VEML7700_ReadReg(VEML7700_handle_t *handle, uint8_t reg, uint16_t *val);
static floatsat_err_t VEML7700_ValidateParams(VEML7700_handle_t *handle);

static float apply_sensor_correction(float raw_val_lux);
static floatsat_err_t reg_value_to_lux(VEML7700_handle_t *handle, uint16_t reg_val, float *out);

static const float VEML7700_CONV_FACTOR_TABLE[6][4] = {
//              GAIN_1      GAIN_2      GAIN_1_8    GAIN_1_4
/* 100ms */    {0.0672f,    0.0336f,    0.5376f,    0.2688f},
/* 200ms */    {0.0336f,    0.0168f,    0.2688f,    0.1344f},
/* 400ms */    {0.0168f,    0.0084f,    0.1344f,    0.0672f},
/* 800ms */    {0.0084f,    0.0042f,    0.0672f,    0.0336f},
/* 50ms */     {0.1344f,    0.0672f,    1.0752f,    0.5376f},
/* 25ms */     {0.2688f,    0.1344f,    2.1504f,    1.0752f},
};



floatsat_err_t VEML7700_Init(VEML7700_handle_t *handle)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }

    if(!handle->i2c.periph || !handle->i2c.gpio_port){
        return ERR_INVALID_ARG;
    }
    
    if(VEML7700_ValidateParams(handle) != ERR_OK){
        VEML7700_TurnOff(handle);
        return ERR_INVALID_ARG;
    }

    floatsat_err_t ret;
    for (size_t i = 0; i < VEML7700_INIT_ATTEMPTS; i++)
    {
        ret = VEML7700_InitRegs(handle);
        if(ret == ERR_OK){
            break;
        }else{
            I2C_BusRecover(&handle->i2c);
        }
    }
    if(ret != ERR_OK){
        VEML7700_TurnOff(handle);
        return ERR_DEV_I2C_FAIL;
    }
    
    return ERR_OK;
}

static floatsat_err_t VEML7700_InitRegs(VEML7700_handle_t *handle)
{
    uint16_t reg_val;
    floatsat_err_t ret;
    GOTO_ON_ERR(VEML7700_ReadReg(handle, VEML7700_REG_ALS_CONF_0, &reg_val),err, ret);
    
    // Turn off
    GOTO_ON_ERR(VEML7700_WriteReg(handle, VEML7700_REG_ALS_CONF_0, reg_val | 0x01), err, ret);

    reg_val &= (uint16_t)~(VEML7700_MASK_ALS_GAIN | VEML7700_MASK_ALS_IT | VEML7700_MASK_ALS_PERS);
    reg_val |= handle->gain | handle->int_time | handle->persistence;
    reg_val &= (uint16_t)~0x01; // Reset SD bit to turn on the sensor

    // Write and turn on
    GOTO_ON_ERR(VEML7700_WriteReg(handle, VEML7700_REG_ALS_CONF_0, reg_val), err, ret);
    return ERR_OK;
err:
    return ret;

}

floatsat_err_t VEML7700_ReadLight(VEML7700_handle_t *handle, float *value)
{
    if(!handle || !value){
        return ERR_INVALID_ARG;
    }

    uint16_t reg_val;
    floatsat_err_t ret = VEML7700_ReadReg(handle, VEML7700_REG_ALS, &reg_val);
    if(ret != ERR_OK) {
        return ret;
    }

    float lux = 0U;
    ret = reg_value_to_lux(handle, reg_val, &lux);
    if(ret != ERR_OK) {
        return ret;
    }

    *value = apply_sensor_correction(lux);

    return ERR_OK;
}

floatsat_err_t VEML7700_SetGain(VEML7700_handle_t *handle, uint16_t gain)
{
    floatsat_err_t ret;
    GOTO_ON_ERR(VEML7700_TurnOff(handle), err, ret);
    GOTO_ON_ERR(VEML7700_WriteField(handle, VEML7700_REG_ALS_CONF_0, VEML7700_MASK_ALS_GAIN, gain), err, ret);
    handle->gain = gain;
    GOTO_ON_ERR(VEML7700_TurnOn(handle), err, ret);
err:
    return ret;
}

floatsat_err_t VEML7700_IntTime(VEML7700_handle_t *handle, uint16_t int_time)
{
    floatsat_err_t ret;
    GOTO_ON_ERR(VEML7700_TurnOff(handle), err, ret);
    GOTO_ON_ERR(VEML7700_WriteField(handle, VEML7700_REG_ALS_CONF_0, VEML7700_MASK_ALS_IT, int_time), err, ret);
    handle->int_time = int_time;
    GOTO_ON_ERR(VEML7700_TurnOn(handle), err, ret);
err:
    return ret;
}

floatsat_err_t VEML7700_TurnOn(VEML7700_handle_t *handle)
{
    return VEML7700_WriteField(handle, VEML7700_REG_ALS_CONF_0, (1U << 0), 0x00);
}

floatsat_err_t VEML7700_TurnOff(VEML7700_handle_t *handle)
{
    return VEML7700_WriteField(handle, VEML7700_REG_ALS_CONF_0, (1U << 0), 0x01);
}

static floatsat_err_t VEML7700_WriteField(VEML7700_handle_t *handle, uint8_t reg, 
                                          uint16_t field_mask, uint16_t field_val)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }

    // The field value has bits outisde of the provided mask
    if((field_val & (uint16_t)~field_mask) != 0U){
        return ERR_INVALID_ARG;
    }

    uint16_t reg_val;
    floatsat_err_t ret = VEML7700_ReadReg(handle, reg, &reg_val);
    if(ret != ERR_OK){
        return ret;
    }

    reg_val &= ~field_mask;
    reg_val |= field_val;

    return VEML7700_WriteReg(handle, reg, reg_val);

}

static floatsat_err_t VEML7700_WriteReg(VEML7700_handle_t *handle, uint8_t reg, uint16_t val)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }
    HAL_StatusTypeDef status;

    status = HAL_I2C_Mem_Write( handle->i2c.periph, VEML7700_ADDRESS << 1, 
                                reg, 1U, 
                                (uint8_t*)&val, 2U,
                                VEML7700_TIMEOUT_MS);
    if(status != HAL_OK){
        return ERR_DEV_I2C_FAIL;
    }


    return ERR_OK;
}

static floatsat_err_t VEML7700_ReadReg(VEML7700_handle_t *handle, uint8_t reg, uint16_t *val)
{
    if(!handle || !val){
        return ERR_INVALID_ARG;
    }
    HAL_StatusTypeDef status;
    status = HAL_I2C_Mem_Read(  handle->i2c.periph, VEML7700_ADDRESS << 1,
                                reg, 1U,
                                (uint8_t*)val, 2U,
                                VEML7700_TIMEOUT_MS);
    if(status != HAL_OK){
        return ERR_DEV_I2C_FAIL;
    }
    return ERR_OK;
}

/// @brief Compute the result of the correction polynomial as described in page 5 of the application note
/// @param raw_val Sensor value in lux (after multiplying by the corresponding conversion factor)
/// @return The corrected lux value
static float apply_sensor_correction(float raw_val_lux)
{
    static const float coeffs[4] = {6.0135e-13f, -9.3924e-9f, 8.1488e-5f, 1.0023f};

    float x = (float)raw_val_lux;
    
     return   (x*x*x*x) * coeffs[0]
            + (x*x*x)   * coeffs[1]
            + (x*x)     * coeffs[2]
            + (x)       * coeffs[3];
    
}


static floatsat_err_t VEML7700_ValidateParams(VEML7700_handle_t *handle)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }

    uint16_t param; // Field value without the shifting
    
    // Check gian
    param = handle->gain >> 11;
    if(param > 0x03){
        return ERR_INVALID_ARG;
    }

    //Check integration time
    param = handle->int_time >> 6;
    if(param > 0x03 && param != 0x08 && param != 0x0C){
        return ERR_INVALID_ARG;
    }

    // Check persistence
    param = (uint16_t)~VEML7700_MASK_ALS_PERS >> 4;
    if(param > 0x03){
        return ERR_INVALID_ARG;
    }

    return ERR_OK;
}

static floatsat_err_t reg_value_to_lux(VEML7700_handle_t *handle, uint16_t reg_val, float *out)
{
    if(!handle || !out){
        return ERR_INVALID_ARG;
    }

    floatsat_err_t ret = VEML7700_ValidateParams(handle);
    if(ret != ERR_OK){
        return ret;
    }

    uint16_t gain       = handle->gain >> 11;
    uint16_t int_time   = handle->int_time >> 6;

    int int_idx = (int_time <= 0x03) ? handle->int_time :
                     (int_time == 0x08) ? 0x04 :
                     (int_time == 0x0C) ? 0x05 : -1;
                     
    if(int_idx == -1){
        // This should not happen becuase we already validate the parameters at 
        /// the start, but I'll leave it here to be extra safe
        return ERR_INVALID_ARG;
    }

    float conv_factor = VEML7700_CONV_FACTOR_TABLE[int_idx][gain];

    *out = (float)reg_val * conv_factor;
    return ERR_OK;
}