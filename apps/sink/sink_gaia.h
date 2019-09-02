/****************************************************************************
Copyright (c) 2010 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.1

FILE NAME
    sink_gaia.h        

DESCRIPTION
    Header file for interface with Generic Application Interface Architecture
    library

NOTES

*/
#ifndef _SINK_GAIA_H_
#define _SINK_GAIA_H_

#include <gaia.h>
#include "sink_private.h"


#ifndef GATT_ENABLED
#undef GATT_GAIA_SERVER
#endif

#ifdef GATT_GAIA_SERVER
#include "sink_gatt_db.h"
#endif

#ifdef DEBUG_GAIA
#define GAIA_DEBUG(x) DEBUG(x)
#else
#define GAIA_DEBUG(x) 
#endif

#define GAIA_API_MINOR_VERSION (4)

#define GAIA_TONE_BUFFER_SIZE (94)
#define GAIA_TONE_MAX_LENGTH ((GAIA_TONE_BUFFER_SIZE - 4) / 2)

#define GAIA_ILLEGAL_PARTITION (15)
#define GAIA_DFU_REQUEST_TIMEOUT (30)



typedef struct
{
    unsigned fixed:1;
    unsigned size:15;
} gaia_config_entry_size_t;

/*For complete information about the GAIA commands, refer to the document GAIAHeadsetCommandReference*/

#define NUM_WORDS_GAIA_CMD_HDR 2 /*No. of words in GAIA Command header */

#define NUM_WORDS_GAIA_CMD_PER_EQ_PARAM 4 /*No. of words for each user EQ parameter in GAIA command*/

#define CHANGE_NUMBER_OF_ACTIVE_BANKS   0xFFFF
#define CHANGE_NUMBER_OF_BANDS          0xF0FF
#define CHANGE_BANK_MASTER_GAIN         0xF0FE
#define CHANGE_BAND_PARAMETER           0xF000
#define USER_EQ_BANK_INDEX              1

typedef enum
{
    PARAM_HI_OFFSET,
    PARAM_LO_OFFSET,
    VALUE_HI_OFFSET,
    VALUE_LO_OFFSET    
}gaia_cmd_payload_offset;


/*************************************************************************
NAME
    gaiaReportPioChange
    
DESCRIPTION
    Relay any registered PIO Change events to the Gaia client
    We handle the PIO-like GAIA_EVENT_CHARGER_CONNECTION here too
*/
void gaiaReportPioChange(uint32 pio_state);


/*************************************************************************
NAME
    gaiaReportEvent
    
DESCRIPTION
    Relay any significant application events to the Gaia client
*/
void gaiaReportEvent(uint16 id);


/*************************************************************************
NAME
    gaiaReportUserEvent
    
DESCRIPTION
    Relay any user-generated events to the Gaia client
*/
void gaiaReportUserEvent(uint16 id);

        
/*************************************************************************
NAME
    gaiaReportSpeechRecResult
    
DESCRIPTION
    Relay a speech recognition result to the Gaia client
*/
void gaiaReportSpeechRecResult(uint16 id);


/*************************************************************************
NAME
    handleGaiaMessage
    
DESCRIPTION
    Handle messages passed up from the Gaia library
*/
void handleGaiaMessage(Task task, MessageId id, Message message);

/*************************************************************************
NAME
    gaiaGetBdAddrNonGaiaDevice
    
DESCRIPTION
    Determines the BD Address of the device which is not connected to GAIA.
*/
bool gaiaGetBdAddrNonGaiaDevice(bdaddr *bdaddr_non_gaia_device);



/*************************************************************************
NAME
    gaiaDfuRequest
    
DESCRIPTION
    Request Device Firmware Upgrade from the GAIA host
*/
void gaiaDfuRequest(void);


/*************************************************************************
NAME
    gaiaDisconnect
    
DESCRIPTION
    Disconnect from GAIA client
*/
void gaiaDisconnect(void);


/*************************************************************************
NAME    
    gaia_send_response
    
DESCRIPTION
    Build and Send a Gaia acknowledgement packet
   
*/ 
void gaia_send_response(uint16 vendor_id, uint16 command_id, uint16 status,
                          uint16 payload_length, uint8 *payload);


/*************************************************************************
NAME    
    gaia_send_sppdata
    
DESCRIPTION
    Build and Send a Gaia custom spp packet
   
*/ 
void gaia_send_sppdata(uint8 *payload, uint16 payload_length);

/*************************************************************************
NAME    
    gaia_send_response_16
    
DESCRIPTION
    Build and Send a Gaia acknowledgement packet from a uint16[] payload
   
*/ 
void gaia_send_response_16(uint16 command_id, uint16 status,
                          uint16 payload_length, uint16 *payload);


#if defined GATT_GAIA_SERVER && defined ENABLE_GAIA
#define gaiaGattEnabled() (TRUE)
#else
#define gaiaGattEnabled() (FALSE)
#endif


#if defined GATT_GAIA_SERVER && defined ENABLE_GAIA
#define gaiaGattServerInitialise(void) GaiaStartGattServer(HANDLE_GAIA_SERVICE, HANDLE_GAIA_SERVICE_END)
#else
#define gaiaGattServerInitialise(void)
#endif


#if defined GATT_GAIA_SERVER && defined ENABLE_GAIA
#define gaiaGattConnect(cid) GaiaConnectGatt(cid)
#else
#define gaiaGattConnect(cid)
#endif

#if defined GATT_GAIA_SERVER && defined ENABLE_GAIA
#define gaiaGattDisconnect(cid) GaiaDisconnectGatt(cid)
#else
#define gaiaGattDisconnect(cid)
#endif


#endif /*_SINK_GAIA_H_*/
