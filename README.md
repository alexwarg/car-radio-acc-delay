# Car Head-Unit ACC timer

Small circuit and software based on an AVR ATTiny85 that provides a separate power-button logic for a car head unit.
The circuit is placed in line to the vehicle ignition (ACC)
signal and allows to power off the head unit even with ACC on, as well as, powering on the head-unit with ACC off with an
auto-power-off timer.

The project is available under AIT license (see LICENSE.txt).

## Hardware

The hardware is absed on a minimal number of components around an ATTiny85 MCU, and designed for very low power consumption in power-off mode. (around 40uA in my case).

![auto-acc-timer](hardware/auto-acc-timer/auto-acc-timer.png)

## Software

The software is using the avr-libc and does not require any extra libraries.

The timeout for auto-power-down is configurable and preconfigured for 30 minutes.

The software uses agressive power management and puts the AVR MCU into low-power sleep modes in particular when ACC input
is off.
