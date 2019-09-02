/****************************************************************************
Copyright (c) 2014 - 2016 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_ble.c

DESCRIPTION
    BLE functionality
*/

#include "sink_ble.h"

#include "sink_ble_advertising.h"
#include "sink_ble_scanning.h"
#include "sink_debug.h"
#include "sink_development.h"
#include "sink_private.h"
#include "sink_statemanager.h"
#include "sink_powermanager.h"


#include <connection.h>
#include <string.h>
#include <vm.h>

#ifdef GATT_ENABLED

#include <gatt.h>
#include <gatt_battery_client.h>
#include <gatt_battery_server.h>
#include <gatt_gap_server.h>
#include <gatt_manager.h>
#include <gatt_server.h>

#include "sink_gatt_common.h"
#include "sink_gatt.h"
#include "sink_gatt_client_battery.h"
#include "sink_gatt_client_gatt.h"
#include "sink_gatt_client_hid.h"
#include "sink_gatt_client_dis.h"
#include "sink_gatt_init.h"
#include "sink_gatt_manager.h"
#include "sink_gatt_server_battery.h"
#include "sink_gatt_server_ias.h"
#include "sink_gatt_client_ias.h"
#include "sink_gatt_server_lls.h"
#include "sink_gatt_server_hrs.h"
#include "sink_gatt_client_ancs.h"
#include "sink_gatt_client_spc.h"
#include "sink_gatt_client_hrs.h"
#include "sink_gatt_server_gap.h"
#include "sink_gatt_server_gatt.h"

#ifdef DEBUG_BLE
#define BLE_INFO(x) DEBUG(x)
#define BLE_ERROR(x) DEBUG(x) TOLERATED_ERROR(x)
#else
#define BLE_INFO(x)
#define BLE_ERROR(x)
#endif


#define DEVICE_IS_ON                        (stateManagerGetState() != deviceLimbo)
#define ENCRYPTION_RETRY_EVENT_DELAY_MS     500

/* The below values are considered from  ble_master_update_conn_params and 
     ble_slave_update_conn_params. */

#define SINK_BLE_CONN_PARAM_SUPERVISION_TIMEOUT					400		/* Supervision timeout */
#define SINK_BLE_CONN_PARAM_CE_LENGTH_MIN						0		/* Minimum length of the connection */
#define SINK_BLE_CONN_PARAM_CE_LENGTH_MAX						160		/* Maximum length of the connection */
#define SINK_BLE_CONN_PARAM_CONN_INTERVAL_MAX					88		/* Maximum connection interval */
#define SINK_BLE_CONN_PARAM_MASTER_CONN_INTERVAL_MIN			80		/* Master minimum connection interval */
#define SINK_BLE_CONN_PARAM_MASTER_CONN_LATENCY					8		/* Master connection latency */
#define SINK_BLE_CONN_PARAM_SLAVE_CONN_INTERVAL_MIN				72		/* Slave connection latency */
#define SINK_BLE_CONN_PARAM_SLAVE_CONN_LATENCY					4		/* Slave connection latency */
#define SINK_BLE_CONN_PARAM_MASTER_INIT_CONN_INTERVAL_MIN		24      /* Initial Minimum connection interval */
#define SINK_BLE_CONN_PARAM_MASTER_INIT_CONN_INTERVAL_MAX		40    	/* Initial maximum connection interval */
#define SINK_BLE_CONN_PARAM_MASTER_INIT_CONN_LATENCY        	0      	/* Initial connection latency */
#define SINK_BLE_CONN_PARAM_MASTER_INIT_CONN_ATTEMPT_TIMEOUT	50   	/* Initial connection attempt timeout */
#define SINK_BLE_CONN_PARAM_MASTER_INIT_CONN_LATENCY_MAX        64    	/* Maximum connection latency */
#define SINK_BLE_CONN_PARAM_MASTER_SUPERVISION_TIMEOUT_MIN      400   	/* Minimum supervision timeout */
#define SINK_BLE_CONN_PARAM_MASTER_SUPERVISION_TIMEOUT_MAX     	400   	/* Maximum supervision timeout */

/* Default BLE configuration */
static const ble_configuration_t ble_config = {
                                10,     /* Bondable Pairing timeout (secs) */
                                60,     /* Bondable Connection Timeout (secs) */
                                128,    /* Fast scan interval */
                                12,     /* Fast scan window */
                                120,    /* Gap mode switch timer */
                                10,     /* Time to scan for whitelist devices before reverting to general scanning, 
                                            if a private device has been paired with */
                                30,     /* Fast scan timer */
                                2048,   /* Slow scan interval */
                                18,     /* Slow scan window */
                                32,     /* Fast adv interval min */
                                48,     /* Fast adv interval max */
                                30,     /* Fast adv timer */
                                1000,   /* Slow adv interval min */
                                1200    /* Slow adv interval max */
                                
};


/* Default connection parameters when Master and initiating connection (parameters to allow quick connection/discovery of database) */
static const ble_connection_initial_parameters_t ble_master_initial_conn_params = {
    SINK_BLE_CONN_PARAM_MASTER_INIT_CONN_INTERVAL_MIN,		/* Minimum connection interval */ 
    SINK_BLE_CONN_PARAM_MASTER_INIT_CONN_INTERVAL_MAX,      /* Maximum connection interval */
    SINK_BLE_CONN_PARAM_MASTER_INIT_CONN_LATENCY,           /* Connection latency */
    SINK_BLE_CONN_PARAM_SUPERVISION_TIMEOUT,                /* Supervision timeout */
    SINK_BLE_CONN_PARAM_MASTER_INIT_CONN_ATTEMPT_TIMEOUT,   /* Connection attempt timeout */
    SINK_BLE_CONN_PARAM_MASTER_INIT_CONN_LATENCY_MAX,       /* Connection latency max */
    SINK_BLE_CONN_PARAM_MASTER_SUPERVISION_TIMEOUT_MIN,     /* Minimum supervision timeout */
    SINK_BLE_CONN_PARAM_MASTER_SUPERVISION_TIMEOUT_MAX      /* Maximum supervision timeout */
};


/* Default connection parameters when Master and the connection has been initiated (initial discovery has been performed,
   can relax parameters) */
static const ble_connection_update_parameters_t ble_master_update_conn_params = {
    SINK_BLE_CONN_PARAM_MASTER_CONN_INTERVAL_MIN,	/* Minimum connection interval */
    SINK_BLE_CONN_PARAM_CONN_INTERVAL_MAX,          /* Maximum connection interval */
    SINK_BLE_CONN_PARAM_MASTER_CONN_LATENCY,        /* Connection latency */
    SINK_BLE_CONN_PARAM_SUPERVISION_TIMEOUT,        /* Supervision timeout */
    SINK_BLE_CONN_PARAM_CE_LENGTH_MIN,              /* Minimum length of the connection */
    SINK_BLE_CONN_PARAM_CE_LENGTH_MAX               /* Maximum length of the connection */
};


/* Default connection update parameters when Slave and the connection has been established */
static const ble_connection_update_parameters_t ble_slave_update_conn_params = {
    SINK_BLE_CONN_PARAM_SLAVE_CONN_INTERVAL_MIN,	/* Minimum connection interval */
    SINK_BLE_CONN_PARAM_CONN_INTERVAL_MAX,          /* Maximum connection interval */
    SINK_BLE_CONN_PARAM_SLAVE_CONN_LATENCY,         /* Connection latency */
    SINK_BLE_CONN_PARAM_SUPERVISION_TIMEOUT,        /* supervision timeout */
    SINK_BLE_CONN_PARAM_CE_LENGTH_MIN,              /* Minimum length of the connection */
    SINK_BLE_CONN_PARAM_CE_LENGTH_MAX               /* Maximum length of the connection */
};


/*******************************************************************************
NAME
    handleInitComplete
    
DESCRIPTION
    Handle when BLE_INTERNAL_MESSAGE_INIT_COMPLETE message was received
    
PARAMETERS
    None
    
RETURNS
    None
*/
static void handleInitComplete(void)
{
    /* After initialisation decide if scanning/advertising needs to start */
    if (sinkBleIsActiveOnPowerOff() || DEVICE_IS_ON)
    {
        /* Power on BLE */
        sinkBlePowerOnEvent();     
    }
}

/*******************************************************************************
NAME
    handleEncryptionRetry
    
DESCRIPTION
    Handle when BLE_INTERNAL_MESSAGE_ENCRYPTION_RETRY_TIMER message was received
    
PARAMETERS
    payload The message payload
    
RETURNS
    None
*/
static void handleEncryptionRetry(Message message)
{
    ble_gap_event_t event;
    ble_gap_event_args_t args;
    uint16 cid = *(uint16 *) message;
    tp_bdaddr tpaddrt;
    bool active_conn = FALSE;

    /* Send GAP event to retry encryption */
    if (cid != 0)
        active_conn = VmGetBdAddrtFromCid(cid, &tpaddrt);
    
    if (active_conn)
    {
        event.id = ble_gap_event_retry_encryption;
        args.encryption_retry.taddr = tpaddrt.taddr;
        event.args = &args;
        sinkBleGapEvent(event);
    }
}

/*******************************************************************************
NAME
    sinkBleHandleCLMessage
    
DESCRIPTION
    Connection library messages that are sent to the BLE message handler.
    
PARAMETERS
    task    The task the message is delivered
    id      The ID for the GATT message
    payload The message payload
    
RETURNS
    None
*/
static void sinkBleHandleCLMessage(Task task, MessageId id, Message message)
{
    switch (id)
    {
        case CL_DM_LOCAL_NAME_COMPLETE:
        {
            sinkBleGapReadLocalNameComplete((CL_DM_LOCAL_NAME_COMPLETE_T*)message);
        }
        break;
        case CL_DM_BLE_SECURITY_CFM:
        {
            sinkBleGapSetSecurityCfm((CL_DM_BLE_SECURITY_CFM_T*)message);
        }
        break;
        case CL_DM_BLE_CONNECTION_PARAMETERS_UPDATE_CFM:
        {
            BLE_INFO(("CL_DM_BLE_CONNECTION_PARAMETERS_UPDATE_CFM [%x]\n", ((CL_DM_BLE_CONNECTION_PARAMETERS_UPDATE_CFM_T*)message)->status));
        }
        break;
        default:
        {
            BLE_ERROR(("Unhandled BLE connection msg [0x%x]\n", id));
        }
        break;
    }
}


/*******************************************************************************
NAME
    bleInternalMsgHandler
    
DESCRIPTION
    Internal BLE messages that are sent to this message handler.
    
PARAMETERS
    task    The task the message is delivered
    id      The ID for the GATT message
    payload The message payload
    
RETURNS
    None
*/
static void bleInternalMsgHandler(Task task, MessageId id, Message message)
{
    ble_gap_event_t event;
    
    switch (id)
    {
        case BLE_INTERNAL_MESSAGE_INIT_COMPLETE:
        {
            BLE_INFO(("BLE_INTERNAL_MESSAGE_INIT_COMPLETE\n"));
            
            handleInitComplete();
        }
        break;
        case BLE_INTERNAL_MESSAGE_EVENT_NO_CONNECTIONS:
        {
            BLE_INFO(("BLE_INTERNAL_MESSAGE_EVENT_NO_CONNECTIONS\n"));
        
            /* Send GAP event to indicate there are no BLE connections */
            event.id = ble_gap_event_no_connections;
            event.args = NULL;
            sinkBleGapEvent(event);
        }
        break;
        case BLE_INTERNAL_MESSAGE_WHITELIST_TIMER:
        {
            BLE_INFO(("BLE_INTERNAL_MESSAGE_WHITELIST_TIMER\n"));
            
            /* Send GAP event to indicate whitelist timer */
            event.id = ble_gap_event_whitelist_timeout;
            event.args = NULL;
            sinkBleGapEvent(event);
        }
        break;
        case BLE_INTERNAL_MESSAGE_FAST_SCAN_TIMER:
        {
            BLE_INFO(("BLE_INTERNAL_MESSAGE_FAST_SCAN_TIMER\n"));
            
            /* Send GAP event to indicate the fast scan has timed out */
            event.id = ble_gap_event_fast_scan_timeout;
            event.args = NULL;
            sinkBleGapEvent(event);
        }
        break;
        case BLE_INTERNAL_MESSAGE_FAST_ADV_TIMER:
        {
            BLE_INFO(("BLE_INTERNAL_MESSAGE_FAST_ADV_TIMER\n"));
            
            /* Send GAP event to indicate the fast scan has timed out */
            event.id = ble_gap_event_fast_adv_timeout;
            event.args = NULL;
            sinkBleGapEvent(event);
        }
        break;
        case BLE_INTERNAL_MESSAGE_ENCRYPTION_RETRY_TIMER:
        {
            BLE_INFO(("BLE_INTERNAL_MESSAGE_ENCRYPTION_RETRY_TIMER\n"));
            handleEncryptionRetry(message);
        }
        break;
        case BLE_INTERNAL_MESSAGE_REDISCOVER_SERVER_SERVICES:
        {
            BLE_INFO(("BLE_INTERNAL_MESSAGE_REDISCOVER_SERVER_SERVICES\n"));
            gattClientRediscoverServices(((BLE_INTERNAL_MESSAGE_REDISCOVER_SERVER_SERVICES_T *)message)->cid);
        }
        break;
        case BLE_INTERNAL_MESSAGE_GATT_CLIENT_SERVICE_INITIALISED:
        {
            BLE_INFO(("BLE_INTERNAL_MESSAGE_GATT_CLIENT_SERVICE_INITIALISED\n"));
            gattClientDiscoveredServiceInitialised(gattClientFindByCid(((BLE_INTERNAL_MESSAGE_GATT_CLIENT_SERVICE_INITIALISED_T *)message)->cid));
        }
        break;
		case BLE_INTERNAL_MESSAGE_BATTERY_READ_TIMER:
		{
			BLE_INFO(("BLE_INTERNAL_MESSAGE_BATTERY_READ_TIMER\n"));
			sinkBleBatteryLevelReadSendAndRepeat();
		}
		break;
    	case BLE_INTERNAL_MESSAGE_HR_READ_TIMER:
    	{
    		BLE_INFO(("BLE_INTERNAL_MESSAGE_HR_READ_TIMER\n"));
    		sinkBleNotifyHrMeasurements();
    	}
    	break;
        case BLE_INTERNAL_MESSAGE_GATT_CLIENT_DISCOVERY_ERROR:
        {
            BLE_INFO(("BLE_INTERNAL_MESSAGE_GATT_CLIENT_DISCOVERY_ERROR\n"));
            gattClientDiscoveryError(((BLE_INTERNAL_MESSAGE_GATT_CLIENT_DISCOVERY_ERROR_T *)message)->connection);
        }
        break;
        default:
        {
            BLE_ERROR(("Unhandled BLE internal msg [0x%x]\n", id));
        }
        break;
    }
}


/******************************************************************************/
void sinkBleInitialiseDevice(void)
{
    BLE_INFO(("Initialise BLE...\n"));
    
    /* Setup BLE Message handler */
    memset(&BLE, 0, sizeof(ble_data_t));
    BLE.task.handler = sinkBleMsgHandler;
    
    /* Setup whitelist from Paired Device List on initialisation */
    ConnectionDmBleAddTdlDevicesToWhiteListReq(TRUE);
    
    /* Initialise GATT */
    if (!sinkGattInitInitialiseDevice())
    {
        FATAL_ERROR(("GATT failed to initialise!\n"));
    }
    
    /* Initialise GAP */
    sinkBleGapInitialise();

    /* Setup advertising filters for supported services */
    if(theSink.features.BleAdvertisingFilter & (1<<ANCS_AD_BIT))
    {
        sinkGattAncsClientSetupAdvertisingFilter();
    }
    if(theSink.features.BleAdvertisingFilter & (1<<BATTERY_AD_BIT))
    {
        gattBatteryClientSetupAdvertisingFilter();
    }
    if(theSink.features.BleAdvertisingFilter & (1<<DIS_AD_BIT))
    {
        sinkGattDisClientSetupAdvertisingFilter();
    }
    if(theSink.features.BleAdvertisingFilter & (1<<HID_AD_BIT))
    {
        sinkGattHidClientSetupAdvertisingFilter();
    }
    if(theSink.features.BleAdvertisingFilter & (1<<IAS_AD_BIT))
    {
        sinkGattIasClientSetupAdvertisingFilter();
    }
    if(theSink.features.BleAdvertisingFilter & (1<<SPC_AD_BIT))
    {
        sinkGattSpClientSetupAdvertisingFilter();
    } 
    if(theSink.features.BleAdvertisingFilter & (1<<HRS_AD_BIT))
    {
        sinkGattHrsClientSetupAdvertisingFilter();
    }
}


/******************************************************************************/
void sinkBleMsgHandler(Task task, MessageId id, Message message)
{
    if ( (id >= CL_MESSAGE_BASE) && (id < CL_MESSAGE_TOP) )
    {
        sinkBleHandleCLMessage(task, id,  message); 
    }
    else if ( (id >= GATT_MESSAGE_BASE) && (id < GATT_MESSAGE_TOP))
    {
        sinkGattMsgHandler(task, id, message);
    }
    else if ((id >= GATT_MANAGER_MESSAGE_BASE) && (id < GATT_MANAGER_MESSAGE_TOP))
    {
        sinkGattManagerMsgHandler(task, id, message);
    }
    else if ( (id >= GATT_SERVER_MESSAGE_BASE) && (id < GATT_SERVER_MESSAGE_TOP))
    {
        sinkGattServerMsgHandler(task, id, message);
    }
    else if ( (id >= GATT_GAP_SERVER_MESSAGE_BASE) && (id < GATT_GAP_SERVER_MESSAGE_TOP))
    {
        if (sinkGattGapServerMsgHandler(task, id, message) == gap_msg_read_name_required)
        {
            sinkBleGapStartReadLocalName(ble_gap_read_name_gap_server);
        }
    }
    else if ( (id >= GATT_BATTERY_SERVER_MESSAGE_BASE) && (id < GATT_BATTERY_SERVER_MESSAGE_TOP))
    {
        sinkGattBatteryServerMsgHandler(task, id, message);
    }
    else if ( (id >= GATT_BATTERY_CLIENT_MESSAGE_BASE) && (id < GATT_BATTERY_CLIENT_MESSAGE_TOP))
    {
        gattBatteryClientMsgHandler(task, id, message);
    }
    else if ( (id >= GATT_IMM_ALERT_SERVER_MESSAGE_BASE) && (id < GATT_IMM_ALERT_SERVER_MESSAGE_TOP))
    {
        sinkGattImmAlertServerMsgHandler(task, id, message);
    }
    else if( (id >= GATT_LINK_LOSS_SERVER_MESSAGE_BASE) && (id < GATT_LLS_ALERT_SERVER_MESSAGE_TOP))
    {
        sinkGattLinkLossServerMsgHandler(task, id, message);
    }
    else if ( (id >= GATT_IMM_ALERT_CLIENT_MESSAGE_BASE) && (id < GATT_IMM_ALERT_CLIENT_MESSAGE_TOP))
    {
        sinkGattIasClientMsgHandler(task, id, message);
    }
    else if ( (id >= GATT_HR_SERVER_MESSAGE_BASE) && (id < GATT_HR_SERVER_MESSAGE_TOP))
    {
        sinkGattHeartRateServerMsgHandler(task, id, message);
    }
    else if( (id >= GATT_ANCS_MESSAGE_BASE) && (id < GATT_ANCS_MESSAGE_TOP))
    {
        sinkGattAncsClientMsgHandler(task, id, message);
    }    
    else if((id >= GATT_HID_CLIENT_MESSAGE_BASE) && (id < GATT_HID_CLIENT_MESSAGE_TOP))
    {
        sinkGattHidClientMsgHandler(task, id, message);
    }
    else if((id >= GATT_DEVICE_INFO_CLIENT_MESSAGE_BASE) && (id < GATT_DEVICE_INFO_CLIENT_MESSAGE_TOP))
    {
        sinkGattDisClientMsgHandler(task, id, message);
    }
    else if((id >= GATT_SCAN_PARAMS_CLIENT_MESSAGE_BASE) && (id < GATT_SCAN_PARAMS_CLIENT_MESSAGE_TOP))
    {
        sinkGattSpClientMsgHandler(task, id, message);
    }
    else if((id >= GATT_HEART_RATE_CLIENT_MESSAGE_BASE) && (id<= GATT_HEART_RATE_CLIENT_MESSAGE_TOP))
    {
        sinkGattHrsClientMsgHandler(task, id, message);
    }
    else if((id >= GATT_CLIENT_MESSAGE_BASE) && (id < GATT_CLIENT_MESSAGE_TOP))
    {
        sinkGattClientServiceMsgHandler(task, id, message);
    }
    else if ( (id >= BLE_INTERNAL_MESSAGE_BASE) && (id < BLE_INTERNAL_MESSAGE_TOP))
    {
        bleInternalMsgHandler(task, id, message);
    }
    else
    {
        BLE_ERROR(("Unhandled BLE msg [0x%x]\n", id));
    }
}


/******************************************************************************/
void sinkBleBondableEvent(void)
{
    ble_gap_event_t event;
        
    /* Send GAP event to become bondable */
    event.id = ble_gap_event_bondable;
    event.args = NULL;
    sinkBleGapEvent(event);
}


/******************************************************************************/
void sinkBleBondablePairingTimeoutEvent(void)
{
    ble_gap_event_t event;
        
    /* Send GAP event to exit bondable mode */
    event.id = ble_gap_event_bondable_pairing_timeout;
    event.args = NULL;
    
    sinkBleGapEvent(event);
}

/******************************************************************************/
void sinkBleBondableConnectionTimeoutEvent(void)
{
    ble_gap_event_t event;
        
    /* Send GAP event to exit bondable connection mode */
    event.id = ble_gap_event_bondable_connection_timeout;
    event.args = NULL;
    sinkBleGapEvent(event);
}

/******************************************************************************/
void sinkBleMasterConnCompleteEvent(uint16 cid)
{
    gatt_client_connection_t *connection = NULL;
    tp_bdaddr *tpaddrt = NULL;
    ble_gap_event_t event;
    ble_gap_event_args_t args;
    bool active_conn = FALSE;

    connection = gattClientFindByCid(cid);
    
    if (connection != NULL)
    {
        tpaddrt = PanicUnlessMalloc(sizeof(tp_bdaddr));
        active_conn = VmGetBdAddrtFromCid(cid, tpaddrt);
    }
    
    /* Send GAP event when central connection has complete, which can be used to restart scanning */
    event.id = ble_gap_event_central_conn_complete;
    if (!active_conn)
    {
        event.args = NULL;
    }
    else
    {
        args.master_conn_complete.taddr = (*tpaddrt).taddr;
        event.args = &args;
    }

    if(tpaddrt)
    {
        free(tpaddrt);
    }
    
    sinkBleGapEvent(event);
}

/******************************************************************************/
void sinkBleDisconnectionEvent(uint16 cid)
{
    ble_gap_event_t event;
    ble_gap_event_args_t args;

    /* Send GAP event for disconnection linked to central role */
    event.id = ble_gap_event_disconn_ind;
    args.connection_id= cid;
    event.args = &args;
    sinkBleGapEvent(event);
}


/******************************************************************************/
void sinkBleCancelAdvertisingEvent(void)
{
    ble_gap_event_t event;
        
    /* Send GAP event when advertising is cancelled */
    event.id = ble_gap_event_cancelled_advertising;
    event.args = NULL;
    sinkBleGapEvent(event);
}


/******************************************************************************/
void sinkBleSlaveConnIndEvent(uint16 cid)
{
    ble_gap_event_t event;
    /* Remove no connections message from queue, it no longer applies */
    MessageCancelFirst(sinkGetBleTask(), BLE_INTERNAL_MESSAGE_EVENT_NO_CONNECTIONS);
        
    /* Send GAP event when remote connection succeeded */
    event.id = ble_gap_event_peripheral_conn_ind;
    event.args = (ble_gap_event_args_t *)&cid;
    sinkBleGapEvent(event);
}

/******************************************************************************/
void sinkBleCheckNoConnectionsEvent(void)
{
    if (!gattClientHasClients() && gattServerIsFullyDisconnected())
    {
        /* If no connections exist, send an event to report this.
           so that the application can start advertising/scanning. */
        MessageCancelFirst(sinkGetBleTask(), BLE_INTERNAL_MESSAGE_EVENT_NO_CONNECTIONS);
        MessageSend(sinkGetBleTask(), BLE_INTERNAL_MESSAGE_EVENT_NO_CONNECTIONS, NULL);
    }
}

/******************************************************************************/
void sinkBleRetryEncryptionEvent(uint16 cid)
{
    /* If there is pairing in progress then encryption request needs to be retried as Bluestack returns host busy status 
    */
    uint16 *message = PanicUnlessNew(uint16);
    MessageCancelFirst(sinkGetBleTask(), BLE_INTERNAL_MESSAGE_ENCRYPTION_RETRY_TIMER);
    *message = cid;
    MessageSendLater(sinkGetBleTask(), BLE_INTERNAL_MESSAGE_ENCRYPTION_RETRY_TIMER, message, ENCRYPTION_RETRY_EVENT_DELAY_MS);
}

/******************************************************************************/
void sinkBleEncryptionCompleteEvent(void)
{
    ble_gap_event_t event;
        
    /* Send GAP event to power on */
    event.id = ble_gap_event_encyption_complete;
    event.args = NULL;
    sinkBleGapEvent(event);    
}

/******************************************************************************/
void sinkBlePowerOnEvent(void)
{
    ble_gap_event_t event;
        
    /* Send GAP event to power on */
    event.id = ble_gap_event_power_on;
    event.args = NULL;
    sinkBleGapEvent(event);    
}


/******************************************************************************/
void sinkBlePowerOffEvent(void)
{
    ble_gap_event_t event;

    /* Send GAP event to power off */
    event.id = ble_gap_event_power_off;
    event.args = NULL;
    sinkBleGapEvent(event);
}

/******************************************************************************/
void sinkBleClearWhiteListEvent(void)
{
    ble_gap_event_t event;

    /* Send GAP event to power off */
    event.id = ble_gap_event_clear_white_list;
    event.args = NULL;
    sinkBleGapEvent(event);
}


/******************************************************************************/
const ble_configuration_t *sinkBleGetConfiguration(void)
{
    return &ble_config;
}

/******************************************************************************/
void sinkBleSetAdvertisingParamsDefault(uint16 adv_interval_min, uint16 adv_interval_max)
{
    ble_adv_params_t params;

    params.undirect_adv.adv_interval_min = adv_interval_min;
    params.undirect_adv.adv_interval_max = adv_interval_max;
    params.undirect_adv.filter_policy = ble_filter_none;

    BLE_INFO(("Set BLE Default Advertising Params\n"));

    ConnectionDmBleSetAdvertisingParamsReq(ble_adv_ind, FALSE, BLE_ADV_CHANNEL_ALL, &params);
}

/******************************************************************************/
void sinkBleSetMasterConnectionParamsDefault(uint16 scan_interval, uint16 scan_window)
{
    ble_connection_params params;

    params.scan_interval = scan_interval;
    params.scan_window = scan_window;
    params.conn_interval_min = ble_master_initial_conn_params.conn_interval_min;
    params.conn_interval_max = ble_master_initial_conn_params.conn_interval_max;
    params.conn_latency = ble_master_initial_conn_params.conn_latency;
    params.supervision_timeout = ble_master_initial_conn_params.supervision_timeout;
    params.conn_attempt_timeout = ble_master_initial_conn_params.conn_attempt_timeout;
    params.conn_latency_max = ble_master_initial_conn_params.conn_latency_max;
    params.supervision_timeout_min = ble_master_initial_conn_params.supervision_timeout_min;
    params.supervision_timeout_max = ble_master_initial_conn_params.supervision_timeout_max;
    params.own_address_type = TYPED_BDADDR_PUBLIC;
    
    BLE_INFO(("Set BLE Default Connection Params\n"));
    
    ConnectionDmBleSetConnectionParametersReq(&params);
}


/******************************************************************************/
void sinkBleSetMasterConnectionParamsUpdate(typed_bdaddr *taddr)
{
    BLE_INFO(("Set BLE Updated Master Connection Params\n"));
    
    ConnectionDmBleConnectionParametersUpdateReq(
                sinkGetBleTask(),
                taddr,
                ble_master_update_conn_params.conn_interval_min,
                ble_master_update_conn_params.conn_interval_max,
                ble_master_update_conn_params.conn_latency,
                ble_master_update_conn_params.supervision_timeout,
                ble_master_update_conn_params.ce_length_min,
                ble_master_update_conn_params.ce_length_max
                );
}


/******************************************************************************/
void sinkBleSetSlaveConnectionParamsUpdate(typed_bdaddr *taddr)
{
    BLE_INFO(("Set BLE Updated Slave Connection Params\n"));
    
    ConnectionDmBleConnectionParametersUpdateReq(
                sinkGetBleTask(),
                taddr,
                ble_slave_update_conn_params.conn_interval_min,
                ble_slave_update_conn_params.conn_interval_max,
                ble_slave_update_conn_params.conn_latency,
                ble_slave_update_conn_params.supervision_timeout,
                ble_slave_update_conn_params.ce_length_min,
                ble_slave_update_conn_params.ce_length_max
                );
}


/******************************************************************************/
void sinkBleGetAdvertisingParameters(bool fast_adv, ble_advertising_parameters_t *adv_params)
{
    if (fast_adv)
    {
        adv_params->interval_min = sinkBleGetConfiguration()->adv_interval_min_fast;
        adv_params->interval_max = sinkBleGetConfiguration()->adv_interval_max_fast;
    }
    else
    {
        adv_params->interval_min = sinkBleGetConfiguration()->adv_interval_min_slow;
        adv_params->interval_max = sinkBleGetConfiguration()->adv_interval_max_slow;
    }
}


/******************************************************************************/
void sinkBleGetScanningParameters(bool fast_scan, ble_scanning_parameters_t *scan_params)
{
    if (fast_scan)
    {
        scan_params->interval = sinkBleGetConfiguration()->scan_interval_fast;
        scan_params->window = sinkBleGetConfiguration()->scan_window_fast;
    }
    else
    {
        scan_params->interval = sinkBleGetConfiguration()->scan_interval_slow;
        scan_params->window = sinkBleGetConfiguration()->scan_window_slow;
    }
}

/******************************************************************************/
void sinkBleDeleteMarkedDevices(void)
{
    if (BdaddrIsZero(&GAP.security_failed_device.addr))
    {
        /* No device is marked for deletion */
        return;
    }
    /* Delete the device from TDL */
    ConnectionSmDeleteAuthDeviceReq(GAP.security_failed_device.type, &GAP.security_failed_device.addr);
    BLE_INFO(("Marked LE Device deleted...\n"));
    /* clear the marked device */
    BdaddrTypedSetEmpty(&GAP.security_failed_device);
}

/******************************************************************************/
void sinkBleSimplePairingCompleteInd(const CL_SM_BLE_SIMPLE_PAIRING_COMPLETE_IND_T *ind)
{
    uint16 cid = GattGetCidForBdaddr(&ind->tpaddr.taddr);

    /* A client might have written Client Characteristic Configuration data before bonding.
       If the device is now bonded, this configuration should persist for future connections. */
    if (cid && (ind->status == success))
    {
        gattServerStoreConfigAttributes(cid, gatt_attr_service_all);

        sinkGattHidAddPriorityDevicesList(cid);
        /* Make sure that priority devices are looked after in the PDL.*/
        deviceManagerUpdatePriorityDevices();        
    }
}

/******************************************************************************/
bool SinkBleConnectionParameterIsOutOfRange(const CL_DM_BLE_ACCEPT_CONNECTION_PAR_UPDATE_IND_T *ind)
{
    bool outOfRange = FALSE;
    gatt_client_connection_t *connection = NULL;
    uint16 cid = GattGetCidForBdaddr(&ind->taddr);

    connection = gattClientFindByCid(cid);

    /* Accept or reject the connection parameter update request; will be rejected if the
     * latency, minimum and maximum requested values are not in the allowed range.
     */
    if(connection->role == ble_gap_role_central)
    {
        if(ind->conn_interval_min < SINK_BLE_CONN_PARAM_MASTER_CONN_INTERVAL_MIN)
            {
                outOfRange = TRUE;
            }
    }
    else if (connection->role == ble_gap_role_peripheral)
    {
        if(ind->conn_interval_min < SINK_BLE_CONN_PARAM_SLAVE_CONN_INTERVAL_MIN || 
            ind->conn_interval_max > SINK_BLE_CONN_PARAM_CONN_INTERVAL_MAX ||
            ind->conn_latency < SINK_BLE_CONN_PARAM_SLAVE_CONN_LATENCY)
            {
                outOfRange = TRUE;
            }
    }

    return outOfRange;
}

/******************************************************************************/
bool sinkBleCheckBdAddrtIsBonded(const typed_bdaddr *client_taddr, typed_bdaddr *public_taddr)
 {

    tp_bdaddr current_addr;
    tp_bdaddr public_addr;
 
     BLE_INFO(("gattServerBdAddrtIsBonded\n"));
 
    if(client_taddr == NULL || client_taddr->type == TYPED_BDADDR_INVALID)
        return FALSE;

    current_addr.transport = TRANSPORT_BLE_ACL;
    current_addr.taddr = *client_taddr;

    /* Retrieve permanent address if this is a random address */
    if(current_addr.taddr.type == TYPED_BDADDR_RANDOM)
    {
        VmGetPublicAddress(&current_addr, &public_addr);
    }
    else
    {
        /* Provided address is PUBLIC address, copy it */
        public_addr.transport = TRANSPORT_BLE_ACL;
        public_addr.taddr = *client_taddr;
    }
 
    *public_taddr = public_addr.taddr;
 
    BLE_INFO((" Public Addr[%x:%x:%lx]\n",
        public_taddr->addr.nap,
        public_taddr->addr.uap,
        public_taddr->addr.lap));
 
    return ConnectionSmGetAttributeNow(0, &public_taddr->addr, 0, NULL);
 }

#endif /* GATT_ENABLED */

