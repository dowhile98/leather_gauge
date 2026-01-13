################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_callback.c \
../Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_controller_disable.c \
../Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_ed_obtain.c \
../Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_endpoint_create.c \
../Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_endpoint_destroy.c \
../Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_endpoint_reset.c \
../Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_entry.c \
../Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_frame_number_get.c \
../Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_initialize.c \
../Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_interrupt_handler.c \
../Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_least_traffic_list_get.c \
../Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_periodic_schedule.c \
../Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_port_disable.c \
../Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_port_enable.c \
../Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_port_reset.c \
../Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_port_resume.c \
../Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_port_status_get.c \
../Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_port_suspend.c \
../Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_power_down_port.c \
../Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_power_on_port.c \
../Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_request_bulk_transfer.c \
../Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_request_control_transfer.c \
../Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_request_periodic_transfer.c \
../Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_request_transfer.c \
../Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_transfer_abort.c 

OBJS += \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_callback.o \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_controller_disable.o \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_ed_obtain.o \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_endpoint_create.o \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_endpoint_destroy.o \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_endpoint_reset.o \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_entry.o \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_frame_number_get.o \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_initialize.o \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_interrupt_handler.o \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_least_traffic_list_get.o \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_periodic_schedule.o \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_port_disable.o \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_port_enable.o \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_port_reset.o \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_port_resume.o \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_port_status_get.o \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_port_suspend.o \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_power_down_port.o \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_power_on_port.o \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_request_bulk_transfer.o \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_request_control_transfer.o \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_request_periodic_transfer.o \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_request_transfer.o \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_transfer_abort.o 

C_DEPS += \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_callback.d \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_controller_disable.d \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_ed_obtain.d \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_endpoint_create.d \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_endpoint_destroy.d \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_endpoint_reset.d \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_entry.d \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_frame_number_get.d \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_initialize.d \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_interrupt_handler.d \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_least_traffic_list_get.d \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_periodic_schedule.d \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_port_disable.d \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_port_enable.d \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_port_reset.d \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_port_resume.d \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_port_status_get.d \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_port_suspend.d \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_power_down_port.d \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_power_on_port.d \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_request_bulk_transfer.d \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_request_control_transfer.d \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_request_periodic_transfer.d \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_request_transfer.d \
./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_transfer_abort.d 


# Each subdirectory must supply rules for building sources it contributes
Middlewares/ST/usbx/common/usbx_stm32_host_controllers/%.o Middlewares/ST/usbx/common/usbx_stm32_host_controllers/%.su Middlewares/ST/usbx/common/usbx_stm32_host_controllers/%.cyclo: ../Middlewares/ST/usbx/common/usbx_stm32_host_controllers/%.c Middlewares/ST/usbx/common/usbx_stm32_host_controllers/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F446xx -DTX_INCLUDE_USER_DEFINE_FILE -DUX_INCLUDE_USER_DEFINE_FILE -c -I../Core/Inc -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/leather_gauge_controller/leather_gauge_controller/app/src/hmi" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/leather_gauge_controller/leather_gauge_controller/modules/modbus" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/leather_gauge_controller/leather_gauge_controller/modules/printer" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/middlewares/lwprintf/src/include" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/middlewares/lwprintf/src/include/lwprintf" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/middlewares/lwprintf/src/include/system" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/middlewares/stm32_log" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/leather_gauge_controller/leather_gauge_controller/app/inc" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/leather_gauge_controller/leather_gauge_controller/config" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/middlewares/at24cxx" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/middlewares/dwin" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/middlewares/lwrb/src/include" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/middlewares/lwrb/src/include/lwrb" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/middlewares/nanoMODBUS" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/leather_gauge_controller/leather_gauge_controller/modules/eeprom" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/leather_gauge_controller/leather_gauge_controller/osal/common" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/leather_gauge_controller/leather_gauge_controller/osal/include" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/leather_gauge_controller/leather_gauge_controller/osal/portable/threadx" -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../AZURE_RTOS/App -I../USBX/App -I../USBX/Target -I../Middlewares/ST/usbx/common/usbx_host_classes/inc/ -I../Middlewares/ST/usbx/common/core/inc/ -I../Middlewares/ST/usbx/ports/generic/inc/ -I../Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ -I../Middlewares/ST/threadx/common/inc/ -I../Middlewares/ST/threadx/ports/cortex_m4/gnu/inc/ -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Middlewares-2f-ST-2f-usbx-2f-common-2f-usbx_stm32_host_controllers

clean-Middlewares-2f-ST-2f-usbx-2f-common-2f-usbx_stm32_host_controllers:
	-$(RM) ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_callback.cyclo ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_callback.d ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_callback.o ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_callback.su ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_controller_disable.cyclo ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_controller_disable.d ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_controller_disable.o ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_controller_disable.su ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_ed_obtain.cyclo ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_ed_obtain.d ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_ed_obtain.o ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_ed_obtain.su ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_endpoint_create.cyclo ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_endpoint_create.d ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_endpoint_create.o ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_endpoint_create.su ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_endpoint_destroy.cyclo ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_endpoint_destroy.d ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_endpoint_destroy.o ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_endpoint_destroy.su ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_endpoint_reset.cyclo ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_endpoint_reset.d ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_endpoint_reset.o ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_endpoint_reset.su ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_entry.cyclo ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_entry.d ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_entry.o ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_entry.su ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_frame_number_get.cyclo ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_frame_number_get.d ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_frame_number_get.o ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_frame_number_get.su ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_initialize.cyclo ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_initialize.d ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_initialize.o ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_initialize.su ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_interrupt_handler.cyclo ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_interrupt_handler.d ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_interrupt_handler.o ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_interrupt_handler.su ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_least_traffic_list_get.cyclo ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_least_traffic_list_get.d ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_least_traffic_list_get.o ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_least_traffic_list_get.su ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_periodic_schedule.cyclo ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_periodic_schedule.d ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_periodic_schedule.o ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_periodic_schedule.su ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_port_disable.cyclo ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_port_disable.d ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_port_disable.o ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_port_disable.su ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_port_enable.cyclo ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_port_enable.d ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_port_enable.o ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_port_enable.su ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_port_reset.cyclo ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_port_reset.d ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_port_reset.o ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_port_reset.su ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_port_resume.cyclo ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_port_resume.d ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_port_resume.o ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_port_resume.su ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_port_status_get.cyclo ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_port_status_get.d ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_port_status_get.o ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_port_status_get.su
	-$(RM) ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_port_suspend.cyclo ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_port_suspend.d ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_port_suspend.o ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_port_suspend.su ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_power_down_port.cyclo ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_power_down_port.d ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_power_down_port.o ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_power_down_port.su ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_power_on_port.cyclo ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_power_on_port.d ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_power_on_port.o ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_power_on_port.su ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_request_bulk_transfer.cyclo ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_request_bulk_transfer.d ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_request_bulk_transfer.o ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_request_bulk_transfer.su ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_request_control_transfer.cyclo ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_request_control_transfer.d ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_request_control_transfer.o ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_request_control_transfer.su ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_request_periodic_transfer.cyclo ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_request_periodic_transfer.d ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_request_periodic_transfer.o ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_request_periodic_transfer.su ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_request_transfer.cyclo ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_request_transfer.d ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_request_transfer.o ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_request_transfer.su ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_transfer_abort.cyclo ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_transfer_abort.d ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_transfer_abort.o ./Middlewares/ST/usbx/common/usbx_stm32_host_controllers/ux_hcd_stm32_transfer_abort.su

.PHONY: clean-Middlewares-2f-ST-2f-usbx-2f-common-2f-usbx_stm32_host_controllers

