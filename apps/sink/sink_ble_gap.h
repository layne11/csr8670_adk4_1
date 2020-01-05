/****************************************************************************
Copyright (c) 2014 - 2016 Qualcomm Technologies International, Ltd.
Part of ADK 4.1

FILE NAME
    sink_ble_gap.h

DESCRIPTION
    BLE GAP functionality
*/

#ifndef _SINK_BLE_GAP_H_
#define _SINK_BLE_GAP_H_


#include <connection.h>
#include <csrtypes.h>


#define GAP theSink.rundata->ble.gap

#define MAX_BLE_CONNECTIONS 2

/* GAP events IDs */
typedef enum __ble_gap_event_id
{
     /*!  Event for Power ON . Used in bootup. */
    ble_gap_event_power_on,                         /* 0 */
    
     /*! Event to make the device in bondable mode. */
    ble_gap_event_bondable,                          /* 1 */

     /*! Event to set that advertising data is populated 
           and ready to start advertising. */
    ble_gap_event_set_advertising_complete, /* 2 */

     /*! Event to indicate that timeout had occured for device
           after start of pairing. */
    ble_gap_event_bondable_pairing_timeout,  /* 3 */
    
     /*! Event to trigger connection to a peripheral device */
    ble_gap_event_central_conn_attempt,        /* 4 */
    
     /*! Event to indicate connection to peripheral device has completed with 
           all GATT procedures*/
    ble_gap_event_central_conn_complete,       /* 5 */
    
     /*! Event to indicate connection from a central device had occured  */
    ble_gap_event_peripheral_conn_ind,                   /* 6 */
    
    /*! Event to inform disconnection */
    ble_gap_event_disconn_ind,                        /* 7 */
    
     /*! Event to indicate the power off */
    ble_gap_event_power_off,                           /* 8 */
    
     /*! Event used in case of privacy enabled devices . Whitelist scan is timed out 
           and new scan to be started*/
    ble_gap_event_whitelist_timeout,                /* 9 */

     /*! Event to indicate that fast scan timeout occured*/
    ble_gap_event_fast_scan_timeout,              /* 10 */
    
     /*! Event to indicate that fast advertising timeout occured*/
    ble_gap_event_fast_adv_timeout,               /* 11 */
    
     /*! Event to retry encryption */
    ble_gap_event_retry_encryption,                 /* 12 */

     /*! Event to indicate that there are 
           no LE connections available */
    ble_gap_event_no_connections,                   /* 13 */

     /*! Event to indicate that advertising 
           has been already cancelled as a result of stop advertisement */
    ble_gap_event_cancelled_advertising,               /* 14 */

      /*! Event to indicate that bondable connection timeout 
           had occured */
    ble_gap_event_bondable_connection_timeout,  /* 15 */

      /*! Event to indicate that encryption 
           is completed */
    ble_gap_event_encyption_complete,              /* 16 */

      /*! Event to take necessary step for clearing the whitelisted
            devices */
    ble_gap_event_clear_white_list,             /* 17 */
    
    /* Update gap_events[] array if adding new item */
    ble_gap_event_last                      /* Always leave as last item in enum */ 
} ble_gap_event_id_t;

/* GAP states */
typedef enum __ble_gap_state
{
    /*!  GAP is in idle state after power up */
    ble_gap_state_idle,                                                 /* 0 */

    /*! GAP state to scan for whitelisted devices 
       and advertising */
    ble_gap_state_scanning_advertising,                        /* 1 */

    /*! GAP state to scan for new devices 
       and advertising. Triggered by a bondable event */
    ble_gap_state_bondable_scanning_advertising,        /* 2 */

    /*! GAP state while making connection and updating information on 
    services of remote central or peripheral device*/
    ble_gap_state_connecting,                                       /* 3 */

   /*! GAP state that permitted number of LE connections are all connected
         No more space for any further connections.*/
    ble_gap_state_fully_connected,                                /* 4 */

    /*! GAP state while making connection and updating information on 
    services of a newly paired remote peripheral device*/
    ble_gap_state_bondable_connecting,                        /* 5 */

    /*! GAP state while making connection and updating information on 
    services with a Central device*/
    ble_gap_state_bondable_connected,                        /* 6 */
    
    /* Update gap_states[] array if adding new item */
    ble_gap_state_last                      /* Always leave as last item in enum */
} ble_gap_state_t;


/* GAP event args for ble_gap_event_central_conn_attempt */
typedef struct __ble_gap_args_central_conn_attempt
{
    typed_bdaddr current_taddr;
    typed_bdaddr permanent_taddr;
} ble_gap_args_central_conn_attempt_t;

/* GAP event args for ble_gap_event_central_conn_complete */
typedef struct __ble_gap_args_central_conn_complete
{
    typed_bdaddr taddr;
} ble_gap_args_central_conn_complete_t;

/* GAP event args for ble_gap_event_retry_encryption */
typedef struct __ble_gap_args_retry_encryption
{
    typed_bdaddr taddr;
} ble_gap_args_retry_encryption_t;

/* Union of args used for GAP events */
typedef union __ble_gap_event_args
{
    ble_gap_args_central_conn_attempt_t central_conn_attempt;
    ble_gap_args_central_conn_complete_t master_conn_complete;
    ble_gap_args_retry_encryption_t encryption_retry;
    uint16 connection_id;
} ble_gap_event_args_t;

/* GAP event structure */
typedef struct __ble_gap_event
{
    ble_gap_event_id_t id;
    ble_gap_event_args_t *args;
} ble_gap_event_t;

/* GAP roles */
typedef enum __ble_gap_role
{
    ble_gap_role_unknown,
    ble_gap_role_central,
    ble_gap_role_peripheral
} ble_gap_role_t;

/* Read name reason types (used for bitmask) */
typedef enum __ble_gap_read_name
{
    ble_gap_read_name_advertising = 1,
    ble_gap_read_name_gap_server = 2
} ble_gap_read_name_t;

/* Scan speed */
typedef enum __ble_gap_scan_speed
{
    ble_gap_scan_speed_fast,
    ble_gap_scan_speed_slow
} ble_gap_scan_speed_t;

/* GAP scan state structure */
typedef struct __ble_gap_scan_state
{
    ble_gap_scan_speed_t speed;             /* Stores current scan speed */
    unsigned whitelist_active:1;            /* Flag used to indicate if using whitelist */
} ble_gap_scan_state_t;

/* Advertising speed */
typedef enum __ble_gap_adv_speed
{
    ble_gap_adv_speed_fast,
    ble_gap_adv_speed_slow
} ble_gap_adv_speed_t;

/* GAP advertising state structure */
typedef struct __ble_gap_adv_state
{
    ble_gap_adv_speed_t speed;              /* Stores current advertising speed */
} ble_gap_adv_state_t;

typedef struct __ble_gap_conn_flags
{
    unsigned service_discovery_inprogress:1;/* Flag used to indicated if there is client primary discovery in progress */
    unsigned central_conn_attempt:1; /* Flag used to indicate if master has already initiated connection */
    uint16 cid; /* Stores the cid of the connection */
    typed_bdaddr taddr; /* address of the device for which connection/discovery is in progress */
}ble_gap_conn_flags;

/* GAP variables */
typedef struct __ble_gap
{
    ble_gap_state_t state;                  /* The current GAP state */
    ble_gap_read_name_t name_read;          /* Used to store why the local name is being read */
    typed_bdaddr  security_failed_device;   /* The remote device marked for deletion due to security failure */
    unsigned bonded_to_private_device:1;    /* Flag used to indicate if bonded to private device */
    ble_gap_scan_state_t scan;              /* Used to store current scan state */
    ble_gap_adv_state_t adv;                /* Used to store current advertising state */
    ble_gap_conn_flags gap_conn_flags[MAX_BLE_CONNECTIONS]; /* The GAP flags based on conn */
} ble_gap_t;

/*******************************************************************************
NAME
    sinkBleGapInitialise
    
DESCRIPTION
    Initialises the GAP module.
    
PARAMETERS
    None
    
RETURNS
    None
*/
#ifdef GATT_ENABLED
void sinkBleGapInitialise(void);
#else
#define sinkBleGapInitialise() ((void)(0))
#endif


/*******************************************************************************
NAME
    sinkBleGapEvent
    
DESCRIPTION
    Initiates a GAP event.
    
PARAMETERS
    event   The GAP event
    
RETURNS
    TRUE if the event was handled. FALSE otherwise.
*/
#ifdef GATT_ENABLED
void sinkBleGapEvent(ble_gap_event_t event);
#else
#define sinkBleGapEvent(event) ((void)(0))
#endif

/*******************************************************************************
NAME
    sinkBleGapStartReadLocalName
    
DESCRIPTION
    Read local name for BLE operations.
    
PARAMETERS
    None
    
RETURNS
    None
*/
#ifdef GATT_ENABLED
void sinkBleGapStartReadLocalName(ble_gap_read_name_t reason);
#else
#define sinkBleGapStartReadLocalName(reason) ((void)(0))
#endif


/*******************************************************************************
NAME
    sinkBleGapReadLocalNameComplete
    
DESCRIPTION
    Read local name complete for BLE operations.
    
PARAMETERS
    None
    
RETURNS
    None
*/
#ifdef GATT_ENABLED
void sinkBleGapReadLocalNameComplete(CL_DM_LOCAL_NAME_COMPLETE_T * cfm);
#else
#define sinkBleGapReadLocalNameComplete(cfm) ((void)(0))
#endif

/*******************************************************************************
NAME
    sinkBleGapAddDeviceWhiteListCfm
    
DESCRIPTION
    Handle Add Device whitelist cfm. 
    Added to check for privacy devices been bonded.
    
PARAMETERS
    None
    
RETURNS
    None
*/
#ifdef GATT_ENABLED
void sinkBleGapAddDeviceWhiteListCfm(const CL_DM_BLE_ADD_DEVICE_TO_WHITE_LIST_CFM_T * cfm);
#else
#define sinkBleGapAddDeviceWhiteListCfm(cfm) ((void)(0))
#endif

/*******************************************************************************
NAME
    sinkBleGapIsBondable
    
DESCRIPTION
    Finds if BLE is in bondable mode.
    
PARAMETERS
    None
    
RETURNS
    TRUE if BLE is in bondable mode. FALSE otherwise.
*/
#ifdef GATT_ENABLED
bool sinkBleGapIsBondable(void);
#else
#define sinkBleGapIsBondable() (FALSE)
#endif

/*******************************************************************************
NAME
    sinkBleGapIsConnectable
    
DESCRIPTION
    Finds if BLE is in connectable mode.
    
PARAMETERS
    None
    
RETURNS
    TRUE if BLE is in connectable mode. FALSE otherwise.
*/
#ifdef GATT_ENABLED
bool sinkBleGapIsConnectable(void);
#else
#define sinkBleGapIsConnectable() (FALSE)
#endif

/*******************************************************************************
NAME
    sinkBleGapSetSecurityCfm
    
DESCRIPTION
    Set security confirm for BLE operations.
    
PARAMETERS
    None
    
RETURNS
    None
*/
#ifdef GATT_ENABLED
void sinkBleGapSetSecurityCfm(CL_DM_BLE_SECURITY_CFM_T * cfm);
#else
#define sinkBleGapSetSecurityCfm(cfm) ((void)(0))
#endif

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
#ifdef GATT_ENABLED
void sinkBleGapSetDiscoveryInProgress(bool in_progress, typed_bdaddr *tp_addr, uint16 cid, uint8 index);
#else
#define sinkBleGapSetDiscoveryInProgress(in_progress, tp_addr, cid, index) (void(0))
#endif

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
#ifdef GATT_ENABLED
uint16 sinkBleGapFindGapConnFlagIndex(typed_bdaddr *tp_addr);
#else
#define sinkBleGapFindGapConnFlagIndex(tp_addr) (GATT_INVALID_INDEX)
#endif

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
#ifdef GATT_ENABLED
uint16 sinkBleGapFindGapConnFlagIndexByCid(uint16 cid);
#else
#define sinkBleGapFindGapConnFlagIndexByCid(cid) (GATT_INVALID_INDEX)
#endif

/*******************************************************************************
NAME
    sinkBleGapSetCentralConnAttempt
    
DESCRIPTION
    Sets if master has triggered link connection.
    
PARAMETERS
    conn_attempt  TRUE if central triggered conn attempt. FALSE otherwise.
    tp_addr The address of the link for which the connection is attempted
    index slot available
    
RETURNS
    None
*/
#ifdef GATT_ENABLED
void sinkBleGapSetCentralConnAttempt(bool conn_attempt, const typed_bdaddr *tp_addr, uint16 cid, uint8 index);
#else
#define sinkBleGapSetCentralConnAttempt(conn_attempt, tp_addr, cid, index) (void(0))
#endif

void sink_custom_ble_scan_adv(void);
#endif /* _SINK_BLE_GAP_H_ */
