/****************************************************************************
Copyright (c) 2011 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.1

FILE NAME
    dm_ble_random_address.h      

DESCRIPTION
    This file contains the prototypes for BLE DM Random Address functions.

NOTES

*/

#include "connection.h"
#include "connection_private.h"

/****************************************************************************
NAME    
    ConnectionDmBleConfigureLocalAddressReq

DESCRIPTION
    Configure the local device address used for BLE connections

RETURNS
   void
*/
void connectionHandleDmSmConfigureLocalAddressCfm(
        const DM_SM_CONFIGURE_LOCAL_ADDRESS_CFM_T* cfm
        );
