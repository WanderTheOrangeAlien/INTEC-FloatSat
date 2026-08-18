/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Inicialización y elementos asociados a FreeRTOS.
  *
  * Este archivo contiene:
  *
  *   - Definición y creación de las tareas del sistema.
  *   - Variables globales de diagnóstico y telemetría.
  *   - Parámetros generales del FloatSat.
  *   - Funciones auxiliares para controlar el motor.
  *   - Función de recuperación del bus I2C1.
  *
  * Las secciones delimitadas por USER CODE BEGIN / END son conservadas
  * por STM32CubeMX cuando se vuelve a generar el código.
  ******************************************************************************
  */
/* USER CODE END Header */


/* Includes ------------------------------------------------------------------*/

/*
 * FreeRTOS.h:
 * definiciones principales del kernel FreeRTOS.
 */
#include "FreeRTOS.h"

/*
 * task.h:
 * API nativa de tareas de FreeRTOS.
 */
#include "task.h"

/*
 * main.h:
 * definiciones generales del proyecto, pines, GPIO y funciones globales.
 */
#include "main.h"

/*
 * cmsis_os.h:
 * capa CMSIS-RTOS utilizada para crear tareas mediante osThreadNew(),
 * aplicar retardos mediante osDelay(), etc.
 */
#include "cmsis_os.h"


/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/*
 * tim.h:
 * declara los manejadores de temporizadores, especialmente htim1,
 * utilizado para generar el PWM del motor.
 */
#include "tim.h"

/*
 * gpio.h:
 * declara la inicialización y configuración general de los GPIO.
 */
#include "gpio.h"

/*
 * i2c.h:
 * declara hi2c1 y MX_I2C1_Init(), utilizados para comunicarse con la IMU.
 */
#include "i2c.h"

/*
 * Driver de lectura y configuración de la IMU LSM9DS1.
 */
#include "floatsat_imu.h"

/*
 * Cálculo de orientación instantánea mediante acelerómetro y magnetómetro.
 */
#include "floatsat_orientation.h"

/*
 * Calibración del magnetómetro mediante mínimos, máximos, bias y escalas.
 */
#include "floatsat_mag_calibration.h"

/*
 * Calibración del bias del giroscopio.
 */
#include "floatsat_gyro_calibration.h"

/*
 * Filtro Madgwick para estimar la orientación mediante cuaterniones.
 */
#include "floatsat_madgwick.h"

/*
 * Máquina de estados, control de yaw y secuencia de misión.
 */
#include "floatsat_mission.h"


/*
 * Verificación de que floatsat_mission.h define la versión de su API.
 *
 * Sin esta constante podría estar utilizándose un archivo de cabecera
 * antiguo que no coincida con mission.c o freertos.c.
 */
#ifndef FS_MISSION_API_VERSION
#error "Header floatsat_mission.h incorrecto. Reemplace los tres archivos del paquete API v2."
#endif

/*
 * Este freertos.c requiere exactamente la API 3.0:
 *
 *   0x0300U
 */
#if (FS_MISSION_API_VERSION != 0x0300U)
#error "Version incompatible de floatsat_mission.h. Use el paquete API V3 completo."
#endif

/* USER CODE END Includes */


/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/*
 * Los tipos relacionados con la misión, como:
 *
 *   FS_Mission_t
 *   FS_MissionState_t
 *   FS_MissionInput_t
 *   FS_MissionOutput_t
 *
 * están declarados en floatsat_mission.h.
 */

/* USER CODE END PTD */


/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/*
 * Periodo previsto para el ciclo de control del motor.
 *
 * 10 ms equivale a una frecuencia de:
 *
 *   1 / 0.010 s = 100 Hz
 */
#define FS_MOTOR_CONTROL_PERIOD_MS           10U


/*
 * Duración de la calibración del magnetómetro.
 *
 * 30000 ms = 30 segundos.
 */
#define FS_MAG_CALIBRATION_DURATION_MS       30000U


/*
 * Parámetros de calibración magnética previamente obtenidos.
 *
 * Los bias corrigen desplazamientos de hard-iron:
 *
 *   corregido = valor_original - bias
 *
 * Los factores de escala compensan diferencias entre ejes:
 *
 *   corregido = (valor_original - bias) * escala
 */
#define FS_MAG_PRESET_BIAS_X_GAUSS           (0.030520f)
#define FS_MAG_PRESET_BIAS_Y_GAUSS           (0.485310f)
#define FS_MAG_PRESET_BIAS_Z_GAUSS           (0.017780f)

#define FS_MAG_PRESET_SCALE_X                 (0.954103f)
#define FS_MAG_PRESET_SCALE_Y                 (1.120244f)
#define FS_MAG_PRESET_SCALE_Z                 (0.944080f)

/*
 * Calidad registrada durante la calibración que produjo
 * los parámetros anteriores.
 */
#define FS_MAG_PRESET_QUALITY                 (0.842745f)


/*
 * Cantidad de muestras válidas requeridas para calcular
 * el bias del giroscopio.
 *
 * A 100 Hz:
 *
 *   500 muestras = aproximadamente 5 segundos.
 */
#define FS_GYRO_CALIBRATION_TARGET_SAMPLES    500U

/*
 * Tiempo máximo permitido para terminar la calibración del giroscopio.
 */
#define FS_GYRO_CALIBRATION_TIMEOUT_MS        20000U


/*
 * Parámetros del filtro Madgwick.
 *
 * Un periodo de 0.010 s corresponde a 100 Hz.
 */
#define FS_MADGWICK_SAMPLE_PERIOD_S           (0.010f)

/*
 * Ganancia de corrección del filtro.
 *
 * Un beta mayor produce correcciones más rápidas, pero puede aumentar
 * la sensibilidad al ruido del acelerómetro y magnetómetro.
 */
#define FS_MADGWICK_BETA_DEFAULT              (0.100f)

/* USER CODE END PD */


/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/*
 * No se han definido macros privadas adicionales.
 */

/* USER CODE END PM */


/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/*
 * La mayoría de estas variables son volatile porque pueden ser:
 *
 *   - Modificadas por una tarea.
 *   - Leídas por otra tarea.
 *   - Observadas mediante Live Expressions del depurador.
 *
 * volatile obliga al compilador a acceder realmente a memoria cada vez,
 * pero por sí solo no proporciona exclusión mutua entre tareas.
 */


/* -------------------------------------------------------------------------- */
/* Estado general de las misiones                                             */
/* -------------------------------------------------------------------------- */

/*
 * Habilitación general de la misión.
 *
 *   1U = misión habilitada.
 *   0U = misión deshabilitada y motor detenido.
 */
volatile uint8_t fs_mission_enable = 1U;

/*
 * Estado actual de la máquina de estados de misión.
 */
volatile FS_MissionState_t fs_mission_state =
    FS_MISSION_WAIT_READY;

/*
 * Indica si toda la cadena de actitud es válida:
 *
 *   IMU + calibraciones + Madgwick.
 */
volatile uint8_t fs_mission_attitude_chain_ok = 0U;

/*
 * Indica si el FloatSat se encuentra suficientemente nivelado.
 */
volatile uint8_t fs_mission_level_ok = 0U;

/*
 * Indica si se cumplen todas las condiciones necesarias para iniciar.
 */
volatile uint8_t fs_mission_initial_ready = 0U;

/*
 * Índice del objetivo actual de yaw de la misión 1.
 */
volatile uint8_t fs_mission_target_index = 0U;

/*
 * Bandera de finalización completa de las misiones.
 */
volatile uint8_t fs_mission_complete = 0U;

/*
 * Ángulo de yaw que la misión intenta alcanzar.
 */
volatile float fs_mission_target_yaw_deg = 0.0f;

/*
 * Diferencia angular entre el objetivo y el yaw actual.
 */
volatile float fs_mission_yaw_error_deg = 0.0f;

/*
 * Tiempo transcurrido dentro del estado actual.
 */
volatile uint32_t fs_mission_state_elapsed_ms = 0U;

/*
 * Cantidad de objetivos de yaw que agotaron su tiempo máximo.
 */
volatile uint32_t fs_mission_target_timeout_count = 0U;

/*
 * Contador previsto para timeouts de frenado.
 */
volatile uint32_t fs_mission_brake_timeout_count = 0U;


/* -------------------------------------------------------------------------- */
/* Motor y encoder                                                           */
/* -------------------------------------------------------------------------- */

/*
 * Valor escrito en el registro de comparación del temporizador PWM.
 */
volatile uint16_t fs_motor_pwm_compare = 0U;

/*
 * Magnitud del duty actualmente aplicado, entre 0 y 100 %.
 *
 * El signo no se guarda aquí; se almacena en fs_motor_direction.
 */
volatile float fs_motor_duty_percent = 0.0f;

/*
 * Dirección actual del motor:
 *
 *   +1 = canal 1 activo.
 *   -1 = canal 2 activo.
 *    0 = motor detenido.
 */
volatile int8_t fs_motor_direction = 0;

/*
 * Cambio de cuentas del encoder durante el último periodo de cálculo.
 */
volatile int16_t fs_encoder_delta = 0;

/*
 * Posición acumulada del encoder.
 */
volatile int32_t fs_encoder_position_counts = 0;

/*
 * Velocidad del encoder expresada en cuentas por segundo.
 */
volatile float fs_encoder_counts_per_second = 0.0f;


/* -------------------------------------------------------------------------- */
/* Diagnóstico de orientación instantánea                                     */
/* -------------------------------------------------------------------------- */

/*
 * Validez final del cálculo de orientación accel + mag.
 */
volatile uint8_t fs_orientation_valid = 0U;

/*
 * Indica si la magnitud del acelerómetro está dentro del rango esperado.
 */
volatile uint8_t fs_accel_reliable = 0U;

/*
 * Indica si la magnitud del magnetómetro está dentro del rango esperado.
 */
volatile uint8_t fs_mag_reliable = 0U;

/*
 * Código de retorno de FS_Orientation_Compute().
 */
volatile uint8_t fs_orientation_status = 0U;

/*
 * Magnitudes totales de los vectores de la IMU.
 */
volatile float fs_accel_magnitude_g = 0.0f;
volatile float fs_gyro_magnitude_dps = 0.0f;
volatile float fs_mag_magnitude_gauss = 0.0f;


/* -------------------------------------------------------------------------- */
/* Vectores de referencia expresados en coordenadas del cuerpo                */
/* -------------------------------------------------------------------------- */

/*
 * Vector unitario que representa la dirección Arriba.
 */
volatile float fs_up_body_x = 0.0f;
volatile float fs_up_body_y = 0.0f;
volatile float fs_up_body_z = 0.0f;

/*
 * Vector unitario que representa la dirección Norte.
 */
volatile float fs_north_body_x = 0.0f;
volatile float fs_north_body_y = 0.0f;
volatile float fs_north_body_z = 0.0f;

/*
 * Vector unitario que representa la dirección Este.
 */
volatile float fs_east_body_x = 0.0f;
volatile float fs_east_body_y = 0.0f;
volatile float fs_east_body_z = 0.0f;


/* -------------------------------------------------------------------------- */
/* Ejes del cuerpo expresados en el marco mundial ENU                         */
/* -------------------------------------------------------------------------- */

/*
 * Eje X del FloatSat expresado como:
 *
 *   [Este, Norte, Arriba]
 */
volatile float fs_body_x_world_x = 0.0f;
volatile float fs_body_x_world_y = 0.0f;
volatile float fs_body_x_world_z = 0.0f;

/*
 * Eje Y del FloatSat expresado en el mundo.
 */
volatile float fs_body_y_world_x = 0.0f;
volatile float fs_body_y_world_y = 0.0f;
volatile float fs_body_y_world_z = 0.0f;

/*
 * Eje Z del FloatSat expresado en el mundo.
 */
volatile float fs_body_z_world_x = 0.0f;
volatile float fs_body_z_world_y = 0.0f;
volatile float fs_body_z_world_z = 0.0f;


/* -------------------------------------------------------------------------- */
/* Ángulos finales de actitud                                                */
/* -------------------------------------------------------------------------- */

volatile float fs_roll_deg = 0.0f;   /* Rotación alrededor del eje X. */
volatile float fs_pitch_deg = 0.0f;  /* Rotación alrededor del eje Y. */
volatile float fs_yaw_deg = 0.0f;    /* Rumbo alrededor del eje vertical. */
volatile float fs_tilt_deg = 0.0f;   /* Inclinación total del eje Z. */


/* -------------------------------------------------------------------------- */
/* Estado general de la IMU                                                  */
/* -------------------------------------------------------------------------- */

/*
 * Indica si FS_IMU_Init() terminó correctamente.
 */
volatile uint8_t fs_imu_initialized = 0U;

/*
 * Indica si la última muestra completa fue válida.
 */
volatile uint8_t fs_imu_sample_valid = 0U;

/*
 * Cantidad de muestras válidas leídas.
 */
volatile uint32_t fs_imu_sample_count = 0U;

/*
 * Cantidad de errores durante las lecturas I2C de la IMU.
 */
volatile uint32_t fs_imu_read_errors = 0U;

/*
 * Marca de tiempo de la última muestra.
 */
volatile uint32_t fs_imu_timestamp_ms = 0U;


/* -------------------------------------------------------------------------- */
/* Mediciones raw de la IMU                                                  */
/* -------------------------------------------------------------------------- */

/*
 * Lecturas enteras originales del acelerómetro.
 */
volatile int16_t fs_accel_x_raw = 0;
volatile int16_t fs_accel_y_raw = 0;
volatile int16_t fs_accel_z_raw = 0;

/*
 * Lecturas enteras originales del giroscopio.
 */
volatile int16_t fs_gyro_x_raw = 0;
volatile int16_t fs_gyro_y_raw = 0;
volatile int16_t fs_gyro_z_raw = 0;

/*
 * Lecturas enteras originales del magnetómetro.
 */
volatile int16_t fs_mag_x_raw = 0;
volatile int16_t fs_mag_y_raw = 0;
volatile int16_t fs_mag_z_raw = 0;


/* -------------------------------------------------------------------------- */
/* Mediciones escaladas de la IMU                                             */
/* -------------------------------------------------------------------------- */

/*
 * Acelerómetro en unidades g.
 */
volatile float fs_accel_x_g = 0.0f;
volatile float fs_accel_y_g = 0.0f;
volatile float fs_accel_z_g = 0.0f;

/*
 * Giroscopio en grados por segundo.
 */
volatile float fs_gyro_x_dps = 0.0f;
volatile float fs_gyro_y_dps = 0.0f;
volatile float fs_gyro_z_dps = 0.0f;

/*
 * Magnetómetro en gauss.
 */
volatile float fs_mag_x_gauss = 0.0f;
volatile float fs_mag_y_gauss = 0.0f;
volatile float fs_mag_z_gauss = 0.0f;


/* -------------------------------------------------------------------------- */
/* Variables generales de diagnóstico                                        */
/* -------------------------------------------------------------------------- */

/*
 * Estado lógico del pulsador de usuario.
 */
volatile uint8_t fs_button_pressed = 0U;

/*
 * Contadores de ejecución de cada tarea.
 *
 * Permiten comprobar desde el depurador que las tareas siguen ejecutándose.
 */
volatile uint32_t fs_sensor_cycles = 0U;
volatile uint32_t fs_control_cycles = 0U;
volatile uint32_t fs_supervisor_cycles = 0U;
volatile uint32_t fs_mission_cycles = 0U;
volatile uint32_t fs_telemetry_cycles = 0U;

/*
 * Cuenta instantánea del temporizador utilizado como encoder.
 */
volatile int16_t fs_encoder_count = 0;

/*
 * Banderas de inicialización de periféricos.
 */
volatile uint8_t fs_pwm_started = 0U;
volatile uint8_t fs_encoder_started = 0U;


/* -------------------------------------------------------------------------- */
/* Detección y diagnóstico I2C de la IMU                                      */
/* -------------------------------------------------------------------------- */

/*
 * Indica si ambos bloques del LSM9DS1 fueron detectados.
 */
volatile uint8_t fs_imu_detected = 0U;

/*
 * Resultado individual de la detección:
 *
 *   AG  = acelerómetro + giroscopio.
 *   MAG = magnetómetro.
 */
volatile uint8_t fs_imu_ag_detected = 0U;
volatile uint8_t fs_imu_mag_detected = 0U;

/*
 * Direcciones I2C de 7 bits detectadas.
 */
volatile uint8_t fs_imu_ag_address = 0U;
volatile uint8_t fs_imu_mag_address = 0U;

/*
 * Valores leídos desde los registros WHO_AM_I.
 */
volatile uint8_t fs_imu_ag_whoami = 0U;
volatile uint8_t fs_imu_mag_whoami = 0U;

/*
 * Último estado retornado por el driver de la IMU.
 */
volatile uint8_t fs_imu_status =
    (uint8_t)FS_IMU_STATUS_BOTH_NOT_FOUND;

/*
 * Cantidad de intentos realizados para detectar la IMU.
 */
volatile uint32_t fs_imu_detection_attempts = 0U;

/*
 * Último código de error registrado por el periférico I2C.
 */
volatile uint32_t fs_i2c_error = 0U;


/* -------------------------------------------------------------------------- */
/* Calibración del magnetómetro                                               */
/* -------------------------------------------------------------------------- */

/*
 * Solicitud externa para iniciar una nueva calibración.
 */
volatile uint8_t fs_mag_calibration_request = 0U;

/*
 * Indica si actualmente se están recolectando muestras.
 */
volatile uint8_t fs_mag_calibration_running = 0U;

/*
 * Indica si los parámetros calculados son válidos.
 */
volatile uint8_t fs_mag_calibration_valid = 0U;

/*
 * Estado actual del proceso de calibración.
 */
volatile uint8_t fs_mag_calibration_status =
    (uint8_t)FS_MAG_CAL_IDLE;

/*
 * Cantidad de muestras utilizadas.
 */
volatile uint32_t fs_mag_calibration_samples = 0U;

/*
 * Tiempo transcurrido desde el inicio de la calibración.
 */
volatile uint32_t fs_mag_calibration_elapsed_ms = 0U;


/*
 * Valores mínimos encontrados en cada eje.
 */
volatile float fs_mag_min_x_gauss = 0.0f;
volatile float fs_mag_min_y_gauss = 0.0f;
volatile float fs_mag_min_z_gauss = 0.0f;

/*
 * Valores máximos encontrados en cada eje.
 */
volatile float fs_mag_max_x_gauss = 0.0f;
volatile float fs_mag_max_y_gauss = 0.0f;
volatile float fs_mag_max_z_gauss = 0.0f;

/*
 * Offset de hard-iron calculado para cada eje.
 */
volatile float fs_mag_bias_x_gauss = 0.0f;
volatile float fs_mag_bias_y_gauss = 0.0f;
volatile float fs_mag_bias_z_gauss = 0.0f;

/*
 * Semiamplitud observada en cada eje.
 */
volatile float fs_mag_half_range_x_gauss = 0.0f;
volatile float fs_mag_half_range_y_gauss = 0.0f;
volatile float fs_mag_half_range_z_gauss = 0.0f;

/*
 * Factores de corrección de escala.
 */
volatile float fs_mag_scale_x = 1.0f;
volatile float fs_mag_scale_y = 1.0f;
volatile float fs_mag_scale_z = 1.0f;

/*
 * Radio promedio estimado del campo magnético.
 */
volatile float fs_mag_average_radius_gauss = 0.0f;

/*
 * Relación entre el menor y el mayor radio medido.
 */
volatile float fs_mag_calibration_quality = 0.0f;

/*
 * Lecturas magnéticas después de restar bias y aplicar escalas.
 */
volatile float fs_mag_x_corrected_gauss = 0.0f;
volatile float fs_mag_y_corrected_gauss = 0.0f;
volatile float fs_mag_z_corrected_gauss = 0.0f;


/* -------------------------------------------------------------------------- */
/* Calibración del giroscopio                                                 */
/* -------------------------------------------------------------------------- */

/*
 * Solicitud externa para iniciar la calibración.
 */
volatile uint8_t fs_gyro_calibration_request = 0U;

/*
 * Indica si se están recolectando muestras.
 */
volatile uint8_t fs_gyro_calibration_running = 0U;

/*
 * Indica si el bias calculado es válido.
 */
volatile uint8_t fs_gyro_calibration_valid = 0U;

/*
 * Estado actual del proceso.
 */
volatile uint8_t fs_gyro_calibration_status =
    (uint8_t)FS_GYRO_CAL_IDLE;

/*
 * Cantidad de muestras válidas aceptadas.
 */
volatile uint32_t fs_gyro_calibration_samples = 0U;

/*
 * Cantidad de muestras descartadas por movimiento o aceleración.
 */
volatile uint32_t fs_gyro_calibration_rejected_samples = 0U;

/*
 * Cantidad de veces que se ha reiniciado la calibración.
 */
volatile uint32_t fs_gyro_calibration_restart_count = 0U;

/*
 * Tiempo transcurrido durante la calibración.
 */
volatile uint32_t fs_gyro_calibration_elapsed_ms = 0U;

/*
 * Progreso calculado entre 0 y 100 %.
 */
volatile uint8_t fs_gyro_calibration_progress_percent = 0U;

/*
 * Bias promedio del giroscopio.
 */
volatile float fs_gyro_bias_x_dps = 0.0f;
volatile float fs_gyro_bias_y_dps = 0.0f;
volatile float fs_gyro_bias_z_dps = 0.0f;

/*
 * Mediciones después de restar el bias.
 */
volatile float fs_gyro_x_corrected_dps = 0.0f;
volatile float fs_gyro_y_corrected_dps = 0.0f;
volatile float fs_gyro_z_corrected_dps = 0.0f;


/* -------------------------------------------------------------------------- */
/* Diagnóstico y salida del filtro Madgwick                                   */
/* -------------------------------------------------------------------------- */

/*
 * Indica si el filtro recibió una orientación inicial válida.
 */
volatile uint8_t fs_madgwick_initialized = 0U;

/*
 * Indica si el cuaternión actual es válido.
 */
volatile uint8_t fs_madgwick_valid = 0U;

/*
 * Último estado retornado por el filtro.
 */
volatile uint8_t fs_madgwick_status =
    (uint8_t)FS_MADGWICK_OK;

/*
 * Último modo de actualización:
 *
 *   GYRO_ONLY, IMU o MARG.
 */
volatile uint8_t fs_madgwick_mode =
    (uint8_t)FS_MADGWICK_MODE_GYRO_ONLY;

/*
 * Solicitud para reiniciar e inicializar nuevamente el filtro.
 */
volatile uint8_t fs_madgwick_reset_request = 0U;

/*
 * Cantidad de actualizaciones realizadas.
 */
volatile uint32_t fs_madgwick_update_count = 0U;

/*
 * Ganancia beta utilizada por el filtro.
 */
volatile float fs_madgwick_beta =
    FS_MADGWICK_BETA_DEFAULT;

/*
 * Componentes del cuaternión actual.
 *
 * Se inicia como cuaternión identidad:
 *
 *   q = [1, 0, 0, 0]
 */
volatile float fs_madgwick_q0 = 1.0f;
volatile float fs_madgwick_q1 = 0.0f;
volatile float fs_madgwick_q2 = 0.0f;
volatile float fs_madgwick_q3 = 0.0f;


/* -------------------------------------------------------------------------- */
/* Orientación estática accel + mag                                           */
/* -------------------------------------------------------------------------- */

/*
 * Ángulos calculados sin integrar el giroscopio.
 *
 * Se conservan para comparar la orientación instantánea contra
 * la salida filtrada de Madgwick.
 */
volatile float fs_static_roll_deg = 0.0f;
volatile float fs_static_pitch_deg = 0.0f;
volatile float fs_static_yaw_deg = 0.0f;


/* -------------------------------------------------------------------------- */
/* Diagnóstico de recuperación del bus I2C1                                   */
/* -------------------------------------------------------------------------- */

/*
 * Niveles lógicos de SCL y SDA medidos durante la recuperación.
 */
volatile uint8_t fs_i2c_scl_level = 0U;
volatile uint8_t fs_i2c_sda_level = 0U;

/*
 * Indica si el periférico I2C sigue reportando el bus ocupado.
 */
volatile uint8_t fs_i2c_bus_busy = 0U;

/*
 * Cantidad de veces que se ha ejecutado la recuperación del bus.
 */
volatile uint32_t fs_i2c_recovery_count = 0U;

/* USER CODE END Variables */


/* -------------------------------------------------------------------------- */
/* Definición de tareas FreeRTOS                                              */
/* -------------------------------------------------------------------------- */

/*
 * Identificador de SensorTask.
 *
 * SensorTask debería encargarse de:
 *
 *   - Inicializar y leer la IMU.
 *   - Ejecutar calibraciones.
 *   - Actualizar orientación y Madgwick.
 */
osThreadId_t SensorTaskHandle;

const osThreadAttr_t SensorTask_attributes =
{
  .name = "SensorTask",

  /*
   * CMSIS-RTOS2 expresa stack_size en bytes.
   *
   * 512 * 4 = 2048 bytes.
   */
  .stack_size = 512 * 4,

  /*
   * Prioridad alta para mantener estable la frecuencia de muestreo.
   */
  .priority = (osPriority_t) osPriorityHigh,
};


/*
 * Identificador y configuración de ControlTask.
 *
 * Esta tarea debería encargarse del control periódico del motor
 * y del encoder.
 */
osThreadId_t ControlTaskHandle;

const osThreadAttr_t ControlTask_attributes =
{
  .name = "ControlTask",
  .stack_size = 512 * 4,  /* 2048 bytes. */
  .priority = (osPriority_t) osPriorityHigh,
};


/*
 * SupervisorTask debería supervisar errores, estados y recuperación.
 */
osThreadId_t SupervisorTaskHandle;

const osThreadAttr_t SupervisorTask_attributes =
{
  .name = "SupervisorTask",
  .stack_size = 384 * 4,  /* 1536 bytes. */
  .priority = (osPriority_t) osPriorityAboveNormal,
};


/*
 * MissionTask debería ejecutar la máquina de estados de misión.
 */
osThreadId_t MissionTaskHandle;

const osThreadAttr_t MissionTask_attributes =
{
  .name = "MissionTask",
  .stack_size = 384 * 4,  /* 1536 bytes. */
  .priority = (osPriority_t) osPriorityNormal,
};


/*
 * TelemetryTask debería transmitir variables hacia el exterior,
 * por ejemplo mediante UART y HC-05.
 */
osThreadId_t TelemetryTaskHandle;

const osThreadAttr_t TelemetryTask_attributes =
{
  .name = "TelemetryTask",
  .stack_size = 256 * 4,  /* 1024 bytes. */
  .priority = (osPriority_t) osPriorityLow,
};


/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/*
 * Funciones de entrada de las cinco tareas.
 */
void StartSensorTask(void *argument);
void StartControlTask(void *argument);
void StartSupervisorTask(void *argument);
void StartMissionTask(void *argument);
void StartTelemetryTask(void *argument);

/*
 * Recupera el bus I2C1 cuando un dispositivo mantiene SDA en nivel bajo
 * o el periférico permanece en estado BUSY.
 */
static void FS_I2C1_BusRecovery(void);

/*
 * Detiene inmediatamente ambos canales PWM del motor.
 */
static void FS_MotorStop(void);

/*
 * Aplica un duty firmado:
 *
 *   positivo -> canal 1;
 *   negativo -> canal 2;
 *   cero     -> ambos canales apagados.
 */
static void FS_MotorSetSignedDuty(float signed_duty_percent);

/* USER CODE END FunctionPrototypes */


/*
 * Prototipos generados por CubeMX.
 *
 * Se repiten respecto al bloque anterior, pero las declaraciones
 * duplicadas son compatibles mientras las firmas sean idénticas.
 */
void StartSensorTask(void *argument);
void StartControlTask(void *argument);
void StartSupervisorTask(void *argument);
void StartMissionTask(void *argument);
void StartTelemetryTask(void *argument);


/*
 * Inicialización de los objetos de FreeRTOS.
 *
 * El comentario MISRA indica que se declara explícitamente el tipo
 * de retorno de la función.
 */
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */


/**
  * @brief Inicializa los objetos utilizados por FreeRTOS.
  *
  * En esta versión solamente se crean tareas.
  *
  * No se crean:
  *
  *   - Mutexes.
  *   - Semáforos.
  *   - Timers de software.
  *   - Colas.
  *
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void)
{
  /* USER CODE BEGIN Init */

  /*
   * Espacio reservado para inicializaciones que deban ocurrir
   * antes de crear los objetos RTOS.
   */

  /* USER CODE END Init */


  /* USER CODE BEGIN RTOS_MUTEX */

  /*
   * Aquí podrían crearse mutexes para proteger recursos compartidos,
   * como I2C, UART o estructuras de telemetría.
   */

  /* USER CODE END RTOS_MUTEX */


  /* USER CODE BEGIN RTOS_SEMAPHORES */

  /*
   * Aquí podrían crearse semáforos para sincronizar tareas
   * e interrupciones.
   */

  /* USER CODE END RTOS_SEMAPHORES */


  /* USER CODE BEGIN RTOS_TIMERS */

  /*
   * Aquí podrían crearse timers de software de FreeRTOS.
   */

  /* USER CODE END RTOS_TIMERS */


  /* USER CODE BEGIN RTOS_QUEUES */

  /*
   * Aquí podrían crearse colas para transferir muestras o comandos
   * entre tareas sin depender directamente de variables globales.
   */

  /* USER CODE END RTOS_QUEUES */


  /* Create the thread(s) */

  /*
   * Crear SensorTask.
   *
   * NULL indica que no se le entrega ningún argumento inicial.
   */
  SensorTaskHandle =
      osThreadNew(StartSensorTask,
                  NULL,
                  &SensorTask_attributes);

  /*
   * Crear ControlTask.
   */
  ControlTaskHandle =
      osThreadNew(StartControlTask,
                  NULL,
                  &ControlTask_attributes);

  /*
   * Crear SupervisorTask.
   */
  SupervisorTaskHandle =
      osThreadNew(StartSupervisorTask,
                  NULL,
                  &SupervisorTask_attributes);

  /*
   * Crear MissionTask.
   */
  MissionTaskHandle =
      osThreadNew(StartMissionTask,
                  NULL,
                  &MissionTask_attributes);

  /*
   * Crear TelemetryTask.
   */
  TelemetryTaskHandle =
      osThreadNew(StartTelemetryTask,
                  NULL,
                  &TelemetryTask_attributes);


  /* USER CODE BEGIN RTOS_THREADS */

  /*
   * Aquí podrían crearse tareas adicionales no generadas por CubeMX.
   */

  /* USER CODE END RTOS_THREADS */


  /* USER CODE BEGIN RTOS_EVENTS */

  /*
   * Aquí podrían crearse banderas de eventos.
   */

  /* USER CODE END RTOS_EVENTS */
}


/* USER CODE BEGIN Header_StartSensorTask */
/**
  * @brief Función asociada a SensorTask.
  *
  * @param argument Argumento entregado durante osThreadNew().
  *                 Actualmente no se utiliza.
  *
  * @retval None
  */
/* USER CODE END Header_StartSensorTask */

/*
 * __weak permite que esta función sea reemplazada por otra implementación
 * con el mismo nombre declarada sin __weak en otro archivo.
 *
 * Si no existe una implementación fuerte en otro archivo, se ejecutará
 * este bucle vacío.
 */
__weak void StartSensorTask(void *argument)
{
  /* USER CODE BEGIN StartSensorTask */

  /*
   * Bucle infinito obligatorio para una tarea permanente.
   */
  for (;;)
  {
    /*
     * Ceder el procesador durante al menos un tick del RTOS.
     */
    osDelay(1);
  }

  /* USER CODE END StartSensorTask */
}


/* USER CODE BEGIN Header_StartControlTask */
/**
  * @brief Función asociada a ControlTask.
  *
  * @param argument Argumento opcional de la tarea.
  * @retval None
  */
/* USER CODE END Header_StartControlTask */

__weak void StartControlTask(void *argument)
{
  /* USER CODE BEGIN StartControlTask */

  /*
   * Implementación vacía por defecto.
   *
   * Si no se reemplaza, no se realizará control del motor.
   */
  for (;;)
  {
    osDelay(1);
  }

  /* USER CODE END StartControlTask */
}


/* USER CODE BEGIN Header_StartSupervisorTask */
/**
  * @brief Función asociada a SupervisorTask.
  *
  * @param argument Argumento opcional.
  * @retval None
  */
/* USER CODE END Header_StartSupervisorTask */

__weak void StartSupervisorTask(void *argument)
{
  /* USER CODE BEGIN StartSupervisorTask */

  /*
   * Implementación vacía por defecto.
   */
  for (;;)
  {
    osDelay(1);
  }

  /* USER CODE END StartSupervisorTask */
}


/* USER CODE BEGIN Header_StartMissionTask */
/**
  * @brief Función asociada a MissionTask.
  *
  * @param argument Argumento opcional.
  * @retval None
  */
/* USER CODE END Header_StartMissionTask */

__weak void StartMissionTask(void *argument)
{
  /* USER CODE BEGIN StartMissionTask */

  /*
   * Implementación vacía por defecto.
   *
   * Sin una implementación alternativa no se ejecutará
   * FS_Mission_Update().
   */
  for (;;)
  {
    osDelay(1);
  }

  /* USER CODE END StartMissionTask */
}


/* USER CODE BEGIN Header_StartTelemetryTask */
/**
  * @brief Función asociada a TelemetryTask.
  *
  * @param argument Argumento opcional.
  * @retval None
  */
/* USER CODE END Header_StartTelemetryTask */

__weak void StartTelemetryTask(void *argument)
{
  /* USER CODE BEGIN StartTelemetryTask */

  /*
   * Implementación vacía por defecto.
   *
   * Sin una implementación alternativa no se enviará telemetría.
   */
  for (;;)
  {
    osDelay(1);
  }

  /* USER CODE END StartTelemetryTask */
}


/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */


/* -------------------------------------------------------------------------- */
/* Detención del motor                                                       */
/* -------------------------------------------------------------------------- */

/**
 * @brief Detiene el motor apagando ambos canales PWM.
 *
 * Se colocan en cero:
 *
 *   TIM1 canal 1.
 *   TIM1 canal 2.
 *
 * También se actualizan las variables de diagnóstico.
 */
static void FS_MotorStop(void)
{
  /*
   * Apagar el canal correspondiente al primer sentido.
   */
  __HAL_TIM_SET_COMPARE(&htim1,
                        TIM_CHANNEL_1,
                        0U);

  /*
   * Apagar el canal correspondiente al sentido contrario.
   */
  __HAL_TIM_SET_COMPARE(&htim1,
                        TIM_CHANNEL_2,
                        0U);

  /*
   * Actualizar telemetría y estado interno.
   */
  fs_motor_pwm_compare = 0U;
  fs_motor_duty_percent = 0.0f;
  fs_motor_direction = 0;
}


/* -------------------------------------------------------------------------- */
/* Aplicación de duty firmado                                                */
/* -------------------------------------------------------------------------- */

/**
 * @brief Aplica un duty firmado al motor.
 *
 * Interpretación:
 *
 *   signed_duty_percent > 0
 *       canal 1 activo y canal 2 apagado.
 *
 *   signed_duty_percent < 0
 *       canal 1 apagado y canal 2 activo.
 *
 *   signed_duty_percent = 0
 *       ambos canales apagados.
 *
 * @param signed_duty_percent Duty solicitado entre -100 y +100 %.
 */
static void FS_MotorSetSignedDuty(float signed_duty_percent)
{
  /*
   * Magnitud absoluta del duty.
   */
  float duty_magnitude;

  /*
   * Cantidad total de cuentas de un periodo PWM.
   */
  uint32_t period_counts;

  /*
   * Valor que se escribirá en el registro CCR del temporizador.
   */
  uint32_t compare_counts;


  /*
   * Obtener la magnitud del duty sin utilizar fabsf().
   */
  duty_magnitude =
      (signed_duty_percent >= 0.0f)
      ? signed_duty_percent
      : -signed_duty_percent;


  /*
   * Limitar la magnitud máxima a 100 %.
   */
  if (duty_magnitude > 100.0f)
  {
    duty_magnitude = 100.0f;
  }


  /*
   * Considerar cualquier valor menor de 0.01 % como cero.
   *
   * Esto evita mantener residuos numéricos muy pequeños.
   */
  if (duty_magnitude < 0.01f)
  {
    FS_MotorStop();
    return;
  }


  /*
   * El registro ARR contiene el valor máximo contado por el timer.
   *
   * La cantidad matemática de cuentas del periodo es:
   *
   *   ARR + 1
   */
  period_counts =
      __HAL_TIM_GET_AUTORELOAD(&htim1) + 1U;


  /*
   * Convertir porcentaje a cuentas:
   *
   *   compare = periodo * duty / 100
   */
  compare_counts =
      (uint32_t)(((float)period_counts *
                  duty_magnitude) /
                 100.0f);


  /*
   * El registro de comparación no debe superar ARR.
   */
  if (compare_counts >
      __HAL_TIM_GET_AUTORELOAD(&htim1))
  {
    compare_counts =
        __HAL_TIM_GET_AUTORELOAD(&htim1);
  }


  /*
   * Duty positivo:
   *
   * activar canal 1 y apagar canal 2.
   */
  if (signed_duty_percent > 0.0f)
  {
    __HAL_TIM_SET_COMPARE(&htim1,
                          TIM_CHANNEL_1,
                          compare_counts);

    __HAL_TIM_SET_COMPARE(&htim1,
                          TIM_CHANNEL_2,
                          0U);

    fs_motor_direction = 1;
  }
  else
  {
    /*
     * Duty negativo:
     *
     * apagar canal 1 y activar canal 2.
     */
    __HAL_TIM_SET_COMPARE(&htim1,
                          TIM_CHANNEL_1,
                          0U);

    __HAL_TIM_SET_COMPARE(&htim1,
                          TIM_CHANNEL_2,
                          compare_counts);

    fs_motor_direction = -1;
  }


  /*
   * Guardar el valor de comparación para diagnóstico.
   */
  fs_motor_pwm_compare =
      (uint16_t)compare_counts;

  /*
   * Guardar solamente la magnitud.
   *
   * El sentido se obtiene de fs_motor_direction.
   */
  fs_motor_duty_percent =
      duty_magnitude;
}


/* -------------------------------------------------------------------------- */
/* Recuperación del bus I2C1                                                 */
/* -------------------------------------------------------------------------- */

/**
 * @brief Intenta liberar un bus I2C1 bloqueado.
 *
 * Un bus I2C puede quedar bloqueado cuando un esclavo mantiene SDA
 * en nivel bajo, por ejemplo si una transacción fue interrumpida
 * antes de completarse.
 *
 * Secuencia utilizada:
 *
 *   1. Desinicializar I2C1.
 *   2. Reiniciar el periférico mediante RCC.
 *   3. Convertir SCL y SDA temporalmente en GPIO open-drain.
 *   4. Generar hasta nueve pulsos de reloj.
 *   5. Generar una condición STOP manual.
 *   6. Restaurar la configuración I2C.
 *   7. Comprobar si el bus sigue ocupado.
 *
 * Esta función utiliza osDelay(), por lo que está diseñada para
 * ejecutarse desde una tarea después de iniciar el kernel.
 */
static void FS_I2C1_BusRecovery(void)
{
  /*
   * Estructura temporal para configurar PB8 y PB9 como GPIO.
   */
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /*
   * Contador de pulsos manuales de reloj.
   */
  uint32_t pulse;


  /*
   * Registrar un nuevo intento de recuperación.
   */
  fs_i2c_recovery_count++;


  /*
   * Deshabilitar y desinicializar el periférico I2C1.
   */
  HAL_I2C_DeInit(&hi2c1);


  /*
   * Aplicar un reset hardware interno al periférico I2C1.
   */
  __HAL_RCC_I2C1_FORCE_RESET();

  /*
   * Esperar un tick antes de liberar el reset.
   */
  osDelay(1U);

  __HAL_RCC_I2C1_RELEASE_RESET();


  /*
   * Garantizar que el reloj del puerto GPIOB esté habilitado.
   */
  __HAL_RCC_GPIOB_CLK_ENABLE();


  /*
   * PB8 se utiliza como SCL.
   * PB9 se utiliza como SDA.
   */
  GPIO_InitStruct.Pin =
      GPIO_PIN_8 | GPIO_PIN_9;

  /*
   * Salidas open-drain, como requiere físicamente el bus I2C.
   */
  GPIO_InitStruct.Mode =
      GPIO_MODE_OUTPUT_OD;

  /*
   * Pull-up interno.
   *
   * Normalmente el bus también debería tener resistencias pull-up externas.
   */
  GPIO_InitStruct.Pull =
      GPIO_PULLUP;

  /*
   * Velocidad baja suficiente para los pulsos manuales.
   */
  GPIO_InitStruct.Speed =
      GPIO_SPEED_FREQ_LOW;


  /*
   * Aplicar la configuración temporal.
   */
  HAL_GPIO_Init(GPIOB,
                &GPIO_InitStruct);


  /*
   * Liberar ambas líneas.
   *
   * En open-drain, escribir SET deja que las resistencias pull-up
   * eleven la señal.
   */
  HAL_GPIO_WritePin(GPIOB,
                    GPIO_PIN_8 | GPIO_PIN_9,
                    GPIO_PIN_SET);

  osDelay(1U);


  /*
   * Generar hasta nueve pulsos en SCL.
   *
   * Nueve pulsos permiten que un dispositivo esclavo complete
   * la recepción o transmisión del byte pendiente.
   */
  for (pulse = 0U;
       pulse < 9U;
       pulse++)
  {
    /*
     * Si SDA ya está alta, el esclavo dejó de bloquear el bus
     * y no es necesario seguir generando pulsos.
     */
    if (HAL_GPIO_ReadPin(GPIOB,
                         GPIO_PIN_9) == GPIO_PIN_SET)
    {
      break;
    }

    /*
     * Llevar SCL a nivel bajo.
     */
    HAL_GPIO_WritePin(GPIOB,
                      GPIO_PIN_8,
                      GPIO_PIN_RESET);

    osDelay(1U);

    /*
     * Liberar SCL para generar el flanco ascendente.
     */
    HAL_GPIO_WritePin(GPIOB,
                      GPIO_PIN_8,
                      GPIO_PIN_SET);

    osDelay(1U);
  }


  /*
   * Generar una condición STOP manual.
   *
   * Secuencia:
   *
   *   1. SCL baja y SDA baja.
   *   2. SCL sube mientras SDA permanece baja.
   *   3. SDA sube mientras SCL permanece alta.
   */


  /*
   * SCL baja.
   */
  HAL_GPIO_WritePin(GPIOB,
                    GPIO_PIN_8,
                    GPIO_PIN_RESET);

  /*
   * SDA baja.
   */
  HAL_GPIO_WritePin(GPIOB,
                    GPIO_PIN_9,
                    GPIO_PIN_RESET);

  osDelay(1U);


  /*
   * SCL alta.
   */
  HAL_GPIO_WritePin(GPIOB,
                    GPIO_PIN_8,
                    GPIO_PIN_SET);

  osDelay(1U);


  /*
   * SDA alta con SCL alta: condición STOP.
   */
  HAL_GPIO_WritePin(GPIOB,
                    GPIO_PIN_9,
                    GPIO_PIN_SET);

  osDelay(1U);


  /*
   * Registrar el estado físico de SCL.
   */
  fs_i2c_scl_level =
      (HAL_GPIO_ReadPin(GPIOB,
                        GPIO_PIN_8) == GPIO_PIN_SET)
      ? 1U
      : 0U;


  /*
   * Registrar el estado físico de SDA.
   */
  fs_i2c_sda_level =
      (HAL_GPIO_ReadPin(GPIOB,
                        GPIO_PIN_9) == GPIO_PIN_SET)
      ? 1U
      : 0U;


  /*
   * Eliminar la configuración GPIO temporal.
   */
  HAL_GPIO_DeInit(GPIOB,
                  GPIO_PIN_8 | GPIO_PIN_9);


  /*
   * Restaurar PB8, PB9 y el periférico I2C1 mediante
   * la función generada por CubeMX.
   */
  MX_I2C1_Init();


  /*
   * Comprobar si el periférico todavía indica BUSY.
   */
  fs_i2c_bus_busy =
      (__HAL_I2C_GET_FLAG(&hi2c1,
                          I2C_FLAG_BUSY) != RESET)
      ? 1U
      : 0U;
}

/* USER CODE END Application */
