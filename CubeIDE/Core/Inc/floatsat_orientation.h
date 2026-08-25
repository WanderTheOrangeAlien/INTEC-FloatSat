#ifndef FLOATSAT_ORIENTATION_H_
#define FLOATSAT_ORIENTATION_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "floatsat_imu_old.h"
#include <stdint.h>

typedef enum
{
    FS_ORIENTATION_OK = 0,
    FS_ORIENTATION_INVALID_ARGUMENT,
    FS_ORIENTATION_ACCEL_INVALID,
    FS_ORIENTATION_MAG_INVALID

} FS_OrientationStatus_t;

/*
 * Sistema de referencia mundial:
 *
 * X = Este
 * Y = Norte
 * Z = Arriba
 *
 * Este sistema se conoce como ENU.
 */
typedef struct
{
    /*
     * Vectores originales expresados en los ejes
     * físicos de la IMU.
     */
    float accel_body_g[3];
    float gyro_body_dps[3];
    float mag_body_gauss[3];

    /*
     * Magnitud de cada vector.
     */
    float accel_magnitude_g;
    float gyro_magnitude_dps;
    float mag_magnitude_gauss;

    /*
     * Direcciones de referencia mundial expresadas
     * en las coordenadas físicas de la IMU.
     */
    float up_body[3];
    float north_body[3];
    float east_body[3];

    /*
     * Ejes físicos de la IMU expresados en el
     * sistema mundial ENU.
     *
     * Cada vector contiene:
     * [Este, Norte, Arriba]
     */
    float body_x_world[3];
    float body_y_world[3];
    float body_z_world[3];

    /*
     * Ángulos de orientación.
     */
    float roll_deg;
    float pitch_deg;

    /*
     * Rumbo magnético:
     * 0   = Norte
     * 90  = Este
     * 180 = Sur
     * 270 = Oeste
     */
    float yaw_deg;

    /*
     * Ángulo entre el eje Z de la IMU y la vertical.
     *
     * 0 grados   = Z hacia arriba
     * 90 grados  = Z horizontal
     * 180 grados = Z hacia abajo
     */
    float tilt_deg;

    /*
     * Indicadores de calidad.
     */
    uint8_t accel_reliable;
    uint8_t mag_reliable;
    uint8_t valid;

} FS_Orientation_t;

/**
 * @brief Calcula una orientación estática utilizando
 *        acelerómetro y magnetómetro.
 *
 * @param imu_data Datos escalados del LSM9DS1.
 * @param orientation Resultado de orientación.
 *
 * @return Estado del cálculo.
 */
FS_OrientationStatus_t FS_Orientation_Compute(
        const FS_IMU_ScaledData_t *imu_data,
        FS_Orientation_t *orientation);

#ifdef __cplusplus
}
#endif

#endif /* FLOATSAT_ORIENTATION_H_ */
