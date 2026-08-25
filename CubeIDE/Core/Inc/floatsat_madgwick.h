#ifndef FLOATSAT_MADGWICK_H_
#define FLOATSAT_MADGWICK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum
{
    FS_MADGWICK_OK = 0,
    FS_MADGWICK_INVALID_ARGUMENT = 1,
    FS_MADGWICK_INVALID_CONFIGURATION = 2,
    FS_MADGWICK_INVALID_QUATERNION = 3
} FS_MadgwickStatus_t;

typedef enum
{
    FS_MADGWICK_MODE_GYRO_ONLY = 0,
    FS_MADGWICK_MODE_IMU = 1,
    FS_MADGWICK_MODE_MARG = 2
} FS_MadgwickMode_t;

typedef struct
{
    float q0;
    float q1;
    float q2;
    float q3;

    float beta;
    float sample_period_s;

    uint32_t update_count;
    FS_MadgwickMode_t last_mode;

    uint8_t initialized;
    uint8_t valid;
} FS_Madgwick_t;

typedef struct
{
    float q0;
    float q1;
    float q2;
    float q3;

    float body_x_world[3];
    float body_y_world[3];
    float body_z_world[3];

    float roll_deg;
    float pitch_deg;
    float yaw_deg;
    float tilt_deg;

    FS_MadgwickMode_t mode;
    uint8_t valid;
} FS_MadgwickOutput_t;

void FS_Madgwick_Reset(FS_Madgwick_t *filter,
                        float beta,
                        float sample_period_s);

FS_MadgwickStatus_t FS_Madgwick_InitializeFromBodyAxesENU(
        FS_Madgwick_t *filter,
        const float body_x_world_enu[3],
        const float body_y_world_enu[3],
        const float body_z_world_enu[3]);

FS_MadgwickStatus_t FS_Madgwick_Update(
        FS_Madgwick_t *filter,
        float gyro_x_dps,
        float gyro_y_dps,
        float gyro_z_dps,
        float accel_x_g,
        float accel_y_g,
        float accel_z_g,
        float mag_x_gauss,
        float mag_y_gauss,
        float mag_z_gauss,
        uint8_t accelerometer_valid,
        uint8_t magnetometer_valid);

FS_MadgwickStatus_t FS_Madgwick_GetOutput(
        const FS_Madgwick_t *filter,
        FS_MadgwickOutput_t *output);

#ifdef __cplusplus
}
#endif

#endif
