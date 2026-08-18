/*
 * Driver de comunicación y lectura para la IMU LSM9DS1.
 *
 * El LSM9DS1 está compuesto internamente por dos dispositivos I2C:
 *
 *   1. Bloque acelerómetro + giroscopio.
 *   2. Bloque magnetómetro.
 *
 * Cada bloque tiene:
 *
 *   - Su propia dirección I2C.
 *   - Su propio registro WHO_AM_I.
 *   - Sus propios registros de configuración.
 *   - Sus propios registros de salida.
 *
 * Este archivo realiza:
 *
 *   - Detección automática de ambas direcciones I2C.
 *   - Verificación mediante WHO_AM_I.
 *   - Configuración del acelerómetro, giroscopio y magnetómetro.
 *   - Lectura de los nueve ejes.
 *   - Conversión de valores raw a unidades físicas.
 */

#include "floatsat_imu.h"


/* -------------------------------------------------------------------------- */
/* Configuración I2C                                                          */
/* -------------------------------------------------------------------------- */

/*
 * Tiempo máximo esperado por HAL_I2C_IsDeviceReady() en cada intento.
 *
 * Se utiliza solamente durante la detección inicial del sensor.
 */
#define FS_IMU_I2C_READY_TIMEOUT_MS      20U

/*
 * Tiempo máximo permitido para una lectura o escritura de registros.
 *
 * Se utiliza en:
 *
 *   HAL_I2C_Mem_Write()
 *   HAL_I2C_Mem_Read()
 */
#define FS_IMU_I2C_ACCESS_TIMEOUT_MS     100U

/*
 * Cantidad de intentos realizados para comprobar si una dirección
 * responde en el bus I2C.
 */
#define FS_IMU_I2C_READY_TRIALS          2U

/*
 * En las lecturas múltiples del LSM9DS1, el bit más significativo
 * de la subdirección habilita el incremento automático.
 *
 * Esto permite leer varios registros consecutivos en una sola
 * transacción I2C.
 *
 * Ejemplo:
 *
 *   Registro inicial: 0x18
 *   Dirección enviada: 0x18 | 0x80 = 0x98
 */
#define FS_IMU_AUTO_INCREMENT            0x80U


/* -------------------------------------------------------------------------- */
/* Registros del bloque acelerómetro / giroscopio                             */
/* -------------------------------------------------------------------------- */

/*
 * CTRL_REG4:
 * habilita los ejes X, Y y Z del giroscopio.
 */
#define FS_IMU_REG_CTRL_REG4             0x1EU

/*
 * CTRL_REG5_XL:
 * habilita los ejes X, Y y Z del acelerómetro.
 */
#define FS_IMU_REG_CTRL_REG5_XL          0x1FU

/*
 * CTRL_REG1_G:
 * configura frecuencia de muestreo, escala y ancho de banda
 * del giroscopio.
 */
#define FS_IMU_REG_CTRL_REG1_G           0x10U

/*
 * CTRL_REG6_XL:
 * configura frecuencia de muestreo y escala del acelerómetro.
 */
#define FS_IMU_REG_CTRL_REG6_XL          0x20U

/*
 * CTRL_REG8:
 * configura funciones generales del bloque AG, como BDU
 * e incremento automático.
 */
#define FS_IMU_REG_CTRL_REG8             0x22U

/*
 * Primer registro de salida del giroscopio.
 *
 * A partir de esta dirección se encuentran:
 *
 *   OUT_X_L_G
 *   OUT_X_H_G
 *   OUT_Y_L_G
 *   OUT_Y_H_G
 *   OUT_Z_L_G
 *   OUT_Z_H_G
 */
#define FS_IMU_REG_OUT_X_L_G             0x18U

/*
 * Primer registro de salida del acelerómetro.
 *
 * A partir de esta dirección se encuentran los seis bytes
 * correspondientes a X, Y y Z.
 */
#define FS_IMU_REG_OUT_X_L_XL            0x28U


/* -------------------------------------------------------------------------- */
/* Registros del magnetómetro                                                 */
/* -------------------------------------------------------------------------- */

/*
 * CTRL_REG1_M:
 * frecuencia de salida y rendimiento de los ejes X/Y.
 */
#define FS_IMU_REG_CTRL_REG1_M           0x20U

/*
 * CTRL_REG2_M:
 * escala de medición del campo magnético.
 */
#define FS_IMU_REG_CTRL_REG2_M           0x21U

/*
 * CTRL_REG3_M:
 * modo de operación del magnetómetro.
 */
#define FS_IMU_REG_CTRL_REG3_M           0x22U

/*
 * CTRL_REG4_M:
 * rendimiento del eje Z.
 */
#define FS_IMU_REG_CTRL_REG4_M           0x23U

/*
 * CTRL_REG5_M:
 * configuración de Block Data Update.
 */
#define FS_IMU_REG_CTRL_REG5_M           0x24U

/*
 * Primer registro de salida del magnetómetro.
 *
 * Desde esta dirección se leen seis bytes:
 *
 *   X bajo, X alto
 *   Y bajo, Y alto
 *   Z bajo, Z alto
 */
#define FS_IMU_REG_OUT_X_L_M             0x28U


/* -------------------------------------------------------------------------- */
/* Valores de configuración                                                   */
/* -------------------------------------------------------------------------- */

/*
 * Configuración del giroscopio:
 *
 *   ODR = 119 Hz
 *   Escala = ±245 dps
 *   BW = configuración básica
 *
 * El valor completo que será escrito en CTRL_REG1_G es 0x60.
 */
#define FS_IMU_CTRL_REG1_G_VALUE         0x60U

/*
 * Configuración del acelerómetro:
 *
 *   ODR = 119 Hz
 *   Escala = ±2 g
 */
#define FS_IMU_CTRL_REG6_XL_VALUE        0x60U

/*
 * Habilitar los tres ejes del giroscopio:
 *
 *   Bit X = 1
 *   Bit Y = 1
 *   Bit Z = 1
 */
#define FS_IMU_CTRL_REG4_VALUE           0x38U

/*
 * Habilitar los tres ejes del acelerómetro.
 */
#define FS_IMU_CTRL_REG5_XL_VALUE        0x38U

/*
 * Configuración general del bloque acelerómetro/giroscopio:
 *
 *   BDU habilitado.
 *   Incremento automático de direcciones habilitado.
 *
 * BDU evita que los bytes de una medición cambien mientras
 * se están leyendo.
 */
#define FS_IMU_CTRL_REG8_VALUE           0x44U

/*
 * Configuración del magnetómetro:
 *
 *   Rendimiento alto en X/Y.
 *   ODR = 80 Hz.
 */
#define FS_IMU_CTRL_REG1_M_VALUE         0x7CU

/*
 * Escala del magnetómetro:
 *
 *   ±4 gauss.
 */
#define FS_IMU_CTRL_REG2_M_VALUE         0x00U

/*
 * Modo de conversión continua.
 *
 * El magnetómetro genera mediciones continuamente sin necesidad
 * de iniciar cada conversión manualmente.
 */
#define FS_IMU_CTRL_REG3_M_VALUE         0x00U

/*
 * Rendimiento alto para el eje Z del magnetómetro.
 */
#define FS_IMU_CTRL_REG4_M_VALUE         0x0CU

/*
 * Block Data Update habilitado en el magnetómetro.
 */
#define FS_IMU_CTRL_REG5_M_VALUE         0x40U


/* -------------------------------------------------------------------------- */
/* Sensibilidades                                                             */
/* -------------------------------------------------------------------------- */

/*
 * Sensibilidad del acelerómetro con escala ±2 g:
 *
 *   0.061 mg/LSB
 *   0.000061 g/LSB
 *
 * Conversión:
 *
 *   aceleración_g = valor_raw × 0.000061
 */
#define FS_IMU_ACCEL_SENSITIVITY_G       0.000061f

/*
 * Sensibilidad del giroscopio con escala ±245 dps:
 *
 *   8.75 mdps/LSB
 *   0.00875 dps/LSB
 *
 * Conversión:
 *
 *   velocidad_dps = valor_raw × 0.00875
 */
#define FS_IMU_GYRO_SENSITIVITY_DPS      0.00875f

/*
 * Sensibilidad del magnetómetro con escala ±4 gauss:
 *
 *   0.14 mgauss/LSB
 *   0.00014 gauss/LSB
 *
 * Conversión:
 *
 *   campo_gauss = valor_raw × 0.00014
 */
#define FS_IMU_MAG_SENSITIVITY_GAUSS     0.00014f


/* -------------------------------------------------------------------------- */
/* Variables internas                                                         */
/* -------------------------------------------------------------------------- */

/*
 * Puntero al periférico I2C utilizado por la IMU.
 *
 * Será, por ejemplo:
 *
 *   &hi2c1
 *
 * Se guarda durante FS_IMU_Init() para no tener que pasarlo en cada lectura.
 *
 * static significa que solamente puede utilizarse dentro de este archivo.
 */
static I2C_HandleTypeDef *fs_imu_i2c = NULL;

/*
 * Información detectada de la IMU:
 *
 *   - Dirección del bloque acelerómetro/giroscopio.
 *   - Dirección del magnetómetro.
 *   - Valores WHO_AM_I.
 *   - Banderas de detección.
 */
static FS_IMU_Info_t fs_imu_info = {0};

/*
 * Bandera que indica si:
 *
 *   1. Los dos dispositivos fueron detectados.
 *   2. El bloque AG fue configurado.
 *   3. El magnetómetro fue configurado.
 *
 * Valor:
 *
 *   0U = no inicializada.
 *   1U = inicializada correctamente.
 */
static uint8_t fs_imu_initialized = 0U;

/*
 * Posibles direcciones I2C de 7 bits para el bloque
 * acelerómetro/giroscopio.
 *
 * La dirección real depende del estado del pin de selección
 * correspondiente del LSM9DS1.
 */
static const uint8_t fs_ag_addresses[] =
{
    FS_IMU_AG_ADDRESS_LOW,
    FS_IMU_AG_ADDRESS_HIGH
};

/*
 * Posibles direcciones I2C de 7 bits para el magnetómetro.
 */
static const uint8_t fs_mag_addresses[] =
{
    FS_IMU_MAG_ADDRESS_LOW,
    FS_IMU_MAG_ADDRESS_HIGH
};


/* -------------------------------------------------------------------------- */
/* Funciones privadas                                                         */
/* -------------------------------------------------------------------------- */

/**
 * @brief Convierte una dirección I2C de 7 bits al formato esperado por HAL.
 *
 * Las constantes del driver almacenan las direcciones en formato de 7 bits.
 * Las funciones HAL de STM32 esperan que esa dirección esté desplazada
 * una posición hacia la izquierda.
 *
 * Ejemplo:
 *
 *   Dirección de 7 bits: 0x6B
 *   Dirección para HAL:  0xD6
 *
 * @param address_7bit Dirección I2C de 7 bits.
 *
 * @return Dirección desplazada para utilizar con HAL.
 */
static uint16_t FS_IMU_HALAddress(uint8_t address_7bit)
{
    return (uint16_t)((uint16_t)address_7bit << 1U);
}


/**
 * @brief Escribe un byte en un registro del LSM9DS1.
 *
 * @param hi2c             Periférico I2C utilizado.
 * @param address_7bit     Dirección del dispositivo.
 * @param register_address Registro que se desea escribir.
 * @param value            Valor que se escribirá.
 *
 * @return HAL_OK si la operación fue correcta.
 */
static HAL_StatusTypeDef FS_IMU_WriteRegister(
        I2C_HandleTypeDef *hi2c,
        uint8_t address_7bit,
        uint8_t register_address,
        uint8_t value)
{
    if (hi2c == NULL)                         /* Verificar puntero I2C. */
    {
        return HAL_ERROR;
    }

    return HAL_I2C_Mem_Write(
            hi2c,                             /* Periférico I2C. */
            FS_IMU_HALAddress(address_7bit),  /* Dirección para HAL. */
            register_address,                /* Registro destino. */
            I2C_MEMADD_SIZE_8BIT,             /* Dirección de registro de 8 bits. */
            &value,                           /* Byte que será enviado. */
            1U,                               /* Cantidad de bytes. */
            FS_IMU_I2C_ACCESS_TIMEOUT_MS);    /* Timeout de la operación. */
}


/**
 * @brief Lee un único byte desde un registro del LSM9DS1.
 *
 * @param hi2c             Periférico I2C.
 * @param address_7bit     Dirección del dispositivo.
 * @param register_address Registro que se desea leer.
 * @param value            Puntero donde se guardará el byte leído.
 */
static HAL_StatusTypeDef FS_IMU_ReadRegister(
        I2C_HandleTypeDef *hi2c,
        uint8_t address_7bit,
        uint8_t register_address,
        uint8_t *value)
{
    /*
     * Tanto el periférico como el puntero de salida deben ser válidos.
     */
    if ((hi2c == NULL) || (value == NULL))
    {
        return HAL_ERROR;
    }

    return HAL_I2C_Mem_Read(
            hi2c,                             /* Periférico I2C. */
            FS_IMU_HALAddress(address_7bit),  /* Dirección desplazada. */
            register_address,                /* Registro origen. */
            I2C_MEMADD_SIZE_8BIT,             /* Subdirección de 8 bits. */
            value,                            /* Destino de la lectura. */
            1U,                               /* Leer un byte. */
            FS_IMU_I2C_ACCESS_TIMEOUT_MS);    /* Timeout. */
}


/**
 * @brief Lee varios registros consecutivos.
 *
 * El bit FS_IMU_AUTO_INCREMENT permite que el LSM9DS1 incremente
 * automáticamente la dirección después de cada byte leído.
 *
 * @param hi2c           Periférico I2C.
 * @param address_7bit   Dirección del dispositivo.
 * @param first_register Primer registro que se leerá.
 * @param data           Arreglo donde se guardarán los datos.
 * @param length         Cantidad de bytes que se leerán.
 */
static HAL_StatusTypeDef FS_IMU_ReadRegisters(
        I2C_HandleTypeDef *hi2c,
        uint8_t address_7bit,
        uint8_t first_register,
        uint8_t *data,
        uint16_t length)
{
    uint8_t register_address;

    /*
     * No se permite:
     *
     *   - Periférico I2C nulo.
     *   - Buffer de salida nulo.
     *   - Lectura de cero bytes.
     */
    if ((hi2c == NULL) ||
        (data == NULL) ||
        (length == 0U))
    {
        return HAL_ERROR;
    }

    /*
     * Activar el incremento automático en la subdirección.
     */
    register_address =
        (uint8_t)(first_register | FS_IMU_AUTO_INCREMENT);

    return HAL_I2C_Mem_Read(
            hi2c,
            FS_IMU_HALAddress(address_7bit),
            register_address,
            I2C_MEMADD_SIZE_8BIT,
            data,
            length,
            FS_IMU_I2C_ACCESS_TIMEOUT_MS);
}


/**
 * @brief Busca un dispositivo en una lista de posibles direcciones I2C.
 *
 * La búsqueda se realiza en dos pasos:
 *
 *   1. Comprobar que exista un dispositivo en la dirección.
 *   2. Leer WHO_AM_I y compararlo con el valor esperado.
 *
 * Esto evita aceptar por error otro dispositivo que responda
 * en la misma dirección.
 *
 * @return
 *   1U si se encontró el dispositivo correcto.
 *   0U si no fue encontrado.
 */
static uint8_t FS_IMU_FindDevice(
        I2C_HandleTypeDef *hi2c,
        const uint8_t *addresses,
        uint32_t address_count,
        uint8_t expected_who_am_i,
        uint8_t *detected_address,
        uint8_t *detected_who_am_i)
{
    uint32_t index;                 /* Índice de la dirección que se prueba. */
    uint8_t who_am_i;               /* Valor leído desde WHO_AM_I. */
    HAL_StatusTypeDef status;       /* Resultado de las funciones HAL. */

    /*
     * Todos los punteros son obligatorios.
     */
    if ((hi2c == NULL) ||
        (addresses == NULL) ||
        (detected_address == NULL) ||
        (detected_who_am_i == NULL))
    {
        return 0U;
    }

    /*
     * Limpiar los resultados antes de comenzar la búsqueda.
     */
    *detected_address = 0U;
    *detected_who_am_i = 0U;

    /*
     * Recorrer todas las posibles direcciones del dispositivo.
     */
    for (index = 0U; index < address_count; index++)
    {
        /*
         * Comprobar si algún dispositivo responde en esta dirección.
         */
        status = HAL_I2C_IsDeviceReady(
                hi2c,
                FS_IMU_HALAddress(addresses[index]),
                FS_IMU_I2C_READY_TRIALS,
                FS_IMU_I2C_READY_TIMEOUT_MS);

        if (status != HAL_OK)
        {
            /*
             * No respondió en esta dirección.
             * Continuar con la siguiente.
             */
            continue;
        }

        who_am_i = 0U;

        /*
         * El dispositivo respondió. Ahora leer WHO_AM_I.
         */
        status = FS_IMU_ReadRegister(
                hi2c,
                addresses[index],
                FS_IMU_REG_WHO_AM_I,
                &who_am_i);

        if (status != HAL_OK)
        {
            /*
             * Respondió al reconocimiento I2C, pero no fue posible
             * leer correctamente el registro.
             */
            continue;
        }

        /*
         * Verificar que realmente sea el dispositivo esperado.
         */
        if (who_am_i == expected_who_am_i)
        {
            *detected_address = addresses[index];
            *detected_who_am_i = who_am_i;

            return 1U;
        }
    }

    /*
     * Ninguna dirección produjo el WHO_AM_I esperado.
     */
    return 0U;
}


/**
 * @brief Combina los bytes bajo y alto en un entero con signo de 16 bits.
 *
 * Los datos del LSM9DS1 están organizados en formato little-endian:
 *
 *   byte bajo primero;
 *   byte alto después.
 *
 * Ejemplo:
 *
 *   low_byte  = 0x34
 *   high_byte = 0x12
 *
 *   resultado = 0x1234
 *
 * El resultado se convierte a int16_t porque las mediciones pueden
 * ser positivas o negativas.
 */
static int16_t FS_IMU_JoinBytes(uint8_t low_byte,
                               uint8_t high_byte)
{
    uint16_t unsigned_value;

    unsigned_value =
        ((uint16_t)high_byte << 8U) |     /* Colocar byte alto. */
        (uint16_t)low_byte;               /* Agregar byte bajo. */

    return (int16_t)unsigned_value;
}


/**
 * @brief Configura el bloque acelerómetro/giroscopio.
 *
 * Cada registro se escribe individualmente.
 *
 * Ante el primer error de comunicación, la función termina y devuelve:
 *
 *   FS_IMU_STATUS_CONFIGURATION_ERROR
 */
static FS_IMU_Status_t FS_IMU_ConfigureAG(void)
{
    HAL_StatusTypeDef status;

    /*
     * Configuración general:
     *
     *   - Block Data Update.
     *   - Incremento automático.
     */
    status = FS_IMU_WriteRegister(
            fs_imu_i2c,
            fs_imu_info.ag_address_7bit,
            FS_IMU_REG_CTRL_REG8,
            FS_IMU_CTRL_REG8_VALUE);

    if (status != HAL_OK)
    {
        return FS_IMU_STATUS_CONFIGURATION_ERROR;
    }

    /*
     * Habilitar X, Y y Z del giroscopio.
     */
    status = FS_IMU_WriteRegister(
            fs_imu_i2c,
            fs_imu_info.ag_address_7bit,
            FS_IMU_REG_CTRL_REG4,
            FS_IMU_CTRL_REG4_VALUE);

    if (status != HAL_OK)
    {
        return FS_IMU_STATUS_CONFIGURATION_ERROR;
    }

    /*
     * Habilitar X, Y y Z del acelerómetro.
     */
    status = FS_IMU_WriteRegister(
            fs_imu_i2c,
            fs_imu_info.ag_address_7bit,
            FS_IMU_REG_CTRL_REG5_XL,
            FS_IMU_CTRL_REG5_XL_VALUE);

    if (status != HAL_OK)
    {
        return FS_IMU_STATUS_CONFIGURATION_ERROR;
    }

    /*
     * Configurar giroscopio:
     *
     *   119 Hz
     *   ±245 dps
     */
    status = FS_IMU_WriteRegister(
            fs_imu_i2c,
            fs_imu_info.ag_address_7bit,
            FS_IMU_REG_CTRL_REG1_G,
            FS_IMU_CTRL_REG1_G_VALUE);

    if (status != HAL_OK)
    {
        return FS_IMU_STATUS_CONFIGURATION_ERROR;
    }

    /*
     * Configurar acelerómetro:
     *
     *   119 Hz
     *   ±2 g
     */
    status = FS_IMU_WriteRegister(
            fs_imu_i2c,
            fs_imu_info.ag_address_7bit,
            FS_IMU_REG_CTRL_REG6_XL,
            FS_IMU_CTRL_REG6_XL_VALUE);

    if (status != HAL_OK)
    {
        return FS_IMU_STATUS_CONFIGURATION_ERROR;
    }

    return FS_IMU_STATUS_OK;
}


/**
 * @brief Configura el bloque magnetómetro.
 */
static FS_IMU_Status_t FS_IMU_ConfigureMag(void)
{
    HAL_StatusTypeDef status;

    /*
     * Configurar:
     *
     *   - Rendimiento alto X/Y.
     *   - ODR de 80 Hz.
     */
    status = FS_IMU_WriteRegister(
            fs_imu_i2c,
            fs_imu_info.mag_address_7bit,
            FS_IMU_REG_CTRL_REG1_M,
            FS_IMU_CTRL_REG1_M_VALUE);

    if (status != HAL_OK)
    {
        return FS_IMU_STATUS_CONFIGURATION_ERROR;
    }

    /*
     * Configurar escala ±4 gauss.
     */
    status = FS_IMU_WriteRegister(
            fs_imu_i2c,
            fs_imu_info.mag_address_7bit,
            FS_IMU_REG_CTRL_REG2_M,
            FS_IMU_CTRL_REG2_M_VALUE);

    if (status != HAL_OK)
    {
        return FS_IMU_STATUS_CONFIGURATION_ERROR;
    }

    /*
     * Configurar modo de conversión continua.
     */
    status = FS_IMU_WriteRegister(
            fs_imu_i2c,
            fs_imu_info.mag_address_7bit,
            FS_IMU_REG_CTRL_REG3_M,
            FS_IMU_CTRL_REG3_M_VALUE);

    if (status != HAL_OK)
    {
        return FS_IMU_STATUS_CONFIGURATION_ERROR;
    }

    /*
     * Configurar alto rendimiento para el eje Z.
     */
    status = FS_IMU_WriteRegister(
            fs_imu_i2c,
            fs_imu_info.mag_address_7bit,
            FS_IMU_REG_CTRL_REG4_M,
            FS_IMU_CTRL_REG4_M_VALUE);

    if (status != HAL_OK)
    {
        return FS_IMU_STATUS_CONFIGURATION_ERROR;
    }

    /*
     * Habilitar Block Data Update.
     */
    status = FS_IMU_WriteRegister(
            fs_imu_i2c,
            fs_imu_info.mag_address_7bit,
            FS_IMU_REG_CTRL_REG5_M,
            FS_IMU_CTRL_REG5_M_VALUE);

    if (status != HAL_OK)
    {
        return FS_IMU_STATUS_CONFIGURATION_ERROR;
    }

    return FS_IMU_STATUS_OK;
}


/* -------------------------------------------------------------------------- */
/* Funciones públicas                                                         */
/* -------------------------------------------------------------------------- */

/**
 * @brief Detecta los dos bloques internos del LSM9DS1.
 *
 * Esta función no configura el sensor. Solamente:
 *
 *   - Busca las direcciones I2C.
 *   - Verifica los registros WHO_AM_I.
 *   - Guarda los resultados en FS_IMU_Info_t.
 */
FS_IMU_Status_t FS_IMU_Detect(I2C_HandleTypeDef *hi2c,
                              FS_IMU_Info_t *info)
{
    if ((hi2c == NULL) || (info == NULL))
    {
        return FS_IMU_STATUS_INVALID_ARGUMENT;
    }

    /*
     * Limpiar completamente la estructura de salida.
     */
    *info = (FS_IMU_Info_t){0};

    /*
     * Buscar el bloque acelerómetro/giroscopio.
     *
     * sizeof(fs_ag_addresses) / sizeof(fs_ag_addresses[0])
     * calcula automáticamente la cantidad de elementos del arreglo.
     */
    info->ag_detected = FS_IMU_FindDevice(
            hi2c,
            fs_ag_addresses,
            (uint32_t)(sizeof(fs_ag_addresses) /
                       sizeof(fs_ag_addresses[0])),
            FS_IMU_AG_WHO_AM_I_EXPECTED,
            &info->ag_address_7bit,
            &info->ag_who_am_i);

    /*
     * Buscar el magnetómetro.
     */
    info->mag_detected = FS_IMU_FindDevice(
            hi2c,
            fs_mag_addresses,
            (uint32_t)(sizeof(fs_mag_addresses) /
                       sizeof(fs_mag_addresses[0])),
            FS_IMU_MAG_WHO_AM_I_EXPECTED,
            &info->mag_address_7bit,
            &info->mag_who_am_i);

    /*
     * Ambos bloques encontrados.
     */
    if ((info->ag_detected != 0U) &&
        (info->mag_detected != 0U))
    {
        return FS_IMU_STATUS_OK;
    }

    /*
     * Ninguno de los dos bloques fue encontrado.
     */
    if ((info->ag_detected == 0U) &&
        (info->mag_detected == 0U))
    {
        return FS_IMU_STATUS_BOTH_NOT_FOUND;
    }

    /*
     * Solo falta acelerómetro/giroscopio.
     */
    if (info->ag_detected == 0U)
    {
        return FS_IMU_STATUS_AG_NOT_FOUND;
    }

    /*
     * Si llegó hasta aquí, AG fue encontrado pero el magnetómetro no.
     */
    return FS_IMU_STATUS_MAG_NOT_FOUND;
}


/**
 * @brief Detecta y configura completamente la IMU.
 *
 * Esta función debe llamarse una vez durante la inicialización.
 *
 * Ejemplo:
 *
 *   FS_IMU_Info_t imu_info;
 *   FS_IMU_Init(&hi2c1, &imu_info);
 */
FS_IMU_Status_t FS_IMU_Init(I2C_HandleTypeDef *hi2c,
                            FS_IMU_Info_t *info)
{
    FS_IMU_Status_t status;

    if ((hi2c == NULL) || (info == NULL))
    {
        return FS_IMU_STATUS_INVALID_ARGUMENT;
    }

    /*
     * Marcar primero la IMU como no inicializada.
     *
     * Solo se cambiará a 1U cuando toda la secuencia termine
     * correctamente.
     */
    fs_imu_initialized = 0U;

    /*
     * Guardar el periférico I2C para futuras lecturas.
     */
    fs_imu_i2c = hi2c;

    /*
     * Detectar direcciones y WHO_AM_I.
     *
     * Los resultados se almacenan internamente en fs_imu_info.
     */
    status = FS_IMU_Detect(hi2c, &fs_imu_info);

    /*
     * Copiar la información detectada a la estructura proporcionada
     * por el usuario, incluso si hubo un error parcial.
     *
     * Así se puede saber cuál bloque fue detectado y cuál no.
     */
    *info = fs_imu_info;

    if (status != FS_IMU_STATUS_OK)
    {
        return status;
    }

    /*
     * Configurar acelerómetro y giroscopio.
     */
    status = FS_IMU_ConfigureAG();

    if (status != FS_IMU_STATUS_OK)
    {
        return status;
    }

    /*
     * Configurar magnetómetro.
     */
    status = FS_IMU_ConfigureMag();

    if (status != FS_IMU_STATUS_OK)
    {
        return status;
    }

    /*
     * La IMU solo se considera inicializada después de detectar
     * y configurar correctamente ambos bloques.
     */
    fs_imu_initialized = 1U;

    return FS_IMU_STATUS_OK;
}


/**
 * @brief Lee una muestra completa de nueve ejes.
 *
 * Se obtienen:
 *
 *   - Giroscopio X, Y y Z.
 *   - Acelerómetro X, Y y Z.
 *   - Magnetómetro X, Y y Z.
 *
 * La función entrega:
 *
 *   - Valores raw int16_t.
 *   - Valores escalados en unidades físicas.
 *   - Marca de tiempo.
 *   - Bandera de validez.
 */
FS_IMU_Status_t FS_IMU_ReadSample(FS_IMU_Sample_t *sample)
{
    /*
     * Cada sensor posee tres ejes de 16 bits:
     *
     *   3 ejes × 2 bytes = 6 bytes.
     */
    uint8_t gyro_data[6];
    uint8_t accel_data[6];
    uint8_t mag_data[6];

    HAL_StatusTypeDef status;

    if (sample == NULL)
    {
        return FS_IMU_STATUS_INVALID_ARGUMENT;
    }

    /*
     * Limpiar la muestra completa antes de realizar las lecturas.
     *
     * Si ocurre un error posteriormente:
     *
     *   sample->valid permanecerá en 0U.
     */
    *sample = (FS_IMU_Sample_t){0};

    /*
     * No se permiten lecturas antes de ejecutar correctamente
     * FS_IMU_Init().
     */
    if ((fs_imu_initialized == 0U) ||
        (fs_imu_i2c == NULL))
    {
        return FS_IMU_STATUS_NOT_INITIALIZED;
    }

    /*
     * Leer los seis bytes del giroscopio:
     *
     *   X_L, X_H, Y_L, Y_H, Z_L, Z_H
     */
    status = FS_IMU_ReadRegisters(
            fs_imu_i2c,
            fs_imu_info.ag_address_7bit,
            FS_IMU_REG_OUT_X_L_G,
            gyro_data,
            6U);

    if (status != HAL_OK)
    {
        return FS_IMU_STATUS_READ_ERROR;
    }

    /*
     * Leer los seis bytes del acelerómetro.
     */
    status = FS_IMU_ReadRegisters(
            fs_imu_i2c,
            fs_imu_info.ag_address_7bit,
            FS_IMU_REG_OUT_X_L_XL,
            accel_data,
            6U);

    if (status != HAL_OK)
    {
        return FS_IMU_STATUS_READ_ERROR;
    }

    /*
     * Leer los seis bytes del magnetómetro.
     */
    status = FS_IMU_ReadRegisters(
            fs_imu_i2c,
            fs_imu_info.mag_address_7bit,
            FS_IMU_REG_OUT_X_L_M,
            mag_data,
            6U);

    if (status != HAL_OK)
    {
        return FS_IMU_STATUS_READ_ERROR;
    }

    /*
     * Reconstruir valores raw del giroscopio.
     */
    sample->raw.gyro_x =
        FS_IMU_JoinBytes(gyro_data[0], gyro_data[1]);

    sample->raw.gyro_y =
        FS_IMU_JoinBytes(gyro_data[2], gyro_data[3]);

    sample->raw.gyro_z =
        FS_IMU_JoinBytes(gyro_data[4], gyro_data[5]);

    /*
     * Reconstruir valores raw del acelerómetro.
     */
    sample->raw.accel_x =
        FS_IMU_JoinBytes(accel_data[0], accel_data[1]);

    sample->raw.accel_y =
        FS_IMU_JoinBytes(accel_data[2], accel_data[3]);

    sample->raw.accel_z =
        FS_IMU_JoinBytes(accel_data[4], accel_data[5]);

    /*
     * Reconstruir valores raw del magnetómetro.
     */
    sample->raw.mag_x =
        FS_IMU_JoinBytes(mag_data[0], mag_data[1]);

    sample->raw.mag_y =
        FS_IMU_JoinBytes(mag_data[2], mag_data[3]);

    sample->raw.mag_z =
        FS_IMU_JoinBytes(mag_data[4], mag_data[5]);

    /*
     * Convertir acelerómetro raw a unidades g.
     */
    sample->scaled.accel_x_g =
        (float)sample->raw.accel_x *
        FS_IMU_ACCEL_SENSITIVITY_G;

    sample->scaled.accel_y_g =
        (float)sample->raw.accel_y *
        FS_IMU_ACCEL_SENSITIVITY_G;

    sample->scaled.accel_z_g =
        (float)sample->raw.accel_z *
        FS_IMU_ACCEL_SENSITIVITY_G;

    /*
     * Convertir giroscopio raw a grados por segundo.
     */
    sample->scaled.gyro_x_dps =
        (float)sample->raw.gyro_x *
        FS_IMU_GYRO_SENSITIVITY_DPS;

    sample->scaled.gyro_y_dps =
        (float)sample->raw.gyro_y *
        FS_IMU_GYRO_SENSITIVITY_DPS;

    sample->scaled.gyro_z_dps =
        (float)sample->raw.gyro_z *
        FS_IMU_GYRO_SENSITIVITY_DPS;

    /*
     * Convertir magnetómetro raw a gauss.
     */
    sample->scaled.mag_x_gauss =
        (float)sample->raw.mag_x *
        FS_IMU_MAG_SENSITIVITY_GAUSS;

    sample->scaled.mag_y_gauss =
        (float)sample->raw.mag_y *
        FS_IMU_MAG_SENSITIVITY_GAUSS;

    sample->scaled.mag_z_gauss =
        (float)sample->raw.mag_z *
        FS_IMU_MAG_SENSITIVITY_GAUSS;

    /*
     * Registrar el instante en el que terminó la lectura.
     *
     * HAL_GetTick() devuelve el tiempo del sistema en milisegundos.
     */
    sample->timestamp_ms = HAL_GetTick();

    /*
     * Marcar la muestra como válida solamente después de completar
     * las tres lecturas y todas las conversiones.
     */
    sample->valid = 1U;

    return FS_IMU_STATUS_OK;
}


/**
 * @brief Indica si la IMU fue inicializada correctamente.
 *
 * @return
 *   1U si FS_IMU_Init() terminó correctamente.
 *   0U si no ha sido inicializada o la inicialización falló.
 */
uint8_t FS_IMU_IsInitialized(void)
{
    return fs_imu_initialized;
}
