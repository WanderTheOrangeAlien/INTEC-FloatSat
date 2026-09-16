#ifndef FLOATSAT_CONTROL_H
#define FLOATSAT_CONTROL_H

#include "FreeRTOS.h"
#include "queue.h"

#include "LSM9DS1_stm32.h"
#include "madgwick_filter.h"
#include "floatsat_telemetry.h"

#define CONTROL_LOOP_PERIOD_MS      10

typedef struct control_handle_t {
    IMU_handle_t *imu;
    madgwick_filter_t *madgwick;
    telemetry_handle_t *telemetry_handle;

    TaskHandle_t task_handle;

    control_config_t *g_control_config; // Configuration struct shared with the supervisor

    Vec3_t *g_orientation;  // Shared orientation data so other tasks can access it, for instamce, telemetry

    //  TODO: Add motor drive handle

}control_handle_t;

floatsat_err_t Control_Init(control_handle_t *handle);


floatsat_err_t Control_Update(control_handle_t *handle);



#endif