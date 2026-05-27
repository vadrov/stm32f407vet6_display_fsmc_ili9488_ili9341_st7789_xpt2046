################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../JPEG/jpeg_chan.c \
../JPEG/tjpgd.c 

OBJS += \
./JPEG/jpeg_chan.o \
./JPEG/tjpgd.o 

C_DEPS += \
./JPEG/jpeg_chan.d \
./JPEG/tjpgd.d 


# Each subdirectory must supply rules for building sources it contributes
JPEG/%.o JPEG/%.su JPEG/%.cyclo: ../JPEG/%.c JPEG/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -DSTM32F407xx -DVDD_VALUE=3300 -c -I../Core/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/Vadim/STM32Cube/Repository/stm32f407vet6_display_fsmc_16b_ili9341/Display" -I"C:/Users/Vadim/STM32Cube/Repository/stm32f407vet6_display_fsmc_16b_ili9341/MicroGL2D" -I"C:/Users/Vadim/STM32Cube/Repository/stm32f407vet6_display_fsmc_16b_ili9341/Textures" -I"C:/Users/Vadim/STM32Cube/Repository/stm32f407vet6_display_fsmc_16b_ili9341/JPEG" -I"C:/Users/Vadim/STM32Cube/Repository/stm32f407vet6_display_fsmc_16b_ili9341/XPT2046" -O2 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-JPEG

clean-JPEG:
	-$(RM) ./JPEG/jpeg_chan.cyclo ./JPEG/jpeg_chan.d ./JPEG/jpeg_chan.o ./JPEG/jpeg_chan.su ./JPEG/tjpgd.cyclo ./JPEG/tjpgd.d ./JPEG/tjpgd.o ./JPEG/tjpgd.su

.PHONY: clean-JPEG

