#include <message.h>
#include "main.h"
#include <bdaddr.h>
#include <ps.h>
#include <string.h>
#include "sink_statemanager.h"
#include "sink_parse.h"
#include <uart.h>
#include <gaia.h>
#include "sink_private.h"
#include "sink_gaia.h"
#include "sink_ble_gap.h"
#include "sink_gatt_db.h"
#include "sink_gatt_server.h"

#ifdef DEBUG_CUSTOM
#define CUSTOM_DEBUG(x) UartSendStr(x)
#else
#define CUSTOM_DEBUG(x)
#endif

void custom_app_handle(Task task, MessageId id, Message message);

#define MAX_G_BUFF_SIZE  32
#define PS_VERSION_NUM                 13
#define FIRMWARE_VERSION_NUM    "0.0.2"

#define PSKEY_BDADDR    0x0001
#define PSKEY_LOCAL_DEVICE_NAME 0x0108
#define PS_DEVICE_CLASS 0x0003
#define PS_UART_BAUDRATE 0x01ea

custom_task theCustomTask;
/*= {
    .custom_init = _custom_init,
    .custom_deinit = _custom_deinit,
};*/

void custom_app_handle(Task task, MessageId id, Message message)
{
    CUSTOM_DEBUG(("custom_app_handle init\n"));
}
int _custom_init(void)
{
    CUSTOM_DEBUG(("custom init\n"));
    theCustomTask.task.handler = custom_app_handle;
    theCustomTask.clientTask = &theSink.task;

    return 0;       
}

int _custom_deinit(void)
{
    CUSTOM_DEBUG(("custom deinit\n"));

    return 0; 
}

int IntToStr(char *dst, uint32 src, int len)
{
    int i = len;
    int j = 0;
    if(i>8)
        return 0;
    while (i-- > 0){
		*(dst+j)=(src>>(i*4))&0xf;
		*(dst+j)=(*(dst+j)>9)?*(dst+j)+'A'-10:*(dst+j)+'0';
		j++;
	}

    return len;
}

int StrToInt(uint32 *dst, char *src, int len)
{
    int i = len;
    int j = 0;
    uint32 tmp = 0;
    
    *dst = 0;
    while (i-- > 0) {
        if(!((*(src+j)>='0' && *(src+j)<='9') || (*(src+j)>='A' && *(src+j)<='F') || (*(src+j)>='a' && *(src+j)<='f')))
            return j;
        if((*(src+j)>='a' && *(src+j)<='f'))
            *(src+j) = *(src+j) - ('a' - 'A');        
        tmp = (*(src+j)>'9')?*(src+j)-'A'+10:*(src+j)-'0';
		(*dst)=(*dst)|(tmp<<(i*4));
		j++;
	}

    return len;
}

int StrToAddr(char *str, bdaddr *addr)
{
    if(12 != strlen(str))
        return FALSE;
    StrToInt(&addr->lap, str, 6);
    StrToInt((uint32 *)&addr->uap, str+6, 2);
    StrToInt((uint32 *)&addr->nap, str+8, 4);

    return TRUE;
}
int AddrToStr(char *str, bdaddr *addr)
{
    if(0==addr->lap && 0==addr->uap && 0==addr->nap)
        return FALSE;
	
    IntToStr(str, addr->lap, 6);
    IntToStr(str+6, addr->uap, 2);
    IntToStr(str+8, addr->nap, 4);
    str[12] = '\0';

    return TRUE;
}

/*general at cmd functions*/
void handleGetVersion(Task t)
{
	/*uint8 len = 0;
	char at_buff[MAX_G_BUFF_SIZE] = "v0.0.1";
	memset(at_buff, 0, MAX_G_BUFF_SIZE);*/
		
	UartSendStr("+VER:");
	UartSendStr(FIRMWARE_VERSION_NUM);
	UartSendStr("\r\n");		
}

void handleGetLocalAddr(Task t)
{
    bdaddr local_addr;
    char * at_buff = (char *)mallocPanic(13);

    if(4 == PsFullRetrieve(PSKEY_BDADDR, (uint16 *)&local_addr, 4))
    {
    	IntToStr(at_buff, local_addr.lap, 6);
        IntToStr(at_buff+6, local_addr.uap, 2);
        IntToStr(at_buff+8, local_addr.nap, 4);
        at_buff[12] = '\0';
        UartSendStr("+ADDR:");
        UartSendStr(at_buff);
        UartSendStr("\r\n");
    }
    else
    {
        UartSendStr("ERROR\r\n");
    }
     freePanic(at_buff);
    /*getLocalBdAddrFromPs*/
}
void handleSetLocalAddr(Task t, const struct SetLocalAddr *addr)
{
    bdaddr local_addr;
    StrToAddr((char *)addr->addr.data, &local_addr);

    if(4 == PsStore(PSKEY_BDADDR, (uint16 *)&local_addr, 4))
    {
        UartSendStr("OK\r\n");
    }
    else
    {
        UartSendStr("ERROR\r\n");
    }
}
void handleGetLocalDevName(Task t)
{
    uint16 *retrieve_buff = (uint16 *)mallocPanic(2 * 16);
    char *at_buff = (char *)mallocPanic(33);
    uint16 i = 0;
    uint16 j = 0;

    if(PsFullRetrieve(PSKEY_LOCAL_DEVICE_NAME, (uint16 *)retrieve_buff, 16)){
        for(i=0;i<16;i++){
            if(0 != (retrieve_buff[i] & 0xff))
                at_buff[j++] = retrieve_buff[i] & 0xff;
            else
                break;
            
            if(0 != ((retrieve_buff[i]>>8) & 0xff))
                at_buff[j++] = (retrieve_buff[i]>>8) & 0xff;
            else
                break;
        }
        at_buff[j] = '\0';
        
        UartSendStr("+NAME:");
        UartSendStr(at_buff);
        UartSendStr("\r\n");
    }else{
        UartSendStr("\r\nERROR\r\n");
    }
    freePanic(at_buff);
    freePanic(retrieve_buff);
}

void handleSetLocalDevName(Task t , const struct SetLocalDevName * Name)
{
    uint16 *retri_buff = (uint16 *)mallocPanic(2 * 16);
    uint16 i = 0;
    uint16 j = 0;
    uint16 len = 0;
    if(Name->DevName.length > 32)
        len = 32;
    else
        len = Name->DevName.length;
    
    for(i=0;i<(len+1)/2;i++){
        if((len%2) && (i == (len+1)/2))
            retri_buff[i] = Name->DevName.data[j];
        else
            retri_buff[i] = Name->DevName.data[j] | Name->DevName.data[j+1]<<8;

        j+=2;
    }

    if(PsStore(PSKEY_LOCAL_DEVICE_NAME, retri_buff, i)){
        UartSendStr("OK\r\n");
    }else{
        UartSendStr("ERROR\r\n");
    }
    
    freePanic(retri_buff);
}
void handleGetPin(Task t){}
void handleSetPin(Task t){}
void handleGetDevClass(Task t)
{
    uint32 device_class = 0 ;
    char at_buff[7];

    if(2 == PsFullRetrieve(PS_DEVICE_CLASS, (uint16 *)&device_class, 2))
    {
        IntToStr(at_buff, device_class, 6);
        at_buff[6] = '\0';
        UartSendStr("+CLASS:");
        UartSendStr(at_buff);
        UartSendStr("\r\n");
    }else{
        UartSendStr("ERROR\r\n");
    }
}
void handleSetDevClass(Task t, const struct SetDevClass *dev_class)
{
    uint32 device_class = 0 ;

    if(6 != IntToStr((char *)dev_class->cls.data, device_class, 6));
        UartSendStr("ERROR\r\n");
    if(2 == PsStore(PS_DEVICE_CLASS, (uint16 *)&device_class, 2))
    {
        UartSendStr("OK\r\n");
    }else{
        UartSendStr("ERROR\r\n");
    }
}
void handlePairDev(Task t, const struct PairDev *dut_addr){}
void handleEnterPair(Task t)
{
    MessageSend(theCustomTask.clientTask, EventUsrEnterPairing, 0);
    UartSendStr("OK\r\n");
}
void handleSetScanMode(Task t, const struct SetScanMode *mode){}
void handleGetUartBaudrate(Task t)
{
    uint32 baudrate = 0 ;

    if(2 == PsFullRetrieve(PS_UART_BAUDRATE, (uint16 *)&baudrate, 2))
    {
        UartSendStr("+UART:");
        UartSendDigit(baudrate);
        UartSendStr("\r\n");
    }else{
        UartSendStr("ERROR\r\n");
    }
}
void handleSetUartBaudrate(Task t, const struct SetUartBaudrate *baudrate){}
void handleDiscoverNearDev(Task t, const struct DiscoverNearDev *mode)
{
	if(mode->start){
		if(theSink.inquiry.action == rssi_none){
			MessageSend(theCustomTask.clientTask, EventUsrRssiPair, 0);
			UartSendStr("OK\r\n");
		}else{
			UartSendStr("ERROR\r\n");
		}
	}else{
		inquiryStop();
		UartSendStr("OK\r\n");
	}
}

/*hfp at cmd functions*/
void handleHfpConn(Task t, const struct HfpConn *dut_addr)
{
    bdaddr local_addr;
    
    if(0 == dut_addr->addr.length){
		
    }else{
        StrToAddr((char *)dut_addr->addr.data, &local_addr);
		HfpSlcConnectRequest(&local_addr,0,0);
    }
}
void handleHfpDisc(Task t)
{
	HfpSlcDisconnectRequest(hfp_primary_link);
	UartSendStr("OK\r\n");
}
void handleHfpGetStatus(Task t)
{
	sinkState sta = stateManagerGetState();

	UartSendStr("+HFSTA:");
	UartSendDigit(sta);
	UartSendStr("\r\n");
}
void handleHfpMute(Task t, const struct HfpMute *mute)
{
	if(0 == mute->mute){
		MessageSend(theCustomTask.clientTask, EventUsrMicrophoneMuteOff, 0);
		UartSendStr("+MUTE:1\r\n");
	}else if(1 == mute->mute){
		MessageSend(theCustomTask.clientTask, EventUsrMicrophoneMuteOn, 0);
		UartSendStr("+MUTE:0\r\n");
	}else{
		UartSendStr("ERROR\r\n");
	}

}
void handleHfpDial(Task t, const struct HfpDial *num)
{
    if(0 == num->num.length){
        MessageSend(theCustomTask.clientTask, EventUsrLastNumberRedial, 0);
    }else{
        uint8 phone_number_key[20];
        int i = 0;
        Sink sink;
        
        for(i=0;i<20;i++){
            StrToInt((uint32 *)(phone_number_key+i), (char *)(num->num.data+i), 1);
        }
        if((HfpLinkGetSlcSink(hfp_primary_link, &sink)) && SinkIsValid(sink))
        { 
            HfpDialNumberRequest(hfp_primary_link, num->num.length, phone_number_key);  
        }else{
            UartSendStr("ERROR\r\n");
        }
    }
    UartSendStr("OK\r\n");
}
void handleHfpAnswer(Task t)
{
    MessageSend(theCustomTask.clientTask, EventUsrAnswer, 0);
    
    UartSendStr("OK\r\n");
}
void handleHfpHangUp(Task t)
{
    sinkState m_state;
    
    m_state = stateManagerGetState();
    
    if(deviceIncomingCallEstablish == m_state){
        MessageSend(theCustomTask.clientTask, EventUsrReject, 0);
    }else if(deviceIncomingCallEstablish == m_state || deviceActiveCallSCO == m_state){
        MessageSend(theCustomTask.clientTask, EventUsrCancelEnd, 0);
    }
    UartSendStr("OK\r\n");
}
void handleThreeWayReleaseAllHeld(Task t)
{
    MessageSend(theCustomTask.clientTask, EventUsrThreeWayReleaseAllHeld, 0);
    
    UartSendStr("OK\r\n");
}

void handleThreeWayAddHeldTo3Way(Task t)
{
    MessageSend(theCustomTask.clientTask, EventUsrThreeWayAddHeldTo3Way, 0);
    
    UartSendStr("OK\r\n");
}

void handleThreeWayAcceptWaitingHoldActive(Task t)
{
    MessageSend(theCustomTask.clientTask, EventUsrThreeWayAcceptWaitingHoldActive, 0);
    
    UartSendStr("OK\r\n");
}

void handleThreeWayAcceptWaitingReleaseActive(Task t)
{
    MessageSend(theCustomTask.clientTask, EventUsrThreeWayAcceptWaitingReleaseActive, 0);
    
    UartSendStr("OK\r\n");
}
/*a2dp at cmd functions*/
void handleA2dpGetStatus(Task t)
{
	a2dp_signalling_state sta;

	if(theSink.a2dp_link_data){
		UartSendStr("OK\r\n");
	}else{
		UartSendStr("ERRORr\n");
		return;
	}
	sta = A2dpSignallingGetState(0);

	UartSendStr("+A2DPSTA:");
	UartSendDigit(sta);
	UartSendStr("\r\n");
}

void handleA2dpGetMediaStatus(Task t)
{
	a2dp_stream_state sta;

	if(theSink.a2dp_link_data){
		UartSendStr("OK\r\n");
	}else{
		UartSendStr("ERRORr\n");
		return;
	}
	sta = A2dpMediaGetState(0,0);

	UartSendStr("+A2DPMDSTA:");
	UartSendDigit(sta);
	UartSendStr("\r\n");
}
void handleA2dpConn(Task t, const struct A2dpConn *dut_addr)
{
    bdaddr m_addr;
    if(StrToAddr((char * )dut_addr->addr.data, &m_addr))
        A2dpSignallingConnectRequest(&m_addr);
}
void handleA2dpDisc(Task t)
{
    A2dpSignallingDisconnectRequest(0);
}

/*avrcp at cmd functions*/
void handleAvrcpGetStatus(Task t)
{
	if(theSink.avrcp_link_data)
		UartSendStr("OK\r\n");
	else
		UartSendStr("ERRORr\n");
	
	if(theSink.avrcp_link_data->connected)
		UartSendStr("+AVRCPSTA:1\r\n");
	else
		UartSendStr("+AVRCPSTA:0\r\n");
}
void handleAvrcpNext(Task t)
{
	MessageSend(theCustomTask.clientTask, EventUsrAvrcpSkipForward, 0);
    
    UartSendStr("OK\r\n");
}
void handleAvrcpPrev(Task t)
{
	MessageSend(theCustomTask.clientTask, EventUsrAvrcpSkipBackward, 0);
    
    UartSendStr("OK\r\n");
}
void handleAvrcpStop(Task t)
{
	MessageSend(theCustomTask.clientTask, EventUsrAvrcpStop, 0);
    
    UartSendStr("OK\r\n");
}
void handleAvrcpPlay(Task t)
{
	MessageSend(theCustomTask.clientTask, EventUsrAvrcpPlay, 0);
    
    UartSendStr("OK\r\n");
}
void handleAvrcpPause(Task t)
{
	MessageSend(theCustomTask.clientTask, EventUsrAvrcpPause, 0);
    
    UartSendStr("OK\r\n");
}
void handleAvrcpVolUp(Task t)
{
	MessageSend(theCustomTask.clientTask, EventUsrMainOutVolumeUp, 0);
    
    UartSendStr("OK\r\n");
}
void handleAvrcpVolDown(Task t)
{
	MessageSend(theCustomTask.clientTask, EventUsrMainOutVolumeDown, 0);
    
    UartSendStr("OK\r\n");
}

/*pbap at cmd functions*/
void handlePbapDisc(Task t){}
void handlePbapGetStatus(Task t){}
void handlePbapConn(Task t, const struct PbapConn *dut_addr){}
/*
    pbap_telecom, 本地所有记录
    pbap_pb, 联系人
    pbap_ich, 来电记录
    pbap_och, 拨出记录
    pbap_mch, 未接电话
    pbap_cch,来电、拨出和未接全部记录
    pbap_root
*/
void handlePbapSetType(Task t, const struct PbapSetType *Type)
{
	if(Type->type < pbap_root){
		theSink.pbapc_data.pbap_active_pb = Type->type;
		theSink.pbapc_data.pbap_phone_repository = 1;
		/*MessageSend(theCustomTask.clientTask, EventUsrPbapSetPhonebook, 0);*/
		UartSendStr("OK\r\n");
	}else{
		UartSendStr("ERROR\r\n");
	}
}
void handlePbapGetPhoneBook(Task t)
{
	MessageSend(theCustomTask.clientTask, EventUsrPbapDownloadPhonebook, 0);
	UartSendStr("OK\r\n");
}

/*map at cmd functions*/
void handleMapDisc(Task t){}
void handleMapGetMessage(Task t){}
void handleMapGetStatus(Task t){}
void handleMapConn(Task t, const struct MapConn *dut_addr){}

/*hid at cmd functions*/
void handleHidDisc(Task t){}
void handleHidGetStatus(Task t){}
void handleHidConn(Task t, const struct HidConn *dut_addr){}
void handleHidKeyboardReport(Task t, const struct HidKeyboardReport *report){}
void handleHidMouseReport(Task t, const struct HidMouseReport *report){}

/*spp at cmd functions*/
void handleSppConn(Task t, const struct SppConn *dut_addr){}
void handleSppDisc(Task t){}
void handleSppGetStatus(Task t){}
void handleSppSendData(Task t, const struct SppSendData *send_data)
{
	if(stateManagerGetState() > deviceConnDiscoverable){
		gaia_send_sppdata((uint8 *)send_data->data.data, send_data->data.length);
		UartSendStr("OK\r\n");
	}else{
		UartSendStr("ERROR\r\n");
	}
}

/*gatt at cmd functions*/
void handleGattGetStatus(Task t)
{

}
void handleGattDisc(Task t)
{
	gattServerDisconnect(theSink.rundata->ble.gatt[0].cid);

	UartSendStr("OK\r\n");
}
void handleGattConn(Task t, const struct GattConn *dut_addr)
{
	
}
void handleGattAdv(Task t, const struct GattAdv *enable)
{
	/*sink_custom_ble_adv();*/
	if(enable->enable)
		MessageSend(theCustomTask.clientTask, EventUsrBleStartBonding, 0);
	else
		MessageSend(theCustomTask.clientTask, EventSysBleBondablePairingTimeout, 0);

	UartSendStr("OK\r\n");    
}
void handleGattSendData(Task t, const struct GattSendData *send_data)
{
	uint16 index = 0;
	uint16 custom_cid = theSink.rundata->ble.gatt[index].cid;
	if(stateManagerGetState() > deviceConnDiscoverable){
		GattNotificationRequest(sinkGetBleTask(), custom_cid, HANDLE_READ,\
												send_data->data.length, send_data->data.data);
		/*gaia_send_gattdata((uint8 *)send_data->data.data, send_data->data.length);*/
		UartSendStr("OK\r\n");
	}else{
		UartSendStr("ERROR\r\n");
	}
}

