################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../adc.c \
../encoder.c \
../main.c \
../pwm.c \
../system.c \
../time.c \
../uart.c 

OBJS += \
./adc.o \
./encoder.o \
./main.o \
./pwm.o \
./system.o \
./time.o \
./uart.o 

C_DEPS += \
./adc.d \
./encoder.d \
./main.d \
./pwm.d \
./system.d \
./time.d \
./uart.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Compiler'
	avr-gcc -Wall -g2 -gstabs -O2 -fpack-struct -fshort-enums -ffunction-sections -fdata-sections -std=gnu99 -funsigned-char -funsigned-bitfields -mmcu=atmega328p -DF_CPU=16000000UL -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


