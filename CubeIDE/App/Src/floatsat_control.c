#include "floatsat_control.h"

void control_task(void *args)
{
    floatsat_control_handle_t *handle =  (floatsat_control_handle_t*)args;
    IMU_data_t imu_data = {0};

    while(1){

        // TODO: Implement Mutex for the I2C peripheral
        IMU_ReadData(handle->imu, &imu_data); // Take IMU measurements

        Madgwick_Update(handle->madgwick, &imu_data); // Estimate orientation

        #warning Pending implementation of Quaternion to Vec3 conversion for orientation data
        portENTER_CRITICAL();
        *(handle->g_orientation) = (Vec3_t){0};
        portEXIT_CRITICAL();

        Control_Update(handle);

        
        xTaskNotify(handle->telemetry_task_handle, 0U, eNoAction);

    }

}

floatsat_err_t Control_Init(floatsat_control_handle_t *handle)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }

    if(!handle->g_orientation || !handle->g_target_angle){
        return ERR_INVALID_ARG;
    }



    return ERR_OK;
}

floatsat_err_t Control_Update(floatsat_control_handle_t *handle)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }


    return ERR_OK;
}