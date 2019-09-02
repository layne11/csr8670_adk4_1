/*****************************************************************
Copyright (c) 2011 - 2015 Qualcomm Technologies International, Ltd.
*/
#include "gaia.h"
#include "gaia_private.h"
#include "gaia_transport_gatt.h"
#include "gaia_transport.h"
#include "gaia_transport_common.h"

#ifdef GAIA_TRANSPORT_GATT

#include "gaia_db.h"

static void process_command(gaia_transport *transport, uint16 size_command, uint8 *command)
{
/*  Short packets are by definition badly framed and hence silently ignored  */
    if (size_command >= GAIA_GATT_OFFS_PAYLOAD)
    {
        uint8 *payload = command + GAIA_GATT_OFFS_PAYLOAD;
        uint16 size_payload = size_command - GAIA_GATT_OFFS_PAYLOAD;
        uint16 vendor_id = (command[GAIA_GATT_OFFS_VENDOR_ID_H] << 8) | command[GAIA_GATT_OFFS_VENDOR_ID_L];
        uint16 command_id = (command[GAIA_GATT_OFFS_COMMAND_ID_H] << 8) | command[GAIA_GATT_OFFS_COMMAND_ID_L];
#ifdef DEBUG_GAIA_TRANSPORT
        uint16 i;
    
        GAIA_TRANS_DEBUG(("gaia: command:"));
        for (i = 0; i < size_command; ++i)
        {
            GAIA_TRANS_DEBUG((" %02X",  command[i]));
        }
        GAIA_TRANS_DEBUG(("\n"));
#endif
        transport->state.gatt.size_response = 5;
        memcpy(transport->state.gatt.response, command, 4);
        transport->state.gatt.response[GAIA_GATT_OFFS_COMMAND_ID_H] |= GAIA_ACK_MASK_H;
        transport->state.gatt.response[GAIA_GATT_OFFS_PAYLOAD] = GAIA_STATUS_IN_PROGRESS;
        
        gaiaProcessCommand(transport, vendor_id, command_id, size_payload, payload);
    }
}

static void process_data(gaia_transport *transport, uint16 size_data, uint8 *data)
{
#ifdef DEBUG_GAIA_TRANSPORT
    uint16 i;
    
    GAIA_TRANS_DEBUG(("gaia: data:"));
    for (i = 0; i < size_data; ++i)
    {
        GAIA_TRANS_DEBUG((" %02X",  data[i]));
    }
    GAIA_TRANS_DEBUG(("\n"));
#endif
}

/*! @brief
 */
void GaiaConnectGatt(uint16 cid)
{
    gaia_transport *transport = gaiaTransportFindFree();
    bool ok = FALSE;
    
    if (transport)
    {
        transport->type = gaia_transport_gatt;
        transport->state.gatt.cid = cid;
        
        transport->state.gatt.config_not = TRUE; /* we're really supposed to persist these */
        transport->state.gatt.config_ind = FALSE;
    
        ok = TRUE;
    }
    
    gaiaTransportCommonSendGaiaConnectInd(transport, ok);
}


/*! @brief
 */
void GaiaDisconnectGatt(uint16 cid)
{
    gaia_transport *transport = gaiaTransportFromCid(cid);

    GAIA_TRANS_DEBUG(("gaia: disconnect cid=0x%04X tra=0x%04X\n",
        cid, (uint16) transport));

    if(transport)   
    {
        gaiaTransportCommonSendGaiaDisconnectInd(transport);       
    }
}


/*************************************************************************
NAME
    gaiaTransportGattRegisterServer
    
DESCRIPTION
    Register the GAIA GATT server with the GATT Manager
*/
void gaiaTransportGattRegisterServer(uint16 start_handle, uint16 end_handle)
{
    gatt_manager_status_t status;
    gatt_manager_server_registration_params_t registration_params;
    
    registration_params.task = &gaia->task_data;
    registration_params.start_handle = start_handle;
    registration_params.end_handle = end_handle;
            
    status = GattManagerRegisterServer(&registration_params);
    GAIA_TRANS_DEBUG(("GM reg %u\n", status));
    
    gaiaTransportCommonSendGaiaStartServiceCfm(gaia_transport_gatt, NULL, status == gatt_manager_status_success);
}


/*************************************************************************
NAME
    gaiaTransportGattRes
    
DESCRIPTION
    Copy a response to the transport buffer and notify the central
*/
void gaiaTransportGattRes(gaia_transport *transport, uint16 size_response, uint8 *response)
{
    if (size_response >= GAIA_GATT_OFFS_PAYLOAD)
    {
        if (transport->state.gatt.config_not)
        {
            GAIA_TRANS_DEBUG(("gaia: not %02X%02X %02X%02X\n", response[0], response[1], response[2], response[3]));
            GattManagerRemoteClientNotify(&gaia->task_data, transport->state.gatt.cid, HANDLE_GAIA_RESPONSE_ENDPOINT, size_response, response);
        }

        if (transport->state.gatt.config_ind)
        {
            GAIA_TRANS_DEBUG(("gaia: ind %02X%02X %02X%02X\n", response[0], response[1], response[2], response[3]));
            GattManagerRemoteClientIndicate(&gaia->task_data, transport->state.gatt.cid, HANDLE_GAIA_RESPONSE_ENDPOINT, size_response, response);
        }
        
        if (response[GAIA_GATT_OFFS_COMMAND_ID_H] & GAIA_ACK_MASK_H)
        {
        /*  Response is an acknowledgement; cache it  */
            if (size_response > GATT_BUFFER_SIZE)
            {
            /*  No space to store the response; store vendor + command + status only  */
                size_response = GAIA_GATT_OFFS_PAYLOAD + 1;
            }
            
            transport->state.gatt.size_response = size_response;
            memcpy(transport->state.gatt.response, response, size_response);
        }
    }        
}

/*************************************************************************
NAME
    gaiaTransportGattSend
    
DESCRIPTION
    Build and send a short format GAIA packet
    If unpack is true then the payload is treated as uint16[]
    
    0 bytes  1        2        3        4               len+5
    +--------+--------+--------+--------+ +--------+--/ /---+
    |   VENDOR ID     |   COMMAND ID    | | PAYLOAD   ...   |
    +--------+--------+--------+--------+ +--------+--/ /---+
 */

void gaiaTransportGattSend(gaia_transport *transport, 
                           uint16 vendor_id,
                           uint16 command_id,
                           uint8 status, 
                           uint8 size_payload,
                           void *payload,
                           bool unpack)
{
    uint16 size_response;
    uint8 *response;

    if (unpack)
    {
        size_response = GAIA_GATT_OFFS_PAYLOAD + 2 * size_payload;
    }
    else
    {
        size_response = GAIA_GATT_OFFS_PAYLOAD + size_payload;
    }
    
    if (status != GAIA_STATUS_NONE)
    {
        ++size_response;
    }
    
    response = malloc(size_response);
    if (response)
    {
        uint8 *r = response;
        
        *r++ = HIGH(vendor_id);
        *r++ = LOW(vendor_id);
        *r++ = HIGH(command_id);
        *r++ = LOW(command_id);
        
        if (status != GAIA_STATUS_NONE)
        {
            *r++ = status;
        }
        
        if (unpack)
        {
            uint16 *data = (uint16 *) payload;
            
            while (size_payload--)
            {
                *r++ = HIGH(*data);
                *r++ = LOW(*data++);
            }
        }
        else
        {
            uint8 *data = (uint8 *) payload;
            
            while (size_payload--)
            {
                *r++ = *data++;
            }
        }
        
        gaiaTransportGattRes(transport, size_response, response);
        free(response);
    }
}

/*************************************************************************
NAME
    gaiaTransportGattSendPacket
    
DESCRIPTION
    Convert internal (long) format to GATT (short) format response
    If <task> is not NULL, send a confirmation message
*/
void gaiaTransportGattSendPacket(Task task, gaia_transport *transport, uint16 length, uint8 *data)
{
    uint16 size_response = length - GAIA_OFFS_PAYLOAD + GAIA_GATT_OFFS_PAYLOAD;
    uint8 *response = data + GAIA_OFFS_PAYLOAD - GAIA_GATT_OFFS_PAYLOAD;
    
    gaiaTransportGattRes(transport, size_response, response);
    
    if (task)
    {
        gaiaTransportCommonSendGaiaSendPacketCfm(transport, data, TRUE);
    }
    else
    {
        free(data);
    }
}

/*! @brief
 */
void gaiaHandleGattManagerAccessInd(GATT_MANAGER_SERVER_ACCESS_IND_T *ind)
{
    gaia_transport *transport = gaiaTransportFromCid(ind->cid);
    uint16 handle = ind->handle;
    uint16 flags = ind->flags;
    gatt_status_t status = gatt_status_success;
    uint8 *response = NULL;
    uint16 size_response = 0;
    
    GAIA_TRANS_DEBUG(("gaia: acc cid=0x%04X hdl=0x%04X flg=%c%c%c%c off=%u siz=%u\n",
        ind->cid, ind->handle, 
        flags & ATT_ACCESS_PERMISSION       ? 'p' : '-',
        flags & ATT_ACCESS_WRITE_COMPLETE   ? 'c' : '-',
        flags & ATT_ACCESS_WRITE            ? 'w' : '-',
        flags & ATT_ACCESS_READ             ? 'r' : '-',
        ind->offset, ind->size_value));

    if (transport)
    {
        if (flags == (ATT_ACCESS_PERMISSION | ATT_ACCESS_WRITE_COMPLETE | ATT_ACCESS_WRITE))
        {
            if (handle == HANDLE_GAIA_COMMAND_ENDPOINT)
            {
                process_command(transport, ind->size_value, ind->value);
            }
            else if (handle == HANDLE_GAIA_DATA_ENDPOINT)
            {
                 process_data(transport, ind->size_value, ind->value);
            }
            else if (handle == HANDLE_GAIA_RESPONSE_CLIENT_CONFIG)
            {
                transport->state.gatt.config_not = (ind->value[0] & 1) != 0;
                transport->state.gatt.config_ind = (ind->value[0] & 2) != 0;
                
                GAIA_TRANS_DEBUG(("gaia: cli not=%u ind=%u\n", 
                    transport->state.gatt.config_not, 
                    transport->state.gatt.config_ind));
            }
            else
            {
                status = gatt_status_write_not_permitted;
            }
        }
        else if (flags == (ATT_ACCESS_PERMISSION | ATT_ACCESS_READ))
        {
            if (handle == HANDLE_GAIA_RESPONSE_ENDPOINT)
            {
                response = transport->state.gatt.response;
                size_response = transport->state.gatt.size_response;
            }
            else if (handle == HANDLE_GAIA_DATA_ENDPOINT)
            {
            }
            else
            {
                status = gatt_status_read_not_permitted;
            }
        }
        else
        {
            status = gatt_status_request_not_supported;
        }
        
        if (handle)
        {
        /*  Handle 0 is handled by the demultiplexer  */
            GattManagerServerAccessResponse(&gaia->task_data, ind->cid, ind->handle, status, size_response, response);
        }
    }
    else
    {
        /* If there is is an access indication, it is required to respond */
        GattManagerServerAccessResponse(&gaia->task_data, ind->cid, ind->handle, gatt_status_insufficient_resources, 0, NULL);
        GAIA_TRANS_DEBUG(("gaia: no transport\n"));
    }
}

/*! @brief
 */
void gaiaTransportGattDisconnectReq(gaia_transport *transport)
{
    GattManagerDisconnectRequest(transport->state.gatt.cid);
}

/*! @brief
 */
void gaiaTransportGattDisconnectRes(gaia_transport *transport)
{
    gaiaTransportTidyUpOnDisconnection(transport);
}

/*************************************************************************
NAME
    gaiaTransportGattDropState
    
DESCRIPTION
    Clear down GATT-specific components of transport state
*/
void gaiaTransportGattDropState(gaia_transport *transport)
{
    memset(&transport->state.gatt, 0, sizeof transport->state.gatt);
}


#endif /* GAIA_TRANSPORT_GATT */
