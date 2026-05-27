################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../XPT2046/calibrate_touch.c \
../XPT2046/demo.c \
../XPT2046/xpt2046.c 

OBJS += \
./XPT2046/calibrate_touch.o \
./XPT2046/demo.o \
./XPT2046/xpt2046.o 

C_DEPS += \
./XPT2046/calibrate_touch.d \
./XPT2046/demo.d \
./XPT2046/xpt2046.d 


# Each subdirectory must supply rules for building sources it contributes
XPT2046/%.o XPT2046/%.su XPT2046/%.cyclo: ../XPT2046/%.c XPT2046/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -DSTM32F407xx -DVDD_VALUE=3300 -c -I../Core/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/Vadim/STM32Cube/Repository/stm32f407vet6_display_fsmc_16b_ili9341/Display" -I"C:/Users/Vadim/STM32Cube/Repository/stm32f407vet6_display_fsmc_16b_ili9341/MicroGL2D" -I"C:/Users/Vadim/STM32Cube/Repository/stm32f407vet6_display_fsmc_16b_ili9341/Textures" -I"C:/Users/Vadim/STM32Cube/Repository/stm32f407vet6_display_fsmc_16b_ili9341/JPEG" -I"C:/Users/Vadim/STM32Cube/Repository/stm32f407vet6_display_fsmc_16b_ili9341/XPT2046" -O2 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-XPT2046

clean-XPT2046:
	-$(RM) ./XPT2046/calibrate_touch.cyclo ./XPT2046/calibrate_touch.d ./XPT2046/calibrate_touch.o ./XPT2046/calibrate_touch.su ./XPT2046/demo.cyclo ./XPT2046/demo.d ./XPT2046/demo.o ./XPT2046/demo.su ./XPT2046/xpt2046.cyclo ./XPT2046/xpt2046.d ./XPT2046/xpt2046.o ./XPT2046/xpt2046.su

.PHONY: clean-XPT2046

