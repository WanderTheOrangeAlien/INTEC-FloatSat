#include "floatsat_motor_controller.h"

static floatsat_err_t Motor_AddTimerToRegistry(motor_handle_t *handle, TIM_HandleTypeDef *timer);

static motor_handle_t *timer_ownership_registry[FLOATSAT_TIMER_NUM] = {0};

static const char LOG_TAG[] = "MOTOR"; 


floatsat_err_t Motor_Init(motor_handle_t *handle)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }

    if(!handle->encoder_timer || !handle->pwm_timer || !handle->encoder_sampling_timer
        || handle->encoder_sampling_period_ms == 0){
        return ERR_INVALID_ARG;
    }
    // Add all times to registry
    floatsat_err_t ret = ERR_OK;
    GOTO_ON_ERR(Motor_AddTimerToRegistry(handle, handle->pwm_timer),err, ret);
    GOTO_ON_ERR(Motor_AddTimerToRegistry(handle, handle->encoder_timer),err, ret);
    GOTO_ON_ERR(Motor_AddTimerToRegistry(handle, handle->encoder_sampling_timer),err, ret);


    Motor_Stop(handle);

    return ERR_OK;

err:
    return ret;

}

floatsat_err_t Motor_SetDutyCycle(motor_handle_t *handle, float duty)
{
    if(!handle || duty > 1.0f || duty < -1.0f){
        return ERR_INVALID_ARG;
    }

    float abs = (duty >= 0.0f) ? (duty) : (-duty);
    bool is_positive = (duty >= 0.0f);
    uint32_t period = __HAL_TIM_GET_AUTORELOAD(handle->pwm_timer); 
    // TODO: The period is set in peripheral initialization and is not supposed to change. Should we set a handle property instead?

    uint16_t compare = (uint16_t)(period * abs);

    if(is_positive){
        // Right rotation
        __HAL_TIM_SET_COMPARE(handle->pwm_timer, MOTOR_PWM_CHANNEL_R, compare);
        __HAL_TIM_SET_COMPARE(handle->pwm_timer, MOTOR_PWM_CHANNEL_L, 0U);
    }else{
        // left rotation
        __HAL_TIM_SET_COMPARE(handle->pwm_timer, MOTOR_PWM_CHANNEL_L, compare);
        __HAL_TIM_SET_COMPARE(handle->pwm_timer, MOTOR_PWM_CHANNEL_R, 0U);
    }

    if(!handle->pwm_started){
        Motor_Start(handle);
    }


    return ERR_OK;
}

floatsat_err_t Motor_Start(motor_handle_t *handle)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }

    handle->pwm_started = true;
    HAL_TIM_PWM_Start(handle->pwm_timer, MOTOR_PWM_CHANNEL_R);
    HAL_TIM_PWM_Start(handle->pwm_timer, MOTOR_PWM_CHANNEL_L);

    return ERR_OK;
}


/// @brief Stop PWM generation. WARNING: Must be inside a critical section
/// @param handle 
/// @return 
floatsat_err_t Motor_Stop(motor_handle_t *handle)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }

    handle->pwm_started = false;
    __HAL_TIM_SET_COMPARE(handle->pwm_timer, MOTOR_PWM_CHANNEL_R, 0U);
    __HAL_TIM_SET_COMPARE(handle->pwm_timer, MOTOR_PWM_CHANNEL_R, 0U);
    HAL_TIM_PWM_Stop(handle->pwm_timer, MOTOR_PWM_CHANNEL_L);
    HAL_TIM_PWM_Stop(handle->pwm_timer, MOTOR_PWM_CHANNEL_R);

    return ERR_OK;
}

floatsat_err_t Motor_GetSpeed(motor_handle_t *handle, float *out_speed)
{

}

static floatsat_err_t Motor_AddTimerToRegistry(motor_handle_t *handle, TIM_HandleTypeDef *timer)
{
    int index = Util_TIM2Index(timer->Instance);
    if(index < 0){
        return ERR_INVALID_ARG;
    }

    if(timer_ownership_registry[index] == NULL){
        timer_ownership_registry[index] = handle;
    }else{
        LOGE(LOG_TAG,"Timer %d is already owned",index);
        return ERR_RESOURCE_NOT_AVAILABLE;
    }
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    int index = Util_TIM2Index(htim->Instance);
    if(index < 0){
        LOGE(LOG_TAG, "HOW?");
        return;
    }
    motor_handle_t *handle = timer_ownership_registry[index];
    if(!handle){
        LOGE(LOG_TAG,"Timer Capture callback invoked by an timer not owned by a motor handle");
        return;
    }

    if(htim != handle->pwm_timer){
        return;
    }

    // TODO: THIS IS WRONG

    // Encoder calculations
    uint32_t tick = HAL_GetTick();
    uint32_t delta_t = tick - handle->encoder_last_update_ms;
    handle->encoder_last_update_ms = tick; 

    // uint8_t dir = 

    // TODO: Do we have to divide by 4?
    handle->current_speed =  (1000.0f) / (delta_t * MOTOR_CPR); 

}

void Motor_Cb_EncoderSampler(TIM_HandleTypeDef *tim)
{
    int index = Util_TIM2Index(tim->Instance);
    if(index < 0){
        LOGE(LOG_TAG, "HOW?");
        return;
    }
    motor_handle_t *handle = timer_ownership_registry[index];
    if(!handle){
        LOGE(LOG_TAG,"Timer Capture callback invoked by an timer not owned by a motor handle");
        return;
    }

    uint16_t count =__HAL_TIM_GET_COUNTER(tim);
    // This casting handles the underflow/overflow
    int16_t delta = (int16_t)(count - handle->last_encoder_count); 
    
    handle->current_speed = delta / 

    handle->last_encoder_count = count;    

}