/*
Copyright (c) 2005 - 2016 Qualcomm Technologies International, Ltd.
*/

/*!
\file    
\ingroup sink_app
\brief   
    Handles AVRCP browsing channel connection/disconnection.
    Support for connection/disconnection of browsing channel is independent of browsing support. 
    So, even if browsing is not supported but if AVRCP CATEGORY-1 or CATEGORY-3 target is supported 
    then connection/disconnection of browsing channel must be supported.
    NOTE: Connection/disconnection of browsing channel functionality is independent of ENABLE_AVRCP_BROWSING
    
    The connection of the browsing channel will always be performed before sending a browsing command using the Request functions,
    so it is not neccessary to call sinkAvrcpBrowsingChannelConnectRequest directly.
    
    The disconnection of the browsing channel will always be made when the AVRCP control channel is disconnected. The browsing channel can
    also be removed after a period of inactivity by calling sinkAvrcpBrowsingDisconnectOnIdleRequest, where disconnection will occur
    if no commands are sent within the AVRCP_BROWSING_DISCONNECT_ON_IDLE_TIMER timeout.
*/
#ifdef ENABLE_AVRCP
    
#include "sink_avrcp_browsing_channel.h"
#include "sink_avrcp_browsing.h"
#include "sink_private.h"

#ifdef DEBUG_AVRCP_BROWSING_CHANNEL
#define AVRCP_BROWSING_CHANNEL_DEBUG(x) DEBUG(x)
#else
#define AVRCP_BROWSING_CHANNEL_DEBUG(x) 
#endif

/* browsing channel connection states */
typedef enum
{
    browsing_channel_connected,
    browsing_channel_disconnected,
    browsing_channel_pending,
    browsing_channel_disconnecting    
} browsing_channel_state;

/*************************************************************************
NAME    
    sinkAvrcpBrowsingChannelInit
    
DESCRIPTION
    Initialises the AVRCP Browsing Channel

**************************************************************************/
void sinkAvrcpBrowsingChannelInit(bool all_links, uint16 link_index)
{
    uint16 i = 0;
    uint16 start_index = 0;
    uint16 end_index = MAX_AVRCP_CONNECTIONS;
    
    if (!all_links)
    {
        start_index = link_index;
        end_index = start_index + 1;
    }
    for (i = start_index; i < end_index; i++)
    {
        theSink.avrcp_link_data->browsing_channel[i] = browsing_channel_disconnected;
        sinkAvrcpBrowsingInit(i);
    }
}

/*************************************************************************
NAME    
    sinkAvrcpBrowsingChannelConnectInd
    
DESCRIPTION
    Responds to incoming AVRCP Browsing Channel connection

**************************************************************************/
void sinkAvrcpBrowsingChannelConnectInd(const AVRCP_BROWSE_CONNECT_IND_T *msg)
{
    uint16 Index;
    bool accept = FALSE;
    
    if (sinkAvrcpGetIndexFromBdaddr(&msg->bd_addr, &Index, TRUE))
    {
        accept = TRUE;
        theSink.avrcp_link_data->browsing_channel[Index] = browsing_channel_pending;
        AVRCP_BROWSING_CHANNEL_DEBUG(("  accept\n"));
        AvrcpBrowseConnectResponse(theSink.avrcp_link_data->avrcp[Index], msg->connection_id, msg->signal_id, accept);
    }
    /* AvrcpBrowseConnectResponse takes AVRCP* as first param but AVRCP_BROWSE_CONNECT_IND doesn't contain this.
       A response cannot be sent if AVRCP* not found. 
    */
}

/*************************************************************************
NAME    
    sinkAvrcpBrowsingChannelDisconnectInd
    
DESCRIPTION
    AVRCP Browsing Channel has disconnected

**************************************************************************/
void sinkAvrcpBrowsingChannelDisconnectInd(const AVRCP_BROWSE_DISCONNECT_IND_T *msg)
{
    uint16 Index;
    
    if (sinkAvrcpBrowsingChannelGetIndexFromInstance(msg->avrcp, &Index))
    {
        browsing_channel_state state = theSink.avrcp_link_data->browsing_channel[Index];
        
        sinkAvrcpBrowsingChannelInit(FALSE, Index);
        sinkAvrcpBrowsingCancelDisconnectOnIdle(Index);
        if (state == browsing_channel_pending)
        {
            sinkAvrcpBrowsingChannelConnectRequest(msg->avrcp);
        }
        else
        {
            /* release AVRCP pointer if both AVRCP and Browsing connection are removed */
            if (!theSink.avrcp_link_data->connected[Index])
            {
                theSink.avrcp_link_data->avrcp[Index] = NULL;
            }
        }       
    }
}

/*************************************************************************
NAME    
    sinkAvrcpBrowsingChannelDisconnectRequest
    
DESCRIPTION
    Disconnects the AVRCP Browsing Channel Immediately. This will be called if the AVRCP control connection is removed.
    The function sinkAvrcpBrowsingChannelDisconnectInd will be called when the browsing channel has disconnected.

**************************************************************************/
void sinkAvrcpBrowsingChannelDisconnectRequest(AVRCP *avrcp)
{
    uint16 Index;
    
    if (sinkAvrcpBrowsingChannelGetIndexFromInstance(avrcp, &Index))
    {
        if (theSink.avrcp_link_data->browsing_channel[Index] == browsing_channel_connected)
        {
            AVRCP_BROWSING_CHANNEL_DEBUG(("AVRCP Browsing Disconnect Request\n"));
            theSink.avrcp_link_data->browsing_channel[Index] = browsing_channel_disconnecting;
            AvrcpBrowseDisconnectRequest(avrcp);
        }
    }
}

/*************************************************************************
NAME    
    sinkAvrcpBrowsingChannelConnectRequest
    
DESCRIPTION
    Connects the AVRCP Browsing Channel. A browsing channel will always be connected before sending a browsing command using functions in this file.
    The function sinkAvrcpBrowsingChannelConnectCfm will be called as confirmation of the connection attempt.

**************************************************************************/
void sinkAvrcpBrowsingChannelConnectRequest(AVRCP *avrcp)
{
    uint16 Index;
    
    if (sinkAvrcpGetIndexFromInstance(avrcp, &Index))
    {
        if (theSink.avrcp_link_data->browsing_channel[Index] == browsing_channel_disconnected)
        {
            theSink.avrcp_link_data->browsing_channel[Index] = browsing_channel_pending;
            AVRCP_BROWSING_CHANNEL_DEBUG(("AVRCP Browsing Connect Request\n"));
            AvrcpBrowseConnectRequest(avrcp, &theSink.avrcp_link_data->bd_addr[Index]);
        }
        else if (theSink.avrcp_link_data->browsing_channel[Index] == browsing_channel_disconnecting)
        {
            /* if disconnecting, set to pending so it can connect on disconnect */
            theSink.avrcp_link_data->browsing_channel[Index] = browsing_channel_pending;
        }
        
    }
}

/*************************************************************************
NAME    
    sinkAvrcpBrowsingChannelConnectCfm
    
DESCRIPTION
    AVRCP Browsing Channel connection outcome

**************************************************************************/
void sinkAvrcpBrowsingChannelConnectCfm(const AVRCP_BROWSE_CONNECT_CFM_T *msg)
{
    uint16 Index;
    
    if (sinkAvrcpBrowsingChannelGetIndexFromInstance(msg->avrcp, &Index))
    {
        if (msg->status == avrcp_success)
        {
            AVRCP_BROWSING_CHANNEL_DEBUG(("  success\n"));            
            theSink.avrcp_link_data->browsing_channel[Index] = browsing_channel_connected;            
            if (!theSink.avrcp_link_data->connected[Index] || (Index != sinkAvrcpGetActiveConnection()))
            {
                /* Disconnect Browsing channel if AVRCP has disconnected or is not active connection */
                sinkAvrcpBrowsingChannelDisconnectRequest(theSink.avrcp_link_data->avrcp[Index]);
            }
            else
            {
                /* register for UID changed notifications */
                sinkAvrcpBrowsingRegisterChangedUIDNotification(msg->avrcp);
            }
            return;
        }
        else
        {
            AVRCP_BROWSING_CHANNEL_DEBUG(("  fail %d\n", msg->status));
            if (theSink.avrcp_link_data->browsing_channel[Index] == browsing_channel_pending)
            {
                theSink.avrcp_link_data->browsing_channel[Index] = browsing_channel_disconnected;
                sinkAvrcpBrowsingFlushHandlerTask(Index);
            }
            /* release AVRCP pointer if both AVRCP and Browsing connection are removed */
            if (!theSink.avrcp_link_data->connected[Index])
            {
                theSink.avrcp_link_data->avrcp[Index] = NULL;
            }
        }
    }
}

/*************************************************************************
NAME    
    sinkAvrcpBrowsingChannelGetIndexFromInstance
    
DESCRIPTION
    Retrieve the correct AVRCP browsing connection index based on the AVRCP library instance pointer.
   
RETURNS
    Returns TRUE if the AVRCP browsing connection was found, FASLE otherwise.
    The actual connection index is returned in the Index variable.

**************************************************************************/
bool sinkAvrcpBrowsingChannelGetIndexFromInstance(AVRCP *avrcp, uint16 *Index)
{
    uint8 i;
    
    /* go through Avrcp instances looking for device_id match */
    for_all_avrcp(i)
    {
      
        /* if a device_id match is found return its value and a
               status of successful match found */
        if (avrcp && (theSink.avrcp_link_data->avrcp[i] == avrcp))
        {
            *Index = i;
            AVRCP_BROWSING_CHANNEL_DEBUG(("AVRCP: browsing getIndex = %d\n", i)); 
            return TRUE;
        }
    }
    /* no matches found so return not successful */    
    return FALSE;
}

/*************************************************************************
NAME    
    sinkAvrcpBrowsingChannelIsConnected
    
DESCRIPTION
    Utility function to check if the browsing channel is connected.
   
RETURNS
    Returns TRUE if the AVRCP browsing connection is connected, FASLE otherwise.

**************************************************************************/
bool sinkAvrcpBrowsingChannelIsConnected(uint16 index)
{
    if(index < MAX_AVRCP_CONNECTIONS)
    {
        if( theSink.avrcp_link_data->browsing_channel[index] == browsing_channel_connected )
            return TRUE;
    }
    return FALSE;
}

/*************************************************************************
NAME    
    sinkAvrcpBrowsingChannelIsDisconnected
    
DESCRIPTION
    Utility function to check if the browsing channel is disconnected.
   
RETURNS
    Returns TRUE if the AVRCP browsing connection is disconnected, FASLE otherwise.

**************************************************************************/
bool sinkAvrcpBrowsingChannelIsDisconnected(uint16 index)
{
    if(index < MAX_AVRCP_CONNECTIONS)
    {
        if( theSink.avrcp_link_data->browsing_channel[index] == browsing_channel_disconnected )
            return TRUE;
    }
    return FALSE;
}

/*************************************************************************
NAME    
    sinkAvrcpBrowsingChannelSendMessageWhenConnected
    
DESCRIPTION
    Utility function to send the message only upon the browsing channel being connected.
   
RETURNS
    None

**************************************************************************/
void sinkAvrcpBrowsingChannelSendMessageWhenConnected(Task task, MessageId id, void* message, uint16 index)
{
    if(index < MAX_AVRCP_CONNECTIONS)
    {
        MessageSendConditionally(task, id, message, &theSink.avrcp_link_data->browsing_channel[index]);
    }
    else
    {
        /* Invalid index has been sent, something majorly has gone wrong. We need to panic */
        Panic();
    }
}

#endif /* ENABLE_AVRCP */
