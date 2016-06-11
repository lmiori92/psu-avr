################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
LSS += \
psu_2ch.lss \

FLASH_IMAGE += \
psu_2ch.hex \

EEPROM_IMAGE += \
psu_2ch.eep \

SIZEDUMMY += \
sizedummy \


# All Target
all: psu_2ch.elf secondary-outputs

# Tool invocations
psu_2ch.elf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: AVR C Linker'
	avr-gcc -Wl,-Map,psu_2ch.map -mmcu=atmega328p -o "psu_2ch.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

psu_2ch.lss: psu_2ch.elf
	@echo 'Invoking: AVR Create Extended Listing'
	-avr-objdump -h -S psu_2ch.elf  >"psu_2ch.lss"
	@echo 'Finished building: $@'
	@echo ' '

psu_2ch.hex: psu_2ch.elf
	@echo 'Create Flash image (ihex format)'
	-avr-objcopy -R .eeprom -R .fuse -R .lock -R .signature -O ihex psu_2ch.elf  "psu_2ch.hex"
	@echo 'Finished building: $@'
	@echo ' '

psu_2ch.eep: psu_2ch.elf
	@echo 'Create eeprom image (ihex format)'
	-avr-objcopy -j .eeprom --no-change-warnings --change-section-lma .eeprom=0 -O ihex psu_2ch.elf  "psu_2ch.eep"
	@echo 'Finished building: $@'
	@echo ' '

sizedummy: psu_2ch.elf
	@echo 'Invoking: Print Size'
	-avr-size --format=avr --mcu=atmega328p psu_2ch.elf
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(FLASH_IMAGE)$(ELFS)$(OBJS)$(ASM_DEPS)$(EEPROM_IMAGE)$(S_DEPS)$(SIZEDUMMY)$(S_UPPER_DEPS)$(LSS)$(C_DEPS) psu_2ch.elf
	-@echo ' '

secondary-outputs: $(LSS) $(FLASH_IMAGE) $(EEPROM_IMAGE) $(SIZEDUMMY)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
