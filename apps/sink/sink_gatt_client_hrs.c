/****************************************************************************
Copyright (c) 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_gatt_client_hrs.c

DESCRIPTION
    Routines to handle the GATT Heart Rate Serivce Client - Collector Role.
*/
#include <stdlib.h>
#include <connection.h>
#include <gatt.h>

/* Include dependent Headers */
#include "sink_gatt_client.h"
#include "sink_ble.h"
#include "sink_private.h"
#include "sink_config.h"

/* Include Module Header */
#include "sink_gatt_client_hrs.h"

#include "sink_display.h"
#include "display_plugin_if.h"

#ifdef GATT_HRS_CLIENT

static const uint8 hrs_ble_advertising_filter[] = {GATT_SERVICE_UUID_HEART_RATE & 0xFF, GATT_SERVICE_UUID_HEART_RATE >> 8};

#ifdef DEBUG_GATT_HRS_CLIENT
#define GATT_HRS_CLIENT_DEBUG(x) DEBUG(x)
#else
#define GATT_HRS_CLIENT_DEBUG(x) 
#endif

/****************************STATIC FUNCTIONS************************************/

/*******************************************************************************
NAME
    gattHrsClientServiceInitialised
    
DESCRIPTION
    Called when the Heart rate client Service is initialised.
    
PARAMETERS
    ghrsc   The Heart Rate Service client instance pointer
    status  Status returned by GATT_HEART_RATE_CLIENT_INIT_CFM
    
*/
static void gattHrsClientServiceInitialised(const GHRSC_T *ghrsc, gatt_heart_rate_client_status status)
{
    uint16 index = 0;
    gatt_client_services_t *data = NULL;
    gatt_client_connection_t *client = NULL;
    
    for (index = 0; index < MAX_BLE_CONNECTIONS; index++)
    {
        client = &GATT_CLIENT.connection;
        data = gattClientGetServiceData(client);
        if (data && (data->hrs == ghrsc))
        {
            /* Heart Rate client library was not able to successfully initialize 
             * Remove the client from client_data structure */
            if(status == gatt_heart_rate_client_status_failed)
            {
                gattClientRemoveDiscoveredService(client, gatt_client_hrs);
            }
            gattClientDiscoveredServiceInitialised(client);
            return;
        }
    }
}

/*******************************************************************************
NAME
    gattHrsClientInitCfm
    
DESCRIPTION
    Handle the GATT_HEART_RATE_CLIENT_INIT_CFM message
    
PARAMETERS
    cfm    The GATT_HEART_RATE_CLIENT_INIT_CFM message
*/
static void gattHrsClientInitCfm(const GATT_HEART_RATE_CLIENT_INIT_CFM_T *cfm)
{
    GATT_HRS_CLIENT_DEBUG(("GATT_HEART_RATE_CLIENT_INIT_CFM status[%u], sensor location[%u], control point support[%u] \n", 
                            cfm->status, cfm->hr_sensor_location, cfm->hr_control_point_support));
    
    /* The service initialisation is complete */
    gattHrsClientServiceInitialised(cfm->heart_rate_client, cfm->status);

    if ((cfm->status==gatt_heart_rate_client_status_success) && (cfm->heart_rate_client!=NULL))   
    {
        /* Set notifications for getting Heart Rate measurment values*/
        GattHeartRateRegisterForNotification(cfm->heart_rate_client, TRUE);
    }
}

/*******************************************************************************
NAME
    gattHrsClientNotificationCfm
    
DESCRIPTION
    Handle the GATT_HEART_RATE_CLIENT_NOTIFICATION_CFM_T message
    
PARAMETERS
    cfm    The GATT_HEART_RATE_CLIENT_NOTIFICATION_CFM_T message
*/
static void gattHrsClientNotificationCfm(const GATT_HEART_RATE_CLIENT_NOTIFICATION_CFM_T *cfm)
{
    GATT_HRS_CLIENT_DEBUG(("GATT_HEART_RATE_CLIENT_NOTIFICATION_CFM_T status[%u]\n", cfm->status));
}

/*******************************************************************************
NAME
    gattHrsClientNotificationInd
    
DESCRIPTION
    Handle the GATT_HEART_RATE_CLIENT_NOTIFICATION_IND_T message
    
PARAMETERS
    ind    The GATT_HEART_RATE_CLIENT_NOTIFICATION_IND_T message
*/
static void gattHrsClientNotificationInd(const GATT_HEART_RATE_CLIENT_NOTIFICATION_IND_T *ind)
{   
    GATT_HRS_CLIENT_DEBUG(("SINK GATT_HEART_RATE_CLIENT_NOTIFICATION_IND_T Flags [0x%x] \n", ind->heart_rate_characteristic_flags));

    /* Print HR measurement value */
    GATT_HRS_CLIENT_DEBUG((" Heart rate measurement value [%u] bpm \n", ind->heart_rate_value));

    /*Inform user if heart rate value is beyond threshold*/
    if ((ind->heart_rate_value < theSink.features.BleHrsMinThreshold) || 
         (ind->heart_rate_value > theSink.features.BleHrsMaxThreshold))
    {   
        GATT_HRS_CLIENT_DEBUG(("HR VALUE THRESHOLD REACHED \n"));
        MessageSend(&theSink.task, EventSysHeartRateThresholdReached, 0);
    }

#ifdef DEBUG_GATT_HRS_CLIENT
    /* Print HR energy expended value */
    if (ind->heart_rate_characteristic_flags & HR_ENERGY_EXP_PRESENT)
    {
        GATT_HRS_CLIENT_DEBUG((" Heart rate energy expended value [0x%x] \n", ind->energy_expended));
    }
    else
    {            
        GATT_HRS_CLIENT_DEBUG((" Heart rate energy expended value not present \n"));
    }
    
    /* Print HR energy expended value */
    if (ind->heart_rate_characteristic_flags & HR_RR_INTERVAL_PRESENT)
    {    
        uint8 count=0;

        GATT_HRS_CLIENT_DEBUG((" (uint8) Size of HR RR interval [%u] \n", ind->size_rr_interval));

        for (count=0;count<ind->size_rr_interval;count++)
        {
            GATT_HRS_CLIENT_DEBUG(("HR RR interval value [0x%x]\n", ind->rr_interval[count]));
        }
    }    
    else
    {            
        GATT_HRS_CLIENT_DEBUG((" Heart rate RR interval not present \n"));
    }
#endif

    /* Display HR measurement value */
    displayShowText((char*)&ind->heart_rate_value,  sizeof(ind->heart_rate_value), 2, DISPLAY_TEXT_SCROLL_STATIC, 500, 1000, FALSE, 5);         
}


/****************************GLOBAL FUNCTIONS***********************************/

/****************************************************************************/
void sinkGattHrsClientSetupAdvertisingFilter(void)
{    
    GATT_HRS_CLIENT_DEBUG(("HRS Set up Adv filter\n"));
    ConnectionBleAddAdvertisingReportFilter(ble_ad_type_more_uuid16, sizeof(hrs_ble_advertising_filter), sizeof(hrs_ble_advertising_filter), hrs_ble_advertising_filter);
    ConnectionBleAddAdvertisingReportFilter(ble_ad_type_complete_uuid16, sizeof(hrs_ble_advertising_filter), sizeof(hrs_ble_advertising_filter), hrs_ble_advertising_filter);

}

/****************************************************************************/
bool sinkGattHrsClientAddService(uint16 cid, uint16 start, uint16 end)
{
    gatt_client_services_t *client_services = NULL;
    gatt_client_connection_t *connection = gattClientFindByCid(cid);
    uint16 *service = gattClientAddService(connection, sizeof(GHRSC_T));

    GATT_HRS_CLIENT_DEBUG(("Add Heart Rate Client Service\n"));
    if (service)
    {
        GATT_HEART_RATE_CLIENT_INIT_PARAMS_T params;
        GHRSC_T *ghrsc = NULL;
    
        client_services = gattClientGetServiceData(connection);
        client_services->hrs = (GHRSC_T *)service;
        ghrsc = client_services->hrs;
  
        params.cid = cid;
        params.start_handle = start;
        params.end_handle = end;
        
        if (GattHeartRateClientInit(sinkGetBleTask(), ghrsc, &params, NULL))
        {
            return TRUE;
        }
    }
    
    return FALSE;
}


/****************************************************************************/
void sinkGattHrsClientRemoveService(GHRSC_T *ghrsc)
{
    /* Deinit GATT HR client */
    GattHeartRateClientDeInit(ghrsc);
}

/******************************************************************************/
void sinkGattHrsClientMsgHandler(Task task, MessageId id, Message message)
{
    switch(id)
    {
        case GATT_HEART_RATE_CLIENT_INIT_CFM:
        {
            gattHrsClientInitCfm((GATT_HEART_RATE_CLIENT_INIT_CFM_T*)message);
        }
        break;
        
        case GATT_HEART_RATE_CLIENT_NOTIFICATION_CFM:
        {
            gattHrsClientNotificationCfm((GATT_HEART_RATE_CLIENT_NOTIFICATION_CFM_T*)message);
        }
        break;
        
        case GATT_HEART_RATE_CLIENT_NOTIFICATION_IND:
        {
            gattHrsClientNotificationInd((GATT_HEART_RATE_CLIENT_NOTIFICATION_IND_T*)message);
        }
        break;

        default:
        {
            GATT_HRS_CLIENT_DEBUG(("Unhandled Heart Rate Client msg [0x%x]\n", id));
        }
        break;
    }
}

#endif /* GATT_HRS_CLIENT */
