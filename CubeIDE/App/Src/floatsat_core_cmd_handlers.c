#include "floatsat_core_priv.h"

static uint32_t get_sys_cb_flag(const char *name);

static const char LOG_TAG[] = "CORE";

static const sys_cb_entry_t sys_cb_table[]  = {
    {CORE_ACTION_PHOTO_CLEANUP, "on_photo_finish"},
};

floatsat_err_t Core_SetControl(floatsat_core_t *handle, floatsat_cmd_t *cmd)
{
    if(!handle || ! cmd){
        return ERR_INVALID_ARG;
    }
    if(cmd->control_args.action_flags & CMD_CONTROL_FLAG_TYPE){
        handle->g_control_config->control_type = cmd->control_args.control_type;
        LOGI(LOG_TAG,"Changed control type to %u",handle->g_control_config->control_type);
    }

    if(cmd->control_args.action_flags & CMD_CONTROL_FLAG_PARAM){
        taskENTER_CRITICAL();
        handle->g_control_config->params.k_theta = cmd->control_args.k_theta;
        handle->g_control_config->params.k_p = cmd->control_args.k_p;
        handle->g_control_config->params.k_i = cmd->control_args.k_i;
        taskEXIT_CRITICAL();

        LOGI(LOG_TAG,
            "Changed control params to:{\n"
            "\tk_theta = %.4f\n"
            "\tk_p = %.4f\n"
            "\tk_i = %.4f\n",
            (double)handle->g_control_config->params.k_theta,
            (double)handle->g_control_config->params.k_p,
            (double)handle->g_control_config->params.k_i
        );
    }

    if(cmd->control_args.action_flags & CMD_CONTROL_FLAG_ANGLE){
        handle->g_control_config->target_angle = cmd->control_args.angle;
        LOGI(LOG_TAG,"Changed target angle to %.2f",(double)handle->g_control_config->target_angle);
    }

    if(cmd->control_args.action_flags & CMD_CONTROL_FLAG_DBG_ID){
        handle->g_control_config->debug_id = cmd->control_args.debug_id;
        LOGI(LOG_TAG,"Changed control debug packet id to %u", handle->g_control_config->debug_id);
    }

    if(cmd->control_args.action_flags & CMD_CONTROL_FLAG_ENABLE){
        handle->g_control_config->control_active = true;
        LOGI(LOG_TAG,"Control loop enabled");
    }

    if(cmd->control_args.action_flags & CMD_CONTROL_FLAG_DISABLE){
        handle->g_control_config->control_active = false;
        LOGI(LOG_TAG,"Control loop disabled");
    }


    return ERR_OK;
}

floatsat_err_t Core_ChangeMission(floatsat_core_t *handle, uint32_t mission)
{
    // Set the flag for mission change 
    handle->action_flags |= (CORE_ACTION_CHANGE_MISSION);
    UNUSED(mission); // TODO: Use this,lol

    return ERR_OK;
}

floatsat_err_t Core_AddPhoto(floatsat_core_t *handle, photo_info_t *photo)
{
    if(!handle || !photo){
        return ERR_INVALID_ARG;
    }

    if(handle->photo_arr_len >= CORE_MAX_PHOTO_INSTRUCTIONS){
        LOGE(LOG_TAG,"Error: Photo array full");
        return ERR_CORE_MAX_PHOTOS;
    }
    handle->photo_arr[handle->photo_arr_len] = *photo;

    photo_info_t *new_photo = &handle->photo_arr[handle->photo_arr_len];
    LOGI(LOG_TAG,
        "New photo instruction at index %u: {\n"
        "Angle: %.2f\n"
        "Time: %02u:%02u:%02u\n"
        "Amount: %u\n}",
        handle->photo_arr_len,
        (double)new_photo->angle, 
        new_photo->time.hours, new_photo->time.minutes, new_photo->time.seconds,
        new_photo->amount);

    handle->photo_arr_len++;
    handle->action_flags |= CORE_ACTION_UPDATE_PHOTO_ALARM;

    return ERR_OK;
}

floatsat_err_t Core_EditPhoto(floatsat_core_t *handle, photo_info_t *photo, uint8_t index, uint32_t flags)
{
    if(!handle || !photo){
        return ERR_INVALID_ARG;
    }

    if(index >= handle->photo_arr_len ){
        LOGE(LOG_TAG,"Error: The photo instruction at index %u has not been created",index);
        return ERR_INVALID_ARG;
    }

    if(flags == 0U){
        LOGE(LOG_TAG,"Tried to edit a photo instruction, but no fields were provided");
        return ERR_INVALID_ARG;
    }

    if(flags & CMD_PHOTO_FLAG_FIELD_ANGLE){
        handle->photo_arr[index].angle = photo->angle;
        LOGI(LOG_TAG,"[photo %u] Angle changed to %.2f",index,(double)handle->photo_arr[index].angle);
    }

    if(flags & CMD_PHOTO_FLAG_FIELD_TIME){
        handle->photo_arr[index].time = photo->time;
        LOGI(LOG_TAG,"[photo %u] Time changed to %02u:%02u:%02u",index,
            handle->photo_arr[index].time.hours,
            handle->photo_arr[index].time.minutes,
            handle->photo_arr[index].time.seconds);
    }

    if(flags & CMD_PHOTO_FLAG_FIELD_AMOUNT){
        handle->photo_arr[index].amount = photo->amount;
        LOGI(LOG_TAG,"[photo %u] Amount changed to %u",index,handle->photo_arr[index].amount);
    }

    handle->action_flags |= CORE_ACTION_UPDATE_PHOTO_ALARM;
    return ERR_OK;
}

floatsat_err_t Core_RemovePhoto(floatsat_core_t *handle, uint8_t index)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }

    if(handle->photo_arr_len == 0U){
        LOGE(LOG_TAG,"No photo instructions to remove");
        return ERR_INVALID_ARG;
    }

    if(index >= handle->photo_arr_len){
        LOGE(LOG_TAG,"Cannot remove photo instruction at index %u because it has not been created", index);
        return ERR_INVALID_ARG;
    }


    if(index + 1U == handle->photo_arr_len){
        // Last photo, no memmove required
        memset(&handle->photo_arr[index], 0, sizeof(photo_info_t));
        handle->photo_arr_len--;
    }else{
        memmove(&handle->photo_arr[index], 
            &handle->photo_arr[index + 1U], 
            (handle->photo_arr_len - index - 1U) * sizeof(photo_info_t));

        // Clear the last entry and decrease the index
        memset(&handle->photo_arr[handle->photo_arr_len - 1U], 0 , sizeof(photo_info_t));
        handle->photo_arr_len--;
    }

    LOGI(LOG_TAG,"Removed photo instruction at index %u", index);
    handle->action_flags |= CORE_ACTION_UPDATE_PHOTO_ALARM;
    return ERR_OK;
}


floatsat_err_t Core_ResetPhotos(floatsat_core_t *handle)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }
    // This is a lazy operation (done at the very end) because we don't want to 
    // interrupt the floatsat if it is currently taking a photo
    handle->action_flags |= CORE_ACTION_RESET_PHOTOS;
    return ERR_OK;

}

floatsat_err_t Core_MovePhoto(floatsat_core_t *handle, uint8_t src, uint8_t dest)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }

    if(src >= handle->photo_arr_len){
        LOGE(LOG_TAG, "Source out of bounds");
        return ERR_INVALID_ARG;
    }

    if(dest >= handle->photo_arr_len){
        LOGE(LOG_TAG, "Destination out of bounds");
        return ERR_INVALID_ARG;
    }

    LOGW(LOG_TAG,"Not implemented yet");
    return ERR_OK;
}

void Core_PrintPhotoInfo(floatsat_core_t *handle)
{
    PRINTF("%-8s | %-8s | %-12s |\n","Time","Angle","Amount");
    PRINTF("---------|----------|--------------|\n");

    for (size_t i = 0; i < handle->photo_arr_len; i++)
    {
        PRINTF("%02u:%02u:%02u | %8.2f | %12u\n",
            handle->photo_arr[i].time.hours, 
            handle->photo_arr[i].time.minutes, 
            handle->photo_arr[i].time.seconds,
            (double)handle->photo_arr[i].angle,
            handle->photo_arr[i].amount);
    }
    


}

floatsat_err_t Core_ReadIMU(floatsat_core_t *handle, uint32_t flags)
{
    if(!handle || flags == 0U){
        return ERR_INVALID_ARG;
    }
    floatsat_err_t ret = ERR_OK;
    IMU_data_raw_t data_raw = {0};
    IMU_data_t data = {0};


    if((flags & CMD_IMU_READ_FLAG_RAW) > 0U){
        GOTO_ON_ERR(IMU_ReadDataRaw(handle->imu_handle, &data_raw), err, ret);
    }else{
        GOTO_ON_ERR(IMU_ReadData(handle->imu_handle, &data),err, ret);
    }

    return ERR_OK;

err:
    LOGE(LOG_TAG,"Error reading IMU. Code: 0x%04x", ret);
    return ret;
}

floatsat_err_t Core_IMUCalibrate(floatsat_core_t *handle, uint32_t flags)
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

floatsat_err_t Core_IMUCalibrate_Gyro(floatsat_core_t *handle)
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

floatsat_err_t Core_Sysctl(floatsat_core_t *handle, floatsat_cmd_t *cmd)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }

    if(cmd->sysctl_args.action_flags & CMD_SYSCTL_FLAG_RESET){
        LOGI(LOG_TAG,"sysctl: Rebooting...");
        HAL_NVIC_SystemReset();
        return ERR_OK;
    }

    if(cmd->sysctl_args.action_flags & CMD_SYSCTL_FLAG_DBG_ID){
        handle->debug_id = cmd->sysctl_args.debug_id;
        handle->action_flags |= CORE_ACTION_UPDATE_DEBUG;
        LOGI(LOG_TAG, "Changed debug packet ID to %u", handle->debug_id);
    }

    if(cmd->sysctl_args.action_flags & CMD_SYSCTL_FLAG_INVOKE_CB){
        // The callback is not directly invoked from here. We set a lazy action flag instead
        // so the Core invokes the callback at the end of its behavior update

        uint32_t cb_flag = get_sys_cb_flag(cmd->sysctl_args.callback_name);
        handle->action_flags |= cb_flag;

        if(cb_flag != 0U){
            LOGI(LOG_TAG, "Invoking callback \"%s\"",cmd->sysctl_args.callback_name);
        }else{
            LOGW(LOG_TAG,"Callback name \"%s\" not found",cmd->sysctl_args.callback_name);
        }

    }

    return ERR_OK;
}


static uint32_t get_sys_cb_flag(const char *name)
{
    if(!name){
        return 0U;
    }

    for (size_t i = 0; i < sizeof(sys_cb_table) / sizeof(sys_cb_table[0]); i++)
    {
        if(strncmp(name, sys_cb_table[i].name, CMD_SYSCTL_MAX_CB_NAME_LEN) == 0){
            return sys_cb_table[i].flag;
        }
    }
    
    return 0U;
}





