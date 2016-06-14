/*
 * Plaadi test.c
 *
 * Created: 30.05.2016 20:51.19
 * Author : Andres
 */ 
#define F_CPU 16000000


#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include "usb_serial.h"

//various pin definitions, for easier readability
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

void usbSendString(char* string);
void receiveDataFromUSB();

uint8_t rotationOverflow; //flags if the rotation speed is too slow and the period counter overflows
uint8_t screenStep; //which step of the screen we are currently at
uint8_t screenData[128][6]; //the data that is displayed, 128 segments
uint8_t screenClearData[6] = {0}; //used for clearing the screen when needed
uint8_t screenTestData[6] = {0,0,0,1<<7,1<<7,1<<7}; //used for testing

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
	//update the screen, the timer interrupts 128 times each rotation
	if(screenStep < 128){
		shift(screenData[screenStep]);
		++screenStep;
	}
	else{
		//shift(screenClearData);
		TIMSK3 = 0;
	}
}

ISR(TIMER1_OVF_vect){
	//too much time passed to measure the rotation period, rotation < 4Hz
	rotationOverflow = 1;
	shift(screenTestData); //light up one led, to show that the device is working
	TIMSK3 = 0; //turn off screen updates by disabling the screen update interrupts
}

ISR(INT0_vect){
	//magnet detected
	//PINC |= LED5; //for debugging purposes flash a led on the underside
	if(!rotationOverflow){
		//start the screen updating again, the rotation period didn't overflow and we can trust the period value
		TIMSK3 = 1<<OCIE3A;
	}
	//update the counter so that we will get an interrupt 128 times per second given the last TCNT1 value, zero the rotation period counter
	OCR3A = TCNT1>>1;
	TCNT3 = OCR3A-1; //this gives us an interrupt on the next counter increase
	TCNT1 = 0; //zero the period counter
	rotationOverflow = 0; //zero the period timer overflow flag
	screenStep = 0; //we are at screen segment 0
}

void usbSendString(char* string){
	//a function used for sending strings with usb
	uint8_t len = 0; //let's find the length of the string
	while(string[len] != 0){
		++len;
	}
	if(len != 0){
		usb_serial_write((uint8_t*)string, len);
	}
}

void readDataFromEeprom(){
	//this reads the data from eeprom into RAM during startup
	eeprom_read_block(screenData, 0, 6*128);
}

void receiveDataFromUSB(){
	//this function receives via usb 6 bytes of data for each of the 128 segments and saves it to eeprom
	for(uint8_t n = 0; n < 128; ++n){
		for(uint8_t m = 0; m < 6; ++m){
			while(!usb_serial_available()); //wait for a char
			int16_t c = usb_serial_getchar();
			if(c == -1){
				usbSendString("ATMEGA USB RECEIVE ERROR\n");
			}
			screenData[n][m] = c&0xFF; 
		}
	}
	
	//send an acknowledgement back, that data has been received and write it to the eeprom
	usbSendString("Data received\n");
	usbSendString("Writing to eeprom...\n");
	eeprom_write_block(screenData, 0, 6*128);
	usbSendString("Eeprom written\n");
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
	
	//this timer gives a compare event 128 times each rotation, so that we can update the states of the leds
	TCCR3B = 0b001 | (1<<WGM12); //prescaler 1, CTC
	OCR3A = TCNT1>>1; //divide by two so that we get 128 interrupts
	TIMSK3 = 1<<OCIE3A;
	
	//magnet sensor interrupt
	EICRA = 0b10;
	EIMSK = 1;
	
	PORTD |= LED1; //these leds are on the other side, for testing purposes
	PORTD |= LED2;
	readDataFromEeprom(); //read the saved image data from the eeprom
	
	usb_init(); //init usb
	while(!usb_configured());
	sei();
	
    while (1) 
    {
		if(usb_serial_available()){ //check is data transmission is being started from the usb
			receiveDataFromUSB();
		}
    }
}
