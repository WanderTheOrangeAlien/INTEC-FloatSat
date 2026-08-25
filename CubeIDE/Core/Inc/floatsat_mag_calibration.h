#ifndef FLOATSAT_MAG_CALIBRATION_H_
#define FLOATSAT_MAG_CALIBRATION_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum
{
    FS_MAG_CAL_IDLE = 0,
    FS_MAG_CAL_COLLECTING = 1,
    FS_MAG_CAL_VALID = 2,
    FS_MAG_CAL_ERROR_NOT_ENOUGH_SAMPLES = 3,
    FS_MAG_CAL_ERROR_AXIS_RANGE = 4,
    FS_MAG_CAL_ERROR_SCALE = 5

} FS_MagCalibrationStatus_t;

typedef struct
{
    FS_MagCalibrationStatus_t status;

    uint8_t valid;

    uint32_t sample_count;
    uint32_t start_time_ms;
    uint32_t duration_ms;

    /* Valores extremos capturados */
    float min_x;
    float min_y;
    float min_z;

    float max_x;
    float max_y;
    float max_z;

    /* Offset de hard-iron */
    float bias_x;
    float bias_y;
    float bias_z;

    /* Semiamplitud original de cada eje */
    float half_range_x;
    float half_range_y;
    float half_range_z;

    /* Corrección de escala por eje */
    float scale_x;
    float scale_y;
    float scale_z;

    /*
     * Radio magnético medio después de centrar
     * el elipsoide.
     */
    float average_radius;

    /*
     * Indicador entre 0 y 1.
     *
     * Es la relación entre la menor y la mayor
     * semiamplitud capturada.
     */
    float coverage_quality;

} FS_MagCalibration_t;

/**
 * @brief Reinicia completamente una calibración.
 */
void FS_MagCalibration_Reset(
        FS_MagCalibration_t *calibration);

/**
 * @brief Inicia la captura de máximos y mínimos.
 */
void FS_MagCalibration_Start(
        FS_MagCalibration_t *calibration,
        uint32_t current_time_ms,
        uint32_t duration_ms);

/**
 * @brief Agrega una muestra magnética.
 */
void FS_MagCalibration_Update(
        FS_MagCalibration_t *calibration,
        float mag_x_gauss,
        float mag_y_gauss,
        float mag_z_gauss);

/**
 * @brief Indica si terminó el tiempo de captura.
 */
uint8_t FS_MagCalibration_TimeComplete(
        const FS_MagCalibration_t *calibration,
        uint32_t current_time_ms);

/**
 * @brief Calcula offsets y factores de escala.
 */
FS_MagCalibrationStatus_t FS_MagCalibration_Finish(
        FS_MagCalibration_t *calibration);

/**
 * @brief Aplica la calibración a una muestra.
 *
 * Si no existe una calibración válida, la función
 * devuelve la muestra sin modificar.
 */
void FS_MagCalibration_Apply(
        const FS_MagCalibration_t *calibration,
        float raw_x,
        float raw_y,
        float raw_z,
        float *corrected_x,
        float *corrected_y,
        float *corrected_z);

#ifdef __cplusplus
}
#endif

#endif /* FLOATSAT_MAG_CALIBRATION_H_ */
