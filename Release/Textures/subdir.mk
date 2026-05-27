################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Textures/jpeg_files.c \
../Textures/textures.c 

OBJS += \
./Textures/jpeg_files.o \
./Textures/textures.o 

C_DEPS += \
./Textures/jpeg_files.d \
./Textures/textures.d 


# Each subdirectory must supply rules for building sources it contributes
Textures/%.o Textures/%.su Textures/%.cyclo: ../Textures/%.c Textures/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -DSTM32F407xx -DVDD_VALUE=3300 -c -I../Core/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/Vadim/STM32Cube/Repository/stm32f407vet6_display_fsmc_16b_ili9341/Display" -I"C:/Users/Vadim/STM32Cube/Repository/stm32f407vet6_display_fsmc_16b_ili9341/MicroGL2D" -I"C:/Users/Vadim/STM32Cube/Repository/stm32f407vet6_display_fsmc_16b_ili9341/Textures" -I"C:/Users/Vadim/STM32Cube/Repository/stm32f407vet6_display_fsmc_16b_ili9341/JPEG" -I"C:/Users/Vadim/STM32Cube/Repository/stm32f407vet6_display_fsmc_16b_ili9341/XPT2046" -O2 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Textures

clean-Textures:
	-$(RM) ./Textures/jpeg_files.cyclo ./Textures/jpeg_files.d ./Textures/jpeg_files.o ./Textures/jpeg_files.su ./Textures/textures.cyclo ./Textures/textures.d ./Textures/textures.o ./Textures/textures.su

.PHONY: clean-Textures

