# Ardunio Power Restart Circuit

This is a circuit designed to allow a microcontroller, in this case a PJRC Teensy 4.x board, to restart itself. A 555 timer is used to so when triggered the circuit stays off for a few seconds to allow all the attached sensors and devices to have a clean restart.

<img src="https://github.com/kazzle101/ArdunioPowerRestartCircuit/blob/main/555circuitdiagram.jpg" width="800">

Source code can be found in the src directory, Ardunio IDE users may need to rename the file to power_restart.ino to use it in that editor. The Teensy 4.x Watchdog library can be gotten from: https://github.com/tonton81/WDT_T4 

The INA219 Current Sensor Library is available from the library manager within the IDE (Ardunio and VS Code).

# Links
The library used here is for the Teensy 4.0 and 4.1 boards only, using a different board will need a different libaray
+ The Watchdog Library for Teensy 4.x boards: https://github.com/tonton81/WDT_T4
+ Some library documentation: https://forum.pjrc.com/threads/59257-WDT_T4-Watchdog-Library-for-Teensy-4

# YouTube Video
[![Youtube Video](https://github.com/kazzle101/ArdunioPowerRestartCircuit/blob/main/thumbnail.png)](https://www.youtube.com/watch?v=uxVkhOFc7_M)

# My Notes Used in the Watchdog section of the Video

My experience of watchdog timers is very limited, and this project is the first time that I have used one, so what I am doing here is probably missing some of the extra functionality that they provide.

### What are they for?

Watchdog timers are for checking to see if the microcontroller is alive, that the software hasn’t crashed or gotten stuck in an infinite loop; you have to periodically feed the watchdog so that the timer doesn’t trigger a crisis event. They can also be used to detect if a response is happening too quickly when using window mode.

They won’t detect that a sensor is giving false readings, as in only returning a zero result.

### This watchdog timer library

This library I am using is only for the PJRC Teensy 4 and 4.1 development boards which have the i.MX RT1060 processor - based in an ARM Cortex-M7 design and is running in the Arduino environment.

Each microcontroller manufacturer has their own way of calling watchdog timers and you will need to select the library appropriate for your use.

### Selecting a Watchdog

There are four timers in the Teensy, each behaves differently

WDOG1 and WDOG2 - These internal timers are very similar, they both have two options for outputting to an external trigger - like I am showing here, in this case with WDOG1 I am using pin 20.

The differences with these two are:

WDOG1 - When triggered, after the timeout period the chosen output pin constantly goes low and the board is software reset (if nothing externally is set to do this).

WDOG2 - When triggered, after the timeout completes the timer sends a short pulse to the output pin, the processor is not reset by this watchdog.

EWM - External Watchdog Timer - As far as I can tell the EWM is supposed to be used in conjunction with either WDOG1 or WDOG2 it sits outside the main processor and is supposed to only trigger if an internal watchdog being used fails. I found that when triggered it would not give enough time for the callback function to be completed, but in a way I suppose as it’s a secondary trigger then it will be assuming the microcontroller has gone critical. The EWM can trigger an output pin low which remains low until the board is reset.

WDOG3 (RTWDOG) - Rather than using time, this one watches clock cycles. I couldn’t get this working in the test setup, the microcontroller would reset on startup. This watchdog does not have any output pins for an external trigger.

All of these timers have to be fed within a set time period for them to remain happy, WDOG1 and WDOG2 can be up to 128 seconds and the EWM up to two seconds. 

### Software

I am using WDOG1 in this setup, the configuration consists of setting:

A callback function - this function is called when a watchdog has been triggered. This is to allow you to collect debug information and save it to an SD card or send it down a serial connection.

Timeout and trigger - This caused me a bit of confusion, from what I can tell the timeout is the total time for the restart, and the trigger is the amount of time the callback function is run for, so with a timeout of six seconds, and a trigger of four seconds the watchdog needs to be fed every two seconds to prevent a timeout. On a timeout event then the callback function will run for up to four seconds before a soft reset is triggered.

There is also a window mode, this can be used to test if the watchdog is being fed too fast for when a sensor fails and starts sending pulses too quickly. I have not used this here.

### Implementation 

In the test setup at the end of the loop() you can see the LED being toggled on and off and below that the watchdog.feed() is being completed every 1.5 seconds.

To trigger the watchdog I force an infinite loop by pressing the blue button, and in that code you can see the loop with a counter for the timeout.

Once the timer gets to two, the watchdogCallback() function is called which again goes into an infinite loop, this time the four second trigger is shown on the serial output once that reaches climax the trigger has completed, the output pin goes low and the restart begins.


