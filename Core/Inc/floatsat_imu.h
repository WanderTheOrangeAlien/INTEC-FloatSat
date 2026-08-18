#ifndef FLOATSAT_IMU_H_
#define FLOATSAT_IMU_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include <stdint.h>

/* Registro de identificación */
#define FS_IMU_REG_WHO_AM_I              0x0FU

/* Identificadores esperados */
#define FS_IMU_AG_WHO_AM_I_EXPECTED      0x68U
#define FS_IMU_MAG_WHO_AM_I_EXPECTED     0x3DU

/* Direcciones I2C posibles, formato de 7 bits */
#define FS_IMU_AG_ADDRESS_LOW            0x6AU
#define FS_IMU_AG_ADDRESS_HIGH           0x6BU

#define FS_IMU_MAG_ADDRESS_LOW           0x1CU
#define FS_IMU_MAG_ADDRESS_HIGH          0x1EU

typedef enum
{
    FS_IMU_STATUS_OK = 0,
    FS_IMU_STATUS_INVALID_ARGUMENT,
    FS_IMU_STATUS_AG_NOT_FOUND,
    FS_IMU_STATUS_MAG_NOT_FOUND,
    FS_IMU_STATUS_BOTH_NOT_FOUND,
    FS_IMU_STATUS_CONFIGURATION_ERROR,
    FS_IMU_STATUS_READ_ERROR,
    FS_IMU_STATUS_NOT_INITIALIZED

} FS_IMU_Status_t;

/*
 * Resultado de la detección.
 * Las direcciones se almacenan en formato de 7 bits.
 */
typedef struct
{
    uint8_t ag_detected;
    uint8_t mag_detected;

    uint8_t ag_address_7bit;
    uint8_t mag_address_7bit;

    uint8_t ag_who_am_i;
    uint8_t mag_who_am_i;

} FS_IMU_Info_t;

/*
 * Mediciones crudas de 16 bits entregadas por el sensor.
 */
typedef struct
{
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;

    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;

    int16_t mag_x;
    int16_t mag_y;
    int16_t mag_z;

} FS_IMU_RawData_t;

/*
 * Mediciones convertidas a unidades físicas.
 *
 * Acelerómetro: g
 * Giroscopio:   grados por segundo
 * Magnetómetro: gauss
 */
typedef struct
{
    float accel_x_g;
    float accel_y_g;
    float accel_z_g;

    float gyro_x_dps;
    float gyro_y_dps;
    float gyro_z_dps;

    float mag_x_gauss;
    float mag_y_gauss;
    float mag_z_gauss;

} FS_IMU_ScaledData_t;

/*
 * Muestra completa.
 */
typedef struct
{
    FS_IMU_RawData_t raw;
    FS_IMU_ScaledData_t scaled;

    uint32_t timestamp_ms;
    uint8_t valid;

} FS_IMU_Sample_t;

/**
 * @brief Busca las dos secciones internas del LSM9DS1.
 */
FS_IMU_Status_t FS_IMU_Detect(I2C_HandleTypeDef *hi2c,
                              FS_IMU_Info_t *info);

/**
 * @brief Detecta y configura acelerómetro, giroscopio y magnetómetro.
 */
FS_IMU_Status_t FS_IMU_Init(I2C_HandleTypeDef *hi2c,
                            FS_IMU_Info_t *info);

/**
 * @brief Lee una muestra de los nueve ejes.
 */
FS_IMU_Status_t FS_IMU_ReadSample(FS_IMU_Sample_t *sample);

/**
 * @brief Indica si el driver fue inicializado correctamente.
 */
uint8_t FS_IMU_IsInitialized(void);

#ifdef __cplusplus
}
#endif

#endif /* FLOATSAT_IMU_H_ */
