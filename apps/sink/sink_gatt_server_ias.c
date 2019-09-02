/****************************************************************************
Copyright (c) 2015 - 2016 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_gatt_server_ias.c

DESCRIPTION
    Routines to handle messages sent from the GATT Immediate Alert Server Task.
*/


/* Firmware headers */
#include <csrtypes.h>
#include <message.h>

/* Library headers */
#include <gatt_imm_alert_server.h>

/* Application headers */
#include "sink_gatt_db.h"
#include "sink_gatt_server_ias.h"
#include "sink_gatt_client_ias.h"
#include "sink_gatt_client_hid.h"
#include "sink_debug.h"
#include "sink_gatt_server.h"
#include "sink_powermanager.h"
#include "sink_private.h"
#include "sink_ble_advertising.h"
#include "sink_ble_scanning.h"


#ifdef GATT_IAS_SERVER

#ifdef DEBUG_GATT
#define GATT_DEBUG(x) DEBUG(x)
#else
#define GATT_DEBUG(x) 
#endif

/*******************************************************************************
NAME
    sinkGattImmAlertServerInitialise
    
DESCRIPTION
    Initialise IAS server task.
    
PARAMETERS
    ptr - pointer to allocated memory to store server tasks rundata.
    
RETURNS
    TRUE if the IAS server task was initialised, FALSE otherwise.
*/
bool sinkGattImmAlertServerInitialise(uint16 **ptr)
{
    if (GattImmAlertServiceServerInit( sinkGetBleTask(),  (GIASS_T *)*ptr,
                                                                     HANDLE_IMM_ALERT_SERVICE,
                                                                     HANDLE_IMM_ALERT_SERVICE_END))
    {
        GATT_DEBUG(("GATT Immediate Alert Server initialised\n"));
        /* The size of IAS is also calculated and memory is alocated.
         * So advance the ptr so that the next Server while initializing.
         * shall not over write the same memory */
        *ptr += sizeof(GIASS_T);
        return TRUE;
    }
    else
    {
        GATT_DEBUG(("GATT Immediate Alert Server init failed \n"));
        return FALSE;
    }
}


/*******************************************************************************
NAME
    handleWriteAlertLevel
    
DESCRIPTION
    Handle when a GATT_IMM_ALERT_SERVER_WRITE_LEVEL_IND message is recieved.
    
PARAMETERS
    ind Pointer to a GATT_IMM_ALERT_SERVER_WRITE_LEVEL_IND message.
    
RETURNS
    void
*/
static void handleWriteAlertLevel(GATT_IMM_ALERT_SERVER_WRITE_LEVEL_IND_T * ind)
{    
    GATT_DEBUG(("GATT_IMM_ALERT_SERVER_WRITE_LEVEL_IND IAS=[0x%p] cid=[0x%x]\n", (void *)ind->imm_alert_service, ind->cid));

    sinkGattImmAlertLocalAlert(ind->alert_level);
}

/******************************************************************************/
void sinkGattImmAlertLocalAlert(gatt_imm_alert_level alert_level)
{
   
    bool alertStopTimer = FALSE;

    /* Write Immediate Alert level */
    switch(alert_level)
    {
        case alert_level_no:
            {
                /* stop alerting/take no action if not alerting */
                sinkGattServerImmAlertStopAlert();
            }
            break;

        case alert_level_mild:
            {
                /* Generate Mild Alert level event */
                sinkGattServerImmAlertStopAlert();
                sinkGattServerImmAlertMild(0);
                alertStopTimer = TRUE;
            }
            break;

        case alert_level_high:
            {
                /* Generate High Alert level event */
                sinkGattServerImmAlertStopAlert();
                sinkGattServerImmAlertHigh(0);
                alertStopTimer = TRUE;
            }
            break;

        default:
            /* Invalid alert level*/
            break;
    }
    /* Start Alert stop Timeout */
    if(alertStopTimer)
    {
        MessageSendLater(&theSink.task, EventSysImmAlertTimeout , 0, D_SEC(theSink.conf1->timeouts.ImmediateAlertStopTimeout_s));
    }
    GATT_DEBUG(("   Alert  level=[%u]\n", alert_level));
}

/******************************************************************************/
void sinkGattServerImmAlertStopAlert(void)
{
    MessageCancelAll(&theSink.task, EventSysImmAlertTimeout);
    MessageCancelAll(&theSink.task, EventSysImmAlertMild);
    MessageCancelAll(&theSink.task, EventSysImmAlertHigh);
}

/******************************************************************************/
void sinkGattImmAlertServerMsgHandler(Task task, MessageId id, Message message)
{
    switch(id)
    {
        case GATT_IMM_ALERT_SERVER_WRITE_LEVEL_IND:
        {
            /*Local Alert*/
            handleWriteAlertLevel((GATT_IMM_ALERT_SERVER_WRITE_LEVEL_IND_T*)message);
        }
        break;
    }
}

#endif /* GATT_IAS_SERVER */
