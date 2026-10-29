#include "floatsat_core.h"
#include <stdlib.h>
#include <string.h>

static bool cmd_used_malloc(uint8_t cmd_id);

STATIC floatsat_err_t Core_HandleCmd(floatsat_core_t *handle, floatsat_cmd_t *cmd);
STATIC floatsat_err_t Core_ChangeMission(floatsat_core_t *handle, uint32_t mission);
STATIC floatsat_err_t Core_AddPhoto(floatsat_core_t *handle, photo_info_t *photo_info);
STATIC floatsat_err_t Core_SetControl(floatsat_core_t *handle, floatsat_cmd_t *cmd);
STATIC floatsat_err_t Core_ResetPhotos(floatsat_core_t *handle);
STATIC void Core_PrintPhotoInfo(floatsat_core_t *handle);

static floatsat_err_t Core_Mission_Playground(floatsat_core_t *handle);

static const char LOG_TAG[] = "CORE";

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
    
    while(1){

        // Check if there is a new command. Do not block
        // TODO: Finish core behaviour. For now we block to ease debugging
        if(xQueueReceive(handle->cmd_queue, &handle->current_cmd, portMAX_DELAY)){
            Core_HandleCmd(handle, &handle->current_cmd); // TODO: Check error
        }

        

    }
}

floatsat_err_t Core_Init(floatsat_core_t *handle)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }

    if(!handle->photo_info_arr || !handle->telementry_handle
        || !handle->g_control_config){
        return ERR_INVALID_ARG;
    }

    LOGI(LOG_TAG,"Initialization succesful");
    return ERR_OK;

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
            float *params = handle->g_control_config->params;
            taskEXIT_CRITICAL();
            LOGI(LOG_TAG,"(INFO_CONTROL) {Type = %u\n Params: [%f %f %f %f]\n Angle: %f\n}",
                    handle->g_control_config->control_type, 
                    (double)params[0],(double)params[1],(double)params[2],(double)params[3],
                    (double)handle->g_control_config->target_angle);
            
            break;
        case CMD_ID_CONTROL_SET:
            GOTO_ON_ERR(Core_SetControl(handle, cmd), err, ret);

            break;

        case CMD_ID_CONTROL_ANGLE:
            taskENTER_CRITICAL();
            memcpy(&handle->g_control_config->target_angle, &cmd->params, sizeof(float));
            LOGI(LOG_TAG,"Set target angle to: %f",(double)handle->g_control_config->target_angle);
            taskEXIT_CRITICAL();
 
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
    memcpy(handle->g_control_config->params, params->control_params, sizeof(params->control_params));

    if(params->control_type != CONTROL_TYPE_NO_CHANGE){
        handle->g_control_config->control_type = params->control_type;
    }
    taskEXIT_CRITICAL();
    LOGI(LOG_TAG,"Control updated: {Type = %u. Params = [%f %f %f %f]}",
            handle->g_control_config->control_type,
            (double)handle->g_control_config->params[0], (double)handle->g_control_config->params[1],
            (double)handle->g_control_config->params[2], (double)handle->g_control_config->params[3]
        );


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
