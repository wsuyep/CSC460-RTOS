/*
 * Exercise_2.cpp
 *
 * Created: 1/13/2016 4:41:59 PM
 * Author : gcc91_000
 */ 


#define EOT (4)

extern "C"
{
	#include "main.h"
	#include "lcd_drv.h"
}
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include "usart.h"


int main(void)
{
	USART0_Init(MYUBRR);
	USART1_Init(MYUBRR);
	int i;
	char sendBuff[31];	
	char recvBuff[31];

	lcd_init();
	DDRC |= (1<<7); //set pin 30 to output mode
	DDRC |= (1<<6); //set pin 31 to output mode
	
	lcd_xy(0, 0);

	while(1)
	{		
		PORTC |= (1<<7); //set pin 30 to high
		lcd_puts((void*)"1234567890ABCDEF1234567890ABCDEF");
		PORTC &= ~(1<<7); //set pin 30 to low
	
		PORTC |= (1<<6); //set pin 31 to high
		USART0_Send_string("1234567890ABCDEF1234567890ABCDEF");
		PORTC &= ~(1<<6); //set pin 31 to low
		
		//// initialize the send buffer
		//for (i = 0; i < 30; i++)
		//{
			//sendBuff[i] = '*';
		//}
		
		//i = 0;
		//
		//for(;;)
		//{
			//sendBuff[i] = USART0_Receive(); // try to receive until EOF is received
			//
			//if(sendBuff[i] == '\r'){
				//USART0_Send_string("\\r received, replaced to \\n\n");
				//sendBuff[i] = '\n';
			//}
			//
			//if(sendBuff[i] == EOT){
				//sendBuff[i] = '\0';
				//break;
			//}
			//i++;
		//}
		//
		//i = 0;
		//
		//for(;;)
		//{
			//USART1_Sendbyte(sendBuff[i]);
			//recvBuff[i] = USART1_Receive();
			//if(recvBuff[i] == '\0')
			//{
				//i++;
				//break;
			//}
			//i++;
		//}
		//
		//USART0_Send_string(recvBuff);
        ////USART0_Send_string(sendBuff);
		//sprintf(sendBuff, "Total: %d bytes sent and received.\n", i);
		//USART0_Send_string(sendBuff);
		////USART0_Send_string("New version\n");
		//USART0_Send_string("1234567890ABCDEF1234567890ABCDEF");
		_delay_ms(1000);
	}
}