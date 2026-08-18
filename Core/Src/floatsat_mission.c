/*
 * Máquina de estados y lógica de control de las misiones del FloatSat.
 *
 * Este módulo coordina:
 *
 *   1. Verificación inicial del sistema.
 *   2. Señalización de inicio mediante LEDs.
 *   3. Misión 1: orientación sucesiva hacia varios ángulos de yaw.
 *   4. Misión 2: giro libre en ambos sentidos y frenado.
 *   5. Finalización segura de la secuencia.
 *
 * El módulo no controla directamente los GPIO ni los registros PWM.
 * En su lugar recibe datos mediante FS_MissionInput_t y devuelve las
 * órdenes mediante FS_MissionOutput_t.
 */

#include "floatsat_mission.h"


/* -------------------------------------------------------------------------- */
/* Comprobación de compatibilidad entre archivos                              */
/* -------------------------------------------------------------------------- */

/*
 * FS_MISSION_API_VERSION debe estar definido en floatsat_mission.h.
 *
 * Esta comprobación evita compilar mission.c con una versión antigua
 * o incompatible del archivo de cabecera.
 */
#ifndef FS_MISSION_API_VERSION
#error "Reemplace mission.h, mission.c y freertos.c simultaneamente."
#endif

/*
 * Este archivo está diseñado específicamente para la API versión 3.0:
 *
 *   0x0300U
 *
 * Si el valor no coincide, la compilación se detiene inmediatamente.
 */
#if (FS_MISSION_API_VERSION != 0x0300U)
#error "Version incompatible de floatsat_mission.h. Use API V3."
#endif

/*
 * Proporciona la definición de NULL.
 */
#include <stddef.h>


/* -------------------------------------------------------------------------- */
/* Requisitos para habilitar el arranque de la misión                         */
/* -------------------------------------------------------------------------- */

/*
 * Tolerancia permitida en roll y pitch antes de comenzar.
 *
 * El sistema se considera nivelado si ambos ángulos permanecen
 * dentro del intervalo:
 *
 *   -7° <= ángulo <= +7°
 */
#define FS_LEVEL_TOLERANCE_DEG                  (7.0f)

/*
 * Límites permitidos para la componente vertical del eje Z corporal.
 *
 * body_z_world_z debería estar próxima a 1 cuando el eje Z del FloatSat
 * apunta hacia arriba.
 *
 * El límite superior ligeramente mayor que 1 admite pequeños errores
 * numéricos del filtro de orientación.
 */
#define FS_BODY_Z_MIN                           (0.985f)
#define FS_BODY_Z_MAX                           (1.010f)

/*
 * Tiempo durante el cual todas las condiciones iniciales deben
 * mantenerse válidas de manera continua.
 */
#define FS_INITIAL_STABLE_MS                    2000U

/*
 * Duración de la señal verde previa al inicio.
 */
#define FS_GREEN_START_SIGNAL_MS                1000U

/*
 * Retardo adicional después de la señal de inicio.
 */
#define FS_START_DELAY_MS                       3000U


/* -------------------------------------------------------------------------- */
/* Misión 1: posicionamiento angular en yaw                                    */
/* -------------------------------------------------------------------------- */

/*
 * Error angular máximo permitido para considerar alcanzado un objetivo.
 */
#define FS_YAW_TOLERANCE_DEG                    (4.0f)

/*
 * Velocidad angular máxima permitida para considerar que el sistema
 * está prácticamente detenido sobre el objetivo.
 */
#define FS_YAW_RATE_TOLERANCE_DPS               (2.5f)

/*
 * Tiempo durante el cual deben cumplirse simultáneamente:
 *
 *   |error yaw| <= 4°
 *   |velocidad yaw| <= 2.5 dps
 */
#define FS_YAW_SETTLE_MS                        500U

/*
 * Duración de la señal de confirmación de cada objetivo.
 */
#define FS_TARGET_CONFIRM_MS                    1000U

/*
 * Tiempo máximo permitido para intentar alcanzar cada objetivo.
 *
 * Si el tiempo se agota, se registra un timeout, pero la misión
 * continúa con el siguiente objetivo.
 */
#define FS_TARGET_TIMEOUT_MS                    20000U

/*
 * Duración de la secuencia de LEDs al terminar una misión.
 */
#define FS_FINISH_LED_SEQUENCE_MS               600U


/* -------------------------------------------------------------------------- */
/* Ganancias del controlador de yaw                                           */
/* -------------------------------------------------------------------------- */

/*
 * Ganancia proporcional:
 *
 * genera una acción proporcional al error angular.
 */
#define FS_YAW_KP                               (0.40f)

/*
 * Ganancia derivativa:
 *
 * reduce la acción cuando el FloatSat ya está girando hacia el objetivo
 * y ayuda a disminuir el sobreimpulso.
 */
#define FS_YAW_KD                               (1.20f)


/* -------------------------------------------------------------------------- */
/* Límites de PWM del motor                                                   */
/* -------------------------------------------------------------------------- */

/*
 * Duty mínimo utilizado para iniciar el movimiento de una rueda
 * aparentemente detenida.
 */
#define FS_MOTOR_START_DUTY_PERCENT             (12.0f)

/*
 * Duty mínimo utilizado para mantener una rueda que ya está moviéndose.
 */
#define FS_MOTOR_HOLD_DUTY_PERCENT              (10.0f)

/*
 * Duty máximo cuando el error angular es grande.
 */
#define FS_MOTOR_MAX_DUTY_PERCENT               (40.0f)

/*
 * Duty máximo para errores intermedios.
 */
#define FS_MOTOR_MID_DUTY_PERCENT               (25.0f)

/*
 * Duty máximo cuando el sistema está cerca del objetivo.
 */
#define FS_MOTOR_NEAR_DUTY_PERCENT              (16.0f)

/*
 * Duty máximo para correcciones finas.
 */
#define FS_MOTOR_FINE_DUTY_PERCENT              (12.0f)

/*
 * Variación máxima de duty permitida en una actualización.
 *
 * Evita cambios bruscos de PWM:
 *
 *   duty_nuevo - duty_anterior <= ±1 %
 */
#define FS_MOTOR_SLEW_PERCENT_PER_CYCLE         (1.0f)


/* -------------------------------------------------------------------------- */
/* Límites basados en el encoder                                              */
/* -------------------------------------------------------------------------- */

/*
 * Por debajo de este valor, la rueda se considera prácticamente detenida.
 */
#define FS_ENCODER_MOTION_CPS                   (50.0f)

/*
 * Límite suave de velocidad de la rueda, expresado en cuentas por segundo.
 *
 * Cuando se supera, se bloquea cualquier orden que aumente todavía más
 * la velocidad de la rueda.
 */
#define FS_ENCODER_SOFT_LIMIT_CPS               (1800.0f)


/* -------------------------------------------------------------------------- */
/* Misión 2: giro libre y frenado                                             */
/* -------------------------------------------------------------------------- */

/*
 * Duración de la señal previa al inicio de la misión 2.
 */
#define FS_MISSION_2_START_SIGNAL_MS            500U

/*
 * Duración de cada movimiento de la misión 2.
 *
 * 20000 ms = 20 segundos.
 */
#define FS_MISSION_2_MOVE_MS                    20000U

/*
 * Duty constante empleado durante el giro libre.
 */
#define FS_MISSION_2_MOVE_DUTY_PERCENT          (30.0f)


/*
 * Frenado simplificado de la misión 2:
 *
 *   1. El motor se apaga inicialmente.
 *   2. Se mide el movimiento de yaw.
 *   3. Se aplica PWM contrario a la velocidad angular.
 *   4. Cuando yaw baja de 1 dps, se considera detectada la parada.
 *   5. Se esperan tres segundos con el motor apagado.
 *
 * No se exige una posición absoluta de yaw ni una velocidad absoluta
 * específica de la rueda.
 */

/*
 * Tiempo inicial durante el cual el motor permanece completamente apagado
 * antes de aplicar frenado activo.
 */
#define FS_MISSION_2_BRAKE_MOTOR_OFF_MS         500U

/*
 * Umbral que selecciona el duty fuerte de frenado.
 */
#define FS_MISSION_2_BRAKE_FAST_RATE_DPS        (10.0f)

/*
 * Umbral que selecciona el duty medio de frenado.
 */
#define FS_MISSION_2_BRAKE_MEDIUM_RATE_DPS      (4.0f)

/*
 * Por debajo de esta velocidad de yaw se considera detectada la parada.
 */
#define FS_MISSION_2_BRAKE_STOP_RATE_DPS        (1.0f)

/*
 * Duty aplicado cuando la velocidad angular todavía es elevada.
 */
#define FS_MISSION_2_BRAKE_FAST_DUTY_PERCENT    (22.0f)

/*
 * Duty aplicado para velocidades angulares intermedias.
 */
#define FS_MISSION_2_BRAKE_MEDIUM_DUTY_PERCENT  (16.0f)

/*
 * Duty aplicado cuando el sistema está cerca de detenerse.
 */
#define FS_MISSION_2_BRAKE_FINE_DUTY_PERCENT    (12.0f)

/*
 * Tiempo de espera después de detectar la parada.
 */
#define FS_MISSION_2_BRAKE_STABLE_MS            3000U

/*
 * Banda angular prevista para verificar estabilidad alrededor del yaw
 * de referencia.
 *
 * En esta versión del código la constante está definida, pero no se utiliza.
 */
#define FS_MISSION_2_STABLE_YAW_BAND_DEG        (2.0f)


/* -------------------------------------------------------------------------- */
/* Estimación de velocidad de yaw                                             */
/* -------------------------------------------------------------------------- */

/*
 * Coeficiente del filtro pasa-bajos aplicado a la velocidad de yaw.
 *
 * Ecuación:
 *
 *   salida += alpha * (entrada - salida)
 *
 * Un alpha pequeño produce una salida más suave, pero más lenta.
 */
#define FS_YAW_RATE_FILTER_ALPHA                (0.15f)

/*
 * Límite de seguridad para rechazar velocidades de yaw absurdamente altas
 * producidas por saltos, errores de tiempo o datos inválidos.
 */
#define FS_MAX_REASONABLE_YAW_RATE_DPS          (250.0f)


/* -------------------------------------------------------------------------- */
/* Objetivos de yaw de la misión 1                                            */
/* -------------------------------------------------------------------------- */

/*
 * Secuencia de posiciones:
 *
 *   0° -> 90° -> 180° -> 270° -> 0°
 *
 * El último objetivo devuelve el FloatSat a su orientación inicial.
 */
static const float FS_MISSION_1_TARGETS_DEG[5] =
{
  0.0f,
  90.0f,
  180.0f,
  270.0f,
  0.0f
};


/* -------------------------------------------------------------------------- */
/* Funciones matemáticas auxiliares                                           */
/* -------------------------------------------------------------------------- */

/**
 * @brief Devuelve el valor absoluto de un número float.
 */
static float FS_AbsFloat(float value)
{
  return (value >= 0.0f) ? value : -value;
}


/**
 * @brief Limita un número al intervalo [minimum, maximum].
 */
static float FS_ClampFloat(float value,
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
 * @brief Devuelve el signo de un número.
 *
 * @return
 *   +1.0 si value es positivo.
 *   -1.0 si value es negativo.
 *    0.0 si value es cero.
 */
static float FS_SignFloat(float value)
{
  if (value > 0.0f)
  {
    return 1.0f;
  }

  if (value < 0.0f)
  {
    return -1.0f;
  }

  return 0.0f;
}


/**
 * @brief Lleva un ángulo al intervalo [0°, 360°).
 *
 * Ejemplos:
 *
 *   370° -> 10°
 *   -10° -> 350°
 */
static float FS_Wrap360Deg(float angle_deg)
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
 * @brief Lleva un ángulo al intervalo (-180°, 180°].
 *
 * Se utiliza para calcular el camino angular más corto.
 *
 * Ejemplo:
 *
 *   objetivo = 0°
 *   yaw actual = 350°
 *
 *   error directo = -350°
 *   error corregido = +10°
 */
static float FS_Wrap180Deg(float angle_deg)
{
  angle_deg = FS_Wrap360Deg(angle_deg);

  if (angle_deg > 180.0f)
  {
    angle_deg -= 360.0f;
  }

  return angle_deg;
}


/* -------------------------------------------------------------------------- */
/* Verificación de sensores y orientación                                     */
/* -------------------------------------------------------------------------- */

/**
 * @brief Comprueba que toda la cadena de estimación de actitud sea válida.
 *
 * Se exige:
 *
 *   - Muestra válida de la IMU.
 *   - Salida válida del filtro Madgwick.
 *   - Calibración válida del giroscopio.
 *   - Calibración válida del magnetómetro.
 *
 * Aunque Madgwick puede funcionar sin magnetómetro, esta misión exige
 * explícitamente que la calibración magnética sea válida antes de iniciar.
 */
static uint8_t FS_AttitudeChainOK(const FS_MissionInput_t *input)
{
  if (input == NULL)
  {
    return 0U;
  }

  return
      ((input->imu_sample_valid != 0U) &&
       (input->madgwick_valid != 0U) &&
       (input->gyro_calibration_valid != 0U) &&
       (input->mag_calibration_valid != 0U))
      ? 1U
      : 0U;
}


/**
 * @brief Comprueba que el FloatSat esté suficientemente nivelado.
 *
 * Se revisan simultáneamente:
 *
 *   - Roll.
 *   - Pitch.
 *   - Componente vertical del eje Z corporal.
 */
static uint8_t FS_LevelOK(const FS_MissionInput_t *input)
{
  if (input == NULL)
  {
    return 0U;
  }

  return
      ((input->roll_deg >= -FS_LEVEL_TOLERANCE_DEG) &&
       (input->roll_deg <=  FS_LEVEL_TOLERANCE_DEG) &&
       (input->pitch_deg >= -FS_LEVEL_TOLERANCE_DEG) &&
       (input->pitch_deg <=  FS_LEVEL_TOLERANCE_DEG) &&
       (input->body_z_world_z >= FS_BODY_Z_MIN) &&
       (input->body_z_world_z <= FS_BODY_Z_MAX))
      ? 1U
      : 0U;
}


/* -------------------------------------------------------------------------- */
/* Administración de estados                                                 */
/* -------------------------------------------------------------------------- */

/**
 * @brief Cambia la máquina de estados a un nuevo estado.
 *
 * Cada transición reinicia:
 *
 *   - Tiempo dentro del estado.
 *   - Tiempo de estabilización del objetivo.
 */
static void FS_EnterState(FS_Mission_t *mission,
                          FS_MissionState_t state)
{
  mission->state = state;
  mission->state_elapsed_ms = 0U;
  mission->settle_elapsed_ms = 0U;

  /*
   * Los estados de frenado necesitan además reiniciar sus variables
   * particulares.
   */
  if ((state == FS_MISSION_2_BRAKE_LEFT) ||
      (state == FS_MISSION_2_BRAKE_RIGHT))
  {
    /*
     * No se arrastra el duty del movimiento anterior.
     * El frenado comienza desde cero.
     */
    mission->applied_duty_percent = 0.0f;

    /*
     * Todavía no se ha detectado la parada de yaw.
     */
    mission->brake_reference_valid = 0U;

    /*
     * Reiniciar el conteo de estabilidad posterior al frenado.
     */
    mission->brake_stable_elapsed_ms = 0U;
  }
}


/**
 * @brief Restablece toda la máquina de misión a su estado inicial.
 */
static void FS_Reset(FS_Mission_t *mission)
{
  mission->state = FS_MISSION_WAIT_READY;

  /*
   * Empezar con el primer objetivo de la misión 1: 0°.
   */
  mission->target_index = 0U;

  /*
   * La estimación de velocidad de yaw todavía no tiene una muestra previa.
   */
  mission->yaw_rate_initialized = 0U;

  /*
   * Todavía no existe referencia de frenado.
   */
  mission->brake_reference_valid = 0U;

  mission->mission_started = 0U;

  /*
   * Reiniciar todos los temporizadores.
   */
  mission->state_elapsed_ms = 0U;
  mission->initial_stable_elapsed_ms = 0U;
  mission->settle_elapsed_ms = 0U;
  mission->brake_stable_elapsed_ms = 0U;

  /*
   * Reiniciar contadores de diagnóstico.
   */
  mission->target_timeout_count = 0U;
  mission->brake_timeout_count = 0U;

  /*
   * Reiniciar variables dinámicas.
   */
  mission->previous_yaw_deg = 0.0f;
  mission->yaw_rate_filtered_dps = 0.0f;
  mission->applied_duty_percent = 0.0f;
  mission->brake_reference_yaw_deg = 0.0f;
}


/* -------------------------------------------------------------------------- */
/* Cálculo de velocidad de yaw                                                */
/* -------------------------------------------------------------------------- */

/**
 * @brief Estima la velocidad angular de yaw a partir de posiciones sucesivas.
 *
 * El proceso es:
 *
 *   1. Normalizar yaw a [0°, 360°).
 *   2. Calcular la diferencia angular mínima.
 *   3. Dividir entre el tiempo transcurrido.
 *   4. Limitar valores absurdos.
 *   5. Aplicar un filtro pasa-bajos.
 */
static void FS_UpdateYawRate(FS_Mission_t *mission,
                             const FS_MissionInput_t *input)
{
  float current_yaw;
  float raw_rate;
  float dt_s;

  /*
   * Normalizar el yaw recibido.
   */
  current_yaw = FS_Wrap360Deg(input->yaw_deg);

  /*
   * En la primera muestra no existe un yaw anterior con el cual calcular
   * una velocidad.
   *
   * Tampoco puede calcularse si dt_ms es cero.
   */
  if ((mission->yaw_rate_initialized == 0U) ||
      (input->dt_ms == 0U))
  {
    mission->previous_yaw_deg = current_yaw;
    mission->yaw_rate_filtered_dps = 0.0f;
    mission->yaw_rate_initialized = 1U;

    return;
  }

  /*
   * Convertir el periodo de milisegundos a segundos.
   */
  dt_s = (float)input->dt_ms * 0.001f;

  /*
   * Calcular velocidad angular:
   *
   *   velocidad = cambio angular / tiempo
   *
   * Wrap180 evita un salto incorrecto al cruzar entre 359° y 0°.
   */
  raw_rate =
      FS_Wrap180Deg(current_yaw - mission->previous_yaw_deg) /
      dt_s;

  /*
   * Limitar valores extremos producidos por datos erróneos.
   */
  raw_rate =
      FS_ClampFloat(raw_rate,
                    -FS_MAX_REASONABLE_YAW_RATE_DPS,
                     FS_MAX_REASONABLE_YAW_RATE_DPS);

  /*
   * Filtro pasa-bajos de primer orden:
   *
   *   filtrado = filtrado +
   *              alpha × (medición - filtrado)
   */
  mission->yaw_rate_filtered_dps +=
      FS_YAW_RATE_FILTER_ALPHA *
      (raw_rate - mission->yaw_rate_filtered_dps);

  /*
   * Guardar yaw para la siguiente actualización.
   */
  mission->previous_yaw_deg = current_yaw;
}


/* -------------------------------------------------------------------------- */
/* Control de posición de la misión 1                                         */
/* -------------------------------------------------------------------------- */

/**
 * @brief Calcula el duty necesario para alcanzar un objetivo de yaw.
 *
 * Se utiliza un controlador proporcional-derivativo:
 *
 *   duty = Kp × error - Kd × velocidad_yaw
 *
 * Además se aplican:
 *
 *   - Límites de duty según distancia al objetivo.
 *   - Duty mínimo para superar fricción.
 *   - Frenado cerca del objetivo.
 *   - Protección por velocidad del encoder.
 */
static float FS_PositionDuty(FS_Mission_t *mission,
                             const FS_MissionInput_t *input,
                             float yaw_error_deg)
{
  float absolute_error;
  float maximum_duty;
  float minimum_duty;
  float duty;
  float normalized_encoder_cps;

  absolute_error = FS_AbsFloat(yaw_error_deg);

  /*
   * Seleccionar el duty máximo permitido según la distancia angular.
   *
   * Cuanto más cerca se encuentra el objetivo, menor es el máximo.
   */
  if (absolute_error > 45.0f)
  {
    maximum_duty = FS_MOTOR_MAX_DUTY_PERCENT;
  }
  else if (absolute_error > 15.0f)
  {
    maximum_duty = FS_MOTOR_MID_DUTY_PERCENT;
  }
  else if (absolute_error > 6.0f)
  {
    maximum_duty = FS_MOTOR_NEAR_DUTY_PERCENT;
  }
  else
  {
    maximum_duty = FS_MOTOR_FINE_DUTY_PERCENT;
  }

  /*
   * Control PD:
   *
   *   término P: impulsa hacia el objetivo.
   *   término D: se opone al movimiento angular actual.
   */
  duty =
      (FS_YAW_KP * yaw_error_deg) -
      (FS_YAW_KD * mission->yaw_rate_filtered_dps);

  /*
   * Limitar la salida según la región angular.
   */
  duty =
      FS_ClampFloat(duty,
                    -maximum_duty,
                     maximum_duty);

  /*
   * Elegir duty mínimo:
   *
   *   - Si la rueda parece detenida, usar duty de arranque.
   *   - Si ya está en movimiento, usar duty de mantenimiento.
   */
  minimum_duty =
      (FS_AbsFloat(input->encoder_counts_per_second) <
       FS_ENCODER_MOTION_CPS)
      ? FS_MOTOR_START_DUTY_PERCENT
      : FS_MOTOR_HOLD_DUTY_PERCENT;

  /*
   * Cuando el error ya está dentro de la tolerancia angular:
   *
   *   - Si el cuerpo todavía está girando, aplicar frenado.
   *   - Si también está casi detenido, apagar el motor.
   */
  if (absolute_error <= FS_YAW_TOLERANCE_DEG)
  {
    if (FS_AbsFloat(mission->yaw_rate_filtered_dps) >
        FS_YAW_RATE_TOLERANCE_DPS)
    {
      duty =
          -FS_SignFloat(mission->yaw_rate_filtered_dps) *
          FS_MOTOR_HOLD_DUTY_PERCENT;
    }
    else
    {
      duty = 0.0f;
    }
  }

  /*
   * Fuera de la tolerancia, impedir órdenes demasiado pequeñas que
   * probablemente no vencerían la fricción del motor.
   */
  else if (FS_AbsFloat(duty) < minimum_duty)
  {
    /*
     * Si el FloatSat ya está girando hacia el objetivo demasiado rápido,
     * aplicar duty contrario para frenarlo.
     */
    if ((FS_AbsFloat(mission->yaw_rate_filtered_dps) >
         FS_YAW_RATE_TOLERANCE_DPS) &&
        ((mission->yaw_rate_filtered_dps * yaw_error_deg) > 0.0f))
    {
      duty =
          -FS_SignFloat(mission->yaw_rate_filtered_dps) *
          minimum_duty;
    }
    else
    {
      /*
       * De lo contrario, aplicar el duty mínimo en dirección al objetivo.
       */
      duty =
          FS_SignFloat(yaw_error_deg) *
          minimum_duty;
    }
  }

  /*
   * Polaridad física medida:
   *
   *   Orden de motor positiva:
   *     yaw aumenta.
   *     encoder entrega cuentas negativas.
   *
   * Se cambia el signo del encoder para trabajar con una dirección
   * normalizada coherente con el duty.
   */
  normalized_encoder_cps =
      -input->encoder_counts_per_second;

  /*
   * Protección de velocidad:
   *
   * Si la rueda supera el límite y el duty pretende acelerarla todavía
   * más en la misma dirección, la salida se anula.
   *
   * El duty contrario continúa permitido para poder frenarla.
   */
  if ((FS_AbsFloat(normalized_encoder_cps) >=
       FS_ENCODER_SOFT_LIMIT_CPS) &&
      ((normalized_encoder_cps * duty) > 0.0f))
  {
    duty = 0.0f;
  }

  return duty;
}


/* -------------------------------------------------------------------------- */
/* Frenado de la misión 2                                                     */
/* -------------------------------------------------------------------------- */

/**
 * @brief Calcula la acción de frenado utilizando la velocidad de yaw.
 *
 * El frenado no busca una posición angular concreta. Solo intenta reducir
 * la velocidad de yaw hasta considerarla prácticamente nula.
 */
static float FS_BrakeDuty(FS_Mission_t *mission,
                          const FS_MissionInput_t *input)
{
  float absolute_rate;
  float brake_duty;

  /*
   * Magnitud de la velocidad angular actual.
   */
  absolute_rate =
      FS_AbsFloat(
          mission->yaw_rate_filtered_dps);

  /*
   * Primera fase:
   *
   * apagar completamente el motor durante 500 ms.
   *
   * Esto elimina el PWM residual del movimiento anterior antes de decidir
   * qué duty de frenado debe aplicarse.
   */
  if (mission->state_elapsed_ms <
      FS_MISSION_2_BRAKE_MOTOR_OFF_MS)
  {
    mission->applied_duty_percent = 0.0f;
    mission->brake_reference_valid = 0U;
    mission->brake_stable_elapsed_ms = 0U;

    return 0.0f;
  }

  /*
   * Mientras todavía no se haya detectado la parada, aplicar frenado
   * activo en sentido contrario al movimiento de yaw.
   */
  if (mission->brake_reference_valid == 0U)
  {
    /*
     * Si yaw todavía supera el umbral de parada, seguir frenando.
     */
    if (absolute_rate >
        FS_MISSION_2_BRAKE_STOP_RATE_DPS)
    {
      mission->brake_stable_elapsed_ms = 0U;

      /*
       * Seleccionar duty según la velocidad angular.
       */
      if (absolute_rate >
          FS_MISSION_2_BRAKE_FAST_RATE_DPS)
      {
        brake_duty =
            FS_MISSION_2_BRAKE_FAST_DUTY_PERCENT;
      }
      else if (absolute_rate >
               FS_MISSION_2_BRAKE_MEDIUM_RATE_DPS)
      {
        brake_duty =
            FS_MISSION_2_BRAKE_MEDIUM_DUTY_PERCENT;
      }
      else
      {
        brake_duty =
            FS_MISSION_2_BRAKE_FINE_DUTY_PERCENT;
      }

      /*
       * El signo es opuesto a la velocidad de yaw.
       */
      return
          -FS_SignFloat(
              mission->yaw_rate_filtered_dps) *
          brake_duty;
    }

    /*
     * Primera detección de parada:
     *
     * se registra el yaw actual como referencia y comienza el conteo
     * de tres segundos con el motor apagado.
     */
    mission->brake_reference_yaw_deg =
        FS_Wrap360Deg(input->yaw_deg);

    mission->brake_reference_valid = 1U;
    mission->brake_stable_elapsed_ms = 0U;
    mission->applied_duty_percent = 0.0f;

    return 0.0f;
  }

  /*
   * Una vez detectada la parada:
   *
   *   - El contador ya no se reinicia.
   *   - No se aplica frenado adicional.
   *   - Se espera hasta alcanzar los 3000 ms.
   */
  mission->brake_stable_elapsed_ms +=
      input->dt_ms;

  mission->applied_duty_percent = 0.0f;

  return 0.0f;
}


/* -------------------------------------------------------------------------- */
/* Limitación de variación del PWM                                            */
/* -------------------------------------------------------------------------- */

/**
 * @brief Limita cuánto puede cambiar el duty en una actualización.
 *
 * Este limitador suaviza el arranque, la inversión y el frenado del motor.
 */
static float FS_ApplySlew(FS_Mission_t *mission,
                          float desired_duty)
{
  float delta;

  /*
   * Diferencia entre el duty deseado y el que actualmente está aplicado.
   */
  delta = desired_duty - mission->applied_duty_percent;

  /*
   * Limitar la variación a ±1 % por ciclo.
   */
  delta =
      FS_ClampFloat(
          delta,
          -FS_MOTOR_SLEW_PERCENT_PER_CYCLE,
           FS_MOTOR_SLEW_PERCENT_PER_CYCLE);

  /*
   * Aplicar solamente la variación permitida.
   */
  mission->applied_duty_percent += delta;

  /*
   * Eliminar pequeños residuos numéricos cercanos a cero.
   */
  if (FS_AbsFloat(mission->applied_duty_percent) < 0.01f)
  {
    mission->applied_duty_percent = 0.0f;
  }

  return mission->applied_duty_percent;
}


/* -------------------------------------------------------------------------- */
/* Inicialización pública                                                     */
/* -------------------------------------------------------------------------- */

/**
 * @brief Inicializa la máquina de estados de misión.
 */
void FS_Mission_Init(FS_Mission_t *mission)
{
  if (mission == NULL)
  {
    return;
  }

  FS_Reset(mission);
}


/* -------------------------------------------------------------------------- */
/* Actualización principal de la misión                                       */
/* -------------------------------------------------------------------------- */

/**
 * @brief Ejecuta un ciclo de la máquina de estados.
 *
 * Esta función debe llamarse periódicamente con un dt_ms coherente.
 *
 * @param mission Estructura interna de la misión.
 * @param input   Datos actuales de sensores, actitud y periféricos.
 * @param enable  Habilitación general de la misión.
 * @param output  Resultado del ciclo y orden de motor.
 */
void FS_Mission_Update(FS_Mission_t *mission,
                       const FS_MissionInput_t *input,
                       uint8_t enable,
                       FS_MissionOutput_t *output)
{
  uint8_t chain_ok;
  uint8_t level_ok;
  uint8_t initial_ready;
  uint8_t drive_state;

  float desired_duty;
  float target_yaw;
  float yaw_error;

  /*
   * Todos los punteros son obligatorios.
   */
  if ((mission == NULL) ||
      (input == NULL) ||
      (output == NULL))
  {
    return;
  }

  /*
   * Comprobar validez de sensores y filtro.
   */
  chain_ok = FS_AttitudeChainOK(input);

  /*
   * Comprobar nivelación física.
   */
  level_ok = FS_LevelOK(input);

  /*
   * Condición completa para iniciar:
   *
   *   - Cadena de actitud válida.
   *   - FloatSat nivelado.
   *   - PWM inicializado.
   *   - Encoder inicializado.
   */
  initial_ready =
      ((chain_ok != 0U) &&
       (level_ok != 0U) &&
       (input->pwm_started != 0U) &&
       (input->encoder_started != 0U))
      ? 1U
      : 0U;

  /*
   * Por seguridad, cada ciclo comienza solicitando duty cero.
   * Los estados activos lo modificarán posteriormente.
   */
  desired_duty = 0.0f;

  /*
   * Objetivo actual de misión 1.
   */
  target_yaw =
      FS_MISSION_1_TARGETS_DEG[mission->target_index];

  /*
   * Actualizar velocidad de yaw antes de ejecutar la máquina de estados.
   */
  FS_UpdateYawRate(mission, input);

  /*
   * Si la misión está deshabilitada:
   *
   *   - Pasar al estado DISABLED.
   *   - Apagar inmediatamente el motor.
   */
  if (enable == 0U)
  {
    mission->state = FS_MISSION_DISABLED;
    mission->applied_duty_percent = 0.0f;
    mission->state_elapsed_ms = 0U;
  }
  else
  {
    /*
     * Al volver a habilitar desde DISABLED, reiniciar toda la misión.
     */
    if (mission->state == FS_MISSION_DISABLED)
    {
      FS_Reset(mission);
    }

    /*
     * Ejecutar la lógica correspondiente al estado actual.
     */
    switch (mission->state)
    {
      /* -------------------------------------------------------------------- */
      /* Espera de condiciones iniciales                                      */
      /* -------------------------------------------------------------------- */

      case FS_MISSION_WAIT_READY:
        mission->applied_duty_percent = 0.0f;

        /*
         * Las condiciones iniciales deben permanecer válidas durante
         * dos segundos continuos.
         */
        if (initial_ready != 0U)
        {
          mission->initial_stable_elapsed_ms += input->dt_ms;

          if (mission->initial_stable_elapsed_ms >=
              FS_INITIAL_STABLE_MS)
          {
            FS_EnterState(mission,
                          FS_MISSION_START_SIGNAL);
          }
        }
        else
        {
          /*
           * Cualquier pérdida de condiciones reinicia el conteo.
           */
          mission->initial_stable_elapsed_ms = 0U;
        }
        break;


      /* -------------------------------------------------------------------- */
      /* Señal verde de inicio                                                */
      /* -------------------------------------------------------------------- */

      case FS_MISSION_START_SIGNAL:
        mission->state_elapsed_ms += input->dt_ms;

        if (mission->state_elapsed_ms >=
            FS_GREEN_START_SIGNAL_MS)
        {
          FS_EnterState(mission,
                        FS_MISSION_START_DELAY);
        }
        break;


      /* -------------------------------------------------------------------- */
      /* Retardo antes de comenzar el movimiento                              */
      /* -------------------------------------------------------------------- */

      case FS_MISSION_START_DELAY:
        mission->state_elapsed_ms += input->dt_ms;

        if (mission->state_elapsed_ms >=
            FS_START_DELAY_MS)
        {
          mission->target_index = 0U;
          mission->mission_started = 1U;

          FS_EnterState(mission,
                        FS_MISSION_1_POINTING);
        }
        break;


      /* -------------------------------------------------------------------- */
      /* Misión 1: orientación hacia el objetivo actual                       */
      /* -------------------------------------------------------------------- */

      case FS_MISSION_1_POINTING:
        target_yaw =
            FS_MISSION_1_TARGETS_DEG[mission->target_index];

        /*
         * Calcular el error por el camino angular más corto.
         */
        yaw_error =
            FS_Wrap180Deg(target_yaw - input->yaw_deg);

        /*
         * Obtener orden de motor mediante control PD.
         */
        desired_duty =
            FS_PositionDuty(mission,
                            input,
                            yaw_error);

        mission->state_elapsed_ms += input->dt_ms;

        /*
         * El objetivo solo se considera estable cuando:
         *
         *   - El error angular está dentro de tolerancia.
         *   - La velocidad de yaw también está dentro de tolerancia.
         */
        if ((FS_AbsFloat(yaw_error) <=
             FS_YAW_TOLERANCE_DEG) &&
            (FS_AbsFloat(mission->yaw_rate_filtered_dps) <=
             FS_YAW_RATE_TOLERANCE_DPS))
        {
          mission->settle_elapsed_ms += input->dt_ms;
        }
        else
        {
          /*
           * Si alguna condición deja de cumplirse, comenzar de nuevo
           * el conteo de estabilidad.
           */
          mission->settle_elapsed_ms = 0U;
        }

        /*
         * Objetivo alcanzado correctamente.
         */
        if (mission->settle_elapsed_ms >=
            FS_YAW_SETTLE_MS)
        {
          FS_EnterState(mission,
                        FS_MISSION_1_TARGET_CONFIRM);
        }

        /*
         * Tiempo agotado.
         *
         * El contador aumenta, pero la misión continúa igualmente.
         */
        else if (mission->state_elapsed_ms >=
                 FS_TARGET_TIMEOUT_MS)
        {
          mission->target_timeout_count++;

          FS_EnterState(mission,
                        FS_MISSION_1_TARGET_CONFIRM);
        }
        break;


      /* -------------------------------------------------------------------- */
      /* Confirmación visual del objetivo                                     */
      /* -------------------------------------------------------------------- */

      case FS_MISSION_1_TARGET_CONFIRM:
        /*
         * Durante la confirmación el motor permanece apagado.
         */
        desired_duty = 0.0f;
        mission->applied_duty_percent = 0.0f;
        mission->state_elapsed_ms += input->dt_ms;

        /*
         * Al terminar el segundo de confirmación:
         *
         *   - avanzar al siguiente objetivo;
         *   - o terminar la misión 1.
         */
        if (mission->state_elapsed_ms >=
            FS_TARGET_CONFIRM_MS)
        {
          if (mission->target_index < 4U)
          {
            mission->target_index++;

            FS_EnterState(mission,
                          FS_MISSION_1_POINTING);
          }
          else
          {
            FS_EnterState(mission,
                          FS_MISSION_1_FINISH_SEQUENCE);
          }
        }
        break;


      /* -------------------------------------------------------------------- */
      /* Secuencia final de la misión 1                                       */
      /* -------------------------------------------------------------------- */

      case FS_MISSION_1_FINISH_SEQUENCE:
        mission->applied_duty_percent = 0.0f;
        mission->state_elapsed_ms += input->dt_ms;

        if (mission->state_elapsed_ms >=
            FS_FINISH_LED_SEQUENCE_MS)
        {
          FS_EnterState(mission,
                        FS_MISSION_2_START_SIGNAL);
        }
        break;


      /* -------------------------------------------------------------------- */
      /* Señal inicial de la misión 2                                         */
      /* -------------------------------------------------------------------- */

      case FS_MISSION_2_START_SIGNAL:
        mission->applied_duty_percent = 0.0f;
        mission->state_elapsed_ms += input->dt_ms;

        if (mission->state_elapsed_ms >=
            FS_MISSION_2_START_SIGNAL_MS)
        {
          FS_EnterState(mission,
                        FS_MISSION_2_MOVE_LEFT);
        }
        break;


      /* -------------------------------------------------------------------- */
      /* Movimiento en el primer sentido                                     */
      /* -------------------------------------------------------------------- */

      case FS_MISSION_2_MOVE_LEFT:
        /*
         * Aplicar 30 % de duty positivo.
         */
        desired_duty =
            FS_MISSION_2_MOVE_DUTY_PERCENT;

        mission->state_elapsed_ms += input->dt_ms;

        /*
         * Después de 20 segundos, comenzar a frenar.
         */
        if (mission->state_elapsed_ms >=
            FS_MISSION_2_MOVE_MS)
        {
          FS_EnterState(mission,
                        FS_MISSION_2_BRAKE_LEFT);
        }
        break;


      /* -------------------------------------------------------------------- */
      /* Frenado del primer movimiento                                       */
      /* -------------------------------------------------------------------- */

      case FS_MISSION_2_BRAKE_LEFT:
        desired_duty =
            FS_BrakeDuty(mission, input);

        mission->state_elapsed_ms += input->dt_ms;

        /*
         * Después de tres segundos desde la detección de parada,
         * comenzar el movimiento contrario.
         */
        if (mission->brake_stable_elapsed_ms >=
            FS_MISSION_2_BRAKE_STABLE_MS)
        {
          FS_EnterState(mission,
                        FS_MISSION_2_MOVE_RIGHT);
        }
        break;


      /* -------------------------------------------------------------------- */
      /* Movimiento en el segundo sentido                                    */
      /* -------------------------------------------------------------------- */

      case FS_MISSION_2_MOVE_RIGHT:
        /*
         * Aplicar 30 % de duty negativo.
         */
        desired_duty =
            -FS_MISSION_2_MOVE_DUTY_PERCENT;

        mission->state_elapsed_ms += input->dt_ms;

        if (mission->state_elapsed_ms >=
            FS_MISSION_2_MOVE_MS)
        {
          FS_EnterState(mission,
                        FS_MISSION_2_BRAKE_RIGHT);
        }
        break;


      /* -------------------------------------------------------------------- */
      /* Frenado del segundo movimiento                                      */
      /* -------------------------------------------------------------------- */

      case FS_MISSION_2_BRAKE_RIGHT:
        desired_duty =
            FS_BrakeDuty(mission, input);

        mission->state_elapsed_ms += input->dt_ms;

        if (mission->brake_stable_elapsed_ms >=
            FS_MISSION_2_BRAKE_STABLE_MS)
        {
          FS_EnterState(mission,
                        FS_MISSION_2_FINISH_SEQUENCE);
        }
        break;


      /* -------------------------------------------------------------------- */
      /* Secuencia final de la misión 2                                       */
      /* -------------------------------------------------------------------- */

      case FS_MISSION_2_FINISH_SEQUENCE:
        mission->applied_duty_percent = 0.0f;
        mission->state_elapsed_ms += input->dt_ms;

        if (mission->state_elapsed_ms >=
            FS_FINISH_LED_SEQUENCE_MS)
        {
          FS_EnterState(mission,
                        FS_MISSION_COMPLETE);
        }
        break;


      /* -------------------------------------------------------------------- */
      /* Misión completada                                                    */
      /* -------------------------------------------------------------------- */

      case FS_MISSION_COMPLETE:
        /*
         * Estado final permanente.
         */
        mission->applied_duty_percent = 0.0f;
        break;


      /* -------------------------------------------------------------------- */
      /* Estado inválido o error                                              */
      /* -------------------------------------------------------------------- */

      case FS_MISSION_ERROR:
      default:
        /*
         * Cualquier estado desconocido lleva a una condición segura.
         */
        mission->state = FS_MISSION_ERROR;
        mission->applied_duty_percent = 0.0f;
        break;
    }
  }


  /* ------------------------------------------------------------------------ */
  /* Aplicación final del duty                                                */
  /* ------------------------------------------------------------------------ */

  /*
   * Determinar si el estado actual permite energizar el motor.
   */
  drive_state =
      ((mission->state == FS_MISSION_1_POINTING) ||
       (mission->state == FS_MISSION_2_MOVE_LEFT) ||
       (mission->state == FS_MISSION_2_BRAKE_LEFT) ||
       (mission->state == FS_MISSION_2_MOVE_RIGHT) ||
       (mission->state == FS_MISSION_2_BRAKE_RIGHT))
      ? 1U
      : 0U;

  /*
   * Fuera de los estados de movimiento, el motor se apaga inmediatamente.
   */
  if ((enable == 0U) ||
      (drive_state == 0U))
  {
    mission->applied_duty_percent = 0.0f;
  }
  else
  {
    /*
     * En estados activos, aproximar suavemente el duty aplicado
     * al duty solicitado.
     */
    FS_ApplySlew(mission, desired_duty);
  }


  /* ------------------------------------------------------------------------ */
  /* Preparación de la salida                                                 */
  /* ------------------------------------------------------------------------ */

  /*
   * Recalcular el objetivo actual para informar telemetría.
   */
  target_yaw =
      FS_MISSION_1_TARGETS_DEG[mission->target_index];

  /*
   * Error angular mostrado en la salida, incluso durante misión 2.
   */
  yaw_error =
      FS_Wrap180Deg(target_yaw - input->yaw_deg);

  /*
   * Estado actual de la máquina.
   */
  output->state = mission->state;

  /*
   * Indicadores de diagnóstico.
   */
  output->attitude_chain_ok = chain_ok;
  output->level_ok = level_ok;
  output->initial_ready = initial_ready;

  /*
   * Objetivo actual de misión 1.
   */
  output->target_index = mission->target_index;

  /*
   * Bandera de finalización.
   */
  output->mission_complete =
      (mission->state == FS_MISSION_COMPLETE)
      ? 1U
      : 0U;

  /*
   * Variables de control y telemetría.
   */
  output->target_yaw_deg = target_yaw;
  output->yaw_error_deg = yaw_error;
  output->yaw_rate_dps = mission->yaw_rate_filtered_dps;

  /*
   * Duty firmado:
   *
   *   positivo -> un sentido;
   *   negativo -> sentido contrario;
   *   cero     -> motor apagado.
   */
  output->motor_signed_duty_percent =
      mission->applied_duty_percent;

  /*
   * Temporización y contadores de diagnóstico.
   */
  output->state_elapsed_ms = mission->state_elapsed_ms;

  output->target_timeout_count =
      mission->target_timeout_count;

  output->brake_timeout_count =
      mission->brake_timeout_count;
}
