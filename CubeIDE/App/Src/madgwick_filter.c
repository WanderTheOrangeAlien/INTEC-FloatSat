#include "madgwick_filter.h"

#include <string.h>


/* C implementation of the Madgwick filter taken from the original paper:
An efficient orientation filter for inertial and inertial/magnetic sensor arrays
by Sebastian O.H. Madgwick, 2010.

Modified a little bit to adapt to the project's architecture
*/


floatsat_err_t Madgwick_Init(madgwick_filter_t *handle)
{
    if(!handle){
        return ERR_OK;
    }

    memset(handle,0,sizeof(madgwick_filter_t));

    handle->b.x =1;
    handle->SEq.a = 1;
    return ERR_OK;
}


// Global system variables
// float a_x, a_y, a_z; // accelerometer measurements
// float w_x, w_y, w_z; // gyroscope measurements in rad/s
// float m_x, m_y, m_z; // magnetometer measurements



// float f->SEq.a = 1, f->SEq.b = 0, f->SEq.c = 0, f->SEq.d = 0; // estimated orientation quaternion elements with initial conditions
// float f->b.x = 1, f->b.z = 0; // reference direction of flux in earth frame
// float f->w_b.x = 0, f->w_b.y = 0, f->w_b.z = 0; // estimate gyroscope biases error

void Madgwick_Update(madgwick_filter_t *f, const IMU_data_t *imu)
{
    float a_x = imu->accel.x,   a_y = imu->accel.y,     a_z = imu->accel.z;

    float w_x = imu->gyro.x,    w_y = imu->gyro.y,      w_z = imu->gyro.z;

    float m_x = imu->mag.x,     m_y = imu->mag.y,       m_z = imu->mag.z; 


    // local system variables
    float norm; // vector norm
    float SEqDot_omega_1, SEqDot_omega_2, SEqDot_omega_3, SEqDot_omega_4; // quaternion rate from gyroscopes elements
    float f_1, f_2, f_3, f_4, f_5, f_6; // objective function elements
    float J_11or24, J_12or23, J_13or22, J_14or21, J_32, J_33, // objective function Jacobian elements
    J_41, J_42, J_43, J_44, J_51, J_52, J_53, J_54, J_61, J_62, J_63, J_64; //
    float SEqHatDot_1, SEqHatDot_2, SEqHatDot_3, SEqHatDot_4; // estimated direction of the gyroscope error
    float w_err_x, w_err_y, w_err_z; // estimated direction of the gyroscope error (angular)
    float h_x, h_y, h_z; // computed flux in the earth frame
    // axulirary variables to avoid reapeated calcualtions
    float halfSEq_1 = 0.5f * f->SEq.a;
    float halfSEq_2 = 0.5f * f->SEq.b;
    float halfSEq_3 = 0.5f * f->SEq.c;
    float halfSEq_4 = 0.5f * f->SEq.d;
    float twoSEq_1 = 2.0f * f->SEq.a;
    float twoSEq_2 = 2.0f * f->SEq.b;
    float twoSEq_3 = 2.0f * f->SEq.c;
    float twoSEq_4 = 2.0f * f->SEq.d;
    float twob_x = 2.0f * f->b.x;
    float twob_z = 2.0f * f->b.z;
    float twob_xSEq_1 = 2.0f * f->b.x * f->SEq.a;
    float twob_xSEq_2 = 2.0f * f->b.x * f->SEq.b;
    float twob_xSEq_3 = 2.0f * f->b.x * f->SEq.c;
    float twob_xSEq_4 = 2.0f * f->b.x * f->SEq.d;
    float twob_zSEq_1 = 2.0f * f->b.z * f->SEq.a;
    float twob_zSEq_2 = 2.0f * f->b.z * f->SEq.b;
    float twob_zSEq_3 = 2.0f * f->b.z * f->SEq.c;
    float twob_zSEq_4 = 2.0f * f->b.z * f->SEq.d;
    float SEq_1SEq_2;
    float SEq_1SEq_3 = f->SEq.a * f->SEq.c;
    float SEq_1SEq_4;
    float SEq_2SEq_3;
    float SEq_2SEq_4 = f->SEq.b * f->SEq.d;
    float SEq_3SEq_4;
    float twom_x = 2.0f * m_x;
    float twom_y = 2.0f * m_y;
    float twom_z = 2.0f * m_z;
    // normalise the accelerometer measurement
    norm = sqrt(a_x * a_x + a_y * a_y + a_z * a_z);
    a_x /= norm;
    a_y /= norm;
    a_z /= norm;
    // normalise the magnetometer measurement
    norm = sqrt(m_x * m_x + m_y * m_y + m_z * m_z);
    m_x /= norm;
    m_y /= norm;
    m_z /= norm;
    // compute the objective function and Jacobian
    f_1 = twoSEq_2 * f->SEq.d - twoSEq_1 * f->SEq.c - a_x;
    f_2 = twoSEq_1 * f->SEq.b + twoSEq_3 * f->SEq.d - a_y;
    f_3 = 1.0f - twoSEq_2 * f->SEq.b - twoSEq_3 * f->SEq.c - a_z;
    f_4 = twob_x * (0.5f - f->SEq.c * f->SEq.c - f->SEq.d * f->SEq.d) + twob_z * (SEq_2SEq_4 - SEq_1SEq_3) - m_x;
    f_5 = twob_x * (f->SEq.b * f->SEq.c - f->SEq.a * f->SEq.d) + twob_z * (f->SEq.a * f->SEq.b + f->SEq.c * f->SEq.d) - m_y;
    f_6 = twob_x * (SEq_1SEq_3 + SEq_2SEq_4) + twob_z * (0.5f - f->SEq.b * f->SEq.b - f->SEq.c * f->SEq.c) - m_z;
    J_11or24 = twoSEq_3; // J_11 negated in matrix multiplication
    J_12or23 = 2.0f * f->SEq.d;
    J_13or22 = twoSEq_1; // J_12 negated in matrix multiplication
    J_14or21 = twoSEq_2;
    J_32 = 2.0f * J_14or21; // negated in matrix multiplication
    J_33 = 2.0f * J_11or24; // negated in matrix multiplication
    J_41 = twob_zSEq_3; // negated in matrix multiplication
    J_42 = twob_zSEq_4;
    J_43 = 2.0f * twob_xSEq_3 + twob_zSEq_1; // negated in matrix multiplication
    J_44 = 2.0f * twob_xSEq_4 - twob_zSEq_2; // negated in matrix multiplication
    J_51 = twob_xSEq_4 - twob_zSEq_2; // negated in matrix multiplication
    J_52 = twob_xSEq_3 + twob_zSEq_1;
    J_53 = twob_xSEq_2 + twob_zSEq_4;
    J_54 = twob_xSEq_1 - twob_zSEq_3; // negated in matrix multiplication
    J_61 = twob_xSEq_3;
    J_62 = twob_xSEq_4 - 2.0f * twob_zSEq_2;
    J_63 = twob_xSEq_1 - 2.0f * twob_zSEq_3;
    J_64 = twob_xSEq_2;
    // compute the gradient (matrix multiplication)
    SEqHatDot_1 = J_14or21 * f_2 - J_11or24 * f_1 - J_41 * f_4 - J_51 * f_5 + J_61 * f_6;
    SEqHatDot_2 = J_12or23 * f_1 + J_13or22 * f_2 - J_32 * f_3 + J_42 * f_4 + J_52 * f_5 + J_62 * f_6;
    SEqHatDot_3 = J_12or23 * f_2 - J_33 * f_3 - J_13or22 * f_1 - J_43 * f_4 + J_53 * f_5 + J_63 * f_6;
    SEqHatDot_4 = J_14or21 * f_1 + J_11or24 * f_2 - J_44 * f_4 - J_54 * f_5 + J_64 * f_6;
    // normalise the gradient to estimate direction of the gyroscope error
    norm = sqrt(SEqHatDot_1 * SEqHatDot_1 + SEqHatDot_2 * SEqHatDot_2 + SEqHatDot_3 * SEqHatDot_3 + SEqHatDot_4 * SEqHatDot_4);
    SEqHatDot_1 = SEqHatDot_1 / norm;
    SEqHatDot_2 = SEqHatDot_2 / norm;

    SEqHatDot_3 = SEqHatDot_3 / norm;
    SEqHatDot_4 = SEqHatDot_4 / norm;
    // compute angular estimated direction of the gyroscope error
    w_err_x = twoSEq_1 * SEqHatDot_2 - twoSEq_2 * SEqHatDot_1 - twoSEq_3 * SEqHatDot_4 + twoSEq_4 * SEqHatDot_3;
    w_err_y = twoSEq_1 * SEqHatDot_3 + twoSEq_2 * SEqHatDot_4 - twoSEq_3 * SEqHatDot_1 - twoSEq_4 * SEqHatDot_2;
    w_err_z = twoSEq_1 * SEqHatDot_4 - twoSEq_2 * SEqHatDot_3 + twoSEq_3 * SEqHatDot_2 - twoSEq_4 * SEqHatDot_1;
    // compute and remove the gyroscope baises
    f->w_b.x += w_err_x * MADGWICK_DELTA_T * MADGWICK_ZETA;
    f->w_b.y += w_err_y * MADGWICK_DELTA_T * MADGWICK_ZETA;
    f->w_b.z += w_err_z * MADGWICK_DELTA_T * MADGWICK_ZETA;
    w_x -= f->w_b.x;
    w_y -= f->w_b.y;
    w_z -= f->w_b.z;
    // compute the quaternion rate measured by gyroscopes
    SEqDot_omega_1 = -halfSEq_2 * w_x - halfSEq_3 * w_y - halfSEq_4 * w_z;
    SEqDot_omega_2 = halfSEq_1 * w_x + halfSEq_3 * w_z - halfSEq_4 * w_y;
    SEqDot_omega_3 = halfSEq_1 * w_y - halfSEq_2 * w_z + halfSEq_4 * w_x;
    SEqDot_omega_4 = halfSEq_1 * w_z + halfSEq_2 * w_y - halfSEq_3 * w_x;
    // compute then integrate the estimated quaternion rate
    f->SEq.a += (SEqDot_omega_1 - (MADGWICK_BETA * SEqHatDot_1)) * MADGWICK_DELTA_T;
    f->SEq.b += (SEqDot_omega_2 - (MADGWICK_BETA * SEqHatDot_2)) * MADGWICK_DELTA_T;
    f->SEq.c += (SEqDot_omega_3 - (MADGWICK_BETA * SEqHatDot_3)) * MADGWICK_DELTA_T;
    f->SEq.d += (SEqDot_omega_4 - (MADGWICK_BETA * SEqHatDot_4)) * MADGWICK_DELTA_T;
    // normalise quaternion
    norm = sqrt(f->SEq.a * f->SEq.a + f->SEq.b * f->SEq.b + f->SEq.c * f->SEq.c + f->SEq.d * f->SEq.d);
    f->SEq.a /= norm;
    f->SEq.b /= norm;
    f->SEq.c /= norm;
    f->SEq.d /= norm;
    // compute flux in the earth frame
    SEq_1SEq_2 = f->SEq.a * f->SEq.b; // recompute axulirary variables
    SEq_1SEq_3 = f->SEq.a * f->SEq.c;
    SEq_1SEq_4 = f->SEq.a * f->SEq.d;
    SEq_3SEq_4 = f->SEq.c * f->SEq.d;
    SEq_2SEq_3 = f->SEq.b * f->SEq.c;
    SEq_2SEq_4 = f->SEq.b * f->SEq.d;
    h_x = twom_x * (0.5f - f->SEq.c * f->SEq.c - f->SEq.d * f->SEq.d) + twom_y * (SEq_2SEq_3 - SEq_1SEq_4) + twom_z * (SEq_2SEq_4 + SEq_1SEq_3);
    h_y = twom_x * (SEq_2SEq_3 + SEq_1SEq_4) + twom_y * (0.5f - f->SEq.b * f->SEq.b - f->SEq.d * f->SEq.d) + twom_z * (SEq_3SEq_4 - SEq_1SEq_2);
    h_z = twom_x * (SEq_2SEq_4 - SEq_1SEq_3) + twom_y * (SEq_3SEq_4 + SEq_1SEq_2) + twom_z * (0.5f - f->SEq.b * f->SEq.b - f->SEq.c * f->SEq.c);
    // normalise the flux vector to have only components in the x and z
    f->b.x = sqrt((h_x * h_x) + (h_y * h_y));
    f->b.z = h_z;

}