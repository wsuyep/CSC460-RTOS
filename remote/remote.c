#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdio.h>
#include "./uart/uart.h"
#include "./roomba/roomba.h"
#include "avr_console.h"


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
     uint8_t roomba_x;
     uint8_t roomba_y;
     for(;;){
         roomba_x = Bluetooth_Receive_Byte();
         printf("rommba data x %d\n", roomba_x);
         _delay_ms(1000);
         roomba_y = Bluetooth_Receive_Byte();
         printf("rommba data y %d\n", roomba_y);
     }
     
}



int main(){
    //Roomba_Init();
    //init_uart_bt();
    //Servo_Init();

    uart_init();
    stdout = &uart_output;
    stdin = &uart_input;
    
    printf("hi\n");
    //DDRB=0x83;
    //OS_Init();
    //Task_Create_System(receive_byte,1);
    //sei();
    //OS_Start();
    return -1;

}
