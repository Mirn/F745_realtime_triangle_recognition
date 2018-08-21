################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/FSK_modem.c \
../src/FSK_modem_test.c \
../src/hw_init.c \
../src/img_test.c \
../src/img_test_main.c \
../src/main.c \
../src/memtest.c \
../src/sysFastMemCopy.c \
../src/triangle_find.c 

OBJS += \
./src/FSK_modem.o \
./src/FSK_modem_test.o \
./src/hw_init.o \
./src/img_test.o \
./src/img_test_main.o \
./src/main.o \
./src/memtest.o \
./src/sysFastMemCopy.o \
./src/triangle_find.o 

C_DEPS += \
./src/FSK_modem.d \
./src/FSK_modem_test.d \
./src/hw_init.d \
./src/img_test.d \
./src/img_test_main.d \
./src/main.d \
./src/memtest.d \
./src/sysFastMemCopy.d \
./src/triangle_find.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -mcpu=cortex-m7 -O3 -fmessage-length=0 -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -Wunused -Wuninitialized -Wall -Wextra -Wpointer-arith -Wshadow -Wlogical-op -Waggregate-return -Wfloat-equal -Wno-sign-compare -DSTM32F746xx -I../src/cmsis -I../src/cmsis_boot -I../src/kiss/inc -I../src/usart_lib -I../src/cmsis_lib/include_inline -I../src -I../src/hal/include -I../src/img_data -I../src/BSP -std=gnu11 -Wbad-function-cast -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


