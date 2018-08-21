################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/BSP/stm32746g_discovery.c \
../src/BSP/stm32746g_discovery_audio.c \
../src/BSP/stm32746g_discovery_camera.c \
../src/BSP/stm32746g_discovery_eeprom.c \
../src/BSP/stm32746g_discovery_lcd.c \
../src/BSP/stm32746g_discovery_qspi.c \
../src/BSP/stm32746g_discovery_sd.c \
../src/BSP/stm32746g_discovery_sdram.c \
../src/BSP/stm32746g_discovery_ts.c 

OBJS += \
./src/BSP/stm32746g_discovery.o \
./src/BSP/stm32746g_discovery_audio.o \
./src/BSP/stm32746g_discovery_camera.o \
./src/BSP/stm32746g_discovery_eeprom.o \
./src/BSP/stm32746g_discovery_lcd.o \
./src/BSP/stm32746g_discovery_qspi.o \
./src/BSP/stm32746g_discovery_sd.o \
./src/BSP/stm32746g_discovery_sdram.o \
./src/BSP/stm32746g_discovery_ts.o 

C_DEPS += \
./src/BSP/stm32746g_discovery.d \
./src/BSP/stm32746g_discovery_audio.d \
./src/BSP/stm32746g_discovery_camera.d \
./src/BSP/stm32746g_discovery_eeprom.d \
./src/BSP/stm32746g_discovery_lcd.d \
./src/BSP/stm32746g_discovery_qspi.d \
./src/BSP/stm32746g_discovery_sd.d \
./src/BSP/stm32746g_discovery_sdram.d \
./src/BSP/stm32746g_discovery_ts.d 


# Each subdirectory must supply rules for building sources it contributes
src/BSP/%.o: ../src/BSP/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -mcpu=cortex-m7 -O3 -fmessage-length=0 -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -Wunused -Wuninitialized -Wall -Wextra -Wpointer-arith -Wshadow -Wlogical-op -Waggregate-return -Wfloat-equal -Wno-sign-compare -DSTM32F746xx -I../src/cmsis -I../src/cmsis_boot -I../src/kiss/inc -I../src/usart_lib -I../src/cmsis_lib/include_inline -I../src -I../src/hal/include -I../src/img_data -I../src/BSP -std=gnu11 -Wbad-function-cast -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


