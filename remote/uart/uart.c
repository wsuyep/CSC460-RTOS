/**
 * @file   uart.c
 * @author Justin Tanner
 * @date   Sat Nov 22 21:32:03 2008
 *
 * @brief  UART Driver targetted for the AT90USB1287
 *
 */
#include "uart.h"


#define F_CPU 16000000UL

#define BAUD 19200
#define UBRR_VALUE (((F_CPU / (BAUD * 16UL))) - 1)

	// This needs to be defined, but is never used directly.
#include <util/setbaud.h>


static volatile uint8_t uart_buffer[UART_BUFFER_SIZE];
static volatile uint8_t uart_buffer_index;

void init_uart_roomba(UART_BPS baud){
    
    UBRR0H = 0;
    
    // Enable receiver, transmitter
    UCSR0B = (1<<RXEN0) | (1<<TXEN0)|_BV(RXCIE0);

    // 8-bit data
    UCSR0C = ((1<<UCSZ01)|(1<<UCSZ00));

    // disable 2x speed
    UCSR0A = _BV(U2X1);

    	switch (baud)
	{
    #if F_CPU==8000000UL
        case UART_19200:
            UBRR0L = 51;
            break;
        case UART_38400:
            UBRR0L = 25;
            break;
        case UART_57600:
            UBRR0L = 16;
            break;
        default:
            UBRR0L = 51;
    #elif F_CPU==16000000UL
        case UART_19200:
            UBRR0L = 103;
            break;
        case UART_38400:
            UBRR0L = 51;
            break;
        case UART_57600:
            UBRR0L = 34;
            break;
        default:
            UBRR0L = 103;
    #elif F_CPU==18432000UL
        case UART_19200:
            UBRR0L = 119;
            break;
        case UART_38400:
            UBRR0L = 59;
            break;
        case UART_57600:
            UBRR0L = 39;
            break;
        default:
            UBRR0L = 119;
            break;
    #else
    #warning "F_CPU undefined or not supported in uart.c."
        default:
            UBRR0L = 71;
            break;
    #endif
        }
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
ISR(USART1_RX_vect)
{
	while(!(UCSR1A & (1<<RXC1)));
    uart_buffer[uart_buffer_index] = UDR1;
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
    UCSR1B = (1<<RXEN1) | (1<<TXEN1)| _BV(RXCIE1);

    // 8-bit data
    UCSR1C = ((1<<UCSZ11)|(1<<UCSZ10));

    // disable 2x speed
    UCSR1A &= ~(1<<U2X1);

    UBRR1H = 0;
    UBRR1L = 103;
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




