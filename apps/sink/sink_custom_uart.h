#ifndef SINK_CUSTOM_UART_H
#define SINK_CUSTOM_UART_H
#include <sink.h>

void app_uart_handler(Task t, MessageId id, Message msg);
void custom_uart_init(Task client);

typedef struct
{
	TaskData task;
	Task client;
	Sink uart_sink;
    unsigned uart_src_need_drop:1;
	uint8* pUartSrcStart;
	uint8* pUartSrcEnd;
}UARTStreamTaskData;

extern UARTStreamTaskData theUARTTask;
void UartSendInt(int32 data);

#endif
