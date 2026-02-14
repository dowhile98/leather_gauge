################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include leather_gauge_sensor/modules/sensor/subdir.mk
-include leather_gauge_sensor/modules/modbus/subdir.mk
-include leather_gauge_sensor/modules/eeprom/subdir.mk
-include leather_gauge_sensor/middlewares/nanoMODBUS/subdir.mk
-include leather_gauge_sensor/middlewares/lwrb/src/lwrb/subdir.mk
-include leather_gauge_sensor/middlewares/DSP_Biquad/subdir.mk
-include leather_gauge_sensor/app/Src/subdir.mk
-include Drivers/STM32G0xx_HAL_Driver/Src/subdir.mk
-include Core/Startup/subdir.mk
-include Core/Src/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := leather_gauge_sensor_v4
BUILD_ARTIFACT_EXTENSION := elf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 
EXECUTABLES += \
leather_gauge_sensor_v4.elf \

MAP_FILES += \
leather_gauge_sensor_v4.map \

SIZE_OUTPUT += \
default.size.stdout \

OBJDUMP_LIST += \
leather_gauge_sensor_v4.list \


# All Target
all: main-build

# Main-build Target
main-build: leather_gauge_sensor_v4.elf secondary-outputs

# Tool invocations
leather_gauge_sensor_v4.elf leather_gauge_sensor_v4.map: $(OBJS) $(USER_OBJS) /home/tecna-smart-lab/GitHub/leather_gauge/Firmware/leather_gauge_sensor_v4/STM32G030C8TX_FLASH.ld makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-gcc -o "leather_gauge_sensor_v4.elf" @"objects.list" $(USER_OBJS) $(LIBS) -mcpu=cortex-m0plus -T"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/leather_gauge_sensor_v4/STM32G030C8TX_FLASH.ld" --specs=nosys.specs -Wl,-Map="leather_gauge_sensor_v4.map" -Wl,--gc-sections -static --specs=nano.specs -mfloat-abi=soft -mthumb -Wl,--start-group -lc -lm -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

default.size.stdout: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-size  $(EXECUTABLES)
	@echo 'Finished building: $@'
	@echo ' '

leather_gauge_sensor_v4.list: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objdump -h -S $(EXECUTABLES) > "leather_gauge_sensor_v4.list"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) default.size.stdout leather_gauge_sensor_v4.elf leather_gauge_sensor_v4.list leather_gauge_sensor_v4.map
	-@echo ' '

secondary-outputs: $(SIZE_OUTPUT) $(OBJDUMP_LIST)

fail-specified-linker-script-missing:
	@echo 'Error: Cannot find the specified linker script. Check the linker settings in the build configuration.'
	@exit 2

warn-no-linker-script-specified:
	@echo 'Warning: No linker script specified. Check the linker settings in the build configuration.'

.PHONY: all clean dependents main-build fail-specified-linker-script-missing warn-no-linker-script-specified

-include ../makefile.targets
