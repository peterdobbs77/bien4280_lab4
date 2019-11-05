# ultrasonic_ucos
Ultrasonic sensor controlled by MicroC/OS-II on an Arduino Uno. Data is displayed through a python GUI.

## features
* Interrupt-driven serial communication (both transmit and receive)
* Distance sensing with HC-SR04 Ultrasonic Sensor.
  * Timer1 used as a counter for deteriming the duration of echo pulse
  * Timer2 used as a timer for the trigger pulse

## reference	
* http://arduinobasics.blogspot.com/2012/11/hc-sr04-ultrasonic-sensor.html
