################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Display_P10_App/Components/dmd/Bitmap.cpp \
../Display_P10_App/Components/dmd/dmd.cpp 

OBJS += \
./Display_P10_App/Components/dmd/Bitmap.o \
./Display_P10_App/Components/dmd/dmd.o 

CPP_DEPS += \
./Display_P10_App/Components/dmd/Bitmap.d \
./Display_P10_App/Components/dmd/dmd.d 


# Each subdirectory must supply rules for building sources it contributes
Display_P10_App/Components/dmd/%.o Display_P10_App/Components/dmd/%.su Display_P10_App/Components/dmd/%.cyclo: ../Display_P10_App/Components/dmd/%.cpp Display_P10_App/Components/dmd/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m3 -std=gnu++14 -g3 -DDEBUG -DLWRB_DISABLE_ATOMIC -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I"C:/Users/eplim/Downloads/firmware/Display_P10_Apli/Display_P10_App/Components/at24cxx" -I"C:/Users/eplim/Downloads/firmware/Display_P10_Apli/Display_P10_App/Components/at24cxx/interface" -I"C:/Users/eplim/Downloads/firmware/Display_P10_Apli/Display_P10_App/Components/dmd" -I"C:/Users/eplim/Downloads/firmware/Display_P10_Apli/Display_P10_App/Components/lwprintf/src/include" -I"C:/Users/eplim/Downloads/firmware/Display_P10_Apli/Display_P10_App/Components/lwrb/src/include" -I"C:/Users/eplim/Downloads/firmware/Display_P10_Apli/Display_P10_App/Components/nanoMODBUS" -I"C:/Users/eplim/Downloads/firmware/Display_P10_Apli/Display_P10_App/Common" -I"C:/Users/eplim/Downloads/firmware/Display_P10_Apli/Display_P10_App/App/Inc" -I"C:/Users/eplim/Downloads/firmware/Display_P10_Apli/Display_P10_App/Components/lwrb/src/include/lwrb" -I"C:/Users/eplim/Downloads/firmware/Display_P10_Apli/Display_P10_App/Components/lwprintf/src/include/lwprintf" -I"C:/Users/eplim/Downloads/firmware/Display_P10_Apli/Display_P10_App/Components/lwprintf/src/include/system" -I"C:/Users/eplim/Downloads/firmware/Display_P10_Apli/Display_P10_App/Components/dmd/fonts" -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3 -O1 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Display_P10_App-2f-Components-2f-dmd

clean-Display_P10_App-2f-Components-2f-dmd:
	-$(RM) ./Display_P10_App/Components/dmd/Bitmap.cyclo ./Display_P10_App/Components/dmd/Bitmap.d ./Display_P10_App/Components/dmd/Bitmap.o ./Display_P10_App/Components/dmd/Bitmap.su ./Display_P10_App/Components/dmd/dmd.cyclo ./Display_P10_App/Components/dmd/dmd.d ./Display_P10_App/Components/dmd/dmd.o ./Display_P10_App/Components/dmd/dmd.su

.PHONY: clean-Display_P10_App-2f-Components-2f-dmd

