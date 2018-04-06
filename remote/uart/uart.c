/**
 * @file   uart.c
 * @author Justin Tanner
 * @date   Sat Nov 22 21:32:03 2008
 *
 * @brief  UART Driver targetted for the AT90USB1287
 *
 */
#include "uart.h"


#define F_CPU 16000000

#ifndef F_CPU
#warning "F_CPU not defined for uart.c."
#define F_CPU 11059200UL
#endif

#define BAUD 115200
#define UBRR_VALUE (((F_CPU / (BAUD * 16UL))) - 1)

	// This needs to be defined, but is never used directly.
#include <util/setbaud.h>


static volatile uint8_t uart_buffer[UART_BUFFER_SIZE];
static volatile uint8_t uart_buffer_index;

void init_uart_roomba(UART_BPS baud){
    
    /*
    UBRR0H = 0;	// for any speed >= 9600 bps, the UBBR value fits in the low byte.
	UCSR0A = _BV(U2X1);
	UCSR0B = _BV(RXEN0) | _BV(TXEN0) | _BV(RXCIE0);
	UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);
    */
    
    /*
    UBRR2H = 0b0000;		// This is for 19200 Baud.
	UBRR2L = 0b00110011;	// This is for 19200 Baud.
	
	// Clear USART Transmit complete flag, normal USART transmission speed
	UCSR2A = (1 << TXC2) | (0 << U2X2);

	// Enable receiver, transmitter, rx complete interrupt and tx complete interrupt.
	UCSR2B = (1 << RXEN2) | (1 << TXEN2) | (1 << RXCIE1);

	// 8-bit data
	UCSR2C = ((1 << UCSZ21) | (1 << UCSZ20));

	// Disable 2x speed
    UCSR2A &= ~(1 << U2X2);
    */
    
    UBRR0H = 0;
    
    // Enable receiver, transmitter
    UCSR0B = (1<<RXEN0) | (1<<TXEN0)|_BV(RXCIE0);

    // 8-bit data
    UCSR0C = ((1<<UCSZ01)|(1<<UCSZ00));

    // disable 2x speed
    UCSR0A = _BV(U2X1);
}


/**
 * Transmit one byte
 * NOTE: This function uses busy waiting
 *
 * @param byte data to trasmit
 */
void uart_send_byte(uint8_t byte)
{
    /* wait for empty transmit buffer */
    while (!( UCSR0A & (1 << UDRE0)));

    /* Put data into buffer, sends the data */
    UDR0 = byte;
}

/**
 * Receive a single byte from the receive buffer
 *
 * @param index
 *
 * @return
 */
uint8_t uart_get_byte(int index)
{
    if (index < UART_BUFFER_SIZE)
    {
        return uart_buffer[index];
    }
    return 0;
}

/**
 * Get the number of bytes received on UART
 *
 * @return number of bytes received on UART
 */
uint8_t uart_bytes_received(void)
{
    return uart_buffer_index;
}

/**
 * Prepares UART to receive another payload
 *
 */
void uart_reset_receive(void)
{
    uart_buffer_index = 0;
}

/**
 * UART receive byte ISR
 */
ISR(USART0_RX_vect)
{
	while(!(UCSR0A & (1<<RXC0)));
    uart_buffer[uart_buffer_index] = UDR0;
    uart_buffer_index = (uart_buffer_index + 1) % UART_BUFFER_SIZE;
}

void uart_print(uint8_t* output, int size)
{
	uint8_t i;
	for (i = 0; i < size && output[i] != 0; i++)
	{
		uart_send_byte(output[i]);
	}
}

void init_uart_bt(){
    // Enable receiver, transmitter
    UCSR1B = (1<<RXEN1) | (1<<TXEN1);

    // 8-bit data
    UCSR1C = ((1<<UCSZ11)|(1<<UCSZ10));

    // disable 2x speed
    UCSR1A &= ~(1<<U2X1);
}

void Bluetooth_Send_Byte(uint8_t data_out){
    while(!( UCSR1A & (1<<UDRE1)));
    // Put data into buffer
    UDR1 = data_out;
}

unsigned char Bluetooth_Receive_Byte(){      
    // Wait for data to be received
    while(!( UCSR1A & (1<<RXC1)));
    // Get and return data from buffer
    return UDR1;
}




