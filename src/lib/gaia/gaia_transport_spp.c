/*****************************************************************
Copyright (c) 2011 - 2015 Qualcomm Technologies International, Ltd.
*/

#include "gaia.h"
#include "gaia_private.h"

#ifdef GAIA_TRANSPORT_SPP
#include <string.h>
#include <stream.h>
#include <sink.h>
#ifdef GAIA_HAVE_SPP_CLIENT
#include <sppc.h>
#endif
#include <library.h>

#include "gaia_transport.h"
#include "gaia_transport_spp.h"
#include "gaia_transport_common.h"


static const uint8 gaia_service_record[] =
{
    0x09, 0x00, 0x01,           /*  0  1  2  ServiceClassIDList(0x0001) */
    0x35,    3,                 /*  3  4     DataElSeq 3 bytes */
    0x19, 0x11, 0x01,           /*  5  6  7  UUID SerialPort(0x1101) */
    0x09, 0x00, 0x04,           /*  8  9 10  ProtocolDescriptorList(0x0004) */
    0x35,   12,                 /* 11 12     DataElSeq 12 bytes */
    0x35,    3,                 /* 13 14     DataElSeq 3 bytes */
    0x19, 0x01, 0x00,           /* 15 16 17  UUID L2CAP(0x0100) */
    0x35,    5,                 /* 18 19     DataElSeq 5 bytes */
    0x19, 0x00, 0x03,           /* 20 21 22  UUID RFCOMM(0x0003) */
    0x08, SPP_DEFAULT_CHANNEL,  /* 23 24     uint8 RFCOMM channel */
#define GAIA_SR_CH_IDX (24)
    0x09, 0x00, 0x06,           /* 25 26 27  LanguageBaseAttributeIDList(0x0006) */
    0x35,    9,                 /* 28 29     DataElSeq 9 bytes */
    0x09,  'e',  'n',           /* 30 31 32  Language: English */
    0x09, 0x00, 0x6A,           /* 33 34 35  Encoding: UTF-8 */
    0x09, 0x01, 0x00,           /* 36 37 38  ID base: 0x0100 */
    0x09, 0x01, 0x00,           /* 39 40 41  ServiceName 0x0100, base + 0 */
    0x25,   11,                 /* 42 43     String length 11 */
    'C', 'S', 'R', ' ', 'G', 'A', 'I', 'A', 
    0xE2, 0x84, 0xA2,       /* 52 53 54  U+2122, Trade Mark sign */	
    0x09, 0x00, 0x09,       /* 55 56 57  BluetoothProfileDescriptorList(0x0009) */
    0x35, 0x06,             /* 58 59     DataElSeq 3 bytes */
    0x19, 0x11, 0x01,       /* 60 61 62  UUID SerialPort(0x1101) */   
    0x09, 0x01, 0x02,       /* 63 64 65  SerialPort Version (0x0102) */
};


static const rfcomm_config_params rfcomm_config = 
{
    RFCOMM_DEFAULT_PAYLOAD_SIZE,
    RFCOMM_DEFAULT_MODEM_SIGNAL,
    RFCOMM_DEFAULT_BREAK_SIGNAL,
    RFCOMM_DEFAULT_MSC_TIMEOUT
};
        

static void sdp_register_rfcomm(uint8 channel)
{
    /* Default to use const record */
    const uint8 *sr = gaiaTransportCommonServiceRecord(gaia_service_record, GAIA_SR_CH_IDX, channel);

    if(!sr)
        GAIA_PANIC();
    
    /* Store the RFCOMM channel */
    gaia->spp_listen_channel = channel;
    GAIA_TRANS_DEBUG(("gaia: ch %u\n", channel));
    
    /* Register the SDP record */
    ConnectionRegisterServiceRecord(&gaia->task_data, sizeof(gaia_service_record), sr);
}



/*************************************************************************
NAME
    gaiaTransportSppDropState
    
DESCRIPTION
    Clear down SPP-specific components of transport state
*/
void gaiaTransportSppDropState(gaia_transport *transport)
{
    transport->state.spp.sink = NULL;
}


/*************************************************************************
NAME
    gaiaTransportSppSendPacket
    
DESCRIPTION
    Copy the passed packet to the transport sink and flush it
    If <task> is not NULL, send a confirmation message
*/
void gaiaTransportSppSendPacket(Task task, gaia_transport *transport, uint16 length, uint8 *data)
{
    bool status = FALSE;
    
    if (gaia)
    {
        Sink sink = gaiaTransportGetSink(transport);
        
        if (SinkClaim(sink, length) == BAD_SINK_CLAIM)
        {
            GAIA_TRANS_DEBUG(("gaia: no sink\n"));
        }

        else
        {
            uint8 *sink_data = SinkMap(sink);
            memcpy (sink_data, data, length);

#ifdef DEBUG_GAIA_TRANSPORT
            {
                uint16 idx;
                GAIA_DEBUG(("gaia: put"));
                for (idx = 0; idx < length; ++idx)
                    GAIA_DEBUG((" %02x", data[idx]));
                GAIA_DEBUG(("\n"));
            }
#endif
            status = SinkFlush(sink, length);
        }   
    }
    
    if (task)
        gaiaTransportCommonSendGaiaSendPacketCfm(transport, data, status);
    
    else
        free(data);
}


#ifdef GAIA_HAVE_SPP_CLIENT
/*! @brief
 */
void gaiaTransportSppConnectReq(gaia_transport *transport, bdaddr *address)
{
    SppConnectRequest(&gaia->task_data, address, 0, 0);
}
#endif /* GAIA_HAVE_SPP_CLIENT */

/*! @brief
 */
void gaiaTransportSppConnectRes(gaia_transport *transport)
{

}

/*! @brief
 */
void gaiaTransportSppDisconnectReq(gaia_transport *transport)
{
    ConnectionRfcommDisconnectRequest(&gaia->task_data, gaiaTransportSppGetSink(transport));
}

/*! @brief
 */
void gaiaTransportSppDisconnectRes(gaia_transport *transport)
{

}

/*! @brief
 */
void gaiaTransportSppStartService(void)
{
    ConnectionRfcommAllocateChannel(&gaia->task_data, SPP_DEFAULT_CHANNEL);
}

/*! @brief
 */
Sink gaiaTransportSppGetSink(gaia_transport *transport)
{
    return transport->state.spp.sink;
}


/*! @brief
 */
bool gaiaTransportSppHandleMessage(Task task, MessageId id, Message message)
{
    bool msg_handled = TRUE;    /* default position is we've handled the message */

    switch (id)
    {
        case GAIA_INTERNAL_MORE_DATA:
            {
                GAIA_INTERNAL_MORE_DATA_T *m = (GAIA_INTERNAL_MORE_DATA_T *) message;
                GAIA_TRANS_DEBUG(("gaia: GAIA_INTERNAL_MORE_DATA: t=%04x\n", (uint16) m->transport));
                gaiaTransportProcessSource(m->transport);
            }
            break;
            
            
        case MESSAGE_MORE_DATA:
            {
                MessageMoreData *m = (MessageMoreData *) message;
                gaia_transport *t = gaiaTransportFromSink(StreamSinkFromSource(m->source));
                GAIA_TRANS_DEBUG(("gaia: MESSAGE_MORE_DATA: t=%04x\n", (uint16) t));
                
                if (t && (t->type == gaia_transport_spp))
                    gaiaTransportProcessSource(t);
                
                else
                    msg_handled = FALSE;
            }
            break;
            

        case CL_RFCOMM_REGISTER_CFM:
            {
                CL_RFCOMM_REGISTER_CFM_T *m = (CL_RFCOMM_REGISTER_CFM_T *) message;
                GAIA_TRANS_DEBUG(("gaia: CL_RFCOMM_REGISTER_CFM: %d = %d\n", m->server_channel, m->status));
                
                if (m->status == success)
                    sdp_register_rfcomm(m->server_channel);
                
                else
                    gaiaTransportCommonSendGaiaStartServiceCfm(gaia_transport_spp, NULL, FALSE);
            }
            break;
        
            
        case CL_SDP_REGISTER_CFM:
            {
                CL_SDP_REGISTER_CFM_T *m = (CL_SDP_REGISTER_CFM_T *) message;
                GAIA_TRANS_DEBUG(("gaia: CL_SDP_REGISTER_CFM: %d\n", m->status));
                
                if (m->status == sds_status_success)
                {
                    if (gaia->spp_sdp_handle == 0)
                        gaiaTransportCommonSendGaiaStartServiceCfm(gaia_transport_spp, NULL, TRUE);
                    
                    gaia->spp_sdp_handle = m->service_handle;
                }
                
                else
                    gaiaTransportCommonSendGaiaStartServiceCfm(gaia_transport_spp, NULL, FALSE);
            }
            break;

            
        case CL_RFCOMM_CONNECT_IND:
            {
                CL_RFCOMM_CONNECT_IND_T *m = (CL_RFCOMM_CONNECT_IND_T *) message;
                gaia_transport *transport = gaiaTransportFindFree();

                GAIA_TRANS_DEBUG(("gaia: CL_RFCOMM_CONNECT_IND\n"));
                
                if (transport == NULL)
                    ConnectionRfcommConnectResponse(task, FALSE, m->sink, m->server_channel, &rfcomm_config);
                
                else
                {
                    transport->type = gaia_transport_spp;
                    transport->state.spp.sink = m->sink;
                    transport->state.spp.rfcomm_channel = m->server_channel;
                    ConnectionRfcommConnectResponse(task, TRUE, m->sink, m->server_channel, &rfcomm_config);
                }
            }
            break;
            
            
        case CL_RFCOMM_SERVER_CONNECT_CFM:
            {
                CL_RFCOMM_SERVER_CONNECT_CFM_T *m = (CL_RFCOMM_SERVER_CONNECT_CFM_T *) message;
                gaia_transport *transport = gaiaTransportFromRfcommChannel(m->server_channel);
                
                GAIA_TRANS_DEBUG(("gaia: CL_RFCOMM_SERVER_CONNECT_CFM: ch=%d sts=%d\n", 
                                  m->server_channel, m->status));
                
                if (m->status == rfcomm_connect_success)
                {
                    transport->state.spp.sink = m->sink;
                    transport->state.spp.rfcomm_channel = m->server_channel;
                    ConnectionUnregisterServiceRecord(task, gaia->spp_sdp_handle);
                    gaiaTransportCommonSendGaiaConnectInd(transport, TRUE);
                    transport->connected = TRUE;
                    transport->enabled = TRUE;
                }
                
                else
                    gaiaTransportCommonSendGaiaConnectInd(transport, FALSE);
                    
            }
            break;
            
            
        case CL_SDP_UNREGISTER_CFM:
            {
                CL_SDP_UNREGISTER_CFM_T *m = (CL_SDP_UNREGISTER_CFM_T *) message;
                GAIA_TRANS_DEBUG(("gaia: CL_SDP_UNREGISTER_CFM: %d\n", m->status));
                if (m->status == success)
                {
                /*  Get another channel from the pool  */
                    ConnectionRfcommAllocateChannel(task, SPP_DEFAULT_CHANNEL);
                }
            }
            break;
 
            
            
        case CL_RFCOMM_DISCONNECT_IND:
            {
                CL_RFCOMM_DISCONNECT_IND_T *m = (CL_RFCOMM_DISCONNECT_IND_T *) message;
                gaia_transport *transport = gaiaTransportFromSink(m->sink);
                
                GAIA_TRANS_DEBUG(("gaia: CL_RFCOMM_DISCONNECT_IND\n"));
                
                ConnectionRfcommDisconnectResponse(m->sink);
		/* throw away any remaining input data */
		gaiaTransportFlushInput(transport);

            /*  release channel for re-use  */
                ConnectionRfcommDeallocateChannel(task, transport->state.spp.rfcomm_channel);
            }
            break;
        
        
        case CL_RFCOMM_DISCONNECT_CFM:
            {
                CL_RFCOMM_DISCONNECT_CFM_T *m = (CL_RFCOMM_DISCONNECT_CFM_T *) message;
                gaia_transport *transport = gaiaTransportFromSink(m->sink);
                
                GAIA_TRANS_DEBUG(("gaia: CL_RFCOMM_DISCONNECT_CFM\n"));
                gaiaTransportTidyUpOnDisconnection(transport);
                gaiaTransportCommonSendGaiaDisconnectCfm(transport);
            }
            break;
            
            
        case CL_RFCOMM_UNREGISTER_CFM:
            {
                CL_RFCOMM_UNREGISTER_CFM_T *m = (CL_RFCOMM_UNREGISTER_CFM_T *) message;
                GAIA_TRANS_DEBUG(("gaia: CL_RFCOMM_UNREGISTER_CFM\n"));
                
                if (m->status == success)
                {
                    gaia_transport *transport = gaiaTransportFromRfcommChannel(m->server_channel);
                    gaiaTransportCommonSendGaiaDisconnectInd(transport);                            
                }
            }
            break;
            
            
        case CL_RFCOMM_PORTNEG_IND:
            {
                CL_RFCOMM_PORTNEG_IND_T *m = (CL_RFCOMM_PORTNEG_IND_T *) message;
                GAIA_TRANS_DEBUG(("gaia: CL_RFCOMM_PORTNEG_IND\n"));
            /*  If this was a request send default params, otherwise accept changes  */
                ConnectionRfcommPortNegResponse(task, m->sink, m->request ? NULL : &m->port_params);
            }
            break;
                        
#ifdef GAIA_HAVE_SPP_CLIENT
        case SPP_MESSAGE_MORE_DATA:
            {
                SPP_MESSAGE_MORE_DATA_T *m = (SPP_MESSAGE_MORE_DATA_T *) message;
                gaia_transport *t = gaiaTransportFromSink(StreamSinkFromSource(m->source));
                GAIA_TRANS_DEBUG(("gaia: SPP_MESSAGE_MORE_DATA: t=%04x\n", (uint16) t));

                if (t && (t->type == gaia_transport_spp))
                    gaiaTransportProcessSource(t);
                
                else
                    msg_handled = FALSE;
            }
            break;

            
        case SPP_CLIENT_CONNECT_CFM:
            {
                SPP_CLIENT_CONNECT_CFM_T *m = (SPP_CLIENT_CONNECT_CFM_T *) message;
                bool success = m->status == spp_connect_success;

                GAIA_TRANS_DEBUG(("gaia: SPP_CLIENT_CONNECT_CFM: %u\n", m->status));

                if (m->status != spp_connect_pending)
                {
                    gaia_transport *transport = gaiaTransportFromRfcommChannel(0);

                    if (transport != NULL)
                    {
                        transport->enabled = TRUE;
                        transport->connected = success;
                        transport->state.spp.sink = m->sink;
                    }

                    gaiaTransportCommonSendGaiaConnectCfm(transport, success);
                }
            }
            break;

            case SPP_DISCONNECT_IND:
            {
                SPP_DISCONNECT_IND_T *m = (SPP_DISCONNECT_IND_T *) message;
                gaia_transport *transport = gaiaTransportFromSink(m->sink);

                SppDisconnectResponse(m->spp);
                gaiaTransportCommonSendGaiaDisconnectInd(transport);
            }
            break;
#endif /* GAIA_HAVE_SPP_CLIENT */

            /*  Things to ignore  */
        case MESSAGE_MORE_SPACE:
        case MESSAGE_SOURCE_EMPTY:
        case CL_RFCOMM_CONTROL_IND:
        case CL_RFCOMM_LINE_STATUS_IND:
            break;

        default:
            {
                /* indicate we couldn't handle the message */
            /*  GAIA_DEBUG(("gaia: spp: unh 0x%04X\n", id));  */
                msg_handled = FALSE;
            }
            break;
    }

    return msg_handled;
}

#endif /* GAIA_TRANSPORT_SPP */
