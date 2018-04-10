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
        //uint8_t x = 3;
        //Bluetooth_Send_Byte('l');
        //stop
        if (rx>503-20 && rx<503+20 && ry>521-20 && ry<521+20){
            Bluetooth_Send_Byte('s');
        }else{
             if(rx<300){
               //left
                Bluetooth_Send_Byte('l');
             }else if(rx>700){
               //right 
                //Roomba_Drive(50, -1);
                Bluetooth_Send_Byte('r');
             }
             if(ry>800){
               //forward
               Bluetooth_Send_Byte('b');
               //Roomba_Drive(100, 32768);
             }else if(ry<300){
               //backward  
               Bluetooth_Send_Byte('f');
               //Roomba_Drive(-100, 32768);
             }
         }
        
    }
}



int main() 
{
   //Roomba_Init();
   init_uart_bt();
//   uart_init();
//   stdout = &uart_output;
//   stdin = &uart_input;
  // cli();
   //OS_Init();

   config();
   RommbaControl();
    
//   sei();
  // OS_Start();
   return 0;
}