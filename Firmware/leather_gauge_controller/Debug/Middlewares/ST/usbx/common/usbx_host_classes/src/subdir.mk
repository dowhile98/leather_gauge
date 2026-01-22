################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_activate.c \
../Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_configure.c \
../Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_deactivate.c \
../Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_device_id_get.c \
../Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_endpoints_get.c \
../Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_entry.c \
../Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_name_get.c \
../Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_read.c \
../Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_soft_reset.c \
../Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_status_get.c \
../Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_write.c 

OBJS += \
./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_activate.o \
./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_configure.o \
./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_deactivate.o \
./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_device_id_get.o \
./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_endpoints_get.o \
./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_entry.o \
./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_name_get.o \
./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_read.o \
./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_soft_reset.o \
./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_status_get.o \
./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_write.o 

C_DEPS += \
./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_activate.d \
./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_configure.d \
./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_deactivate.d \
./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_device_id_get.d \
./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_endpoints_get.d \
./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_entry.d \
./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_name_get.d \
./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_read.d \
./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_soft_reset.d \
./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_status_get.d \
./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_write.d 


# Each subdirectory must supply rules for building sources it contributes
Middlewares/ST/usbx/common/usbx_host_classes/src/%.o Middlewares/ST/usbx/common/usbx_host_classes/src/%.su Middlewares/ST/usbx/common/usbx_host_classes/src/%.cyclo: ../Middlewares/ST/usbx/common/usbx_host_classes/src/%.c Middlewares/ST/usbx/common/usbx_host_classes/src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F446xx -DTX_INCLUDE_USER_DEFINE_FILE -DUX_INCLUDE_USER_DEFINE_FILE -c -I../Core/Inc -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/leather_gauge_controller/leather_gauge_controller/modules/rtc" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/leather_gauge_controller/leather_gauge_controller/modules/encoder" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/leather_gauge_controller/leather_gauge_controller/modules/di" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/middlewares/lwbtn/src/include" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/middlewares/lwbtn/src/include/lwbtn" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/leather_gauge_controller/leather_gauge_controller/app/src/hmi" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/leather_gauge_controller/leather_gauge_controller/modules/modbus" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/leather_gauge_controller/leather_gauge_controller/modules/printer" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/middlewares/lwprintf/src/include" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/middlewares/lwprintf/src/include/lwprintf" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/middlewares/lwprintf/src/include/system" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/middlewares/stm32_log" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/leather_gauge_controller/leather_gauge_controller/app/inc" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/leather_gauge_controller/leather_gauge_controller/config" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/middlewares/at24cxx" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/middlewares/dwin" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/middlewares/lwrb/src/include" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/middlewares/lwrb/src/include/lwrb" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/middlewares/nanoMODBUS" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/leather_gauge_controller/leather_gauge_controller/modules/eeprom" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/leather_gauge_controller/leather_gauge_controller/osal/common" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/leather_gauge_controller/leather_gauge_controller/osal/include" -I"C:/Users/eplim/Downloads/leather_gauge-main/leather_gauge-main/Firmware/leather_gauge_controller/leather_gauge_controller/osal/portable/threadx" -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../AZURE_RTOS/App -I../USBX/App -I../USBX/Target -I../Middlewares/ST/usbx/common/usbx_host_classes/inc/ -I../Middlewares/ST/usbx/common/core/inc/ -I../Middlewares/ST/usbx/ports/generic/inc/ -I../Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ -I../Middlewares/ST/threadx/common/inc/ -I../Middlewares/ST/threadx/ports/cortex_m4/gnu/inc/ -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Middlewares-2f-ST-2f-usbx-2f-common-2f-usbx_host_classes-2f-src

clean-Middlewares-2f-ST-2f-usbx-2f-common-2f-usbx_host_classes-2f-src:
	-$(RM) ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_activate.cyclo ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_activate.d ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_activate.o ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_activate.su ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_configure.cyclo ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_configure.d ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_configure.o ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_configure.su ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_deactivate.cyclo ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_deactivate.d ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_deactivate.o ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_deactivate.su ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_device_id_get.cyclo ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_device_id_get.d ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_device_id_get.o ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_device_id_get.su ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_endpoints_get.cyclo ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_endpoints_get.d ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_endpoints_get.o ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_endpoints_get.su ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_entry.cyclo ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_entry.d ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_entry.o ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_entry.su ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_name_get.cyclo ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_name_get.d ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_name_get.o ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_name_get.su ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_read.cyclo ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_read.d ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_read.o ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_read.su ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_soft_reset.cyclo ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_soft_reset.d ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_soft_reset.o ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_soft_reset.su ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_status_get.cyclo ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_status_get.d ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_status_get.o ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_status_get.su ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_write.cyclo ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_write.d ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_write.o ./Middlewares/ST/usbx/common/usbx_host_classes/src/ux_host_class_printer_write.su

.PHONY: clean-Middlewares-2f-ST-2f-usbx-2f-common-2f-usbx_host_classes-2f-src

