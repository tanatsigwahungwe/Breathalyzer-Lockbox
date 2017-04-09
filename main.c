//Authors: Nikita Gawande, Tanatsigwa Hungwe, Rebecca Wolf
#include "msp430.h"

// Button Bit on pin P1.3
#define BUTTON BIT3
volatile unsigned int button_presses = 0;
// if button has been pressed once, lock door
// if button has been pressed twice, test adc input
volatile int state = 0;
volatile unsigned char last_button = 0;

// LED Bit on pin P1.0
#define RED BIT0  // on when door is locked



// ADC pin P1.4 input is the alcohol gas sensor
#define ADC_INPUT_BIT_MASK BIT4
#define ADC_INCH INCH_4

 // function declarations
 void init_button(void);
 void init_adc(void);
 void init_wdt(void);
 void init_timer(void);


// =======Button Initialization========
void init_button(){
  P1DIR &= ~BUTTON;
  P1OUT |= BUTTON;
  P1REN |= BUTTON;
}

// =======ADC Initialization and Interrupt Handler========
 volatile int latest_result;   // most recent conversion
 volatile unsigned long conversion_count = 0; //total number of conversions
 volatile int did_pass = 0;


/*
 * The ADC handler is invoked when a conversion is complete.
 * Here we determine whether or not to engage the servo (lock or unlock the door)
 */

void interrupt adc_handler(){
	 latest_result = ADC10MEM;   // will be a value between 0 and 1023
	 ++conversion_count;       // increment the total conversion count
   // did the sample pass the test?
   if (latest_result > 1022) {
    did_pass = 0;
   }
   else if (latest_result <= 1022) {
    did_pass = 1;
   }

  // STATE MACHINE:
   if (state == 0) {

   }
  if (state == 1) {
    // turn on Red LED
    P1OUT |= RED;
    // lock the door (engage servo)
    TA0CCR1 = 2000;


  }
  else if (state == 2) {  // attempted test, did not pass, button has been pressed twice
    // reset button_presses to 1 so we can attempt test again
    button_presses = 1;
    // Do nothing to the LED
    // do nothing to the servo

  }
  else if (state == 3) {  // passed test and button has been pressed twice
    // reset button_presses to 0
    button_presses = 0;
    // turn off LED
    P1OUT &= ~RED;
    // reset state machine
    state = 0;
    // unlock the door (engage servo)
    TA0CCR1 = 1000;


  }


}
ISR_VECTOR(adc_handler, ".int05")



// Initialization of ADC
void init_adc(){
  ADC10CTL1 = (ADC_INCH	+ SHS_0 + ADC10DIV_4 + ADC10SSEL_0 + CONSEQ_0);
  ADC10AE0 = ADC_INPUT_BIT_MASK;
  ADC10CTL0 = (SREF_0	+ ADC10SHT_3 + ADC10ON	+ ENC + ADC10IE);
}




 // ===== Watchdog Timer Interrupt Handler =====
interrupt void WDT_interval_handler(){

  // trigger a conversion
  ADC10CTL0 |= ADC10SC;


  unsigned char b;
  b = (P1IN & BUTTON);  // read the BUTTON bit
//  if (b == 1) {
//	  P1OUT |= RED;
//  }


  if (last_button && (b == 0)) { // button bit: high -> low
    button_presses++; // increase num of times button has been pressed
    // state machine control

    // in state 0 the door is unlocked and nothing has happened. after the very
    // first button press, this can only be reassigned after the test has been passed

    if (button_presses == 1) {
      state = 1;
    }
    else if ( (button_presses == 2) && (did_pass == 0) ) {
      state = 2;
    }
    else if ( (button_presses == 2) && (did_pass == 1) ) {
      state = 3;
    }
  }
  last_button = b;





}
ISR_VECTOR(WDT_interval_handler, ".int10")



 void init_wdt(){
	// setup the watchdog timer as an interval timer
  	WDTCTL = (WDTPW + WDTTMSEL + WDTCNTCL);
    IE1 |= WDTIE;			// enable the WDT interrupt
 }



void main(){

	WDTCTL = WDTPW + WDTHOLD;       // Stop watchdog timer
	BCSCTL1 = CALBC1_1MHZ;			// 1Mhz calibration for clock -> needed for servo
	DCOCTL  = CALDCO_1MHZ;

  // initializations for red LED
  P1DIR |= RED;
  P1OUT &= ~RED;  // start with LED off

  // initializations for servo on pin 1.6
  P1DIR |= BIT6;							// P1.6/TA0.1 is used for PWM, thus also an output -> servo
  P1SEL |= BIT6;                          // P1.6 select TA0.1 option
  TA0CCR0 = 20000-1;                           // PWM Period TA0.1

  TA0CCTL1 = OUTMOD_7;                       // CCR1 reset/set
  	TA0CTL   = TASSEL_2 + MC_1;                // SMCLK, up mode




     init_button();
   	init_adc();
   	init_wdt();



	_bis_SR_register(GIE+LPM0_bits);
}
