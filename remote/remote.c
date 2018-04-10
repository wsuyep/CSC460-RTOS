#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdio.h>
#include "./uart/uart.h"
#include "./roomba/roomba.h"
//#include "avr_console.h"


void Servo_Init() {
	// Setup ports and timers
    DDRA = 0xFF; // All output
    PORTA = 0;

    // Configure timer/counter1 as phase and frequency PWM mode
    TCNT4 = 0;
    TCCR4A = (1<<COM4A1) | (1<<COM4B1) | (1<<WGM41);  //NON Inverted PWM
    TCCR4B |= (1<<WGM43) | (1<<WGM42) | (1<<CS41) | (1<<CS40); //PRESCALER=64 MODE 14 (FAST PWM)
    ICR4 = 4999;

    OCR4A = 375; // 90 Degrees
}

void receive_byte(){
     unsigned char roomba_x;
     unsigned char roomba_y;
     for(;;){
        uint8_t x = Bluetooth_Receive_Byte();
        switch(x){
            case(108):
            case(139):
                //left
                Roomba_Drive(50,1);
                break;
            case(41):
            case(249):
                //forward
                Roomba_Drive(-100,32768);
                break;
            case(137):
            case(44):
                //backward
                Roomba_Drive(100,32768);
                break;
            case(46):
            case(201):
                //right
                Roomba_Drive(50,-1);
                break;
            case(205):
            case(174):
                //stop
                Roomba_Drive(0,0);
                break;
        }

     //     roomba_x = Bluetooth_Receive_Byte();
     //     printf("rommba data x %d\n", roomba_x);
     //     roomba_y = Bluetooth_Receive_Byte();
     //     printf("rommba data y %d\n", roomba_y);
     }
     
}



int main(){
    Roomba_Init();
    init_uart_bt();
    //Servo_Init();
    //uart_init();
    //stdout = &uart_output;
    //stdin = &uart_input;
    
    //DDRB=0x83;
    //OS_Init();
    receive_byte();
    //Task_Create_System(receive_byte,1);
    //sei();
    //OS_Start();
    return -1;

}
