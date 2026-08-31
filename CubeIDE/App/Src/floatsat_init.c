/**
  ******************************************************************************
  * @file    floatsat_init.c
  * @author  Miguel Urena
  * @brief   Initialization of floatsat
  * 
  * @verbatim The function `FloatSat_Init()` calls all the initialization 
  * functions of the different FloatSat modules. The handles and param structures 
  * for each one are created as static variables
  *
  ******************************************************************************
**/

#include "floatsat_init.h"
static const char *LOG_TAG = "INIT";

/* ============================= Module handles ============================= */

/* -------------- IMU -------------- */
static const LSM9DS1_params_t IMU_params = {
    .gyro_odr           =   GYRO_ODR_119HZ,     
    .acc_odr            =   ACC_ODR_119HZ,      
    .mag_odr            =   MAG_ODR_80HZ,

    .gyro_scale         =   GYRO_FS_500DPS,      
    .acc_scale          =   ACC_FS_2G,           
    .mag_scale          =   MAG_FS_4GAUSS,
    
    .gyro_axes          =   LSM9DS1_GYRO_XEN_G | LSM9DS1_GYRO_YEN_G | LSM9DS1_GYRO_ZEN_G,
    .acc_axes           =   LSM9DS1_ACC_XEN_XL | LSM9DS1_ACC_YEN_XL | LSM9DS1_ACC_ZEN_XL, 
    
    .mag_xy_mode        =   MAG_XY_MODE_ULTRA_HIGH,
    .mag_op_mode        =   MAG_OP_MODE_CONTINUOUS,
    
    .mag_en_temp_comp   =   LSM9DS1_MAG_TEMP_COMP_EN
};

static IMU_handle_t IMU_handle = {
    .params = &IMU_params,
};

/* -------------- Madgwick filter -------------- */
static madgwick_filter_t madgwick_filter = {0};


floatsat_err_t FloatSat_RunUnitTests()
{

    return ERR_OK;
}

floatsat_err_t FloatSat_Init(floatsat_periph_t *periph)
{
    if(!periph){
        return ERR_INVALID_ARG;
    }
    floatsat_err_t ret = ERR_OK;
    // Assign the peripheral to each corresponding handle and then call each init
    
    // IMU
    IMU_handle.i2c_handle = periph->imu_i2c;
    GOTO_ON_ERR_LOG(IMU_Init(&IMU_handle), err, ret,
        LOG_TAG, "Error initializaing IMU. Error code: 0x%04x",ret);
    
    IMU_CheckParams(&IMU_handle); // Test for proper initialization

    // Madgwick filter
    GOTO_ON_ERR_LOG(Madgwick_Init(&madgwick_filter),err ,ret,
        LOG_TAG, "Errorr initializing Madgwick filter. Error code: 0x%04x",ret);

    return ERR_OK;

err:
    return ret;

}