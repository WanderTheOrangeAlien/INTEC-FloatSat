#ifndef FLOATSAT_MISSION_H
#define FLOATSAT_MISSION_H

#include <stdint.h>

/*
 * API V3:
 * - La IMU y la nivelación solamente bloquean el arranque.
 * - Después de iniciar, la misión no se reinicia.
 * - Los timeouts permiten finalizar las dos misiones.
 */
#define FS_MISSION_API_VERSION  (0x0300U)

typedef enum
{
  FS_MISSION_DISABLED = 0,
  FS_MISSION_WAIT_READY,
  FS_MISSION_START_SIGNAL,
  FS_MISSION_START_DELAY,

  FS_MISSION_1_POINTING,
  FS_MISSION_1_TARGET_CONFIRM,
  FS_MISSION_1_FINISH_SEQUENCE,

  FS_MISSION_2_START_SIGNAL,
  FS_MISSION_2_MOVE_LEFT,
  FS_MISSION_2_BRAKE_LEFT,
  FS_MISSION_2_MOVE_RIGHT,
  FS_MISSION_2_BRAKE_RIGHT,
  FS_MISSION_2_FINISH_SEQUENCE,

  FS_MISSION_COMPLETE,
  FS_MISSION_ERROR

} FS_MissionState_t;

typedef struct
{
  float roll_deg;
  float pitch_deg;
  float yaw_deg;
  float body_z_world_z;
  float encoder_counts_per_second;

  uint8_t imu_sample_valid;
  uint8_t orientation_valid;
  uint8_t madgwick_valid;
  uint8_t gyro_calibration_valid;
  uint8_t mag_calibration_valid;
  uint8_t pwm_started;
  uint8_t encoder_started;

  uint32_t dt_ms;

} FS_MissionInput_t;

typedef struct
{
  FS_MissionState_t state;

  uint8_t attitude_chain_ok;
  uint8_t level_ok;
  uint8_t initial_ready;
  uint8_t target_index;
  uint8_t mission_complete;

  float target_yaw_deg;
  float yaw_error_deg;
  float yaw_rate_dps;
  float motor_signed_duty_percent;

  uint32_t state_elapsed_ms;
  uint32_t target_timeout_count;
  uint32_t brake_timeout_count;

} FS_MissionOutput_t;

typedef struct
{
  FS_MissionState_t state;

  uint8_t target_index;
  uint8_t yaw_rate_initialized;
  uint8_t brake_reference_valid;
  uint8_t mission_started;

  uint32_t state_elapsed_ms;
  uint32_t initial_stable_elapsed_ms;
  uint32_t settle_elapsed_ms;
  uint32_t brake_stable_elapsed_ms;
  uint32_t target_timeout_count;
  uint32_t brake_timeout_count;

  float previous_yaw_deg;
  float yaw_rate_filtered_dps;
  float applied_duty_percent;
  float brake_reference_yaw_deg;

} FS_Mission_t;

void FS_Mission_Init(FS_Mission_t *mission);

void FS_Mission_Update(FS_Mission_t *mission,
                       const FS_MissionInput_t *input,
                       uint8_t enable,
                       FS_MissionOutput_t *output);

#endif /* FLOATSAT_MISSION_H */
