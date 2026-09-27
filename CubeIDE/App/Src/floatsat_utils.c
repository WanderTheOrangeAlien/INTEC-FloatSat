#include "floatsat_utils.h"

#include "stm32f4xx_hal.h"

static uint64_t time_us = 0U;

static const USART_TypeDef *const UART_MAP[] = {
    USART1, USART2, USART3, UART4, UART5
};

static const TIM_TypeDef *const TIM_MAP[] = {
    TIM1, TIM2, TIM3, TIM4, TIM5, TIM6, TIM7, 
    TIM8, TIM9, TIM10, TIM11, TIM12, TIM13, TIM14
};

/// @brief Increment the microsecond time value
void Util_IncTimeUs(){
    time_us++;
}

uint64_t Util_GetTimeUs(){
    return time_us;
}


int Util_UART2Index(USART_TypeDef *uart_base)
{
    if(!uart_base){
        return -1;
    }

    for (size_t i = 0; i < sizeof(UART_MAP) / sizeof(USART_TypeDef*); i++){
        if(uart_base == UART_MAP[i]){
            return i;
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
            return i;
        }
    }

    return -1;
    
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