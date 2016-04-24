# psu-avr
A digital-controlled power supply based on the Atmega328P.

## Features

* 2 independent (floating) channels
* 0 - 24V @ 0 - 2A
* Full 10-bit resolution for voltage adjustment
* Full 8-bit resolution for current adjustment
* Digitally-controlled analog circuitry to have fast feedback action
* Software calibration for the best accuracy

## Software

* Modular C program
* Several channels can be added, hardware permitting (external ADCs/DACs etc)
* Immediate and easy GUI using a 2-line display (final design yet to be defined)

## Hardware
* atmega328
* 3 HW timers (1 for system clock; 2 for voltage PWM; 2 for current PWM)
* 6 ADC channels (2 voltage measurement; 2 current measurement; 2 auxiliary) 
* Buffering op-amps (final desing yet to be defined)
