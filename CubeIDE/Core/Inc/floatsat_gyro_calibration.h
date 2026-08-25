#ifndef FLOATSAT_GYRO_CALIBRATION_H_
#define FLOATSAT_GYRO_CALIBRATION_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum
{
    FS_GYRO_CAL_IDLE = 0,
    FS_GYRO_CAL_COLLECTING = 1,
    FS_GYRO_CAL_VALID = 2,
    FS_GYRO_CAL_ERROR_TIMEOUT = 3,
    FS_GYRO_CAL_ERROR_INVALID_ARGUMENT = 4

} FS_GyroCalibrationStatus_t;

typedef struct
{
    FS_GyroCalibrationStatus_t status;

    uint8_t valid;

    uint32_t target_samples;
    uint32_t sample_count;

    uint32_t rejected_samples;
    uint32_t restart_count;

    uint32_t start_time_ms;
    uint32_t timeout_ms;

    float sum_x_dps;
    float sum_y_dps;
    float sum_z_dps;

    float bias_x_dps;
    float bias_y_dps;
    float bias_z_dps;

} FS_GyroCalibration_t;

/**
 * @brief Reinicia completamente la calibración.
 */
void FS_GyroCalibration_Reset(
        FS_GyroCalibration_t *calibration);

/**
 * @brief Inicia una calibración nueva.
 */
void FS_GyroCalibration_Start(
        FS_GyroCalibration_t *calibration,
        uint32_t current_time_ms,
        uint32_t target_samples,
        uint32_t timeout_ms);

/**
 * @brief Agrega una muestra.
 *
 * La muestra solamente se acepta cuando:
 * - La aceleración total está cerca de 1 g.
 * - La velocidad angular total es pequeña.
 *
 * Si se detecta movimiento, la ventana de muestras
 * consecutivas se reinicia.
 */
FS_GyroCalibrationStatus_t FS_GyroCalibration_Update(
        FS_GyroCalibration_t *calibration,
        float gyro_x_dps,
        float gyro_y_dps,
        float gyro_z_dps,
        float accel_x_g,
        float accel_y_g,
        float accel_z_g,
        uint32_t current_time_ms);

/**
 * @brief Resta el bias de una muestra del giroscopio.
 *
 * Si todavía no hay calibración válida, devuelve
 * la muestra original.
 */
void FS_GyroCalibration_Apply(
        const FS_GyroCalibration_t *calibration,
        float measured_x_dps,
        float measured_y_dps,
        float measured_z_dps,
        float *corrected_x_dps,
        float *corrected_y_dps,
        float *corrected_z_dps);

#ifdef __cplusplus
}
#endif

#endif /* FLOATSAT_GYRO_CALIBRATION_H_ */
