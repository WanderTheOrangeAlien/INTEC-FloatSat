################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/UnitTests/LSM9DS1_tests.c 

OBJS += \
./Core/Src/UnitTests/LSM9DS1_tests.o 

C_DEPS += \
./Core/Src/UnitTests/LSM9DS1_tests.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/UnitTests/%.o Core/Src/UnitTests/%.su Core/Src/UnitTests/%.cyclo: ../Core/Src/UnitTests/%.c Core/Src/UnitTests/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../build/vendor/cmock/src -I../build/vendor/unity/src -I../App/Src -I../App/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-UnitTests

clean-Core-2f-Src-2f-UnitTests:
	-$(RM) ./Core/Src/UnitTests/LSM9DS1_tests.cyclo ./Core/Src/UnitTests/LSM9DS1_tests.d ./Core/Src/UnitTests/LSM9DS1_tests.o ./Core/Src/UnitTests/LSM9DS1_tests.su

.PHONY: clean-Core-2f-Src-2f-UnitTests

