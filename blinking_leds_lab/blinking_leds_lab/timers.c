#include "timer.h"
#include "LEDs.h"

#include <avr/interrupt.h>

// GLOBALS
extern uint32_t G_green_ticks;
extern uint32_t G_yellow_ticks;
extern uint32_t G_ms_ticks;

extern uint16_t G_red_period;
extern uint16_t G_green_period;
extern uint16_t G_yellow_period;

extern uint16_t G_release_red;

void init_timers() {

	// -------------------------  RED --------------------------------------//
	// Software Clock Using Timer/Counter 0.
	// THE ISR for this is below.

	// SET appropriate bits in TCCR....

	// Using CTC mode with OCR0 for TOP. This is mode X, thus WGM0/1/0 = .
	TCCR0A = 0x82; //table 15-9
	//TCCR0A =   10 00 | 00 10
	//TCCR0A =   clear_OC0A ignore | notused WGM1_WGM0
	
	TCCR0B = 0x04; //table 15-9
	//TCCR0B =   00 00 | 0 100     100 == 256 prescaler
	//TCCR0B =   ignore notused | WGM2 prescaler
	
	
	// Using pre-scaler XX. This is CS0/2/1/0 = 
	//CLKPR=0x84;  //Sec 9.12
	
	
	// Software Clock Interrupt Frequency: 1000 = f_IO / (prescaler*OCR0)
	// Set OCR0 appropriately for TOP to generate desired frequency of 1KHz
	printf("Initializing software clock to freq 1000Hz (period 1 ms)\n");	
	OCR0A = 77;

	//Enable output compare match interrupt on timer 0A
	TIMSK0 = 0x02;  //table 15.9.6 //page 105
	//means use OCR0A instead of OCR0B

	// Initialize counter
	G_ms_ticks = 0;


	//--------------------------- YELLOW ----------------------------------//
	// Set-up of interrupt for toggling yellow LEDs. 
	// This task is "self-scheduled" in that it runs inside the ISR that is 
	// generated from a COMPARE MATCH of 
	//      Timer/Counter 3 to OCR3A.
	// Obviously, we could use a single timer to schedule everything, but we are experimenting here!
	// THE ISR for this is in the LEDs.c file


	// SET appropriate bits in TCCR ...
	TCCR3A = 0x80;  //table 16-2
	//TCCR3A =   10 00 | 00 00
	//TCCR3A =   clear_OC3A ignore | notused WGM1_WGM0
	
	TCCR3B = 0x0D; //table 16-2
	//TCCR3B =   00 0 0 |  1 101
	//TCCR3B =   ignore notused WGM3 | WGM2 prescaler
	
	TCCR3C = 0x00;
	
	//20000000 / (1024) = 19531	
	//19531 times per second TCNT3 is incremented
	//
	
	
	// Using CTC mode with OCR3A for TOP. This is mode XX, thus WGM1/3210 = .
	TIMSK3 = 0x02;
	//means use the OCR3A instead of OCR3B
	
	// Using pre-scaler XX. This is CS1_210 = 
	//CS1_210 = 


	// Interrupt Frequency: 10 = f_IO / (prescaler*OCR3A)
	// Set OCR3A appropriately for TOP to generate desired frequency using Y_TIMER_RESOLUTION (100 ms).
	// NOTE: This is not the toggle frequency, rather a tick frequency used to time toggles.
	OCR3A = 1952;
	uint32_t freq = 20000000 / (1024* OCR3A);
	printf("Initializing yellow clock to freq %dHz (period %d ms)\n",(int)freq,Y_TIMER_RESOLUTION);	
	char* tmp = malloc(32);
	int l = sprintf(tmp, "frequency is %d\n", freq);
	print_usb(tmp,l+1);

	//Enable output compare match interrupt on timer 3A

	G_yellow_ticks = 0;


	//--------------------------- GREEN ----------------------------------//
	// Set-up of interrupt for toggling green LED. 
	// This "task" is implemented in hardware, because the OC1A pin will be toggled when 
	// a COMPARE MATCH is made of 
	//      Timer/Counter 1 to OCR1A.
	// We will keep track of the number of matches (thus toggles) inside the ISR (in the LEDs.c file)
	// Limits are being placed on the frequency because the frequency of the clock
	// used to toggle the LED is limited.

	// Using CTC mode with OCR1A for TOP. This is mode XX, thus WGM3/3210 = .
	TCCR1A = 0x82;  //table 15-9
	//TCCR1A =   10 00 | 00 10
	//TCCR1A =   clear_OC1A ignore | notused WGM1_WGM0
	
	TCCR1B = 0x15; //table 15-9
	//TCCR1B =   00 0 1 |  0 101
	//TCCR1B =   ignore notused WGM3 | WGM2 prescaler

	// Toggle OC1A on a compare match. Thus COM1A_10 = 10

	// Using pre-scaler 1024. This is CS1/2/1/0 = XXX
	
	TIMSK1 = 0x02;
	
	//20000000 / ( 1024 ) = 19531 times per second that TNCT1A is incremented
	
	// Interrupt Frequency: ? = f_IO / (1024*OCR1A)
	// Set OCR1A appropriately for TOP to generate desired frequency.
	// NOTE: This IS the toggle frequency.
	printf("green period %d\n",G_green_period);
	OCR1A = 19531*((float)G_green_period/2000);
	ICR1  = 39062*((float)G_green_period/2000);
	printf("Set OCR1A to %d\n",OCR1A);
	//printf("Initializing green clock to freq %dHz (period %d ms)\n",(int)(XXXX),G_green_period);	

	// A match to this will toggle the green LED.
	// Regardless of its value (provided it is less than OCR1A), it will match at the frequency of timer 1.
	//OCR1B = 1;

	//Enable output compare match interrupt on timer 1B

}

ISR(TIMER0_COMPA_vect)
{
	// This is the Interrupt Service Routine for Timer0 (10ms clock used for scheduling red).
	// Each time the TCNT count is equal to the OCR0 register, this interrupt is "fired".

	// Increment ticks
	G_ms_ticks++;

	// if time to toggle the RED LED, set flag to release
	if ( ( G_ms_ticks % G_red_period ) == 0 )
	G_release_red = 1;
}
