#ifndef _UART_LIB_H_
#define _UART_LIB_H_
#include <panic.h>
#include <sink.h>
#include <message.h>
#include <message_.h>
#include <stream.h>
#include <string.h>
#include <source.h>
#include <csrtypes.h>
#include <stdio.h>

#ifdef UART_DEBUG_LIB
	#define DEBUG_UART_SEND_STR(x) UartSendStr(x)
	#define DEBUG_UART_SEND_DATA(x, y) UartSendData(x, y)
	#define DEBUG_UART_SEND_DIGIT(x) UartSendDigit(x)
	#define DEBUG_UART_SEND_BDADDR(x) UartSendBDAddr(x)
	#define DEBUG_UART_SEND_HEX(x)				UartSendHex(x)
#else
	#define DEBUG_UART_SEND_STR(x)
	#define DEBUG_UART_SEND_DATA(x,y)
	#define DEBUG_UART_SEND_DIGIT(x)
	#define DEBUG_UART_SEND_BDADDR(x)
	#define DEBUG_UART_SEND_HEX(x)
#endif

Sink UartInit(Task task);
bool UartSendChar(uint8 c);
bool UartSendStr(char* str);
bool UartSendData(uint8 *data, uint16 size);
bool UartSendBDAddr(bdaddr *bd);
bool UartSendHex( uint32 i);
bool UartSendDigit(uint32 i);
bool UartSendStringNew(char *s,uint16 len);

#endif
