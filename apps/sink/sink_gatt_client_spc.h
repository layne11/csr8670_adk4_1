/*******************************************************************************
Copyright (c) 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.1

FILE NAME
    sink_gatt_client_spc.h

DESCRIPTION
    Routines to handle messages sent from the GATT Scan parameters Service Client Task.
    
NOTES

*/

#ifndef _SINK_GATT_CLIENT_SPC_H_
#define _SINK_GATT_CLIENT_SPC_H_


#ifndef GATT_ENABLED
#undef GATT_SPC_CLIENT
#endif

#include <csrtypes.h>
#include <message.h>

#include <gatt_scan_params_client.h>

/****************************************************************************
NAME    
    sinkGattSpClientSetupAdvertisingFilter
    
DESCRIPTION
    Adds Scan parameter service to the advertising filter
*/    
#ifdef GATT_SPC_CLIENT
void sinkGattSpClientSetupAdvertisingFilter(void);
#else
#define sinkGattSpClientSetupAdvertisingFilter() ((void)(0))
#endif

/****************************************************************************
NAME    
    sinkGattSpClientAddService
    
DESCRIPTION
    Adds Scan Parameters client connection service.
    
PARAMETERS
    cid              The connection ID
    start           The start handle of the Scan Parameters Service
    end             The end handle of the Scan Parameters Service
    
RETURNS    
    TRUE if the Scan Parameters Service was successfully added, FALSE otherwise.
*/
#ifdef GATT_SPC_CLIENT
bool sinkGattSpClientAddService(uint16 cid, uint16 start, uint16 end);
#else
#define sinkGattSpClientAddService(cid, start, end) (FALSE)
#endif

/****************************************************************************
NAME    
    sinkGattSpClientSetScanIntervalWindow
    
DESCRIPTION
    To update the new Scan Interval and window to remote scan server.
    
PARAMETERS
    fast_scan           Should be TRUE if fast scanning. FALSE otherwise.
    
*/
#ifdef GATT_SPC_CLIENT
void sinkGattSpClientSetScanIntervalWindow(bool fast_scan);
#else
#define sinkGattSpClientSetScanIntervalWindow(fast_scan) ((void)(0))
#endif

/****************************************************************************
NAME    
    sinkGattSpClientRemoveService
    
DESCRIPTION
    Removes the SPC service
    
PARAMETERS
    gspc           The SPC client pointer
*/
#ifdef GATT_SPC_CLIENT
void sinkGattSpClientRemoveService(GSPC_T *gspc);
#else
#define sinkGattSpClientRemoveService(gspc) ((void)(0))
#endif

/*******************************************************************************
NAME
    sinkGattSpClientMsgHandler
    
DESCRIPTION
    Handle messages from the GATT Client Task library
    
PARAMETERS
    task     The task the message is delivered
    id         The ID for the GATT message
    payload The message payload
    
*/

#ifdef GATT_SPC_CLIENT
void sinkGattSpClientMsgHandler(Task task, MessageId id, Message message);
#else
#define sinkGattSpClientMsgHandler(task, id, message) ((void)(0))
#endif

/*******************************************************************************
NAME
    sinkGattSpClientEnabled
    
DESCRIPTION
    Returns TRUE if Scan Parameters client is enabled.
*/
#ifdef GATT_SPC_CLIENT
#define sinkGattSpClientEnabled(void) (TRUE)
#else
#define sinkGattSpClientEnabled(void) (FALSE)
#endif


#endif /* _SINK_GATT_CLIENT_SPC_H_ */


