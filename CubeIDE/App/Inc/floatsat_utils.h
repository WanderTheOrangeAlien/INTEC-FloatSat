#ifndef FLOATSAT_UTILS_H
#define FLOATSAT_UTILS_H

#include "stm32f4xx_hal.h"
#include "floatsat_error.h"
#include "floatsat_log.h"


floatsat_err_t Util_Init(TIM_HandleTypeDef *microsec_timer);
void Util_IncTimeUsOverflow(void);
uint64_t Util_GetTimeUs(void);

// floatsat_err_t Util_InitTimer(void); // Ummm, what was this for? It has no definition

int Util_UART2Index(USART_TypeDef *uart_base);
int Util_TIM2Index(TIM_TypeDef *tim_base);
int Util_I2C2Index(I2C_TypeDef *i2c);
int Util_GPIO2Index(GPIO_TypeDef *gpio);

floatsat_err_t Util_EnableGPIOClock(GPIO_TypeDef *gpio);

void Util_Stopwatch_Start(uint64_t *time_us);
void Util_Stopwatch_Stop(uint64_t *time_us, const char *message);


#endif