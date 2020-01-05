
#include <message.h>
#include <pio.h>
/*#include "uart.h"*/
#include "sink_events.h"
#include "sink_config.h"
#include "sink_configmanager.h"
#include "sink_a2dp.h"
#include "sink_custom_uart.h"
#include "sink_parse.h"

#define MESSAGE_CUSTOM_TEST 0x0001

UARTStreamTaskData theUARTTask;


void app_uart_handler(Task t, MessageId id, Message payload)
{
    UARTStreamTaskData *app = &theUARTTask;

    switch(id)
    {
        case MESSAGE_MORE_DATA:
        {
            Source src=StreamSourceFromSink(app->uart_sink);
            const uint8 * s = (uint8*)SourceMap(src);
            const uint8 * e = s + SourceSize(src);
            const uint8 * p = NULL;           
            /*app->pUartSrcStart = (uint8*)s;
            app->pUartSrcEnd = (uint8*)e;
            app->uart_src_need_drop = FALSE;*/
		#if 0
            /*UartSendData(app->pUartSrcStart , SourceSize(src));
            UartSendStr("\r\n");
            UartSendStr("MESSAGE_MORE_DATA\r\n");
            if(0<(-state) && (-state)<61)
                theUARTTask.uart_src_need_drop = TRUE;*/
            p = UartparseData(s, e, &app->task);
            if(app->uart_src_need_drop){
                SourceDrop(src, SourceSize(src));
            }
		#else
            p = UartparseData(s, e, &app->task);
            SourceDrop(src, p-s);

		#endif
         }
            break;
         case MESSAGE_CUSTOM_TEST:
         {
            /*UartSendStr("MESSAGE_CUSTOM_TEST\r\n");*/
			/*MessageSendLater(theUARTTask.client, EventUsrBleStartBonding, NULL, 2000);
			sink_custom_ble_adv();
            MessageSendLater(&theUARTTask.task, MESSAGE_CUSTOM_TEST, NULL, 1000);*/
         }
            break;
         default:
         {
            UartSendStr("ERROR\r\n");
         }
            break;
    }
}

void custom_uart_init(Task client)
{
    /*UartSendStr("custom_uart_init\r\n");*/
    theUARTTask.task.handler = app_uart_handler;
    theUARTTask.client = client;
    theUARTTask.uart_sink = UartInit(&theUARTTask.task);
    /*MessageSend(&theUARTTask.task, MESSAGE_CUSTOM_TEST, 0);
	MessageSendLater(&theUARTTask.task, MESSAGE_CUSTOM_TEST, NULL, 5000);*/
}

void UarthandleUnrecognised(const uint8 *data, uint16 length, Task task)
{
	UartSendStr("ERROR\r\n");
}

void UartSendInt(int32 data)
{
	if(data<0){
		data = ~data +1;
		UartSendStr("-");
	}
	
	UartSendDigit((uint32)data);

}


