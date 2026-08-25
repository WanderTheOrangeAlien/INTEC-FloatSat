/*
 * Archivo encargado de calibrar el giroscopio.
 *
 * La calibración calcula el sesgo o bias de cada eje del giroscopio
 * mientras la IMU permanece aproximadamente inmóvil.
 *
 * El bias representa la velocidad angular que el sensor entrega
 * incluso cuando realmente no está girando.
 *
 * Ejemplo:
 *
 *   Medición con el sensor inmóvil:
 *     X = 7.30 dps
 *     Y = 5.80 dps
 *     Z = -0.75 dps
 *
 *   Esos valores se almacenan como bias y posteriormente se restan
 *   de todas las mediciones del giroscopio.
 */

#include "floatsat_gyro_calibration.h"

/*
 * math.h
 *
 * Se utiliza para acceder a sqrtf(), función que calcula la raíz
 * cuadrada utilizando números float.
 *
 * sqrtf() se emplea para calcular:
 *
 *   - La magnitud total del vector del acelerómetro.
 *   - La magnitud total del vector del giroscopio.
 */
#include <math.h>

/*
 * stddef.h
 *
 * Proporciona la definición de NULL.
 *
 * NULL se utiliza para verificar que los punteros recibidos por las
 * funciones sean válidos antes de acceder a ellos.
 */
#include <stddef.h>


/* --------------------------------------------------------------------------
 * CONDICIONES PARA ACEPTAR UNA MUESTRA DE CALIBRACIÓN
 * --------------------------------------------------------------------------
 *
 * Una muestra solamente se utiliza para calcular el bias cuando la IMU
 * parece estar aproximadamente en reposo.
 *
 * Se comprueban dos condiciones:
 *
 * 1. La magnitud total del acelerómetro debe estar próxima a 1 g.
 *
 *    Cuando el sensor está quieto, normalmente mide principalmente
 *    la gravedad terrestre.
 *
 * 2. La magnitud total del giroscopio no debe superar cierto límite.
 *
 *    Esto evita utilizar muestras tomadas mientras el sistema está
 *    rotando con una velocidad considerable.
 */

/*
 * Límite inferior permitido para la magnitud del acelerómetro.
 *
 * Una medición menor de 0.90 g se considera poco compatible con
 * una IMU estable y correctamente apoyada.
 */
#define FS_GYRO_CAL_ACCEL_MIN_G       (0.90f)

/*
 * Límite superior permitido para la magnitud del acelerómetro.
 *
 * Una medición mayor de 1.10 g puede indicar movimiento, vibración
 * o aceleraciones adicionales a la gravedad.
 */
#define FS_GYRO_CAL_ACCEL_MAX_G       (1.10f)

/*
 * Máxima magnitud de velocidad angular permitida para aceptar
 * una muestra durante la calibración.
 *
 * La muestra se rechaza cuando:
 *
 *   gyro_magnitude_dps > 20.0 dps
 */
#define FS_GYRO_CAL_MAX_RATE_DPS      (20.00f)


/**
 * @brief Reinicia completamente una estructura de calibración.
 *
 * Coloca todos sus acumuladores, contadores, biases y banderas en cero.
 * Después establece explícitamente el estado como inactivo.
 *
 * @param calibration
 *        Puntero a la estructura de calibración que se desea reiniciar.
 *
 * Si el puntero es NULL, la función termina sin realizar ninguna acción.
 */
void FS_GyroCalibration_Reset(
        FS_GyroCalibration_t *calibration)
{
    /*
     * Protección contra punteros inválidos.
     *
     * Sin esta comprobación, acceder a *calibration cuando calibration
     * sea NULL produciría un fallo de memoria.
     */
    if (calibration == NULL)
    {
        return;
    }

    /*
     * Inicialización completa de la estructura con ceros.
     *
     * Esto reinicia, entre otros:
     *
     *   sample_count
     *   rejected_samples
     *   sum_x_dps
     *   sum_y_dps
     *   sum_z_dps
     *   bias_x_dps
     *   bias_y_dps
     *   bias_z_dps
     *   target_samples
     *   timeout_ms
     *   start_time_ms
     */
    *calibration = (FS_GyroCalibration_t){0};

    /*
     * La calibración queda inactiva y todavía no está recolectando
     * muestras.
     */
    calibration->status =
        FS_GYRO_CAL_IDLE;

    /*
     * Indica que aún no existe un bias válido que pueda aplicarse
     * a las mediciones.
     */
    calibration->valid = 0U;
}


/**
 * @brief Inicia un nuevo proceso de calibración del giroscopio.
 *
 * Primero reinicia la estructura y después configura:
 *
 *   - Cantidad de muestras válidas necesarias.
 *   - Tiempo máximo permitido.
 *   - Instante en el que comenzó el proceso.
 *
 * @param calibration
 *        Estructura donde se almacenará el progreso de calibración.
 *
 * @param current_time_ms
 *        Tiempo actual del sistema en milisegundos.
 *
 * @param target_samples
 *        Cantidad de muestras válidas que se deben reunir.
 *
 * @param timeout_ms
 *        Tiempo máximo permitido para completar la calibración.
 */
void FS_GyroCalibration_Start(
        FS_GyroCalibration_t *calibration,
        uint32_t current_time_ms,
        uint32_t target_samples,
        uint32_t timeout_ms)
{
    /*
     * No se puede iniciar la calibración si la estructura no existe.
     */
    if (calibration == NULL)
    {
        return;
    }

    /*
     * Eliminar cualquier calibración, suma o contador anterior.
     */
    FS_GyroCalibration_Reset(calibration);

    /*
     * Guardar la cantidad de muestras válidas necesarias.
     */
    calibration->target_samples =
        target_samples;

    /*
     * Guardar el tiempo máximo permitido para completar la calibración.
     */
    calibration->timeout_ms =
        timeout_ms;

    /*
     * Registrar el instante de inicio.
     *
     * Posteriormente se utilizará para calcular:
     *
     *   elapsed_ms = current_time_ms - start_time_ms
     */
    calibration->start_time_ms =
        current_time_ms;

    /*
     * Cambiar el estado para habilitar la recolección de muestras.
     */
    calibration->status =
        FS_GYRO_CAL_COLLECTING;
}


/**
 * @brief Procesa una nueva muestra durante la calibración.
 *
 * Esta función debe llamarse periódicamente mientras el estado sea:
 *
 *   FS_GYRO_CAL_COLLECTING
 *
 * Cada llamada:
 *
 *   1. Comprueba los argumentos y el estado.
 *   2. Comprueba el timeout.
 *   3. Calcula las magnitudes del acelerómetro y del giroscopio.
 *   4. Rechaza la muestra si el sistema parece estar moviéndose.
 *   5. Acumula las muestras válidas.
 *   6. Calcula los biases cuando se alcanza la cantidad solicitada.
 *
 * @param calibration
 *        Estructura que contiene el progreso de la calibración.
 *
 * @param gyro_x_dps
 *        Velocidad angular medida en el eje X, en grados por segundo.
 *
 * @param gyro_y_dps
 *        Velocidad angular medida en el eje Y, en grados por segundo.
 *
 * @param gyro_z_dps
 *        Velocidad angular medida en el eje Z, en grados por segundo.
 *
 * @param accel_x_g
 *        Aceleración medida en X, expresada en unidades g.
 *
 * @param accel_y_g
 *        Aceleración medida en Y, expresada en unidades g.
 *
 * @param accel_z_g
 *        Aceleración medida en Z, expresada en unidades g.
 *
 * @param current_time_ms
 *        Tiempo actual del sistema en milisegundos.
 *
 * @return Estado actual de la calibración.
 */
FS_GyroCalibrationStatus_t FS_GyroCalibration_Update(
        FS_GyroCalibration_t *calibration,
        float gyro_x_dps,
        float gyro_y_dps,
        float gyro_z_dps,
        float accel_x_g,
        float accel_y_g,
        float accel_z_g,
        uint32_t current_time_ms)
{
    /*
     * Magnitud del vector de aceleración:
     *
     *   |a| = sqrt(ax² + ay² + az²)
     */
    float accel_magnitude_g;

    /*
     * Magnitud del vector de velocidad angular:
     *
     *   |w| = sqrt(wx² + wy² + wz²)
     */
    float gyro_magnitude_dps;

    /*
     * Tiempo transcurrido desde el inicio de la calibración.
     */
    uint32_t elapsed_ms;

    /*
     * Sin una estructura válida no se puede guardar ni actualizar
     * el proceso de calibración.
     */
    if (calibration == NULL)
    {
        return FS_GYRO_CAL_ERROR_INVALID_ARGUMENT;
    }

    /*
     * Solamente se procesan muestras mientras la calibración está
     * en estado de recolección.
     *
     * Si ya terminó, produjo un error o está inactiva, se devuelve
     * directamente su estado actual.
     */
    if (calibration->status !=
        FS_GYRO_CAL_COLLECTING)
    {
        return calibration->status;
    }

    /*
     * Una calibración con cero muestras objetivo no es válida.
     *
     * Sin esta comprobación, la condición:
     *
     *   sample_count >= target_samples
     *
     * sería verdadera inmediatamente.
     */
    if (calibration->target_samples == 0U)
    {
        calibration->status =
            FS_GYRO_CAL_ERROR_INVALID_ARGUMENT;

        calibration->valid = 0U;

        return calibration->status;
    }

    /*
     * Calcular cuánto tiempo lleva activa la calibración.
     *
     * Esta resta también funciona correctamente cuando el contador
     * uint32_t desborda, siempre que el intervalo sea razonablemente
     * menor que el rango completo del contador.
     */
    elapsed_ms =
        current_time_ms -
        calibration->start_time_ms;

    /*
     * Detener el proceso si se supera el tiempo máximo permitido.
     */
    if (elapsed_ms >= calibration->timeout_ms)
    {
        calibration->status =
            FS_GYRO_CAL_ERROR_TIMEOUT;

        calibration->valid = 0U;

        return calibration->status;
    }

    /*
     * Calcular la magnitud total del acelerómetro.
     *
     * Para una IMU aproximadamente inmóvil se espera un valor
     * cercano a 1 g.
     */
    accel_magnitude_g =
        sqrtf(
            accel_x_g * accel_x_g +
            accel_y_g * accel_y_g +
            accel_z_g * accel_z_g);

    /*
     * Calcular la magnitud total de velocidad angular.
     *
     * Se utiliza la magnitud porque importa cuánto está girando
     * el sistema, independientemente del eje o del signo.
     */
    gyro_magnitude_dps =
        sqrtf(
            gyro_x_dps * gyro_x_dps +
            gyro_y_dps * gyro_y_dps +
            gyro_z_dps * gyro_z_dps);

    /*
     * Rechazar la muestra actual si la IMU no parece estar quieta.
     *
     * La muestra se rechaza cuando:
     *
     *   - La aceleración total es menor de 0.90 g.
     *   - La aceleración total es mayor de 1.10 g.
     *   - La velocidad angular total es mayor de 20 dps.
     *
     * Importante:
     *
     * Las muestras válidas acumuladas anteriormente NO se eliminan.
     * Únicamente se incrementa el contador de muestras rechazadas.
     */
    if ((accel_magnitude_g <
         FS_GYRO_CAL_ACCEL_MIN_G) ||
        (accel_magnitude_g >
         FS_GYRO_CAL_ACCEL_MAX_G) ||
        (gyro_magnitude_dps >
         FS_GYRO_CAL_MAX_RATE_DPS))
    {
        calibration->rejected_samples++;

        /*
         * El estado permanece como FS_GYRO_CAL_COLLECTING.
         */
        return calibration->status;
    }

    /*
     * La muestra pasó todas las condiciones.
     *
     * Acumular las mediciones de cada eje para posteriormente
     * obtener su promedio.
     */
    calibration->sum_x_dps +=
        gyro_x_dps;

    calibration->sum_y_dps +=
        gyro_y_dps;

    calibration->sum_z_dps +=
        gyro_z_dps;

    /*
     * Registrar una nueva muestra válida.
     */
    calibration->sample_count++;

    /*
     * Finalizar cuando se alcance la cantidad solicitada de
     * muestras válidas.
     *
     * El bias de cada eje es el promedio de las mediciones:
     *
     *   bias_x = suma_x / cantidad_muestras
     *   bias_y = suma_y / cantidad_muestras
     *   bias_z = suma_z / cantidad_muestras
     */
    if (calibration->sample_count >=
        calibration->target_samples)
    {
        calibration->bias_x_dps =
            calibration->sum_x_dps /
            (float)calibration->sample_count;

        calibration->bias_y_dps =
            calibration->sum_y_dps /
            (float)calibration->sample_count;

        calibration->bias_z_dps =
            calibration->sum_z_dps /
            (float)calibration->sample_count;

        /*
         * La calibración ya contiene biases utilizables.
         */
        calibration->valid = 1U;

        /*
         * Marcar el proceso como completado correctamente.
         */
        calibration->status =
            FS_GYRO_CAL_VALID;
    }

    /*
     * Devolver el estado actualizado:
     *
     *   FS_GYRO_CAL_COLLECTING
     *   FS_GYRO_CAL_VALID
     *   o algún estado de error.
     */
    return calibration->status;
}


/**
 * @brief Aplica los biases calculados a una medición del giroscopio.
 *
 * La corrección se realiza restando el bias:
 *
 *   valor_corregido = valor_medido - bias
 *
 * @param calibration
 *        Estructura que contiene los biases calculados.
 *
 * @param measured_x_dps
 *        Medición original del eje X.
 *
 * @param measured_y_dps
 *        Medición original del eje Y.
 *
 * @param measured_z_dps
 *        Medición original del eje Z.
 *
 * @param corrected_x_dps
 *        Puntero donde se escribirá el valor corregido de X.
 *
 * @param corrected_y_dps
 *        Puntero donde se escribirá el valor corregido de Y.
 *
 * @param corrected_z_dps
 *        Puntero donde se escribirá el valor corregido de Z.
 */
void FS_GyroCalibration_Apply(
        const FS_GyroCalibration_t *calibration,
        float measured_x_dps,
        float measured_y_dps,
        float measured_z_dps,
        float *corrected_x_dps,
        float *corrected_y_dps,
        float *corrected_z_dps)
{
    /*
     * Los tres punteros de salida son obligatorios.
     *
     * Si alguno es NULL, no se puede devolver de forma segura
     * el resultado completo.
     */
    if ((corrected_x_dps == NULL) ||
        (corrected_y_dps == NULL) ||
        (corrected_z_dps == NULL))
    {
        return;
    }

    /*
     * Mientras no exista una calibración válida, se devuelven las
     * mediciones originales sin modificación.
     *
     * Esto permite utilizar la función incluso durante el arranque,
     * antes de que termine la calibración.
     */
    if ((calibration == NULL) ||
        (calibration->valid == 0U))
    {
        *corrected_x_dps =
            measured_x_dps;

        *corrected_y_dps =
            measured_y_dps;

        *corrected_z_dps =
            measured_z_dps;

        return;
    }

    /*
     * Eliminar de cada eje el desplazamiento promedio medido
     * durante la calibración.
     */
    *corrected_x_dps =
        measured_x_dps -
        calibration->bias_x_dps;

    *corrected_y_dps =
        measured_y_dps -
        calibration->bias_y_dps;

    *corrected_z_dps =
        measured_z_dps -
        calibration->bias_z_dps;
}
