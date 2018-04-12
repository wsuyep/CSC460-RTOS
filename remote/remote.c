#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdio.h>
#include "./uart/uart.h"
#include "./roomba/roomba.h"
#include "avr_console.h"
#include <time.h>
#include "os.h"

int curr_state = 0;
TICK last_tick=0;
TICK curr_ticks=0;
TICK cumulative_ticks=0;
TICK tick_thresh = 3000;

TICK sixty_sec_timer = 0;
TICK last_switch = 0;
TICK switch_period = 1000;
int mode = 0; // 0 = full control, 1 = autonomous

void autonomous_spin(){
    Roomba_Drive(200,1);
}

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
    DDRK &= ~(1 << DDK0);
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

void switchMode(){
    if(mode == 0){
        mode = 1;

    }else{
        mode = 0;
    }
}

void Servo_Init() {
    // Set PORTL Pin 5 (Digital Pin 44) as output
    DDRL |= (1 << DDL5);
    // Set PORTL Pin 4 (Digital Pin 45) as output
    DDRL |= (1 << DDL4);
    // Set PWM, Phase Correct, 10-bit; Top == 0x3FF; Update of OCR5 at TOP; TOV Flag Set On BOTTOM;
    TCCR5A |= (1 << WGM51) | (1 << WGM50);
    TCCR5B |= (0 << WGM52);
    // Clear OC5C on Compare Match when up-counting. Set OC5C on Compare Match when down-counting
    TCCR5A |= (1 << COM5C1) | (0 << COM5C0);
    // Clear OC5B on Compare Match when up-counting. Set OC5B on Compare Match when down-counting
    TCCR5A |= (1 << COM5B1) | (0 << COM5B0);
    // clk(I/O)/(256); period = 0.0327439423706614 second
    TCCR5B |= (1 << CS52) | (0 << CS51) | (0 << CS50);
    // Initialize base motor (Digital Pin 44) Lower bound: 16; Upper bound: 30;
    OCR5C = 16;
    // Initialize arm motor (Digital Pin 45) Lower bound: 39; Upper bound: 59; Middle: 49
    OCR5B = 49;
}

void Servo_Drive_X(int dir){
    // dir 0 == left, dir 1 == right
    if(dir == 0 && OCR5B>40){
        OCR5B -= 1;
    }else if(dir == 1&& OCR5B<58){
        OCR5B += 1;
    }
}

void Servo_Drive_Y(int dir){
    // dir 0 == down, dir 1 == up
    if(dir == 0 && OCR5C>16){
        OCR5C -= 1;
    }else if(dir == 1&& OCR5C<30){
        OCR5C += 1;
    }
}

void readRoombaSensor(){
    roomba_sensor_data_t packet;
    Roomba_UpdateSensorPacket(EXTERNAL, &packet);
    _delay_ms(30);
    printf("bumper sensor %d\n", packet.bumps_wheeldrops);
    if(packet.bumps_wheeldrops == 1){
             //backward
             Roomba_Drive(-100,32768);
             _delay_ms(500);
    }
    
}

void auto_drive(){
    readRoombaSensor();
    Roomba_Drive(50,1);
    Roomba_Drive(100, 50);
    
}

void checkTimer(){
    curr_ticks = getTicks();
    if(curr_state == 1 && (cumulative_ticks+(curr_ticks-last_tick)) > tick_thresh){
        PORTA = 0b00000000;
        curr_state = 0;
        cumulative_ticks += (curr_ticks-last_tick);
        last_tick = curr_ticks;
    }

    if((curr_ticks % switch_period) < 50 && (last_switch < curr_ticks - 1000)){
        last_switch = curr_ticks;
        switchMode();
    }
}

void receive_byte(){
     unsigned char roomba_x;
     unsigned char roomba_y;
     for(;;){
        checkTimer();
        unsigned char x = Bluetooth_Receive_Byte();
        if(mode == 0){
            switch(x){
                case('l'):
                    //left
                    Roomba_Drive(50,1);
                    break;
                case('f'):
                    //forward
                    //printf("received: %c",x);
                    Roomba_Drive(100,32768);
                    break;
                case('b'):
                    //backward
                    Roomba_Drive(-100,32768);
                    break;
                case('r'):
                    //right
                    Roomba_Drive(50,-1);
                    break;
                case('s'):
                    //stop
                    //Roomba_Drive(0,0);
                    auto_drive();
                    break;

                //servo cases
                // left
                case('z'):
                    Servo_Drive_X(1);
                    break;
                // right
                case('y'):
                    Servo_Drive_X(0);
                    break;
                // up
                case('q'):
                    Servo_Drive_Y(1);
                    break;
                // down
                case('h'):
                    Servo_Drive_Y(0);
                    break;

                // laser
                case('1'):
                    curr_ticks = getTicks();
                    if(curr_state == 0 && curr_ticks>(last_tick+20) && cumulative_ticks<tick_thresh){
                        PORTA = 0b00000001;
                        curr_state = 1;
                        last_tick = curr_ticks;
                    }else if(curr_state == 1 && curr_ticks>(last_tick+20)){
                        PORTA = 0b00000000;
                        curr_state = 0;
                        cumulative_ticks += (curr_ticks-last_tick);
                        last_tick = curr_ticks;
                    }
                    break;
            }
        }else{
            autonomous_spin();
        }
        

     //     roomba_x = Bluetooth_Receive_Byte();
     //     printf("rommba data x %d\n", roomba_x);
     //     roomba_y = Bluetooth_Receive_Byte();
     //     printf("rommba data y %d\n", roomba_y);
     }
     
}




void dummy(){
    for(;;){

    }
}



int main(){
    //uart_init();
    //stdout = &uart_output;
    //stdin = &uart_input;
    //cli();
    config();

    setupTimer();
    DDRA = 0xff;
    Roomba_Init();
    // init_uart_bt();
    Servo_Init();
    receive_byte();
    // cli();
    // OS_Init();
    // setupTimer();
    // Task_Create_Period(receive_byte,1,10,1,0);
    // Task_Create_RR(dummy,1);
    // sei();
    // OS_Start();
    return -1;

}
