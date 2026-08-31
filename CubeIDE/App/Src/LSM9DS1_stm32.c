/**
  ******************************************************************************
  * @file    LSM9DS1.c
  * @author  Miguel Urena
  * @brief   Library for communication with the LSM9DS1 IMU.
  *
  ******************************************************************************
**/

#include "LSM9DS1_stm32.h"
#include "vec_math.h"
#include "floatsat_log.h"

/*
Notes

Gyro ODR:   CTRL_REG1_G
Accel ORD:  CTRL_REG6_XL

Other
Gyro
CTRL_REG1_G   ODR and measurement range
CTRL_REG2_G   Interrupt selection
CTRL_REG3_G   Low power and high pass frequency
CTRL_REG4     Gyro Axis enable


Accel
CTRL_REG5_XL  Decimation and axis enable
CTRL_REG6_XL  ODR, range, anti-aliasing filter bandwidth
CTRL_REG7_XL  High resolution mode and filter selection


CTRL_REG8     Misc control functions
CTRL_REG9     

IMPORTANT:
Don't forget to turn on the devices as follows

Accelerometer + Gyro: Change the ODR at CTRL_REG1_G. 
As per thte datasheet: "writing to CTRL_REG1_G (10h) both
accelerometer and gyroscope are activated at the same ODR."

Magnetometer: Change the MD at CTRL_REG3_M


*/


/* ========================== Forward Declarations ========================== */

static floatsat_err_t IMU_ReadReg(const IMU_handle_t *handle, uint8_t device, 
                                  uint8_t reg,  uint8_t *data);

static floatsat_err_t IMU_ReadRegInc(const IMU_handle_t *handle, uint8_t device,
                                        uint8_t reg, uint8_t *data, uint8_t data_len);

static floatsat_err_t IMU_WriteReg(const IMU_handle_t *handle, uint8_t device,
                                    uint8_t reg, uint8_t *data);

static floatsat_err_t IMU_WriteRegInc(const IMU_handle_t *handle, uint8_t device,
                                        uint8_t reg, uint8_t *data, uint8_t data_len);
static floatsat_err_t IMU_ConvertRaw(IMU_handle_t *handle, IMU_data_raw_t *data_raw, IMU_data_t *data);

static floatsat_err_t IMU_FindDevices(IMU_handle_t *handle);

static uint8_t CheckValue8(uint8_t value, uint8_t expected, const char *name);

STATIC INLINE bool LSM9DS1_IsValidReg(uint8_t reg, uint8_t dev);


/* ====================== Static variables and constants ======================= */
static const char *LOG_TAG = "IMU";
static const uint8_t known_addresses[] = {};

// Accelerometer conversion factors mg/LSB
static const float ACC_CONV_FACTORS[] = {
    0.061f,     // 2g
    0.732f,     // 16g
    0.122f,     // 4g
    0.244f      // 8g
};

// Gyroscope conversion factors mdps/LSB (milii-degree per second)
static const float GYRO_CONV_FACTORS[] = {
    8.750f,      // 245 dps
    11.500f,     // 500 dps
    70.000f      // 2000 dps
};

// Magnetometer conversion factors mgauss/LSB
static const float MAG_CONV_FACTORS[] = {
    0.140f,     // 4 gauss
    0.290f,     // 8 gauss
    0.430f,     // 12 gauss
    0.580f      // 16 gauss 
};


/// @brief Initializes the LSM9DS1 IMU using the handle `params` field
/// @param handle Pointer to the IMU 
/// @return `ERR_OK` on success. `ERR_INVALID_ARG` if a NULL pointer was passed. 
/// `ERR_DEV_I2C_FAIL` if an I2C transaction resulted in an error
floatsat_err_t IMU_Init(IMU_handle_t *handle)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }
    
    // Start by finding the addresses of the Acc+gyro and the magnetometer
    floatsat_err_t ret;
    GOTO_ON_ERR(IMU_FindDevices(handle),err,ret);
    uint8_t reg_val = 0x00;

    /*------------------------ Accelerometer + Gyroscope----------------------*/

    //Enable gyroscope axes
    GOTO_ON_ERR(IMU_ReadReg(handle, LSM9DS1_DEV_AG, LSM9DS1_CTRL_REG4, &reg_val), err, ret);
    reg_val &= ~LSM9DS1_CTRL_REG4_MASK_AXIS_EN;
    reg_val |= handle->params->gyro_axes;
    GOTO_ON_ERR(IMU_WriteReg(handle, LSM9DS1_DEV_AG, LSM9DS1_CTRL_REG4, &reg_val), err, ret);

    // Enable accelerometer axes
    GOTO_ON_ERR(IMU_ReadReg(handle, LSM9DS1_DEV_AG, LSM9DS1_CTRL_REG5_XL, &reg_val), err, ret);
    reg_val &= ~LSM9DS1_CTRL_REG5_XL_MASK_AXIS_EN;
    reg_val |= handle->params->acc_axes;
    GOTO_ON_ERR(IMU_WriteReg(handle,LSM9DS1_DEV_AG,LSM9DS1_CTRL_REG5_XL,&reg_val),err,ret);

    // Configure Gyroscope ODR and scale
    GOTO_ON_ERR(IMU_ReadReg(handle, LSM9DS1_DEV_AG, LSM9DS1_CTRL_REG1_G, &reg_val), err, ret);
    reg_val &= ~(LSM9DS1_CTRL_REG1_MASK_FS | LSM9DS1_CTRL_REG1_MASK_ODR);
    reg_val |= (handle->params->gyro_odr | handle->params->gyro_scale);
    GOTO_ON_ERR(IMU_WriteReg(handle, LSM9DS1_DEV_AG, LSM9DS1_CTRL_REG1_G, &reg_val), err, ret);

    // Configure Accelerometer ODR and scale
    GOTO_ON_ERR(IMU_ReadReg(handle, LSM9DS1_DEV_AG, LSM9DS1_CTRL_REG6_XL, &reg_val), err, ret);
    reg_val &= ~(LSM9DS1_CTRL_REG6_XL_MASK_FS | LSM9DS1_CTRL_REG6_XL_MASK_ODR);
    reg_val |= (handle->params->acc_odr | handle->params->acc_scale);
    GOTO_ON_ERR(IMU_WriteReg(handle, LSM9DS1_DEV_AG, LSM9DS1_CTRL_REG6_XL, &reg_val), err, ret);

    /*----------------------------- Magnetometer -----------------------------*/

    // Configure ODR, XY axes operational mode and temperature compensation
    GOTO_ON_ERR(IMU_ReadReg(handle, LSM9DS1_DEV_MAG, LSM9DS1_CTRL_REG1_M, &reg_val), err, ret);
    reg_val &= ~(LSM9DS1_CTRL_REG1_M_MASK_ODR | LSM9DS1_CTRL_REG1_M_MASK_XY_OM | LSM9DS1_MAG_TEMP_COMP_EN);
    reg_val |= handle->params->mag_odr | handle->params->mag_xy_mode | handle->params->mag_en_temp_comp;
    GOTO_ON_ERR(IMU_WriteReg(handle, LSM9DS1_DEV_MAG, LSM9DS1_CTRL_REG1_M, &reg_val), err, ret);

    // Configure scale
    GOTO_ON_ERR(IMU_ReadReg(handle, LSM9DS1_DEV_MAG, LSM9DS1_CTRL_REG2_M, &reg_val), err, ret);
    reg_val &= ~LSM9DS1_CTRL_REG2_M_MASK_FS;
    reg_val |= handle->params->mag_scale;
    GOTO_ON_ERR(IMU_WriteReg(handle, LSM9DS1_DEV_MAG, LSM9DS1_CTRL_REG2_M, &reg_val), err, ret);

    // Configure magnetometer device operating mode
    GOTO_ON_ERR(IMU_ReadReg(handle, LSM9DS1_DEV_MAG, LSM9DS1_CTRL_REG3_M, &reg_val), err, ret);
    reg_val &= ~LSM9DS1_CTRL_REG3_M_MASK_MD;
    reg_val |= handle->params->mag_op_mode;
    GOTO_ON_ERR(IMU_WriteReg(handle, LSM9DS1_DEV_MAG, LSM9DS1_CTRL_REG3_M, &reg_val), err, ret);


    return ERR_OK;
err:
    return ret;
}   

/// @brief Debug function that reads the IMU control registers and checks whether their values
/// correspond to the handle parameters. Results are printed as debug messages
/// @param handle 
/// @return 
floatsat_err_t IMU_CheckParams(IMU_handle_t *handle)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }

    floatsat_err_t ret = ERR_OK;
    uint8_t reg_val = 0x00;
    uint8_t field_val = 0x00; // Value of the field to test

    /*------------------------ Accelerometer + Gyroscope----------------------*/

    // Check for gyroscope enabled axes
    GOTO_ON_ERR(IMU_ReadReg(handle,LSM9DS1_DEV_AG, LSM9DS1_CTRL_REG4, &reg_val), err, ret);
    field_val = (reg_val & LSM9DS1_CTRL_REG4_MASK_AXIS_EN);
    CheckValue8(field_val, handle->params->gyro_axes, "GYRO_AXES");

    // Check for accelerometer enabled axes
    GOTO_ON_ERR(IMU_ReadReg(handle,LSM9DS1_DEV_AG, LSM9DS1_CTRL_REG5_XL, &reg_val), err, ret);
    field_val = (reg_val & LSM9DS1_CTRL_REG5_XL_MASK_AXIS_EN);
    CheckValue8(field_val, handle->params->acc_axes, "ACC_AXES");

    // Check for Gyro ODR and scale
    GOTO_ON_ERR(IMU_ReadReg(handle,LSM9DS1_DEV_AG, LSM9DS1_CTRL_REG1_G,&reg_val), err, ret);
    field_val = (reg_val & LSM9DS1_CTRL_REG1_MASK_ODR);
    CheckValue8(field_val, handle->params->gyro_odr, "GYRO_ODR");
    field_val = (reg_val & LSM9DS1_CTRL_REG1_MASK_FS);
    CheckValue8(field_val, handle->params->gyro_scale, "GYRO_SCALE");

    // Check for accelerometer ODR and scale
    GOTO_ON_ERR(IMU_ReadReg(handle,LSM9DS1_DEV_AG, LSM9DS1_CTRL_REG6_XL, &reg_val), err, ret);
    field_val = (reg_val & LSM9DS1_CTRL_REG6_XL_MASK_ODR);
    CheckValue8(field_val, handle->params->acc_odr, "ACC_ODR");
    field_val = (reg_val & LSM9DS1_CTRL_REG6_XL_MASK_FS);
    CheckValue8(field_val, handle->params->acc_scale, "ACC_SCALE");

    /*----------------------------- Magnetometer -----------------------------*/

    // Check for magnetometer ODR, XY mode and temperature compensation
    GOTO_ON_ERR(IMU_ReadReg(handle,LSM9DS1_DEV_MAG, LSM9DS1_CTRL_REG1_M, &reg_val), err, ret);
    field_val = (reg_val & LSM9DS1_CTRL_REG1_M_MASK_ODR);
    CheckValue8(field_val, handle->params->mag_odr, "MAG_ODR");
    field_val = (reg_val & LSM9DS1_CTRL_REG1_M_MASK_XY_OM);
    CheckValue8(field_val, handle->params->mag_xy_mode, "MAG_XY_MODE");
    field_val = (reg_val & LSM9DS1_MAG_TEMP_COMP_EN);
    CheckValue8(field_val, handle->params->mag_en_temp_comp, "MAG_TEMP_CONV");


    // Check for magnetometer scale
    GOTO_ON_ERR(IMU_ReadReg(handle,LSM9DS1_DEV_MAG, LSM9DS1_CTRL_REG2_M, &reg_val), err, ret);
    field_val = (reg_val & LSM9DS1_CTRL_REG2_M_MASK_FS);
    CheckValue8(field_val, handle->params->mag_scale, "MAG_SCALE");

    // Check for magnetometer operation mode
    GOTO_ON_ERR(IMU_ReadReg(handle,LSM9DS1_DEV_MAG, LSM9DS1_CTRL_REG3_M, &reg_val), err, ret);
    field_val = (reg_val & LSM9DS1_CTRL_REG3_M_MASK_MD);
    CheckValue8(field_val, handle->params->mag_op_mode, "MAG_OP_MODE");
    
    
    return ERR_OK;

err:
    LOGE(LOG_TAG, "Error reading register");
    return ret;

}

static uint8_t CheckValue8(uint8_t value, uint8_t expected, const char *name)
{
    if(value == expected){
        LOGD(LOG_TAG, LOG_COLOR_GREEN "Test for value \"%s\" succesful (0x%02x == 0x%02x)" LOG_COLOR_RESET,
            name, value, expected);
        return 1U;
    }else{
        LOGD(LOG_TAG, LOG_COLOR_RED "Test for value \"%s\" failed (0x%02x != 0x%02x)" LOG_COLOR_RESET,
            name, value, expected);
        return 0U;
    }
}

floatsat_err_t IMU_ReadDataRaw(IMU_handle_t *handle, IMU_data_raw_t *data)
{
    // TODO: This whole operation should be atomic (nothing should interrupt it)
    if(!handle || !data){
        return ERR_INVALID_ARG;
    }
    floatsat_err_t ret = ERR_OK;

    // Perform 6 subsequent reads for each sensor (2 bytes/axis * 3 axes)

    // Read gyroscope
    GOTO_ON_ERR(IMU_ReadRegInc(handle, LSM9DS1_DEV_AG, LSM9DS1_OUT_X_G,
                               (uint8_t*)&(data->gyro), 6U), err, ret); 

    // Read accelerometer
    GOTO_ON_ERR(IMU_ReadRegInc(handle, LSM9DS1_DEV_AG, LSM9DS1_OUT_X_XL,
                               (uint8_t*)&(data->accel), 6U), err, ret); 

    // Read magnetometer
    GOTO_ON_ERR(IMU_ReadRegInc(handle, LSM9DS1_DEV_MAG, LSM9DS1_OUT_X_M,
                               (uint8_t*)&(data->mag), 6U), err, ret); 

    data->is_valid = 1U;
    return ERR_OK;

err:
    *data = (IMU_data_raw_t){0};
    return ret;
}

floatsat_err_t IMU_ReadData(IMU_handle_t *handle, IMU_data_t *data)
{
    IMU_data_raw_t data_raw = {0};
    floatsat_err_t ret = IMU_ReadDataRaw(handle,&data_raw);

    if(ret != ERR_OK){
        return ret;
    }

    return IMU_ConvertRaw(handle,&data_raw,data);
}

/// @brief Convert raw readings to physical units depending on the device scale settings
/// @param handle Pointer to IMU handle
/// @param data_raw Pointer to raw data struct to convert
/// @param data Pointer to converted data struct
/// @return ERR_OK on success. ERR_INVALID_VAL if a NULL pointer was passed. 
/// ERR_INVALID_STATE if a handle scale parameter was out of bounds 
static floatsat_err_t IMU_ConvertRaw(IMU_handle_t *handle, IMU_data_raw_t *data_raw, IMU_data_t *data)
{
    // This function assumes that the hardware registers are synchronized with
    // the handle parameters
    if(!handle || !data || !data_raw){
        return ERR_INVALID_ARG;
    }
    
    uint8_t conv_factor_idx = 0U;

    // Convert accelerometer readings
    conv_factor_idx = handle->params->acc_scale >> 3;
    if(conv_factor_idx > (ACC_FS_8G >> 3)){
        return ERR_INVALID_STATE;
    }
    data->accel = (Vec3_t) {
        .x = data_raw->accel.x * ACC_CONV_FACTORS[conv_factor_idx],
        .y = data_raw->accel.y * ACC_CONV_FACTORS[conv_factor_idx],
        .z = data_raw->accel.z * ACC_CONV_FACTORS[conv_factor_idx]
    };

    // Convert gyroscope readings
    conv_factor_idx = handle->params->gyro_scale >> 3;
    if(conv_factor_idx > (GYRO_FS_2000DPS >> 3)){
        return ERR_INVALID_STATE;
    }
    data->gyro = (Vec3_t) {
        .x = data_raw->gyro.x * GYRO_CONV_FACTORS[conv_factor_idx],
        .y = data_raw->gyro.y * GYRO_CONV_FACTORS[conv_factor_idx],
        .z = data_raw->gyro.z * GYRO_CONV_FACTORS[conv_factor_idx]
    };

    // Convert gyroscope readings
    conv_factor_idx = handle->params->mag_scale >> 5;
    if(conv_factor_idx > (MAG_FS_16GAUSS >> 5)){
        return ERR_INVALID_STATE;
    }
    data->mag = (Vec3_t) {
        .x = data_raw->mag.x * MAG_CONV_FACTORS[conv_factor_idx],
        .y = data_raw->mag.y * MAG_CONV_FACTORS[conv_factor_idx],
        .z = data_raw->mag.z * MAG_CONV_FACTORS[conv_factor_idx]
    };


    return ERR_OK;
}

/// @brief Find the addresses of the Acc+Gyro and Mag devices by probing known addresses
/// and checking the WHO_AM_I register. Writes to the handle `acc_gyro_address` and `mag_address` fields
/// @param handle Point4er to IMU handle
/// @return 
static floatsat_err_t IMU_FindDevices(IMU_handle_t *handle)
{
    if(!handle){
        return ERR_INVALID_ARG;
    }
    handle->acc_gyro_address = 0x00U;
    handle->mag_address = 0x00U;


    HAL_StatusTypeDef status;
    for (uint8_t  i = 0; i < sizeof(known_addresses); i++)
    {
        // Probe device
        status = HAL_I2C_IsDeviceReady(handle->i2c_handle, known_addresses[i] << 1,
                            LSM9DS1_I2C_TRIALS, LSM9DS1_I2C_TIMEOUT);

        if(status != HAL_OK){
            continue;
        }

        //Check Who Am I register. It's the same for both devices
        uint8_t who_am_i_val = 0;
        status = HAL_I2C_Mem_Read(handle->i2c_handle, known_addresses[i] << 1,
                                    LSM9DS1_MAG_WHO_AM_I, 1U, &who_am_i_val,
                                    1U, LSM9DS1_I2C_TIMEOUT);
        if(status != HAL_OK){
            // Return error because, previously, the device was probed succesfully
            // So the fact that the read went wrong means a fault 
            return ERR_DEV_I2C_FAIL;
        }

        if(who_am_i_val == LSM9DS1_MAG_WHO_AM_I_VAL){
            handle->mag_address = known_addresses[i];
        }else if(who_am_i_val == LSM9DS1_GYRO_WHO_AM_I_VAL){
            handle->acc_gyro_address = known_addresses[i];
        }else{
            return ERR_DEVICE_NOT_FOUND;
        }

    }
    
    if(handle->acc_gyro_address == 0x00 || handle->mag_address == 0x00){
        return ERR_DEVICE_NOT_FOUND;
    }

    return ERR_OK;


}

/// @brief Read a register from the IMU
/// @param handle Pointer to IMU handle
/// @param device Device to read from (Acc+gryo or Mag)
/// @param address Register address
/// @return ERR_OK on success. 
static floatsat_err_t IMU_ReadReg(const IMU_handle_t *handle, uint8_t device, 
                                  uint8_t reg,  uint8_t *data)
{
    if( !handle || !data || device > LSM9DS1_DEV_MAG){
        return ERR_INVALID_ARG;
    }

    // Check if the address is valid. The datasheet said that access to invalid 
    // ones can damage the IMU permanently!
    if( !LSM9DS1_IsValidReg(reg, device)){
        return ERR_INVALID_REG;
    }

    uint8_t dev_addr = 0x00;
    HAL_StatusTypeDef status = HAL_OK;

    dev_addr = (device == LSM9DS1_DEV_AG ? 
                handle->acc_gyro_address : handle->mag_address);

    status = HAL_I2C_Mem_Read(handle->i2c_handle, dev_addr << 1, reg,
                              1U, data, 1U, LSM9DS1_I2C_TIMEOUT);

    if(status != HAL_OK){
        return ERR_DEV_I2C_FAIL;
    }

    return ERR_OK;
}
 
/// @brief Read multiple subsequent registers. 
/// @param handle IMU device handle
/// @param device Device to read from (Acc+Gyro or Mag)
/// @param address Starting address to read
/// @param data Buffer to store the data to
/// @param data_len Number of bytes to read
/// @return 
static floatsat_err_t IMU_ReadRegInc(const IMU_handle_t *handle, uint8_t device,
                                        uint8_t reg, uint8_t *data, uint8_t data_len)
{
    /*  The LSM9DS1 implements an auto-increment function, where setting the 8th
        bit in the reg address makes following master reads automatically get
        the contents of subsequent registers. 
        The IF_ADD_INC bit in CTRL_REG8 must be set (it is by default)
    */
    if(!handle || !data || data_len == 0){
        return ERR_INVALID_ARG;
    }

    // Check if there will be invalid registers in the read sequence
    // TODO: There must be a better way of doing this
    for (size_t i = 0; i < data_len; i++){
        if(!LSM9DS1_IsValidReg(reg + i, device)){
            return ERR_INVALID_REG;
        }
    }

    uint8_t dev_addr = (device == LSM9DS1_DEV_AG ? 
            handle->acc_gyro_address : handle->mag_address);
    
    uint8_t reg_addr = reg | LSM9DS1_AUTO_INC_BIT;

    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(handle->i2c_handle, dev_addr << 1, 
                                reg_addr, 1U, data, data_len, LSM9DS1_I2C_TIMEOUT);
    
    if(status != HAL_OK){
        return ERR_DEV_I2C_FAIL;
    }

    return ERR_OK;
}


static floatsat_err_t IMU_WriteReg(const IMU_handle_t *handle, uint8_t device,
                                    uint8_t reg, uint8_t *data)
{
    if( !handle || !data || device > LSM9DS1_DEV_MAG){
        return ERR_INVALID_ARG;
    }

    // Check if the address is valid. The datasheet said that access to invalid 
    // ones can damage the IMU permanently!
    if( !LSM9DS1_IsValidReg(reg, device)){
        return ERR_INVALID_REG;
    }
    
    uint8_t dev_addr = 0x00;
    HAL_StatusTypeDef status = HAL_OK;

    dev_addr = (device == LSM9DS1_DEV_AG ? 
                handle->acc_gyro_address : handle->mag_address);

    status = HAL_I2C_Mem_Write(handle->i2c_handle, dev_addr << 1, reg, 
                                1U, data, 1U, LSM9DS1_I2C_TIMEOUT);

    if(status != HAL_OK){
        return ERR_DEV_I2C_FAIL;
    }

    return ERR_OK;
}

static floatsat_err_t IMU_WriteRegInc(const IMU_handle_t *handle, uint8_t device,
                                        uint8_t reg, uint8_t *data, uint8_t data_len)
{
    #warning "Not implemented yet!"

    return ERR_OK;
}


/* =========================== Helper Functions ============================= */



// Check if the address is not a reserved register in the acc+gyro device
STATIC INLINE bool LSM9DS1_IsValidDev0Reg(uint8_t reg)
{
    return (reg > 0x03U 
            && (reg != 0x0EU) && (reg != 0x25U)
            && reg < 0x38U);
}

// Check if the address is not a reserved register in the mag device
STATIC INLINE bool LSM9DS1_IsValidDev1Reg(uint8_t reg)
{
    return ( reg > 0x04
            && (reg < 0x0BU || reg > 0x0EU)
            && (reg < 0x10U || reg > 0x1FU)
            && (reg < 0x25U || reg > 0x26U)
            && (reg < 0x2EU || reg > 0x2FU)
            && reg < 0x34U    
        );
}

STATIC INLINE bool LSM9DS1_IsValidReg(uint8_t reg, uint8_t dev)
{
    return ((dev == LSM9DS1_DEV_AG && LSM9DS1_IsValidDev0Reg(reg)) || 
        (dev == LSM9DS1_DEV_MAG && LSM9DS1_IsValidDev1Reg(reg)) );
}