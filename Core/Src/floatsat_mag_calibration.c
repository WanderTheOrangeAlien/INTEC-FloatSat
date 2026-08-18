/*
 * Calibración del magnetómetro mediante el método de mínimos y máximos.
 *
 * Durante la calibración, el sistema debe orientarse en distintas
 * direcciones para que cada eje del magnetómetro recorra una parte
 * suficientemente amplia del campo magnético.
 *
 * A partir de los valores mínimos y máximos de cada eje se calculan:
 *
 *   1. Bias u offset de hard-iron:
 *
 *      bias = (máximo + mínimo) / 2
 *
 *   2. Semiamplitud de cada eje:
 *
 *      half_range = (máximo - mínimo) / 2
 *
 *   3. Corrección de escala:
 *
 *      scale = radio_promedio / half_range
 *
 * La corrección final aplicada a cada eje es:
 *
 *   valor_corregido = (valor_original - bias) * scale
 */

#include "floatsat_mag_calibration.h"

/*
 * float.h proporciona FLT_MAX:
 *
 * el mayor valor positivo representable por un float.
 *
 * Se utiliza para inicializar los mínimos y máximos antes de comenzar
 * a recibir muestras.
 */
#include <float.h>

/*
 * stddef.h proporciona la definición de NULL.
 */
#include <stddef.h>


/* -------------------------------------------------------------------------- */
/* Condiciones mínimas de calibración                                         */
/* -------------------------------------------------------------------------- */

/*
 * Cantidad mínima de muestras necesarias para aceptar una calibración.
 *
 * Ejemplos:
 *
 *   A 100 Hz:
 *     500 muestras = 5 segundos.
 *
 *   Durante una calibración de 30 segundos:
 *     aproximadamente 3000 muestras.
 *
 * Esta condición evita calcular una calibración con muy pocos datos.
 */
#define FS_MAG_CAL_MINIMUM_SAMPLES       500U

/*
 * Semiamplitud mínima requerida en cada eje.
 *
 * Cada eje debe recorrer al menos ±0.050 gauss aproximadamente
 * respecto a su centro medido.
 *
 * Si un eje tiene un rango menor, probablemente el sensor no se giró
 * suficientemente alrededor de esa dirección.
 */
#define FS_MAG_CAL_MIN_HALF_RANGE_GAUSS  0.050f

/*
 * Límites aceptables para los factores de corrección de escala.
 *
 * Estos límites evitan aceptar resultados absurdos causados por:
 *
 *   - Movimiento insuficiente.
 *   - Datos corruptos.
 *   - Lecturas erróneas.
 *   - Presencia magnética extrema durante la calibración.
 */
#define FS_MAG_CAL_MIN_SCALE             0.20f
#define FS_MAG_CAL_MAX_SCALE             5.00f


/* -------------------------------------------------------------------------- */
/* Funciones privadas auxiliares                                              */
/* -------------------------------------------------------------------------- */

/**
 * @brief Devuelve el menor de tres valores.
 *
 * Se utiliza para determinar cuál eje tuvo la menor semiamplitud
 * durante la calibración.
 */
static float FS_MagCalibration_Min3(
        float a,
        float b,
        float c)
{
    float minimum = a;              /* Suponer inicialmente que a es el menor. */

    if (b < minimum)
    {
        minimum = b;
    }

    if (c < minimum)
    {
        minimum = c;
    }

    return minimum;
}


/**
 * @brief Devuelve el mayor de tres valores.
 *
 * Se utiliza para determinar cuál eje tuvo la mayor semiamplitud
 * durante la calibración.
 */
static float FS_MagCalibration_Max3(
        float a,
        float b,
        float c)
{
    float maximum = a;              /* Suponer inicialmente que a es el mayor. */

    if (b > maximum)
    {
        maximum = b;
    }

    if (c > maximum)
    {
        maximum = c;
    }

    return maximum;
}


/* -------------------------------------------------------------------------- */
/* Reinicio e inicio de la calibración                                        */
/* -------------------------------------------------------------------------- */

/**
 * @brief Reinicia completamente una estructura de calibración magnética.
 *
 * Elimina:
 *
 *   - Muestras anteriores.
 *   - Mínimos y máximos.
 *   - Bias calculados.
 *   - Factores de escala.
 *   - Estado anterior.
 *
 * Después del reinicio, la calibración queda inactiva.
 *
 * @param calibration Estructura que se desea reiniciar.
 */
void FS_MagCalibration_Reset(
        FS_MagCalibration_t *calibration)
{
    /*
     * Evitar acceder a una dirección de memoria inválida.
     */
    if (calibration == NULL)
    {
        return;
    }

    /*
     * Colocar todos los campos de la estructura en cero.
     */
    *calibration = (FS_MagCalibration_t){0};

    calibration->status =
        FS_MAG_CAL_IDLE;             /* La calibración todavía no ha iniciado. */

    calibration->valid = 0U;         /* No existen parámetros válidos. */

    /*
     * Inicializar los mínimos con el mayor float positivo.
     *
     * De esta manera, la primera muestra real será necesariamente
     * menor y reemplazará el valor inicial.
     */
    calibration->min_x = FLT_MAX;
    calibration->min_y = FLT_MAX;
    calibration->min_z = FLT_MAX;

    /*
     * Inicializar los máximos con el mayor float negativo.
     *
     * De esta manera, la primera muestra real será necesariamente
     * mayor y reemplazará el valor inicial.
     */
    calibration->max_x = -FLT_MAX;
    calibration->max_y = -FLT_MAX;
    calibration->max_z = -FLT_MAX;

    /*
     * Escala neutra.
     *
     * Multiplicar por 1.0 no modifica las mediciones.
     */
    calibration->scale_x = 1.0f;
    calibration->scale_y = 1.0f;
    calibration->scale_z = 1.0f;
}


/**
 * @brief Inicia una nueva calibración del magnetómetro.
 *
 * Primero reinicia toda la estructura y luego guarda:
 *
 *   - El instante inicial.
 *   - La duración deseada.
 *   - El estado de recolección.
 *
 * @param calibration    Estructura de calibración.
 * @param current_time_ms Tiempo actual del sistema, en milisegundos.
 * @param duration_ms     Duración total prevista de la calibración.
 */
void FS_MagCalibration_Start(
        FS_MagCalibration_t *calibration,
        uint32_t current_time_ms,
        uint32_t duration_ms)
{
    if (calibration == NULL)
    {
        return;
    }

    /*
     * Eliminar cualquier calibración anterior.
     */
    FS_MagCalibration_Reset(calibration);

    /*
     * Registrar el tiempo de inicio.
     */
    calibration->start_time_ms =
        current_time_ms;

    /*
     * Guardar cuánto tiempo debe durar la recolección.
     */
    calibration->duration_ms =
        duration_ms;

    /*
     * Habilitar la recepción de muestras.
     */
    calibration->status =
        FS_MAG_CAL_COLLECTING;
}


/* -------------------------------------------------------------------------- */
/* Recolección de muestras                                                    */
/* -------------------------------------------------------------------------- */

/**
 * @brief Incorpora una nueva muestra a la calibración.
 *
 * Esta función no guarda todas las muestras. Solamente conserva:
 *
 *   - El mínimo de cada eje.
 *   - El máximo de cada eje.
 *   - La cantidad total de muestras recibidas.
 *
 * Esto reduce considerablemente el uso de memoria.
 *
 * @param calibration Estructura de calibración activa.
 * @param mag_x_gauss Medición magnética en X.
 * @param mag_y_gauss Medición magnética en Y.
 * @param mag_z_gauss Medición magnética en Z.
 */
void FS_MagCalibration_Update(
        FS_MagCalibration_t *calibration,
        float mag_x_gauss,
        float mag_y_gauss,
        float mag_z_gauss)
{
    if (calibration == NULL)
    {
        return;
    }

    /*
     * Ignorar muestras si la calibración no está recolectando datos.
     */
    if (calibration->status !=
        FS_MAG_CAL_COLLECTING)
    {
        return;
    }

    /*
     * Actualizar mínimo y máximo del eje X.
     */
    if (mag_x_gauss < calibration->min_x)
    {
        calibration->min_x = mag_x_gauss;
    }

    if (mag_x_gauss > calibration->max_x)
    {
        calibration->max_x = mag_x_gauss;
    }

    /*
     * Actualizar mínimo y máximo del eje Y.
     */
    if (mag_y_gauss < calibration->min_y)
    {
        calibration->min_y = mag_y_gauss;
    }

    if (mag_y_gauss > calibration->max_y)
    {
        calibration->max_y = mag_y_gauss;
    }

    /*
     * Actualizar mínimo y máximo del eje Z.
     */
    if (mag_z_gauss < calibration->min_z)
    {
        calibration->min_z = mag_z_gauss;
    }

    if (mag_z_gauss > calibration->max_z)
    {
        calibration->max_z = mag_z_gauss;
    }

    /*
     * Registrar que se procesó una nueva muestra.
     */
    calibration->sample_count++;
}


/* -------------------------------------------------------------------------- */
/* Comprobación del tiempo                                                    */
/* -------------------------------------------------------------------------- */

/**
 * @brief Indica si terminó el tiempo previsto de calibración.
 *
 * La función solamente comprueba el tiempo; no finaliza ni calcula
 * los parámetros. Después de obtener 1U debe llamarse:
 *
 *   FS_MagCalibration_Finish()
 *
 * @param calibration    Estructura de calibración.
 * @param current_time_ms Tiempo actual del sistema.
 *
 * @return
 *   1U si se alcanzó la duración configurada.
 *   0U si todavía no terminó o la calibración no está activa.
 */
uint8_t FS_MagCalibration_TimeComplete(
        const FS_MagCalibration_t *calibration,
        uint32_t current_time_ms)
{
    uint32_t elapsed_ms;             /* Tiempo transcurrido desde el inicio. */

    if (calibration == NULL)
    {
        return 0U;
    }

    /*
     * La comprobación solo tiene sentido mientras se recopilan datos.
     */
    if (calibration->status !=
        FS_MAG_CAL_COLLECTING)
    {
        return 0U;
    }

    /*
     * Calcular el tiempo transcurrido:
     *
     *   tiempo_transcurrido = tiempo_actual - tiempo_inicial
     *
     * Al utilizar uint32_t, esta resta funciona correctamente incluso
     * cuando HAL_GetTick() desborda, siempre que el intervalo medido
     * sea menor que el rango completo del contador.
     */
    elapsed_ms =
        current_time_ms -
        calibration->start_time_ms;

    /*
     * El operador ternario equivale a:
     *
     *   if (elapsed_ms >= duration_ms)
     *       return 1U;
     *   else
     *       return 0U;
     */
    return (elapsed_ms >= calibration->duration_ms)
           ? 1U
           : 0U;
}


/* -------------------------------------------------------------------------- */
/* Cálculo de parámetros                                                      */
/* -------------------------------------------------------------------------- */

/**
 * @brief Finaliza la calibración y calcula bias y escalas.
 *
 * La función:
 *
 *   1. Comprueba que existan suficientes muestras.
 *   2. Calcula la semiamplitud de cada eje.
 *   3. Verifica que los tres ejes tengan suficiente recorrido.
 *   4. Calcula el bias de hard-iron.
 *   5. Calcula factores de escala por eje.
 *   6. Evalúa la calidad de cobertura.
 *
 * @param calibration Estructura con las muestras acumuladas.
 *
 * @return Estado final de la calibración.
 */
FS_MagCalibrationStatus_t FS_MagCalibration_Finish(
        FS_MagCalibration_t *calibration)
{
    /*
     * Menor y mayor semiamplitud observadas entre los tres ejes.
     *
     * Se utilizan para calcular la calidad de cobertura.
     */
    float minimum_radius;
    float maximum_radius;

    /*
     * No existe un estado específico de argumento inválido, por lo que
     * el código utiliza FS_MAG_CAL_ERROR_AXIS_RANGE para un puntero nulo.
     */
    if (calibration == NULL)
    {
        return FS_MAG_CAL_ERROR_AXIS_RANGE;
    }

    /*
     * La calibración deja de considerarse válida hasta que todas
     * las comprobaciones terminen correctamente.
     */
    calibration->valid = 0U;

    /*
     * Verificar que se hayan recolectado al menos 500 muestras.
     */
    if (calibration->sample_count <
        FS_MAG_CAL_MINIMUM_SAMPLES)
    {
        calibration->status =
            FS_MAG_CAL_ERROR_NOT_ENOUGH_SAMPLES;

        return calibration->status;
    }

    /*
     * Calcular la semiamplitud de cada eje:
     *
     *   half_range = (máximo - mínimo) / 2
     *
     * Este valor representa aproximadamente el radio medido
     * del elipsoide en ese eje.
     */
    calibration->half_range_x =
        (calibration->max_x -
         calibration->min_x) * 0.5f;

    calibration->half_range_y =
        (calibration->max_y -
         calibration->min_y) * 0.5f;

    calibration->half_range_z =
        (calibration->max_z -
         calibration->min_z) * 0.5f;

    /*
     * Comprobar que cada eje recorrió suficiente campo magnético.
     *
     * Si uno de los ejes apenas cambió, significa que el sensor
     * probablemente no fue orientado adecuadamente durante la prueba.
     */
    if ((calibration->half_range_x <
         FS_MAG_CAL_MIN_HALF_RANGE_GAUSS) ||
        (calibration->half_range_y <
         FS_MAG_CAL_MIN_HALF_RANGE_GAUSS) ||
        (calibration->half_range_z <
         FS_MAG_CAL_MIN_HALF_RANGE_GAUSS))
    {
        calibration->status =
            FS_MAG_CAL_ERROR_AXIS_RANGE;

        return calibration->status;
    }

    /*
     * Calcular el centro de cada eje:
     *
     *   bias = (máximo + mínimo) / 2
     *
     * Este desplazamiento representa principalmente el error hard-iron:
     * un campo magnético aproximadamente constante producido por
     * componentes cercanos al sensor.
     */
    calibration->bias_x =
        (calibration->max_x +
         calibration->min_x) * 0.5f;

    calibration->bias_y =
        (calibration->max_y +
         calibration->min_y) * 0.5f;

    calibration->bias_z =
        (calibration->max_z +
         calibration->min_z) * 0.5f;

    /*
     * Calcular el radio promedio de los tres ejes.
     *
     * Después de la corrección se pretende que los tres ejes tengan
     * aproximadamente este mismo radio.
     */
    calibration->average_radius =
        (calibration->half_range_x +
         calibration->half_range_y +
         calibration->half_range_z) /
        3.0f;

    /*
     * Calcular el factor de escala de cada eje:
     *
     *   scale = radio_promedio / radio_del_eje
     *
     * Un eje con un radio demasiado grande será reducido.
     * Un eje con un radio demasiado pequeño será amplificado.
     */
    calibration->scale_x =
        calibration->average_radius /
        calibration->half_range_x;

    calibration->scale_y =
        calibration->average_radius /
        calibration->half_range_y;

    calibration->scale_z =
        calibration->average_radius /
        calibration->half_range_z;

    /*
     * Rechazar factores de escala fuera del intervalo permitido.
     *
     * Un valor extremo puede indicar:
     *
     *   - Calibración incompleta.
     *   - Interferencia magnética fuerte.
     *   - Lecturas erróneas.
     */
    if ((calibration->scale_x <
         FS_MAG_CAL_MIN_SCALE) ||
        (calibration->scale_x >
         FS_MAG_CAL_MAX_SCALE) ||

        (calibration->scale_y <
         FS_MAG_CAL_MIN_SCALE) ||
        (calibration->scale_y >
         FS_MAG_CAL_MAX_SCALE) ||

        (calibration->scale_z <
         FS_MAG_CAL_MIN_SCALE) ||
        (calibration->scale_z >
         FS_MAG_CAL_MAX_SCALE))
    {
        calibration->status =
            FS_MAG_CAL_ERROR_SCALE;

        return calibration->status;
    }

    /*
     * Buscar el menor radio medido entre los tres ejes.
     */
    minimum_radius =
        FS_MagCalibration_Min3(
            calibration->half_range_x,
            calibration->half_range_y,
            calibration->half_range_z);

    /*
     * Buscar el mayor radio medido entre los tres ejes.
     */
    maximum_radius =
        FS_MagCalibration_Max3(
            calibration->half_range_x,
            calibration->half_range_y,
            calibration->half_range_z);

    /*
     * Calcular una medida simple de calidad:
     *
     *   calidad = menor_radio / mayor_radio
     *
     * Interpretación aproximada:
     *
     *   1.0  -> los tres radios son iguales.
     *   0.8  -> existe una diferencia moderada.
     *   0.5  -> existe una diferencia considerable.
     *   cerca de 0 -> cobertura muy desigual.
     *
     * Esta variable describe la similitud de los rangos medidos antes
     * de aplicar la corrección de escala.
     */
    calibration->coverage_quality =
        minimum_radius / maximum_radius;

    /*
     * Todas las comprobaciones fueron superadas.
     */
    calibration->valid = 1U;

    calibration->status =
        FS_MAG_CAL_VALID;

    return calibration->status;
}


/* -------------------------------------------------------------------------- */
/* Aplicación de la calibración                                               */
/* -------------------------------------------------------------------------- */

/**
 * @brief Aplica bias y escala a una muestra del magnetómetro.
 *
 * Corrección utilizada:
 *
 *   corregido = (valor_original - bias) * escala
 *
 * @param calibration Estructura con los parámetros calculados.
 * @param raw_x       Medición original del eje X.
 * @param raw_y       Medición original del eje Y.
 * @param raw_z       Medición original del eje Z.
 * @param corrected_x Dirección donde se guardará el eje X corregido.
 * @param corrected_y Dirección donde se guardará el eje Y corregido.
 * @param corrected_z Dirección donde se guardará el eje Z corregido.
 */
void FS_MagCalibration_Apply(
        const FS_MagCalibration_t *calibration,
        float raw_x,
        float raw_y,
        float raw_z,
        float *corrected_x,
        float *corrected_y,
        float *corrected_z)
{
    /*
     * Los tres punteros de salida son obligatorios.
     *
     * Si alguno es NULL, no se escribe ningún resultado.
     */
    if ((corrected_x == NULL) ||
        (corrected_y == NULL) ||
        (corrected_z == NULL))
    {
        return;
    }

    /*
     * Si no existe una calibración válida, devolver la muestra original.
     *
     * Esto permite continuar observando las lecturas del magnetómetro
     * aunque la calibración aún no se haya completado.
     */
    if ((calibration == NULL) ||
        (calibration->valid == 0U))
    {
        *corrected_x = raw_x;
        *corrected_y = raw_y;
        *corrected_z = raw_z;

        return;
    }

    /*
     * Eliminar el offset de hard-iron y aplicar la corrección
     * de escala correspondiente a cada eje.
     */
    *corrected_x =
        (raw_x - calibration->bias_x) *
        calibration->scale_x;

    *corrected_y =
        (raw_y - calibration->bias_y) *
        calibration->scale_y;

    *corrected_z =
        (raw_z - calibration->bias_z) *
        calibration->scale_z;
}
