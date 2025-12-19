################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/middlewares/nanoMODBUS/nanomodbus.c 

OBJS += \
./leather_gauge_sensor/middlewares/nanoMODBUS/nanomodbus.o 

C_DEPS += \
./leather_gauge_sensor/middlewares/nanoMODBUS/nanomodbus.d 


# Each subdirectory must supply rules for building sources it contributes
leather_gauge_sensor/middlewares/nanoMODBUS/nanomodbus.o: /home/tecna-smart-lab/GitHub/leather_gauge/Firmware/middlewares/nanoMODBUS/nanomodbus.c leather_gauge_sensor/middlewares/nanoMODBUS/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32G030xx -c -I../Core/Inc -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/leather_gauge_sensor/leather_gauge_sensor/modules/modbus" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/leather_gauge_sensor/leather_gauge_sensor/modules/eeprom" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/leather_gauge_sensor/leather_gauge_sensor/modules/sensor" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/leather_gauge_sensor/leather_gauge_sensor/config" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/middlewares/lwrb/src/include" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/middlewares/lwrb/src/include/lwrb" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/leather_gauge_sensor/leather_gauge_sensor/app/Inc" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/middlewares/DSP_Biquad" -I"/home/tecna-smart-lab/GitHub/leather_gauge/Firmware/middlewares/nanoMODBUS" -I../Drivers/STM32G0xx_HAL_Driver/Inc -I../Drivers/STM32G0xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32G0xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-leather_gauge_sensor-2f-middlewares-2f-nanoMODBUS

clean-leather_gauge_sensor-2f-middlewares-2f-nanoMODBUS:
	-$(RM) ./leather_gauge_sensor/middlewares/nanoMODBUS/nanomodbus.cyclo ./leather_gauge_sensor/middlewares/nanoMODBUS/nanomodbus.d ./leather_gauge_sensor/middlewares/nanoMODBUS/nanomodbus.o ./leather_gauge_sensor/middlewares/nanoMODBUS/nanomodbus.su

.PHONY: clean-leather_gauge_sensor-2f-middlewares-2f-nanoMODBUS

