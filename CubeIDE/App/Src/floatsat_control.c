#include "floatsat_control.h"
#include <string.h>

static floatsat_err_t Control_ComputeOutput(
    const control_handle_t *handle,
    const IMU_data_t *imu_data, 
    float *out, 
    control_data_t *debug_data_out);

static floatsat_err_t Control_EstimateOrientation(control_handle_t *handle, IMU_data_t *imu_data);
static floatsat_err_t Control_SendDebug(control_handle_t *handle, IMU_data_t *imu_data, control_data_t *control_data);
static void handle_rw_saturation(control_handle_t *handle, const IMU_data_t *imu_data);
static void angular_vel_failsafe(control_handle_t *handle, const IMU_data_t *imu_data);
static bool is_stationary(float w, float threshold);


static const Vec3_t forward_vec = {1,0,0}; // The north corresponds to the X axis. 

static const control_cascade_params_t default_control_params = {
    .k_theta    = -0.5f ,
    .k_p        = -0.01230458266f,
    .k_i        = -0.00499678814f
};

static const char LOG_TAG[] = "CONTROL";

floatsat_err_t Control_Init(control_handle_t *handle)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }

    if(!handle->telemetry_handle || !handle->imu || !handle->madgwick 
        || !handle->g_control_config || !handle->motor || !handle->control_vars){
        return ERR_INVALID_ARG;
    }
    floatsat_err_t ret = ERR_OK;

    // if(!handle->g_target_angle || !handle->g_target_angle){
    //     return ERR_INVALID_ARG;
    // }

    GOTO_ON_ERR_LOG(DataBus_GetWriter_IMU(&handle->data_bus_writer),err, ret,
            LOG_TAG, "Could not acquire data bus writer");

    handle->g_control_config->control_internal  = handle->control_vars;

    handle->g_control_config->control_state     = CONTROL_STATE_NOMINAL;
    handle->g_control_config->params            = default_control_params;
    handle->g_control_config->control_active    = false;
    handle->g_control_config->control_variable  = CONTROL_VAR_ANGLE;
    handle->g_control_config->debug_id          = DEBUG_PACKET_ID_CONTROL;
            
    memset(handle->control_vars, 0, sizeof(control_vars_t));

#ifdef FUSION_H
    UNUSED(forward_vec);
    FusionAhrsInitialise(handle->fusion_ahrs);
    FusionAhrsSetSettings(handle->fusion_ahrs, handle->fusion_ahrs_settings);
    FusionBiasInitialise(handle->fusion_bias);
    FusionBiasSetSettings(handle->fusion_bias, handle->fusion_bias_settings);

#else
    Madgwick_Init(&handle->madgwick);
#endif

    LOGI(LOG_TAG,"Initialization succesful");
    return ERR_OK;
err:
    return ret;

}


__attribute__((noreturn))
void Task_ControlFn(void *args)
{
    control_handle_t *handle =  (control_handle_t*)args;
    handle->task_handle = xTaskGetCurrentTaskHandle();
    
    IMU_data_t  imu_data = {0}; // Current IMU data
    float       pwm_out = 0.0f;
    TickType_t  last_wake_time = xTaskGetTickCount();

    uint32_t    saturation_time_ms = 0U;

    control_data_t debug_control_data = {0};
    floatsat_err_t ret = ERR_OK;

    uint32_t dbg_count = 0U;
    UNUSED(dbg_count);

    while(1){

        /* ======================= Control Loop Start ======================= */

        ret = Control_EstimateOrientation(handle, &imu_data);
        if(ret == ERR_CONTROL_READ_FAIL){
            LOGE(LOG_TAG,"CRITICAL: Too many IMU misses. Recovering...");
            // TODO: Implement recovery
        }

        handle_rw_saturation(handle, &imu_data);
        angular_vel_failsafe(handle, &imu_data);

        if(handle->g_control_config->control_active){
            if(ret == ERR_OK){
                ret = Control_ComputeOutput(handle, &imu_data, &pwm_out, &debug_control_data);
            }
            
            if(ret == ERR_OK){
                Motor_SetDutyCycle(handle->motor, pwm_out);
            }
        }else{
            pwm_out = 0.0f;
        }

        // Check for saturation
        if(fabsf(pwm_out) >= CONTROL_MAX_PWM - CONTROL_PWM_SAT_THRS
            && handle->g_control_config->control_state == CONTROL_STATE_NOMINAL){
            saturation_time_ms += CONTROL_LOOP_PERIOD_MS;
        }else{
            saturation_time_ms = 0U;
        }

        if(saturation_time_ms >= CONTROL_MAX_SAT_TIME_MS){
            handle->g_control_config->control_state = CONTROL_STATE_RW_SAT;
        }

        /* ======================= Control Loop End ======================= */

        /* ======================= Reporting ======================= */
        DataBus_WriteIMU(handle->data_bus_writer, &imu_data);
        
        telemetry_packet_fast_t packet = {
            .orientation    = handle->current_orientation,
            .angular_vel    = imu_data.gyro.z * RADS_TO_DEG,
            .rw_speed       = handle->motor->current_speed, // In rev/s
            .theta_ref      = handle->g_control_config->target_angle
        };

        // TODO: I don't know if this is proper when having the data bus. It's a direct form of communication, though
        Telemetry_AddFast(handle->telemetry_handle, &packet);


        if(++dbg_count >= 4U){
            Control_SendDebug(handle, &imu_data, &debug_control_data);
            dbg_count = 0U;
        }

        xTaskNotify(handle->telemetry_handle->task_handle, 0U, eIncrement);

        if(*handle->p_core_task){
            xTaskNotify(*handle->p_core_task, 0U, eIncrement);
        }

        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(CONTROL_LOOP_PERIOD_MS));

    }

}


/// @brief 
/// @param handle 
/// @param imu_data 
static void handle_rw_saturation(control_handle_t *handle, const IMU_data_t *imu_data)
{
    /* Safety Procedure P-01: The FloatSat will send a alert to the Ground Station 
    and blink a red LED. The user must manually stop the FloatSat's rotation. 
    When Control detects that the  FloatSat is not rotating anymore, it will 
    turn off the reaction wheel to dissipate the accumulated momentum. The user 
    must hold the FloatSat still while the LED is still blinking. When the RW 
    speed reaches zero, normal operation resumes and the LED stops blinking */

    // TODO: Have the Core read the state and blink the LED


    switch (handle->g_control_config->control_state)
    {
    case CONTROL_STATE_NOMINAL:
        return;
        
    case CONTROL_STATE_RW_SAT: // Set in the control loop
        // Wait for the floatsat to be externally stopped (angular velocity near zero)
        if( is_stationary(imu_data->gyro.z * RADS_TO_DEG, CONTROL_MIN_ANGULAR_VEL)){
            handle->g_control_config->control_state = CONTROL_STATE_RW_SAT_RECOVERY;
            LOGI(LOG_TAG, "Entering RW_SAT_RECOVERY state");
        }

        break;

    case CONTROL_STATE_RW_SAT_RECOVERY:
        // deactivate the control and stop the reaction wheel
        handle->g_control_config->control_active = false;
        Motor_SetDutyCycle(handle->motor, 0.0f);

        // Wait for the reaction wheel to stop and reactivate the control
        if(is_stationary(handle->motor->current_speed, CONTROL_MIN_RW_SPEED)){
            // Reset the controller
            memset(handle->control_vars, 0, sizeof(control_vars_t));
            
            taskENTER_CRITICAL();
            handle->g_control_config->control_state = CONTROL_STATE_NOMINAL;
            handle->g_control_config->control_active = true;
            taskEXIT_CRITICAL();

            LOGI(LOG_TAG, "RW stopped, resuming normal operation");
        }
        break;

    default:
        break;
    }

}



static void angular_vel_failsafe(control_handle_t *handle, const IMU_data_t *imu_data)
{
    static bool is_rec_from_max_w = false;
    float w = imu_data->gyro.z * RADS_TO_DEG;

    // Angular vel failsafe with hysteresis
    if( fabsf(w) > CONTROL_MAX_ANGULAR_VEL && !is_rec_from_max_w){
        LOGW(LOG_TAG, "Warning: Exceeded angular velocity. Resetting control to default values");
        is_rec_from_max_w = true;

        memset(handle->control_vars, 0, sizeof(control_vars_t));

        taskENTER_CRITICAL();
        handle->g_control_config->params = default_control_params;
        handle->g_control_config->target_angle = 0.0f;
        handle->g_control_config->control_variable = CONTROL_VAR_ANGLE;
        taskEXIT_CRITICAL();

    }else if(fabsf(w) < CONTROL_MAX_ANGULAR_VEL - CONTROL_MAX_ANGULAR_VEL_HYST){
        is_rec_from_max_w = false;
    } 
}

/// @brief Determine if the floatsat it stationary. This is true if fabsf(w) < `threshold`
/// for CONTROL_STATIONARY_TIMESTEPS timesteps
/// @param w Current angular velocity in degrees
/// @return true if fabsf(w) has been lower than `threshold` for CONTROL_STATIONARY_TIMESTEPS
/// calls to this function
static bool is_stationary(float w, float threshold)
{
    static uint32_t stationary_timesteps = 0U;
    bool ret;

    if(fabsf(w) < threshold){
        stationary_timesteps++;
    }else{
        stationary_timesteps = 0U;
    }
    
    if(stationary_timesteps > CONTROL_STATIONARY_TIMESTEPS){
        ret = true;
        stationary_timesteps = 0U;
    }else{
        ret = false;
    }

    return ret;

}




/// @brief Reads the IMU, handles its errors, and updates the Madgwick filter to  
/// estimate current orientation. The result is saved in `handle->current_orientation`
/// @param handle Pointer to control handle
/// @param imu_data IMU_data_t struct to fill with IMU read
/// @return `ERR_INVALID_ARG` if `handle` was NULL. `ERR_CONTROL_READ_FAIL` if 
/// the IMU could not be read in the set amount of attempts. `ERR_OK` on success
static floatsat_err_t Control_EstimateOrientation(control_handle_t *handle, IMU_data_t *imu_data)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }
    static timestamp_t now_us = 0UL, last_update_us = 0UL;
    static uint32_t imu_misses = 0UL;
    
    float dt = 0.0f;
    floatsat_err_t  ret = ERR_OK;

    // TODO: Implement Mutex for the I2C peripheral
    ret = IMU_ReadData(handle->imu, imu_data); // Take IMU measurements

    if(ret == ERR_OK){
        imu_data->gyro  = Vec_SMult(&imu_data->gyro, MDPS_TO_RADS); // Convert gyro units from mDPS to rad/s
        imu_data->accel = Vec_SMult(&imu_data->accel, CM_TO_M);
        imu_data->mag   = Vec_SMult(&imu_data->mag, 1e-3f); // mGaus to Gaus 
        
        
        now_us = Util_GetTimeUs();
        dt = (float)(now_us - last_update_us) * 1.0e-6f;
        
        if(dt > CONTROL_MAX_DT_S){
            LOGW(LOG_TAG, "Madgwick orientation reset");
            Madgwick_ResetOrientation(handle->madgwick); // Delta t too long, previous estimate is invalid, reset the filter
            dt = (float)CONTROL_LOOP_PERIOD_MS * 1e-3f;
        }
        
        last_update_us = now_us;
        imu_misses = 0U;
    }else{
        imu_misses++;
    }
    
    if(imu_misses > CONTROL_MAX_IMU_MISSES){
        return ERR_CONTROL_READ_FAIL;
    }

    if(!handle->imu->is_calibrated){
        return ERR_IMU_NOT_CALIBRATED;
    }


    if(imu_misses == 0U){
#ifndef FUSION_H
        Madgwick_Update(handle->madgwick, imu_data, dt);
        handle->current_orientation = Vec3_Rotate(&forward_vec, &handle->madgwick->SEq);
#else
        // In g
        FusionVector acc = {
            .array = {
                imu_data->accel.x / G_TO_MS2, 
                imu_data->accel.y / G_TO_MS2, 
                imu_data->accel.z / G_TO_MS2}
        };

        // In degree/s
        FusionVector gyro = {
            .array = {
                imu_data->gyro.x * RADS_TO_DEG, 
                imu_data->gyro.y * RADS_TO_DEG, 
                imu_data->gyro.z * RADS_TO_DEG} 
        };

        FusionVector mag = {
            .array = {imu_data->mag.x, imu_data->mag.y, imu_data->mag.z} 
        };

        // FusionBiasUpdate(handle->fusion_bias, gyro);

        FusionAhrsSetSamplePeriod(handle->fusion_ahrs, dt);
        FusionAhrsUpdate(handle->fusion_ahrs, gyro, acc, mag);
        
        // Convert from Quaternion to vector. For how we just asume that north is (1,0,0)
        FusionQuaternion q = FusionAhrsGetQuaternion(handle->fusion_ahrs);
        FusionQuaternion q_conj = {
            .array = {q.array[0], -q.array[1], -q.array[2], -q.array[3]}
        };

        FusionQuaternion temp = FusionQuaternionVectorProduct(q, (FusionVector){.array = { 1.0, 0.0f, 0.0f}});
        temp = FusionQuaternionProduct(temp, q_conj);

        handle->current_orientation = (Vec3_t) {
            .x = temp.array[1],
            .y = temp.array[2],
            .z = temp.array[3],
        };

#endif

    
    }
    return ERR_OK;

}

/// @brief Uodate the control output based on the previously sampled state
/// @param handle 
/// @return 
static floatsat_err_t Control_ComputeOutput(
    const control_handle_t *handle,
    const IMU_data_t *imu_data, 
    float *out, 
    control_data_t *debug_data_out)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }
    
    control_cascade_params_t *params = &handle->g_control_config->params;
    control_vars_t *v = handle->control_vars;

    v->w = imu_data->gyro.z * (RADS_TO_DEG);
    v->theta   = atan2f(handle->current_orientation.y, handle->current_orientation.x) * (180.0f / (float)M_PI );
    
    if(handle->g_control_config->control_variable == CONTROL_VAR_ANGLE){

        // Azimuth angle (Relative to X Axis)
        
        v->e_theta = wrap_180(handle->g_control_config->target_angle - v->theta);
        v->u_theta = params->k_theta * v->e_theta;
        
        v->w_ref   = saturate_symmetric(v->u_theta, CONTROL_MAX_ANGULAR_VEL);
    }else if(handle->g_control_config->control_variable == CONTROL_VAR_ANGULAR_VEL){
        
        v->w_ref = handle->g_control_config->target_w;
    }

    v->e_w     = v->w_ref - v->w;

    // TODO: Add integral saturation
    v->u_P     = params->k_p * v->e_w;
    v->u_I     = v->u_I_prev + params->k_i * ((float)CONTROL_LOOP_PERIOD_MS * 1e-3f) * v->e_w;

    v->u       = v->u_P + v->u_I;
    v->u_sat   = saturate_symmetric(v->u, CONTROL_MAX_PWM); 
    
    v->u_I_prev = v->u_I;
    *out        = v->u_sat;

    if(debug_data_out){
        *debug_data_out = (control_data_t) {
            .angle      = v->theta,
            .angle_ref  = handle->g_control_config->target_angle,

            .w          = v->w,
            .w_ref      = v->w_ref,

            .pwm_out    = v->u_sat
        };
    }


    return ERR_OK;
}

// __attribute__((unused))
static floatsat_err_t Control_SendDebug(control_handle_t *handle, IMU_data_t *imu_data, control_data_t *control_data)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }

    telemetry_packet_debug_t debug_packet = {0};
    debug_packet.type       = PACKET_TYPE_DEBUG;
    debug_packet.debug_id   = handle->g_control_config->debug_id;
    debug_packet.timestamp  = HAL_GetTick();


    switch (debug_packet.debug_id)
    {
    case DEBUG_PACKET_ID_IMU:
        debug_packet.imu_data = *imu_data;
        break;
    
    case DEBUG_PACKET_ID_CONTROL:
        debug_packet.control_data = *control_data;
        break;

    default:
        return ERR_OK;
    }

    Telemetry_AddDebug(handle->telemetry_handle, &debug_packet);

    return ERR_OK;
}
