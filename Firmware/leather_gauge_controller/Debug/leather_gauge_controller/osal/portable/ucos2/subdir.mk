################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../leather_gauge_controller/osal/portable/ucos2/os_port_ucos2.c 

OBJS += \
./leather_gauge_controller/osal/portable/ucos2/os_port_ucos2.o 

C_DEPS += \
./leather_gauge_controller/osal/portable/ucos2/os_port_ucos2.d 


# Each subdirectory must supply rules for building sources it contributes
leather_gauge_controller/osal/portable/ucos2/%.o leather_gauge_controller/osal/portable/ucos2/%.su leather_gauge_controller/osal/portable/ucos2/%.cyclo: ../leather_gauge_controller/osal/portable/ucos2/%.c leather_gauge_controller/osal/portable/ucos2/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F446xx -DTX_INCLUDE_USER_DEFINE_FILE -DUX_INCLUDE_USER_DEFINE_FILE -c -I../Core/Inc -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/leather_gauge_controller/leather_gauge_controller/modules/rtc" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/leather_gauge_controller/leather_gauge_controller/modules/encoder" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/leather_gauge_controller/leather_gauge_controller/modules/di" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/middlewares/lwbtn/src/include" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/middlewares/lwbtn/src/include/lwbtn" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/leather_gauge_controller/leather_gauge_controller/app/src/hmi" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/leather_gauge_controller/leather_gauge_controller/modules/modbus" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/leather_gauge_controller/leather_gauge_controller/modules/printer" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/middlewares/lwprintf/src/include" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/middlewares/lwprintf/src/include/lwprintf" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/middlewares/lwprintf/src/include/system" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/middlewares/stm32_log" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/leather_gauge_controller/leather_gauge_controller/app/inc" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/leather_gauge_controller/leather_gauge_controller/config" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/middlewares/at24cxx" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/middlewares/dwin" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/middlewares/lwrb/src/include" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/middlewares/lwrb/src/include/lwrb" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/middlewares/nanoMODBUS" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/leather_gauge_controller/leather_gauge_controller/modules/eeprom" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/leather_gauge_controller/leather_gauge_controller/osal/common" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/leather_gauge_controller/leather_gauge_controller/osal/include" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/leather_gauge_controller/leather_gauge_controller/osal/portable/threadx" -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../AZURE_RTOS/App -I../USBX/App -I../USBX/Target -I../Middlewares/ST/usbx/common/usbx_host_classes/inc/ -I../Middlewares/ST/usbx/common/core/inc/ -I../Middlewares/ST/usbx/ports/generic/inc/ -I../Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ -I../Middlewares/ST/threadx/common/inc/ -I../Middlewares/ST/threadx/ports/cortex_m4/gnu/inc/ -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-leather_gauge_controller-2f-osal-2f-portable-2f-ucos2

clean-leather_gauge_controller-2f-osal-2f-portable-2f-ucos2:
	-$(RM) ./leather_gauge_controller/osal/portable/ucos2/os_port_ucos2.cyclo ./leather_gauge_controller/osal/portable/ucos2/os_port_ucos2.d ./leather_gauge_controller/osal/portable/ucos2/os_port_ucos2.o ./leather_gauge_controller/osal/portable/ucos2/os_port_ucos2.su

.PHONY: clean-leather_gauge_controller-2f-osal-2f-portable-2f-ucos2

