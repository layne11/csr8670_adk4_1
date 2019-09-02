/*
Copyright (c) 2004 - 2016 Qualcomm Technologies International, Ltd.
*/

/*!
@file
@ingroup sink_app
@brief   
    Handles HFP indicators sent by the AG

*/

/****************************************************************************
    Header files
*/
#include "sink_private.h"
#include "sink_indicators.h"


/****************************************************************************
NAME    
    indicatorsHandleServiceInd
    
DESCRIPTION
    Interprets the service Indicator messages and sends the appropriate message 

RETURNS
    void
*/
void indicatorsHandleServiceInd ( const HFP_SERVICE_IND_T *pInd ) 
{
    /* only update the state if not set to network is present, this prevents repeated
       network is present indications from re-enabling the led's if they have gone to 
       sleep (timeout period) */  
    if(pInd->service != theSink.NetworkIsPresent)
    {
        if ( pInd->service )
        {
            MessageSend(&theSink.task , EventSysNetworkOrServicePresent , 0 ) ;
            theSink.NetworkIsPresent = TRUE ;
        }
        else /*the network service is OK*/
        {
            /*should only send this if not currently sending it*/
            if (theSink.NetworkIsPresent)
            {
                MessageSend(&theSink.task , EventSysNetworkOrServiceNotPresent  , 0 ) ;     
                theSink.NetworkIsPresent = FALSE ;
            }
        }
    }
}

