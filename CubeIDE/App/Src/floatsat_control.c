#include "floatsat_control.h"
#include <string.h>

static floatsat_err_t Control_ComputeOutput(
    const control_handle_t *handle, 
    const IMU_data_t *imu_data, 
    float *out, 
    control_data_t *debug_data_out);

static floatsat_err_t Control_EstimateOrientation(control_handle_t *handle, IMU_data_t *imu_data);
static floatsat_err_t Control_SendDebug(control_handle_t *handle, IMU_data_t *imu_data, control_data_t *control_data);


static const Vec3_t forward_vec = {1,0,0}; // Here we assume that the north corresponds to the X axis. 
                                           // TODO: Check the IMU orientation to confirm this 

static const char LOG_TAG[] = "CONTROL";

floatsat_err_t Control_Init(control_handle_t *handle)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }

    if(!handle->telemetry_handle || !handle->imu || !handle->madgwick 
        || !handle->g_control_config || !handle->motor){
        return ERR_INVALID_ARG;
    }
    floatsat_err_t ret = ERR_OK;

    // if(!handle->g_target_angle || !handle->g_target_angle){
    //     return ERR_INVALID_ARG;
    // }

    GOTO_ON_ERR_LOG(DataBus_GetWriter_IMU(&handle->data_bus_writer),err, ret,
            LOG_TAG, "Could not acquire data bus writer");

    handle->debug_packet_id = DEBUG_PACKET_ID_IMU;


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

    control_data_t debug_control_data = {0};


    floatsat_err_t ret = ERR_OK;
    while(1){

        /* ======================= Control Loop Start ======================= */

        ret = Control_EstimateOrientation(handle, &imu_data);
        if(ret == ERR_CONTROL_READ_FAIL){
            LOGE(LOG_TAG,"CRITICAL: Too many IMU misses. Recovering...");
            // TODO: Implement recovery
        }

        if(ret == ERR_OK){
            ret = Control_ComputeOutput(handle, &imu_data, &pwm_out, &debug_control_data);
        }

        /* ======================= Control Loop End ======================= */

        /* ======================= Reporting ======================= */
        DataBus_WriteIMU(handle->data_bus_writer, &imu_data);
        
        telemetry_packet_fast_t packet = {
            .orientation    = handle->current_orientation,
            .rw_speed       = handle->motor->current_speed
        };

        // TODO: I don't know if this is proper when having the data bus. It's a direct form of communication, though
        Telemetry_AddFast(handle->telemetry_handle, &packet);
        Control_SendDebug(handle, &imu_data, &debug_control_data);

        xTaskNotify(handle->telemetry_handle->task_handle, 0U, eNoAction);
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(CONTROL_LOOP_PERIOD_MS));

    }

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
        imu_data->gyro = Vec_SMult(&imu_data->gyro, MDPS_TO_RADS); // Convert gyro units from mDPS to rad/s
        imu_data->accel = Vec_SMult(&imu_data->accel, CM_TO_M);

        now_us = Util_GetTimeUs();
        dt = (float)(now_us - last_update_us) * 1.0e-6f;
        
        if(dt > CONTROL_MAX_DT_S){
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
        Madgwick_Update(handle->madgwick, imu_data, dt);
        handle->current_orientation = Vec3_Rotate(&forward_vec, &handle->madgwick->SEq);
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

    float theta, e_theta, u_theta;
    float w_ref, e_w;
    float u_P, u, u_sat;
    static float u_I = 0.0f, u_I_prev = 0.0f;
    control_cascade_params_t *params = &handle->g_control_config->params;


    // Azimuth angle (Relative to X Axis)
    theta   = atan2f(handle->current_orientation.y, handle->current_orientation.x) * (180.0f / (float)M_PI );
    
    e_theta = wrap_180(handle->g_control_config->target_angle - theta);
    u_theta = params->k_theta * e_theta;

    w_ref   = saturate_symmetric(u_theta, CONTROL_MAX_ANGULAR_VEL);
    e_w     = w_ref - imu_data->gyro.z;

    u_P     = params->k_p * e_w;
    u_I     = u_I_prev + params->k_i * ((float)CONTROL_LOOP_PERIOD_MS * 1e-3f) * e_w;

    u       = u_P + u_I;
    u_sat   = saturate_symmetric(u, 1.0f); 
    
    u_I_prev = u_I;
    *out    = u_sat;

    if(debug_data_out){
        *debug_data_out = (control_data_t) {
            .timestamp  = HAL_GetTick(),

            .angle      = theta,
            .angle_ref  = handle->g_control_config->target_angle,

            .w          = imu_data->gyro.z,
            .w_ref      = w_ref,

            .pwm_out    = u_sat
        };
    }


    return ERR_OK;
}


static floatsat_err_t Control_SendDebug(control_handle_t *handle, IMU_data_t *imu_data, control_data_t *control_data)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }

    telemetry_packet_debug_t debug_packet = {0};
    debug_packet.type       = PACKET_TYPE_DEBUG;
    debug_packet.debug_id   = handle->debug_packet_id;

    switch (handle->debug_packet_id)
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
