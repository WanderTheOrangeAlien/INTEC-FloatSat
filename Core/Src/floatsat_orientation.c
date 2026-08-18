/*
 * Cálculo de orientación inicial a partir del acelerómetro
 * y del magnetómetro.
 *
 * Este módulo construye un sistema de referencia tridimensional:
 *
 *   - Arriba: obtenido a partir del acelerómetro.
 *   - Norte: obtenido proyectando el campo magnético sobre
 *            el plano horizontal.
 *   - Este: obtenido mediante producto cruzado.
 *
 * Con estos vectores se forma una matriz de orientación cuerpo -> mundo,
 * de la cual se calculan:
 *
 *   - Roll.
 *   - Pitch.
 *   - Yaw o rumbo magnético.
 *   - Inclinación total del eje Z.
 *
 * Esta orientación es instantánea y no integra el giroscopio.
 * Resulta especialmente útil para inicializar el filtro Madgwick.
 */

#include "floatsat_orientation.h"

/*
 * math.h proporciona:
 *
 *   sqrtf()  : raíz cuadrada.
 *   atan2f() : arco tangente conservando el cuadrante.
 *   asinf()  : arco seno.
 *   acosf()  : arco coseno.
 */
#include <math.h>


/* -------------------------------------------------------------------------- */
/* Constantes matemáticas                                                     */
/* -------------------------------------------------------------------------- */

/*
 * Conversión de radianes a grados:
 *
 *   grados = radianes × 180 / pi
 */
#define FS_RAD_TO_DEG             57.2957795131f

/*
 * Norma mínima aceptada para normalizar un vector.
 *
 * Evita dividir entre cero o entre un valor extremadamente pequeño.
 */
#define FS_VECTOR_MIN_NORM        0.000001f


/* -------------------------------------------------------------------------- */
/* Límites de confiabilidad del acelerómetro                                  */
/* -------------------------------------------------------------------------- */

/*
 * Cuando la IMU está en reposo, la magnitud del acelerómetro debería
 * estar aproximadamente alrededor de 1 g.
 *
 * Se considera inicialmente confiable cuando:
 *
 *   0.75 g <= |acelerómetro| <= 1.25 g
 *
 * Este intervalo es relativamente amplio para permitir cierto ruido
 * y pequeñas aceleraciones.
 */
#define FS_ACCEL_MIN_RELIABLE_G   0.75f
#define FS_ACCEL_MAX_RELIABLE_G   1.25f


/* -------------------------------------------------------------------------- */
/* Límites de confiabilidad del magnetómetro                                  */
/* -------------------------------------------------------------------------- */

/*
 * Intervalo amplio para aceptar la magnitud del campo magnético.
 *
 * Esta comprobación no reemplaza una calibración magnética.
 * Solamente ayuda a detectar:
 *
 *   - Un vector prácticamente nulo.
 *   - Una lectura excesivamente grande.
 *   - Una posible saturación o dato corrupto.
 */
#define FS_MAG_MIN_RELIABLE_GAUSS 0.02f
#define FS_MAG_MAX_RELIABLE_GAUSS 3.50f


/* -------------------------------------------------------------------------- */
/* Funciones vectoriales privadas                                             */
/* -------------------------------------------------------------------------- */

/**
 * @brief Calcula la magnitud de un vector tridimensional.
 *
 * Para un vector:
 *
 *   v = [x, y, z]
 *
 * su norma es:
 *
 *   |v| = sqrt(x² + y² + z²)
 *
 * @param vector Vector de tres componentes.
 *
 * @return Magnitud del vector.
 */
static float FS_VectorNorm(const float vector[3])
{
    return sqrtf(
            vector[0] * vector[0] +
            vector[1] * vector[1] +
            vector[2] * vector[2]);
}


/**
 * @brief Normaliza un vector tridimensional.
 *
 * Después de normalizar:
 *
 *   |vector| = 1
 *
 * La función modifica directamente el vector recibido.
 *
 * @param vector Vector que se desea normalizar.
 *
 * @return
 *   1U si el vector pudo normalizarse.
 *   0U si su magnitud era demasiado pequeña.
 */
static uint8_t FS_VectorNormalize(float vector[3])
{
    float norm;

    norm = FS_VectorNorm(vector);   /* Calcular longitud del vector. */

    /*
     * No se puede normalizar un vector nulo o casi nulo.
     */
    if (norm < FS_VECTOR_MIN_NORM)
    {
        return 0U;
    }

    /*
     * Dividir cada componente entre la magnitud.
     */
    vector[0] /= norm;
    vector[1] /= norm;
    vector[2] /= norm;

    return 1U;
}


/**
 * @brief Calcula el producto punto entre dos vectores.
 *
 * Para:
 *
 *   a = [ax, ay, az]
 *   b = [bx, by, bz]
 *
 *   a · b = ax*bx + ay*by + az*bz
 *
 * El producto punto se utiliza para determinar cuánto de un vector
 * está proyectado sobre otro.
 */
static float FS_VectorDot(const float a[3],
                          const float b[3])
{
    return a[0] * b[0] +
           a[1] * b[1] +
           a[2] * b[2];
}


/**
 * @brief Calcula el producto cruzado entre dos vectores.
 *
 * El resultado es perpendicular a los dos vectores de entrada.
 *
 * El orden es importante:
 *
 *   a × b = -(b × a)
 */
static void FS_VectorCross(const float a[3],
                           const float b[3],
                           float result[3])
{
    result[0] =
        a[1] * b[2] -
        a[2] * b[1];

    result[1] =
        a[2] * b[0] -
        a[0] * b[2];

    result[2] =
        a[0] * b[1] -
        a[1] * b[0];
}


/**
 * @brief Limita un valor dentro de un intervalo.
 *
 * Se utiliza antes de asinf() y acosf(), ya que esas funciones
 * solamente aceptan argumentos entre -1 y 1.
 */
static float FS_Clamp(float value,
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
 */
static float FS_Wrap360(float angle_deg)
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


/* -------------------------------------------------------------------------- */
/* Cálculo principal de orientación                                           */
/* -------------------------------------------------------------------------- */

/**
 * @brief Calcula la orientación instantánea usando acelerómetro
 *        y magnetómetro.
 *
 * El procedimiento es:
 *
 *   1. Copiar las mediciones de la IMU.
 *   2. Calcular la magnitud de cada sensor.
 *   3. Obtener el vector vertical a partir del acelerómetro.
 *   4. Proyectar el campo magnético sobre el plano horizontal.
 *   5. Construir los vectores Norte, Este y Arriba.
 *   6. Construir la matriz cuerpo -> mundo.
 *   7. Calcular roll, pitch, yaw y tilt.
 *
 * @param imu_data
 *        Datos escalados del acelerómetro, giroscopio y magnetómetro.
 *
 * @param orientation
 *        Estructura donde se almacenará el resultado.
 *
 * @return Estado del cálculo.
 */
FS_OrientationStatus_t FS_Orientation_Compute(
        const FS_IMU_ScaledData_t *imu_data,
        FS_Orientation_t *orientation)
{
    /*
     * Componente del campo magnético paralela al vector vertical.
     *
     * Se eliminará para obtener solamente la componente horizontal.
     */
    float magnetic_vertical_component;

    /*
     * Yaw matemático calculado en el sistema ENU.
     *
     * En este sistema, cero grados corresponde inicialmente al Este.
     */
    float yaw_enu_deg;

    /*
     * Verificar que ambos punteros sean válidos.
     */
    if ((imu_data == NULL) ||
        (orientation == NULL))
    {
        return FS_ORIENTATION_INVALID_ARGUMENT;
    }

    /*
     * Limpiar toda la estructura de salida.
     *
     * Si la función termina antes de completar el cálculo,
     * orientation->valid permanecerá en cero.
     */
    *orientation = (FS_Orientation_t){0};


    /* ---------------------------------------------------------------------- */
    /* Copia de los datos originales de la IMU                               */
    /* ---------------------------------------------------------------------- */

    /*
     * Vector del acelerómetro expresado en los ejes del cuerpo:
     *
     *   [X, Y, Z] en unidades g.
     */
    orientation->accel_body_g[0] =
        imu_data->accel_x_g;

    orientation->accel_body_g[1] =
        imu_data->accel_y_g;

    orientation->accel_body_g[2] =
        imu_data->accel_z_g;

    /*
     * Vector del giroscopio expresado en los ejes del cuerpo:
     *
     *   [X, Y, Z] en grados por segundo.
     *
     * En esta función no se utiliza para calcular la orientación,
     * pero se copia para diagnóstico y telemetría.
     */
    orientation->gyro_body_dps[0] =
        imu_data->gyro_x_dps;

    orientation->gyro_body_dps[1] =
        imu_data->gyro_y_dps;

    orientation->gyro_body_dps[2] =
        imu_data->gyro_z_dps;

    /*
     * Vector del campo magnético expresado en los ejes del cuerpo:
     *
     *   [X, Y, Z] en gauss.
     */
    orientation->mag_body_gauss[0] =
        imu_data->mag_x_gauss;

    orientation->mag_body_gauss[1] =
        imu_data->mag_y_gauss;

    orientation->mag_body_gauss[2] =
        imu_data->mag_z_gauss;


    /* ---------------------------------------------------------------------- */
    /* Magnitudes de los sensores                                            */
    /* ---------------------------------------------------------------------- */

    orientation->accel_magnitude_g =
        FS_VectorNorm(orientation->accel_body_g);

    orientation->gyro_magnitude_dps =
        FS_VectorNorm(orientation->gyro_body_dps);

    orientation->mag_magnitude_gauss =
        FS_VectorNorm(orientation->mag_body_gauss);


    /* ---------------------------------------------------------------------- */
    /* Evaluación inicial de confiabilidad                                   */
    /* ---------------------------------------------------------------------- */

    /*
     * El acelerómetro se considera confiable cuando su magnitud está
     * razonablemente cerca de 1 g.
     */
    orientation->accel_reliable =
        ((orientation->accel_magnitude_g >=
          FS_ACCEL_MIN_RELIABLE_G) &&
         (orientation->accel_magnitude_g <=
          FS_ACCEL_MAX_RELIABLE_G))
        ? 1U
        : 0U;

    /*
     * El magnetómetro se considera confiable cuando su magnitud se
     * encuentra dentro del intervalo de seguridad definido.
     */
    orientation->mag_reliable =
        ((orientation->mag_magnitude_gauss >=
          FS_MAG_MIN_RELIABLE_GAUSS) &&
         (orientation->mag_magnitude_gauss <=
          FS_MAG_MAX_RELIABLE_GAUSS))
        ? 1U
        : 0U;


    /* ---------------------------------------------------------------------- */
    /* Obtención del vector Arriba                                           */
    /* ---------------------------------------------------------------------- */

    /*
     * En reposo, el acelerómetro se utiliza como una aproximación
     * de la dirección vertical.
     *
     * Se copia inicialmente el vector completo.
     */
    orientation->up_body[0] =
        orientation->accel_body_g[0];

    orientation->up_body[1] =
        orientation->accel_body_g[1];

    orientation->up_body[2] =
        orientation->accel_body_g[2];

    /*
     * Convertirlo en un vector unitario.
     *
     * Después de normalizar, solamente importa la dirección,
     * no la magnitud original de la aceleración.
     */
    if (FS_VectorNormalize(orientation->up_body) == 0U)
    {
        return FS_ORIENTATION_ACCEL_INVALID;
    }


    /* ---------------------------------------------------------------------- */
    /* Obtención del vector Norte horizontal                                 */
    /* ---------------------------------------------------------------------- */

    /*
     * Calcular cuánto del campo magnético está alineado con la vertical:
     *
     *   componente_vertical = campo_magnético · arriba
     */
    magnetic_vertical_component =
        FS_VectorDot(
                orientation->mag_body_gauss,
                orientation->up_body);

    /*
     * Eliminar la componente vertical del campo magnético:
     *
     *   campo_horizontal =
     *       campo_magnético -
     *       componente_vertical × arriba
     *
     * El resultado queda contenido en el plano horizontal.
     */
    orientation->north_body[0] =
        orientation->mag_body_gauss[0] -
        magnetic_vertical_component *
        orientation->up_body[0];

    orientation->north_body[1] =
        orientation->mag_body_gauss[1] -
        magnetic_vertical_component *
        orientation->up_body[1];

    orientation->north_body[2] =
        orientation->mag_body_gauss[2] -
        magnetic_vertical_component *
        orientation->up_body[2];

    /*
     * Normalizar el vector Norte.
     *
     * Si la proyección horizontal es prácticamente nula, no puede
     * determinarse el rumbo magnético.
     */
    if (FS_VectorNormalize(orientation->north_body) == 0U)
    {
        return FS_ORIENTATION_MAG_INVALID;
    }


    /* ---------------------------------------------------------------------- */
    /* Obtención del vector Este                                             */
    /* ---------------------------------------------------------------------- */

    /*
     * Construir un sistema derecho ENU:
     *
     *   Este = Norte × Arriba
     */
    FS_VectorCross(
            orientation->north_body,
            orientation->up_body,
            orientation->east_body);

    if (FS_VectorNormalize(orientation->east_body) == 0U)
    {
        return FS_ORIENTATION_MAG_INVALID;
    }


    /* ---------------------------------------------------------------------- */
    /* Reajuste del vector Norte                                             */
    /* ---------------------------------------------------------------------- */

    /*
     * Recalcular Norte para garantizar que sea exactamente perpendicular
     * a Este y Arriba:
     *
     *   Norte = Arriba × Este
     *
     * Esto elimina pequeños errores de ortogonalidad provocados
     * por ruido y redondeo numérico.
     */
    FS_VectorCross(
            orientation->up_body,
            orientation->east_body,
            orientation->north_body);

    if (FS_VectorNormalize(orientation->north_body) == 0U)
    {
        return FS_ORIENTATION_MAG_INVALID;
    }


    /* ---------------------------------------------------------------------- */
    /* Construcción de la matriz cuerpo -> mundo ENU                         */
    /* ---------------------------------------------------------------------- */

    /*
     * Los vectores east_body, north_body y up_body indican cómo están
     * expresados los ejes mundiales dentro de los ejes del cuerpo.
     *
     * Al reorganizar sus componentes se obtienen los ejes corporales
     * expresados en el marco mundial ENU.
     *
     * Cada vector utiliza:
     *
     *   índice 0 = Este.
     *   índice 1 = Norte.
     *   índice 2 = Arriba.
     */

    /*
     * Eje X del cuerpo expresado en coordenadas mundiales.
     */
    orientation->body_x_world[0] =
        orientation->east_body[0];      /* Componente Este. */

    orientation->body_x_world[1] =
        orientation->north_body[0];     /* Componente Norte. */

    orientation->body_x_world[2] =
        orientation->up_body[0];        /* Componente Arriba. */

    /*
     * Eje Y del cuerpo expresado en coordenadas mundiales.
     */
    orientation->body_y_world[0] =
        orientation->east_body[1];

    orientation->body_y_world[1] =
        orientation->north_body[1];

    orientation->body_y_world[2] =
        orientation->up_body[1];

    /*
     * Eje Z del cuerpo expresado en coordenadas mundiales.
     */
    orientation->body_z_world[0] =
        orientation->east_body[2];

    orientation->body_z_world[1] =
        orientation->north_body[2];

    orientation->body_z_world[2] =
        orientation->up_body[2];


    /* ---------------------------------------------------------------------- */
    /* Conversión de matriz a ángulos de Euler Z-Y-X                         */
    /* ---------------------------------------------------------------------- */

    /*
     * Roll:
     *
     * rotación alrededor del eje X corporal.
     */
    orientation->roll_deg =
        atan2f(
                orientation->body_y_world[2],
                orientation->body_z_world[2]) *
        FS_RAD_TO_DEG;

    /*
     * Pitch:
     *
     * rotación alrededor del eje Y corporal.
     *
     * FS_Clamp evita que errores numéricos produzcan valores
     * ligeramente fuera del intervalo [-1, 1].
     */
    orientation->pitch_deg =
        asinf(
                FS_Clamp(
                        -orientation->body_x_world[2],
                        -1.0f,
                        1.0f)) *
        FS_RAD_TO_DEG;


    /* ---------------------------------------------------------------------- */
    /* Cálculo de yaw                                                        */
    /* ---------------------------------------------------------------------- */

    /*
     * Yaw matemático en el sistema ENU:
     *
     *   0°   cuando el eje X del cuerpo apunta al Este.
     *   90°  cuando apunta al Norte.
     *
     * atan2f devuelve inicialmente un resultado entre -180° y 180°.
     */
    yaw_enu_deg =
        atan2f(
                orientation->body_x_world[1],
                orientation->body_x_world[0]) *
        FS_RAD_TO_DEG;

    /*
     * Convertir el ángulo matemático ENU a rumbo magnético:
     *
     *   0°   = Norte.
     *   90°  = Este.
     *   180° = Sur.
     *   270° = Oeste.
     *
     * La resta:
     *
     *   90° - yaw_enu
     *
     * cambia el origen angular y el sentido de crecimiento.
     */
    orientation->yaw_deg =
        FS_Wrap360(90.0f - yaw_enu_deg);


    /* ---------------------------------------------------------------------- */
    /* Inclinación total                                                     */
    /* ---------------------------------------------------------------------- */

    /*
     * Tilt representa el ángulo entre el eje Z del cuerpo
     * y la vertical mundial.
     *
     * Interpretación:
     *
     *   0°   : eje Z completamente hacia arriba.
     *   90°  : eje Z horizontal.
     *   180° : eje Z completamente invertido.
     */
    orientation->tilt_deg =
        acosf(
                FS_Clamp(
                        orientation->body_z_world[2],
                        -1.0f,
                        1.0f)) *
        FS_RAD_TO_DEG;


    /* ---------------------------------------------------------------------- */
    /* Validez final                                                         */
    /* ---------------------------------------------------------------------- */

    /*
     * El resultado solamente se marca como válido cuando tanto
     * el acelerómetro como el magnetómetro están dentro de sus
     * respectivos intervalos de confiabilidad.
     *
     * Es posible que la función llegue hasta aquí y devuelva
     * FS_ORIENTATION_OK, pero orientation->valid sea 0U.
     */
    orientation->valid =
        ((orientation->accel_reliable != 0U) &&
         (orientation->mag_reliable != 0U))
        ? 1U
        : 0U;

    return FS_ORIENTATION_OK;
}
