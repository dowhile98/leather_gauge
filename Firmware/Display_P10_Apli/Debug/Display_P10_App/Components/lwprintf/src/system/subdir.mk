################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Display_P10_App/Components/lwprintf/src/system/lwprintf_sys_cmsis_os.c 

C_DEPS += \
./Display_P10_App/Components/lwprintf/src/system/lwprintf_sys_cmsis_os.d 

OBJS += \
./Display_P10_App/Components/lwprintf/src/system/lwprintf_sys_cmsis_os.o 


# Each subdirectory must supply rules for building sources it contributes
Display_P10_App/Components/lwprintf/src/system/%.o Display_P10_App/Components/lwprintf/src/system/%.su Display_P10_App/Components/lwprintf/src/system/%.cyclo: ../Display_P10_App/Components/lwprintf/src/system/%.c Display_P10_App/Components/lwprintf/src/system/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DLWRB_DISABLE_ATOMIC -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I"C:/Users/eplim/Downloads/firmware/Display_P10_Apli/Display_P10_App/Components/at24cxx" -I"C:/Users/eplim/Downloads/firmware/Display_P10_Apli/Display_P10_App/Components/at24cxx/interface" -I"C:/Users/eplim/Downloads/firmware/Display_P10_Apli/Display_P10_App/App/Inc" -I"C:/Users/eplim/Downloads/firmware/Display_P10_Apli/Display_P10_App/Common" -I"C:/Users/eplim/Downloads/firmware/Display_P10_Apli/Display_P10_App/Components/lwprintf/src/include" -I"C:/Users/eplim/Downloads/firmware/Display_P10_Apli/Display_P10_App/Components/lwprintf/src/include/lwprintf" -I"C:/Users/eplim/Downloads/firmware/Display_P10_Apli/Display_P10_App/Components/lwprintf/src/include/system" -I"C:/Users/eplim/Downloads/firmware/Display_P10_Apli/Display_P10_App/Components/lwrb/src/include" -I"C:/Users/eplim/Downloads/firmware/Display_P10_Apli/Display_P10_App/Components/lwrb/src/include/lwrb" -I"C:/Users/eplim/Downloads/firmware/Display_P10_Apli/Display_P10_App/Components/nanoMODBUS" -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3 -O1 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Display_P10_App-2f-Components-2f-lwprintf-2f-src-2f-system

clean-Display_P10_App-2f-Components-2f-lwprintf-2f-src-2f-system:
	-$(RM) ./Display_P10_App/Components/lwprintf/src/system/lwprintf_sys_cmsis_os.cyclo ./Display_P10_App/Components/lwprintf/src/system/lwprintf_sys_cmsis_os.d ./Display_P10_App/Components/lwprintf/src/system/lwprintf_sys_cmsis_os.o ./Display_P10_App/Components/lwprintf/src/system/lwprintf_sys_cmsis_os.su

.PHONY: clean-Display_P10_App-2f-Components-2f-lwprintf-2f-src-2f-system

