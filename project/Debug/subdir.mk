################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../display.c \
../display_config.c \
../keypad.c \
../lib.c \
../main.c \
../menu.c \
../pid.c \
../psu.c \
../remote.c 

O_SRCS += \
../display.o \
../keypad.o \
../lib.o \
../menu.o \
../pid.o \
../psu.o \
../remote.o 

OBJS += \
./display.o \
./display_config.o \
./keypad.o \
./lib.o \
./main.o \
./menu.o \
./pid.o \
./psu.o \
./remote.o 

C_DEPS += \
./display.d \
./display_config.d \
./keypad.d \
./lib.d \
./main.d \
./menu.d \
./pid.d \
./psu.d \
./remote.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Compiler'
	avr-gcc -I"/home/lorenzo/git/psu-avr/project" -Wall -g2 -gstabs -Os -fshort-enums -ffunction-sections -fdata-sections -std=gnu99 -funsigned-char -funsigned-bitfields -mmcu=atmega328p -DF_CPU=16000000UL -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


