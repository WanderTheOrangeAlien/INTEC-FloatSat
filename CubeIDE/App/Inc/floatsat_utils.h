#ifndef FLOATSAT_UTILS_H
#define FLOATSAT_UTILS_H

#include "stm32f4xx_hal.h"
#include "floatsat_error.h"
#include "floatsat_log.h"

void Util_IncTimeUs(void);
uint64_t Util_GetTimeUs(void);

floatsat_err_t Util_InitTimer();

int Util_UART2Index(USART_TypeDef *uart_base);

void Util_Stopwatch_Start(uint64_t *time_us);
void Util_Stopwatch_Stop(uint64_t *time_us, const char *message);


#endif