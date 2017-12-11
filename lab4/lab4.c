/*
 * lab4.c
 *
 * Created: 12/8/2017 7:11:11 PM
 *  Author: Peter
 */


#include <avr/io.h>
#include <stdio.h>
//#include <avr/interrupt.h>
#include <string.h>
#include "includes.h"
#include <util/delay.h>

#define TASK_STK_SIZE					128		/* Size of each task's stacks (# of WORDs)            */
#define START_TASK_STK_SIZE				64
#define TX_BUFFER_SIZE					36	    /* Size of buffers used to store character strings    */
#define RX_BUFFER_SIZE					24

#define LED_TIMEOUT						1

#define NO_MSG_SENT						0
#define NO_SYSTEM_ERROR					1
#define MEDIUM_PRIORITY_ERROR			2
#define HIGH_PRIORITY_ERROR				3

#define NO_ERR_MSG						"0\n\r"
#define OUT_OF_RANGE					"-1\n\r"

#define TOGGLE_LED						101		//"e"
#define TOGGLE_TX						100		//"d"
#define UPDATE_RATE						114		//"r"

#define	SOUND_CONVERSION_FACTOR			117 //58.2*2	// in cm : conversion factor based on speed of sound

#define ECHO_OFF_STATE					0
#define ECHO_ON_STATE					1

#define COMMAND_DEFAULT					0
#define COMMAND_OFF						1
#define COMMAND_RATECHANGE				2

/*
 *********************************************************************************************************
 *                                               VARIABLES
 *********************************************************************************************************
 */
OS_STK		TaskStartStk[START_TASK_STK_SIZE];
OS_STK		TaskTimerStk[TASK_STK_SIZE];
OS_STK      TaskLedStk[TASK_STK_SIZE];
OS_STK      TaskSensorStk[TASK_STK_SIZE];
OS_STK		TaskSerialTransmitStk[TASK_STK_SIZE];
OS_STK		SerialReceiveTaskStk[TASK_STK_SIZE];

OS_EVENT	*SerialTxSem;
OS_EVENT    *SerialTxMBox;
OS_EVENT	*LedMBox;
OS_EVENT	*SerialRxMbox;
OS_EVENT	*TriggerMbox;
OS_EVENT	*TriggerSem;

volatile float timeoutPeriod;
unsigned char echoState;
unsigned char receiveCommandState;
volatile unsigned int cnt;

/*
 *********************************************************************************************************
 *                                           FUNCTION PROTOTYPES
 *********************************************************************************************************
 */

extern void InitPeripherals(void);
//extern int cliBuildCommand(char nextChar);
//extern void cliProcessCommand(void);

void	TaskStart(void *data);              /* Function prototypes of Startup task */
void	TimerTask(void *data);				//
void	LedTask(void *data);				//
void	SerialTransmitTask(void *data);		//
void	SerialReceiveTask(void *data);		//
void	SensorTask(void *data);				//

/*
 *********************************************************************************************************
 *                                                MAIN
 *********************************************************************************************************
 */
int main (void) {
	InitPeripherals();

    OSInit();                                              /* Initialize uC/OS-II                      */

/* Create OS_EVENT resources here  */;
	//LedMBox = OSMboxCreate((void *)0);
	SerialTxSem = OSSemCreate(1);
	SerialTxMBox = OSMboxCreate((void *)0);
	SerialRxMbox = OSMboxCreate((void *)0);
	TriggerSem = OSSemCreate(1);
	TriggerMbox = OSMboxCreate((void *)0);
/* END Create OS_EVENT resources   */

    OSTaskCreate(TaskStart, (void *)0, &TaskStartStk[START_TASK_STK_SIZE - 1], 0);

    OSStart();                                             /* Start multitasking                       */

	while (1);
}

/*
 *********************************************************************************************************
 *                                              STARTUP TASK
 *********************************************************************************************************
 */
void  TaskStart (void *pdata) {
    pdata = pdata;                                         /* Prevent compiler warning                 */

	OSStatInit(); //uncomment if set OS_TASK_STAT_EN = 1   /* Initialize uC/OS-II's statistics         */

	//OSTaskCreate(TimerTask, (void *)0, &TaskTimerStk[TASK_STK_SIZE - 1], 6);
	OSTaskCreate(SensorTask, (void *)0, &TaskSensorStk[TASK_STK_SIZE - 1], 8);
	//OSTaskCreate(LedTask, (void *)0, &TaskLedStk[TASK_STK_SIZE - 1], 10);

	OSTaskCreate(SerialTransmitTask, (void *) 0, &TaskSerialTransmitStk[TASK_STK_SIZE-1], 12);
	//OSTaskCreate(SerialReceiveTask, (void *) 0, &SerialReceiveTaskStk[TASK_STK_SIZE-1], 14);

    for (;;) {
        OSCtxSwCtr = 0;                         /* Clear context switch counter             */
        OSTimeDly(OS_TICKS_PER_SEC);			/* Wait one second                          */
    }
}

/*
 *********************************************************************************************************
 *                                              TIMER TASK
 *********************************************************************************************************
 */
void  TimerTask (void *pdata){
	void *Message;
	char  TextMessage[TX_BUFFER_SIZE];

	for (;;) {
		Message = (void *) NO_SYSTEM_ERROR;
		OSMboxPost(LedMBox, Message);
		strcpy(TextMessage,"No error\n\r");
		OSMboxPost(SerialTxMBox, (void *)TextMessage);
		OSTimeDly(5*OS_TICKS_PER_SEC);			/* Wait 5 second                          */

		Message = (void *) MEDIUM_PRIORITY_ERROR;
		OSMboxPost(LedMBox, Message);
		strcpy(TextMessage, "Medium Error\n\r");
		OSMboxPost(SerialTxMBox, (void *)TextMessage);
		OSTimeDly(5*OS_TICKS_PER_SEC);			/* Wait 5 second                          */

		Message = (void *) HIGH_PRIORITY_ERROR;
		OSMboxPost(LedMBox, Message);
		strcpy(TextMessage, "HIGH Error\n\r");
		OSMboxPost(SerialTxMBox, (void *)TextMessage);
		OSTimeDly(5*OS_TICKS_PER_SEC);			/* Wait 5 second                          */
	}
}

/*
 *********************************************************************************************************
 *                                              LED TASK (blinky)
 *********************************************************************************************************
 */
void  LedTask (void *pdata) {
	void *msg;
	FP32 frequency = 1.0;
	INT16U OnPeriodTimeout = OS_TICKS_PER_SEC/10;
	INT16U OffPeriodTimeout = OS_TICKS_PER_SEC-OnPeriodTimeout;
	INT16U LocalMessage = NO_SYSTEM_ERROR;

	for (;;) {
		msg = OSMboxAccept(LedMBox);
		LocalMessage = (INT16U)msg;
		switch (LocalMessage){
			case NO_SYSTEM_ERROR:		//f = 1.0 Hz (10% duty)
				frequency = 1.0;
				OnPeriodTimeout = OS_TICKS_PER_SEC/10;
				OffPeriodTimeout = OS_TICKS_PER_SEC-OnPeriodTimeout;
				break;
			case MEDIUM_PRIORITY_ERROR:	//f = 0.4 Hz (50% duty)
				frequency = 0.4;
				OnPeriodTimeout = OS_TICKS_PER_SEC/2;
				OffPeriodTimeout = OS_TICKS_PER_SEC-OnPeriodTimeout;
				break;
			case HIGH_PRIORITY_ERROR:	//f = 2.4 Hz (50% duty)
				frequency = 2.4;
				OnPeriodTimeout = OS_TICKS_PER_SEC/2;
				OffPeriodTimeout = OS_TICKS_PER_SEC-OnPeriodTimeout;
				break;
			case NO_MSG_SENT:
			default:
				break;
		}
		PORTB |= _BV(PORTB5); //LED ON
		OSTimeDly(OnPeriodTimeout/frequency);
		PORTB &= ~_BV(PORTB5); //LED OFF
		OSTimeDly(OffPeriodTimeout/frequency);
	}
}

/*
 *********************************************************************************************************
 *                                              SerialTransmitTask
 *********************************************************************************************************
 */
void  SerialTransmitTask (void *pdata) {
	INT8U  err;
	void *msg;
	INT8U CharCounter=0;
	INT16U StringLength;
	char *LocalMessage;

	for (;;) {
		msg = OSMboxPend(SerialTxMBox, 0, &err);
		switch(err){
			case OS_NO_ERR:
				LocalMessage = (char*)msg;
				StringLength = (INT16U)strlen(LocalMessage);
				UCSR0B |= _BV(TXCIE0);	//enable TX_Empty Interrupt
				for (CharCounter=0; CharCounter<StringLength; ++CharCounter) {
					UDR0 = LocalMessage[CharCounter];
					OSSemPend(SerialTxSem,0,&err);
				}
				UCSR0B &= ~_BV(TXCIE0);	//disable TX_Empty Interrupt
				break;
			default:
				break;
		}
	}
}

/*	Routine to Post the Transmit buffer empty semaphore	*/
void PostTxCompleteSem (void) {
	OSSemPost(SerialTxSem);
}

/*
 *********************************************************************************************************
 *                                                  SerialReceiveTask
 *********************************************************************************************************
 */
void SerialReceiveTask(void *pdata){
	INT8U err;
	void *msg;
	char rxChar;
	char *parameters;
	int parameterCounter = 0;
	timeoutPeriod = 1/10;	// 10 Hz

	for(;;){
		msg = OSMboxPend(SerialRxMbox,0,&err);
		switch(err){
			case OS_NO_ERR:
				rxChar = (char)msg;
				if(receiveCommandState != COMMAND_DEFAULT){
					if(receiveCommandState == COMMAND_RATECHANGE){
						parameters = strcat(parameters,rxChar);
						parameterCounter++;
						if(parameterCounter>3){
							parameters = strcat(parameters,'\0');
							parameterCounter = 0;
							//
							timeoutPeriod = (1/(int)parameters);
						}
						break;
					}
				} else {
					switch(rxChar){
						case 'x':
							receiveCommandState = COMMAND_OFF;
							timeoutPeriod = 0;
							break;
						case 'r':
							receiveCommandState = COMMAND_RATECHANGE;
							break;
						default:
							receiveCommandState = COMMAND_DEFAULT;
							OSMboxPost(SerialTxMBox,NO_ERR_MSG);
							break;
					}
				}
				break;
			default:	/* TimeOut: Should Never Get Here! */
				receiveCommandState = COMMAND_DEFAULT;
				break;
		}
	}
}

/* Routine to Post to the receive mailbox */
void ReadSerialChar(void){
	char rxChar = UDR0;	// fetch received value into variable "rxChar"
	//		must read UDR0 to clear interrupt flag
	// extend this function so that it places the rxByte into a QUEUE
	OSMboxPost(SerialRxMbox, (void *)rxChar);
}

void ReceiveHelper(){

}

/*
 *********************************************************************************************************
 *                                              ECOLOCATION
 *********************************************************************************************************
 */
void  SensorTask (void *pdata){
	INT8U err;
	void *msg;
	char LocalMessage;
	timeoutPeriod = 1/10;	// 10 Hz
	INT16U TriggerTimeOut = OS_TICKS_PER_SEC*timeoutPeriod;

	for (;;) {
		msg = OSMboxPend(TriggerMbox, TriggerTimeOut, &err);
		switch(err){
			case OS_NO_ERR: // message
				LocalMessage = (char)msg;
				switch (LocalMessage){
					case 'x':
						TriggerTimeOut = 0;
						break;
					case 'r':
						TriggerTimeOut = OS_TICKS_PER_SEC*timeoutPeriod;
						break;
					default:
						TriggerTimeOut = 0;
						break;
				}
				break;
			case OS_TIMEOUT: // trigger routine
				/*	Following cycle used to determine distance
					of nearest object through echolocation		*/
				PORTB |= _BV(PORTB4);	// set trigPin HIGH
				cli();	//disable interrupts
				EICRA = (1<<ISC01)|(1<<ISC00); // RISING EDGE on INT0 GENERATES AN INTERRUPT (see 328p Data Sheet, Table 12-2)
				TCNT2 = 0xE0;			/* sets counter value to 224 (11100000b)
											allows for 31 increments until overflow */
				PRR &= ~_BV(PRTIM2);	/* reset the bit to turn on Timer2 module
											in the power management section */
				sei();	//enable interrupts
				echoState = ECHO_OFF_STATE;

				OSSemPend(TriggerSem,TriggerTimeOut,&err);
				break;
		}
	}
}

/* Interrupt driven by Timer2 overflow */
void PostTriggerComplete(void) {
	PORTB &= ~_BV(PORTB4);	// turn trigger pin off
	PRR |= _BV(PRTIM2);		/* set the bit to turn off the Timer2 module
								in the power management section */
}

/* Interrupt driven by INT0 and using Timer1 for counting */
void EchoHelper(void){
	char DistMessage[TX_BUFFER_SIZE];

	switch(echoState){
		case ECHO_OFF_STATE:
			TCNT1 = 0x0000;				// reset timer/counter1
			EICRA = (1<<ISC01);			// INT0 FALLING EDGE => INTERRUPT (see 328p Data Sheet, Table 12-2)

			echoState = ECHO_ON_STATE;
			break;
		case ECHO_ON_STATE:
			cnt = TCNT1;				// read timer/counter1
			cnt /= SOUND_CONVERSION_FACTOR;
			sprintf(DistMessage, "%u\n\r", cnt);
			OSMboxPost(SerialTxMBox, (void *)DistMessage);

			echoState = ECHO_OFF_STATE;
			OSSemPost(TriggerSem);
			break;
		default:
			echoState = ECHO_OFF_STATE;
			break;
	}
}