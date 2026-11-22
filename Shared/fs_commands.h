#ifndef FS_COMMANDS_H
#define FS_COMMANDS_H

#include <stdint.h>

#define CMD_MAX_PHOTO_AMOUNT_PER_INSTRUCTION   (10)


#define CMD_ID_CONTROL_INFO     ((uint8_t)0x10)
#define CMD_ID_CONTROL_MODIFY   ((uint8_t)0x11)


#define CMD_ID_MISSION_INFO     ((uint8_t)0x20)
#define CMD_ID_MISSION_SET      ((uint8_t)0x21)

#define CMD_ID_PHOTO_INFO       ((uint8_t)0x30)
#define CMD_ID_PHOTO_ADD        ((uint8_t)0x31)
#define CMD_ID_PHOTO_EDIT       ((uint8_t)0x32)
#define CMD_ID_PHOTO_REMOVE     ((uint8_t)0x33)
#define CMD_ID_PHOTO_RESET      ((uint8_t)0x34)
#define CMD_ID_PHOTO_MOVE       ((uint8_t)0x35)
#define CMD_ID_PHOTO_TAKE       ((uint8_t)0x36)


#define CMD_ID_IMU_READ         ((uint8_t)0x40)
#define CMD_ID_IMU_CALIBRATE    ((uint8_t)0x41)

#define CMD_ID_MOTOR_PWM_SET    ((uint8_t)0x50)

#define CMD_ID_SYSCTL           ((uint8_t)0x60)


#define CMD_CONTROL_FLAG_TYPE      (1U << 0)
#define CMD_CONTROL_FLAG_PARAM     (1U << 1)
#define CMD_CONTROL_FLAG_ANGLE     (1U << 2)
#define CMD_CONTROL_FLAG_DBG_ID    (1U << 3)
#define CMD_CONTROL_FLAG_ENABLE    (1U << 4)
#define CMD_CONTROL_FLAG_DISABLE   (1U << 5)

#define CMD_PHOTO_FLAG_FIELD_ANGLE      (1U << 0)
#define CMD_PHOTO_FLAG_FIELD_TIME       (1U << 1)
#define CMD_PHOTO_FLAG_FIELD_AMOUNT   (1U << 2)

#define CMD_PHOTO_ALL_FIELDS        (CMD_PHOTO_FLAG_FIELD_ANGLE | CMD_PHOTO_FLAG_FIELD_TIME | CMD_PHOTO_FLAG_FIELD_AMOUNT)


#define CMD_IMU_READ_FLAG_ACCEL     (1U << 0)
#define CMD_IMU_READ_FLAG_GYRO      (1U << 1)
#define CMD_IMU_READ_FLAG_MAG       (1U << 2)
#define CMD_IMU_READ_FLAG_RAW       (1U << 3)

#define CMD_IMU_CAL_FLAG_ACCEL      (1U << 4)
#define CMD_IMU_CAL_FLAG_GYRO       (1U << 5)
#define CMD_IMU_CAL_FLAG_MAG        (1U << 6)

#define CMD_SYSCTL_FLAG_RESET       (1U << 0)
#define CMD_SYSCTL_FLAG_DBG_ID      (1U << 1)
#define CMD_SYSCTL_FLAG_INVOKE_CB   (1U << 2)
#define CMD_SYSCTL_FLAG_SET_RTC     (1U << 3)

#define CMD_SYSCTL_MAX_CB_NAME_LEN      32U
#define CMD_SYSCTL_MAX_RTC_DATETIME_LEN 18U // "yy/mm/dd hh:mm:ss"



typedef struct floatsat_time_t {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
}floatsat_time_t;

typedef struct floatsat_date_t {
    uint8_t     year;
    uint8_t     month;
    uint8_t     day;
}floatsat_date_t;

typedef struct photo_info_t {
    float angle;
    floatsat_time_t time;
    uint8_t amount;

}photo_info_t;


typedef struct cmd_control_args_t {
    uint32_t action_flags;
    float k_theta;
    float k_p;
    float k_i;
    float angle;
    uint8_t control_type;
    uint8_t debug_id;
}cmd_control_args_t;

typedef struct cmd_imu_args_t {
    uint32_t action_flags;
}cmd_imu_args_t;

typedef struct cmd_motor_args_t {
    float pwm_val;
}cmd_motor_args_t;

typedef struct cmd_sysctl_args_t {
    uint32_t action_flags;
    uint8_t debug_id;
    char callback_name[CMD_SYSCTL_MAX_CB_NAME_LEN + 1U];
    // char rtc_datetime[CMD_SYSCTL_MAX_RTC_DATETIME_LEN];

    floatsat_time_t rtc_time;
    floatsat_date_t rtc_date;

}cmd_sysctl_args_t;

typedef struct cmd_mission_args_t {
    uint8_t mission_id;
}cmd_mission_args_t;

typedef struct cmd_photo_args_t {
    uint32_t        action_flags;
    photo_info_t    photo;
    uint8_t         index;
    uint8_t         dest_index;
}cmd_photo_args_t;

typedef struct floatsat_cmd_t {
    union {
        cmd_control_args_t  control_args;
        cmd_imu_args_t      imu_args;
        cmd_motor_args_t    motor_args;
        cmd_mission_args_t  mission_args;
        cmd_photo_args_t    photo_args;
        cmd_sysctl_args_t   sysctl_args;
    };
    uint8_t cmd_id;

}floatsat_cmd_t;


//@mission_id
#define MISSION_ID_PLAYGROUND       0
#define MISSION_ID_FOLLOW_SUN       1
#define MISSION_ID_PHOTO            2


/* @control_type */
#define CONTROL_TYPE_NO_CHANGE      0
#define CONTROL_TYPE_PID            1
#define CONTROL_TYPE_LQR            2

#endif