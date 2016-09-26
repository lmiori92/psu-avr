# psu-avr
A digital-controlled power supply based on the Atmega328P.

## Features

* 2 independent (floating) channels
* 0 - 24V @ 0 - 2A
* Full 10-bit resolution for voltage adjustment
* Full 8-bit resolution for current adjustment
* Digitally-controlled analog circuitry to have fast feedback action
* Software calibration for the best accuracy
* Real-time display of measurements and setpoints

## Software

* Modular C program
* Several channels can be added, hardware permitting (external ADCs/DACs etc)
* Immediate and easy GUI using a 2-line display
* Encoder with smooth control of setpoints via a logarithmic acceleration scale
* Debug and Development Simulator PC application

## Hardware
* 1x atmega328p (master); 1...n atmega328p (slaves)
* 3 HW timers (1 for system clock; 1 for voltage PWM; 1 for current PWM)
* 6 ADC channels (1 voltage measurement; 1 current measurement; 4 auxiliary) 
* Buffering op-amps (final desing yet to be defined)

## Architetture


To ensure a complete channel isolation the system is composed of a master node which handles the encoder, the LCD and the communication with the slaves.
=======
# keypad
The reusable yet simple-to-use keypad library for your bare metal uP project!

# Author's notes
A generic and lightweight keypad library which allows debouncing and management of simple events.
It is so generic that it has no dependencies and can be used with a variety of input sources like physical buttons, soft-buttons, touch-buttons and everything else that can be digitalized in a boolean value.

# Highlights
- Lightweight: no scientific data yet but works pretty well.
- Suitable for physical GPIO buttons, soft-buttons, touch-buttons
- Simple and generic API: timer flag in and boolean in -> event-type out.
- Cross-Platform due to standard C and careful coding

# MISRA
The code should (almost) follow MISRA rules with some exeptions. Please be aware that I did NOT run an analyzer tool yet, hence there is no guarantee the code actually is. The fact is the code has been compiled without warnings nor strange behavior on a 64-bit Linux machine

# Dependencies

None in particular. GNU-GCC compiler tested (AVR and AMD64).
