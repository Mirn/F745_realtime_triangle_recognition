################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/kiss/src/stm32kiss_ticks.c 

OBJS += \
./src/kiss/src/stm32kiss_ticks.o 

C_DEPS += \
./src/kiss/src/stm32kiss_ticks.d 


# Each subdirectory must supply rules for building sources it contributes
src/kiss/src/%.o: ../src/kiss/src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -mcpu=cortex-m7 -O3 -fmessage-length=0 -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -Wunused -Wuninitialized -Wall -Wextra -Wpointer-arith -Wshadow -Wlogical-op -Waggregate-return -Wfloat-equal -Wno-sign-compare -DSTM32F746xx -I../src/cmsis -I../src/cmsis_boot -I../src/kiss/inc -I../src/usart_lib -I../src -I../src/hal/include -I../src/img_data -I../src/BSP -std=gnu11 -Wbad-function-cast -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


