#include "floatsat_utils.h"

#include "stm32f4xx_hal.h"

static volatile uint32_t us_overflow_count = 0U;

static const USART_TypeDef *const UART_MAP[] = {
    USART1, USART2, USART3, UART4, UART5
};

static const TIM_TypeDef *const TIM_MAP[] = {
    TIM1, TIM2, TIM3, TIM4, TIM5, TIM6, TIM7, 
    TIM8, TIM9, TIM10, TIM11, TIM12, TIM13, TIM14
};

static const I2C_TypeDef *const I2C_MAP[] = {
    I2C1, I2C2, I2C3,
};

static const GPIO_TypeDef *const GPIO_MAP[] = {
    GPIOA, GPIOB, GPIOC, GPIOD, GPIOE,
    GPIOF, GPIOG, GPIOH, GPIOI
};

static TIM_HandleTypeDef *us_timer = NULL;

floatsat_err_t Util_Init(TIM_HandleTypeDef *microsec_timer)
{
    if(!microsec_timer){
        return ERR_INVALID_ARG;
    }
    us_timer = microsec_timer;

    HAL_TIM_Base_Start(microsec_timer);
    return ERR_OK;
}

/// @brief Increment the amount of times the us timer overflowed from 32bit
void Util_IncTimeUsOverflow(void){
    us_overflow_count++;
}

uint64_t Util_GetTimeUs(void)
{
    // Get the hardware timer value
    uint32_t hi1, hi2, lo;

    do{
        hi1 = us_overflow_count;
        lo = __HAL_TIM_GET_COUNTER(us_timer);
        hi2 = us_overflow_count; // To check if an overflow occured during read
    }while(hi1 != hi2);

    return ((uint64_t)hi1 << 32) | (uint64_t)lo;

}


int Util_UART2Index(USART_TypeDef *uart_base)
{
    if(!uart_base){
        return -1;
    }

    for (size_t i = 0; i < sizeof(UART_MAP) / sizeof(USART_TypeDef*); i++){
        if(uart_base == UART_MAP[i]){
            return (int)i;
        }
    }
    
    return -1;
    
}

int Util_TIM2Index(TIM_TypeDef *tim_base)
{
    if(!tim_base){
        return -1;
    }

    for (size_t i = 0; i < sizeof(TIM_MAP) / sizeof(TIM_TypeDef*); i++)
    {
        if(tim_base == TIM_MAP[i]){
            return (int)i;
        }
    }

    return -1;
    
}

int Util_I2C2Index(I2C_TypeDef *i2c)
{
    if(!i2c){
        return -1;
    }

    for (size_t i = 0; i < sizeof(I2C_MAP) / sizeof(I2C_MAP[0]); i++){
        if(I2C_MAP[i] == i2c){
            return (int)i;
        }
    }
    
    return -1;
}

int Util_GPIO2Index(GPIO_TypeDef *gpio)
{
    if(!gpio){
        return -1;
    }

    for (size_t i = 0; i < sizeof(GPIO_MAP) / sizeof(GPIO_MAP[0]); i++)
    {
        if(gpio == GPIO_MAP[i]){
            return (int)i;
        }
    }
    

    return -1;
}

floatsat_err_t Util_EnableGPIOClock(GPIO_TypeDef *gpio)
{
    if(!gpio){
        return ERR_INVALID_ARG;
    }

    int index = Util_GPIO2Index(gpio);
    if(index == -1){
        return ERR_INVALID_ARG;
    }

    switch (index)
    {
    case 0:
        __HAL_RCC_GPIOA_CLK_ENABLE();
        break;

    case 1:
        __HAL_RCC_GPIOB_CLK_ENABLE();
        break;

    case 2:
        __HAL_RCC_GPIOC_CLK_ENABLE();
        break;

    case 3:
        __HAL_RCC_GPIOD_CLK_ENABLE();
        break;

    case 4:
        __HAL_RCC_GPIOE_CLK_ENABLE();
        break;

    case 5:
        __HAL_RCC_GPIOF_CLK_ENABLE();
        break;

    case 6:
        __HAL_RCC_GPIOG_CLK_ENABLE();
        break;

    case 7:
        __HAL_RCC_GPIOH_CLK_ENABLE();
        break;

    case 8:
        __HAL_RCC_GPIOI_CLK_ENABLE();
        break;

    default:
        return ERR_INVALID_ARG;
    }

    return ERR_OK;
}


void Util_Stopwatch_Start(uint64_t *start_time_us)
{
    *start_time_us = Util_GetTimeUs();
}

void Util_Stopwatch_Stop(uint64_t *start_time_us, const char *message)
{
    uint64_t elapsed = Util_GetTimeUs() - *start_time_us;
    LOGI("STOPWATCH","%s. Elapsed time: %llu us", message, elapsed);
}