#ifndef FLOATSAT_CONTROL_H
#define FLOATSAT_CONTROL_H

#include "FreeRTOS.h"
#include "queue.h"

#include "data_bus.h"
#include "LSM9DS1_stm32.h"
#include "madgwick_filter.h"
#include "floatsat_telemetry.h"
#include "floatsat_motor_controller.h"
#include "vec_math.h"

#define CONTROL_LOOP_PERIOD_MS      10
#define MDPS_TO_RADS                (0.01745329f / 1000.0f)
#define RADS_TO_MDPS                (1000.0f / 0.01745329f)
#define CM_TO_M                     (0.01f)


#define CONTROL_MAX_IMU_MISSES      5
#define CONTROL_MAX_DT_S            ((float)CONTROL_MAX_IMU_MISSES * (float)CONTROL_LOOP_PERIOD_MS * 1e-3f)
#define CONTROL_MAX_ANGULAR_VEL     (1.5f * 2.0f * (float)M_PI)     // In rad/s



typedef struct control_handle_t {
    IMU_handle_t        *imu;
    madgwick_filter_t   *madgwick;
    Vec3_t              current_orientation;
    telemetry_handle_t  *telemetry_handle;
    motor_handle_t      *motor;
    TaskHandle_t        task_handle;

    control_config_t    *g_control_config;      // Configuration struct shared with the supervisor

    data_bus_writer_t   *data_bus_writer;
}control_handle_t;

void Task_ControlFn(void *args);
floatsat_err_t Control_Init(control_handle_t *handle);




#endif