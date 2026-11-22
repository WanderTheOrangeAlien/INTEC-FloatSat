#include "floatsat_core.h"
#include <stdlib.h>
#include <string.h>

// static bool cmd_used_malloc(uint8_t cmd_id);
#include "floatsat_core_priv.h"


static floatsat_err_t Core_HandleCmd(floatsat_core_t *handle, floatsat_cmd_t *cmd);

static floatsat_err_t Core_Mission_Playground(floatsat_core_t *handle);

static floatsat_err_t Core_Update(floatsat_core_t *handle);
static floatsat_err_t Core_DebugUpdate(floatsat_core_t *handle);
static floatsat_err_t Core_SendDebug(floatsat_core_t *handle);

static floatsat_err_t Core_UpdatePhotoAlarm(floatsat_core_t *handle);
static void Core_OnPhotoAlarm(rtc_handle_t *rtc);
static void Core_OnPhotoFinish(floatsat_core_t *handle);
static floatsat_err_t get_next_photo_index(const floatsat_core_t *handle, int *index);

static floatsat_err_t Core_TakePhoto(floatsat_core_t *handle);

static const char LOG_TAG[] = "CORE";

floatsat_err_t Core_Init(floatsat_core_t *handle)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }

    if(!handle->photo_arr || !handle->telementry_handle
        || !handle->g_control_config || !handle->motor_handle || !handle->imu_handle
        || !handle->rtc || !handle->light_sensor){
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
    
    handle->imu_handle->calibration_data.mag_offset = IMU_MAG_OFFSET;
    handle->imu_handle->calibration_data.mag_scale = IMU_MAG_SCALE;

    vTaskDelay(pdMS_TO_TICKS(1000));

    while(1){
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // Check if there is a new command. Do not block
        // TODO: Finish core behaviour. For now we block to ease debugging
        if(xQueueReceive(handle->cmd_queue, &handle->current_cmd, 0)){
            Core_HandleCmd(handle, &handle->current_cmd);
        }

        // If the commmand requested an action, perform it at the end
        if(handle->action_flags != 0U){
            Core_Update(handle);
        }

        Core_SendDebug(handle);

    }
}


static floatsat_err_t Core_Update(floatsat_core_t *handle)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }
    floatsat_err_t ret = ERR_OK;

    if(handle->action_flags & CORE_ACTION_UPDATE_DEBUG){
        GOTO_ON_ERR(Core_DebugUpdate(handle), err, ret);
        handle->action_flags &= (uint32_t)~(CORE_ACTION_UPDATE_DEBUG);
    }

    if(handle->action_flags & CORE_ACTION_UPDATE_PHOTO_ALARM){
        GOTO_ON_ERR(Core_UpdatePhotoAlarm(handle), err, ret);
        handle->action_flags &= (uint32_t)~CORE_ACTION_UPDATE_PHOTO_ALARM;
    }

    if(handle->action_flags & CORE_ACTION_SET_PHOTO_ALARM){
        GOTO_ON_ERR(RTC_SetAlarm(handle->rtc, &handle->next_photo.time, 0U, Core_OnPhotoAlarm, handle), err, ret);
        LOGI(LOG_TAG,"Create new alarm at %02u:%02u:%02u",
            handle->next_photo.time.hours,
            handle->next_photo.time.minutes,
            handle->next_photo.time.seconds);
        handle->action_flags &= (uint32_t)~CORE_ACTION_SET_PHOTO_ALARM;
    }


    if(handle->action_flags & CORE_ACTION_PHOTO_CLEANUP){
        Core_OnPhotoFinish(handle);
        handle->action_flags &= (uint32_t)~CORE_ACTION_PHOTO_CLEANUP;
    }

    if(handle->action_flags & CORE_ACTION_BRIDGE_ACTIVE){
        handle->telementry_handle->bridge_active = true;
        handle->action_flags &= (uint32_t)~CORE_ACTION_BRIDGE_ACTIVE;
        LOGI(LOG_TAG,"Telemetry now active");

    }

     if(handle->action_flags & CORE_ACTION_TAKE_PHOTO){
        Core_TakePhoto(handle);
        handle->action_flags &= (uint32_t)~CORE_ACTION_TAKE_PHOTO;
    }




    return ERR_OK;

err:
    LOGE(LOG_TAG, "Error: Error in Core update. Action flags: 0x%08x. Error code: 0x%04x", handle->action_flags, ret);
    return ret;
}

#pragma region REGION_DEBUG

static floatsat_err_t Core_DebugUpdate(floatsat_core_t *handle)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }

    switch (handle->debug_id)
    {      
    case DEBUG_PACKET_ID_NONE:
    case DEBUG_PACKET_ID_CONTROL:
    case DEBUG_PACKET_ID_IMU:
        handle->g_control_config->debug_id = handle->debug_id;
        break;
    
    default:
        handle->g_control_config->debug_id = DEBUG_PACKET_ID_NONE;
        break;
    }

    return ERR_OK;
}

static floatsat_err_t Core_SendDebug(floatsat_core_t *handle)
{
    static uint32_t next_send_ms = 0U;

    switch (handle->debug_id)
    {
    case DEBUG_PACKET_ID_RTC:
        if(HAL_GetTick() >= next_send_ms){
            uint32_t ms = HAL_GetTick();
            next_send_ms = ms + 1000U;
            telemetry_packet_debug_t packet = {
                .type = PACKET_TYPE_DEBUG,
                .timestamp = ms,
                .debug_id = DEBUG_PACKET_ID_RTC,
            };

            RTC_GetTimeDate(handle->rtc, &packet.rtc_time, NULL);
            Telemetry_AddDebug(handle->telementry_handle,&packet);
        }
        break;
    
    default:
        break;
    }
  

    return ERR_OK;
}
#pragma endregion

#pragma region PHOTO

static floatsat_err_t Core_UpdatePhotoAlarm(floatsat_core_t *handle)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }
    floatsat_err_t ret = ERR_OK;

    int next_photo_index;
    GOTO_ON_ERR(get_next_photo_index(handle, &next_photo_index), err, ret);

    if(next_photo_index == -1){
        LOGI(LOG_TAG,"No alarm updates required");
        return ERR_OK;
    }

    // Do not modify the photo instruction if it is being executed
    if(handle->mission_state != MISSION_STATE_PHOTO_BUSY){
        handle->next_photo = handle->photo_arr[next_photo_index];
    }


    // The floatsat needs several seconds to reach the photo position, so we set the alarm a few seconds before scheduled
    floatsat_time_t alarm_time;
    GOTO_ON_ERR(
        Time_SubtractSeconds(&handle->next_photo.time, 
                            CORE_PHOTO_PREPARATION_TIME_S,
                            &alarm_time),
        err,ret);

    RTC_SetAlarm(handle->rtc, &alarm_time, 0U, 
                 &Core_OnPhotoAlarm, (void*)handle);

    LOGI(LOG_TAG, "Created new alarm at %02u:%02u:%02u",
        alarm_time.hours, alarm_time.minutes, alarm_time.seconds);

    return ERR_OK;

err:
    LOGE(LOG_TAG,"Error: Error updating the alarm. Error code 0x%04x",ret);
    return ret;
}

/// @brief Get the index of the next photo to take (The closest in time from now)
/// @param handle 
/// @param index 
/// @return 
static floatsat_err_t get_next_photo_index(const floatsat_core_t *handle, int *index)
{
    if(!handle || !index){
        return ERR_INVALID_ARG;
    }

    floatsat_time_t now;
    floatsat_err_t ret = RTC_GetTimeDate(handle->rtc, &now, NULL);
    if(ret != ERR_OK){
        return ret;
    }
    
    int next_photo_index = -1;
    int min_delta = INT32_MAX, delta = 0;
    for (size_t i = 0; i < handle->photo_arr_len; i++)
    {

        delta = Time_Delta(&handle->photo_arr[i].time, &now);
        if(delta < 0){
            continue;
        }
        if(delta < min_delta){
            min_delta = delta;
            next_photo_index = (int)i;
        }
    }
    
    *index = next_photo_index;

    return ERR_OK;
}


static void Core_OnPhotoAlarm(rtc_handle_t *rtc)
{
    floatsat_core_t *handle = (floatsat_core_t*)rtc->ctx;
    
    // RTC and telemetry functions are not ISR safe, becuase they use non-ISR freeRTOS functions
    // To work around this, we raise a lazy action flag so that the Core calls the corresponding function
    // duirng its behavior update

    switch (handle->mission_state)
    {
    case MISSION_STATE_PHOTO_IDLE:
        handle->g_control_config->target_angle = handle->next_photo.angle;

        handle->mission_state = MISSION_STATE_PHOTO_MOVING;
        handle->action_flags |= CORE_ACTION_SET_PHOTO_ALARM;
        break;

    case MISSION_STATE_PHOTO_MOVING:
        handle->mission_state = MISSION_STATE_PHOTO_BUSY;
        handle->action_flags |= CORE_ACTION_TAKE_PHOTO;
        break;
    
    default:
        break;
    }

}

static floatsat_err_t Core_TakePhoto(floatsat_core_t *handle)
{
    telemetry_packet_cmd_t cmd = {
        .type = PACKET_TYPE_CMD,
        .cmd = {
            .cmd_id = EXT_CMD_TAKE_PHOTO,
            .args.photo_amount = handle->next_photo.amount
        },
    };

    Telemetry_AddCommand(handle->telementry_handle, &cmd);

    return ERR_OK;
}

static void Core_OnPhotoFinish(floatsat_core_t *handle)
{
    handle->mission_state = MISSION_STATE_PHOTO_IDLE;
    handle->action_flags |= CORE_ACTION_UPDATE_PHOTO_ALARM;

    LOGI(LOG_TAG,"Photo taken successfully!");
}


#pragma endregion


__attribute__((unused))
static floatsat_err_t Core_Mission_Playground(floatsat_core_t *handle)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }



    return ERR_OK;
}
#pragma region command_handling

static floatsat_err_t Core_HandleCmd(floatsat_core_t *handle, floatsat_cmd_t *cmd)
{
    if(!handle || !cmd){
        return ERR_INVALID_ARG;
    }
    floatsat_err_t ret = ERR_OK;
    // free_pending = cmd_used_malloc(cmd->cmd_id);
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

        case CMD_ID_CONTROL_MODIFY:
            Core_SetControl(handle, cmd);
            break;
        

        case CMD_ID_MISSION_INFO:
            LOGI(LOG_TAG,"(INFO_MISSION) Current mission ID: %u", handle->mission_id);
            break;
        case CMD_ID_MISSION_SET:
            GOTO_ON_ERR(Core_ChangeMission(handle, cmd->mission_args.mission_id), err, ret);
            break;


        case CMD_ID_PHOTO_INFO:
            Core_PrintPhotoInfo(handle);
            break;
        case CMD_ID_PHOTO_ADD:
            GOTO_ON_ERR(Core_AddPhoto(handle,&cmd->photo_args.photo ), err, ret);
            break;

        case CMD_ID_PHOTO_EDIT:
            GOTO_ON_ERR(Core_EditPhoto(handle, &cmd->photo_args.photo, 
                cmd->photo_args.index, cmd->photo_args.action_flags), err, ret);
            break;

        case CMD_ID_PHOTO_REMOVE:
            GOTO_ON_ERR(Core_RemovePhoto(handle, cmd->photo_args.index), err, ret);
            break;

        case CMD_ID_PHOTO_RESET:
            GOTO_ON_ERR(Core_ResetPhotos(handle), err, ret);
            break;

        case CMD_ID_PHOTO_TAKE:
            GOTO_ON_ERR(Core_TakePhoto(handle), err, ret);
            break;

        case CMD_ID_PHOTO_MOVE:
            GOTO_ON_ERR(Core_MovePhoto(handle, cmd->photo_args.index, cmd->photo_args.dest_index), err, ret);
            break;

        case CMD_ID_IMU_READ: {
            GOTO_ON_ERR(Core_ReadIMU(handle, cmd->imu_args.action_flags),err, ret);
            break;
        }

        case CMD_ID_IMU_CALIBRATE: {
            GOTO_ON_ERR(Core_IMUCalibrate(handle, cmd->imu_args.action_flags),err, ret);
            break;
        }
            

        case CMD_ID_MOTOR_PWM_SET:{
            GOTO_ON_ERR(Motor_SetDutyCycle(handle->motor_handle, cmd->motor_args.pwm_val), err, ret);
            LOGI(LOG_TAG,"Motor duty set to: %.4f",(double)cmd->motor_args.pwm_val);
            break;
        }

        case CMD_ID_SYSCTL: {
            GOTO_ON_ERR(Core_Sysctl(handle, cmd), err, ret);
        }
            
    


    default:
        break;
    }

    return ERR_OK;

err:

    return ret; 
}

#pragma endregion
