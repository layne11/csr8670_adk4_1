/****************************************************************************
Copyright (c) 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_gatt_client_spc.c

DESCRIPTION
    Routines to handle the GATT Scan Parameter Client.
*/
#include <stdlib.h>
#include <connection.h>
#include <gatt.h>

/* Include dependent Headers */
#include "sink_gatt_client.h"
#include "sink_ble.h"
#include "sink_private.h"

/* Include Module Header */
#include "sink_gatt_client_spc.h"

#ifdef GATT_SPC_CLIENT

static const uint8 spc_ble_advertising_filter[] = {GATT_SERVICE_UUID_SCAN_PARAMETERS & 0xFF, GATT_SERVICE_UUID_SCAN_PARAMETERS >> 8};

#ifdef DEBUG_GATT_SPC_CLIENT
#define GATT_SPC_CLIENT_DEBUG(x) DEBUG(x)
#else
#define GATT_SPC_CLIENT_DEBUG(x) 
#endif

/****************************STATIC FUNCTIONS************************************/

/*******************************************************************************
NAME
    gattSpClientServiceInitialised
    
DESCRIPTION
    Called when the Device Information Service is initialised.
    
PARAMETERS
    gspc    The Device Information Service client instance pointer
    status  Status returned by GATT_SCAN_PARAMS_CLIENT_INIT_CFM from gatt_scan_params_client_status enum
    
*/
static void gattSpClientServiceInitialised(const GSPC_T *gspc, gatt_scan_params_client_status status)
{
    uint16 index = 0;
    gatt_client_services_t *data = NULL;
    gatt_client_connection_t *client = NULL;
    
    for (index = 0; index < MAX_BLE_CONNECTIONS; index++)
    {
        client = &GATT_CLIENT.connection;
        data = gattClientGetServiceData(client);
        if (data && (data->spc == gspc))
        {
            /* Scan Params library was not able to successfully initialize 
             * Remove the Scan Params client from client_data structure */
            if(status == gatt_scan_params_client_status_failed)
            {
                gattClientRemoveDiscoveredService(&GATT_CLIENT.connection, gatt_client_spc);
            }
            gattClientDiscoveredServiceInitialised(client);
            return;
        }
    }
}

/*******************************************************************************
NAME
    gattSpClientInitCfm
    
DESCRIPTION
    Handle the GATT_SCAN_PARAMS_CLIENT_INIT_CFM message
    
PARAMETERS
    cfm    The GATT_SCAN_PARAMS_CLIENT_INIT_CFM message
*/
static void gattSpClientInitCfm(const GATT_SCAN_PARAMS_CLIENT_INIT_CFM_T *cfm)
{
    GATT_SPC_CLIENT_DEBUG(("GATT_SCAN_PARAMS_CLIENT_INIT_CFM status[%u]\n", cfm->status));
    /* The service initialisation is complete */
    gattSpClientServiceInitialised(cfm->scan_params_client, cfm->status);
}

/****************************GLOBAL FUNCTIONS***********************************/

/****************************************************************************/
void sinkGattSpClientSetupAdvertisingFilter(void)
{
    
    GATT_SPC_CLIENT_DEBUG(("GattSpc: Add Scan Params scan filter\n"));
    ConnectionBleAddAdvertisingReportFilter(ble_ad_type_more_uuid16, sizeof(spc_ble_advertising_filter), sizeof(spc_ble_advertising_filter), spc_ble_advertising_filter);
    ConnectionBleAddAdvertisingReportFilter(ble_ad_type_complete_uuid16, sizeof(spc_ble_advertising_filter), sizeof(spc_ble_advertising_filter), spc_ble_advertising_filter);
}

/****************************************************************************/
bool sinkGattSpClientAddService(uint16 cid, uint16 start, uint16 end)
{
    gatt_client_services_t *client_services = NULL;
    gatt_client_connection_t *connection = gattClientFindByCid(cid);
    uint16 *service = gattClientAddService(connection, sizeof(GSPC_T));

    GATT_SPC_CLIENT_DEBUG(("Add Scan Parameters Client Service\n"));
    if (service)
    {
        GATT_SCAN_PARAMS_CLIENT_INIT_PARAMS_T params;
        GSPC_T *gspc = NULL;
        ble_scanning_parameters_t scan_params;
    
        client_services = gattClientGetServiceData(connection);
        client_services->spc = (GSPC_T *)service;
        gspc = client_services->spc;

        /* Get the Initial scan parameters for fast scan 
            by default use slow scan parameters.
        */
        sinkBleGetScanningParameters(FALSE, &scan_params);
    
        params.cid = cid;
        params.start_handle = start;
        params.end_handle = end;
        params.scan_interval = scan_params.interval;
        params.scan_window = scan_params.window;
        
        if (GattScanParamsClientInit(sinkGetBleTask(), gspc, &params, NULL))
        {
            return TRUE;
        }
    }
    
    return FALSE;
}

/****************************************************************************/
void sinkGattSpClientSetScanIntervalWindow(bool fast_scan)
{
    ble_scanning_parameters_t scan_params;
    gatt_client_services_t *client_services = NULL;
    gatt_client_connection_t *connection = NULL;
    uint16 index = 0;

    /* Get the scan parameters for the current mode */
    sinkBleGetScanningParameters(fast_scan, &scan_params);

    GATT_SPC_CLIENT_DEBUG(("sinkGattSpClientSetScanIntervalWindow(): fast_scan %s\n", fast_scan?"TRUE": "FALSE"));
    /*Check if the remote device is connected*/
    for (index = 0; index < MAX_BLE_CONNECTIONS; index++)
    {
        if (GATT[index].cid)
        {
            connection = gattClientFindByCid(GATT[index].cid);
            
            /* Get Client services */
            client_services = gattClientGetServiceData(connection);

            if ((client_services != NULL) && (client_services->spc))
                {
                    /* Send the new Scan Interval and Window to remote scan server. The failure to 
                       update scan params can be ignored as it is not critical to device operation */
                    GattScanParamsSetIntervalWindow((GSPC_T *)client_services->spc, scan_params.interval, scan_params.window);
                }
            }
        }
}


/****************************************************************************/
void sinkGattSpClientRemoveService(GSPC_T *gspc)
{
    /* Deinit GATT SP client */
    GattScanParamsClientDeInit(gspc);
}

/******************************************************************************/
void sinkGattSpClientMsgHandler(Task task, MessageId id, Message message)
{
    switch(id)
    {
        case GATT_SCAN_PARAMS_CLIENT_INIT_CFM:
        {
            gattSpClientInitCfm((GATT_SCAN_PARAMS_CLIENT_INIT_CFM_T*)message);
        }
        break;

        default:
        {
            GATT_SPC_CLIENT_DEBUG(("Unhandled Scan Params Client msg [%x]\n", id));
        }
        break;
    }
}

#endif /* GATT_SPC_CLIENT */
