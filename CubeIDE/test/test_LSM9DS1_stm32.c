
#ifdef TEST

#include "unity.h"

// Mocks
#include "mock_stm32f4xx_hal.h"         // For HAL_GetTick() used in logging
#include "mock_stm32f4xx_hal_i2c.h"     // HAL I2C functions


// File under test
#include "LSM9DS1_stm32.h"  




void setUp(void)
{
}

void tearDown(void)
{
}

void test_InvalidRegisters(void)
{
    uint8_t invalid_regs_ag[] = {
        0x00, 0x01, 0x02, 0x03,
        0x0E,
        0x25
    };
    for (size_t i = 0; i < sizeof(invalid_regs_ag); i++){
        TEST_ASSERT_FALSE(LSM9DS1_IsValidReg(invalid_regs_ag[i],LSM9DS1_DEV_AG));
    }
    // Test for range 0x38-0x7F
    for (size_t i = 0x38; i < 0x80; i++){
        TEST_ASSERT_FALSE(LSM9DS1_IsValidReg(i,LSM9DS1_DEV_AG));
    }
    
    uint8_t invalid_regs_mag[] = {
        0x00, 0x01, 0x02, 0x03, 0x04,
        0x0B, 0x0C, 0x0D, 0x0E,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
        0x25, 0x26,
        0x2E, 0x2F,
    };

    for (size_t i = 0; i < sizeof(invalid_regs_mag); i++){
        TEST_ASSERT_FALSE(LSM9DS1_IsValidReg(invalid_regs_mag[i],LSM9DS1_DEV_MAG));
    }
    // Test for range 0x34-0x7F
    for (size_t i = 0x34; i < 0x80; i++){
        TEST_ASSERT_FALSE(LSM9DS1_IsValidReg(i,LSM9DS1_DEV_MAG));
    }
}

// TODO: Add a valid registers test



#endif // TEST
