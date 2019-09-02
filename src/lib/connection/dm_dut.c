/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.1

FILE NAME
    dm_dut.c        

DESCRIPTION    

NOTES

*/


/****************************************************************************
    Header files
*/
#include "connection.h"
#include "connection_private.h"


/*****************************************************************************/
void ConnectionEnterDutMode(void)
{    
    /* All requests are sent through the internal state handler */    
    MessageSend(connectionGetCmTask(), CL_INTERNAL_DM_DUT_REQ, 0);
}

