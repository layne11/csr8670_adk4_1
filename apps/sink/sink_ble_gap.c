/****************************************************************************
Copyright (c) 2014 - 2016 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_ble_gap.c

DESCRIPTION
    BLE GAP functionality
*/

#include "sink_ble_gap.h"

#include "sink_ble.h"
#include "sink_ble_sc.h"
#include "sink_ble_advertising.h"
#include "sink_ble_scanning.h"
#include "sink_debug.h"
#include "sink_development.h"
#include "sink_gatt_client.h"
#include "sink_gatt_client_ancs.h"
#include "sink_gatt_client_hid.h"
#include "sink_gatt_client_spc.h"
#include "sink_gatt_manager.h"
#include "sink_gatt_server.h"
#include "sink_gatt_server_gap.h"
#include "sink_private.h"
#include "sink_gatt_common.h"

#include <gatt_manager.h>
#include <vm.h>

#ifdef GATT_ENABLED


#ifdef DEBUG_BLE_GAP
#define BLE_GAP_INFO(x) DEBUG(x)
#define BLE_GAP_ERROR(x) DEBUG(x) TOLERATED_ERROR(x)
const char * const gap_states[ble_gap_state_last] = {
    "IDLE",
    "SCAN_ADV",
    "BONDED_SCAN_ADV",
    "CONNECTING",
    "FULLY CONNECTED",
    "BONDABLE_CONNECTING",
    "BONDABLE_CONNECTED",
};
const char * const gap_events[ble_gap_event_last] = {
    "POWER_ON",
    "BONDABLE",
    "SET_ADV_COMPLETE",
    "BOND_PAIR_TIMEOUT",
    "CENTRAL_CONNECT_ATTEMPT",
    "CENTRAL_CONNECT_COMPLETE",
    "PERIPHERAL_CONNECT_IND",
    "DISCONNECT_IND",
    "POWER_OFF",
    "WHITELIST_TIMEOUT",
    "FAST_SCAN_TIMEOUT",
    "FAST_ADV_TIMEOUT",
    "RETRY_ENCRYPTION",
    "NO_CONNECTION",
    "CANCELLED_ADV",
    "BOND_CONN_TIMEOUT",
    "ENCRYPTION_COMPLETE",
};
#else
#define BLE_GAP_INFO(x)
#define BLE_GAP_ERROR(x)
#endif /* DEBUG_BLE_GAP */

#ifdef DEBUG_BLE_GAP_SM
#define BLE_GAP_SM_INFO(x) DEBUG(x)
#define BLE_GAP_SM_ERROR(x) DEBUG(x) TOLERATED_ERROR(x)
#else
#define BLE_GAP_SM_INFO(x)
#define BLE_GAP_SM_ERROR(x)
#endif

      
#define MAX_GATT_CONNECTIONS    2
/*******************************************************************************
NAME
    sinkBleGetGapState
    
DESCRIPTION
    Gets the GAP state.
    
PARAMETERS
    None
    
RETURNS
    The GAP state.
*/
static ble_gap_state_t sinkBleGetGapState(void)
{
    return GAP.state;
}


/*******************************************************************************
NAME
    sinkBleSetGapState
    
DESCRIPTION
    Sets the GAP state.
    
PARAMETERS
    The GAP state.
    
RETURNS
    None
*/
static void sinkBleSetGapState(ble_gap_state_t state)
{
    GAP.state = state;
    BLE_GAP_INFO(("GAP new state=[%s]\n", gap_states[state]));
    BLE_GAP_SM_INFO(("GAP new state=[%d] \n", state));
}


/*******************************************************************************
NAME
    sinkBleGapSetBondedToPrivacyDevice
    
DESCRIPTION
    Sets if bonded to privacy enable device.
    
PARAMETERS
    is_bonded   TRUE if bonded to privacy enable device. FALSE otherwise.
    
RETURNS
    None
*/
static void sinkBleGapSetBondedToPrivacyDevice(bool is_bonded)
{
    GAP.bonded_to_private_device = is_bonded;
    
    BLE_GAP_INFO(("GAP Bonded to private device=[%u]\n", is_bonded));
}

/*******************************************************************************
NAME
    sinkBleGapInitGapConnFlag
    
DESCRIPTION
    Initialize the conn flags.
    
PARAMETERS
    None
    
RETURNS
    None
*/
static void sinkBleGapInitGapConnFlag(void)
{
    uint16 index;
    for(index = 0; index < MAX_BLE_CONNECTIONS; index ++)
    {
        BdaddrTypedSetEmpty(&GAP.gap_conn_flags[index].taddr);
        GAP.gap_conn_flags[index].cid = 0;
        GAP.gap_conn_flags[index].service_discovery_inprogress = FALSE;
        GAP.gap_conn_flags[index].central_conn_attempt = FALSE;
    }
}

/*******************************************************************************
NAME
    sinkBleGapFindGapConnFlagIndexByCid
    
DESCRIPTION
    Finds the index where the supplied cid has been set.
    
PARAMETERS
    cid   Get the index based on this connection identifier.
    
RETURNS
    uint16 index of the matched slot
*/
uint16 sinkBleGapFindGapConnFlagIndexByCid(uint16 cid)
{
    uint16 index;
    if(cid)
    {
        for(index = 0; index < MAX_BLE_CONNECTIONS; index ++)
        {
            if(GAP.gap_conn_flags[index].cid == cid)
            {
                return index;
            }
        }
    }
    return GATT_INVALID_INDEX;
}

/*******************************************************************************
NAME
    sinkBleGapFindGapConnFlagIndex
    
DESCRIPTION
    Finds the index where the supplied BT address has been set.
    
PARAMETERS
    tp_addr   Address which needs to be looked into.
    
RETURNS
    uint16 index of the matched slot
*/
uint16 sinkBleGapFindGapConnFlagIndex(typed_bdaddr *tp_addr)
{
    uint16 index;
    for(index = 0; index < MAX_BLE_CONNECTIONS; index ++)
    {
        /* we need to check if the API is called for finding empty slot or for finding the slot 
         * for the address sent */
        if(((tp_addr == NULL) && (BdaddrTypedIsEmpty(&GAP.gap_conn_flags[index].taddr))) ||
           ((tp_addr) && (BdaddrTypedIsSame(tp_addr, &GAP.gap_conn_flags[index].taddr))))
        {
            return index;
        }
    }
    return GATT_INVALID_INDEX;
}

/*******************************************************************************
NAME
    sinkBleGapSetCentralConnAttempt
    
DESCRIPTION
    Sets if master has triggered link connection.
    
PARAMETERS
    conn_attempt   TRUE if central triggered conn attempt. FALSE otherwise.
    tp_addr The address of the link for which the connection is attempted
    index slot available
    
RETURNS
    None
*/
void sinkBleGapSetCentralConnAttempt(bool conn_attempt, const typed_bdaddr *tp_addr, uint16 cid, uint8 index)
{
    /* Update the index */
    if(index != GATT_INVALID_INDEX)
    {
        BdaddrTypedSetEmpty(&GAP.gap_conn_flags[index].taddr);
        if(tp_addr)
        {
            GAP.gap_conn_flags[index].taddr = *tp_addr;
        }
        GAP.gap_conn_flags[index].cid = cid;
        GAP.gap_conn_flags[index].central_conn_attempt = conn_attempt;
        BLE_GAP_INFO(("GAP Set Master Conn Attempt =[%u]\n", conn_attempt));
    }
}

/*******************************************************************************
NAME
    sinkBleGapResetCentralConnAttempt
    
DESCRIPTION
    Resets the connection attempt flag.
    
PARAMETERS
    None
    
RETURNS
    None
*/
static void sinkBleGapResetCentralConnAttempt(void)
{
    uint16 index;
    /* Its OK to reset based on the below condition because we cannot 
     * have more than 1 connection @ a time. But if we allow central conn @
     * same time, then we should reset based on cid/bdaddress of the disconnected
     * link */
    for(index = 0; index < MAX_BLE_CONNECTIONS; index ++)
    {
        /* Check if the attempt flag is set, then also check if the bd address is not 
         * empty and also CID js 0. This means there was a trigger for connection
         * which failed, and we dont even have the cid. Reset the unsuccessful attempt */
        if((GAP.gap_conn_flags[index].central_conn_attempt) && 
           (GAP.gap_conn_flags[index].cid == 0) &&
           !(BdaddrTypedIsEmpty(&GAP.gap_conn_flags[index].taddr)))
        {
            BLE_GAP_INFO(("GAP found the unsuccessful connection attempt\n"));
            sinkBleGapSetCentralConnAttempt(FALSE, NULL, 0, index);
        }
    }
}

/*******************************************************************************
NAME
    sinkBleGapHasCentralConnAttempt
    
DESCRIPTION
    Returns the status of central conn attempt flag.
    
PARAMETERS
    None
    
RETURNS
    bool - TRUE if discovery in progress, otherwise FALSE
*/
static bool sinkBleGapHasCentralConnAttempt(void)
{
    uint8 index;
    for(index = 0; index < MAX_BLE_CONNECTIONS; index ++)
    {
        BLE_GAP_INFO(("GAP Central conn attempt=[%u]\n", GAP.gap_conn_flags[index].central_conn_attempt));
        /* If atlest one conn is being attempted, then don't allow the second */
        if(GAP.gap_conn_flags[index].central_conn_attempt)
            return TRUE;
    }
    return FALSE;
}

/*******************************************************************************
NAME
    sinkBleGapSetDiscoveryInProgress
    
DESCRIPTION
    Sets if primary service discovery is in progress.
    
PARAMETERS
    in_progress   TRUE if discovery in progress. FALSE otherwise.
    
RETURNS
    None
*/
void sinkBleGapSetDiscoveryInProgress(bool in_progress, typed_bdaddr *tp_addr, uint16 cid, uint8 index)
{
    if(index != GATT_INVALID_INDEX)
    {
        BdaddrTypedSetEmpty(&GAP.gap_conn_flags[index].taddr);
        if(tp_addr)
        {
            GAP.gap_conn_flags[index].taddr = *tp_addr;
        }
        GAP.gap_conn_flags[index].cid = cid;
        GAP.gap_conn_flags[index].service_discovery_inprogress = in_progress;
        BLE_GAP_INFO(("GAP Set Discovery in progress=[%u]\n", in_progress));
    }
}

/*******************************************************************************
NAME
    sinkBleGapIsDiscoveryInProgress
    
DESCRIPTION
    Returns the status of discovery in progress flag.
    
PARAMETERS
    None
    
RETURNS
    bool - TRUE if discovery in progress, otherwise FALSE
*/
static bool sinkBleGapIsDiscoveryInProgress(uint8 index)
{
    if(index != GATT_INVALID_INDEX)
    {
        BLE_GAP_INFO(("GAP Discovery in progress=[%u]\n", GAP.gap_conn_flags[index].service_discovery_inprogress));
        return (GAP.gap_conn_flags[index].service_discovery_inprogress ? TRUE : FALSE);
    }
    return FALSE;
}

/*******************************************************************************
NAME
    sinkBleGapGetBondedToPrivacyDevice
    
DESCRIPTION
    Gets if bonded to privacy enable device.
    
PARAMETERS
    None
    
RETURNS
    TRUE if bonded to privacy enable device. FALSE otherwise.
*/
static bool sinkBleGapGetBondedToPrivacyDevice(void)
{
    return GAP.bonded_to_private_device;
}


/*******************************************************************************
NAME
    gapSetWhitelistScanActive
    
DESCRIPTION
    Sets the whitelist active state.
    
PARAMETERS
    active   The active state
    
RETURNS
    None
*/
static void gapSetWhitelistScanActive(bool active)
{
    GAP.scan.whitelist_active = active;
}


/*******************************************************************************
NAME
    gapGetWhitelistScanActive
    
DESCRIPTION
    Gets the whitelist active state.
    
PARAMETERS
    None
    
RETURNS
    The active state
*/
static ble_gap_scan_speed_t gapGetWhitelistScanActive(void)
{
    return GAP.scan.whitelist_active;
}


/*******************************************************************************
NAME
    gapStartWhitelistTimer
    
DESCRIPTION
    Start timer for Whitelist scanning.
    
PARAMETERS
    None
    
RETURNS
    None
*/
static void gapStartWhitelistTimer(void)
{
    if (sinkBleGapGetBondedToPrivacyDevice())
    {
        /* Whitelist scanning will not work if bonded to a private device.
           Revert to general scanning after a configurable time which can then attempt to find these private devices */
           
        MessageSendLater(sinkGetBleTask(), BLE_INTERNAL_MESSAGE_WHITELIST_TIMER, 0, D_SEC(sinkBleGetConfiguration()->whitelist_scan_timeout_when_private_addr_s));
    }
    
    /* Keep track of when it is whitelist scanning */
    gapSetWhitelistScanActive(TRUE);
}
        
        
/*******************************************************************************
NAME
    gapStopWhitelistTimer
    
DESCRIPTION
    Stop timer for Whitelist scanning.
    
PARAMETERS
    None
    
RETURNS
    None
*/
static void gapStopWhitelistTimer(void)
{
    MessageCancelFirst(sinkGetBleTask(), BLE_INTERNAL_MESSAGE_WHITELIST_TIMER);
    
    /* Keep track of when it is whitelist scanning */
    gapSetWhitelistScanActive(FALSE);
}


/*******************************************************************************
NAME
    gapSetScanSpeed
    
DESCRIPTION
    Sets the scan speed state.
    
PARAMETERS
    speed   The scan speed
    
RETURNS
    None
*/
static void gapSetScanSpeed(ble_gap_scan_speed_t speed)
{
    GAP.scan.speed = speed;
}


/*******************************************************************************
NAME
    gapIsFastScanSpeed
    
DESCRIPTION
    Gets if fast scanning.
    
PARAMETERS
    None
    
RETURNS
    TRUE if scan speed is fast. FALSE otherwise.
*/
static bool gapIsFastScanSpeed(void)
{
    return (GAP.scan.speed == ble_gap_scan_speed_fast ? TRUE : FALSE);
}



/*******************************************************************************
NAME
    gapSetAdvSpeed
    
DESCRIPTION
    Sets the advertising speed state.
    
PARAMETERS
    speed   The advertising speed
    
RETURNS
    None
*/
static void gapSetAdvSpeed(ble_gap_adv_speed_t speed)
{
    GAP.adv.speed = speed;
}


/*******************************************************************************
NAME
    gapIsFastAdvSpeed
    
DESCRIPTION
    Gets if fast advertising.
    
PARAMETERS
    None
    
RETURNS
    TRUE if advertising speed is fast. FALSE otherwise.
*/
static bool gapIsFastAdvSpeed(void)
{
    return (GAP.adv.speed == ble_gap_adv_speed_fast ? TRUE : FALSE);
}


/*******************************************************************************
NAME
    gapStartFastScanTimer
    
DESCRIPTION
    Starts the timer for fast scanning.
    
PARAMETERS
    None
    
RETURNS
    None
*/
static void gapStartFastScanTimer(void)
{
    MessageCancelFirst(sinkGetBleTask(), BLE_INTERNAL_MESSAGE_FAST_SCAN_TIMER);
    MessageSendLater(sinkGetBleTask(), BLE_INTERNAL_MESSAGE_FAST_SCAN_TIMER, 0, D_SEC(sinkBleGetConfiguration()->scan_timer_fast_s));

    /* Keep track of when it is fast scanning */
    gapSetScanSpeed(ble_gap_scan_speed_fast);
}


/*******************************************************************************
NAME
    gapStopFastScanTimer
    
DESCRIPTION
    Stops the timer for fast scanning.
    
PARAMETERS
    None
    
RETURNS
    None
*/
static void gapStopFastScanTimer(void)
{
    MessageCancelFirst(sinkGetBleTask(), BLE_INTERNAL_MESSAGE_FAST_SCAN_TIMER);
}


/*******************************************************************************
NAME
    gapStartFastAdvTimer
    
DESCRIPTION
    Starts the timer for fast advertising.
    
PARAMETERS
    None
    
RETURNS
    None
*/
static void gapStartFastAdvTimer(void)
{
    MessageCancelFirst(sinkGetBleTask(), BLE_INTERNAL_MESSAGE_FAST_ADV_TIMER);
    MessageSendLater(sinkGetBleTask(), BLE_INTERNAL_MESSAGE_FAST_ADV_TIMER, 0, D_SEC(sinkBleGetConfiguration()->adv_timer_fast_s));
}


/*******************************************************************************
NAME
    gapStopFastAdvTimer
    
DESCRIPTION
    Stops the timer for fast advertising.
    
PARAMETERS
    None
    
RETURNS
    None
*/
static void gapStopFastAdvTimer(void)
{
    MessageCancelFirst(sinkGetBleTask(), BLE_INTERNAL_MESSAGE_FAST_ADV_TIMER);
}

/*******************************************************************************
NAME
    gapStopAdvertising
    
DESCRIPTION
    Stop advertising to devices
    
PARAMETERS
    new_speed       Set the speed for the next advertising event
    
RETURNS
    None
*/
static void gapStopAdvertising(ble_gap_adv_speed_t new_speed)
{
    gapStopFastAdvTimer();
    gapSetAdvSpeed(new_speed);
    sinkGattManagerStopAdvertising();
}


/*******************************************************************************
NAME
    gapStartScanning
    
DESCRIPTION
    Start scanning for devices.
    
PARAMETERS
    new_scan If TRUE will start a new scan, eg. white list scanning first for non-bonding mode
    
RETURNS
    TRUE if the scanning was started. FALSE otherwise.
*/
static bool gapStartScanning(bool new_scan)
{
    bool white_list = TRUE;
    bool fast_scan = TRUE;

    /* Number of BLE links for scatternet is reached, then dont allow any more scans */
    if(gattCommonGetNumberOfConn() >= MAX_GATT_CONNECTIONS)
    {
        /* Don't resume scanning if at connection limit */
        BLE_GAP_INFO((" GAP central fully connected, don't start scanning/advertising\n"));
        sinkBleSetGapState(ble_gap_state_fully_connected);
        gapStopWhitelistTimer();
        gapStopFastScanTimer();
        /* Even stop advertising */
        gapStopAdvertising(ble_gap_adv_speed_fast);
        return FALSE;
    }
    else if(gattCommonIsMaxConnReached(ble_gap_role_central))
    {
        /* don't allow scanning as we have reached maximum central connection */
        gapStopWhitelistTimer();
        gapStopFastScanTimer();
        /* update the state accordingly */
        if (sinkBleGetGapState() != ble_gap_state_bondable_scanning_advertising)
        {
            sinkBleSetGapState(ble_gap_state_scanning_advertising);
        }
        return FALSE;
    }
    
    if (sinkBleGetGapState() == ble_gap_state_bondable_scanning_advertising)
    {
        white_list = FALSE;
    }
    else
    {
        sinkBleSetGapState(ble_gap_state_scanning_advertising);
        if (!new_scan)
        { 
            white_list = gapGetWhitelistScanActive();
            fast_scan = gapIsFastScanSpeed();
        }
        if (fast_scan)
        {
            /* Restart fast scan timer which will trigger slow scan on timeout */           
            gapStartFastScanTimer();
        }
        if (white_list)
        {
            /* Restart whitelist timer which will trigger general scan on timeout */
            gapStartWhitelistTimer();
        }
    }
    bleStartScanning(white_list, fast_scan);
    /* Update the scan interval and window to remote scan server */
    sinkGattSpClientSetScanIntervalWindow(fast_scan);
    return TRUE;
}


/*******************************************************************************
NAME
    gapStopScanning
    
DESCRIPTION
    Stop scanning for devices.
    
PARAMETERS
    None
    
RETURNS
    None
*/
static void gapStopScanning(void)
{
    bleStopScanning();
        
    gapStopFastScanTimer();
    gapStopWhitelistTimer();
}


/*******************************************************************************
NAME
    gapSetConnectionParamsDefault
    
DESCRIPTION
    Sets the default connection paramaters when no connection active.
    
PARAMETERS
    None
    
RETURNS
    None
*/
static void gapSetConnectionParamsDefault(void)
{
    ble_scanning_parameters_t scan_params;
    
    BLE_GAP_INFO(("GAP Scanning fast=[%u]\n", gapIsFastScanSpeed()));
    
    /* Get the current scan parameters */
    sinkBleGetScanningParameters(gapIsFastScanSpeed(), &scan_params);

    /* Set connection parameters while connecting */
    sinkBleSetMasterConnectionParamsDefault(scan_params.interval, scan_params.window);
}

/*******************************************************************************
NAME
    gapSetAdvertisingParamsDefault
    
DESCRIPTION
    Sets the default advertising paramaters.
    
PARAMETERS
    None
    
RETURNS
    None
*/
static void gapSetAdvertisingParamsDefault(void)
{
    ble_advertising_parameters_t adv_params;

    BLE_GAP_INFO(("Adv fast=[%u]\n", gapIsFastAdvSpeed()));

    /* Get the current advertising parameters */
    sinkBleGetAdvertisingParameters(gapIsFastAdvSpeed(), &adv_params);

    /* Set advertising parameters */
    sinkBleSetAdvertisingParamsDefault(adv_params.interval_min, adv_params.interval_max);
}

/*******************************************************************************
NAME
    gapStartAdvertising
    
DESCRIPTION
    Start advertising to devices
    
PARAMETERS
    None
    
RETURNS
    None
*/
static bool gapStartAdvertising(void)
{

    if(gattCommonGetNumberOfConn() >= MAX_GATT_CONNECTIONS)
    {
        BLE_GAP_INFO((" GAP central fully connected, don't start scanning/advertising\n"));
        sinkBleSetGapState(ble_gap_state_fully_connected);
        /* Stop even scanning */
        bleStopScanning();
        
        gapStopFastScanTimer();
        gapStopWhitelistTimer();
        return FALSE;
    }
    else if(gattCommonIsMaxConnReached(ble_gap_role_peripheral))
    {
        /* Don't allow advertising as we have reached maximum peripheral connection */
        if (sinkBleGetGapState() != ble_gap_state_bondable_scanning_advertising)
        {
            sinkBleSetGapState(ble_gap_state_scanning_advertising);
        }
        return FALSE;
    }
    
    /* Change advertising params */
    gapSetAdvertisingParamsDefault();
    
    /* Start sending advertisments */
    sinkGattManagerStartAdvertising();
    
    /* Update state and timers */    
    BLE_GAP_INFO(("GAP advertsising\n"));
    if (sinkBleGetGapState() != ble_gap_state_bondable_scanning_advertising)
    {
        sinkBleSetGapState(ble_gap_state_scanning_advertising);
        
        if (gapIsFastAdvSpeed())
            gapStartFastAdvTimer();
    }
    return TRUE;
}

void sink_custom_ble_scan_adv(void)
{
	gapStartAdvertising();
}
/*******************************************************************************
NAME
    gapStartAdvertisingInConnecting
    
DESCRIPTION
    Start advertising to devices in BLE Connecting state
    
PARAMETERS
    None
    
RETURNS
    None
*/
static bool gapStartAdvertisingInConnecting(void)
{
    if(gattCommonIsMaxConnReached(ble_gap_role_peripheral))
    {
        /* Don't allow advertising as we have reached maximum peripheral connection */
        return FALSE;
    }
    
    /* Change advertising params */
    gapSetAdvertisingParamsDefault();
    
    /* Start sending advertisments */
    sinkGattManagerStartAdvertising();
    
    /* Update state and timers */    
    BLE_GAP_INFO(("GAP advertsising without changing the state\n"));
    if (gapIsFastAdvSpeed())
        gapStartFastAdvTimer();
    return TRUE;
}

/*******************************************************************************
NAME
    gapStartBondableConnectionTimer
    
DESCRIPTION
    Starts a connection timer when the GAP bonding role is entered.
    
PARAMETERS
    None
    
RETURNS
    None
*/
static void gapStartBondableConnectionTimer(void)
{
    uint16 timeout_s = sinkBleGetConfiguration()->bonding_connection_period_s;
    
    BLE_GAP_INFO(("GAP gapStartBondableConnectionTimer timeout=[%u s]\n", timeout_s));
    /* Make sure any pending messages are cancelled */
    MessageCancelFirst(sinkGetMainTask(), EventSysBleBondableConnectionTimeout);
    /* Start bonding timeout */
    MessageSendLater(sinkGetMainTask(), EventSysBleBondableConnectionTimeout, 0, D_SEC(timeout_s));
}

/*******************************************************************************
NAME
    gapStopBondableConnectionTimer
    
DESCRIPTION
    Stops a connection timer when the connection is successful or timer expired.
    
PARAMETERS
    None
    
RETURNS
    None
*/
static void gapStopBondableConnectionTimer(void)
{
    BLE_GAP_INFO(("GAP gapStopBondableConnectionTimer\n"));
    /* End bondable mode */
    if (MessageCancelFirst(sinkGetMainTask(), EventSysBleBondableConnectionTimeout))
    {
        /* Send system event if bonding mode was exited */
        MessageSend(sinkGetMainTask(), EventSysBleBondableConnectionTimeout, 0);
    }
}


/*******************************************************************************
NAME
    gapStartBondablePairingTimer
    
DESCRIPTION
    Starts a timer when the GAP bonding role is entered.
    
PARAMETERS
    None
    
RETURNS
    None
*/
static void gapStartBondablePairingTimer(void)
{
    uint16 timeout_s = sinkBleGetConfiguration()->bonding_pairing_period_s;
    
    BLE_GAP_INFO(("GAP gapStartBondable Pairing Timer timeout=[%u s]\n", timeout_s));
    /* Make sure any pending messages are cancelled */
    MessageCancelFirst(sinkGetMainTask(), EventSysBleBondablePairingTimeout);
    /* Start bonding timeout */
    MessageSendLater(sinkGetMainTask(), EventSysBleBondablePairingTimeout, 0, D_SEC(timeout_s));   
}

/*******************************************************************************
NAME
    gapStopBondablePairingTimer
    
DESCRIPTION
    Stops a timer when the GAP bonding role is exited.
    
PARAMETERS
    None
    
RETURNS
    None
*/
static void gapStopBondablePairingTimer(void)
{
    BLE_GAP_INFO(("GAP gapStopBondable Pairing Timer\n"));
    /* End bondable mode */
    if (MessageCancelFirst(sinkGetMainTask(), EventSysBleBondablePairingTimeout))
    {
        /* Send system event if bonding mode was exited */
        MessageSend(sinkGetMainTask(), EventSysBleBondablePairingTimeout, 0);
    }
}

/*******************************************************************************
NAME
    gapRetryEncryption
    
DESCRIPTION
    Retry encryption by sending Security request
    
PARAMETERS
    BD address of the device link to be encrypted
    
RETURNS
    None
*/
static void gapRetryEncryption(const typed_bdaddr *taddr)
{
    if (taddr != NULL)
    {
        BLE_GAP_INFO(("Gatt Retry Encryption request \n"));
        
        ConnectionDmBleSecurityReq(sinkGetBleTask(), 
                                   taddr, 
                                   ble_security_encrypted_bonded,
                                   ble_connection_master_directed
                                   );
    }
}

/*******************************************************************************
NAME
    gapStartEncryption
    
DESCRIPTION
    Start Encryption for Peripheral role
    
PARAMETERS
    paired_device encryption for paired device
    cid connection identifier

RETURNS
     bool encryption has been started or not
*/
static bool gapStartEncryption(bool paired_device, uint16 cid)
{
    gatt_client_connection_t *connection = gattClientFindByCid(cid);
    gatt_client_services_t *data = gattClientGetServiceData(connection);
    gatt_client_discovery_t *discover = gattClientGetDiscoveredServices(connection);
    tp_bdaddr current_bd_addr;
    uint16 service_count = 0;
    bool start_encryption = FALSE;

    /* If the remote device is already paired, then this request has come to after the remote device 
      * is successfully connected. We need to check if the remote device has any service which requires
      * encryption so that we can trigger it from our end */
    if(paired_device)
    {
        if((discover) && VmGetBdAddrtFromCid(cid, &current_bd_addr))
        {
            if((connection) && (connection->role == ble_gap_role_peripheral) && (data))
            {
                for(service_count=0; service_count < data->number_discovered_services ; service_count++)
                {
                    /* Check any services required security */
                    if((discover) && (discover->service == gatt_client_ancs || discover->service == gatt_client_ias ))
                    {
                        start_encryption = TRUE;
                        break;
                    }
                    discover += 1;
                }
            }
        }
    }
    else
    {
        /* The bond timer has expired, if the remote device is still connected then trigger 
          * bonding. ideally the remote device has to trigger bonding checking that there 
          * is an encryption bit in the database. If it fails to do so, trigger it from our end */
        
        if((cid)&&(VmGetBdAddrtFromCid(cid, &current_bd_addr)))
        {
            start_encryption = TRUE;
        }
    }
    BLE_GAP_INFO(("GAP gapStartEncryption Start Encryption : %d \n",start_encryption));

    if(start_encryption)
    {
        ConnectionDmBleSecurityReq(sinkGetBleTask(), 
                        (const typed_bdaddr *)&current_bd_addr.taddr, 
                        ble_security_encrypted_bonded,
                        ble_connection_master_directed
                        );

     }

    return start_encryption;
}

/*******************************************************************************
NAME
    gapStartCentralBonding
    
DESCRIPTION
    Start bonding mode for Central role
    
PARAMETERS
    None
    
RETURNS
    None
*/
static void gapStartBonding(void)
{        
    bool isScanStarted = FALSE;
    bool isAdvertStarted = FALSE;
    BLE_GAP_INFO(("GAP gapStartBonding\n"));
 
    if (sinkBleGetGapState() == ble_gap_state_scanning_advertising)
    {
        /* Must stop scanning before it is restarted with new parameters */
        gapStopScanning();
    }
    
    /* Now in bonding state */
    sinkBleSetGapState(ble_gap_state_bondable_scanning_advertising);
    isScanStarted = gapStartScanning(TRUE);
    isAdvertStarted = gapStartAdvertising();
    /* If either scanning started or advert started or both started, start the timer */
    if ( isScanStarted || isAdvertStarted )
    {
        /* Start bondable connection timer */
        gapStartBondableConnectionTimer();
    }
}

/*******************************************************************************
NAME
    gapStopCentralBonding
    
DESCRIPTION
    Stop bonding mode for Central role and revert to standard scanning
    
PARAMETERS
    None
    
RETURNS
    None
*/
static void gapStopBonding(void)
{        
    BLE_GAP_INFO(("GAP gapStopBonding\n"));
 
    /* Must stop scanning before it is restarted with new parameters */
    gapStopScanning();

    /* Revert to normal scan state */
    sinkBleSetGapState(ble_gap_state_scanning_advertising);
    
    /* Start scanning in general mode */
    gapStartScanning(TRUE);
    gapStartAdvertising();
}

/*******************************************************************************
NAME
    gapSetSecurityFailedDevice 
    
DESCRIPTION
    Called when failure in encryption has occurred with an already paired device
    due to missing link key reported by remote device.
    
PARAMETERS
    tp_addr   Typed address of the remote device.
    
RETURNS
    None
*/
static void gapSetSecurityFailedDevice (const typed_bdaddr * tp_addr)
{
    tp_bdaddr current_addr;
    tp_bdaddr public_addr;

    current_addr.transport = TRANSPORT_BLE_ACL;
    memcpy(&current_addr.taddr,tp_addr, sizeof(typed_bdaddr));

      /* Retreive permanent address if this is a random address */
    if(current_addr.taddr.type == TYPED_BDADDR_RANDOM)
    {
        VmGetPublicAddress(&current_addr, &public_addr);
    }
    else
    {
        public_addr.transport = TRANSPORT_BLE_ACL;
        memcpy(&public_addr.taddr,tp_addr, sizeof(typed_bdaddr));
    }

    /* Mark the device address to be deleted */
    memcpy(&GAP.security_failed_device, &public_addr.taddr, sizeof(typed_bdaddr));
    
    /* Generate user event to indicate encryption failure */
    BLE_GAP_INFO(("GAP gapEncyptionFailureKeyMissing: Encryption failed event\n"));
    MessageSend(&theSink.task, EventSysBleEncryptionFailed , 0);
}

/*******************************************************************************
NAME
    gapClearSecurityFailedDevice
    
DESCRIPTION
    Check and clear tagged security failure device if encryption has succeeded.
    
PARAMETERS
    tp_addr   Typed address of the remote device.
    
RETURNS
    None
*/
static void gapClearSecurityFailedDevice(const typed_bdaddr * tp_addr)
{
    tp_bdaddr public_addr;

    /* Retreive permanent address if this is a random address */
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

    /* Check if this device is earlier marked for security failure */
    if(BdaddrIsSame(&public_addr.taddr.addr, &GAP.security_failed_device.addr))
    {
        /* clear the marked device */
        BdaddrTypedSetEmpty(&GAP.security_failed_device);
    }
}

/*******************************************************************************
NAME
    gapProcessEncryptionFailure
    
DESCRIPTION
    This utility function acts on encryption failure by removing the client.
    
PARAMETERS
    connection                  The GATT client connection

RETURNS
    None.
*/
static void gapProcessEncryptionFailure(gatt_client_connection_t *connection, uint16 cid)
{
    /* Pairing or encryption failed, so disconnect GATT.
      * Basically either pairing/encryption failed, since in case of peripheral we require encryption
      * and in case of central we anyway require pairing. So, failure in either case
      * disconnect the link as we cannot proceed further */

    if(connection)
    {
        /* Maybe gatt client service is added, so we need to remove it */
        gattClientRemoveServices(connection);
        gattClientRemove(cid);
        GattManagerDisconnectRequest(cid);
    }
}

/*******************************************************************************
NAME
    gapStartConnection
    
DESCRIPTION
    Start connection to remote device.
    
PARAMETERS
    taddr   Typed address of the remote device.
    
RETURNS
    None
*/
static void gapStartConnection(const typed_bdaddr *taddr)
{
    /* Set Connection params before connecting */
    gapSetConnectionParamsDefault();
    /* Start connection attempt */
    sinkGattManagerStartConnection(taddr);
}


/*******************************************************************************
NAME
    gapCentralConnectToBondedDevice
    
DESCRIPTION
    Connect to device if the permanent address is bonded.
    
PARAMETERS
    remote_taddr    Lists the current_taddr and permanent_taddr of the device to connect with
    
RETURNS
    None
*/
static bool gapCentralConnectToBondedDevice(const typed_bdaddr *current_taddr, const typed_bdaddr *permanent_taddr)
{          
    bool connection_attempted = FALSE; 

    BLE_GAP_INFO(("GAP Connect to Bonded Device\n"));
    /* First of all check if this is not connect request for the existing link, if yes then don't initiate it */
    if (!BdaddrIsZero(&current_taddr->addr) && !BdaddrIsZero(&permanent_taddr->addr) &&
        !GattGetCidForBdaddr(current_taddr))
    {        
        /* Need to find if permanent_taddr matches an entry in the PDL */
        if (ConnectionSmGetAttributeNow(0, &permanent_taddr->addr, 0, NULL))
        {
            BLE_GAP_INFO(("GAP connect attempt connect_addr=[(%u) %x:%x:%lx]\n",
                      current_taddr->type, current_taddr->addr.nap, current_taddr->addr.uap, current_taddr->addr.lap
                      ));

            gapStartConnection(current_taddr);
            connection_attempted = TRUE;
        }
    }
    
    if (connection_attempted)
    {
        /* Since the connection was attempted, set the GAP conn flag so that no other simentaneous conn is allowed */
        sinkBleGapSetCentralConnAttempt(TRUE, current_taddr, 0, sinkBleGapFindGapConnFlagIndex(NULL));
    }
    
    return connection_attempted;
}


/*******************************************************************************
NAME
    gapCentralConnectToAnyDevice
    
DESCRIPTION
    Connects to device as a result of scanning for advertising devices.
    
PARAMETERS
    current_taddr       The current address of the device to connect with
    permanent_taddr     The permanent address of the device to connect with
    
RETURNS
    None
*/
static bool gapCentralConnectToAnyDevice(const typed_bdaddr *current_taddr, const typed_bdaddr *permanent_taddr)
{          
    bool connection_attempted = FALSE;
    typed_bdaddr connect_addr;
    
    BLE_GAP_INFO(("GAP Connect To Any Device\n"));
    
    /* Clear address that will be used for connection */
    BdaddrTypedSetEmpty(&connect_addr);
      
    /* Check if any addresses are set that can be used to connect */
    if (!BdaddrIsZero(&current_taddr->addr))
    {
        connect_addr = *current_taddr;
    }
    else if (!BdaddrIsZero(&permanent_taddr->addr))
    {
        connect_addr = *permanent_taddr;
    }
    
    if (!BdaddrTypedIsEmpty(&connect_addr) && !GattGetCidForBdaddr(&connect_addr))
    {
        BLE_GAP_INFO(("GAP connect attempt whitelist=[%u]:\n\tconnect_addr=[(%u) %x:%x:%lx]\n\tcurrent_addr=[(%u) %x:%x:%lx]\n\tpermanent_addr=[(%u) %x:%x:%lx]\n",
                      gapGetWhitelistScanActive(),
                      connect_addr.type, connect_addr.addr.nap, connect_addr.addr.uap, connect_addr.addr.lap,
                      current_taddr->type, current_taddr->addr.nap, current_taddr->addr.uap, current_taddr->addr.lap,
                      permanent_taddr->type, permanent_taddr->addr.nap, permanent_taddr->addr.uap, permanent_taddr->addr.lap
                      ));
        /* Connection address is set, start connection attempt */
        gapStartConnection(&connect_addr);
        connection_attempted = TRUE;
    }
    
    if (connection_attempted)
    {
        /* Since the connection was attempted, set the GAP conn flag so that no other simentaneous conn is allowed */
        sinkBleGapSetCentralConnAttempt(TRUE, &connect_addr, 0, sinkBleGapFindGapConnFlagIndex(NULL));
    }

    return connection_attempted;
}


/*******************************************************************************
NAME
    gapStopScanWhileConnecting
    
DESCRIPTION
    Stop scanning while attempting connection to a device
    
PARAMETERS
    None
    
RETURNS
    None
*/
static void gapStopScanWhileConnecting(void)
{

    if (sinkBleGetGapState() == ble_gap_state_bondable_scanning_advertising)
    {
        sinkBleSetGapState(ble_gap_state_bondable_connecting);
    }
    else
    {
        sinkBleSetGapState(ble_gap_state_connecting);
    }
    /* Stop any scanning while connecting */
    gapStopScanning();
}

/*******************************************************************************
NAME
    gapCentralPowerOff
    
DESCRIPTION
    Powers off BLE operation when in Central role.
    
PARAMETERS
    disconnect_immediately  If TRUE will disconnect from any peripheral devices.
    
RETURNS
    None
*/
static void gapPowerOff(bool disconnect_immediately)
{
    /* Check if BLE can be powered off */
    if (!sinkBleIsActiveOnPowerOff())
    {
        gapStopBondableConnectionTimer();
        /* Stop bonding */
        gapStopBondablePairingTimer();
        /* Make sure any scanning is stopped immediately before powering off */
        gapStopScanning();
        /* Try to stop advertsting, if no current connections */
        gapStopAdvertising(ble_gap_adv_speed_fast);
        /* Clear the encryption failure device marked for deletion */
        BdaddrTypedSetEmpty(&GAP.security_failed_device);

        /* Disconnect current connections */
        gattServerDisconnectAll();
        if (disconnect_immediately)
        {
            /* Disconnect all the connections immediately */
            gattClientDisconnectAll();
        }
        /* Set state to IDLE */
        sinkBleSetGapState(ble_gap_state_idle);
    }
}

/*******************************************************************************/
static void gapClearWhiteList(void)
{
    BLE_GAP_INFO(("BLE : Clear the BLE whitelist\n"));

    if (sinkBleGetGapState() != ble_gap_state_idle)
    {
        /*Stop timers*/
        gapStopBondablePairingTimer();
        /* clear the marked device */
        BdaddrTypedSetEmpty(&GAP.security_failed_device);

        /* Disconnect current connections */
        gattClientDisconnectAll();
        gattServerDisconnectAll();
        
        /*If the sink device is scanning, it must be turned off before clearing the whitelist*/
        gapStopScanning();

        /*If the sink device is advertising, it must be turned off before clearing the whitelist*/
        gapStopAdvertising(ble_gap_adv_speed_fast);
        
        /* It's now safe to clear the BLE white list */
        ConnectionDmBleClearWhiteListReq();

        /*Clearing whitelist would be similar to power on, hence check based on default state can be done*/
        sinkBleSetGapState(ble_gap_state_scanning_advertising);
        
        /*In case nothing to be disconnected, send NoConnectionsEvent to start scanning/advertising in respective state*/
        sinkBleCheckNoConnectionsEvent();
    }
    else
    {
        ConnectionDmBleClearWhiteListReq();
    }

    /* Reset bonded with a privacy enabled device*/
    sinkBleGapSetBondedToPrivacyDevice(FALSE);
}

/*******************************************************************************
NAME
    gapStateConnectingHandleEvent
    
DESCRIPTION
    Handles an event in the CONNECTING state. Could also mean it is connected to a device, 
    but still discovering services.
    
PARAMETERS
    event   The event details.
    
RETURNS
    None
*/
static bool gapStateIdleHandleEvent(ble_gap_event_t event)
{
    /* Assume we handle the event below until proved otherwise */
    bool event_handled = TRUE;

    switch (event.id)
    {
        case ble_gap_event_power_on:
        {
            /*On bootup, start whitelist scan/adverts only if LE device available in paired device list*/
            if(ConnectionDmBleCheckTdlDeviceAvailable())
            {
                /* Move to scanning and advertising state */
                sinkBleSetGapState(ble_gap_state_scanning_advertising);
                /* This will trigger scanning and advertising */
                sinkBleCheckNoConnectionsEvent();
            }
            
        }
        break;
         case ble_gap_event_central_conn_complete:
        {
           /* Try to disconnect devices */
            if (!gattClientDisconnectAll())
            {
                /* Check if all disconnected */
                sinkBleCheckNoConnectionsEvent();
            }
        }
        break;

        case ble_gap_event_bondable:
        {
            /*The event is now triggered in a scenario where there were no LE devices paired earlier*/
            gapStartBonding();
        }
        break;

        case ble_gap_event_clear_white_list:
        {
            gapClearWhiteList();
        }
        break;

        /*ignored events*/
        case ble_gap_event_cancelled_advertising:/*powered off after start of advertising*/
        case ble_gap_event_disconn_ind:/*powered off disconn ind received meanwhile*/
        case ble_gap_event_power_off:  /*system in idle state, no LE activity had started because of non availability of devices
                                                                in whitelist*/
                                                                    
        case ble_gap_event_no_connections:/*because of handling of ble_gap_event_central_conn_complete in idle state*/
        {
            BLE_GAP_INFO(("GAP event ignored in state\n"));
        }
        break;
            
        /* Events not expected or handled */
        default:
        {
            event_handled = FALSE;
            BLE_GAP_INFO(("GAP event not handled in state\n"));
        }
        break;
    }
    
    return event_handled;
}

/*******************************************************************************
NAME
    gapStateConnectingHandleEvent
    
DESCRIPTION
    Handles an event in the CONNECTING state. Could also mean it is connected to a device, 
    but still discovering services.
    
PARAMETERS
    event   The event details.
    
RETURNS
    None
*/
static bool gapStateScanAdvHandleEvent(ble_gap_event_t event)
{
    /* Assume we handle the event below until proved otherwise */
    uint8 index;
    bool event_handled = TRUE;

    switch (event.id)
    {
        case ble_gap_event_no_connections:
        {
            /* Read local name to set new advertising data */
            sinkBleGapStartReadLocalName(ble_gap_read_name_advertising);
            /* Restart the scanning if advertising has been cancelled, or a device has disconnected */
            gapStartScanning(TRUE);
        }
        break;

        case ble_gap_event_cancelled_advertising:
        {
            /* Read local name to set new advertising data */
            sinkBleGapStartReadLocalName(ble_gap_read_name_advertising);
        }
        break;

        case ble_gap_event_central_conn_attempt:
        {
            if(!sinkBleGapHasCentralConnAttempt())
            {
                /* Stop scanning while connection is made */
                gapStopScanWhileConnecting();
                /* Attempt connection to bonded device */
                if(!gapCentralConnectToBondedDevice(&event.args->central_conn_attempt.current_taddr,
                                             &event.args->central_conn_attempt.permanent_taddr))
                {
                    BLE_GAP_INFO(("    No addr\n"));
                    /* Restart scanning if connection wasn't initiated */
                    gapStartScanning(FALSE);
                }
            }
        }
        break;

        case ble_gap_event_peripheral_conn_ind:
        {
            /* Stop advertising timer and set interval back to fast */
            gapStopFastAdvTimer();
            gapSetAdvSpeed(ble_gap_adv_speed_fast);

            /* Initiate encryption in case of ancs and ias server connections
                if device is previously bonded then encryption will be successful 
                if not bonded do not allow pairing as soundbar is not in bondable
                state.
            */
            if(gapStartEncryption(TRUE,event.args->connection_id))
            {
                sinkBleSetGapState(ble_gap_state_connecting);
                gapStopAdvertising(ble_gap_adv_speed_slow);
            }
            else if(!gapStartAdvertising())
            {
               gapStopAdvertising(ble_gap_adv_speed_slow);
            }
        }
        break;
        case ble_gap_event_bondable:
        {
            gapStartBonding();
        }
        break;
        case ble_gap_event_set_advertising_complete:
        {
            /* Restart advertising after setting advertising data */
            gapStartAdvertising();
        }
        break;
        case ble_gap_event_disconn_ind:
        {
            /* Maybe ANCS or IAS client for which the discovery is in progress, just reset the flag */
            index = sinkBleGapFindGapConnFlagIndexByCid(event.args->connection_id);
            sinkBleGapSetDiscoveryInProgress(FALSE, NULL, 0, index);
            sinkBleGapSetCentralConnAttempt(FALSE, NULL, 0, index);
            /* Trigger scanning/advertising */
            gapStartScanning(TRUE);
            gapStartAdvertising();
        }
        break;
        case ble_gap_event_fast_adv_timeout:
        {
            /* First stop fast advertising and start slow advertising */
            gapStopAdvertising(ble_gap_adv_speed_slow);
        }
        break;
        case ble_gap_event_fast_scan_timeout:
        {
            /* Store new scan speed */
            gapSetScanSpeed(ble_gap_scan_speed_slow);
            /* Stop current fast scan and fall back to slower scan rate */
            gapStopScanning();
            gapStartScanning(FALSE);
        }
        break;
        case ble_gap_event_power_off:
        {
            gapPowerOff(TRUE);
        }
        break;
        case ble_gap_event_whitelist_timeout:
        {
            /* Switch from whitelist scanning to general scanning to look for private devices */
            gapSetWhitelistScanActive(FALSE);
            gapStopScanning();
            gapStartScanning(FALSE);
        }
        break;
        case ble_gap_event_clear_white_list:
        {
            gapClearWhiteList();
        }
        break;
        
        /* Events not expected or handled */
        default:
        {
            event_handled = FALSE;
            BLE_GAP_INFO(("GAP event not handled in state\n"));
        }
        break;
    }
    
    return event_handled;
}

/*******************************************************************************
NAME
    gapStateConnectingHandleEvent
    
DESCRIPTION
    Handles an event in the CONNECTING state. Could also mean it is connected to a device, 
    but still discovering services.
    
PARAMETERS
    event   The event details.
    
RETURNS
    None
*/
static bool gapStateBondableScanAdvHandleEvent(ble_gap_event_t event)
{
    /* Assume we handle the event below until proved otherwise */
    bool event_handled = TRUE;

    switch (event.id)
    {
        case ble_gap_event_bondable_connection_timeout:
        case ble_gap_event_bondable_pairing_timeout:
        {
             /* If we are still trying to do discovery, then wait for it to complete,
                after that we shall trigger the pairing */
            if(!sinkBleGapIsDiscoveryInProgress(
                            sinkBleGapFindGapConnFlagIndexByCid(
                                                                                        gattCommonFindCidBondingDevice())))
            {
                if(!gapStartEncryption(FALSE,gattCommonFindCidBondingDevice()))
                {
                    gapStopBonding();
                }
                /* Done with pairing, reset the flag */
                gattCommonSetDeviceBonding(gattCommonFindCidBondingDevice(), FALSE);
            }
            else
            {
               /* Restart pairing timer since we are still in discovery process */
               gapStartBondablePairingTimer();
            }
        }
        break;
        case ble_gap_event_central_conn_attempt:
        {
            if(!sinkBleGapHasCentralConnAttempt())
            {
                /* Stop advertising also */
                gapStopAdvertising(ble_gap_adv_speed_fast);
                /* Stop scanning while connection is made */
                gapStopScanWhileConnecting();
                /* Stop connection timeout */
                gapStopBondableConnectionTimer();
                /* Attempt connection to advertising device */
                if(!gapCentralConnectToAnyDevice(&event.args->central_conn_attempt.current_taddr,
                                             &event.args->central_conn_attempt.permanent_taddr))
                {
                    BLE_GAP_INFO(("    No addr\n"));
                    /* Restart scanning if connection wasn't initiated */
                    gapStartScanning(FALSE);
                }
            }
        }
        break;
        case ble_gap_event_central_conn_complete:
        {
            if (event.args != NULL)
            {
                /* Update connection parameters after successful connection */
                sinkBleSetMasterConnectionParamsUpdate(&event.args->master_conn_complete.taddr);
                sinkBleGapSetCentralConnAttempt(FALSE, NULL, 0,
                                            sinkBleGapFindGapConnFlagIndex(&event.args->master_conn_complete.taddr));
            }
            else
            {
                /* could be that the connection didnt go through, so had to stop the connection
                    result of which central_conn_complete event is sent with args set to NULL.
                    However GAP central conn attempt flag needs to be reset */
                    sinkBleGapResetCentralConnAttempt();
            }
            /* Restart scanning after connection attempt */
            gapStartScanning(TRUE);
            gapStartAdvertising();
        }
        break;
        case ble_gap_event_peripheral_conn_ind:
        {
            /* Stop advertising timer and set interval back to fast */
            gapStopScanWhileConnecting();
            gapStopAdvertising(ble_gap_adv_speed_fast);

            /* Stop connection timeout */
            gapStopBondableConnectionTimer();
            /* Start the bondable timeout */
            gapStartBondablePairingTimer();
            /* Changing the state */
            sinkBleSetGapState(ble_gap_state_bondable_connected);
            /* Set the bonding flag */
            gattCommonSetDeviceBonding(event.args->connection_id, TRUE);
        }
        break;
        case ble_gap_event_set_advertising_complete:
        {
            /* Restart advertising after setting advertising data */
            gapStartAdvertising();
        }
        break;
        case ble_gap_event_disconn_ind:
        {
            /* If at all discovery was in progresss, reset the flag */
            sinkBleGapSetDiscoveryInProgress(FALSE, NULL, 0,
                                sinkBleGapFindGapConnFlagIndexByCid(event.args->connection_id));
            sinkBleGapSetCentralConnAttempt(FALSE, NULL, 0,
                                sinkBleGapFindGapConnFlagIndexByCid(event.args->connection_id));
            gapStartScanning(TRUE);
            gapStartAdvertising();
        }
        break;
        case ble_gap_event_cancelled_advertising:
        {
            /* Read local name to set new advertising data */
            sinkBleGapStartReadLocalName(ble_gap_read_name_advertising);
        }
        break;
        case ble_gap_event_fast_adv_timeout:
        {
            /* First stop fast advertising and start slow advertising */
            gapStopAdvertising(ble_gap_adv_speed_slow);
        }
        break;
        case ble_gap_event_fast_scan_timeout:
        {
            /* Store new scan speed */
            gapSetScanSpeed(ble_gap_scan_speed_slow);
            /* Stop current fast scan and fall back to slower scan rate */
            gapStopScanning();
            gapStartScanning(FALSE);
        }
        break;
        case ble_gap_event_power_off:
        {
            gapPowerOff(TRUE);
        }
        break;
        case ble_gap_event_clear_white_list:
        {
            gapClearWhiteList();
        }
        break;
        
        /* Events not expected or handled */
        default:
        {
            event_handled = FALSE;
            BLE_GAP_INFO(("GAP event not handled in state\n"));
        }
        break;
    }
    
    return event_handled;
}

/*******************************************************************************
NAME
    gapStateConnectingHandleEvent
    
DESCRIPTION
    Handles an event in the CONNECTING state. Could also mean it is connected to a device, 
    but still discovering services.
    
PARAMETERS
    event   The event details.
    
RETURNS
    None
*/
static bool gapStateConnectingHandleEvent(ble_gap_event_t event)
{
    /* Assume we handle the event below until proved otherwise */
    uint8 index;
    bool event_handled = TRUE;

    switch (event.id)
    {
        case ble_gap_event_central_conn_attempt:
        {
            if(!sinkBleGapHasCentralConnAttempt())
            {
                /* Stop scanning while connection is made */
                gapStopScanWhileConnecting();
                /* Attempt connection to bonded device */
                if(!gapCentralConnectToBondedDevice(&event.args->central_conn_attempt.current_taddr,
                                             &event.args->central_conn_attempt.permanent_taddr))
                {
                     BLE_GAP_INFO(("    No addr\n"));
                    /* Restart scanning if connection wasn't initiated */
                    gapStartScanning(FALSE);
                }
            }
        }
        break;
        case ble_gap_event_central_conn_complete:
        {
            if (event.args != NULL)
            {
                /* Update connection parameters after successful connection */
                sinkBleSetMasterConnectionParamsUpdate(&event.args->master_conn_complete.taddr);
                /* Restart scanning after connection attempt */
                index = sinkBleGapFindGapConnFlagIndex(&event.args->master_conn_complete.taddr);
                sinkBleGapSetCentralConnAttempt(FALSE, NULL, 0, index);
            }
            else
            {
                /* could be that the connection didnt go through, so had to stop the connection
                    result of which central_conn_complete event is sent with args set to NULL.
                    However GAP central conn attempt flag needs to be reset */
                sinkBleGapResetCentralConnAttempt();
            }
            gapStartScanning(TRUE);
        }
        break;

        case ble_gap_event_peripheral_conn_ind:
        {
            /* Stop advertising timer and set interval back to fast */
            gapStopFastAdvTimer();
            gapSetAdvSpeed(ble_gap_adv_speed_fast);

            /* Initiate encryption in case of ancs and ias server connections
                if device is previously bonded then encryption will be successful 
                if not bonded do not allow pairing as soundbar is not in bondable
                state.
            */
            if(gapStartEncryption(TRUE,event.args->connection_id))
            {
                sinkBleSetGapState(ble_gap_state_connecting);
                gapStopAdvertising(ble_gap_adv_speed_slow);
            }
            else if(!gapStartAdvertising())
            {
               gapStopAdvertising(ble_gap_adv_speed_slow);
            }
        }
        break;
        case ble_gap_event_bondable:
        {
            sinkBleSetGapState(ble_gap_state_bondable_connecting);
        }
        break;
        case ble_gap_event_retry_encryption:
        {
              if (event.args != NULL)
              {
                  gapRetryEncryption((const typed_bdaddr *)&event.args->encryption_retry.taddr);
              }
        }
        break;
        case ble_gap_event_disconn_ind:
        {
            /* If at all discovery was in progresss, reset the flag */
            index = sinkBleGapFindGapConnFlagIndexByCid(event.args->connection_id);
            sinkBleGapSetDiscoveryInProgress(FALSE, NULL, 0, index);
            sinkBleGapSetCentralConnAttempt(FALSE, NULL, 0, index);
            /* Don't change the state, because we might be still trying to connect cenral
             * device. Anyhow, if are not connecting, then gapStartScanning, shall change the 
             * state to scanning_advertising */
            gapStartAdvertisingInConnecting();
            /* If there is a connection attemp, means that remote server connection
             * is in progress, even if disconnected, need to wait until this connection
             * is either successfully complete, or this also gets disconnected */
            if(!sinkBleGapHasCentralConnAttempt())
            {
                gapStartScanning(TRUE);
            }
        }
        break;
        case ble_gap_event_power_off:
        {
            gapPowerOff(FALSE);
        }
        break;
        case ble_gap_event_clear_white_list:
        {
            gapClearWhiteList();
        }
        break;

        /*ignored events*/
        case ble_gap_event_cancelled_advertising:/* stop advertising was called while making connection*/
        {
            BLE_GAP_INFO(("GAP event ignored in state\n"));
        }
        break;

        /* Events not expected or handled */
        default:
        {
            event_handled = FALSE;
            BLE_GAP_INFO(("GAP event not handled in state\n"));
        }
        break;
    }
    
    return event_handled;
}

/*******************************************************************************
NAME
    gapStateConnectedHandleEvent
    
DESCRIPTION
    Handles an event in the CONNECTING state. Could also mean it is connected to a device, 
    but still discovering services.
    
PARAMETERS
    event   The event details.
    
RETURNS
    None
*/
static bool gapStateConnectedHandleEvent(ble_gap_event_t event)
{
    /* Assume we handle the event below until proved otherwise */
    uint8 index;
    bool event_handled = TRUE;

    switch (event.id)
    {
        case ble_gap_event_disconn_ind:
        case ble_gap_event_no_connections:
        {
            if(event.args != NULL)
            {
                index = sinkBleGapFindGapConnFlagIndexByCid(event.args->connection_id);
                sinkBleGapSetCentralConnAttempt(FALSE, NULL, 0, index);
            }
            gapStartScanning(TRUE);
            gapStartAdvertising();
        }
        break;
        case ble_gap_event_power_off:
        {
            gapPowerOff(TRUE);
        }
        break;
        case ble_gap_event_peripheral_conn_ind:
        {
            gapStartEncryption(TRUE,event.args->connection_id);
        }
        break;
        case ble_gap_event_central_conn_complete:
        {
            if (event.args != NULL)
            {
                /* Update connection parameters after successful connection */
                sinkBleSetMasterConnectionParamsUpdate(&event.args->master_conn_complete.taddr);
                index = sinkBleGapFindGapConnFlagIndex(&event.args->master_conn_complete.taddr);
                sinkBleGapSetCentralConnAttempt(FALSE, NULL, 0, index);
            }
            else
            {
                /* could be that the connection didnt go through, so had to stop the connection
                    result of which central_conn_complete event is sent with args set to NULL.
                    However GAP central conn attempt flag needs to be reset */
                sinkBleGapResetCentralConnAttempt();
            }
        }
        break;
        case ble_gap_event_clear_white_list:
        {
            gapClearWhiteList();
        }
        break;
        
        /* Events not expected or handled */
        default:
        {
            event_handled = FALSE;
            BLE_GAP_INFO(("GAP event not handled in state\n"));
        }
        break;
    }
    
    return event_handled;
}

/*******************************************************************************
NAME
    gapStateBondableConnectingHandleEvent
    
DESCRIPTION
    Handles an event in the BONDABLE_CONNECTING state. Could also mean it is connected to a device, 
    but still discovering services.
    
PARAMETERS
    event   The event details.
    
RETURNS
    None
*/
static bool gapStateBondableConnectingHandleEvent(ble_gap_event_t event)
{
    /* Assume we handle the event below until proved otherwise */
    uint8 index;
    bool event_handled = TRUE;

    switch (event.id)
    {
        case ble_gap_event_central_conn_complete:
        {
            if(event.args != NULL)
            {
                /* Update connection parameters after successful connection */
                sinkBleSetMasterConnectionParamsUpdate(&event.args->master_conn_complete.taddr);
                
                /* Restart scanning after bondable connection attempt */
                index = sinkBleGapFindGapConnFlagIndex(&event.args->master_conn_complete.taddr);
                sinkBleGapSetCentralConnAttempt(FALSE, NULL, 0, index);
            }
            else
            {
                /* could be that the connection didnt go through, so had to stop the connection
                    result of which central_conn_complete event is sent with args set to NULL.
                    However GAP central conn attempt flag needs to be reset */
                sinkBleGapResetCentralConnAttempt();
            }
            gapStartScanning(TRUE);
            gapStartAdvertising();
        }
        break;
        case ble_gap_event_retry_encryption:
        {
              if (event.args != NULL)
              {
                  gapRetryEncryption((const typed_bdaddr *)&event.args->encryption_retry.taddr);
              }
        }
        break;
        case ble_gap_event_disconn_ind:
        {
            /* If at all discovery was in progresss, reset the flag */
            index = sinkBleGapFindGapConnFlagIndexByCid(event.args->connection_id);
            sinkBleGapSetDiscoveryInProgress(FALSE, NULL, 0, index);
            sinkBleGapSetCentralConnAttempt(FALSE, NULL, 0, index);
            /* Don't allow either scanning or advertisment untill the remote server
              * device which we were attempting to connect itself disconnected */
            if(!sinkBleGapHasCentralConnAttempt())
            {
                gapStartScanning(TRUE);
                gapStartAdvertising();
            }
        }
        break;
        case ble_gap_event_power_off:
        {
            gapPowerOff(FALSE);
        }
        break;
        case ble_gap_event_clear_white_list:
        {
            gapClearWhiteList();
        }
        break;

        /*ignored events*/
        case ble_gap_event_cancelled_advertising:/* stop advertising was called while making connection*/
        {
            BLE_GAP_INFO(("GAP event ignored in state\n"));
        }
        break;


        /* Events not expected or handled */
        default:
        {
            event_handled = FALSE;
            BLE_GAP_INFO(("GAP event not handled in state\n"));
        }
        break;
    }
    
    return event_handled;
}

/*******************************************************************************
NAME
    gapStateBondableConnectedHandleEvent
    
DESCRIPTION
    Handles an event in the BONDABLE_CONNECTED state. Could also mean it is connected to a device, 
    but still discovering services.
    
PARAMETERS
    event   The event details.
    
RETURNS
    None
*/
static bool gapStateBondableConnectedHandleEvent(ble_gap_event_t event)
{
    /* Assume we handle the event below until proved otherwise */
    uint8 index;
    bool event_handled = TRUE;

    switch (event.id)
    {
        case ble_gap_event_bondable_pairing_timeout:
        {
            /* If we are still trying to do discovery, then wait for it to complete,
                after that we shall trigger the pairing */
            index = sinkBleGapFindGapConnFlagIndexByCid(gattCommonFindCidBondingDevice());
            if(!sinkBleGapIsDiscoveryInProgress(index))
            {
                 if(!gapStartEncryption(FALSE,gattCommonFindCidBondingDevice()))
                {
                    gapStopBonding();
                }
                /* Done with pairing, reset the flag */
                gattCommonSetDeviceBonding(gattCommonFindCidBondingDevice(), FALSE);
            }
            else
            {
               /* Restart pairing timer since we are still in discovery process */
               gapStartBondablePairingTimer();
            }
        }
        break;
        case ble_gap_event_retry_encryption:
        {
            if (event.args != NULL)
            {
                BLE_GAP_INFO(("Gatt Retry Encryption request \n"));
                
                ConnectionDmBleSecurityReq(sinkGetBleTask(), 
                                            (const typed_bdaddr *)&event.args->encryption_retry.taddr, 
                                            ble_security_encrypted_bonded,
                                            ble_connection_master_directed
                                            );
            }
        }
        break;
        case ble_gap_event_encyption_complete:
        {
             gapSetAdvSpeed(ble_gap_adv_speed_fast);
             gapStartAdvertising();
             /* Restart the scanning if advertising has been cancelled, or a device has disconnected */
             gapStartScanning(TRUE);
        }
        break;
        case ble_gap_event_no_connections:
        {
            /* Stop the bond timer */
            gapStopBondablePairingTimer();
            /* There was a disconnect, so change the state */
            sinkBleSetGapState(ble_gap_state_scanning_advertising);
            /* Start advertising if no Central connections */
            /* Read local name to set new advertising data */
            sinkBleGapStartReadLocalName(ble_gap_read_name_advertising);
            /* Restart the scanning if advertising has been cancelled, or a device has disconnected */
            gapStartScanning(TRUE);
        }
        break;
        case ble_gap_event_disconn_ind:
        {
            /* If at all discovery was in progresss, reset the flag */
            index = sinkBleGapFindGapConnFlagIndexByCid(event.args->connection_id);
            sinkBleGapSetDiscoveryInProgress(FALSE, NULL, 0, index);
            sinkBleGapSetCentralConnAttempt(FALSE, NULL, 0, index);
           /* Dont trigger either advertisment or scanning, until the device the remote central 
            * device which we were trying to connect itself disconnected */
            if(!gattCommonFindCidBondingDevice())
            {
                /* No point in waiting for bond timer */
                gapStopBondablePairingTimer();
                gapStartScanning(TRUE);
                gapStartAdvertising();
            }
        }
        break;
        case ble_gap_event_power_off:
        {
            gapPowerOff(TRUE);
        }
        break;
        case ble_gap_event_clear_white_list:
        {
            gapClearWhiteList();
        }
        break;
        
        /*ignored events*/
        case ble_gap_event_cancelled_advertising:/*scenario of no more connections, hence stop advertising was called*/
        {
            BLE_GAP_INFO(("GAP event ignored in state\n"));
        }
        break;
     
        default:
        {
            event_handled = FALSE;
            BLE_GAP_INFO(("GAP event not handled in state\n"));
        }
        break;
    }
    
    return event_handled;
}

/******************************************************************************/
void sinkBleGapInitialise(void)
{
    /* Set initial GAP state */
    sinkBleSetGapState(ble_gap_state_idle);

    /* Set the inital GAP conn flags */
    sinkBleGapInitGapConnFlag();
    
    /* Set if initially bonded to private device */
    sinkBleGapSetBondedToPrivacyDevice(ConnectionBondedToPrivacyEnabledDevice());
    
    /* Set initial advertising speed */
    gapSetAdvSpeed(ble_gap_adv_speed_fast);

}


/******************************************************************************/
void sinkBleGapEvent(ble_gap_event_t event)
{
    /* Indication if event handled by current state */
    bool event_handled = FALSE;
    ble_gap_state_t state = sinkBleGetGapState();

    BLE_GAP_INFO(("GAP new event=[%s] state=[%s]\n", gap_events[event.id], gap_states[state]));
    
    switch (state)
    {
        case ble_gap_state_idle:
        {
            BLE_GAP_SM_INFO(("GAP state=[IDLE] Event=%d\n", event.id));
            event_handled = gapStateIdleHandleEvent(event);
        }
        break;
        case ble_gap_state_scanning_advertising:
        {
             BLE_GAP_SM_INFO(("GAP state=[SCANNING_ADVERTISING] Event=%d\n", event.id));
            event_handled = gapStateScanAdvHandleEvent(event);
        }
        break;
        case ble_gap_state_bondable_scanning_advertising:
        {
            BLE_GAP_SM_INFO(("GAP state=[BONDABLE_SCANNING_ADVERTISING] Event=%d\n", event.id));
            event_handled = gapStateBondableScanAdvHandleEvent(event);
        }
        break;
        case ble_gap_state_connecting:
        {
            BLE_GAP_SM_INFO(("GAP state=[CONNECTING] Event=%d\n", event.id));
            event_handled = gapStateConnectingHandleEvent(event);
        }
        break;
        case ble_gap_state_fully_connected:
        {
            BLE_GAP_SM_INFO(("GAP state=[CONNECTED] Event=%d\n", event.id));
            event_handled = gapStateConnectedHandleEvent(event);
        }
        break;
        case ble_gap_state_bondable_connecting:
        {
            BLE_GAP_SM_INFO(("GAP state=[BONDABLE_CONNECTING] Event=%d\n", event.id));
            event_handled = gapStateBondableConnectingHandleEvent(event);
        }
        break;
        case ble_gap_state_bondable_connected:
        {
            BLE_GAP_SM_INFO(("GAP state=[BONDABLE_CONNECTED] Event=%d\n", event.id));
            event_handled = gapStateBondableConnectedHandleEvent(event);
        }
        break;
        default:
        {
            BLE_GAP_SM_ERROR(("GAP event in unknown state\n"));
        }
        break;
    }
    
    if (!event_handled)
    {
        BLE_GAP_SM_ERROR(("Check behaviour of GAP event=[%d] in state=[%d]!\n", event.id, state));
    }
}

/******************************************************************************/
void sinkBleGapStartReadLocalName(ble_gap_read_name_t reason)
{
    if (!(GAP.name_read & reason))
    {
        GAP.name_read |= reason;
    
        ConnectionReadLocalName(sinkGetBleTask());
    }
}


/******************************************************************************/
void sinkBleGapReadLocalNameComplete(CL_DM_LOCAL_NAME_COMPLETE_T * cfm)
{      
    BLE_GAP_INFO(("CL_DM_LOCAL_NAME_COMPLETE\n"));
    
    if (cfm->status != success)
    {
        BLE_GAP_ERROR(("  Failed!\n"));
    }
    
    if (GAP.name_read & ble_gap_read_name_gap_server)
    {
        /* Use local name to respond as GAP server */
        BLE_GAP_INFO(("    Gap server response\n"));
        sinkGattGapServerSendLocalNameResponse(cfm);
    }
    if (GAP.name_read & ble_gap_read_name_advertising)
    {
        /* Use local name to setup advertising data */
        BLE_GAP_INFO(("    Set advertising data bondable=[%u]\n", sinkBleGetGapState() == ble_gap_state_bondable_scanning_advertising ? TRUE : FALSE));
        bleSetupAdvertisingData(cfm->size_local_name, 
                                cfm->local_name, 
                                sinkBleGetGapState() == ble_gap_state_bondable_scanning_advertising ? adv_discoverable_mode_limited : adv_non_discoverable_mode);
    }
    
    GAP.name_read = 0;
}

/******************************************************************************/
void sinkBleGapSetSecurityCfm(CL_DM_BLE_SECURITY_CFM_T * cfm)
{
    gatt_client_connection_t *connection = NULL;
    ble_gap_role_t connection_role = ble_gap_role_unknown;
    uint16 cid = GattGetCidForBdaddr(&cfm->taddr);

    /* It is possible to get invalid cid from GATT library, because there could be race condition
     * where in even before the security cfm was processed, the link is already disconnected.
     * in that case, we need not process the cfm.
     * The memory allocated for client services shall get freed as part of handling the
     * disconnect indication */
    if(cid != GATT_CLIENT_INVALID_CID)
    {
        connection = gattClientFindByCid(cid);
        BLE_GAP_INFO(("CL_DM_BLE_SECURITY_CFM status=[0x%x]\n", cfm->status));

        if(connection != NULL)
        {
            connection_role = connection->role;

            if (cfm->status == ble_security_success)
            {
                /* Add device to whitelist */
                ConnectionDmBleAddDeviceToWhiteListReq(cfm->taddr.type, 
                                                        &cfm->taddr.addr);
                gattClientInitialiseDiscoveredServices(connection);
                /* if this success is for tagged security failure device then untag the device */
                gapClearSecurityFailedDevice(&cfm->taddr);

                /* the link is encrypted, inform the user if pairing was done using secure connection */
                sinkBleSetLeLinkIndication(&cfm->taddr);
                
                /* Send Connection Parameters Update Request now instead of
                   just after connection is established.
                   This is needed to avoid sending this request too early
                   i.e. in the first connection event, which seems to cause
                   issues while pairing with iOS devices.
                 */
                if(connection_role == ble_gap_role_peripheral)
                {
                    sinkBleSetSlaveConnectionParamsUpdate(&cfm->taddr);
                }
            }
            else if(cfm->status == ble_security_pairing_in_progress)
            {
                /* LE pairing is inprogress re schedule encryption */
                sinkBleRetryEncryptionEvent(cid);
            }
            else
            {
                /* If encryption failed due to link key missing tag the device */
                if(cfm->status == ble_security_link_key_missing)
                    gapSetSecurityFailedDevice(&cfm->taddr);

                 /* Remove the connection */
                 gapProcessEncryptionFailure(connection, cid);

                /* Restart scanning if in central mode */
                if(connection_role == ble_gap_role_central)
                {
                    /* Connection is now complete after setting security */
                    sinkBleMasterConnCompleteEvent(cid);
                }
            }
         }

        sinkBleEncryptionCompleteEvent();
        gapStopBondablePairingTimer();
    }
}

/******************************************************************************/
void sinkBleGapAddDeviceWhiteListCfm(const CL_DM_BLE_ADD_DEVICE_TO_WHITE_LIST_CFM_T * cfm)
{
    if ((cfm->status==success) && (sinkBleGapGetBondedToPrivacyDevice()!=TRUE))
    {
        /* Set bonded with a privacy enabled device*/
        sinkBleGapSetBondedToPrivacyDevice(ConnectionBondedToPrivacyEnabledDevice());
    }
}

/******************************************************************************/
bool sinkBleGapIsBondable(void)
{
    ble_gap_state_t state = sinkBleGetGapState();
    bool bondable = FALSE;
    
    switch (state)
    {
        case ble_gap_state_bondable_scanning_advertising:
        case ble_gap_state_bondable_connecting:
        case ble_gap_state_bondable_connected:
            bondable = TRUE;
            break;
         default:
            bondable = FALSE;
            break;
    }
    
    return bondable;
}

/******************************************************************************/
bool sinkBleGapIsConnectable(void)
{
    ble_gap_state_t state = sinkBleGetGapState();
    bool connectable = FALSE;

    switch(state)
    {
        case ble_gap_state_scanning_advertising:
        case ble_gap_state_bondable_scanning_advertising:
        case ble_gap_state_connecting:
            connectable = TRUE;
            break;

        default:
            connectable = FALSE;
            break;
    }

    BLE_GAP_INFO(("BLE is in %s state\n", (connectable)?"Connectable":"Not Connectable"));
    return connectable;
}

#endif /* GATT_ENABLED */

