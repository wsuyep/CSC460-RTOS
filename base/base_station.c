#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdio.h>
#include "base_station.h"
#include "os.h"
//#include "avr_console.h"
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
        rx = readADC(X);
        ry = readADC(Y);
        //printf("x value %d\n", rx);
        //_delay_ms(1000);
        //printf("y value %d\n", ry);
        //503, 522
        
        
        if(rx>503-20 && rx<503+20 && ry>521-20 && ry<521+20){
            uart_send_byte(145);
            uart_send_byte(0);
            uart_send_byte(0);
            uart_send_byte(0);
            uart_send_byte(0);
        }else{
             if(rx<300){
               //left
               Roomba_Drive(50, 1);
             }else if(rx>700){
               //right 
                Roomba_Drive(50, -1);
             }
             if(ry>800){
               //forward
               Roomba_Drive(100, 32768);
             }else if(ry<300){
               //backward  
               Roomba_Drive(-100, 32768);
             }
         }
         
    }
}



int main() 
{
   Roomba_Init();
   //init_uart_bt();
//   uart_init();
//   stdout = &uart_output;
//   stdin = &uart_input;
  // cli();
   //OS_Init();
    /*
    uart_send_byte(145);
    uart_send_byte(1);
    uart_send_byte(44);
    uart_send_byte(254);
    uart_send_byte(-44);
    */
   config();
   RommbaControl();
    
//   sei();
  // OS_Start();
   return 0;
}