################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../leather_gauge_controller/osal/portable/embos/os_port_embos.c 

OBJS += \
./leather_gauge_controller/osal/portable/embos/os_port_embos.o 

C_DEPS += \
./leather_gauge_controller/osal/portable/embos/os_port_embos.d 


# Each subdirectory must supply rules for building sources it contributes
leather_gauge_controller/osal/portable/embos/%.o leather_gauge_controller/osal/portable/embos/%.su leather_gauge_controller/osal/portable/embos/%.cyclo: ../leather_gauge_controller/osal/portable/embos/%.c leather_gauge_controller/osal/portable/embos/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F446xx -DTX_INCLUDE_USER_DEFINE_FILE -DUX_INCLUDE_USER_DEFINE_FILE -c -I../Core/Inc -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/leather_gauge_controller/leather_gauge_controller/modules/encoder" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/leather_gauge_controller/leather_gauge_controller/modules/di" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/middlewares/lwbtn/src/include" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/middlewares/lwbtn/src/include/lwbtn" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/leather_gauge_controller/leather_gauge_controller/app/src/hmi" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/leather_gauge_controller/leather_gauge_controller/modules/modbus" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/leather_gauge_controller/leather_gauge_controller/modules/printer" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/middlewares/lwprintf/src/include" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/middlewares/lwprintf/src/include/lwprintf" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/middlewares/lwprintf/src/include/system" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/middlewares/stm32_log" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/leather_gauge_controller/leather_gauge_controller/app/inc" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/leather_gauge_controller/leather_gauge_controller/config" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/middlewares/at24cxx" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/middlewares/dwin" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/middlewares/lwrb/src/include" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/middlewares/lwrb/src/include/lwrb" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/middlewares/nanoMODBUS" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/leather_gauge_controller/leather_gauge_controller/modules/eeprom" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/leather_gauge_controller/leather_gauge_controller/osal/common" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/leather_gauge_controller/leather_gauge_controller/osal/include" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/leather_gauge_controller/leather_gauge_controller/osal/portable/threadx" -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../AZURE_RTOS/App -I../USBX/App -I../USBX/Target -I../Middlewares/ST/usbx/common/usbx_host_classes/inc/ -I../Middlewares/ST/usbx/common/core/inc/ -I../Middlewares/ST/usbx/ports/generic/inc/ -I../Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ -I../Middlewares/ST/threadx/common/inc/ -I../Middlewares/ST/threadx/ports/cortex_m4/gnu/inc/ -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-leather_gauge_controller-2f-osal-2f-portable-2f-embos

clean-leather_gauge_controller-2f-osal-2f-portable-2f-embos:
	-$(RM) ./leather_gauge_controller/osal/portable/embos/os_port_embos.cyclo ./leather_gauge_controller/osal/portable/embos/os_port_embos.d ./leather_gauge_controller/osal/portable/embos/os_port_embos.o ./leather_gauge_controller/osal/portable/embos/os_port_embos.su

.PHONY: clean-leather_gauge_controller-2f-osal-2f-portable-2f-embos

