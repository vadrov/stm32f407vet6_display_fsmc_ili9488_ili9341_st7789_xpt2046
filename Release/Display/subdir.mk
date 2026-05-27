################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Display/display.c \
../Display/fonts.c \
../Display/ili9341.c \
../Display/ili9488.c \
../Display/st7789.c 

OBJS += \
./Display/display.o \
./Display/fonts.o \
./Display/ili9341.o \
./Display/ili9488.o \
./Display/st7789.o 

C_DEPS += \
./Display/display.d \
./Display/fonts.d \
./Display/ili9341.d \
./Display/ili9488.d \
./Display/st7789.d 


# Each subdirectory must supply rules for building sources it contributes
Display/%.o Display/%.su Display/%.cyclo: ../Display/%.c Display/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -DSTM32F407xx -DVDD_VALUE=3300 -c -I../Core/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/Vadim/STM32Cube/Repository/stm32f407vet6_display_fsmc_16b_ili9341/Display" -I"C:/Users/Vadim/STM32Cube/Repository/stm32f407vet6_display_fsmc_16b_ili9341/MicroGL2D" -I"C:/Users/Vadim/STM32Cube/Repository/stm32f407vet6_display_fsmc_16b_ili9341/Textures" -I"C:/Users/Vadim/STM32Cube/Repository/stm32f407vet6_display_fsmc_16b_ili9341/JPEG" -I"C:/Users/Vadim/STM32Cube/Repository/stm32f407vet6_display_fsmc_16b_ili9341/XPT2046" -O2 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Display

clean-Display:
	-$(RM) ./Display/display.cyclo ./Display/display.d ./Display/display.o ./Display/display.su ./Display/fonts.cyclo ./Display/fonts.d ./Display/fonts.o ./Display/fonts.su ./Display/ili9341.cyclo ./Display/ili9341.d ./Display/ili9341.o ./Display/ili9341.su ./Display/ili9488.cyclo ./Display/ili9488.d ./Display/ili9488.o ./Display/ili9488.su ./Display/st7789.cyclo ./Display/st7789.d ./Display/st7789.o ./Display/st7789.su

.PHONY: clean-Display

