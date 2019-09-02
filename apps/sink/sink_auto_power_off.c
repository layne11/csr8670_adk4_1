/*
Copyright (c) 2015 - 2016 Qualcomm Technologies International, Ltd. 
Part of ADK 4.1
*/
/**
\file
\ingroup sink_app
\brief
    This file handles all auto power off funcationality.
*/

/****************************************************************************
    Header files       */
#include "sink_private.h"
#include "sink_statemanager.h"
#include "sink_gatt_client.h"

#ifdef DEBUG_AUTO_POWER_OFF
#define AUTO_POWER_OFF_DEBUG(x) DEBUG(x)
#else
#define AUTO_POWER_OFF_DEBUG(x)
#endif

/*************************************************************************
NAME
    sinkHandleAutoPowerOff

DESCRIPTION
    This function determines whether to send power off immediately or need to reset the 
    auto power off timer again.
    Depends the device state it clear/updates the auto_power_off bit mask value based on 
    eithr BLE connection status or silence detection during media streaming.

RETURNS
    TRUE for postponed the auto power off, FALSE otherwise

**************************************************************************/
bool sinkHandleAutoPowerOff(Task task, sinkState lState)
{
    /* Intialise for not do shutdown operation. */
    bool shutdown = FALSE; 

    AUTO_POWER_OFF_DEBUG(("APO: sinkHandleAutoPowerOff UE state [%d]\n", lState));

    AUTO_POWER_OFF_DEBUG(("APO: Auto S Off[%d] sec elapsed\n" , theSink.conf1->timeouts.AutoSwitchOffTime_s )) ;

    switch ( lState )
    {
        case deviceLimbo:
        case deviceConnectable:
        case deviceConnDiscoverable:
        case deviceConnected:
            /* Allow auto power off in all the above device states. since theSink.routed_audio NULL and this will set the 
              * shutdown to TRUE and therefore there is no BREAK here and allow to FALL TRHOUGH */
        case deviceA2DPStreaming:
            /* Is connected to any Audio Source. */
            if ( theSink.routed_audio )
            {
                if(theSink.silence_detected)
                {
                    /* Media is connected to source but while streaming silence detected.*/
                    shutdown = TRUE;
                }
             }
            else
            {
                /* Media is not streaming through any of the audio source so allow auto power off. */
                 shutdown = TRUE;
            }
            break;

         /* Don't allow auto power off while during call. */
        case deviceOutgoingCallEstablish:
        case deviceIncomingCallEstablish:
        case deviceActiveCallSCO:
        case deviceActiveCallNoSCO:
        case deviceTestMode:
        case deviceThreeWayCallWaiting:
        case deviceThreeWayCallOnHold:
        case deviceThreeWayMulticall:
        case deviceIncomingCallOnHold:
            break ;

        default:
            break ;
    }

    /* Before power down check is any BLE connection exist. */
    if(shutdown)
    {
#ifndef HYDRACORE_TODO
        /* Update the shutdown value based on the BLE connection status. */
        if (!gattClientHasClients() && gattServerIsFullyDisconnected())
        {
            /* No BLE connection exist so allow auto power off. */
            AUTO_POWER_OFF_DEBUG(("APO: NO active BLE connection \n"));
        }
        else
        {
            /* BLE connection exist hence postponed auto power off. */
            AUTO_POWER_OFF_DEBUG(("APO: Auto Switch Off cancelled due to active BLE connection \n"));
            shutdown = FALSE;
        }
#endif /* HYDRACORE_TODO */
    }

#ifdef ENABLE_GAIA
    /* Look for GAIA connection exist or not. */
    if(shutdown)
    {
        /* Don't allow shutdown if it is connected to any GAIA application. */
        if((theSink.rundata->gaia_data.gaia_transport != NULL) && (!theSink.features.GaiaRemainConnected))
        {
            shutdown = FALSE;
        }
    }
#endif

    /* If application is not in the process of link loss recovery, 
     * then process automatic power-down request else postponed 
     */
#ifdef ENABLE_PEER
    if(theSink.a2dp_link_data->peer_link_loss_reconnect)
    {
        /*TWS linkloss recovery is in progress,hence postpone
          the automatic powerdown*/      
        shutdown = FALSE;        
    }
#endif
    if (shutdown)
    {
        AUTO_POWER_OFF_DEBUG(("APO: Sending EventUsrPowerOff\n"));

        MessageSend(task, EventUsrPowerOff , 0);
        return FALSE;
    }
    return TRUE;      /* Postponed the Auto Power Off. */
}

/*************************************************************************
NAME
    sinkStartAutoPowerOffTimer

DESCRIPTION
    This function starts the auto power OFF timer. This cancels all the previous 
    EventSysAutoSwitchOff messages and sends a new one after AutoSwitchOffTime_s
    delay.

RETURNS
    VOID

**************************************************************************/

void sinkStartAutoPowerOffTimer(void)
{
    AUTO_POWER_OFF_DEBUG(("APO: Starting auto power OFF timer \n"));
    if(theSink.conf1->timeouts.AutoSwitchOffTime_s !=0)
    {
        MessageCancelAll(&theSink.task, EventSysAutoSwitchOff);
        sinkSendLater(EventSysAutoSwitchOff, D_SEC(theSink.conf1->timeouts.AutoSwitchOffTime_s));
    }
}

/*************************************************************************
NAME
    sinkStopAutoPowerOffTimer

DESCRIPTION
    This function stops the auto power OFF timer. This cancels all the  
    EventSysAutoSwitchOff messages sent.

RETURNS
    VOID

**************************************************************************/

void sinkStopAutoPowerOffTimer(void)
{
    AUTO_POWER_OFF_DEBUG(("APO: Stopping auto power OFF timer \n"));
    if(theSink.conf1->timeouts.AutoSwitchOffTime_s !=0)
    {
        MessageCancelAll(&theSink.task, EventSysAutoSwitchOff);
    }
}

