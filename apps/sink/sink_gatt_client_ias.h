/*******************************************************************************
Copyright (c) 2015 - 2016 Qualcomm Technologies International, Ltd.
Part of ADK 4.1

FILE NAME
    sink_gatt_client_ias.h

DESCRIPTION
    Routines to handle messages sent from the GATT Immediate Alert Client Task.
    
NOTES

*/

#ifndef _SINK_GATT_CLIENT_IAS_H_
#define _SINK_GATT_CLIENT_IAS_H_


#ifndef GATT_ENABLED
#undef GATT_IAS_CLIENT
#endif


#include <csrtypes.h>
#include <message.h>

#include <gatt_imm_alert_client.h>
#include "sink_ble_gap.h"

#define IASC_INVALID_ALERT_LEVEL 0xF

/*
    Imm Alert device type used to identify which mode to switch to
*/
typedef enum
{
    sink_gatt_ias_alert_none=0,    /* No alert */
    sink_gatt_ias_alert_remote,    /* Alert Remote control */
    sink_gatt_ias_alert_phone      /* Alert phone */
} sink_gatt_ias_dev_type;

/*!
    @brief Imm Alert return codes.
*/
typedef enum
{
    ias_alert_success,              /*!< Success */
    ias_alert_param_invalid,        /*!< Parameter invalid */
    ias_alert_not_supported        /*!< Feature not supported */
} ias_alert_status_t;

       
/****************************************************************************
NAME    
    sinkGattIasClientSetupAdvertisingFilter
    
DESCRIPTION
    Adds Immediate Alert service to the advertising filter
*/    
#ifdef GATT_IAS_CLIENT
void sinkGattIasClientSetupAdvertisingFilter(void);
#else
#define sinkGattIasClientSetupAdvertisingFilter() ((void)(0))
#endif

/****************************************************************************
NAME    
    sinkGattIasClientAddService
    
DESCRIPTION
    Adds Immediate Alert to list of client connection service.
    
PARAMETERS
    cid              The connection ID
    start           The start handle of the Imm Alert service
    end             The end handle of the Imm Alert service
    
RETURNS    
    TRUE if the Imm Alert service was successfully added, FALSE otherwise.
*/
#ifdef GATT_IAS_CLIENT
bool sinkGattIasClientAddService(uint16 cid, uint16 start, uint16 end);
#else
#define sinkGattIasClientAddService(cid, start, end) (FALSE)
#endif


/****************************************************************************
NAME    
    sinkGattIasClientRemoveService
    
DESCRIPTION
    Removes the Imm Alert Client service associated with the connection ID.
    
PARAMETERS
    giasc           The IAS client pointer
    cid             The connection ID
*/
#ifdef GATT_IAS_CLIENT
void sinkGattIasClientRemoveService(GIASC_T *giasc);
#else
#define sinkGattIasClientRemoveService(giasc) ((void)(0))
#endif




/*******************************************************************************
NAME
    sinkGattIasClientMsgHandler
    
DESCRIPTION
    Handle messages from the GATT Client Task library
    
PARAMETERS
    task     The task the message is delivered
    id         The ID for the GATT message
    payload The message payload
    
*/

#ifdef GATT_IAS_CLIENT
void sinkGattIasClientMsgHandler(Task task, MessageId id, Message message);
#else
#define sinkGattIasClientMsgHandler(task, id, message) ((void)(0))
#endif


/*******************************************************************************
NAME
    sinkGattIasClientSetAlert
    
DESCRIPTION
    Set Alert Level from Sink app 
    
PARAMETERS
    alert_level     The Alert Level to be set

*/
#ifdef GATT_IAS_CLIENT
ias_alert_status_t sinkGattIasClientSetAlert(gatt_imm_alert_set_level alert_level, sink_gatt_ias_dev_type dev_type);
#else
#define sinkGattIasClientSetAlert(alert_level, dev_type) ((void)(0))
#endif

/****************************************************************************
NAME    
    sinkGattIasClientInit
    
DESCRIPTION
    Initialisation of Imm Alert client service for cached data
*/    
#ifdef GATT_IAS_CLIENT
void sinkGattIasClientInit(void);
#else
#define sinkGattIasClientInit() ((void)(0))
#endif

/*******************************************************************************
NAME
    sinkGattIasClientEnabled
    
DESCRIPTION
    Returns if Imm Alert Client Service is enabled.

*/
#ifdef GATT_IAS_CLIENT
#define sinkGattIasClientEnabled(void) (TRUE)
#else
#define sinkGattIasClientEnabled(void) (FALSE)
#endif


#endif /* _SINK_GATT_CLIENT_IAS_H_ */
