################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/display.c \
../Core/Src/hts221_polling.c \
../Core/Src/hts221_reg.c \
../Core/Src/lighthz.c \
../Core/Src/lora.c \
../Core/Src/lps22hh_reg.c \
../Core/Src/main.c \
../Core/Src/periph_init.c \
../Core/Src/pressure_polling.c \
../Core/Src/sensors.c \
../Core/Src/stm32n6xx_hal_msp.c \
../Core/Src/stm32n6xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32n6xx_ns.c 

OBJS += \
./Core/Src/display.o \
./Core/Src/hts221_polling.o \
./Core/Src/hts221_reg.o \
./Core/Src/lighthz.o \
./Core/Src/lora.o \
./Core/Src/lps22hh_reg.o \
./Core/Src/main.o \
./Core/Src/periph_init.o \
./Core/Src/pressure_polling.o \
./Core/Src/sensors.o \
./Core/Src/stm32n6xx_hal_msp.o \
./Core/Src/stm32n6xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32n6xx_ns.o 

C_DEPS += \
./Core/Src/display.d \
./Core/Src/hts221_polling.d \
./Core/Src/hts221_reg.d \
./Core/Src/lighthz.d \
./Core/Src/lora.d \
./Core/Src/lps22hh_reg.d \
./Core/Src/main.d \
./Core/Src/periph_init.d \
./Core/Src/pressure_polling.d \
./Core/Src/sensors.d \
./Core/Src/stm32n6xx_hal_msp.d \
./Core/Src/stm32n6xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32n6xx_ns.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m55 -std=gnu11 -g3 -DDEBUG -DLL_ATON_PLATFORM=9 -DLL_ATON_OSAL=1 -DUSE_HAL_DRIVER -DSTM32N657xx -c -I../Core/Inc -I../Middlewares/ST/AI/Misc/Inc -I../AI/App -I../ai_generated_network -I../Middlewares/ST/AI/Inc -I../Middlewares/ST/AI/Npu -I../../Secure_nsclib -I../../Drivers/STM32N6xx_HAL_Driver/Inc -I../../Drivers/CMSIS/Device/ST/STM32N6xx/Include -I../../Drivers/STM32N6xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I"C:/Users/Administrateur/STM32CubeIDE/workspace_2.1.1/4/sensor/meteo/AppliNonSecure/Core/Src" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/display.cyclo ./Core/Src/display.d ./Core/Src/display.o ./Core/Src/display.su ./Core/Src/hts221_polling.cyclo ./Core/Src/hts221_polling.d ./Core/Src/hts221_polling.o ./Core/Src/hts221_polling.su ./Core/Src/hts221_reg.cyclo ./Core/Src/hts221_reg.d ./Core/Src/hts221_reg.o ./Core/Src/hts221_reg.su ./Core/Src/lighthz.cyclo ./Core/Src/lighthz.d ./Core/Src/lighthz.o ./Core/Src/lighthz.su ./Core/Src/lora.cyclo ./Core/Src/lora.d ./Core/Src/lora.o ./Core/Src/lora.su ./Core/Src/lps22hh_reg.cyclo ./Core/Src/lps22hh_reg.d ./Core/Src/lps22hh_reg.o ./Core/Src/lps22hh_reg.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/periph_init.cyclo ./Core/Src/periph_init.d ./Core/Src/periph_init.o ./Core/Src/periph_init.su ./Core/Src/pressure_polling.cyclo ./Core/Src/pressure_polling.d ./Core/Src/pressure_polling.o ./Core/Src/pressure_polling.su ./Core/Src/sensors.cyclo ./Core/Src/sensors.d ./Core/Src/sensors.o ./Core/Src/sensors.su ./Core/Src/stm32n6xx_hal_msp.cyclo ./Core/Src/stm32n6xx_hal_msp.d ./Core/Src/stm32n6xx_hal_msp.o ./Core/Src/stm32n6xx_hal_msp.su ./Core/Src/stm32n6xx_it.cyclo ./Core/Src/stm32n6xx_it.d ./Core/Src/stm32n6xx_it.o ./Core/Src/stm32n6xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32n6xx_ns.cyclo ./Core/Src/system_stm32n6xx_ns.d ./Core/Src/system_stm32n6xx_ns.o ./Core/Src/system_stm32n6xx_ns.su

.PHONY: clean-Core-2f-Src

