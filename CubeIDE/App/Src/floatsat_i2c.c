/*
    Wrapper for the HAL I2C with FreeRTOS mutexes for access protection
*/


#include "floatsat_i2c.h"


floatsat_err_t I2C_BusRecover(i2c_info_t *i2c)
{
    if(!i2c){
        return ERR_INVALID_ARG;
    }

    if(!i2c->gpio_port || !i2c->periph || !i2c->init_fn){
        return ERR_INVALID_ARG;
    }

    // De-init to release GPIOS
    HAL_I2C_DeInit(i2c->periph);

    // Reconfigure GPIO as open drain outputs
    GPIO_InitTypeDef gpio_config = {0};

    gpio_config.Mode    = GPIO_MODE_OUTPUT_OD;
    gpio_config.Pin     = i2c->scl_pin_mask | i2c->sda_pin_mask;
    gpio_config.Pull    = GPIO_NOPULL;
    gpio_config.Speed   = GPIO_SPEED_FREQ_LOW;

    Util_EnableGPIOClock(i2c->gpio_port);

    HAL_GPIO_Init(i2c->gpio_port, &gpio_config);

    HAL_GPIO_WritePin(i2c->gpio_port, i2c->scl_pin_mask, GPIO_PIN_SET);
    HAL_GPIO_WritePin(i2c->gpio_port, i2c->sda_pin_mask, GPIO_PIN_SET);

    for (size_t i = 0; i < 9; i++)
    {
        if(HAL_GPIO_ReadPin(i2c->gpio_port, i2c->sda_pin_mask)){
            break;
        }
        HAL_GPIO_WritePin(i2c->gpio_port, i2c->scl_pin_mask, GPIO_PIN_RESET);
        HAL_Delay(1);
        HAL_GPIO_WritePin(i2c->gpio_port, i2c->scl_pin_mask, GPIO_PIN_SET);
        HAL_Delay(1);
    }

    // Generate stop condition (SDA transitions from LOW to HIGH while SCL=HIGH)
    HAL_GPIO_WritePin(i2c->gpio_port, i2c->scl_pin_mask, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(i2c->gpio_port, i2c->sda_pin_mask, GPIO_PIN_RESET);
    HAL_Delay(1);

    HAL_GPIO_WritePin(i2c->gpio_port, i2c->scl_pin_mask, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(i2c->gpio_port, i2c->sda_pin_mask, GPIO_PIN_SET);
    HAL_Delay(1);


    Util_I2C_Reset(i2c->periph->Instance);

    // Init once again
    i2c->init_fn();
    

    return ERR_OK;
}
