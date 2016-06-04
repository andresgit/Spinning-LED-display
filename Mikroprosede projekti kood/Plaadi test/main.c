/*
 * Plaadi test.c
 *
 * Created: 30.05.2016 20:51.19
 * Author : Andres
 */ 
#define F_CPU 1000000

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <util/delay.h>
#include <avr/eeprom.h>

#define LED1 (1<<PORTD4)
#define LED2 (1<<PORTD6)
#define LED3 (1<<PORTD7)
#define LED4 (1<<PORTC6)
#define LED5 (1<<PORTC7)
#define MAGNET (1<<PORTD0)
#define TX (1<<PORTD3)
#define CLK (1<<PORTD5)
#define LE1 (1<<PORTF7)
#define NROE1 (1<<PORTF6)
#define NGOE1 (1<<PORTF5)
#define NBOE1 (1<<PORTF4)
#define LE2 (1<<PORTB7)
#define NROE2 (1<<PORTB6)
#define NGOE2 (1<<PORTB5)
#define NBOE2 (1<<PORTB4)

uint16_t rotationLengthCounter;
uint8_t rotationOverflow;
uint8_t screenStep; //which step of the screen we are currently at
uint8_t screenData[128][6];
uint8_t screenClearData[6] = {0};
uint8_t screenTestData[6] = {0,0,0,1<<7,1<<7,1<<7};

void shift(uint8_t data[6]){
	//shift the 6 bytes of data out to the led drivers, the data is in the order RGB, leds start counting from the center outwards
	//in the first byte of the data there are the first 8 reds, then the first 8 blues, then the first 8 greens, then the next 8 reds and so on.
	for(uint8_t byte = 3; byte < 6; ++byte){ //shift the data for the second led driver first
		for(uint8_t bit = (1<<7); bit != 0; bit >>= 1){
			if(data[byte]&bit){
				PORTD |= TX;
			}
			else{
				PORTD &= ~TX;
			}
			PORTD |= CLK;
			PORTD &= ~CLK;
		}
	}
	//shift the data for the first led driver, since the leds are in a different order, we need to shift data in the order 56781234
	for(uint8_t byte = 0; byte < 3; ++byte){
		for(uint8_t bit = (1<<4); bit != 0; bit <<= 1){
			if(data[byte]&bit){
				PORTD |= TX;
			}
			else{
				PORTD &= ~TX;
			}
			PORTD |= CLK;
			PORTD &= ~CLK;
		}
		for(uint8_t bit = 1; bit != (1<<4); bit <<= 1){
			if(data[byte]&bit){
				PORTD |= TX;
			}
			else{
				PORTD &= ~TX;
			}
			if(byte == 2 && bit == (1<<3)){//for the last bit enable the latch
				PORTB |= LE2;
				PORTF |= LE1;
				PORTD |= CLK;
				PORTD &= ~CLK;
				PORTF &= ~LE1;
				PORTB &= ~LE2;
			}
			else{
				PORTD |= CLK;
				PORTD &= ~CLK;
			}
		}
	}
}

ISR(TIMER3_COMPA_vect){
	if(screenStep < 128){
		shift(screenData[screenStep]);
		++screenStep;
	}
	else{
		shift(screenClearData);
		TIMSK3 = 0;
	}
}

ISR(TIMER1_OVF_vect){
	//too much time passed to measure the rotation period, rotation < 4Hz
	rotationOverflow = 1;
	shift(screenTestData); //light up, to show that the device is working
	TIMSK3 = 0; //turn off screen updates
}

ISR(INT0_vect){
	//magnet detected
	PINC |= LED5; //for debugging flash a led on the underside
	if(!rotationOverflow){
		//start the screen updating again, the rotation period didn't overflow and we can use the period
		TIMSK3 = 1<<OCIE3A;
	}
	//update the counter so that we will get an interrupt 128 times per second given the last TCNT1 value, zero the rotation period counter
	OCR3A = TCNT1>>1;
	TCNT3 = OCR3A-1;
	TCNT1 = 0;
	rotationOverflow = 0;
	screenStep = 0;
}

int main(void)
{
	//turn off JTAG, so that we can use the pins
	uint8_t register reg = MCUCR | (1<<JTD);
	MCUCR = reg;
	MCUCR = reg;
	//clock_prescale_set(clock_div_16);

	//these leds are for debugging
	DDRD = LED1 | LED2 | LED3;
	DDRC = LED4 | LED5;
	
	PORTD = LED1|LED2|LED3;
	PORTC = LED4|LED5;
	
	//enable the pins that control the led drivers
	DDRF |= NROE1|NGOE1|NBOE1|LE1;
	DDRB = NROE2|NGOE2|NBOE2|LE2;
	DDRD |= TX|CLK;
	
	//enable led driver outputs
	PORTF &= ~(NROE1|NGOE1|NBOE1);
	PORTB &= ~(NROE2|NGOE2|NBOE2);
	
	//this timer keeps track of how long does it take to make one rotation
	TCCR1B = 0b011; //prescaler 64
	TIMSK1 = 1<<TOIE1;
	
	//this timer gives a compare event 64 times each rotation, so that we can update the states of the leds
	TCCR3B = 0b001 | (1<<WGM12); //prescaler 1, CTC
	OCR3A = TCNT1;
	TIMSK3 = 1<<OCIE3A;
	
	//magnet sensor interrupt
	EICRA = 0b10;
	EIMSK = 1;
	
	
	screenData[61][4] = 1<<6;
	screenData[62][4] = 1<<7;
	screenData[63][4] = 1<<5;
	
	for(uint8_t n = 0; n < 128; ++n){
		screenData[n][4] = 1<<(n%8);
	}
	
	screenData[0][3] |= 2;
	
	sei();
	
	//uint8_t testdata[6]={1,2,4,1,2,4};
	//shift(testdata);
	
    while (1) 
    {
    }
}
