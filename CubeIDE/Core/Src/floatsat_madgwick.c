/*
 * Implementación del filtro de orientación Madgwick.
 *
 * El filtro estima la orientación tridimensional del FloatSat mediante
 * un cuaternión:
 *
 *   q = q0 + q1*i + q2*j + q3*k
 *
 * Donde:
 *
 *   q0 = componente escalar.
 *   q1 = componente asociada al eje X.
 *   q2 = componente asociada al eje Y.
 *   q3 = componente asociada al eje Z.
 *
 * El filtro puede funcionar en tres modos:
 *
 *   FS_MADGWICK_MODE_GYRO_ONLY
 *       Usa solamente el giroscopio.
 *       La orientación puede acumular deriva con el tiempo.
 *
 *   FS_MADGWICK_MODE_IMU
 *       Usa giroscopio y acelerómetro.
 *       Corrige roll y pitch mediante la dirección de la gravedad.
 *       No puede corregir completamente la deriva de yaw.
 *
 *   FS_MADGWICK_MODE_MARG
 *       Usa giroscopio, acelerómetro y magnetómetro.
 *       Corrige roll, pitch y yaw.
 *
 * MARG significa:
 *
 *   Magnetic, Angular Rate and Gravity.
 */

#include "floatsat_madgwick.h"

/*
 * math.h proporciona:
 *
 *   sqrtf()  raíz cuadrada.
 *   atan2f() ángulo con identificación correcta del cuadrante.
 *   asinf()  arco seno.
 *   acosf()  arco coseno.
 */
#include <math.h>

/*
 * stddef.h proporciona la definición de NULL.
 */
#include <stddef.h>


/* -------------------------------------------------------------------------- */
/* Constantes matemáticas                                                     */
/* -------------------------------------------------------------------------- */

/*
 * Conversión de grados a radianes:
 *
 *   radianes = grados × pi / 180
 *
 * El algoritmo de integración del cuaternión necesita las velocidades
 * angulares del giroscopio en radianes por segundo.
 */
#define FS_MADGWICK_DEG_TO_RAD  0.01745329251994329577f

/*
 * Conversión de radianes a grados:
 *
 *   grados = radianes × 180 / pi
 *
 * Se utiliza al calcular roll, pitch, yaw y tilt.
 */
#define FS_MADGWICK_RAD_TO_DEG  57.295779513082320876f

/*
 * Norma mínima considerada válida.
 *
 * Evita:
 *
 *   - Divisiones entre cero.
 *   - Normalizar vectores prácticamente nulos.
 *   - Generar valores infinitos o NaN.
 */
#define FS_MADGWICK_MIN_NORM    1.0e-9f


/* -------------------------------------------------------------------------- */
/* Funciones privadas auxiliares                                              */
/* -------------------------------------------------------------------------- */

/**
 * @brief Limita un valor dentro de un intervalo.
 *
 * Esta función se utiliza principalmente antes de asinf() y acosf().
 *
 * Debido a errores numéricos, un valor que matemáticamente debería ser
 * 1.0 podría terminar siendo 1.0000001. Las funciones trigonométricas
 * inversas no aceptan valores fuera de [-1, 1].
 *
 * @param value   Valor que se desea limitar.
 * @param minimum Límite inferior.
 * @param maximum Límite superior.
 *
 * @return Valor limitado dentro de [minimum, maximum].
 */
static float FS_Madgwick_Clamp(float value,
                               float minimum,
                               float maximum)
{
    if (value < minimum)
    {
        return minimum;
    }

    if (value > maximum)
    {
        return maximum;
    }

    return value;
}


/**
 * @brief Lleva un ángulo al intervalo [0°, 360°).
 *
 * Ejemplos:
 *
 *   370°  -> 10°
 *   -20°  -> 340°
 *   720°  -> 0°
 *
 * Se utiliza para presentar yaw como una dirección continua entre
 * cero y 360 grados.
 */
static float FS_Madgwick_Wrap360(float angle_deg)
{
    while (angle_deg >= 360.0f)
    {
        angle_deg -= 360.0f;
    }

    while (angle_deg < 0.0f)
    {
        angle_deg += 360.0f;
    }

    return angle_deg;
}


/**
 * @brief Normaliza el cuaternión almacenado en el filtro.
 *
 * Un cuaternión que representa una orientación debe tener norma unitaria:
 *
 *   sqrt(q0² + q1² + q2² + q3²) = 1
 *
 * La integración numérica introduce pequeños errores, por lo que el
 * cuaternión debe normalizarse después de cada actualización.
 *
 * @param filter Estructura del filtro Madgwick.
 *
 * @return
 *   1U si el cuaternión pudo normalizarse.
 *   0U si su norma era demasiado pequeña.
 */
static uint8_t FS_Madgwick_NormalizeQuaternion(
        FS_Madgwick_t *filter)
{
    float norm;

    /*
     * Calcular la magnitud del cuaternión.
     */
    norm = sqrtf(
        filter->q0 * filter->q0 +
        filter->q1 * filter->q1 +
        filter->q2 * filter->q2 +
        filter->q3 * filter->q3);

    /*
     * Una norma prácticamente nula no puede normalizarse.
     */
    if (norm < FS_MADGWICK_MIN_NORM)
    {
        return 0U;
    }

    /*
     * Calcular una sola vez el inverso de la norma.
     *
     * Multiplicar por 1/norma es más eficiente que dividir
     * individualmente cada componente.
     */
    norm = 1.0f / norm;

    filter->q0 *= norm;
    filter->q1 *= norm;
    filter->q2 *= norm;
    filter->q3 *= norm;

    return 1U;
}


/**
 * @brief Convierte una matriz de rotación 3x3 en un cuaternión.
 *
 * La matriz recibida representa la orientación cuerpo -> mundo.
 *
 * Matriz:
 *
 *       | r00 r01 r02 |
 *   R = | r10 r11 r12 |
 *       | r20 r21 r22 |
 *
 * La implementación selecciona diferentes fórmulas dependiendo de la
 * traza y del elemento diagonal dominante. Esto mejora la estabilidad
 * numérica y evita divisiones por valores demasiado pequeños.
 *
 * @param q0 Componente escalar de salida.
 * @param q1 Componente X de salida.
 * @param q2 Componente Y de salida.
 * @param q3 Componente Z de salida.
 */
static void FS_Madgwick_MatrixToQuaternion(
        float r00, float r01, float r02,
        float r10, float r11, float r12,
        float r20, float r21, float r22,
        float *q0, float *q1, float *q2, float *q3)
{
    float trace;
    float s;

    /*
     * La traza es la suma de los elementos de la diagonal principal.
     */
    trace = r00 + r11 + r22;

    if (trace > 0.0f)
    {
        /*
         * Caso donde la componente escalar del cuaternión puede
         * calcularse de forma numéricamente estable.
         */
        s = sqrtf(trace + 1.0f) * 2.0f;

        *q0 = 0.25f * s;
        *q1 = (r21 - r12) / s;
        *q2 = (r02 - r20) / s;
        *q3 = (r10 - r01) / s;
    }
    else if ((r00 > r11) && (r00 > r22))
    {
        /*
         * r00 es el elemento diagonal dominante.
         * Se calcula primero la componente q1.
         */
        s = sqrtf(1.0f + r00 - r11 - r22) * 2.0f;

        *q0 = (r21 - r12) / s;
        *q1 = 0.25f * s;
        *q2 = (r01 + r10) / s;
        *q3 = (r02 + r20) / s;
    }
    else if (r11 > r22)
    {
        /*
         * r11 es el elemento diagonal dominante.
         * Se calcula primero la componente q2.
         */
        s = sqrtf(1.0f + r11 - r00 - r22) * 2.0f;

        *q0 = (r02 - r20) / s;
        *q1 = (r01 + r10) / s;
        *q2 = 0.25f * s;
        *q3 = (r12 + r21) / s;
    }
    else
    {
        /*
         * r22 es el elemento diagonal dominante.
         * Se calcula primero la componente q3.
         */
        s = sqrtf(1.0f + r22 - r00 - r11) * 2.0f;

        *q0 = (r10 - r01) / s;
        *q1 = (r02 + r20) / s;
        *q2 = (r12 + r21) / s;
        *q3 = 0.25f * s;
    }
}


/**
 * @brief Integra la derivada del cuaternión durante un periodo de muestra.
 *
 * Se utiliza integración de Euler:
 *
 *   q_nuevo = q_anterior + q_dot × dt
 *
 * Donde:
 *
 *   q_dot = derivada temporal del cuaternión.
 *   dt    = periodo de muestreo en segundos.
 */
static void FS_Madgwick_Integrate(
        FS_Madgwick_t *filter,
        float q_dot0,
        float q_dot1,
        float q_dot2,
        float q_dot3)
{
    filter->q0 += q_dot0 * filter->sample_period_s;
    filter->q1 += q_dot1 * filter->sample_period_s;
    filter->q2 += q_dot2 * filter->sample_period_s;
    filter->q3 += q_dot3 * filter->sample_period_s;
}


/* -------------------------------------------------------------------------- */
/* Inicialización del filtro                                                  */
/* -------------------------------------------------------------------------- */

/**
 * @brief Reinicia la estructura del filtro Madgwick.
 *
 * El cuaternión se coloca inicialmente como identidad:
 *
 *   q = [1, 0, 0, 0]
 *
 * Esto representa una orientación sin rotación respecto al marco mundial.
 *
 * @param filter          Estructura del filtro.
 * @param beta            Ganancia de corrección Madgwick.
 * @param sample_period_s Periodo entre actualizaciones, en segundos.
 *
 * Un beta mayor:
 *
 *   - Corrige más rápido con acelerómetro/magnetómetro.
 *   - Puede introducir más ruido.
 *
 * Un beta menor:
 *
 *   - Produce una salida más suave.
 *   - Corrige la deriva más lentamente.
 */
void FS_Madgwick_Reset(FS_Madgwick_t *filter,
                       float beta,
                       float sample_period_s)
{
    if (filter == NULL)
    {
        return;
    }

    /*
     * Limpiar completamente la estructura.
     */
    *filter = (FS_Madgwick_t){0};

    /*
     * Cuaternión identidad.
     */
    filter->q0 = 1.0f;

    /*
     * Parámetros del algoritmo.
     */
    filter->beta = beta;
    filter->sample_period_s = sample_period_s;

    /*
     * Antes de una actualización válida, se considera que el modo
     * disponible es únicamente giroscopio.
     */
    filter->last_mode = FS_MADGWICK_MODE_GYRO_ONLY;

    /*
     * Reset no equivale a inicialización completa.
     *
     * La orientación inicial debe establecerse posteriormente mediante
     * FS_Madgwick_InitializeFromBodyAxesENU().
     */
    filter->initialized = 0U;
    filter->valid = 0U;
}


/**
 * @brief Inicializa el cuaternión mediante los ejes del cuerpo en el mundo.
 *
 * Las entradas indican cómo están orientados los ejes X, Y y Z del cuerpo
 * dentro del marco mundial ENU:
 *
 *   ENU:
 *     índice 0 = Este.
 *     índice 1 = Norte.
 *     índice 2 = Arriba.
 *
 * El filtro trabaja internamente en un marco NWU:
 *
 *   NWU:
 *     X = Norte.
 *     Y = Oeste.
 *     Z = Arriba.
 *
 * @param body_x_world_enu Vector del eje X corporal expresado en ENU.
 * @param body_y_world_enu Vector del eje Y corporal expresado en ENU.
 * @param body_z_world_enu Vector del eje Z corporal expresado en ENU.
 */
FS_MadgwickStatus_t FS_Madgwick_InitializeFromBodyAxesENU(
        FS_Madgwick_t *filter,
        const float body_x_world_enu[3],
        const float body_y_world_enu[3],
        const float body_z_world_enu[3])
{
    /*
     * Elementos de la matriz cuerpo -> mundo NWU.
     */
    float r00, r01, r02;
    float r10, r11, r12;
    float r20, r21, r22;

    /*
     * Todos los punteros son obligatorios.
     */
    if ((filter == NULL) ||
        (body_x_world_enu == NULL) ||
        (body_y_world_enu == NULL) ||
        (body_z_world_enu == NULL))
    {
        return FS_MADGWICK_INVALID_ARGUMENT;
    }

    /*
     * Validar parámetros del filtro.
     *
     * beta puede ser cero, pero no negativo.
     * El periodo de muestreo debe ser estrictamente positivo.
     */
    if ((filter->beta < 0.0f) ||
        (filter->sample_period_s <= 0.0f))
    {
        return FS_MADGWICK_INVALID_CONFIGURATION;
    }

    /*
     * Conversión de ENU a NWU.
     *
     * ENU:
     *   X = Este
     *   Y = Norte
     *   Z = Arriba
     *
     * NWU:
     *   X = Norte
     *   Y = Oeste = -Este
     *   Z = Arriba
     *
     * Las columnas de la matriz son los ejes corporales expresados
     * en el marco mundial.
     */

    r00 = body_x_world_enu[1];      /* X cuerpo proyectado hacia Norte. */
    r01 = body_y_world_enu[1];      /* Y cuerpo proyectado hacia Norte. */
    r02 = body_z_world_enu[1];      /* Z cuerpo proyectado hacia Norte. */

    r10 = -body_x_world_enu[0];     /* X cuerpo proyectado hacia Oeste. */
    r11 = -body_y_world_enu[0];     /* Y cuerpo proyectado hacia Oeste. */
    r12 = -body_z_world_enu[0];     /* Z cuerpo proyectado hacia Oeste. */

    r20 = body_x_world_enu[2];      /* X cuerpo proyectado hacia Arriba. */
    r21 = body_y_world_enu[2];      /* Y cuerpo proyectado hacia Arriba. */
    r22 = body_z_world_enu[2];      /* Z cuerpo proyectado hacia Arriba. */

    /*
     * Convertir la matriz de orientación inicial a cuaternión.
     */
    FS_Madgwick_MatrixToQuaternion(
        r00, r01, r02,
        r10, r11, r12,
        r20, r21, r22,
        &filter->q0,
        &filter->q1,
        &filter->q2,
        &filter->q3);

    /*
     * La conversión debería producir un cuaternión unitario, pero se
     * normaliza para eliminar errores numéricos.
     */
    if (FS_Madgwick_NormalizeQuaternion(filter) == 0U)
    {
        filter->valid = 0U;

        return FS_MADGWICK_INVALID_QUATERNION;
    }

    filter->initialized = 1U;
    filter->valid = 1U;

    /*
     * Todavía no se han realizado actualizaciones dinámicas.
     */
    filter->update_count = 0U;

    /*
     * El modo se actualizará en la primera llamada a Update().
     */
    filter->last_mode = FS_MADGWICK_MODE_GYRO_ONLY;

    return FS_MADGWICK_OK;
}


/* -------------------------------------------------------------------------- */
/* Actualización del filtro                                                   */
/* -------------------------------------------------------------------------- */

/**
 * @brief Actualiza la orientación utilizando los sensores disponibles.
 *
 * Orden de preferencia:
 *
 *   1. Giroscopio + acelerómetro + magnetómetro: MARG.
 *   2. Giroscopio + acelerómetro: IMU.
 *   3. Solamente giroscopio: GYRO_ONLY.
 *
 * El giroscopio predice la evolución de la orientación.
 * El acelerómetro y el magnetómetro corrigen la deriva mediante
 * descenso de gradiente.
 */
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
        uint8_t magnetometer_valid)
{
    /*
     * Copia local del cuaternión actual.
     *
     * Utilizar variables locales reduce accesos repetidos a la estructura.
     */
    float q0, q1, q2, q3;

    /*
     * Velocidades del giroscopio convertidas a rad/s.
     */
    float gx, gy, gz;

    /*
     * Derivada temporal del cuaternión.
     */
    float q_dot0, q_dot1, q_dot2, q_dot3;

    /*
     * Componentes del gradiente de corrección Madgwick.
     */
    float s0, s1, s2, s3;

    /*
     * Variable reutilizada para normalizar vectores.
     */
    float norm;

    if (filter == NULL)
    {
        return FS_MADGWICK_INVALID_ARGUMENT;
    }

    /*
     * No actualizar un filtro:
     *
     *   - No inicializado.
     *   - Con periodo inválido.
     *   - Con beta negativo.
     */
    if ((filter->initialized == 0U) ||
        (filter->sample_period_s <= 0.0f) ||
        (filter->beta < 0.0f))
    {
        return FS_MADGWICK_INVALID_CONFIGURATION;
    }

    /*
     * Copiar orientación actual.
     */
    q0 = filter->q0;
    q1 = filter->q1;
    q2 = filter->q2;
    q3 = filter->q3;

    /*
     * El giroscopio se recibe en grados por segundo, pero la ecuación
     * cinemática del cuaternión necesita radianes por segundo.
     */
    gx = gyro_x_dps * FS_MADGWICK_DEG_TO_RAD;
    gy = gyro_y_dps * FS_MADGWICK_DEG_TO_RAD;
    gz = gyro_z_dps * FS_MADGWICK_DEG_TO_RAD;

    /*
     * Derivada del cuaternión producida únicamente por el giroscopio:
     *
     *   q_dot = 0.5 × q ⊗ omega
     *
     * donde ⊗ representa la multiplicación de cuaterniones.
     */
    q_dot0 = 0.5f * (-q1 * gx - q2 * gy - q3 * gz);
    q_dot1 = 0.5f * ( q0 * gx + q2 * gz - q3 * gy);
    q_dot2 = 0.5f * ( q0 * gy - q1 * gz + q3 * gx);
    q_dot3 = 0.5f * ( q0 * gz + q1 * gy - q2 * gx);

    /*
     * El modo inicial de cada actualización es gyro-only.
     *
     * Se cambiará a IMU o MARG si existen sensores auxiliares válidos.
     */
    filter->last_mode = FS_MADGWICK_MODE_GYRO_ONLY;

    /*
     * El acelerómetro es necesario tanto para el modo IMU como MARG.
     */
    if (accelerometer_valid != 0U)
    {
        /*
         * Calcular la magnitud del vector de aceleración.
         */
        norm = sqrtf(
            accel_x_g * accel_x_g +
            accel_y_g * accel_y_g +
            accel_z_g * accel_z_g);

        /*
         * Solo normalizar si el vector no es prácticamente cero.
         */
        if (norm > FS_MADGWICK_MIN_NORM)
        {
            /*
             * Normalizar acelerómetro.
             *
             * Después de normalizar solo se conserva la dirección
             * del vector, no su magnitud.
             */
            accel_x_g /= norm;
            accel_y_g /= norm;
            accel_z_g /= norm;

            /*
             * Intentar utilizar también el magnetómetro.
             */
            if (magnetometer_valid != 0U)
            {
                /*
                 * Campo magnético terrestre estimado en el marco mundial.
                 */
                float hx, hy;
                float bx, bz;

                /*
                 * Productos del cuaternión reutilizados muchas veces.
                 *
                 * Se calculan una vez para reducir operaciones.
                 */
                float q0q0, q0q1, q0q2, q0q3;
                float q1q1, q1q2, q1q3;
                float q2q2, q2q3;
                float q3q3;

                /*
                 * Productos auxiliares usados por las ecuaciones MARG.
                 */
                float two_q0mx;
                float two_q0my;
                float two_q0mz;
                float two_q1mx;

                float two_bx;
                float two_bz;
                float four_bx;
                float four_bz;

                /*
                 * Magnitud del vector magnético.
                 */
                norm = sqrtf(
                    mag_x_gauss * mag_x_gauss +
                    mag_y_gauss * mag_y_gauss +
                    mag_z_gauss * mag_z_gauss);

                /*
                 * Si el magnetómetro tiene una norma válida, utilizar
                 * la actualización completa MARG.
                 */
                if (norm > FS_MADGWICK_MIN_NORM)
                {
                    /*
                     * Normalizar el magnetómetro.
                     */
                    mag_x_gauss /= norm;
                    mag_y_gauss /= norm;
                    mag_z_gauss /= norm;

                    /*
                     * Productos del cuaternión.
                     */
                    q0q0 = q0 * q0;
                    q0q1 = q0 * q1;
                    q0q2 = q0 * q2;
                    q0q3 = q0 * q3;

                    q1q1 = q1 * q1;
                    q1q2 = q1 * q2;
                    q1q3 = q1 * q3;

                    q2q2 = q2 * q2;
                    q2q3 = q2 * q3;

                    q3q3 = q3 * q3;

                    /*
                     * Productos auxiliares con el campo magnético.
                     */
                    two_q0mx = 2.0f * q0 * mag_x_gauss;
                    two_q0my = 2.0f * q0 * mag_y_gauss;
                    two_q0mz = 2.0f * q0 * mag_z_gauss;
                    two_q1mx = 2.0f * q1 * mag_x_gauss;

                    /*
                     * Proyectar el campo magnético medido al marco mundial.
                     *
                     * hx y hy representan sus componentes horizontales.
                     */
                    hx =
                        mag_x_gauss * q0q0 -
                        two_q0my * q3 +
                        two_q0mz * q2 +
                        mag_x_gauss * q1q1 +
                        2.0f * q1 * mag_y_gauss * q2 +
                        2.0f * q1 * mag_z_gauss * q3 -
                        mag_x_gauss * q2q2 -
                        mag_x_gauss * q3q3;

                    hy =
                        two_q0mx * q3 +
                        mag_y_gauss * q0q0 -
                        two_q0mz * q1 +
                        two_q1mx * q2 -
                        mag_y_gauss * q1q1 +
                        mag_y_gauss * q2q2 +
                        2.0f * q2 * mag_z_gauss * q3 -
                        mag_y_gauss * q3q3;

                    /*
                     * Componente horizontal estimada del campo terrestre.
                     */
                    bx = sqrtf(hx * hx + hy * hy);

                    /*
                     * Componente vertical estimada del campo terrestre.
                     */
                    bz =
                        -two_q0mx * q2 +
                        two_q0my * q1 +
                        mag_z_gauss * q0q0 +
                        two_q1mx * q3 -
                        mag_z_gauss * q1q1 +
                        2.0f * q2 * mag_y_gauss * q3 -
                        mag_z_gauss * q2q2 +
                        mag_z_gauss * q3q3;

                    /*
                     * Múltiplos reutilizados en el gradiente.
                     */
                    two_bx = 2.0f * bx;
                    two_bz = 2.0f * bz;

                    four_bx = 2.0f * two_bx;
                    four_bz = 2.0f * two_bz;

                    /*
                     * Gradiente de la función de error MARG.
                     *
                     * Estas expresiones comparan:
                     *
                     *   - La gravedad predicha por el cuaternión con
                     *     el acelerómetro normalizado.
                     *
                     *   - El campo magnético predicho con el
                     *     magnetómetro normalizado.
                     *
                     * s0, s1, s2 y s3 indican la dirección en la que
                     * debe corregirse el cuaternión.
                     */
                    s0 =
                        -2.0f * q2 *
                            (2.0f * (q1q3 - q0q2) -
                             accel_x_g) +

                         2.0f * q1 *
                            (2.0f * (q0q1 + q2q3) -
                             accel_y_g) -

                         two_bz * q2 *
                            (two_bx *
                                (0.5f - q2q2 - q3q3) +
                             two_bz *
                                (q1q3 - q0q2) -
                             mag_x_gauss) +

                         (-two_bx * q3 + two_bz * q1) *
                            (two_bx *
                                (q1q2 - q0q3) +
                             two_bz *
                                (q0q1 + q2q3) -
                             mag_y_gauss) +

                         two_bx * q2 *
                            (two_bx *
                                (q0q2 + q1q3) +
                             two_bz *
                                (0.5f - q1q1 - q2q2) -
                             mag_z_gauss);

                    s1 =
                         2.0f * q3 *
                            (2.0f * (q1q3 - q0q2) -
                             accel_x_g) +

                         2.0f * q0 *
                            (2.0f * (q0q1 + q2q3) -
                             accel_y_g) -

                         4.0f * q1 *
                            (1.0f -
                             2.0f * (q1q1 + q2q2) -
                             accel_z_g) +

                         two_bz * q3 *
                            (two_bx *
                                (0.5f - q2q2 - q3q3) +
                             two_bz *
                                (q1q3 - q0q2) -
                             mag_x_gauss) +

                         (two_bx * q2 + two_bz * q0) *
                            (two_bx *
                                (q1q2 - q0q3) +
                             two_bz *
                                (q0q1 + q2q3) -
                             mag_y_gauss) +

                         (two_bx * q3 - four_bz * q1) *
                            (two_bx *
                                (q0q2 + q1q3) +
                             two_bz *
                                (0.5f - q1q1 - q2q2) -
                             mag_z_gauss);

                    s2 =
                        -2.0f * q0 *
                            (2.0f * (q1q3 - q0q2) -
                             accel_x_g) +

                         2.0f * q3 *
                            (2.0f * (q0q1 + q2q3) -
                             accel_y_g) -

                         4.0f * q2 *
                            (1.0f -
                             2.0f * (q1q1 + q2q2) -
                             accel_z_g) +

                         (-four_bx * q2 - two_bz * q0) *
                            (two_bx *
                                (0.5f - q2q2 - q3q3) +
                             two_bz *
                                (q1q3 - q0q2) -
                             mag_x_gauss) +

                         (two_bx * q1 + two_bz * q3) *
                            (two_bx *
                                (q1q2 - q0q3) +
                             two_bz *
                                (q0q1 + q2q3) -
                             mag_y_gauss) +

                         (two_bx * q0 - four_bz * q2) *
                            (two_bx *
                                (q0q2 + q1q3) +
                             two_bz *
                                (0.5f - q1q1 - q2q2) -
                             mag_z_gauss);

                    s3 =
                         2.0f * q1 *
                            (2.0f * (q1q3 - q0q2) -
                             accel_x_g) +

                         2.0f * q2 *
                            (2.0f * (q0q1 + q2q3) -
                             accel_y_g) +

                         (-four_bx * q3 + two_bz * q1) *
                            (two_bx *
                                (0.5f - q2q2 - q3q3) +
                             two_bz *
                                (q1q3 - q0q2) -
                             mag_x_gauss) +

                         (-two_bx * q0 + two_bz * q2) *
                            (two_bx *
                                (q1q2 - q0q3) +
                             two_bz *
                                (q0q1 + q2q3) -
                             mag_y_gauss) +

                         two_bx * q1 *
                            (two_bx *
                                (q0q2 + q1q3) +
                             two_bz *
                                (0.5f - q1q1 - q2q2) -
                             mag_z_gauss);

                    /*
                     * Calcular la magnitud del gradiente.
                     */
                    norm = sqrtf(
                        s0 * s0 +
                        s1 * s1 +
                        s2 * s2 +
                        s3 * s3);

                    /*
                     * Normalizar el gradiente antes de aplicarlo.
                     */
                    if (norm > FS_MADGWICK_MIN_NORM)
                    {
                        norm = 1.0f / norm;

                        s0 *= norm;
                        s1 *= norm;
                        s2 *= norm;
                        s3 *= norm;

                        /*
                         * Corregir la derivada calculada por el giroscopio.
                         *
                         * beta determina la fuerza de esta corrección.
                         */
                        q_dot0 -= filter->beta * s0;
                        q_dot1 -= filter->beta * s1;
                        q_dot2 -= filter->beta * s2;
                        q_dot3 -= filter->beta * s3;
                    }

                    /*
                     * La actualización utilizó los tres sensores.
                     */
                    filter->last_mode =
                        FS_MADGWICK_MODE_MARG;
                }
            }

            /*
             * Si el magnetómetro no era válido o su norma era demasiado
             * pequeña, utilizar solamente acelerómetro y giroscopio.
             */
            if (filter->last_mode !=
                FS_MADGWICK_MODE_MARG)
            {
                /*
                 * Productos auxiliares para la variante IMU del algoritmo.
                 */
                float two_q0 = 2.0f * q0;
                float two_q1 = 2.0f * q1;
                float two_q2 = 2.0f * q2;
                float two_q3 = 2.0f * q3;

                float four_q0 = 4.0f * q0;
                float four_q1 = 4.0f * q1;
                float four_q2 = 4.0f * q2;

                float eight_q1 = 8.0f * q1;
                float eight_q2 = 8.0f * q2;

                float q0q0 = q0 * q0;
                float q1q1 = q1 * q1;
                float q2q2 = q2 * q2;
                float q3q3 = q3 * q3;

                /*
                 * Gradiente del error entre:
                 *
                 *   - Dirección de gravedad predicha por el cuaternión.
                 *   - Dirección medida por el acelerómetro.
                 *
                 * Esta corrección estabiliza roll y pitch.
                 */
                s0 =
                    four_q0 * q2q2 +
                    two_q2 * accel_x_g +
                    four_q0 * q1q1 -
                    two_q1 * accel_y_g;

                s1 =
                    four_q1 * q3q3 -
                    two_q3 * accel_x_g +
                    4.0f * q0q0 * q1 -
                    two_q0 * accel_y_g -
                    four_q1 +
                    eight_q1 * q1q1 +
                    eight_q1 * q2q2 +
                    four_q1 * accel_z_g;

                s2 =
                    4.0f * q0q0 * q2 +
                    two_q0 * accel_x_g +
                    four_q2 * q3q3 -
                    two_q3 * accel_y_g -
                    four_q2 +
                    eight_q2 * q1q1 +
                    eight_q2 * q2q2 +
                    four_q2 * accel_z_g;

                s3 =
                    4.0f * q1q1 * q3 -
                    two_q1 * accel_x_g +
                    4.0f * q2q2 * q3 -
                    two_q2 * accel_y_g;

                /*
                 * Normalizar el gradiente.
                 */
                norm = sqrtf(
                    s0 * s0 +
                    s1 * s1 +
                    s2 * s2 +
                    s3 * s3);

                if (norm > FS_MADGWICK_MIN_NORM)
                {
                    norm = 1.0f / norm;

                    s0 *= norm;
                    s1 *= norm;
                    s2 *= norm;
                    s3 *= norm;

                    /*
                     * Aplicar la corrección del acelerómetro.
                     */
                    q_dot0 -= filter->beta * s0;
                    q_dot1 -= filter->beta * s1;
                    q_dot2 -= filter->beta * s2;
                    q_dot3 -= filter->beta * s3;
                }

                filter->last_mode =
                    FS_MADGWICK_MODE_IMU;
            }
        }
    }

    /*
     * Integrar la derivada corregida durante el periodo de muestra.
     */
    FS_Madgwick_Integrate(
        filter,
        q_dot0,
        q_dot1,
        q_dot2,
        q_dot3);

    /*
     * La integración de Euler puede alejar el cuaternión de norma 1.
     * Debe normalizarse después de cada actualización.
     */
    if (FS_Madgwick_NormalizeQuaternion(filter) == 0U)
    {
        filter->valid = 0U;

        return FS_MADGWICK_INVALID_QUATERNION;
    }

    /*
     * Registrar que se completó una actualización válida.
     */
    filter->update_count++;
    filter->valid = 1U;

    return FS_MADGWICK_OK;
}


/* -------------------------------------------------------------------------- */
/* Obtención de resultados                                                    */
/* -------------------------------------------------------------------------- */

/**
 * @brief Convierte el cuaternión actual en ángulos y vectores de orientación.
 *
 * Produce:
 *
 *   - Cuaternión.
 *   - Vectores de los ejes corporales expresados en ENU.
 *   - Roll.
 *   - Pitch.
 *   - Yaw.
 *   - Tilt.
 *   - Modo utilizado en la última actualización.
 */
FS_MadgwickStatus_t FS_Madgwick_GetOutput(
        const FS_Madgwick_t *filter,
        FS_MadgwickOutput_t *output)
{
    float q0, q1, q2, q3;

    /*
     * Elementos de la matriz cuerpo -> mundo NWU.
     */
    float r00, r01, r02;
    float r10, r11, r12;
    float r20, r21, r22;

    if ((filter == NULL) || (output == NULL))
    {
        return FS_MADGWICK_INVALID_ARGUMENT;
    }

    /*
     * Limpiar la estructura de salida antes de llenarla.
     */
    *output = (FS_MadgwickOutput_t){0};

    /*
     * No se pueden generar resultados si el filtro no ha sido
     * inicializado o si el cuaternión quedó inválido.
     */
    if ((filter->initialized == 0U) ||
        (filter->valid == 0U))
    {
        return FS_MADGWICK_INVALID_QUATERNION;
    }

    /*
     * Copiar el cuaternión actual.
     */
    q0 = filter->q0;
    q1 = filter->q1;
    q2 = filter->q2;
    q3 = filter->q3;

    /*
     * Convertir cuaternión a matriz de rotación cuerpo -> mundo NWU.
     *
     * Las columnas representan los ejes X, Y y Z del cuerpo
     * expresados en el marco mundial.
     */
    r00 = 1.0f - 2.0f * (q2 * q2 + q3 * q3);
    r01 = 2.0f * (q1 * q2 - q0 * q3);
    r02 = 2.0f * (q1 * q3 + q0 * q2);

    r10 = 2.0f * (q1 * q2 + q0 * q3);
    r11 = 1.0f - 2.0f * (q1 * q1 + q3 * q3);
    r12 = 2.0f * (q2 * q3 - q0 * q1);

    r20 = 2.0f * (q1 * q3 - q0 * q2);
    r21 = 2.0f * (q2 * q3 + q0 * q1);
    r22 = 1.0f - 2.0f * (q1 * q1 + q2 * q2);

    /*
     * Copiar el cuaternión a la estructura de salida.
     */
    output->q0 = q0;
    output->q1 = q1;
    output->q2 = q2;
    output->q3 = q3;

    /*
     * Convertir los vectores desde NWU hacia ENU.
     *
     * Relación:
     *
     *   Este  = -Oeste
     *   Norte = Norte
     *   Arriba = Arriba
     *
     * body_x_world representa el eje X corporal expresado en ENU.
     */
    output->body_x_world[0] = -r10;  /* Componente Este. */
    output->body_x_world[1] =  r00;  /* Componente Norte. */
    output->body_x_world[2] =  r20;  /* Componente Arriba. */

    /*
     * Eje Y corporal expresado en ENU.
     */
    output->body_y_world[0] = -r11;
    output->body_y_world[1] =  r01;
    output->body_y_world[2] =  r21;

    /*
     * Eje Z corporal expresado en ENU.
     */
    output->body_z_world[0] = -r12;
    output->body_z_world[1] =  r02;
    output->body_z_world[2] =  r22;

    /*
     * Roll:
     *
     * Rotación alrededor del eje X corporal.
     *
     * atan2f permite conservar correctamente el cuadrante.
     */
    output->roll_deg =
        atan2f(r21, r22) *
        FS_MADGWICK_RAD_TO_DEG;

    /*
     * Pitch:
     *
     * Rotación alrededor del eje Y.
     *
     * Clamp evita que errores numéricos produzcan un argumento
     * ligeramente fuera del intervalo permitido por asinf().
     */
    output->pitch_deg =
        asinf(
            FS_Madgwick_Clamp(
                -r20,
                -1.0f,
                 1.0f)) *
        FS_MADGWICK_RAD_TO_DEG;

    /*
     * Yaw:
     *
     * Rotación horizontal alrededor del eje vertical.
     *
     * El resultado de atan2f está inicialmente entre -180° y 180°.
     * Wrap360 lo convierte al intervalo [0°, 360°).
     */
    output->yaw_deg =
        FS_Madgwick_Wrap360(
            atan2f(-r10, r00) *
            FS_MADGWICK_RAD_TO_DEG);

    /*
     * Tilt:
     *
     * Ángulo total entre el eje Z del cuerpo y el eje vertical mundial.
     *
     *   tilt = 0°   -> cuerpo completamente vertical.
     *   tilt = 90°  -> cuerpo acostado lateralmente.
     *   tilt = 180° -> cuerpo invertido.
     */
    output->tilt_deg =
        acosf(
            FS_Madgwick_Clamp(
                r22,
                -1.0f,
                 1.0f)) *
        FS_MADGWICK_RAD_TO_DEG;

    /*
     * Informar qué combinación de sensores se utilizó en la última
     * actualización.
     */
    output->mode = filter->last_mode;

    /*
     * Marcar la estructura de salida como válida.
     */
    output->valid = 1U;

    return FS_MADGWICK_OK;
}
