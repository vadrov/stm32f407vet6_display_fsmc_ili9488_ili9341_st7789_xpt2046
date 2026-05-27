################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../MicroGL2D/microgl2d.c 

OBJS += \
./MicroGL2D/microgl2d.o 

C_DEPS += \
./MicroGL2D/microgl2d.d 


# Each subdirectory must supply rules for building sources it contributes
MicroGL2D/%.o MicroGL2D/%.su MicroGL2D/%.cyclo: ../MicroGL2D/%.c MicroGL2D/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -DSTM32F407xx -DVDD_VALUE=3300 -c -I../Core/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/Vadim/STM32Cube/Repository/stm32f407vet6_display_fsmc_16b_ili9341/Display" -I"C:/Users/Vadim/STM32Cube/Repository/stm32f407vet6_display_fsmc_16b_ili9341/MicroGL2D" -I"C:/Users/Vadim/STM32Cube/Repository/stm32f407vet6_display_fsmc_16b_ili9341/Textures" -I"C:/Users/Vadim/STM32Cube/Repository/stm32f407vet6_display_fsmc_16b_ili9341/JPEG" -I"C:/Users/Vadim/STM32Cube/Repository/stm32f407vet6_display_fsmc_16b_ili9341/XPT2046" -O2 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-MicroGL2D

clean-MicroGL2D:
	-$(RM) ./MicroGL2D/microgl2d.cyclo ./MicroGL2D/microgl2d.d ./MicroGL2D/microgl2d.o ./MicroGL2D/microgl2d.su

.PHONY: clean-MicroGL2D

