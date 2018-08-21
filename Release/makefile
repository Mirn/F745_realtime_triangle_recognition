################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/usart_lib/subdir.mk
-include src/syscalls/subdir.mk
-include src/kiss/src/subdir.mk
-include src/hal/src/subdir.mk
-include src/cmsis_boot/startup/subdir.mk
-include src/cmsis_boot/subdir.mk
-include src/BSP/Components/wm8994/subdir.mk
-include src/BSP/Components/ov9655/subdir.mk
-include src/BSP/Components/ft5336/subdir.mk
-include src/BSP/subdir.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_FLASH += \
F745_realtime_triangle_recognition.bin \

SECONDARY_LIST += \
F745_realtime_triangle_recognition.lst \

SECONDARY_SIZE += \
F745_realtime_triangle_recognition.siz \


# All Target
all: F745_realtime_triangle_recognition.elf secondary-outputs

# Tool invocations
F745_realtime_triangle_recognition.elf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: Cross ARM C++ Linker'
	arm-none-eabi-g++ -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -mcpu=cortex-m7 -O3 -fmessage-length=0 -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -Wunused -Wuninitialized -Wall -Wextra -Wpointer-arith -Wshadow -Wlogical-op -Waggregate-return -Wfloat-equal -Wno-sign-compare -T ../ldscripts/arm-gcc-link.ld -nostartfiles -Xlinker --gc-sections -Wl,-Map,"F745_realtime_triangle_recognition.map" --specs=nano.specs -u _printf_float -o "F745_realtime_triangle_recognition.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

F745_realtime_triangle_recognition.bin: F745_realtime_triangle_recognition.elf
	@echo 'Invoking: Cross ARM GNU Create Flash Image'
	arm-none-eabi-objcopy -O binary "F745_realtime_triangle_recognition.elf"  "F745_realtime_triangle_recognition.bin"
	@echo 'Finished building: $@'
	@echo ' '

F745_realtime_triangle_recognition.lst: F745_realtime_triangle_recognition.elf
	@echo 'Invoking: Cross ARM GNU Create Listing'
	arm-none-eabi-objdump --demangle --disassemble --wide "F745_realtime_triangle_recognition.elf" > "F745_realtime_triangle_recognition.lst"
	@echo 'Finished building: $@'
	@echo ' '

F745_realtime_triangle_recognition.siz: F745_realtime_triangle_recognition.elf
	@echo 'Invoking: Cross ARM GNU Print Size'
	arm-none-eabi-size --format=berkeley "F745_realtime_triangle_recognition.elf"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(CC_DEPS)$(C++_DEPS)$(OBJS)$(C_UPPER_DEPS)$(CXX_DEPS)$(SECONDARY_FLASH)$(SECONDARY_LIST)$(SECONDARY_SIZE)$(ASM_DEPS)$(S_UPPER_DEPS)$(C_DEPS)$(CPP_DEPS) F745_realtime_triangle_recognition.elf
	-@echo ' '

secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_LIST) $(SECONDARY_SIZE)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
