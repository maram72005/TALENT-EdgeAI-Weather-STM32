################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../ai_generated_network/network.c \
../ai_generated_network/network_atonbuf.xSPI2.c \
../ai_generated_network/stai_network.c 

OBJS += \
./ai_generated_network/network.o \
./ai_generated_network/network_atonbuf.xSPI2.o \
./ai_generated_network/stai_network.o 

C_DEPS += \
./ai_generated_network/network.d \
./ai_generated_network/network_atonbuf.xSPI2.d \
./ai_generated_network/stai_network.d 


# Each subdirectory must supply rules for building sources it contributes
ai_generated_network/%.o ai_generated_network/%.su ai_generated_network/%.cyclo: ../ai_generated_network/%.c ai_generated_network/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -g3 -DDEBUG -DLL_ATON_PLATFORM=9 -DLL_ATON_OSAL=1 -DUSE_HAL_DRIVER -DSTM32N657xx -c -I../Core/Inc -I../Middlewares/ST/AI/Misc/Inc -I../AI/App -I../ai_generated_network -I../Middlewares/ST/AI/Inc -I../Middlewares/ST/AI/Npu -I../../Secure_nsclib -I../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I"C:/Users/Administrateur/STM32CubeIDE/workspace_2.1.1/4/sensor/meteo/AppliNonSecure/Core/Src" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-ai_generated_network

clean-ai_generated_network:
	-$(RM) ./ai_generated_network/network.cyclo ./ai_generated_network/network.d ./ai_generated_network/network.o ./ai_generated_network/network.su ./ai_generated_network/network_atonbuf.xSPI2.cyclo ./ai_generated_network/network_atonbuf.xSPI2.d ./ai_generated_network/network_atonbuf.xSPI2.o ./ai_generated_network/network_atonbuf.xSPI2.su ./ai_generated_network/stai_network.cyclo ./ai_generated_network/stai_network.d ./ai_generated_network/stai_network.o ./ai_generated_network/stai_network.su

.PHONY: clean-ai_generated_network

