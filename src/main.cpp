#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include "Watchdog_t4.h"  // https://github.com/tonton81/WDT_T4

Adafruit_INA219 ina219(0x40);

//WDT_T4<EWM> watchdog;  // External Watchdog Monitor
WDT_T4<WDT1> watchdog;   // WDOG1
//WDT_T4<WDT2> watchdog;     // WDOG2
//WDT_T4<WDT3> watchdog;     // WDOG3

// https://www.allaboutcircuits.com/technical-articles/watchdog-timers-microcontroller-timers/

#define TRIGGER_BTN 11  // yellow button
#define WATCHDOG_BTN 15 // blue button
#define TO_TIMER 14     // output to start 555 circuit
#define WATCHDOG_LED 7  // green led

uint32_t nowMillis = 0;
// feed the watchdog and blink the LED
const uint32_t delayMillis = 1500; 
uint32_t previousMilis;

// reading the Current Senesor every 5 seconds
const uint32_t powerMillis = 5000;
uint32_t previousPwrMilis;

// show a output every second for the watchdog
const uint32_t aSecondsDelay = 1000;
uint32_t aSecondsPrevious;
uint16_t sCounter = 0;

// blinks the LED rapidly
void readyLED(uint8_t led, uint8_t ledWait=120) {

  for (int i = 0; i <= 9; i++) {
    digitalWrite(led,!(digitalRead(led)));
    delay(ledWait);    
  }
  digitalWrite(led, LOW);
}

// used by the wtchdog when an event has been triggered
void watchdogCallback() {
  if (nowMillis < 3000) {
    Serial1.printf("watchdogCallback too early: %lu\n", millis());
    return;
  }

  uint32_t blinkPrevious = nowMillis;

  Serial1.printf("watchdogCallback: %lu\n", millis());
  readyLED(WATCHDOG_LED, 100);
  sCounter = 1;

  while (true) {
      nowMillis = millis();
      if ((nowMillis - aSecondsPrevious) > aSecondsDelay) {
        Serial1.printf("trigger: %u\n", sCounter);
        sCounter ++;
        aSecondsPrevious = nowMillis;
      }

      if ((nowMillis - blinkPrevious) > 50) {
        digitalWrite(WATCHDOG_LED,!(digitalRead(WATCHDOG_LED)));
        blinkPrevious = nowMillis;
      }

      delay(1);
  }

  // this code never runs
  // watchdog.reset();  // reset the MCU
}

void getPowerReadings() {

  static float shuntvoltage = 0;
  static float busvoltage = 0;
  static float current_mA = 0;
  static float loadvoltage = 0;
  // static float power_mW = 0;

  shuntvoltage = ina219.getShuntVoltage_mV();
  busvoltage = ina219.getBusVoltage_V();
  current_mA = ina219.getCurrent_mA();
  // power_mW = ina219.getPower_mW();
  loadvoltage = busvoltage + (shuntvoltage / 1000);
  
  // Serial1.printf("Bus Voltage:   %0.2fV\n", busvoltage);
  // Serial1.printf("Shunt Voltage: %0.2fmV\n", shuntvoltage);
  Serial1.printf("Load Voltage:  %0.2fV\n", loadvoltage);
  Serial1.printf("Current:       %0.2fmA\n", current_mA);
  // Serial1.printf("Power:         %0.2fmW\n",power_mW);
}

// crude button deboune
void waitUntilFingerIsAwayFromTheButton(uint8_t button) {
    boolean state = HIGH;
    do {
      delay(10);
      watchdog.feed();
      state = digitalRead(button);
    } while (state == HIGH);
}

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);

  pinMode(TO_TIMER, OUTPUT);
  pinMode(TRIGGER_BTN, INPUT_PULLDOWN);  // yellow button
  pinMode(WATCHDOG_BTN, INPUT_PULLDOWN); // blue button
  pinMode(WATCHDOG_LED, OUTPUT);
  digitalWrite(TO_TIMER, LOW);
  digitalWrite(WATCHDOG_LED, LOW);

  // https://www.ablic.com/en/semicon/products/automotive/automotive-watchdog-timer/intro/

  WDT_timings_t configWatchdog;
  configWatchdog.callback = watchdogCallback;
  /* 
  WDOG1 or WDOG2
      (seems to need .trigger AND .timeout OR .trigger on its own)
      timeout is the total time for the restart
      trigger is the time the callback function is run for.
      So with a timeout of 6 and a trigger of 4 then the timeout will run 
      for two seconds and the trigger for five in the callback to give 
      a total time before reset of seven seconds.
      in this case the watchdog.feed() will need to execute every < 2 seconds
  */
  configWatchdog.timeout = 6;    /* in seconds, 0->128 */
  configWatchdog.trigger = 4;    /* in seconds, 0->128 */
  /*
      for when operations need to be performed within a specific timeframe,
      must be smaller than timeout, window mode is disabled when omitted
  */
  //configWatchdog.window = 100; /* in seconds, 32ms to 522.232s */
  ////WDOG1 can be on pins: 19 or 20
  configWatchdog.pin = 20;
  ////WDOG2 can be on pins: 13 or 24
  //configWatchdog.pin = 13;

  //// WDOG3 (RTWDOG) - used here this seems to just cause a perpetual reset
  //configWatchdog.window = 3000;    /* in seconds, 32ms to 522.232s, must be smaller than timeout */
  //configWatchdog.timeout = 10000;  /* in seconds, 32ms to 522.232s */
  //// WDOG3 no external pins used  
  
  ///// EWM
  ////(.trigger is not used)
  // configWatchdog.timeout = 2000;  // in milliseconds 0-> 2000
  ////EWM can be on pins: 21 or 25
  // configWatchdog.pin = 21;      // pins 21 or 25 goes low when fault detected

  if (!ina219.begin()) {
    Serial1.println("Failed to find INA219 chip");
    while (1) { 
      delay(10); 
    }
  }
  ina219.setCalibration_32V_1A();
  // ina219.setCalibration_16V_400mA();

  watchdog.begin(configWatchdog);

  Serial1.println("ready");
}

void loop() {

  nowMillis = millis();

  // send a restart to the 555 timer with the yellow buton
  if (digitalRead(TRIGGER_BTN) == HIGH) {
    waitUntilFingerIsAwayFromTheButton(TRIGGER_BTN);
    Serial1.println("sending restart");
    readyLED(WATCHDOG_LED, 100);
    digitalWrite(TO_TIMER, HIGH);
    // the system shoud have now restarted, so nothing afer this gets executed
    delay(200);
    digitalWrite(TO_TIMER, LOW);
    delay(2000);
  }

  // trigger the watchdog with the blue button
  if (digitalRead(WATCHDOG_BTN) == HIGH) {
    waitUntilFingerIsAwayFromTheButton(WATCHDOG_BTN);
    Serial1.println("forcing a timeout");
    Serial1.println("restarting");
    delay(500);   
    sCounter = 1;
     // create an infinate loop to stop the watchdog being fed.
    while (true)
    {
      nowMillis = millis();
      if ((nowMillis - aSecondsPrevious) > aSecondsDelay) {
        Serial1.printf("timeout: %u\n", sCounter);
        sCounter ++;
        aSecondsPrevious = nowMillis;
      }
      delay(1); 
    }
  }

  if ((nowMillis - previousPwrMilis) > powerMillis) {
    getPowerReadings();
    previousPwrMilis = nowMillis;
  }

  if ((nowMillis - previousMilis) > delayMillis) {
    Serial1.printf("feed the watchdog: %lu\n", nowMillis);
    digitalWrite(WATCHDOG_LED, !(digitalRead(WATCHDOG_LED)));
    watchdog.feed();
    previousMilis = nowMillis;
  } 

}