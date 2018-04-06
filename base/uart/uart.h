/**
 * @file   uart.c
 * @author Justin Tanner
 * @date   Sat Nov 22 21:32:03 2008
 *
 * @brief  UART Driver targetted for the AT90USB1287
 *
 */

#ifndef __UART_H__
#define __UART_H__

#include <avr/interrupt.h>
#include <stdint.h>

typedef enum _uart_bps
{
	UART_19200,
	UART_38400,
	UART_57600,
	UART_DEFAULT,
} UART_BPS;

#define UART_BUFFER_SIZE    32

void init_uart_roomba(UART_BPS baud);
void uart_send_byte(uint8_t byte);
uint8_t uart_get_byte(int index);
uint8_t uart_bytes_received(void);
void uart_reset_receive(void);
void uart_print(uint8_t* output, int size);

void init_uart_bt();
void Bluetooth_Send_Byte(uint8_t data_out);
unsigned char Bluetooth_Receive_Byte();
#endif
