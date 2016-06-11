################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../adc.c \
../display.c \
../display_config.c \
../display_hal_hd44780.c \
../display_hal_uart.c \
../encoder.c \
../keypad.c \
../lib.c \
../main.c \
../psu.c \
../pwm.c \
../remote.c \
../system.c \
../time.c \
../uart.c 

O_SRCS += \
../display.o \
../keypad.o \
../lib.o \
../main.o \
../remote.o 

OBJS += \
./adc.o \
./display.o \
./display_config.o \
./display_hal_hd44780.o \
./display_hal_uart.o \
./encoder.o \
./keypad.o \
./lib.o \
./main.o \
./psu.o \
./pwm.o \
./remote.o \
./system.o \
./time.o \
./uart.o 

C_DEPS += \
./adc.d \
./display.d \
./display_config.d \
./display_hal_hd44780.d \
./display_hal_uart.d \
./encoder.d \
./keypad.d \
./lib.d \
./main.d \
./psu.d \
./pwm.d \
./remote.d \
./system.d \
./time.d \
./uart.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Compiler'
	avr-gcc -Wall -g2 -gstabs -Os -fpack-struct -fshort-enums -ffunction-sections -fdata-sections -std=gnu99 -funsigned-char -funsigned-bitfields -mmcu=atmega328p -DF_CPU=16000000UL -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


