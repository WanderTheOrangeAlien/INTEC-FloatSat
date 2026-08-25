// Online C compiler to run C program online
#include <stdio.h>
#include <stdint.h>

static inline uint8_t LSM9DS1_IsValidDev0Reg(uint8_t reg){

    return (reg > 0x04U 
            && (reg != 0x0EU) && (reg != 0x25U)
            && reg < 0x38U);
}

static inline uint8_t LSM9DS1_IsValidDev1Reg(uint8_t reg)
{
    return ( reg > 0x03
            && (reg < 0x0BU || reg > 0x0EU)
            && (reg < 0x10U || reg > 0x1FU)
            && (reg < 0x25U || reg > 0x26U)
            && (reg < 0x2EU || reg > 0x2FU)
            && reg < 0x34U    
        );
}

uint8_t test_regs_dev0[] = {0,1,2,0x0E, 0x25, 0x38, 0xFF}; 
uint8_t test_regs_dev0_len = sizeof(test_regs_dev0);

uint8_t test_regs_dev1[] = {   
    0,1,2,3,
    0x0B, 0x0C, 0x0D, 0x0E, 
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    0x25, 0x26,
    0x2E, 0x2F,
    0x34, 0xFF
    }; 
uint8_t test_regs_dev1_len = sizeof(test_regs_dev1);


int Test_IMU_CheckInvalidRegs() {
    // Write C code here
    uint8_t is_valid = 0;
    for(int i = 0; i < test_regs_dev0_len; i++){
        is_valid = LSM9DS1_IsValidDev0Reg(test_regs_dev0[i]);

        if(is_valid){
            printf("FAILED WITH REG: 0x%02x\n",test_regs_dev0[i]);
            return -1;
        }
    }
    printf("Test 1 succeded\n");

    for(int i = 0; i < test_regs_dev1_len; i++){
        is_valid = LSM9DS1_IsValidDev1Reg(test_regs_dev1[i]);

        if(is_valid){
            printf("FAILED WITH REG: 0x%02x\n",test_regs_dev1[i]);
            return -1;
        }
    }
    printf("Test 2 succeded\n");
    



    return 0;
}