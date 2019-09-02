/****************************************************************************
Copyright (c) 2015 - 2016 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_gatt_client.c

DESCRIPTION
    Routines to handle the GATT Clients.
*/

#include "sink_gatt_client.h"

#include "sink_gatt.h"
#include "sink_ble.h"
#include "sink_debug.h"
#include "sink_gatt_client_battery.h"
#include "sink_gatt_client_ancs.h"
#include "sink_gatt_client_ias.h"
#include "sink_gatt_client_hid.h"
#include "sink_gatt_client_dis.h"
#include "sink_gatt_client_gatt.h"
#include "sink_gatt_client_spc.h"
#include "sink_gatt_client_hrs.h"
#include "sink_gatt_device.h"
#include "sink_private.h"
#include "sink_gatt_common.h"

#include <vm.h>
#include <gatt_manager.h>

#include <message.h>


#ifdef GATT_ENABLED

#ifdef DEBUG_GATT_CLIENT
#define GATT_CLIENT_DEBUG(x) DEBUG(x)
#else
#define GATT_CLIENT_DEBUG(x) 
#endif


#define GATT_CLIENT_DISCOVERY_ALLOC_MEM(ptr) \
{ \
    ptr = PanicUnlessMalloc(sizeof(gatt_client_discovery_t) * MAX_GATT_CLIENT_SERVICES); \
    memset(ptr, 0, sizeof(gatt_client_discovery_t) * MAX_GATT_CLIENT_SERVICES); \
    GATT_CLIENT_DEBUG(("Mem Alloc (Gatt Client - Discovery): size[%u] addr[%p]\n", \
                       sizeof(gatt_client_discovery_t) * MAX_GATT_CLIENT_SERVICES, \
                       (void *)ptr)); \
}

#define GATT_CLIENT_SERVICES_ALLOC_MEM(ptr) \
{ \
    ptr = PanicUnlessMalloc(sizeof(gatt_client_services_t)); \
    memset(ptr, 0, sizeof(gatt_client_services_t)); \
    GATT_CLIENT_DEBUG(("Mem Alloc (Gatt Client - Services): size[%u] addr[%p]\n", \
                       sizeof(gatt_client_services_t), \
                       (void *)ptr)); \
}

#define GATT_CLIENT_FREE_MEM(ptr) \
{ \
    GATT_CLIENT_DEBUG(("Mem Free (Gatt Client): addr[%p]\n", \
                       (void *)ptr)); \
    free(ptr); \
    ptr = NULL; \
}

/* Check security required for current connection */
#define GATT_CLIENT_IS_SEC_REQUIRED(discover) ((discover) && \
        (discover->service == gatt_client_ancs || discover->service == gatt_client_ias || discover->service == gatt_client_hid))

/****************************************************************************/
static uint16 gattClientFindByConnection(gatt_client_connection_t *conn)
{
    uint16 index;
    for(index=0;index < MAX_BLE_CONNECTIONS ;index++)
    {
        if(&GATT_CLIENT.connection == conn)
        {
           return GATT[index].cid;
        }
    }
    return GATT_CLIENT_INVALID_CID;
}

/****************************************************************************
NAME    
    gattClientDiscoveryComplete
    
DESCRIPTION
    Called when discovery is complete.
    
PARAMETERS
    connection  The GATT connection
    
NOTE
    This function MUST be called with a non-NULL *connection argument.
*/
static void gattClientDiscoveryComplete(gatt_client_connection_t *connection)
{
    gatt_client_services_t *data = gattClientGetServiceData(connection);

    /* Panic if Connection is NULL */
    PanicNull(connection);
    
    GATT_CLIENT_DEBUG(("GATT Client Discovery Complete\n"));
    
    if (data)
    {
        GATT_CLIENT_FREE_MEM(data->discovery);
    }
    
    if(connection->role == ble_gap_role_central)
    {
        /* Connection is now complete after setting security */
        sinkBleMasterConnCompleteEvent(gattClientFindByConnection(connection));
    }
    else
    {
        /* in case of peripheral, we need to post slave connected complete */
        sinkBleSlaveConnIndEvent(gattClientFindByConnection(connection));
    }
}


/****************************************************************************
NAME    
    initialiseGattClientService
    
DESCRIPTION
    Called to initialise the GATT service before all other services.
    This is so Service Changed indications can be captured.
    
PARAMETERS
    connection  The connection pointer

*/
static void initialiseGattClientService(gatt_client_connection_t *connection)
{
    gatt_client_discovery_t *discover = gattClientGetDiscoveredServices(connection);
    gatt_client_services_t *data = gattClientGetServiceData(connection);
    bool gatt_service_found = FALSE;

    if (discover && data)
    {
        gatt_service_found = gattClientIsServiceAvailable(connection, gatt_client_gatt);
        if(gatt_service_found)
        {
            gatt_service_found = sinkGattClientServiceAdd(gattClientFindByConnection(connection), 
                                                                                discover->start_handle, discover->end_handle);
            GATT_CLIENT_DEBUG(("Add gatt client service; success[%u] cid[0x%x] start[0x%x] end[0x%x]\n", 
                               gatt_service_found,
                               gattClientFindByConnection(connection), 
                               discover->start_handle,
                               discover->end_handle));
        }
        
        if (!gatt_service_found)
        {
            /* An error has occurred trying to process GATT service.
               Attempt to continue operation and check for security requirements. */
            gattClientProcessSecurityRequirements(connection, data);
        }
    }
}

/****************************************************************************
NAME    
    gattClientIsServiceAvailable
    
DESCRIPTION
    This function is used to find out if the client connection supports the particular service.
    
PARAMETERS
    connection  The connection pointer
    service The service which needs to be checked for 

RETURNS
    TRUE if the service is available, FALSE otherwise

*/
bool gattClientIsServiceAvailable(gatt_client_connection_t *connection, gatt_client_tasks_t service)
{
    gatt_client_discovery_t *discover = gattClientGetDiscoveredServices(connection);
    gatt_client_services_t *data = gattClientGetServiceData(connection);
    uint16 service_count = 0;

    if (discover && data)
    {
        for (service_count = 0 ; service_count < data->number_discovered_services ; service_count++)
        {
            if ((discover) && (discover->service == service))
            {
                return TRUE;
            }
            /* Increment to next discovery data */
            discover +=1;
        }
    }
    return FALSE;
}

/****************************************************************************
NAME    
    gattDiscoverPrimaryServices
    
DESCRIPTION
    Discovers GATT primary services supported on the remote device.
    
PARAMETERS
    connection             The GATT connection

*/
void gattDiscoverPrimaryServices(gatt_client_connection_t *connection)
{
    GattDiscoverAllPrimaryServicesRequest(sinkGetBleTask(), gattClientFindByConnection(connection));
}

/****************************************************************************/
gatt_client_discovery_t *gattClientGetDiscoveredServices(gatt_client_connection_t *connection)
{
    gatt_client_services_t *service = NULL;
    
    if (connection)
    {
        service = gattClientGetServiceData(connection);
        
        if (service)
        {
            return service->discovery;
        }
    }
    
    return NULL;
}

/****************************************************************************/
void gattClientInitialiseDiscoveredServices(gatt_client_connection_t *connection)
{
    gatt_client_discovery_t *discover = gattClientGetDiscoveredServices(connection);
    gatt_client_services_t *data = gattClientGetServiceData(connection);
    uint16 cid = gattClientFindByConnection(connection);
    bool error_flag = TRUE;

    if (discover && data)
    {
        /* Advance the discover pointer based on the current discovered service */
        discover += data->current_discovered_service;
        if(discover)
        {
            GATT_CLIENT_DEBUG(("GATT Discovered Service: cid[0x%x] index[%u]\n", cid, data->current_discovered_service));
            
            GATT_CLIENT_DEBUG(("    service[%u] start[0x%x] end[0x%x]\n", discover->service, discover->start_handle, discover->end_handle));
            switch (discover->service)
            {
                case gatt_client_battery:
                {
                    if (gattBatteryClientAddService(cid, discover->start_handle, discover->end_handle))
                    {
                        error_flag = FALSE;
                    }
                }
                break;
                case gatt_client_ancs:
                {
                    if (sinkGattAncsClientAddService(cid, discover->start_handle, discover->end_handle))
                    {
                        error_flag = FALSE;
                    }
                }
                break;
                case gatt_client_ias:
                {
                    if (sinkGattIasClientAddService(cid, discover->start_handle, discover->end_handle))
                    {
                        error_flag = FALSE;
                    }
                }
                break;
                case gatt_client_hid:
                {
                    if (sinkGattHidClientAddService(cid, discover->start_handle, discover->end_handle))
                    {
                        error_flag = FALSE;
                    }
                }
                break;
                case gatt_client_dis:
                {
                    if (sinkGattDisClientAddService(cid, discover->start_handle, discover->end_handle))
                    {
                        error_flag = FALSE;
                    }
                }
                break;

                case gatt_client_spc:
                {
                    if (sinkGattSpClientAddService(cid, discover->start_handle, discover->end_handle))
                    {
                        error_flag = FALSE;
                    }
                }
                break;

                case gatt_client_hrs:
                {
                    if (sinkGattHrsClientAddService(cid, discover->start_handle, discover->end_handle))
                    {
                        error_flag = FALSE;
                    }
                }
                break;
                
                case gatt_client_gatt:
                {
                    /* Gatt service will already have been initialised before other services.
                       Send internal message to process next service */
                    MESSAGE_MAKE(message, BLE_INTERNAL_MESSAGE_GATT_CLIENT_SERVICE_INITIALISED_T);
                    message->cid = cid;
                    MessageSend(sinkGetBleTask(), BLE_INTERNAL_MESSAGE_GATT_CLIENT_SERVICE_INITIALISED, message);
                    error_flag = FALSE;
                }
                break;
                default:
                {
                    GATT_CLIENT_DEBUG(("GATT unknown discovered service\n"));
                }
                break;
            }
        }
    }
    if (error_flag)
    {
        MESSAGE_MAKE(message, BLE_INTERNAL_MESSAGE_GATT_CLIENT_DISCOVERY_ERROR_T);
        /*message requires the pointer to gatt_client_connection_t as cid is  found using this address in gattClientFindByConnection() later on*/
        message->connection = connection;
        MessageSend(sinkGetBleTask(), BLE_INTERNAL_MESSAGE_GATT_CLIENT_DISCOVERY_ERROR, message);
    }
} 


/****************************************************************************/
bool gattClientAdd(uint16 cid, ble_gap_role_t client_gap_role)
{
    gatt_client_connection_t *connection = NULL;
    gatt_client_services_t *services = NULL;
    tp_bdaddr *tp_addr = NULL;
    uint16 index = GATT_INVALID_INDEX;

    if(client_gap_role == ble_gap_role_central)
    {
        /* Master connect flag shall be set as soon as a connection has been triggered in central role
         * However @ that moment "cid" is not available, so its now time to update
         * the "cid". After this, when there is disconnect, we can reset the conn flags
         * using the cid 
         * Please note that for gap_role_peripheral, the conn flag for master connection attempt
         * is not set, because there is no trigger of master_connection_attempt. So in peripheral 
         * role, this flag shall not be set */
        tp_addr = PanicUnlessMalloc(sizeof(tp_bdaddr));
        if(VmGetBdAddrtFromCid(cid, tp_addr))
        {
            index = sinkBleGapFindGapConnFlagIndex(&tp_addr->taddr);
        }

        /*In case of central, check and update the  connection id*/
        if(!gattCommonAddConnections(cid, ble_gap_role_central))
        {
            /* So, could not add the connection, then reset the connection attempt flag */
            sinkBleGapSetCentralConnAttempt(FALSE, NULL, 0, index);
             /* Free the allocated memory */
            free(tp_addr);
            tp_addr = NULL;
            
            return FALSE;
        }
        /* Okay, was able to add the  connection, now if we run out of memory while allocation for
            discovery pointer, then the caller shall trigger disconnect of the link, on recieving disconnect 
            indication we shall reset the connection attempt flag */
        sinkBleGapSetCentralConnAttempt(TRUE, &tp_addr->taddr, cid, index);
        /* Free the allocated memory */
        free(tp_addr);
        tp_addr = NULL;
    }

    /* Get the index to point to the correct GATT_CLIENT */
    index = gattCommonConnectionFindByCid(cid);
    connection = &(GATT_CLIENT.connection);

    GATT_CLIENT_SERVICES_ALLOC_MEM(services);

                
    if (services)
    {   
        /* Record details of the services */
        connection->data = services;

        /* Set the connection role */
        connection->role = client_gap_role;
        
        GATT_CLIENT_DISCOVERY_ALLOC_MEM(services->discovery);
        
        if (services->discovery)
        {
            /* Set MTU and kick off discovery once MTU configured */
            GattExchangeMtuRequest(sinkGetBleTask(), cid, SINK_GATT_MTU_MIN);
            return TRUE;
        }
        else
        {
            GATT_CLIENT_FREE_MEM(services);
        }
    }
    
    return FALSE;
}


/****************************************************************************/
gatt_client_connection_t *gattClientFindByCid(uint16 cid)
{
    uint16 index = 0;

    index =  gattCommonConnectionFindByCid(cid);
    if(index != GATT_INVALID_INDEX)
    {
        return &(GATT_CLIENT.connection);
    }
    return NULL;
}

/****************************************************************************/
bool gattClientRemove(uint16 cid)
{
    gatt_client_connection_t *connection = gattClientFindByCid(cid);

    if (connection && BLE.number_connections)
    {
        /* if central role, then reset the cid */
        if(connection->role == ble_gap_role_central)
        {
            gattCommonRemoveConnections(cid);
        }
        
        connection->role = ble_gap_role_unknown;
        if (connection->data)
        {
            gatt_client_discovery_t *discover = gattClientGetDiscoveredServices(connection);
            if (discover)
            {
                GATT_CLIENT_FREE_MEM(discover);
            }
            GATT_CLIENT_FREE_MEM(connection->data);
        }
        return TRUE;
    }
    
    return FALSE;
}


/****************************************************************************/
uint16 *gattClientAddService(gatt_client_connection_t *connection, uint16 size_service)
{
    if (connection)
    {
        gatt_client_services_t *services = connection->data;
        uint16 size_client_data = sizeof(gatt_client_services_t) + services->size_client_data + size_service;
        gatt_client_services_t *data = realloc(connection->data, size_client_data);
        
        if (data)
        {
            uint16 *end = data->client_data + services->size_client_data;
            data->size_client_data = size_client_data - sizeof(gatt_client_services_t);
            connection->data = data;
            
            return end;
        }   
    }
    
    return NULL;
}

/****************************************************************************\
 * NOTE: This utility function should be called only in case initialization of client library fails. If called in
 * other situation then the entire client_data shall be corrupted.
*****************************************************************************/
static void gattClientDeleteLastService(gatt_client_connection_t *connection, uint16 size_service)
{
    if (connection)
    {
        gatt_client_services_t *services = connection->data;
        uint16 size_client_data = sizeof(gatt_client_services_t) + services->size_client_data - size_service;
        gatt_client_services_t *data = realloc(connection->data, size_client_data);
        
        if (data)
        {
            data->size_client_data = size_client_data - sizeof(gatt_client_services_t);
            connection->data = data;
        }
    }
}

/****************************************************************************
NAME    
    gattClientGetNumServiceInstance
    
DESCRIPTION
    This function is used to find out the number of instance of a particular service
    
PARAMETERS
    connection  The connection pointer
    service The service which needs to be checked for 

RETURNS
    Number of instances

*/
static uint16 gattClientGetNumServiceInstance(gatt_client_connection_t *connection, gatt_client_tasks_t service)
{
    gatt_client_discovery_t *discover = gattClientGetDiscoveredServices(connection);
    gatt_client_services_t *data = gattClientGetServiceData(connection);
    uint16 service_count = 0;
    uint16 inst_count = 0;

    if (discover && data)
    {
        for (service_count = 0 ; service_count < data->number_discovered_services ; service_count++)
        {
            if ((discover) && (discover->service == service))
            {
                inst_count++;
            }
            /* Increment to next discovery data */
            discover +=1;
        }
    }
    return inst_count;
}

/****************************************************************************
NAME    
    gattClientCanAddClientService
    
DESCRIPTION
    This function is used to find out if the particular client service can be added or not based on
    number of instances
    
PARAMETERS
    connection  The connection pointer
    service The service which needs to be checked for 

RETURNS
    TRUE if possible, else FALSE

*/
static bool gattClientCanAddClientService(gatt_client_connection_t *connection, gatt_client_tasks_t service)
{
    switch(service)
    {
        /* Follow through cases which allows only one instance */
        case gatt_client_battery:
        case gatt_client_ancs:
        case gatt_client_ias:
        case gatt_client_dis:
        case gatt_client_gatt:
        case gatt_client_spc:
        case gatt_client_hrs:
        {
            if(gattClientGetNumServiceInstance(connection, service) < MAX_ONE_INSTANCE)
                return TRUE;
        }
        break;
        /* For hid we allow 2 instances */
        case gatt_client_hid:
        {
            if(gattClientGetNumServiceInstance(connection, service) < MAX_TWO_INSTANCE)
                return TRUE;
        }
        break;
    }
    return FALSE;
}

/****************************************************************************/
void gattClientStoreDiscoveredService(uint16 cid, uint16 uuid_type, uint32 *uuid, uint16 start, uint16 end, bool more)
{
    gatt_client_connection_t *connection = gattClientFindByCid(cid);
    gatt_client_discovery_t *discover = NULL;
    gatt_client_services_t *data = gattClientGetServiceData(connection);
    bool error_flag = TRUE;

    GATT_CLIENT_DEBUG(("Gatt Client Store Discovered Service \n"));

    if (connection && data)
    {
        discover = gattClientGetDiscoveredServices(connection);
        if (discover)
        {
            /* The discover poiter has to be advanced by the number of services found to update the new service */
            discover += data->number_discovered_services;
            if (discover)
            {
                /* Now check if we have not yet discovered max services */
                if(!HAS_MAX_SERVICES_DISCOVERED(data->number_discovered_services))
                {
                    if(uuid_type == gatt_uuid16)
                    {
                        switch (uuid[0])
                        {
                            case GATT_SERVICE_UUID_BATTERY_SERVICE:
                            {
                                /* As per CS-326396-DD connection spec, in peripheral role, 
                                    app should initialize only ANCS and IAS client */
                                if(sinkGattBatteryClientEnabled() && (ble_gap_role_central == connection->role) &&
                                   (gattClientCanAddClientService(connection, gatt_client_battery)))
                                {
                                    GATT_CLIENT_DEBUG(("Gatt Client Storing Battery service handles\n"));

                                    /* Store discovered battery service */
                                    discover->service = gatt_client_battery;
                                    discover->start_handle = start;
                                    discover->end_handle = end;
                                    
                                    data->number_discovered_services++;
                                }
                            }
                            break;

                            case GATT_SERVICE_UUID_HUMAN_INTERFACE_DEVICE:
                            {
                                if(sinkGattHidClientEnabled() && (ble_gap_role_central == connection->role) &&
                                   (gattClientCanAddClientService(connection, gatt_client_hid)))
                                {
                                    /* Store discovered HID service */
                                    discover->service = gatt_client_hid;
                                    discover->start_handle = start;
                                    discover->end_handle = end;
                                    
                                    data->number_discovered_services++;
                                }
                            }
                            break;
                            
                            case GATT_SERVICE_UUID_IMMEDIATE_ALERT:
                            {                   
                                GATT_CLIENT_DEBUG(("Gatt Client Storing Imm Alert service handles\n"));

                                if(sinkGattIasClientEnabled() && gattClientCanAddClientService(connection, gatt_client_ias))
                                {
                                    /* Store discovered Imm Alert service */
                                    discover->service = gatt_client_ias;
                                    discover->start_handle = start;
                                    discover->end_handle = end;
                                
                                    data->number_discovered_services++;
                                }
                            }
                            break;

                            case GATT_SERVICE_UUID_DEVICE_INFORMATION:
                            {                   
                                GATT_CLIENT_DEBUG(("Gatt Client Storing Device Info service handles\n"));

                                if(sinkGattDisClientEnabled() && gattClientCanAddClientService(connection, gatt_client_dis))
                                {
                                    /* Store discovered Device Info service */
                                    discover->service = gatt_client_dis;
                                    discover->start_handle = start;
                                    discover->end_handle = end;
                                    
                                    data->number_discovered_services++;
                                }
                            }
                            break;
                            case GATT_SERVICE_UUID_GENERIC_ATTRIBUTE:
                            {                   
                                GATT_CLIENT_DEBUG(("Gatt Client Storing GATT service handles\n"));
                                if(gattClientCanAddClientService(connection, gatt_client_gatt))
                                {
                                    /* Store discovered GATT service */
                                    discover->service = gatt_client_gatt;
                                    discover->start_handle = start;
                                    discover->end_handle = end;
                                    
                                    data->number_discovered_services++;
                                }
                            }
                            break;

                            case GATT_SERVICE_UUID_SCAN_PARAMETERS:
                            {                   
                                GATT_CLIENT_DEBUG(("Gatt Client Storing Scan params service handles\n"));

                                if(sinkGattSpClientEnabled() && gattClientCanAddClientService(connection, gatt_client_spc))
                                {
                                    /* Store discovered Scan params service */
                                    discover->service = gatt_client_spc;
                                    discover->start_handle = start;
                                    discover->end_handle = end;
                                    
                                    data->number_discovered_services++;
                                }
                            }
                            break;

                            case GATT_SERVICE_UUID_HEART_RATE:
                            {      
                                if(sinkGattHrsClientEnabled() && (ble_gap_role_central == connection->role) &&
                                   (gattClientCanAddClientService(connection, gatt_client_hrs)))
                                {
                                    GATT_CLIENT_DEBUG(("Gatt Client Storing Heart Rate params service handles\n"));

                                    /* Store discovered Heart rate params service */
                                    discover->service = gatt_client_hrs;
                                    discover->start_handle = start;
                                    discover->end_handle = end;
                                    
                                    data->number_discovered_services++;
                                }
                            }
                            break;
                            
                            /* Handle other services that client is interested in */
                            default:
                            {
                                /* Ignore unknown services */
                            }
                            break;
                        }
                    }
                    else if(uuid_type == gatt_uuid128)
                    {
                        if(CHECK_ANCS_SERVICE_UUID(uuid))
                        {
                            if(gattClientCanAddClientService(connection, gatt_client_ancs))
                            {
                                /* Store discovered apple notification service */
                                discover->service = gatt_client_ancs;
                                discover->start_handle = start;
                                discover->end_handle = end;
                                
                                data->number_discovered_services++;
                            }
                        }
                    }
                }

                if (!more)
                {
                    /* We are done with getting the primary service */
                    if(connection->role == ble_gap_role_peripheral)
                    {
                        sinkBleGapSetDiscoveryInProgress(FALSE, NULL, 0, sinkBleGapFindGapConnFlagIndexByCid(cid));
                    }
                    
                    if (data->number_discovered_services)
                    {
                        /* Initialise GATT service first to handle any Service Changed indications */
                        initialiseGattClientService(connection);
                    }
                    else
                    {
                        if(connection->role == ble_gap_role_central)
                        {
                            /* In case the client was connected in central role, then this client connection is not required
                             * because remote device does not have any service of our interest */
                             gattClientDisconnect(cid);
                        }
                        else
                        {
                            tp_bdaddr *tpaddrt = PanicUnlessMalloc(sizeof(tp_bdaddr));
                            /* We are in peripheral role, and found that the remote client doesn't
                             * support any service. However the remote client shall be using our services
                             * so we need to update the connection parameter */
                            if(VmGetBdAddrtFromCid(cid, tpaddrt))
                            {
                                sinkBleSetSlaveConnectionParamsUpdate(&tpaddrt->taddr);
                            }
                            free(tpaddrt);
                            /* In case the client was connected in peripheral role, then just remove the client
                             * Because server might be still connected */
                            gattClientRemove(cid);
                        }
                    }
                }
                error_flag = FALSE;
            }
        }
    }
    
    if (error_flag)
    {
        gattClientDiscoveryError(connection);
    }
}


/****************************************************************************/
gatt_client_services_t *gattClientGetServiceData(gatt_client_connection_t *connection)
{
    if (connection)
    {
        return connection->data;
    }
    
    return NULL;
}


/****************************************************************************/
void gattClientDiscoveredServiceInitialised(gatt_client_connection_t *connection)
{
    gatt_client_services_t *data = gattClientGetServiceData(connection);
    
    if (connection && connection->service_changed)
    {
        if (data)
        {
            GATT_CLIENT_FREE_MEM(data->discovery);
        }
        gattClientServiceChanged(gattClientFindByConnection(connection));
        
        /* Return early as discovery will be restarted */
        return;
    }
    
    if (data)
    {
        data->current_discovered_service++;
        if (data->current_discovered_service >= data->number_discovered_services)
        {
            GATT_CLIENT_DEBUG(("GATT Primary Services Initialised\n"));
            /* Discovery of primary services is now complete */
            gattClientDiscoveryComplete(connection);
        }
        else
        {
            gattClientInitialiseDiscoveredServices(connection);
        }
    }
    else
    {
        gattClientDiscoveryError(connection);
    }
}


/****************************************************************************/
bool gattClientRemoveDiscoveredService(gatt_client_connection_t *connection, 
                                       gatt_client_tasks_t service)
{
    bool retVal = FALSE;
    gatt_client_services_t *data = gattClientGetServiceData(connection);
    if (data)
    {
        /* Move to the last discovered service */
        switch(service)
        {
            case gatt_client_ancs:
            {
                if(data->ancs)
                {
                    sinkGattAncsClientRemoveService(data->ancs);
                    gattClientDeleteLastService(connection, sizeof(GANCS));
                    if(data && data->ancs)
                    {
                        data->ancs = NULL;
                    }
                    retVal = TRUE;
                }
            }
            break;

            case gatt_client_spc:
            {
                if(data->spc)
                {
                    sinkGattSpClientRemoveService(data->spc);
                    gattClientDeleteLastService(connection, sizeof(GSPC_T));
                    if(data && data->spc)
                    {
                        data->spc = NULL;
                    }
                    retVal = TRUE;
                }
            }
            break;
            
            case gatt_client_hrs:
            {
                if(data->hrs)
                {
                    sinkGattHrsClientRemoveService(data->hrs);
                    gattClientDeleteLastService(connection, sizeof(GHRSC_T));
                    if(data && data->hrs)
                    {
                        data->hrs = NULL;
                    }
                    retVal = TRUE;
                }
            }
            break;
            
            default:
            break;
        }
    }

    return retVal;
}

/****************************************************************************/
void gattClientDisconnect(uint16 cid)
{
    if (gattClientFindByCid(cid))
    {
        GattManagerDisconnectRequest(cid);
    }
}


/****************************************************************************/
bool gattClientDisconnectAll(void)
{
    return gattCommonDisconnectAll(TRUE);
}


/***************************************************************************/
void gattClientDiscoveryError(gatt_client_connection_t *connection)
{
    GATT_CLIENT_DEBUG(("GATT Client Discovery Error!\n"));

    if (connection)
    {
        /* Reset the primary service discovery flag */
        if(connection->role == ble_gap_role_peripheral)
        {
            uint16 cid = gattClientFindByConnection(connection);
            sinkBleGapSetDiscoveryInProgress(FALSE, NULL, 0, sinkBleGapFindGapConnFlagIndexByCid(cid));
        }
        gattClientDiscoveryComplete(connection);
    }
}

/***************************************************************************/
static uint16 gattClientGetNumberOfCentralConnected(void)
{
    uint16 central_conn = 0;
    uint16 index =0;

    /* Find the number of central device connected */
    for(index = 0; index < MAX_BLE_CONNECTIONS;index++)
    {
        if(GATT_CLIENT.connection.role == ble_gap_role_central)
            central_conn++;
    }
    return central_conn;
}

/***************************************************************************/
bool gattClientHasMaxCentralClients(void)
{
    /* If max_central_conn is 0, then we should not allow any central connection */
    if(gattCentralRoleMaxConnections())
    {
        /* If already all the client connection is used, then don't allow any more central connection.
          * Need to drop one of the client connection to allow any central connection */
        if(!gattCommonHasMaxConnections())
        {
            /* Get the total number of central devices connected */
            return (gattClientGetNumberOfCentralConnected() >= gattCentralRoleMaxConnections());
        }
    }
    return TRUE;
}

/***************************************************************************/
bool gattClientHasClients(void)
{
    return (gattClientGetNumberOfCentralConnected() != 0);
}


/***************************************************************************/
void gattClientRemoveServices(gatt_client_connection_t *client_connection)
{
    gatt_client_services_t *services = gattClientGetServiceData(client_connection);
    uint16 cid = gattClientFindByConnection(client_connection);
    
    /* Remove services if used */
    if (services)
    {
        if (services->basc)
        {
            gattBatteryClientRemoveService(services->basc, cid);
        }
        if(services->ancs)
        {
            sinkGattAncsClientRemoveService(services->ancs);
        }
        if(services->hidc_instance1)
        {
            sinkGattHIDClientRemoveService(services->hidc_instance1, cid);
        }
        if(services->hidc_instance2)
        {
            sinkGattHIDClientRemoveService(services->hidc_instance2, cid);
        }
        if(services->disc)
        {
            sinkGattDISClientRemoveService(services->disc, cid);
        }
        if(services->iasc)
        {
            sinkGattIasClientRemoveService(services->iasc);
        }
        if (services->gattc)
        {
            sinkGattClientServiceRemove(services->gattc, cid);
        }
        if(services->spc)
        {
            sinkGattSpClientRemoveService(services->spc);
        }
        if(services->hrs)
        {
            sinkGattHrsClientRemoveService(services->hrs);
        }
        /* Remove more services here */
    }            
}


/***************************************************************************/
void gattClientServiceChanged(uint16 cid)
{
    gatt_client_connection_t *connection = gattClientFindByCid(cid);
    gatt_client_services_t *services = gattClientGetServiceData(connection);
    
    if (connection)
    {
        connection->service_changed = TRUE;
        
        /* If service discovery is in progress then wait for discovery to complete
           before re-discovering services.
           Otherwise re-discover services immediately.
        */
        if (services && (services->discovery == NULL))
        {
            MESSAGE_MAKE(message, BLE_INTERNAL_MESSAGE_REDISCOVER_SERVER_SERVICES_T);
            
            /* Remove all services from client */
            gattClientRemoveServices(connection);
            GATT_CLIENT_FREE_MEM(connection->data);
            
            /* Send message to discover services again */
            message->cid = cid;
            MessageCancelFirst(sinkGetBleTask(), BLE_INTERNAL_MESSAGE_REDISCOVER_SERVER_SERVICES);
            MessageSend(sinkGetBleTask(), BLE_INTERNAL_MESSAGE_REDISCOVER_SERVER_SERVICES, message);
        }
    }
}


/***************************************************************************/
void gattClientRediscoverServices(uint16 cid)
{
    gatt_client_connection_t *connection = gattClientFindByCid(cid);
    gatt_client_services_t *services = NULL;
    bool error_flag = TRUE;
    
    if (connection == NULL)
        return;
    
    GATT_CLIENT_SERVICES_ALLOC_MEM(services);
    
    if (services)
    {
        /* Record details of the services */
        connection->data = services;
        
        /* Discover services again for this connection */
        GATT_CLIENT_DISCOVERY_ALLOC_MEM(services->discovery);
            
        if (services->discovery)
        {
            /* Discover primary services on this connection */
            gattDiscoverPrimaryServices(connection);
            error_flag = FALSE;
        }
    }
    
    if (error_flag)
    {
        /* Disconnect on error condition */
        gattClientDisconnect(cid);
    }
    
    connection->service_changed = FALSE;
}


/***************************************************************************/
void gattClientProcessSecurityRequirements(gatt_client_connection_t *connection, gatt_client_services_t *data)
{
   tp_bdaddr current_bd_addr;
   uint16 service_count= 0;
   bool is_security_required = FALSE;
   gatt_client_discovery_t *discover = gattClientGetDiscoveredServices(connection);
   uint16 cid = gattClientFindByConnection(connection);


    /* Here only pairing or encryption in case of central role is taken care and
    in case of peripheral role wait for bonding timeout to occur there by allowing
    for remote central device to pair and if remote device does not pair within the timeout 
    initiate encryption */

    for (service_count = 0 ; service_count < data->number_discovered_services ; service_count++)
    {
        /* Check any services required security */
        if(GATT_CLIENT_IS_SEC_REQUIRED(discover))
        {
            is_security_required = TRUE;
            break;
        }
        /* Get the next discovered service */
        discover+=1;
    }
     GATT_CLIENT_DEBUG(("gattClientProcessSecurityRequirements: is_security_required = %d\n", is_security_required));
    if(is_security_required)
    {
        if(connection->role == ble_gap_role_central)
        {
            if(VmGetBdAddrtFromCid(cid, &current_bd_addr))
            {
                /* Initiate Encryption request */
                GATT_CLIENT_DEBUG(("Gatt Initiate Encryption request \n"));
                
                ConnectionDmBleSecurityReq(sinkGetBleTask(), 
                                            (const typed_bdaddr *)&current_bd_addr.taddr, 
                                            ble_security_encrypted_bonded,
                                            ble_connection_master_directed
                                            );
            }
        }
        else
        {
            /* In case of already bonded peripheral device we shall not have bond timer to send encryption,
             * therefore send remote_connect_success event which shall trigger encryption.
             * On encryption success/failure we shall initialize client library accordingly */
            sinkBleSlaveConnIndEvent(cid);
        }
    }
    else
    {
        tp_bdaddr *tpaddrt = PanicUnlessMalloc(sizeof(tp_bdaddr));
        /* We are in peripheral role, and found that the remote client doesn't
         * support any service which require encryption. However the remote client shall be 
         * using our services so we need to update the connection parameter */
        if((connection->role == ble_gap_role_peripheral) && (VmGetBdAddrtFromCid(cid, tpaddrt)))
        {
            sinkBleSetSlaveConnectionParamsUpdate(&tpaddrt->taddr);
        }
        free(tpaddrt);
        /* If no security required i.e. discovered service does not require encryption 
            initialize discovered services?
        */
        gattClientInitialiseDiscoveredServices(connection);
    }
}

#endif /* GATT_ENABLED */

