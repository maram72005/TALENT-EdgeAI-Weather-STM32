################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../AI/App/app_x-cube-ai.c \
../AI/App/network_weights.c \
../AI/App/npu_cache.c \
../AI/App/npu_init.c \
../AI/App/user_init.c 

OBJS += \
./AI/App/app_x-cube-ai.o \
./AI/App/network_weights.o \
./AI/App/npu_cache.o \
./AI/App/npu_init.o \
./AI/App/user_init.o 

C_DEPS += \
./AI/App/app_x-cube-ai.d \
./AI/App/network_weights.d \
./AI/App/npu_cache.d \
./AI/App/npu_init.d \
./AI/App/user_init.d 


# Each subdirectory must supply rules for building sources it contributes
AI/App/%.o AI/App/%.su AI/App/%.cyclo: ../AI/App/%.c AI/App/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -g3 -DDEBUG -DLL_ATON_PLATFORM=9 -DLL_ATON_OSAL=1 -DUSE_HAL_DRIVER -DSTM32N657xx -c -I../Core/Inc -I../Middlewares/ST/AI/Misc/Inc -I../AI/App -I../ai_generated_network -I../Middlewares/ST/AI/Inc -I../Middlewares/ST/AI/Npu -I../../Secure_nsclib -I../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I"C:/Users/Administrateur/STM32CubeIDE/workspace_2.1.1/4/sensor/meteo/AppliNonSecure/Core/Src" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-AI-2f-App

clean-AI-2f-App:
	-$(RM) ./AI/App/app_x-cube-ai.cyclo ./AI/App/app_x-cube-ai.d ./AI/App/app_x-cube-ai.o ./AI/App/app_x-cube-ai.su ./AI/App/network_weights.cyclo ./AI/App/network_weights.d ./AI/App/network_weights.o ./AI/App/network_weights.su ./AI/App/npu_cache.cyclo ./AI/App/npu_cache.d ./AI/App/npu_cache.o ./AI/App/npu_cache.su ./AI/App/npu_init.cyclo ./AI/App/npu_init.d ./AI/App/npu_init.o ./AI/App/npu_init.su ./AI/App/user_init.cyclo ./AI/App/user_init.d ./AI/App/user_init.o ./AI/App/user_init.su

.PHONY: clean-AI-2f-App

