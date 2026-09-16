#include "floatsat_core.h"
#include <stdlib.h>
#include <string.h>

STATIC floatsat_err_t Core_HandleCmd(floatsat_core_t *handle, floatsat_cmd_t *cmd);
STATIC floatsat_err_t Core_ChangeMission(floatsat_core_t *handle, uint32_t mission);
STATIC floatsat_err_t Core_AddPhoto(floatsat_core_t *handle, photo_info_t *photo_info);
STATIC floatsat_err_t Core_SetControl(floatsat_core_t *handle, floatsat_cmd_t *cmd);
STATIC floatsat_err_t Core_ResetPhotos(floatsat_core_t *handle);






void Task_CoreFn(void *args)
{
    floatsat_core_t *handle = (floatsat_core_t*)args;
    handle->task_handle = xTaskGetCurrentTaskHandle();
    while(1){

        // Check if there is a new command. Do not block
        if(xQueueReceive(handle->cmd_queue, &handle->current_cmd, 0U)){
            Core_HandleCmd(handle, &handle->current_cmd); // TODO: Check error
        }


        

    }
}

floatsat_err_t Core_Init(floatsat_core_t *handle)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }

    if(!handle->photo_info_arr || !handle->cmd_queue || !handle->telementry_handle
        || handle->g_control_config){
        return ERR_INVALID_ARG;
    }



}

#pragma region mission

floatsat_err_t Core_Mission_Playground(floatsat_core_t *handle)
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

    switch (cmd->cmd_id)
    {
        case CMD_ID_CONTROL_INFO:
            
            break;
        case CMD_ID_CONTROL_SET:
            free_pending = true;
            GOTO_ON_ERR(Core_SetControl(handle, cmd), err, ret);

            break;

        case CMD_ID_CONTROL_ANGLE:
            taskENTER_CRITICAL();
            memcpy(&handle->g_control_config->target_angle, cmd->params, sizeof(float));
            taskEXIT_CRITICAL();
 
            break;

        case CMD_ID_MISSION_INFO:

            break;
        case CMD_ID_MISSION_SET:
            GOTO_ON_ERR(Core_ChangeMission(handle, (uint32_t)cmd->params), err, ret);
            break;

        case CMD_ID_PHOTO_INFO:

            break;
        case CMD_ID_PHOTO_ADD:
            free_pending = true;
            GOTO_ON_ERR(Core_AddPhoto(handle, (photo_info_t*)cmd->params), err, ret);
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


    return ERR_OK;
}

STATIC floatsat_err_t Core_ChangeMission(floatsat_core_t *handle, uint32_t mission)
{
    // Set the flag for mission change 
    handle->action_flags |= (CORE_ACTION_FLAG_CHANGE_MISSION);

    return ERR_OK;
}

STATIC floatsat_err_t Core_AddPhoto(floatsat_core_t *handle, photo_info_t *photo_info)
{
    if(handle->last_photo_index >= CORE_MAX_PHOTO_INSTRUCTIONS){
        return ERR_CORE_MAX_PHOTOS;
    }

    // TODO: Validate params

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

#pragma endregion