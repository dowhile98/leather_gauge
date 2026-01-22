################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../leather_gauge_controller/modules/printer/ESC_POS_Printer.c \
../leather_gauge_controller/modules/printer/lgc_interface_printer.c 

OBJS += \
./leather_gauge_controller/modules/printer/ESC_POS_Printer.o \
./leather_gauge_controller/modules/printer/lgc_interface_printer.o 

C_DEPS += \
./leather_gauge_controller/modules/printer/ESC_POS_Printer.d \
./leather_gauge_controller/modules/printer/lgc_interface_printer.d 


# Each subdirectory must supply rules for building sources it contributes
leather_gauge_controller/modules/printer/%.o leather_gauge_controller/modules/printer/%.su leather_gauge_controller/modules/printer/%.cyclo: ../leather_gauge_controller/modules/printer/%.c leather_gauge_controller/modules/printer/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F446xx -DTX_INCLUDE_USER_DEFINE_FILE -DUX_INCLUDE_USER_DEFINE_FILE -c -I../Core/Inc -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/leather_gauge_controller/leather_gauge_controller/modules/rtc" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/leather_gauge_controller/leather_gauge_controller/modules/encoder" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/leather_gauge_controller/leather_gauge_controller/modules/di" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/middlewares/lwbtn/src/include" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/middlewares/lwbtn/src/include/lwbtn" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/leather_gauge_controller/leather_gauge_controller/app/src/hmi" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/leather_gauge_controller/leather_gauge_controller/modules/modbus" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/leather_gauge_controller/leather_gauge_controller/modules/printer" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/middlewares/lwprintf/src/include" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/middlewares/lwprintf/src/include/lwprintf" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/middlewares/lwprintf/src/include/system" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/middlewares/stm32_log" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/leather_gauge_controller/leather_gauge_controller/app/inc" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/leather_gauge_controller/leather_gauge_controller/config" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/middlewares/at24cxx" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/middlewares/dwin" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/middlewares/lwrb/src/include" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/middlewares/lwrb/src/include/lwrb" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/middlewares/nanoMODBUS" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/leather_gauge_controller/leather_gauge_controller/modules/eeprom" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/leather_gauge_controller/leather_gauge_controller/osal/common" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/leather_gauge_controller/leather_gauge_controller/osal/include" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/leather_gauge_controller/leather_gauge_controller/osal/portable/threadx" -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../AZURE_RTOS/App -I../USBX/App -I../USBX/Target -I../Middlewares/ST/usbx/common/usbx_host_classes/inc/ -I../Middlewares/ST/usbx/common/core/inc/ -I../Middlewares/ST/usbx/ports/generic/inc/ -I../Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ -I../Middlewares/ST/threadx/common/inc/ -I../Middlewares/ST/threadx/ports/cortex_m4/gnu/inc/ -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-leather_gauge_controller-2f-modules-2f-printer

clean-leather_gauge_controller-2f-modules-2f-printer:
	-$(RM) ./leather_gauge_controller/modules/printer/ESC_POS_Printer.cyclo ./leather_gauge_controller/modules/printer/ESC_POS_Printer.d ./leather_gauge_controller/modules/printer/ESC_POS_Printer.o ./leather_gauge_controller/modules/printer/ESC_POS_Printer.su ./leather_gauge_controller/modules/printer/lgc_interface_printer.cyclo ./leather_gauge_controller/modules/printer/lgc_interface_printer.d ./leather_gauge_controller/modules/printer/lgc_interface_printer.o ./leather_gauge_controller/modules/printer/lgc_interface_printer.su

.PHONY: clean-leather_gauge_controller-2f-modules-2f-printer

