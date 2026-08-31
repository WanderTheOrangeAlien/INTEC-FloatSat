/**
  ******************************************************************************
  * @file    LSM9DS1_stm32.h
  * @author  Miguel Urena
  * @brief   Library for communication with the LSM9DS1 IMU.
  *
  ******************************************************************************
**/

#ifndef INC_LSM9DS1_H
#define INC_LSM9DS1_H

#include <stdbool.h>

#include "floatsat_types.h"
#include "floatsat_error.h"
#include "stm32f4xx_hal.h"



#define LSM9DS1_DEV_AG              0       // Device selection for read/write
#define LSM9DS1_DEV_MAG             1       // Device selection for read/write

#define LSM9DS1_I2C_TIMEOUT         10      // I2C communication timeout in HAL ticks
#define LSM9DS1_I2C_TRIALS          5       // Number of attempts during device search

#define LSM9DS1_AUTO_INC_BIT        (1 << 7)// If this bit is set in the register address, 
                                            // autoincrement reads are performed
#define LSM9DS1_GYRO_WHO_AM_I_VAL   0x68    // Acc + gyro WHO_AM_I value 
#define LSM9DS1_MAG_WHO_AM_I_VAL    0x3D    // Magnetometer WHO_AM_I value




/* ************************************************************************** */
/* *************************** Register addresses *************************** */
/* ************************************************************************** */

/* ====================== Gyroscope and accelerometer ======================= */

#define LSM9DS1_GYRO_WHO_AM_I   0x0F    // Default value is 0x68
#define LSM9DS1_CTRL_REG1_G     0x10    // Gyroscope control register 1
#define LSM9DS1_CTRL_REG2_G     0x11    // Gyroscope control register 2
#define LSM9DS1_CTRL_REG3_G     0x12    // Gyroscope control register 3

#define LSM9DS1_OUT_TEMP_L      0x15    // Acc+gyro temperature output, expressed as a 12-bit, 
// two's complement signed number, sign extended on the MSB

#define LSM9DS1_STATUS_REG      0x17    // Status register

// All gyroscope output data is expressed as 16-bit two's complement words
#define LSM9DS1_OUT_X_G         0x18    // Gyroscope X output.
#define LSM9DS1_OUT_Y_G         0x1A    // Gyroscope Y output.
#define LSM9DS1_OUT_Z_G         0x1C    // Gyroscope Z output.

#define LSM9DS1_CTRL_REG4       0x1E    // Acc+gyro control register 3

#define LSM9DS1_CTRL_REG5_XL    0x1F    // Accelerometer control register 5
#define LSM9DS1_CTRL_REG6_XL    0x20    // Accelerometer control register 6
#define LSM9DS1_CTRL_REG7_XL    0x21    // Accelerometer control register 6

#define LSM9DS1_CTRL_REG_8      0x22    // Acc+gyro control register 8
#define LSM9DS1_CTRL_REG_9      0x23    // Acc+gyro control register 9

// All accelerometer output data is expressed as 16-bit two's complement words
#define LSM9DS1_OUT_X_XL        0x28    // Accelerometer X output.
#define LSM9DS1_OUT_Y_XL        0x2A    // Accelerometer Y output.
#define LSM9DS1_OUT_Z_XL        0x2C    // Accelerometer Z output.

/* ============================== Magnetometer ============================== */

#define LSM9DS1_OFFSET_X_REG_M  0x05    // Magnetometer environmental offset to be subtracted from the output.
#define LSM9DS1_OFFSET_Y_REG_M  0x07    // Magnetometer environmental offset to be subtracted from the output.
#define LSM9DS1_OFFSET_Z_REG_M  0x09    // Magnetometer environmental offset to be subtracted from the output.

#define LSM9DS1_MAG_WHO_AM_I    0x0F    // Device identification register. Value: 0x3D

#define LSM9DS1_CTRL_REG1_M     0x20    // Magnetometer control register 1
#define LSM9DS1_CTRL_REG2_M     0x21    // Magnetometer control register 2
#define LSM9DS1_CTRL_REG3_M     0x22    // Magnetometer control register 3
#define LSM9DS1_CTRL_REG4_M     0x23    // Magnetometer control register 4
#define LSM9DS1_CTRL_REG5_M     0x24    // Magnetometer control register 5

#define LSM9DS1_STATUS_REG_M    0x27    // Magnetometer status register 

#define LSM9DS1_OUT_X_M         0x28    // Magnetometer X output.
#define LSM9DS1_OUT_Y_M         0x2A    // Magnetometer Y output.
#define LSM9DS1_OUT_Z_M         0x2C    // Magnetometer Z output.

/* ************************************************************************** */
/* ***************************** Register fields **************************** */
/* ************************************************************************** */

/* @gyro_axes */
#define LSM9DS1_GYRO_XEN_G          (1 << 3) // Enable gyroscope X axis. CTRL_REG4
#define LSM9DS1_GYRO_YEN_G          (1 << 4) // Enable gyroscope Y axis. CTRL_REG4
#define LSM9DS1_GYRO_ZEN_G          (1 << 5) // Enable gyroscope Z axis. CTRL_REG4

// Mask for the axis enable fields in CTRL_REG4
#define LSM9DS1_CTRL_REG4_MASK_AXIS_EN (0x03U << 3)   

/* @acc_axes */
#define LSM9DS1_ACC_XEN_XL          (1 << 3) // Enable accelerometer X axis. CTRL_REG5_XL
#define LSM9DS1_ACC_YEN_XL          (1 << 4) // Enable accelerometer Y axis. CTRL_REG5_XL
#define LSM9DS1_ACC_ZEN_XL          (1 << 5) // Enable accelerometer Z axis. CTRL_REG5_XL

// Mask for axis enable fields in CTRL_REG5_XL
#define LSM9DS1_CTRL_REG5_XL_MASK_AXIS_EN (0x03U << 3)


#define LSM9DS1_MAG_TEMP_COMP_EN    (1 << 7) // Enable temperature compensation for magnetomer.CTRL_REG4

/* ======================== Gyroscope ======================== */


// ODR field in CTRL_REG1_G bit [7:5]
typedef enum LSM9DS1_gyro_odr_t {
    GYRO_ODR_POWER_DOWN =   (0x00U << 5),
    GYRO_ODR_14_9HZ     =   (0x01U << 5),
    GYRO_ODR_59_5HZ     =   (0x02U << 5),
    GYRO_ODR_119HZ      =   (0x03U << 5),
    GYRO_ODR_238HZ      =   (0x04U << 5),
    GYRO_ODR_476HZ      =   (0x05U << 5),
    GYRO_ODR_952HZ      =   (0x06U << 5),
}LSM9DS1_gyro_odr_t;
#define LSM9DS1_CTRL_REG1_MASK_ODR  (0x07U << 5)


// FS_G field in CTRL_REG1_G bit [4:3]
typedef enum LSM9DS1_gyro_scale_t {
    GYRO_FS_245DPS      =   (0x00U << 3),
    GYRO_FS_500DPS      =   (0x01U << 3),
    GYRO_FS_2000DPS     =   (0x02U << 3),
}LSM9DS1_gyro_scale_t;
#define LSM9DS1_CTRL_REG1_MASK_FS  (0x03U << 3)


/* ======================== Accelerometer ======================== */


// ODR field in CTRL_REG6_XL bit [7:5]
typedef enum LSM9DS1_accel_odr_t {
    ACC_ODR_POWER_DOWN  =   (0x00U << 5),
    ACC_ODR_10HZ        =   (0x01U << 5),
    ACC_ODR_50HZ        =   (0x02U << 5),
    ACC_ODR_119HZ       =   (0x03U << 5),
    ACC_ODR_238HZ       =   (0x04U << 5),
    ACC_ODR_476HZ       =   (0x05U << 5),
    ACC_ODR_952HZ       =   (0x06U << 5),
} LSM9DS1_accel_odr_t;
#define LSM9DS1_CTRL_REG6_XL_MASK_ODR   (0x07 << 5)

// FS_XL field in CTRL_REG6_XL bit [4:3]
typedef enum LSM9DS1_accel_scale_t {
    ACC_FS_2G           =   (0x00U << 3),
    ACC_FS_16G          =   (0x01U << 3),
    ACC_FS_4G           =   (0x02U << 3),
    ACC_FS_8G           =   (0x03U << 3),
}LSM9DS1_accel_scale_t;
#define LSM9DS1_CTRL_REG6_XL_MASK_FS   (0x03 << 3)



/* ======================== Magnetometer ======================== */

// ODR field in CTRL_REG1_M bit [4:2]
typedef enum LSM9DS1_mag_odr_t {
    MAG_ODR_0_625HZ     =   (0X00U << 2),
    MAG_ODR_1_25HZ      =   (0X01U << 2),
    MAG_ODR_2_5HZ       =   (0X02U << 2),
    MAG_ODR_5HZ         =   (0X03U << 2),
    MAG_ODR_10HZ        =   (0X04U << 2),
    MAG_ODR_20HZ        =   (0X05U << 2),
    MAG_ODR_40HZ        =   (0X06U << 2),
    MAG_ODR_80HZ        =   (0X07U << 2),
} LSM9DS1_mag_odr_t;
#define LSM9DS1_CTRL_REG1_M_MASK_ODR    (0x07U << 2)

// OM field in CTRL_REG1_M bit [6:5]
typedef enum LSM9DS1_mag_xy_mode_t {
    MAG_XY_MODE_LOW_POWER   =   (0x00U << 5),
    MAG_XY_MODE_MEDIUM      =   (0x01U << 5),
    MAG_XY_MODE_HIGH        =   (0x02U << 5),
    MAG_XY_MODE_ULTRA_HIGH  =   (0x03U << 5),
}LSM9DS1_mag_xy_mode_t;
#define LSM9DS1_CTRL_REG1_M_MASK_XY_OM    (0x03U << 5)


//FS field in CTRL_REG2_M bit [6:5]
typedef enum LSM9DS1_mag_scale_t {
    MAG_FS_4GAUSS       =   (0x00U << 5),
    MAG_FS_8GAUSS       =   (0x01U << 5),
    MAG_FS_12GAUSS      =   (0x02U << 5),
    MAG_FS_16GAUSS      =   (0x03U << 5),
}LSM9DS1_mag_scale_t;
#define LSM9DS1_CTRL_REG2_M_MASK_FS    (0x03 << 5)


// MD field int CTRL_REG3_M bit [1:0]
typedef enum LSM9DS1_mag_op_mode_t {
    MAG_OP_MODE_CONTINUOUS  =   (0x00U << 0),
    MAG_OP_MODE_SINGLE      =   (0x01U << 0),
    MAG_OP_MODE_POWER_DOWN  =   (0x02U << 0),
}LSM9DS1_mag_op_mode_t;
#define LSM9DS1_CTRL_REG3_M_MASK_MD  (0x03U << 0)

typedef struct LSM9DS1_params_t {
    LSM9DS1_gyro_odr_t      gyro_odr;   // Gyroscope output data rate
    LSM9DS1_accel_odr_t     acc_odr;    // Accelerometer output data rate
    LSM9DS1_mag_odr_t       mag_odr;    // Magnetometer output data rate

    LSM9DS1_gyro_scale_t    gyro_scale; // Gyroscope scale
    LSM9DS1_accel_scale_t   acc_scale;// Accelerometer scale
    LSM9DS1_mag_scale_t     mag_scale;  // MAgnetometer scale

    uint8_t                 gyro_axes;      // Enabled gyro axes, ORed together. See @gyro_axes
    uint8_t                 acc_axes;       // Enabled accel axes, ORed together. See @acc_axes

    LSM9DS1_mag_xy_mode_t   mag_xy_mode;        // Magnetometer XY axes operating mode
    LSM9DS1_mag_op_mode_t   mag_op_mode;        // Magnetometer operating mode

    uint8_t                 mag_en_temp_comp;    // Enable temperature conpensation
}LSM9DS1_params_t;

typedef struct IMU_handle_t {
    I2C_HandleTypeDef       *i2c_handle;       // HAL I2C handle
    const LSM9DS1_params_t  *params;
  
    uint8_t                 acc_gyro_address; // Accel + gyro I2C address. This is set during device search 
    uint8_t                 mag_address;       // Mag I2C address. This is set during device search

}IMU_handle_t;



floatsat_err_t IMU_Init(IMU_handle_t *handle);
floatsat_err_t IMU_ReadDataRaw(IMU_handle_t *handle, IMU_data_raw_t *data);
floatsat_err_t IMU_ReadData(IMU_handle_t *handle, IMU_data_t *data);

floatsat_err_t IMU_CheckParams(IMU_handle_t *handle);

#ifdef TEST
bool LSM9DS1_IsValidDev0Reg(uint8_t reg);
bool LSM9DS1_IsValidDev1Reg(uint8_t reg);
bool LSM9DS1_IsValidReg(uint8_t reg, uint8_t dev);
#endif


#endif