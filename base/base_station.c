#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdio.h>
#include "base_station.h"
#include "os.h"
#include "avr_console.h"
#include "./roomba/roomba.h"
#include "./uart/uart.h"

int servoState = 375;
uint8_t SERVO = 1;
uint8_t ROOMBA = 4;

void initADC(){
    //set scalar 
    ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
    //set 5V
    ADMUX |= (1 << REFS0);
    //ADMUX |= (1 << ADLAR);
    ADCSRA |= (1 << ADEN);
    //ADCSRA |= (1 << ADSC);
}

void config(){
    //set pin mode input for Analog A10 
    DDRK &= ~(1 << DDK2);
    //set pin mode input for Analog A11
    DDRK &= ~(1 << DDK3);
    //set digital input 50 to low
    PORTD &= ~(1 << PORTD7);
    initADC();
}

uint16_t readADC(uint8_t channel) {
	ADMUX = (ADMUX & 0xE0 ) | (0x07 & channel);
	ADCSRB = (ADCSRB & 0xF7) | (channel & (1 << MUX5));
	ADCSRA |= (1 << ADSC);
    while ((ADCSRA & (1 << ADSC)));
    //scale to 
    ADC = (0.458*ADC) + 140;
	return ADC;
}

void RommbaControl(){
    for(;;){
        x = readADC(X);
        y = readADC(Y);
        printf("x value %d\n", x);
        _delay_ms(1000);
        printf("y value %d\n", y);
        
        
        Bluetooth_Send_Byte(x);
        Bluetooth_Send_Byte(y);
        
    }
}

void test(){
     for(;;){
        printf("test printf");   
     }
}


int main() 
{
    
   uart_init();
   stdout = &uart_output;
   stdin = &uart_input;
   cli();
   //DDRB=0x83;
   OS_Init();
   config();
   init_uart_bt();
   Task_Create_System(RommbaControl,1);
   sei();
   OS_Start();
   return -1;
}