# ultrasonic_ucos
Ultrasonic sensor controlled by uC/OS-II on an Arduino Uno. Data is displayed through a python GUI.

Lab4 Project
	C implementation based on Arduino project from http://arduinobasics.blogspot.com/2012/11/hc-sr04-ultrasonic-sensor.html
	Features:
		1. Interrupt-driven serial communication (both transmit and receive)
		2. Distance sensing with HC-SR04 Ultrasonic Sensor.
			* Timer1 used as a counter for deteriming the duration of echo pulse
			* Timer2 used as a timer for the trigger pulse

	
