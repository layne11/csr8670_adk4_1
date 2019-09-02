/****************************************************************************/
/* Copyright (c) 2014 - 2016 Qualcomm Technologies International, Ltd. */
/* Part of ADK 4.1 

FILE NAME
    sink_gatt_server_lls.c

DESCRIPTION
    Routines to handle messages sent from the GATT Link Loss Server Task.
*/


/* Firmware headers */
#include <csrtypes.h>
#include <message.h>

/* Application headers */
#include "sink_gatt_db.h"
#include "sink_gatt_server_lls.h"

#include "sink_debug.h"
#include "sink_gatt_server.h"
#include "sink_private.h"
#include "sink_gatt_common.h"


#ifdef GATT_LLS_SERVER


#ifdef DEBUG_GATT
#define GATT_DEBUG(x) DEBUG(x)
#else
#define GATT_DEBUG(x) 
#endif


/*******************************************************************************/
bool sinkGattLinkLossServerInitialiseTask(uint16 **ptr)
{
    if(GattLinkLossServerInit(sinkGetBleTask(), (GLLSS_T*)*ptr,
                                HANDLE_LINK_LOSS_SERVICE,
                                               HANDLE_LINK_LOSS_SERVICE_END))
        {
            GATT_DEBUG(("GATT Link Loss Server initialised\n"));
            gattServerSetServicePtr(ptr, gatt_server_service_lls);
           *ptr += sizeof(GLLSS_T);
            return TRUE;
        }
        else
        {
        GATT_DEBUG(("GATT Link Loss Server init failed\n"));
        return FALSE;
        }
}

/******************************************************************************/
void sinkGattHandleLinkUpInd(uint16 cid)
{
    uint16 index = gattCommonConnectionFindByCid(cid);

    if(index != GATT_INVALID_INDEX)
    {
        /* Stop timer and stop alert */
        sinkGattLinkLossAlertStop();
    }

}

/******************************************************************************/
void sinkGattHandleLinkLossInd(uint16 cid)
{
    bool alertStopTimer = FALSE;
    gatt_link_loss_alert_level alert_level = gatt_link_loss_alert_level_reserved;
    uint16 index = gattCommonConnectionFindByCid(cid);

    /* Check if the cid is same of the connected device */
    if(index == GATT_INVALID_INDEX)
    {
        return;
    }

    /* Read the alert level proper server */
    alert_level = GATT_SERVER.alert_level;
    /* Reset the alert level */
     GATT_SERVER.alert_level = gatt_link_loss_alert_level_no;

    /* Only if the alert level is set, handle it properly */
    switch(alert_level)
    {
        case gatt_link_loss_alert_level_no:
        /* Need not alert on link loss */
        break;

        case gatt_link_loss_alert_level_mild:
        /* Generate Mild Alert level event */
        {
            alertStopTimer = TRUE;
            sinkGattLinkLossAlertStop();
            sinkGattLinkLossAlertMild(0);
        }
        break;
        case gatt_link_loss_alert_level_high:
        /* Generate High Alert level event */
        {
            alertStopTimer = TRUE;
            sinkGattLinkLossAlertStop();
            sinkGattLinkLossAlertHigh(0);
        }
        break;
        
        default:
        break;
    }

    if(alertStopTimer)
    {
        /* Start the stop alert timeout if either High/Mild alert is set */
        MessageSendLater(&theSink.task, EventSysLlsAlertTimeout , 0, D_SEC(theSink.conf1->timeouts.LinkLossAlertStopTimeout_s));
    }
}

/******************************************************************************
 * Helper funtion to stop the alterting
******************************************************************************/ 
void sinkGattLinkLossAlertStop(void)
{
    MessageCancelAll(&theSink.task, EventSysLlsAlertTimeout);
    MessageCancelAll(&theSink.task, EventSysLlsAlertMild);
    MessageCancelAll(&theSink.task, EventSysLlsAlertHigh);
}

/*******************************************************************************
NAME
    handleAlertLevelChangeInd
    
DESCRIPTION
    Handle when a GATT_LLS_ALERT_LEVEL_CHANGE_IND message is recieved.
    
PARAMETERS
    ind Pointer to a GATT_LLS_ALERT_LEVEL_CHANGE_IND message.
    
RETURNS
    void
*/
static void handleAlertLevelChangeInd(GATT_LLS_ALERT_LEVEL_CHANGE_IND_T * ind)
{
    uint16 index = gattCommonConnectionFindByCid(ind->cid);
    GATT_DEBUG(("GATT_LLS_ALERT_LEVEL_CHANGE_IND LLS=[0x%p] cid=[0x%x]\n", (void *)ind->link_loss_server, ind->cid));

    if(index != GATT_INVALID_INDEX)
        GATT_SERVER.alert_level = (gatt_link_loss_alert_level)ind->alert_level;
}

/*******************************************************************************
NAME
    handleAlertLevelChangeInd
    
DESCRIPTION
    Handle when a GATT_LLS_ALERT_LEVEL_CHANGE_IND message is recieved.
    
PARAMETERS
    ind Pointer to a GATT_LLS_ALERT_LEVEL_CHANGE_IND message.
    
RETURNS
    void
*/
static void handleAlertLevelReadReq(GATT_LLS_ALERT_LEVEL_READ_REQ_T * req)
{
    uint16 index = gattCommonConnectionFindByCid(req->cid);
    GATT_DEBUG(("GATT_LLS_ALERT_LEVEL_READ_REQ LLS=[0x%p] cid=[0x%x]\n", (void *)req->link_loss_server, req->cid));

    if(index != GATT_INVALID_INDEX)
        GattLinkLossServerReadLevelResponse(req->link_loss_server, req->cid, GATT_SERVER.alert_level);
}

/******************************************************************************/
void sinkGattLinkLossServerMsgHandler(Task task, MessageId id, Message message)
{
    switch(id)
    {
        case GATT_LLS_ALERT_LEVEL_CHANGE_IND:
        {
            GATT_DEBUG(("GATT Link Loss Alert level changed\n"));
            handleAlertLevelChangeInd((GATT_LLS_ALERT_LEVEL_CHANGE_IND_T*)message);
        }
        break;
        case GATT_LLS_ALERT_LEVEL_READ_REQ:
        {
            GATT_DEBUG(("GATT Link Loss Alert level read request\n"));
            handleAlertLevelReadReq((GATT_LLS_ALERT_LEVEL_READ_REQ_T*)message);
        }
        break;

        default:
             GATT_DEBUG(("GATT Unknown message from LLS lib\n"));
         break;
    }
}

#endif /* GATT_LLS_SERVER */

