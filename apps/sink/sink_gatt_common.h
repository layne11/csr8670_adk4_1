/*******************************************************************************
 Copyright (c) 2015 Qualcomm Technologies International, Ltd.
 Part of ADK 4.1
*******************************************************************************/



#ifndef _SINK_GATT_COMMON_H_
#define _SINK_GATT_COMMON_H_

#include "sink_ble_gap.h"
#include <gatt_server.h>
#include <gatt_gap_server.h>
#include <gatt_battery_server.h>
#include <gatt_transmit_power_server.h>
#include <gatt_link_loss_server.h>
#include <gatt_imm_alert_server.h>

#include <csrtypes.h>

#define GATT_INVALID_INDEX 0xff

/***************************************************************************
NAME    
    gattCentralRoleMaxConnections
    
DESCRIPTION
    Utility function called to check if device can start scanning based on central role max connections
    
PARAMETERS
    None
    
NOTE
    Max central connections
*/
#ifdef GATT_ENABLED
uint16 gattCentralRoleMaxConnections(void);
#else
#define gattCentralRoleMaxConnections() (TRUE)
#endif


/***************************************************************************
NAME    
    gattCommonHasMaxConnections
    
DESCRIPTION
    Utility function called to check if maximum gatt connection is reached.
    
PARAMETERS
    None
    
NOTE
    TRUE if reached else FALSE.
*/
#ifdef GATT_ENABLED
bool gattCommonHasMaxConnections(void);
#else
#define gattCommonHasMaxConnections() (TRUE)
#endif
/***************************************************************************
NAME    
    gattCommonAddConnections
    
DESCRIPTION
    Function to add connection.
    
PARAMETERS
    cid
    
NOTE
    TRUE if reached else FALSE.
*/
#ifdef GATT_ENABLED
bool gattCommonAddConnections(uint16 cid, ble_gap_role_t gap_role);
#else
#define gattCommonAddConnections(cid, gap_role) (TRUE)
#endif

/***************************************************************************
NAME    
    gattCommonRemoveConnections
    
DESCRIPTION
    Function to remove connection.
    
PARAMETERS
    cid
    
NOTE
    TRUE if reached else FALSE.
*/
#ifdef GATT_ENABLED
bool gattCommonRemoveConnections(uint16 cid);
#else
#define gattCommonRemoveConnections(cid) (TRUE)
#endif

/****************************************************************************
NAME    
    gattServerConnectionFindByCid
    
DESCRIPTION
    Finds the GATT  connection by connection ID.
    
PARAMETERS
    cid             The connection ID
    
RETURNS    
    returns the index
*/
#ifdef GATT_ENABLED
uint16  gattCommonConnectionFindByCid(uint16 cid);
#else
#define gattCommonConnectionFindByCid(cid) (0xff)
#endif

/****************************************************************************
NAME    
    gattCommonFindCidBondingDevice
    
DESCRIPTION
    Finds the connection ID of the device with which bonding was expected. Also after the ID is 
    found, the flag is reset.
    
PARAMETERS
    none
    
RETURNS    
    returns the connection ID
*/
#ifdef GATT_ENABLED
uint16 gattCommonFindCidBondingDevice(void);
#else
#define gattCommonFindCidBondingDevice() (0xff)
#endif

/****************************************************************************
NAME    
    gattCommonSetDeviceBonding
    
DESCRIPTION
    Sets the bonding flag of the device as expected. This is set when an remote client tries to 
    connect when the app is in Bondable_Scanning_Advertising state.
    By doing so, if after the pairing timeout and the remote client has not yet tried to bond,
    then using this flag the app can know for which cid it needs to trigger bonding
    
PARAMETERS
    none
    
RETURNS    
    returns the connection ID
*/
#ifdef GATT_ENABLED
bool gattCommonSetDeviceBonding(uint16 cid, bool req_set);
#else
#define gattCommonSetDeviceBonding(cid, req_set) (FALSE)
#endif

/****************************************************************************
NAME    
    gattCommonConnectionRemove
    
DESCRIPTION
    Frees resource for a GATT  connection.
    
PARAMETERS
    cid             The connection ID
    
RETURNS    
    TRUE if the connection was found and removed. FALSE otherwise.    
*/
#ifdef GATT_ENABLED
bool gattCommonConnectionRemove(uint16 cid);
#else
#define gattCommonConnectionRemove(cid) (FALSE)
#endif /* GATT_ENABLED */


/****************************************************************************
NAME    
    gattCommonDisconnectAll
    
DESCRIPTION
    Disconnects all  connections.
    
PARAMETERS
    None
    
RETURNS    
    TRUE if a disconnect request was sent. FALSE otherwise.
*/
#ifdef GATT_ENABLED
bool gattCommonDisconnectAll(bool central);
#else
#define gattCommonDisconnectAll(central) (FALSE)
#endif


/****************************************************************************
NAME    
    gattCommonIsFullyDisconnected
    
DESCRIPTION
    Gets if there are no connections active.
    
PARAMETERS
    connection      The GATT connection
    
RETURNS
    TRUE if no connections. FALSE otherwise.

*/
#ifdef GATT_ENABLED
bool gattCommonIsFullyDisconnected(void);
#else
#define gattCommonIsFullyDisconnected() (TRUE)
#endif

/****************************************************************************
NAME    
    gattCommonGetNumberOfServerConn
    
DESCRIPTION
    Gets the number of  connections.
    
RETURNS
    TRUE if no connections. FALSE otherwise.

*/
#ifdef GATT_ENABLED
uint16 gattCommonGetNumberOfConn(void);
#else
#define gattCommonGetNumberOfConn() (0)
#endif

/****************************************************************************
NAME    
    gattCommonIsMaxConnReached
    
DESCRIPTION
    Checks if maximum connection for that particular role has reached
    
RETURNS
    TRUE if max connections. FALSE otherwise.

*/
#ifdef GATT_ENABLED
bool gattCommonIsMaxConnReached(ble_gap_role_t gap_role);
#else
#define gattCommonIsMaxConnReached(gap_role) (TRUE)
#endif


#endif /* _SINK_GATT_SERVER_H_ */


