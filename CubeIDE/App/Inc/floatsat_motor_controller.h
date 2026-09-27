#ifndef FLOATSAT_MOTOR_CONTROLLER_H
#define FLOATSAT_MOTOR_CONTROLLER_H

#include "stm32f4xx_hal.h"

#include "floatsat_error.h"
#include "floatsat_types.h"
#include "floatsat_utils.h"

#define MOTOR_PWM_CHANNEL_R     TIM_CHANNEL_1
#define MOTOR_PWM_CHANNEL_L     TIM_CHANNEL_2

#define MOTOR_CPR               64U // Motor counts per revolution

typedef struct motor_handle_t {
    TIM_HandleTypeDef   *pwm_timer;
    TIM_HandleTypeDef   *encoder_timer;
    TIM_HandleTypeDef   *encoder_sampling_timer; 
    bool                pwm_started;

    volatile float      current_speed;  // Updated by the encoder interrupt 
    volatile uint16_t   last_encoder_count;

    uint32_t            encoder_sampling_period_ms;

}motor_handle_t;

floatsat_err_t Motor_Init(motor_handle_t *handle);


floatsat_err_t Motor_SetDutyCycle(motor_handle_t *handle, float duty);
floatsat_err_t Motor_Stop(motor_handle_t *handle);
floatsat_err_t Motor_Start(motor_handle_t *handle);

floatsat_err_t Motor_GetSpeed(motor_handle_t *handle, float *out_speed);

void Motor_Cb_EncoderSampler(TIM_HandleTypeDef *tim);

#endif