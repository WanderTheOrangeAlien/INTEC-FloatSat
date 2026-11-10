#include "floatsat_core.h"
#include <stdlib.h>
#include <string.h>

static bool cmd_used_malloc(uint8_t cmd_id);

STATIC floatsat_err_t Core_HandleCmd(floatsat_core_t *handle, floatsat_cmd_t *cmd);
STATIC floatsat_err_t Core_ChangeMission(floatsat_core_t *handle, uint32_t mission);
STATIC floatsat_err_t Core_AddPhoto(floatsat_core_t *handle, photo_info_t *photo_info);
STATIC floatsat_err_t Core_SetControl(floatsat_core_t *handle, floatsat_cmd_t *cmd);
STATIC floatsat_err_t Core_ResetPhotos(floatsat_core_t *handle);

static floatsat_err_t Core_ReadIMU(floatsat_core_t *handle, uint32_t flags);
static floatsat_err_t Core_IMUCalibrate(floatsat_core_t *handle, uint32_t flags);
static floatsat_err_t Core_IMUCalibrate_Gyro(floatsat_core_t *handle);

STATIC void Core_PrintPhotoInfo(floatsat_core_t *handle);

static floatsat_err_t Core_Mission_Playground(floatsat_core_t *handle);

static const char LOG_TAG[] = "CORE";

floatsat_err_t Core_Init(floatsat_core_t *handle)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }

    if(!handle->photo_info_arr || !handle->telementry_handle
        || !handle->g_control_config || !handle->motor_handle || !handle->imu_handle){
        return ERR_INVALID_ARG;
    }
    floatsat_err_t ret = ERR_OK;

    GOTO_ON_ERR(DataBus_InitReader(handle->data_bus_reader), err, ret);

    LOGI(LOG_TAG,"Initialization succesful");
    return ERR_OK;

err:
    return ret;
}


__attribute__((noreturn))
void Task_CoreFn(void *args)
{
    floatsat_core_t *handle = (floatsat_core_t*)args;
    assert(handle);

    handle->task_handle = xTaskGetCurrentTaskHandle();
    
    if(!handle->cmd_queue){
        LOGE(LOG_TAG,"Core CMD Queue was not initialized!");
        while(1){
            vTaskDelay(1000);
        }
    }

    Core_IMUCalibrate(handle, 
        CMD_IMU_CAL_FLAG_ACCEL | CMD_IMU_CAL_FLAG_GYRO | CMD_IMU_CAL_FLAG_MAG);
    
    while(1){

        // Check if there is a new command. Do not block
        // TODO: Finish core behaviour. For now we block to ease debugging
        if(xQueueReceive(handle->cmd_queue, &handle->current_cmd, portMAX_DELAY)){
            Core_HandleCmd(handle, &handle->current_cmd); // TODO: Check error
        }

        

    }
}



#pragma region mission

__attribute__((unused))
static floatsat_err_t Core_Mission_Playground(floatsat_core_t *handle)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }



    return ERR_OK;
}

#pragma endregion


#pragma region command_handling

STATIC floatsat_err_t Core_HandleCmd(floatsat_core_t *handle, floatsat_cmd_t *cmd)
{
    bool free_pending = false;
    if(!handle || !cmd){
        return ERR_INVALID_ARG;
    }
    floatsat_err_t ret = ERR_OK;

#ifdef TRACE
    static bool trace_start = true;

    if(trace_start){
        xTraceEnable(TRC_START);
        trace_start = false;
    }

#endif

    free_pending = cmd_used_malloc(cmd->cmd_id);
    switch (cmd->cmd_id)
    {
        case CMD_ID_CONTROL_INFO:
            taskENTER_CRITICAL();
            control_cascade_params_t *params = &handle->g_control_config->params;
            taskEXIT_CRITICAL();
            LOGI(LOG_TAG,"(INFO_CONTROL) {Type = %u\n Params: [K_theta = %f K_p = %f K_I = %f]\n Angle: %f\n}",
                    handle->g_control_config->control_type, 
                    (double)params->k_theta, (double)params->k_p, (double)params->k_i,
                    (double)handle->g_control_config->target_angle);
            
            break;
        case CMD_ID_CONTROL_SET:
            GOTO_ON_ERR(Core_SetControl(handle, cmd), err, ret);

            break;

        case CMD_ID_CONTROL_ANGLE:
            taskENTER_CRITICAL();
            memcpy(&handle->g_control_config->target_angle, &cmd->params, sizeof(float));
            taskEXIT_CRITICAL();
            LOGI(LOG_TAG,"Set target angle to: %f",(double)handle->g_control_config->target_angle);
 
            break;

        case CMD_ID_MISSION_INFO:
            LOGI(LOG_TAG,"(INFO_MISSION) Current mission ID: %u", handle->mission_id);
            break;
        case CMD_ID_MISSION_SET:
            GOTO_ON_ERR(Core_ChangeMission(handle, (uint32_t)cmd->params), err, ret);
            break;


        case CMD_ID_PHOTO_INFO:
            Core_PrintPhotoInfo(handle);
            break;
        case CMD_ID_PHOTO_ADD:
            // GOTO_ON_ERR(Core_AddPhoto(handle, (photo_info_t*)cmd->params), err, ret);
            // vTaskDelay(100);
            // xTraceDisable();
            break;

            break;
        case CMD_ID_PHOTO_RESET:
            GOTO_ON_ERR(Core_ResetPhotos(handle), err, ret);

            break;

        case CMD_ID_IMU_READ: {
            uint32_t flags = 0U;
            memcpy(&flags, &cmd->params, sizeof(uint32_t));
            GOTO_ON_ERR(Core_ReadIMU(handle, flags),err, ret);
            break;
        }

        case CMD_ID_IMU_CALIBRATE: {
            uint32_t flags = 0U;
            memcpy(&flags, &cmd->params, sizeof(uint32_t));
            GOTO_ON_ERR(Core_IMUCalibrate(handle, flags),err, ret);
            break;
        }
            

        case CMD_ID_MOTOR_PWM_SET:{
            float val = 0.0f;
            memcpy(&val, &cmd->params, sizeof(float));
            GOTO_ON_ERR(Motor_SetDutyCycle(handle->motor_handle, val), err, ret);
            LOGI(LOG_TAG,"Motor duty set to: %.4f",(double)val);
            break;
        }   
            
    


    default:
        break;
    }

    if(free_pending){
        free(cmd->params);
        cmd->params = NULL;
    }


    return ERR_OK;

err:
    if(free_pending){
        free(cmd->params);
        cmd->params = NULL;
    }
    return ret; 
}

STATIC floatsat_err_t Core_SetControl(floatsat_core_t *handle, floatsat_cmd_t *cmd)
{
    if(!handle || ! cmd){
        return ERR_INVALID_ARG;
    }
    floatsat_args_set_control_t *params = (floatsat_args_set_control_t*)cmd->params;
    
    taskENTER_CRITICAL();
    handle->g_control_config->params.k_theta = params->control_params[0];
    handle->g_control_config->params.k_p = params->control_params[1];
    handle->g_control_config->params.k_i = params->control_params[2];


    if(params->control_type != CONTROL_TYPE_NO_CHANGE){
        handle->g_control_config->control_type = params->control_type;
    }
    taskEXIT_CRITICAL();
    LOGI(LOG_TAG,"Control updated: {Type = %u. Params = [%f %f %f]}",
            handle->g_control_config->control_type,
            (double)handle->g_control_config->params.k_theta, (double)handle->g_control_config->params.k_p,
            (double)handle->g_control_config->params.k_i);


    return ERR_OK;
}

STATIC floatsat_err_t Core_ChangeMission(floatsat_core_t *handle, uint32_t mission)
{
    // Set the flag for mission change 
    handle->action_flags |= (CORE_ACTION_FLAG_CHANGE_MISSION);
    UNUSED(mission);

    return ERR_OK;
}

__attribute__((unused))
STATIC floatsat_err_t Core_AddPhoto(floatsat_core_t *handle, photo_info_t *photo_info)
{
    if(handle->last_photo_index >= CORE_MAX_PHOTO_INSTRUCTIONS){
        return ERR_CORE_MAX_PHOTOS;
    }

    // TODO: Validate params
    if(photo_info->angle < 0 || photo_info->angle >= 360){
        LOGE(LOG_TAG,"Error: Invalid angle for photo (%f). Must be in the range [0 360)",(double)photo_info->angle);
        return ERR_INVALID_ARG;
    }

    handle->photo_info_arr[handle->last_photo_index++] = *photo_info;

    return ERR_OK;
}

STATIC floatsat_err_t Core_ResetPhotos(floatsat_core_t *handle)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }
    // This is a lazy operation (done at the very end) because we don't want to 
    // interrupt the floatsat if it is currently taking a photo
    handle->action_flags |= CORE_ACTION_FLAG_RESET_PHOTOS;
    return ERR_OK;

}

STATIC void Core_PrintPhotoInfo(floatsat_core_t *handle)
{
    PRINTF("%-8s | %-8s | %-12s |\n","Time","Angle","Duration");
    PRINTF("---------|----------|--------------|\n");

    for (size_t i = 0; i < handle->last_photo_index; i++)
    {
        PRINTF("%02u:%02u:%02u | %8.2f | %12u\n",
            handle->photo_info_arr[i].time.hours, 
            handle->photo_info_arr[i].time.minutes, 
            handle->photo_info_arr[i].time.seconds,
            (double)handle->photo_info_arr[i].angle,
            handle->photo_info_arr[i].duration_ms);
    }
    


}

static floatsat_err_t Core_ReadIMU(floatsat_core_t *handle, uint32_t flags)
{
    if(!handle || flags == 0U){
        return ERR_INVALID_ARG;
    }
    floatsat_err_t ret = ERR_OK;
    IMU_data_raw_t data_raw = {0};
    IMU_data_t data = {0};


    if((flags & CMD_READ_IMU_FLAG_RAW) > 0U){
        GOTO_ON_ERR(IMU_ReadDataRaw(handle->imu_handle, &data_raw), err, ret);

        


    }else{
        GOTO_ON_ERR(IMU_ReadData(handle->imu_handle, &data),err, ret);
    }

    return ERR_OK;

err:
    LOGE(LOG_TAG,"Error reading IMU. Code: 0x%04x", ret);
    return ret;
}

static floatsat_err_t Core_IMUCalibrate(floatsat_core_t *handle, uint32_t flags)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }
    floatsat_err_t ret = ERR_OK;

    if(flags & CMD_IMU_CAL_FLAG_GYRO){
        LOGI(LOG_TAG, "Starting gyroscope calibration...");
        GOTO_ON_ERR(Core_IMUCalibrate_Gyro(handle), err, ret);

        LOGI(LOG_TAG, "Gyroscope calibration complete. Offsets: {%f %f %f}",
        (double)handle->imu_handle->calibration_data.gyro_offset.x,
        (double)handle->imu_handle->calibration_data.gyro_offset.y,
        (double)handle->imu_handle->calibration_data.gyro_offset.z
        );
    }

    if(flags & CMD_IMU_CAL_FLAG_ACCEL){
        LOGI(LOG_TAG, "Starting accelerometer calibration");
    }

    if(flags & CMD_IMU_CAL_FLAG_MAG){
        LOGI(LOG_TAG, "Starting magnetometer calibration");
    }

    if(flags == (CMD_IMU_CAL_FLAG_ACCEL | CMD_IMU_CAL_FLAG_GYRO | CMD_IMU_CAL_FLAG_MAG)){
        handle->imu_handle->is_calibrated = true;
    }

    return ERR_OK;
err:
    LOGE(LOG_TAG,"Error calibrating IMU");
    return ret;
}

static floatsat_err_t Core_IMUCalibrate_Gyro(floatsat_core_t *handle)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }

    IMU_data_t imu_data = {0};
    timestamp_t last_read_ms = 0U, now_ms = 0U;
    size_t i = 0U;
    Vec3_t gyro_bias_avg = {0};

    taskENTER_CRITICAL();
    handle->imu_handle->calibration_data.gyro_offset = (Vec3_t){0};
    taskEXIT_CRITICAL();

    while(++i <= IMU_CAL_GYRO_SAMPLES)
    {
        DataBus_ReadIMU(handle->data_bus_reader, &imu_data, &now_ms);
        if(now_ms == last_read_ms && i != 0){
            i--; // No new IMU read, repeat the step
            continue;
        }
        last_read_ms = now_ms;
        gyro_bias_avg.x += imu_data.gyro.x;
        gyro_bias_avg.y += imu_data.gyro.y;
        gyro_bias_avg.z += imu_data.gyro.z;

        // LOGI(LOG_TAG,"Gyro sample %u / %u",i , IMU_CAL_GYRO_SAMPLES);
        vTaskDelay(pdMS_TO_TICKS(IMU_CAL_GYRO_SAMPLE_PERIOD_MS));
    }

    gyro_bias_avg.x /= IMU_CAL_GYRO_SAMPLES;
    gyro_bias_avg.y /= IMU_CAL_GYRO_SAMPLES;
    gyro_bias_avg.z /= IMU_CAL_GYRO_SAMPLES;

    gyro_bias_avg = Vec_SMult(&gyro_bias_avg, RADS_TO_MDPS);

    taskENTER_CRITICAL();
    handle->imu_handle->calibration_data.gyro_offset = gyro_bias_avg;
    taskEXIT_CRITICAL();

    return ERR_OK;
}

#pragma endregion

#pragma region MISC

// List of CMD IDs that use malloc and require freeing
static const uint8_t malloc_users_cmd_id[] = {
    CMD_ID_CONTROL_SET, CMD_ID_PHOTO_ADD
};

static bool cmd_used_malloc(uint8_t cmd_id)
{
    for (size_t i = 0; i < sizeof(malloc_users_cmd_id); i++)
    {
        if(cmd_id == malloc_users_cmd_id[i]){
            return true;
        }
    }
    
    return false;
}

#pragma endregion
