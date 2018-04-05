#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdio.h>
#include "./uart/uart.h"
#include "./roomba/roomba.h"

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
     
}



int main(){
    Roomba_Init();
    init_uart_bt();
    Servo_Init();
    return -1;
}
