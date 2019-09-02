#include "uart.h"

#define MAX_G_BUFF_SIZE 32

static Sink sUartSink = 0;

Sink UartInit(Task task)
{
	if(!(sUartSink = StreamUartSink()))
		return 0;
		
	/* Configure sink not to send MESSAGE_MORE_SPACE */
	PanicFalse(SinkConfigure(sUartSink, VM_SINK_MESSAGES, VM_MESSAGES_NONE));
	
	StreamConfigure(VM_STREAM_UART_CONFIG, VM_STREAM_UART_LATENCY);
	
	MessageSinkTask(StreamUartSink(), task);
	return sUartSink;
}

bool UartSendData(uint8 *data, uint16 size)
{
	if(!sUartSink)
	{	
		return FALSE;
	}
	
	if (!data || size == 0)
	{
		return FALSE;
	}
	
#ifdef DEBUG_UART
	{
		uint16 i;
		printf("Uart Send:");
		for(i=0;i<size;i++){
			if(*(data+i)=='\r')
				printf("\\r");
			else if(*(data+i)=='\n')
				printf("\\n");
		else
				printf("%c",*(data+i));
		}
		printf("\n");
	}
#endif

	if(SinkClaim(sUartSink, size) != 0xFFFF)
	{
		memmove(SinkMap(sUartSink), data, size);
		(void) PanicZero(SinkFlush(sUartSink, size));

		return TRUE;
	}
	return FALSE;
}

bool UartSendChar(uint8 c ) 
{ 
	if(!sUartSink) 
	{
		return FALSE;
	}
	
	if(SinkClaim(sUartSink, 1) != 0xFFFF) 
    { 
        memcpy(SinkMap(sUartSink), &c, 1);
		(void) PanicZero(SinkFlush(sUartSink, 1)); 
        return TRUE;
    } 
	
	return FALSE;
}

bool UartSendStr(char* str)
{
	if(!str)
		return FALSE;
		
	if(strlen(str) > 0)
		return UartSendData((uint8*)str, strlen(str));	

	return FALSE;
}

bool UartSendDigit(uint32 i)	
{
	uint8 at_buff[MAX_G_BUFF_SIZE] = {0};
	uint8 x=0,j=0,y;
	if (i==0) {	
		at_buff[j]='0';
		j++;
	}
	else while (i>0) {
		at_buff[j]=i%10;	
		at_buff[j]+='0';
		i/=10; j++;
	}
	at_buff[j]='\0';	
	i=j;		/* Keep the length of data */
	if (j>0) {
		j--;
		while (j>x) {		/* reverse */
			y=at_buff[j];
			at_buff[j]=at_buff[x];
			at_buff[x]=y;
			j--;x++;
		}
	}

	return UartSendData( at_buff, i);
}

bool UartSendHex( uint32 i)
{
	uint8 at_buff[MAX_G_BUFF_SIZE] = {0};
	uint8 x=0,j=0,y;
	if (i==0) {
		at_buff[j]='0';
		j++;
	}
	else while (i>0) {
		at_buff[j]=i&0xf;
		at_buff[j]=(at_buff[j]>9)?at_buff[j]+'A'-10:at_buff[j]+'0';
		i>>=4; j++;
	}
	at_buff[j]='\0'; 
	i=j;		/* Keep the length of data */
	if (j>0) {
		j--;
		while (j>x) {		/* reverse */
			y=at_buff[j];
			at_buff[j]=at_buff[x];
			at_buff[x]=y;
			j--;x++;
		}
	}

	return UartSendData(at_buff, i);
}

bool UartSendBDAddr(bdaddr * addr)
{
	uint8 at_buff[MAX_G_BUFF_SIZE] = {0};
	if(!addr)
		return FALSE;

	/*bdToString(addr, (uint8 *)at_buff);*/

	return UartSendData((uint8 *)at_buff, strlen((const char *)at_buff));
}

bool UartSendStringNew(char *s, uint16 len) 
{ 
    uint16 i;	
    if(len == 0 || !s)
    	return FALSE;
    	
    for(i=0;i<len;i++)
    {
	   if(!UartSendChar((uint8)s[i]))
      		return FALSE; 	
    }  

    return TRUE;
}
	
