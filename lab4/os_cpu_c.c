/*
**********************************************************************************************************
*                                               uC/OS-II
*                                         The Real-Time Kernel
*
*                                           AVR Specific code
*
* File         : OS_CPU_C.C
* By           : Ole Saether
* Port Version : V1.01
*
* AVR-GCC port version : 1.0 	2001-04-02 modified/ported to avr-gcc by Jesper Hansen (jesperh@telia.com)
*
* Some modifications by Julius Luukko (Julius.Luukko@lut.fi) to get this compiled with uc/OS-II 2.52
*
* Modifications by Julius Luukko 2003-03-06 (Julius.Luukko@lut.fi):
*
* - RAMPZ is also saved to the stack
*
* Modifications by Julius Luukko 2003-06-24 (Julius.Luukko@lut.fi):
*
* - RAMPZ is only saved to the stack if it is defined, i.e. with chips that have it
*
* Modifications by Julius Luukko 2003-07-21 (Julius.Luukko@lut.fi) to support uC/OS-II v2.70
*
* - OS_TASK_SW_HOOK_EN is tested and if not enabled, no code is generated for OSTaskSwHook()
*
**********************************************************************************************************
*/

// #ifndef  OS_MASTER_FILE
#ifndef OS_GLOBALS
// #define  OS_CPU_GLOBALS
#include "includes.h"
#include "avr/wdt.h"
#endif

//#ifndef OS_TASK_SW_HOOK_EN
//#define OS_TASK_SW_HOOK_EN 1
//#endif


#define  BAUD			38400
#define  FOSC			16000000			// 16 MHz
#define  UBRR			((FOSC/16/BAUD)-1)


extern 	void WatchDogReset();


void InitPeripherals(void);

void InitPeripherals(void)
{
	cli();			// disable global interrupts

	// manage WDT
	WatchDogReset();
	//MCUSR &= ~(1<<WDRF);			// clear WDRF in MCUSR
	//WDTCSR |= (1<<WDCE)|(1<<WDE)|0x08;	// write a logic one to WDCE and WDE and keep old prescalar setting to prevent unintentional timeout
	//WDTCSR=0x00;					// turn wdt off
	wdt_disable();

#if 1
	// PortB
	//
	PORTB &= ~_BV(PORTB5);	// set pin 5 (Arduino DIO pin 13) low to turn led off
	PORTB &= ~_BV(PORTB4);	// set pin 4 (trigPin) low
	PORTB &= ~_BV(PORTB0);	// set pin 0 low
	DDRB |= _BV(DDB5);		// set pin 5 of PORTB for output
	DDRB |= _BV(DDB4);		// set pin 4 of PORTB for output
	DDRB |= _BV(DDB0);		// set pin 0 of PORTB for output
#endif

#if 1
	// PortD
	//		EXTERNAL INTERRUPT PIN (ARDUINO DIO pin 2)
	PORTD &= ~_BV(PORTD2);	// set pin 2 (Arduino DIO pin 2) low
	DDRD &= ~_BV(DDD2);		// set pin 2 (echoPin) for input

	// External Interrupt Control Register A
	EICRA = (1<<ISC01) | (1<<ISC00);	// RISING EDGE on INT0 GENERATES AN INTERRUPT
	EIFR  = (1<<INTF0);				// clear the INT0 interrupt flag
	EIMSK = (1<<INT0);					// enable INT0 interrupt
#endif

#if 1
	// Timer0 : UC/OS-II timer tick
	//
	TCCR0A = _BV(WGM01) | _BV(WGM00);				/* set timer0: OC0A/OC0B disconnected; fast PCM mode           */
	TCCR0B = _BV(WGM02) | _BV(CS02)| _BV(CS00);		/* timer0 clock = system clock/1024      */
	OCR0A = (CPU_CLOCK_HZ/OS_TICKS_PER_SEC/1024)-1; /* This combination yields an interrupt every 5 msec  */
	TIMSK0 |= _BV(TOIE0);							/* enable timer0 CTC-A interrupt */
	PRR &= ~_BV(PRTIM0);							/* turn on the module in the power management section */
#endif

#if 1
	// Timer1 : duration counter
	//
	TCCR1A = 0x00;				/* set timer1: OC1A/OC1B disconnected; normal mode           */
	TCCR1B = _BV(CS11);			/* timer1 clock = system clock/128      */
	TCCR1C = 0x00;				/* NO force Compare */
	TIFR1 |= _BV(ICF1) | _BV(OCF1B) | _BV(OCF1A) | _BV(TOV1);	/* clear all flags */
	PRR &= ~_BV(PRTIM1);			/* reset the bit to turn ON Timer1 module in the power management section */
#endif

#if 1
	// Timer2 : 10us timer
	//
	TCCR2A = 0x00;							// set timer2: OC2A/OC2B disconnected; normal mode
	//TCCR2B = _BV(CS22)|_BV(CS21)|_BV(CS20);	// timer2 clock = system clock/1024
	TCCR2B = _BV(CS21);						// timer2 clock = system clock/8
	TIFR2 |= _BV(TOV2);						// clear overflow bit
	TIMSK2 |= _BV(TOIE2);					// enable timer2 overflow interrupt
	PRR |= _BV(PRTIM2);						// set the bit to turn off the Timer2 module in the power management section
#endif

#if 1
	// USART0 (based on AVR ATMega 328p Data Sheet, pg. 183)
	//
	UBRR0H = (unsigned char)(UBRR>>8);	// set baud rate = 38400
	UBRR0L = (unsigned char)UBRR;		//
	//UCSR0A = (1<<U2X0); // doubles the effective baud rate (because OS_TICKS_PER_SECOND got halved)
	/* enable receiver and transmitter */
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);		//
	/* set frame format: 8data, 1 stop bit */
	UCSR0C = (1<<UCSZ00)|(1<<UCSZ01)|(0<USBS0);
#endif

	// Enable Global Interrupts
	//
	sei();							/* enable interrupts                */
}





/*
 **********************************************************************************************************
 *                                        INITIALIZE A TASK'S STACK
 *
 * Description: This function is called by either OSTaskCreate() or OSTaskCreateExt() to initialize the
 *              stack frame of the task being created. This function is highly processor specific.
 *
 * Arguments  : task          is a pointer to the task code
 *
 *              pdata         is a pointer to a user supplied data area that will be passed to the task
 *                            when the task first executes.
 *
 *              ptos          is a pointer to the top of stack. It is assumed that 'ptos' points to the
 *                            highest valid address on the stack.
 *
 *              opt           specifies options that can be used to alter the behavior of OSTaskStkInit().
 *                            (see uCOS_II.H for OS_TASK_OPT_???).
 *
 * Returns    : Always returns the location of the new top-of-stack' once the processor registers have
 *              been placed on the stack in the proper order.
 *
 * Note(s)    : Interrupts are enabled when your task starts executing. You can change this by setting the
 *              SREG to 0x00 instead. In this case, interrupts would be disabled upon task startup. The
 *              application code would be responsible for enabling interrupts at the beginning of the task
 *              code. You will need to modify OSTaskIdle() and OSTaskStat() so that they enable
 *              interrupts. Failure to do this will make your system crash!
 *
 **********************************************************************************************************
 */

//OS_STK *OSTaskStkInit (void (*task)(void *pd), void *pdata, OS_STK *ptos, INT16U opt)
void *OSTaskStkInit (void (*task)(void *pd), void *pdata, void *ptos, INT16U opt)
{
    INT8U  *stk;
    INT16U  tmp;

    opt     = opt;                          /* 'opt' is not used, prevent warning                       */
    stk     = (INT8U *)ptos;		    /* AVR return stack ("hardware stack")          		*/
    tmp     = (INT16U)task;

    /* "push" initial register values onto the stack */

    *stk-- = (INT8U)tmp;                   /* Put task start address on top of stack          	        */
    *stk-- = (INT8U)(tmp >> 8);

    *stk-- = (INT8U)0x00;                   /* R0  = 0x00                                               */
    *stk-- = (INT8U)0x00;                   /* R1  = 0x00                                               */
    *stk-- = (INT8U)0x00;                   /* R2  = 0x00                                               */
    *stk-- = (INT8U)0x00;                   /* R3  = 0x00                                               */
    *stk-- = (INT8U)0x00;                   /* R4  = 0x00                                               */
    *stk-- = (INT8U)0x00;                   /* R5  = 0x00                                               */
    *stk-- = (INT8U)0x00;                   /* R6  = 0x00                                               */
    *stk-- = (INT8U)0x00;                   /* R7  = 0x00                                               */
    *stk-- = (INT8U)0x00;                   /* R8  = 0x00                                               */
    *stk-- = (INT8U)0x00;                   /* R9  = 0x00                                               */
    *stk-- = (INT8U)0x00;                   /* R10 = 0x00                                               */
    *stk-- = (INT8U)0x00;                   /* R11 = 0x00                                               */
    *stk-- = (INT8U)0x00;                   /* R12 = 0x00                                               */
    *stk-- = (INT8U)0x00;                   /* R13 = 0x00                                               */
    *stk-- = (INT8U)0x00;                   /* R14 = 0x00                                               */
    *stk-- = (INT8U)0x00;                   /* R15 = 0x00                                               */
    *stk-- = (INT8U)0x00;                   /* R16 = 0x00                                               */
    *stk-- = (INT8U)0x00;                   /* R17 = 0x00                                               */
    *stk-- = (INT8U)0x00;                   /* R18 = 0x00                                               */
    *stk-- = (INT8U)0x00;                   /* R19 = 0x00                                               */
    *stk-- = (INT8U)0x00;                   /* R20 = 0x00                                               */
    *stk-- = (INT8U)0x00;                   /* R21 = 0x00                                               */
    *stk-- = (INT8U)0x00;                   /* R22 = 0x00                                               */
    *stk-- = (INT8U)0x00;                   /* R23 = 0x00                                               */

    tmp    = (INT16U)pdata;
    *stk-- = (INT8U)tmp;                    /* Simulate call to function with argument                  */
    *stk-- = (INT8U)(tmp >> 8);		    /* R24, R25 contains argument pointer pdata 		*/

    *stk-- = (INT8U)0x00;                   /* R26 = 0x00                                               */
    *stk-- = (INT8U)0x00;                   /* R27 = 0x00                                               */
    *stk-- = (INT8U)0x00;                   /* R28 = 0x00                                               */
    *stk-- = (INT8U)0x00;                   /* R29 = 0x00                                               */
    *stk-- = (INT8U)0x00;                   /* R30 = 0x00                                               */
    *stk-- = (INT8U)0x00;                   /* R31 = 0x00                                               */
#ifdef RAMPZ
    *stk-- = (INT8U)0x00;                   /* RAMPZ = 0x00                                             */
#endif
    *stk-- = (INT8U)0x80;                   /* SREG = Interrupts enabled                                */
    return ((OS_STK *)stk);
}

/*$PAGE*/

/*
*********************************************************************************************************
*                                          TASK CREATION HOOK
*
* Description: This function is called when a task is created.
*
* Arguments  : ptcb   is a pointer to the task control block of the task being created.
*
* Note(s)    : 1) Interrupts are disabled during this call.
*********************************************************************************************************
*/
#if OS_CPU_HOOKS_EN > 0
void OSTaskCreateHook (OS_TCB *ptcb)
{
    ptcb = ptcb;                       /* Prevent compiler warning                                     */
}
#endif


/*
*********************************************************************************************************
*                                           TASK DELETION HOOK
*
* Description: This function is called when a task is deleted.
*
* Arguments  : ptcb   is a pointer to the task control block of the task being deleted.
*
* Note(s)    : 1) Interrupts are disabled during this call.
*********************************************************************************************************
*/
#if OS_CPU_HOOKS_EN > 0
void OSTaskDelHook (OS_TCB *ptcb)
{
    ptcb = ptcb;                       /* Prevent compiler warning                                     */
}
#endif

/*
*********************************************************************************************************
*                                           TASK SWITCH HOOK
*
* Description: This function is called when a task switch is performed.  This allows you to perform other
*              operations during a context switch.
*
* Arguments  : none
*
* Note(s)    : 1) Interrupts are disabled during this call.
*              2) It is assumed that the global pointer 'OSTCBHighRdy' points to the TCB of the task that
*                 will be 'switched in' (i.e. the highest priority task) and, 'OSTCBCur' points to the
*                 task being switched out (i.e. the preempted task).
*********************************************************************************************************
*/
#if (OS_CPU_HOOKS_EN > 0) && (OS_TASK_SW_HOOK_EN > 0)
void OSTaskSwHook (void)
{
}
#endif

/*
*********************************************************************************************************
*                                           STATISTIC TASK HOOK
*
* Description: This function is called every second by uC/OS-II's statistics task.  This allows your
*              application to add functionality to the statistics task.
*
* Arguments  : none
*********************************************************************************************************
*/
void OSTaskStatHook (void)
{
}

/*
*********************************************************************************************************
*                                               TICK HOOK
*
* Description: This function is called every tick.
*
* Arguments  : none
*
* Note(s)    : 1) Interrupts may or may not be ENABLED during this call.
*********************************************************************************************************
*/
#if (OS_CPU_HOOKS_EN > 0)
//#if (OS_CPU_HOOKS_EN > 0) && (OS_TIME_TICK_HOOK_EN > 0)
void OSTimeTickHook (void)
{
}
#endif

/*
*********************************************************************************************************
*                                       OS INITIALIZATION HOOK
*                                            (BEGINNING)
*
* Description: This function is called by OSInit() at the beginning of OSInit().
*
* Arguments  : none
*
* Note(s)    : 1) Interrupts should be disabled during this call.
*********************************************************************************************************
*/
#if OS_CPU_HOOKS_EN > 0 && OS_VERSION > 203
void OSInitHookBegin (void)
{
}
#endif

/*
*********************************************************************************************************
*                                       OS INITIALIZATION HOOK
*                                               (END)
*
* Description: This function is called by OSInit() at the end of OSInit().
*
* Arguments  : none
*
* Note(s)    : 1) Interrupts should be disabled during this call.
*********************************************************************************************************
*/
#if OS_CPU_HOOKS_EN > 0 && OS_VERSION > 203
void OSInitHookEnd (void)
{
}
#endif

/*
*********************************************************************************************************
*                                             IDLE TASK HOOK
*
* Description: This function is called by the idle task.  This hook has been added to allow you to do
*              such things as STOP the CPU to conserve power.
*
* Arguments  : none
*
* Note(s)    : 1) Interrupts are enabled during this call.
*********************************************************************************************************
*/
#if OS_CPU_HOOKS_EN > 0 && OS_VERSION >= 251
void OSTaskIdleHook (void)
{
}
#endif

/*
*********************************************************************************************************
*                                           OSTCBInit() HOOK
*
* Description: This function is called by OS_TCBInit() after setting up most of the TCB.
*
* Arguments  : ptcb    is a pointer to the TCB of the task being created.
*
* Note(s)    : 1) Interrupts may or may not be ENABLED during this call.
*********************************************************************************************************
*/
#if OS_CPU_HOOKS_EN > 0 && OS_VERSION > 203
void OSTCBInitHook (OS_TCB *ptcb)
{
    ptcb = ptcb;                       /* Prevent compiler warning                                     */
}
#endif

