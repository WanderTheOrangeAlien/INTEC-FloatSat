#include "floatsat_core.h"
#include <stdlib.h>
#include <string.h>

// static bool cmd_used_malloc(uint8_t cmd_id);
#include "floatsat_core_priv.h"


static floatsat_err_t Core_HandleCmd(floatsat_core_t *handle, floatsat_cmd_t *cmd);
static floatsat_err_t Core_ChangeMission(floatsat_core_t *handle);

static floatsat_err_t Core_Mission_Playground(floatsat_core_t *handle);
static floatsat_err_t Core_Mission_Sunchaser(floatsat_core_t *handle);
static bool is_light_acquired(float e_light, float min_delta, uint32_t dt_ms, uint32_t min_stable_time);
static bool light_detected(float S[3], float threshold);
static floatsat_err_t Core_OnLightAcquired(floatsat_core_t *handle);


static floatsat_err_t Core_Update(floatsat_core_t *handle);
static floatsat_err_t Core_UpdateActions(floatsat_core_t *handle);
static floatsat_err_t Core_DebugUpdate(floatsat_core_t *handle);
static floatsat_err_t Core_UpdateStatusLeds(floatsat_core_t *handle);
static floatsat_err_t Core_SendDebug(floatsat_core_t *handle);
static floatsat_err_t Core_SendTelemetry(floatsat_core_t *handle);

static floatsat_err_t Core_UpdatePhotoAlarm(floatsat_core_t *handle);
static void Core_OnPhotoAlarm(rtc_handle_t *rtc);
static void Core_OnPhotoFinish(floatsat_core_t *handle);
static void Core_OnPhotoFail(floatsat_core_t *handle);
static floatsat_err_t get_next_photo_index(const floatsat_core_t *handle, int *index);

static floatsat_err_t Core_TakePhoto(floatsat_core_t *handle);

static void Core_BlinkLED(gpio_info_t *led, uint32_t blink_time_ms);



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

    handle->photo_deadline_tick = portMAX_DELAY;

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


        Core_Update(handle);
        Core_SendTelemetry(handle);


        Core_SendDebug(handle);

    }
}

static floatsat_err_t Core_Update(floatsat_core_t *handle)
{
    floatsat_err_t ret = ERR_OK;


    switch (handle->current_mission)
    {
    case MISSION_ID_PHOTO:
        // The mission is handled asynchronously.    

        // Check for timeout
        if(handle->mission_state == MISSION_STATE_PHOTO_BUSY &&
            xTaskGetTickCount() > handle->photo_deadline_tick)
        {
            LOGE(LOG_TAG,"Photo timeout exceeded");
            handle->photo_deadline_tick = portMAX_DELAY;
            Core_OnPhotoFail(handle);
        }
        break;
    
    case MISSION_ID_SUNCHASER:
        LOG_ON_ERR(Core_Mission_Sunchaser(handle),
            LOG_TAG, "Error in mission update: SUNCHASER. ret = 0x%04x", ret);

    default:
        break;
    }

    Core_UpdateStatusLeds(handle);

    // If the command requested an action, perform it at the end
    if(handle->action_flags != 0U){
    	ret = Core_UpdateActions(handle);
    }

    return ret;
}

static floatsat_err_t Core_UpdateActions(floatsat_core_t *handle)
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

    if(handle->action_flags & CORE_ACTION_PHOTO_FAIL){
        Core_OnPhotoFail(handle);
        handle->action_flags &= (uint32_t)~CORE_ACTION_PHOTO_FAIL;
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

    if(handle->action_flags & CORE_ACTION_CHANGE_MISSION){
        // Attempt mission change
        ret = Core_ChangeMission(handle);
        // Clear the flag only if the mission change was successful. Otherwise leave set 
        // to attempt on the next loop
        if(ret == ERR_OK){
            handle->action_flags &= (uint32_t)~CORE_ACTION_CHANGE_MISSION;
            LOGI(LOG_TAG,"Mission changed to %u",handle->current_mission);
        }
    }




    return ERR_OK;

err:
    LOGE(LOG_TAG, "Error: Error in Core update. Action flags: 0x%08x. Error code: 0x%04x", handle->action_flags, ret);
    return ret;
}

static floatsat_err_t Core_UpdateStatusLeds(floatsat_core_t *handle)
{

    // Flash Red Led during reaction wheel saturation
    if(handle->g_control_config->control_state == CONTROL_STATE_RW_SAT || 
        handle->g_control_config->control_state == CONTROL_STATE_RW_SAT_RECOVERY)
    {
        Core_BlinkLED(&handle->status_leds.led_red, CORE_LED_RED_BLINK_PERIOD_MS);
    }else{
        HAL_GPIO_WritePin(  handle->status_leds.led_red.port,
                            handle->status_leds.led_red.gpio_num, 
                            GPIO_PIN_RESET);

        HAL_GPIO_WritePin(  handle->status_leds.led_green.port,
                            handle->status_leds.led_green.gpio_num, 
                            GPIO_PIN_SET);
    }


    return ERR_OK;
}

#pragma region TELEMETRY
static floatsat_err_t Core_SendTelemetry(floatsat_core_t *handle)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }

    static uint32_t last_ms = 0U;
    uint32_t now_ms = HAL_GetTick();

    if(now_ms < last_ms + CORE_SLOW_TELEEMTRY_PERIOD_MS){
        return ERR_OK;
    }
    last_ms = now_ms;

    telemetry_packet_slow_t packet = {
        .current_mission    = handle->current_mission,
        .control_params     = handle->g_control_config->params,
    };


    if(handle->current_mission == MISSION_ID_SUNCHASER){
        packet.sunchaser_state = handle->mission_state;
        packet.light_l         = handle->light_readings[LIGHT_SENSOR_IDX_LEFT];
        packet.light_r         = handle->light_readings[LIGHT_SENSOR_IDX_RIGHT];
    }

    return Telemetry_AddSlow(handle->telementry_handle, &packet);
}


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
    case DEBUG_PACKET_ID_LIGHT:

        // Packet is set by Core_Mission_Sunchaser when state == MISSION_STATE_SUN_ACQUIRE
        // Otherwise, zero it out
        if(handle->mission_state != MISSION_STATE_SUN_ACQUIRE){
            memset(&handle->current_debug_packet, 0, sizeof(telemetry_packet_debug_t));
            handle->current_debug_packet.type       = PACKET_TYPE_DEBUG;
            handle->current_debug_packet.debug_id   = DEBUG_PACKET_ID_LIGHT;
        }
        Telemetry_AddDebug(handle->telementry_handle, &handle->current_debug_packet);
        break;
        
    
    default:
        break;
    }
  

    return ERR_OK;
}

#pragma endregion

#pragma region PHOTO

/// @brief Check the photo instruction array and set a new alarm if required
/// @param handle 
/// @return 
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
    if(handle->mission_state == MISSION_STATE_PHOTO_BUSY){
        ret = ERR_RESOURCE_BUSY;
        goto err;
    }

    handle->next_photo_idx = (uint8_t)next_photo_index;
    handle->next_photo = handle->photo_arr[next_photo_index];

    // The floatsat needs several seconds to reach the photo position, so we set the alarm a few seconds before scheduled
    floatsat_time_t alarm_time;
    GOTO_ON_ERR(
        Time_SubtractSeconds(&handle->next_photo.time, 
                            CORE_PHOTO_PREPARATION_TIME_S,
                            &alarm_time),
        err,ret);

    RTC_SetAlarm(handle->rtc, &alarm_time, 0U, 
                 &Core_OnPhotoAlarm, (void*)handle);
    handle->next_photo_is_async = false;

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
        handle->g_control_config->control_variable = CONTROL_VAR_ANGLE;
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
    // The state should be BUSY when entering this function
    if(handle->mission_state != MISSION_STATE_PHOTO_BUSY){
        LOGE(LOG_TAG,"Tried to take a photo but mission state was not BUSY");
        return ERR_INVALID_ARG;
    }

    telemetry_packet_cmd_t cmd = {
        .type = PACKET_TYPE_CMD,
        .cmd = {
            .cmd_id = EXT_CMD_TAKE_PHOTO,
            .photo_args = {
                .photo_amount   = handle->next_photo.amount,
                .yaw            = handle->next_photo.angle,
                .timestamp      = handle->next_photo.time
            },
        },
    };

    handle->photo_deadline_tick = xTaskGetTickCount() 
        + pdMS_TO_TICKS(CORE_PHOTO_TIMEOUT_MS) * handle->next_photo.amount;
        
    return Telemetry_AddCommand(handle->telementry_handle, &cmd);
}

static void Core_OnPhotoFinish(floatsat_core_t *handle)
{
    handle->mission_state = MISSION_STATE_PHOTO_IDLE;
    handle->action_flags |= CORE_ACTION_UPDATE_PHOTO_ALARM;
    LOGI(LOG_TAG,"Photo taken successfully!");

    if(!handle->next_photo_is_async){
        Core_RemovePhoto(handle, handle->next_photo_idx);
    }
    
    handle->next_photo_is_async = false;
}

static void Core_OnPhotoFail(floatsat_core_t *handle)
{
    handle->mission_state = MISSION_STATE_PHOTO_IDLE;
    handle->action_flags |= CORE_ACTION_UPDATE_PHOTO_ALARM;

    LOGE(LOG_TAG,"An error ocurred while taking the photo");

    if(!handle->next_photo_is_async){
        Core_RemovePhoto(handle, handle->next_photo_idx);
    }
    
    handle->next_photo_is_async = false;

}


#pragma endregion

#pragma region MISSIONS

static floatsat_err_t Core_ChangeMission(floatsat_core_t *handle)
{

    if(handle->g_control_config->control_state != CONTROL_STATE_NOMINAL){
        return ERR_INVALID_STATE;
    }

    switch (handle->current_mission)
    {
    case MISSION_ID_PLAYGROUND:
    case MISSION_ID_SUNCHASER:

        break;

    case MISSION_ID_PHOTO:
        if(handle->mission_state != MISSION_STATE_PHOTO_IDLE){
            return ERR_INVALID_STATE;
        }
        break;

    default:
        break;
    }

    switch (handle->next_mission)
    {
    case MISSION_ID_PLAYGROUND:
        // Reset control angle
        taskENTER_CRITICAL();
        handle->g_control_config->target_angle = 0.0f;
        handle->g_control_config->control_variable = CONTROL_VAR_ANGLE;
        taskEXIT_CRITICAL();


        break;
    case MISSION_ID_PHOTO:
        // Reset control angle
        taskENTER_CRITICAL();
        handle->g_control_config->target_angle = 0.0f;
        handle->g_control_config->control_variable = CONTROL_VAR_ANGLE;
        taskEXIT_CRITICAL();
        handle->mission_state = MISSION_STATE_PHOTO_IDLE;
        
        break;
        
    case MISSION_ID_SUNCHASER:
        handle->mission_state = MISSION_STATE_SUN_SEARCH;

    default:
        break;
    }

    handle->current_mission = handle->next_mission;

    return ERR_OK;
}


__attribute__((unused))
static floatsat_err_t Core_Mission_Playground(floatsat_core_t *handle)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }



    return ERR_OK;
}

static floatsat_err_t Core_Mission_Sunchaser(floatsat_core_t *handle)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }

    floatsat_err_t ret = ERR_OK;
    control_config_t *control = handle->g_control_config;

    static uint32_t last_acquire_check_ms = 0U, light_loss_ms = 0U;

    GOTO_ON_ERR(LightSensor_ReadAll(handle->light_sensor, handle->light_readings), err, ret);

    switch (handle->mission_state)
    {
    case MISSION_STATE_SUN_SEARCH:{

        // Rotate at a fixed rate until a light is detected
        taskENTER_CRITICAL();
        control->control_variable = CONTROL_VAR_ANGULAR_VEL;
        control->target_w = CORE_SUNCHASER_ANGULAR_VEL;
        taskEXIT_CRITICAL();
        
        if(light_detected(handle->light_readings, CORE_MIN_LIGHT_THRS)){
            // Transition to the acquire state
            handle->mission_state = MISSION_STATE_SUN_ACQUIRE;
            
            // Hold current position
            taskENTER_CRITICAL();
            control->control_variable   = CONTROL_VAR_ANGLE;
            control->target_angle       = control->control_internal->theta;
            taskEXIT_CRITICAL();
        }
        break;
    }

    case MISSION_STATE_SUN_ACQUIRE:{

        if(!light_detected(handle->light_readings, CORE_MIN_LIGHT_THRS)){
            handle->mission_state = MISSION_STATE_SUN_LOST;
            last_acquire_check_ms = 0U;
            light_loss_ms = HAL_GetTick();
            break;
        }

        float e_light = 
              (handle->light_readings[LIGHT_SENSOR_IDX_RIGHT] - handle->light_readings[LIGHT_SENSOR_IDX_LEFT]) 
            / (handle->light_readings[LIGHT_SENSOR_IDX_RIGHT] + handle->light_readings[LIGHT_SENSOR_IDX_LEFT] + CORE_SUNCHASER_EPSILON);
        
        float delta_theta = CORE_SUNCHASER_K * e_light;
        float new_theta = control->target_angle + delta_theta;

        taskENTER_CRITICAL();
        control->control_variable   = CONTROL_VAR_ANGLE;
        control->target_angle       = wrap_180(new_theta);
        taskEXIT_CRITICAL();
        
        // to prevent reading a large delta t last_acquire_check_ms is set to 0 
        // on a transition to other states and delta t is not calculated
        if(last_acquire_check_ms == 0U){
            last_acquire_check_ms = HAL_GetTick();
            break;
        }

        bool acquired = is_light_acquired(  e_light, 
                                            CORE_SUNCHASER_MIN_DELTA_E, 
                                            HAL_GetTick() - last_acquire_check_ms,
                                            CORE_SUNCHASER_MIN_STABLE_TIME_MS);
        last_acquire_check_ms = HAL_GetTick();
        
        if(acquired){
            Core_OnLightAcquired(handle);
        }


        if(handle->debug_id == DEBUG_PACKET_ID_LIGHT){
            handle->current_debug_packet = (telemetry_packet_debug_t){
                .type       = PACKET_TYPE_DEBUG,
                .debug_id   = DEBUG_PACKET_ID_LIGHT,
                .light_data = {
                    .e_light        = e_light,
                    .delta_theta    = delta_theta,
                    .new_theta      = new_theta,
                }
            };
            memcpy( handle->current_debug_packet.light_data.light_readings, 
                    handle->light_readings, 
                    FLOATSAT_LIGHT_SENSOR_AMOUNT * sizeof(float));
        }
        break;
    }

    case MISSION_STATE_SUN_LOST:{
        uint32_t now = HAL_GetTick();
        if(now < light_loss_ms){
            LOGE(LOG_TAG,"Error: Last light loss was in the future. This is an invalid state!");
            return ERR_INVALID_STATE;
        }


        // Hold position until timeout
        control->target_angle = control->control_internal->theta;

        uint32_t dt = now - light_loss_ms;
        if(dt > CORE_SUNCHASER_LOSS_TIMEOUT_MS){
            handle->mission_state = MISSION_STATE_SUN_SEARCH;
        }
        break;
    }
    
    default:
        break;
    }

    return ERR_OK;
err:
    return ret;

}

static bool is_light_acquired(float e_light, float min_delta, uint32_t dt_ms, uint32_t min_stable_time)
{
    static float e_light_prev = 0.0f;
    static uint32_t stable_time_ms = 0U;

    // Consider the light acquired when the error function reaches a local minimum
    float delta_e = e_light - e_light_prev;

    if(delta_e < min_delta){
        stable_time_ms += dt_ms;
    }else{
        stable_time_ms = 0U;
    }

    if(stable_time_ms > min_stable_time){
        return true;
    }

    return false;
}

static bool light_detected(float S[3], float threshold)
{
    return (S[0] + S[2] + S[2] >= threshold);
}

static floatsat_err_t Core_OnLightAcquired(floatsat_core_t *handle)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }
    // Turn on a LED and send an indicator to telemetry (could be on the slow telemetry packet)

    return ERR_OK;
}

#pragma endregion

#pragma region command_handling

static floatsat_err_t Core_HandleCmd(floatsat_core_t *handle, floatsat_cmd_t *cmd)
{
    if(!handle || !cmd){
        return ERR_INVALID_ARG;
    }
    floatsat_err_t ret = ERR_OK;
    if(!Util_MissionAllowsCommand(handle->current_mission, cmd->cmd_id)){
        LOGE(LOG_TAG,"Command ID 0x%02x is not allowed during mission %u", cmd->cmd_id, handle->current_mission);
        return ERR_INVALID_CMD;
    }

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
            LOGI(LOG_TAG,"(INFO_MISSION) Current mission ID: %u", handle->current_mission);
            break;
        case CMD_ID_MISSION_SET:
            GOTO_ON_ERR(Core_SetNextMission(handle, cmd->mission_args.mission_id), err, ret);
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
            handle->next_photo_is_async = true;
            handle->mission_state = MISSION_STATE_PHOTO_BUSY;
            GOTO_ON_ERR(Core_TakePhoto(handle), err, ret);
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

static void Core_BlinkLED(gpio_info_t *led, uint32_t blink_time_ms)
{
    static uint32_t last_blink_ms = 0U;
    uint32_t now = HAL_GetTick();

    if(now >= last_blink_ms + blink_time_ms){
        HAL_GPIO_TogglePin(led->port, led->gpio_num);
        last_blink_ms = now;
    }
}
