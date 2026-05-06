
This is firmware for an AVR microcontroller (likely ATtiny85 or similar) that implements an ACC (accessory power) delay timer for 
a car radio. Here's what the code does:

## Core Purpose

It sits between a car's ACC ignition signal and the car radio, giving you manual control over when the radio turns off — useful
for listening to the radio after parking without draining the battery.

## How It Works

### Inputs:

 - ACC_IN (pin 3): The car's ignition/ACC signal
 - PWR_BTN (pin 1): A manual power button

### Output:

 - ACC_OUT (pin 4): The signal sent to the radio

### Operating Modes (Timed_pwr_on)

1. ACC on (P_acc): When the ignition is on, the radio output mirrors it. The button can toggle the radio off/on while ignition
remains on.
2. Timer mode (P_timer): When the ignition turns off, you can press the button to keep the radio on for a 30-minute countdown. A
long press resets the timer back to 30 minutes. When the timer expires, it cuts power automatically
3. Off (P_off): Radio output is off.

## Supporting Components

 - timer.h: A 40-bit hardware timer built on AVR Timer1 with an 8192 prescaler, providing ~1kHz resolution at 8MHz. Used for
   debouncing and timeout tracking.
 - debounce.h / btn.h: Debounce logic for the ACC input and button, distinguishing short press, long press, and double-click.
 - task_list.h: A cooperative task loop that puts the MCU to sleep (idle mode) between events to save power, waking on pin-change  
   interrupts.
 - display.h / i2c.h: Optional SSD1306 OLED display support (compiled with USE_I2C=1) to show remaining time in MM:SS format using 
   a 7-segment font, plus a settings menu for configuring the timers — stored in EEPROM.
 - display.cc: Font rendering logic for blitting characters to the OLED over I2C.

## Summary

Short press while ACC is off → turn radio on with 30-min timer. Long press while timer is ticking → reset timer to 30 min. Timer
expires → radio off. With the display enabled, you also get a settings UI to configure the timeout duration via EEPROM.
