################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include leather_gauge_controller/osal/portable/zephyr/subdir.mk
-include leather_gauge_controller/osal/portable/windows/subdir.mk
-include leather_gauge_controller/osal/portable/ucos3/subdir.mk
-include leather_gauge_controller/osal/portable/ucos2/subdir.mk
-include leather_gauge_controller/osal/portable/threadx/subdir.mk
-include leather_gauge_controller/osal/portable/sys_bios/subdir.mk
-include leather_gauge_controller/osal/portable/safertos/subdir.mk
-include leather_gauge_controller/osal/portable/rtx/subdir.mk
-include leather_gauge_controller/osal/portable/px5/subdir.mk
-include leather_gauge_controller/osal/portable/posix/subdir.mk
-include leather_gauge_controller/osal/portable/none/subdir.mk
-include leather_gauge_controller/osal/portable/freertos/subdir.mk
-include leather_gauge_controller/osal/portable/embos/subdir.mk
-include leather_gauge_controller/osal/portable/cmx_rtx/subdir.mk
-include leather_gauge_controller/osal/portable/cmsis_rtos2/subdir.mk
-include leather_gauge_controller/osal/portable/cmsis_rtos/subdir.mk
-include leather_gauge_controller/osal/portable/chibios/subdir.mk
-include leather_gauge_controller/osal/common/subdir.mk
-include leather_gauge_controller/modules/rtc/subdir.mk
-include leather_gauge_controller/modules/printer/subdir.mk
-include leather_gauge_controller/modules/modbus/subdir.mk
-include leather_gauge_controller/modules/encoder/subdir.mk
-include leather_gauge_controller/modules/eeprom/subdir.mk
-include leather_gauge_controller/modules/di/subdir.mk
-include leather_gauge_controller/middlewares/stm32_log/subdir.mk
-include leather_gauge_controller/middlewares/nanoMODBUS/subdir.mk
-include leather_gauge_controller/middlewares/lwrb/src/lwrb/subdir.mk
-include leather_gauge_controller/middlewares/lwprintf/src/system/subdir.mk
-include leather_gauge_controller/middlewares/lwprintf/src/lwprintf/subdir.mk
-include leather_gauge_controller/middlewares/lwbtn/src/lwbtn/subdir.mk
-include leather_gauge_controller/middlewares/dwin/subdir.mk
-include leather_gauge_controller/middlewares/at24cxx/subdir.mk
-include leather_gauge_controller/middlewares/DSP_Biquad/subdir.mk
-include leather_gauge_controller/app/src/printer/subdir.mk
-include leather_gauge_controller/app/src/hmi/subdir.mk
-include leather_gauge_controller/app/src/subdir.mk
-include USBX/App/subdir.mk
-include Middlewares/ST/usbx/common/usbx_stm32_host_controllers/subdir.mk
-include Middlewares/ST/usbx/common/usbx_host_classes/src/subdir.mk
-include Middlewares/ST/usbx/common/core/src/subdir.mk
-include Middlewares/ST/threadx/ports/cortex_m4/gnu/src/subdir.mk
-include Middlewares/ST/threadx/common/src/subdir.mk
-include Drivers/STM32F4xx_HAL_Driver/Src/subdir.mk
-include Core/Startup/subdir.mk
-include Core/Src/subdir.mk
-include AZURE_RTOS/App/subdir.mk
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


BUILD_ARTIFACT_NAME := leather_gauge_controller
BUILD_ARTIFACT_EXTENSION := elf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 
EXECUTABLES += \
leather_gauge_controller.elf \

MAP_FILES += \
leather_gauge_controller.map \

SIZE_OUTPUT += \
default.size.stdout \

OBJDUMP_LIST += \
leather_gauge_controller.list \


# All Target
all: main-build

# Main-build Target
main-build: leather_gauge_controller.elf secondary-outputs

# Tool invocations
leather_gauge_controller.elf leather_gauge_controller.map: $(OBJS) $(USER_OBJS) /home/tecna-smart-lab/GitHub/leather_gauge/Firmware/leather_gauge_controller/STM32F446RCTX_FLASH.ld makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-gcc -o "leather_gauge_controller.elf" @"objects.list" $(USER_OBJS) $(LIBS) -mcpu=cortex-m4 -T"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/leather_gauge_controller/STM32F446RCTX_FLASH.ld" --specs=nosys.specs -Wl,-Map="leather_gauge_controller.map" -Wl,--gc-sections -static --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -Wl,--start-group -lc -lm -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

default.size.stdout: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-size  $(EXECUTABLES)
	@echo 'Finished building: $@'
	@echo ' '

leather_gauge_controller.list: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objdump -h -S $(EXECUTABLES) > "leather_gauge_controller.list"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) default.size.stdout leather_gauge_controller.elf leather_gauge_controller.list leather_gauge_controller.map
	-@echo ' '

secondary-outputs: $(SIZE_OUTPUT) $(OBJDUMP_LIST)

fail-specified-linker-script-missing:
	@echo 'Error: Cannot find the specified linker script. Check the linker settings in the build configuration.'
	@exit 2

warn-no-linker-script-specified:
	@echo 'Warning: No linker script specified. Check the linker settings in the build configuration.'

.PHONY: all clean dependents main-build fail-specified-linker-script-missing warn-no-linker-script-specified

-include ../makefile.targets
