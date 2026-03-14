################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Display_P10_App/Components/lwrb/src/lwrb/lwrb.c \
../Display_P10_App/Components/lwrb/src/lwrb/lwrb_ex.c 

C_DEPS += \
./Display_P10_App/Components/lwrb/src/lwrb/lwrb.d \
./Display_P10_App/Components/lwrb/src/lwrb/lwrb_ex.d 

OBJS += \
./Display_P10_App/Components/lwrb/src/lwrb/lwrb.o \
./Display_P10_App/Components/lwrb/src/lwrb/lwrb_ex.o 


# Each subdirectory must supply rules for building sources it contributes
Display_P10_App/Components/lwrb/src/lwrb/%.o Display_P10_App/Components/lwrb/src/lwrb/%.su Display_P10_App/Components/lwrb/src/lwrb/%.cyclo: ../Display_P10_App/Components/lwrb/src/lwrb/%.c Display_P10_App/Components/lwrb/src/lwrb/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DLWRB_DISABLE_ATOMIC -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I"C:/Users/eplim/Downloads/firmware/Display_P10_Apli/Display_P10_App/Components/at24cxx" -I"C:/Users/eplim/Downloads/firmware/Display_P10_Apli/Display_P10_App/Components/at24cxx/interface" -I"C:/Users/eplim/Downloads/firmware/Display_P10_Apli/Display_P10_App/App/Inc" -I"C:/Users/eplim/Downloads/firmware/Display_P10_Apli/Display_P10_App/Common" -I"C:/Users/eplim/Downloads/firmware/Display_P10_Apli/Display_P10_App/Components/lwprintf/src/include" -I"C:/Users/eplim/Downloads/firmware/Display_P10_Apli/Display_P10_App/Components/lwprintf/src/include/lwprintf" -I"C:/Users/eplim/Downloads/firmware/Display_P10_Apli/Display_P10_App/Components/lwprintf/src/include/system" -I"C:/Users/eplim/Downloads/firmware/Display_P10_Apli/Display_P10_App/Components/lwrb/src/include" -I"C:/Users/eplim/Downloads/firmware/Display_P10_Apli/Display_P10_App/Components/lwrb/src/include/lwrb" -I"C:/Users/eplim/Downloads/firmware/Display_P10_Apli/Display_P10_App/Components/nanoMODBUS" -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3 -O1 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Display_P10_App-2f-Components-2f-lwrb-2f-src-2f-lwrb

clean-Display_P10_App-2f-Components-2f-lwrb-2f-src-2f-lwrb:
	-$(RM) ./Display_P10_App/Components/lwrb/src/lwrb/lwrb.cyclo ./Display_P10_App/Components/lwrb/src/lwrb/lwrb.d ./Display_P10_App/Components/lwrb/src/lwrb/lwrb.o ./Display_P10_App/Components/lwrb/src/lwrb/lwrb.su ./Display_P10_App/Components/lwrb/src/lwrb/lwrb_ex.cyclo ./Display_P10_App/Components/lwrb/src/lwrb/lwrb_ex.d ./Display_P10_App/Components/lwrb/src/lwrb/lwrb_ex.o ./Display_P10_App/Components/lwrb/src/lwrb/lwrb_ex.su

.PHONY: clean-Display_P10_App-2f-Components-2f-lwrb-2f-src-2f-lwrb

