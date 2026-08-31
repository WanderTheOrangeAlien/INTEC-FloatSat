#ifndef MADGWICK_FILTER_H
#define MADGWICK_FILTER_H

#include <stdlib.h>
#include <math.h>
#include "floatsat_types.h"
#include "floatsat_error.h"

/* From the original Madgwick filter paper */
// System constants
#define MADGWICK_DELTA_T 0.001f // sampling period in seconds (shown as 1 ms)
#define GYRO_MEAS_ERROR (3.14159265358979 * (5.0f / 180.0f)) // gyroscope measurement error in rad/s (shown as 5 deg/s)
#define GYRO_MEAS_DRIFT (3.14159265358979 * (0.2f / 180.0f)) // gyroscope measurement error in rad/s/s (shown as 0.2f deg/s/s)
#define MADGWICK_BETA (sqrt(3.0f / 4.0f) * GYRO_MEAS_ERROR) // compute beta
#define MADGWICK_ZETA (sqrt(3.0f / 4.0f) * GYRO_MEAS_DRIFT) // compute zeta


typedef struct madgwick_filter_t {
    Quaternion_t SEq;               // Estimated orientation quaternion
    Vec3_t b;                       // Reference direction of flux in Earth frame
    Vec3_t w_b;                     // Gyro bias error

} madgwick_filter_t;

floatsat_err_t Madgwick_Init(madgwick_filter_t *handle);

void Madgwick_Update(madgwick_filter_t *handle, const IMU_data_t *imu);


#endif