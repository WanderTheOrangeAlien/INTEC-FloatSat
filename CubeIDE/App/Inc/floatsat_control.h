#ifndef FLOATSAT_CONTROL_H
#define FLOATSAT_CONTROL_H

#include "FreeRTOS.h"
#include "queue.h"

#include "LSM9DS1_stm32.h"
#include "madgwick_filter.h"

typedef struct floatsat_control_handle_t {
    IMU_handle_t *imu;
    madgwick_filter_t *madgwick;

    TaskHandle_t task_handle;
    TaskHandle_t telemetry_task_handle;

    float *g_target_angle;  // Target angle. The g_ prefix indicates that is a shared resource.
                            // Ensure atomic access

    Vec3_t *g_orientation;  // Shared orientation data so other tasks can access it, for instamce, telemetry

    //  TODO: Add motor drive handle

}floatsat_control_handle_t;

floatsat_err_t Control_Init(floatsat_control_handle_t *handle);


floatsat_err_t Control_Update(floatsat_control_handle_t *handle);



#endif