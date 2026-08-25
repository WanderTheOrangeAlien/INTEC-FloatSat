################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/LSM9DS1_stm32.c \
../Core/Src/floatsat_gyro_calibration.c \
../Core/Src/floatsat_imu_old.c \
../Core/Src/floatsat_init.c \
../Core/Src/floatsat_madgwick.c \
../Core/Src/floatsat_mag_calibration.c \
../Core/Src/floatsat_mission.c \
../Core/Src/floatsat_orientation.c \
../Core/Src/freertos.c \
../Core/Src/gpio.c \
../Core/Src/i2c.c \
../Core/Src/main.c \
../Core/Src/stm32f4xx_hal_msp.c \
../Core/Src/stm32f4xx_hal_timebase_tim.c \
../Core/Src/stm32f4xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32f4xx.c \
../Core/Src/tim.c \
../Core/Src/usart.c \
../Core/Src/vec_math.c 

OBJS += \
./Core/Src/LSM9DS1_stm32.o \
./Core/Src/floatsat_gyro_calibration.o \
./Core/Src/floatsat_imu_old.o \
./Core/Src/floatsat_init.o \
./Core/Src/floatsat_madgwick.o \
./Core/Src/floatsat_mag_calibration.o \
./Core/Src/floatsat_mission.o \
./Core/Src/floatsat_orientation.o \
./Core/Src/freertos.o \
./Core/Src/gpio.o \
./Core/Src/i2c.o \
./Core/Src/main.o \
./Core/Src/stm32f4xx_hal_msp.o \
./Core/Src/stm32f4xx_hal_timebase_tim.o \
./Core/Src/stm32f4xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32f4xx.o \
./Core/Src/tim.o \
./Core/Src/usart.o \
./Core/Src/vec_math.o 

C_DEPS += \
./Core/Src/LSM9DS1_stm32.d \
./Core/Src/floatsat_gyro_calibration.d \
./Core/Src/floatsat_imu_old.d \
./Core/Src/floatsat_init.d \
./Core/Src/floatsat_madgwick.d \
./Core/Src/floatsat_mag_calibration.d \
./Core/Src/floatsat_mission.d \
./Core/Src/floatsat_orientation.d \
./Core/Src/freertos.d \
./Core/Src/gpio.d \
./Core/Src/i2c.d \
./Core/Src/main.d \
./Core/Src/stm32f4xx_hal_msp.d \
./Core/Src/stm32f4xx_hal_timebase_tim.d \
./Core/Src/stm32f4xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32f4xx.d \
./Core/Src/tim.d \
./Core/Src/usart.d \
./Core/Src/vec_math.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/LSM9DS1_stm32.cyclo ./Core/Src/LSM9DS1_stm32.d ./Core/Src/LSM9DS1_stm32.o ./Core/Src/LSM9DS1_stm32.su ./Core/Src/floatsat_gyro_calibration.cyclo ./Core/Src/floatsat_gyro_calibration.d ./Core/Src/floatsat_gyro_calibration.o ./Core/Src/floatsat_gyro_calibration.su ./Core/Src/floatsat_imu_old.cyclo ./Core/Src/floatsat_imu_old.d ./Core/Src/floatsat_imu_old.o ./Core/Src/floatsat_imu_old.su ./Core/Src/floatsat_init.cyclo ./Core/Src/floatsat_init.d ./Core/Src/floatsat_init.o ./Core/Src/floatsat_init.su ./Core/Src/floatsat_madgwick.cyclo ./Core/Src/floatsat_madgwick.d ./Core/Src/floatsat_madgwick.o ./Core/Src/floatsat_madgwick.su ./Core/Src/floatsat_mag_calibration.cyclo ./Core/Src/floatsat_mag_calibration.d ./Core/Src/floatsat_mag_calibration.o ./Core/Src/floatsat_mag_calibration.su ./Core/Src/floatsat_mission.cyclo ./Core/Src/floatsat_mission.d ./Core/Src/floatsat_mission.o ./Core/Src/floatsat_mission.su ./Core/Src/floatsat_orientation.cyclo ./Core/Src/floatsat_orientation.d ./Core/Src/floatsat_orientation.o ./Core/Src/floatsat_orientation.su ./Core/Src/freertos.cyclo ./Core/Src/freertos.d ./Core/Src/freertos.o ./Core/Src/freertos.su ./Core/Src/gpio.cyclo ./Core/Src/gpio.d ./Core/Src/gpio.o ./Core/Src/gpio.su ./Core/Src/i2c.cyclo ./Core/Src/i2c.d ./Core/Src/i2c.o ./Core/Src/i2c.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/stm32f4xx_hal_msp.cyclo ./Core/Src/stm32f4xx_hal_msp.d ./Core/Src/stm32f4xx_hal_msp.o ./Core/Src/stm32f4xx_hal_msp.su ./Core/Src/stm32f4xx_hal_timebase_tim.cyclo ./Core/Src/stm32f4xx_hal_timebase_tim.d ./Core/Src/stm32f4xx_hal_timebase_tim.o ./Core/Src/stm32f4xx_hal_timebase_tim.su ./Core/Src/stm32f4xx_it.cyclo ./Core/Src/stm32f4xx_it.d ./Core/Src/stm32f4xx_it.o ./Core/Src/stm32f4xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32f4xx.cyclo ./Core/Src/system_stm32f4xx.d ./Core/Src/system_stm32f4xx.o ./Core/Src/system_stm32f4xx.su ./Core/Src/tim.cyclo ./Core/Src/tim.d ./Core/Src/tim.o ./Core/Src/tim.su ./Core/Src/usart.cyclo ./Core/Src/usart.d ./Core/Src/usart.o ./Core/Src/usart.su ./Core/Src/vec_math.cyclo ./Core/Src/vec_math.d ./Core/Src/vec_math.o ./Core/Src/vec_math.su

.PHONY: clean-Core-2f-Src

