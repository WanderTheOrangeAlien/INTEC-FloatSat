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

// GCC black magic to avoid -Wfloat-equal errors
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wswitch-default"

#include "Fusion.h"
#pragma GCC diagnostic pop

#define CONTROL_LOOP_PERIOD_MS      10
#define MDPS_TO_RADS                (0.01745329f / 1000.0f)
#define RADS_TO_MDPS                (1000.0f / 0.01745329f)

#define RADS_TO_DEG                 (180.0f / (float)M_PI)
#define CM_TO_M                     (0.01f)
#define G_TO_MS2                    (9.80665f)


#define CONTROL_MAX_IMU_MISSES          5
#define CONTROL_MAX_DT_S                ((float)CONTROL_MAX_IMU_MISSES * (float)CONTROL_LOOP_PERIOD_MS * 1e-3f)
#define CONTROL_MAX_ANGULAR_VEL         (360.0f)     // In deg/s
#define CONTROL_MAX_ANGULAR_VEL_HYST    50.0f
#define CONTROL_MIN_ANGULAR_VEL         (0.5f) // Angular velocity to consider stationary in deg/s
#define CONTROL_STATIONARY_TIMESTEPS    20U

#define CONTROL_MIN_RW_SPEED            (0.05f) // In rev/s

#define CONTROL_MAX_PWM                 0.65f
#define CONTROL_PWM_SAT_THRS            0.05f
#define CONTROL_MAX_INTEGRAL            // TODO: Measure the integral to determine a proper max value        
#define CONTROL_MAX_SAT_TIME_MS         (10U * 1000U)            


// float theta, e_theta, u_theta;
// float w, w_ref, e_w;
// float u_P, u, u_sat;
// static float u_I = 0.0f, u_I_prev = 0.0f;

typedef struct control_vars_t {
    float theta;
    float e_theta;
    float u_theta;
    
    float w;
    float w_ref;
    float e_w;

    float u_P;
    float u;
    float u_sat;

    float u_I;
    float u_I_prev;
}control_vars_t;

typedef struct control_handle_t {
    IMU_handle_t        *imu;
    madgwick_filter_t   *madgwick;
    Vec3_t              current_orientation;
    telemetry_handle_t  *telemetry_handle;
    motor_handle_t      *motor;
    TaskHandle_t        task_handle;
    const TaskHandle_t  *p_core_task;

    control_config_t    *g_control_config;      // Configuration struct shared with the supervisor
    control_vars_t      *control_vars;

    data_bus_writer_t   *data_bus_writer;

#ifdef FUSION_H
    FusionAhrs          *fusion_ahrs;
    FusionBias          *fusion_bias;
    const FusionAhrsSettings  *fusion_ahrs_settings;
    const FusionBiasSettings    *fusion_bias_settings;
#endif

}control_handle_t;

void Task_ControlFn(void *args);
floatsat_err_t Control_Init(control_handle_t *handle);




#endif