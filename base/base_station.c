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
    
    //set pin mode input for Analog A12 
    DDRK &= ~(1 << DDK4);
    //set pin mode input for Analog A13
    DDRK &= ~(1 << DDK5);
    
    //set sensor
    DDRB |= (0<<DDB3);
    PORTB |= (1<<PORTB3);
    
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
        rx_servo = readADC(X_servo);
        ry_servo =  readADC(Y_servo);
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
               //backward
               Bluetooth_Send_Byte('b');
               //Roomba_Drive(100, 32768);
             }else if(ry<300){
               //foward  
               Bluetooth_Send_Byte('f');
               //Roomba_Drive(-100, 32768);
             }
         }
        
        
        //servo

        if(rx_servo<300){
               //left
                Bluetooth_Send_Byte('z');
             }else if(rx_servo>700){
               //right 
                //Roomba_Drive(50, -1);
                Bluetooth_Send_Byte('y');
             }
             if(ry_servo>800){
               //backward
               Bluetooth_Send_Byte('h');
             }else if(ry_servo<300){
                //foward  
               Bluetooth_Send_Byte('q');
             }
        
    
            laser = (PINB & _BV(PB3)) ? '0' : '1' ;
            Bluetooth_Send_Byte(laser);
    }
    
        

}



void dummy(){
    for(;;){
        
    }
}

int main() 
{
   //Roomba_Init();
   init_uart_bt();
   //uart_init();
   //stdout = &uart_output;
   //stdin = &uart_input;
   //cli();
   //OS_Init();
   config();
    //read laser
    /*
    for(;;){
        laser = (PINB & _BV(PB3)) ? 0 : 1 ;
        printf("laser value %d\n", laser);
    }*/
    
    //Bluetooth_Send_Byte(laser);
   RommbaControl();
   //Task_Create_System(RommbaControl, 1);
   //Task_Create_RR(dummy,1);
   //Task_Create_Period(ServoControl, 1, 4, 1, 0);
   //setupTimer();
   //sei();
   //OS_Start();
   return 0;
}