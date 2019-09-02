/****************************************************************************
Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.1

FILE NAME
    sink_ble_advertising.h

DESCRIPTION
    BLE Advertising functionality
*/

#ifndef _SINK_BLE_ADVERTISING_H_
#define _SINK_BLE_ADVERTISING_H_


#include <connection.h>

#include <csrtypes.h>


#define ADVERTISING theSink.rundata->ble.gap.advertising


#define MAX_AD_DATA_SIZE_IN_OCTETS  (0x1F)   /* AD Data max size = 31 octets (defined by BT spec) */
#define AD_DATA_HEADER_SIZE         (0x02)   /* AD header{Octet[0]=length, Octet[1]=Tag} AD data{Octets[2]..[n]} */
#define OCTETS_PER_SERVICE          (0x02)   /* 2 octets per uint16 service UUID */
#define MIN_LOCAL_NAME_LENGTH       (0x10)   /* Minimum length of the local name being advertised*/

/* Discoverable mode */
typedef enum __adv_discoverable_mode
{
    adv_non_discoverable_mode,
    adv_discoverable_mode_general,
    adv_discoverable_mode_limited
} adv_discoverable_mode_t;


/*******************************************************************************
NAME    
    bleSetupAdvertisingData
    
DESCRIPTION
    Function to setup the BLE Advertising data for the device.
    
PARAMETERS
    size_local_name Length of the local name buffer.
    local_name      Buffer containing the local name.
    
RETURN
    None
*/
#ifdef GATT_ENABLED
void bleSetupAdvertisingData(uint16 size_local_name, const uint8 *local_name, adv_discoverable_mode_t mode);
#else
#define bleSetupAdvertisingData(size_local_name, local_name, mode) (void(0))
#endif


/*******************************************************************************
NAME    
    bleHandleSetAdvertisingData
    
DESCRIPTION
    Function to handle when BLE advertising data has been registered with CL.
    
PARAMETERS
    cfm     pointer to a CL_DM_BLE_SET_ADVERTISING_DATA_CFM message.
    
RETURN
    None
*/
#ifdef GATT_ENABLED
void bleHandleSetAdvertisingData(const CL_DM_BLE_SET_ADVERTISING_DATA_CFM_T * cfm);
#else
#define bleHandleSetAdvertisingData(cfm) ((void)(0))
#endif


/*******************************************************************************
NAME    
    bleStopAdvertising

DESCRIPTION
    Function to stop advertising the registered BLE AD Data
    
PARAMETERS
    None
    
RETURNS
    None
*/
#ifdef GATT_ENABLED
void bleStopAdvertising(void);
#else
#define bleStopAdvertising()
#endif


#endif /* _SINK_BLE_ADVERTISING_H_ */
