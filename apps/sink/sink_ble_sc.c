/****************************************************************************
Copyright (c) 2016 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_ble_sc.c

DESCRIPTION
    BLE SC functionality
*/

#include <vm.h>
#include "sink_ble_sc.h"

#ifdef GATT_ENABLED

#ifdef DEBUG_BLE_SC
#define BLE_SC_INFO(x) DEBUG(x)
#else
#define BLE_SC_INFO(x)
#endif

/******************************************************************************/
void sinkBleLinkSecurityInd(const CL_SM_BLE_LINK_SECURITY_IND_T *ind)
{
    sink_attributes attributes;
        
    deviceManagerClearAttributes(&attributes);
    deviceManagerGetAttributes(&attributes, &ind->bd_addr);    
    attributes.le_mode = ind->le_link_sc ? sink_le_secure_connection_mode: sink_no_le_secure_connection;
    deviceManagerStoreAttributes(&attributes, &ind->bd_addr);    
}


/******************************************************************************/
static bool sinkBleLeLinkIsSecure(const typed_bdaddr *tp_addr)
{
    sink_attributes attributes;
    tp_bdaddr public_addr;
    
    if(tp_addr->type == TYPED_BDADDR_RANDOM)
    {
        tp_bdaddr current_addr;
        current_addr.transport = TRANSPORT_BLE_ACL;
        memcpy(&current_addr.taddr,tp_addr, sizeof(typed_bdaddr));
        VmGetPublicAddress(&current_addr, &public_addr);        
    }
    else
    {
        public_addr.transport = TRANSPORT_BLE_ACL;
        memcpy(&public_addr.taddr,tp_addr, sizeof(typed_bdaddr));
    }
    
    deviceManagerClearAttributes(&attributes);
    
    /* Get the attributes for the given bd address*/
    deviceManagerGetAttributes(&attributes, &public_addr.taddr.addr);
    
    BLE_SC_INFO(("LE Link Mode:[%x]\n", attributes.le_mode));
    
    return attributes.le_mode;    
}

/******************************************************************************/
void sinkBleWriteApt(uint16 cid)
{    
    tp_bdaddr current_addr;
    
    if(cid != INVALID_CID)
    {
        if(VmGetBdAddrtFromCid(cid,&current_addr))
        {
            if(sinkBleLeLinkIsSecure(&current_addr.taddr))
            {
                BLE_SC_INFO(("LE Apt Write\n"));
                
                /* Write the APT value to the controller for the link */
                ConnectionWriteAPT(&theSink.task, &current_addr,theSink.conf1->timeouts.LeAuthenticatedPayloadTO_s, cl_apt_bluestack);
            }
        }
    }
}

/******************************************************************************/
void sinkBleSetLeLinkIndication(const typed_bdaddr  *tp_addr)
{ 
    if(sinkBleLeLinkIsSecure(tp_addr))
    {
       /* Notify the app that LE link is secure */
       MessageSend(&theSink.task, EventSysLESecureLink, 0);
    }
}


/******************************************************************************/
void sinkHandleBleEncryptionChangeInd(const CL_SM_ENCRYPTION_CHANGE_IND_T* ind)
{
    if(ind->encrypted && ind->tpaddr.transport == TRANSPORT_BLE_ACL)
    {
        /* if the link is encrypted and transport is TRANSPORT_BLE_ACL,
        * inform the user that LE link is Secure (i.e. link is encrypted 
        * using AES-CCM encryption) if LE pairing was Secure
        */
        sinkBleSetLeLinkIndication(&ind->tpaddr.taddr);
    }
}
#endif /* GATT_ENABLED */

