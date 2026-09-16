#include "floatsat_control.h"

void Task_ControlFn(void *args)
{
    control_handle_t *handle =  (control_handle_t*)args;
    IMU_data_t imu_data = {0};

    handle->task_handle = xTaskGetCurrentTaskHandle();

    TickType_t last_wake_time = xTaskGetTickCount();

    while(1){

        // TODO: Implement Mutex for the I2C peripheral
        IMU_ReadData(handle->imu, &imu_data); // Take IMU measurements

        Madgwick_Update(handle->madgwick, &imu_data); // Estimate orientation

        Control_Update(handle); 
        
        #warning Pending implementation of Quaternion to Vec3 conversion for orientation data
        telemetry_packet_fast_t packet = {
            .orientation    = (Vec3_t) {
                .x = handle->madgwick->SEq.b,
                .y = handle->madgwick->SEq.c,
                .z = handle->madgwick->SEq.d,
            },
            .rw_speed       = 0.0f
        };

        Telemetry_AddFast(handle->telemetry_handle, &packet);
        xTaskNotify(handle->telemetry_handle->task_handle, 0U, eNoAction);
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(CONTROL_LOOP_PERIOD_MS));

    }

}

floatsat_err_t Control_Init(control_handle_t *handle)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }

    if(!handle->telemetry_handle || !handle->imu || !handle->madgwick || !handle->g_control_config){
        return ERR_INVALID_ARG;
    }

    // if(!handle->g_orientation || !handle->g_target_angle){
    //     return ERR_INVALID_ARG;
    // }



    return ERR_OK;
}

floatsat_err_t Control_Update(control_handle_t *handle)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }


    return ERR_OK;
}