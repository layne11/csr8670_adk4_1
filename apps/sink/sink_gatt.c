/****************************************************************************
Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_gatt.c        

DESCRIPTION
    Contains GATT functionality.
*/
#include "sink_gatt.h"

#include "sink_development.h"
#include "sink_gatt_client.h"
#include "sink_gatt_device.h"
#include "sink_gatt_server_gap.h"
#include "sink_debug.h"
#include "sink_private.h"
#include "sink_ble_advertising.h"

#include <gatt.h>
#include <connection_no_ble.h>

#include <csrtypes.h>
#include <vm.h>

#ifdef GATT_ENABLED


#ifdef DEBUG_GATT
#define GATT_INFO(x) DEBUG(x)
#define GATT_ERROR(x) DEBUG(x) TOLERATED_ERROR(x)
#else 
#define GATT_INFO(x) 
#define GATT_ERROR(x) 
#endif


/*******************************************************************************
NAME
    handleGattConnectInd
    
DESCRIPTION
    Handle when GATT_CONNECT_IND message was received
    
PARAMETERS
    ind Pointer to a GATT_CONNECT_IND message
*/
static void handleGattConnectInd(const GATT_CONNECT_IND_T * ind)
{
    GATT_INFO(("GATT_CONNECT_IND - Rejected\n"));
    /* Reject GATT connection over BR/EDR */
    GattConnectResponse(&BLE.task, ind->cid, ind->flags, FALSE);
}


/*******************************************************************************
NAME
    handleGattConnectCfm
    
DESCRIPTION
    Handle when GATT_CONNECT_CFM message was received
    
PARAMETERS
    cfm Pointer to a GATT_CONNECT_CFM_T message
*/
static void handleGattConnectCfm(const GATT_CONNECT_CFM_T * cfm)
{
    GATT_INFO(("GATT_CONNECT_CFM - Ignored\n"));
}


/*******************************************************************************
NAME
    handleGattExchangeMtuInd
    
DESCRIPTION
    Handle when GATT_EXCHANGE_MTU_IND message was received
    
PARAMETERS
    ind Pointer to a GATT_EXCHANGE_MTU_IND message
    
RETURNS
    TRUE if the message was handled, FALSE otherwise
*/
static void handleGattExchangeMtuInd(const GATT_EXCHANGE_MTU_IND_T * ind)
{
    GATT_INFO(("GATT_EXCHANGE_MTU_IND\n"));
    if (ind)
    {
        GattExchangeMtuResponse(ind->cid, SINK_GATT_MTU_MIN);
    }
}

/*******************************************************************************

    handleGattExchangeMtuCfm
            
DESCRIPTION
    Handle when GATT_EXCHANGE_MTU_CFM message was received
    
PARAMETERS
    cfm Pointer to a GATT_EXCHANGE_MTU_CFM message
*/
static void handleGattExchangeMtuCfm(const GATT_EXCHANGE_MTU_CFM_T * cfm)
{
    uint8 index;
    tp_bdaddr *tp_addr = PanicUnlessMalloc(sizeof(tp_bdaddr));
    GATT_INFO(("GATT_EXCHANGE_MTU_CFM\n"));
    if (cfm)
    {
        gatt_client_connection_t *connection = gattClientFindByCid(cfm->cid);
        gatt_client_services_t *data = gattClientGetServiceData(connection);

        if(connection && data && data->number_discovered_services == 0)
        {
            /* Discover primary services on connection */
            gattDiscoverPrimaryServices(connection);
            /* Discovery in progress only for peripheral role,
              * as this flag is not used for central role */
            if(connection->role == ble_gap_role_peripheral)
            {
                index = sinkBleGapFindGapConnFlagIndex(NULL);
                VmGetBdAddrtFromCid(cfm->cid, tp_addr);
                sinkBleGapSetDiscoveryInProgress(TRUE, &tp_addr->taddr, cfm->cid, index);
            }
        }
    }
    /* Free the allocated memory */
    free(tp_addr);
}


/*******************************************************************************
NAME
    handleGattDiscoverAllPrimaryServicesCfm
    
DESCRIPTION
    Handle when GATT_DISCOVER_ALL_PRIMARY_SERVICES_CFM message was received
    
PARAMETERS
    cfm Pointer to a GATT_DISCOVER_ALL_PRIMARY_SERVICES_CFM message
    
RETURNS
    void
*/
static void handleGattDiscoverAllPrimaryServicesCfm(const GATT_DISCOVER_ALL_PRIMARY_SERVICES_CFM_T * cfm)
{
    GATT_INFO(("GATT_DISCOVER_ALL_PRIMARY_SERVICES_CFM\n"));
    
    GATT_INFO(("   cid[0x%x] Start[0x%x] End[0x%x] more[%u]\n", 
                                                                cfm->cid,
                                                                cfm->handle,
                                                                cfm->end,
                                                                cfm->more_to_come
                                                                ));
    
    gattClientStoreDiscoveredService(cfm->cid, cfm->uuid_type, (uint32*)&cfm->uuid[0], cfm->handle, cfm->end, cfm->more_to_come);
}


/******************************************************************************/
void sinkGattMsgHandler(Task task, MessageId id, Message message)
{
    switch(id)
    {
        case GATT_CONNECT_IND:
        {
            handleGattConnectInd((GATT_CONNECT_IND_T*)message);
        }
        break;
        case GATT_CONNECT_CFM:
        {
            handleGattConnectCfm((GATT_CONNECT_CFM_T*)message);
        }
        break;
        case GATT_EXCHANGE_MTU_IND:
        {
            handleGattExchangeMtuInd((GATT_EXCHANGE_MTU_IND_T*)message);
        }
        break;
        case GATT_EXCHANGE_MTU_CFM:
        {
            handleGattExchangeMtuCfm((GATT_EXCHANGE_MTU_CFM_T*)message);
        }
        break;
        case GATT_DISCOVER_ALL_PRIMARY_SERVICES_CFM:
        {
            handleGattDiscoverAllPrimaryServicesCfm((GATT_DISCOVER_ALL_PRIMARY_SERVICES_CFM_T*)message);
        }
        break;
        default:
        {
            GATT_ERROR(("GATT Unhandled msg[%x]\n", id));
        }
        break;
    }
}


#endif /* GATT_ENABLED */
