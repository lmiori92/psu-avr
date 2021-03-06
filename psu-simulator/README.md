# psu-avr simulator
This is the Linux (PC) simulator for the psu-avr microcontroller project. All the logic can be run seamlessly thanks to the carefully written lightweight HAL.

## Dependencies

* lib-ncurses (dev)

## Software and Hardware Simulation
* Display is simulated on the terminal inteface by using the ncurses library
* TIMER and delays are simulated by using the SIGALRM events, ticking at 100us as in the target hardware
* ADC, PWM, ENCODER are currently mock objects

## What is it used for?
Simulation of the remote communication logic for testing and debugging.

Furthermore this also as a very valuable educational porpouse where the concepts of Hardware Abstraction Layer, Portability and more are explored.

## Quick-Start Guide
First step is to create a virtual serial port channel, that is:

```
socat -d -d PTY,link=ttyA PTY,link=ttyB
```

The result is consisting of two end-to-end device links ttyA and ttyB, right in the project folder to avoid the permission nightmare.

The master device being simulated can be started as following

```
./simulator master ttyA
```

Finally, the slave device being simulated can be started as following

```
./simulator slave ttyB
```
