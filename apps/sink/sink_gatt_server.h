/*******************************************************************************
Copyright (c) 2015 Qualcomm Technologies International, Ltd.
 Part of ADK 4.1
*******************************************************************************/



#ifndef _SINK_GATT_SERVER_H_
#define _SINK_GATT_SERVER_H_

#include <gatt_server.h>
#include <gatt_gap_server.h>
#include <gatt_battery_server.h>
#include <gatt_transmit_power_server.h>
#include <gatt_link_loss_server.h>
#include <gatt_imm_alert_server.h>
#include <gatt_heart_rate_server.h>
#include <gatt_device_info_server.h>

#include <csrtypes.h>

/* GATT Client Configuration attributes that need to be stored per remote client */
typedef struct __gatt_ccd_attributes_t
{
    uint16 gatt;
    uint16 battery_local;
    uint16 battery_remote;
    uint16 battery_peer;
    uint16 heart_rate;
} gatt_ccd_attributes_t;

#ifdef GATT_ENABLED


/* Define for Server variables */
/*NOTE - Both server instances point to the same database, so referring with instance 0 is fine */
#define GATT_SERVER theSink.rundata->ble.gatt[index].server

/* Number of peripherals configured */
#define GATT_SERVER_MAX_CONN (theSink.features.MaxPeripheralConnection)

typedef struct __gatt_server_read_name_t
{
    bool        requested;  /* Flag to indicate if client requested read of local name */
    uint16      offset;     /* Offset for local name repsonse */   
} gatt_server_gap_read_name_t;

/* Connection data maintained for each of the connected device irrespective of a 
   client/server connection.The information specific to a connected device is stored here.
*/
typedef enum __gatt_attribute_service_t
{
    gatt_attr_service_battery_local,
    gatt_attr_service_battery_remote,
    gatt_attr_service_battery_peer,
    gatt_attr_service_heart_rate,
    gatt_attr_service_gatt,
    gatt_attr_service_all
} gatt_attribute_service_t;

/*!
    @brief Enumeration for Alert level Value of Alert Level characteristic for Link Loss alert service 
*/

/* For Alert Level characteristic value, refer http://developer.bluetooth.org/
 * gatt/characteristics/Pages/CharacteristicViewer.aspx?u=org.bluetooth.
 * characteristic.alert_level.xml
 */
 
typedef enum {

    gatt_link_loss_alert_level_no          = 0,     /* No Alert request */
    gatt_link_loss_alert_level_mild        = 1,     /* Mild Alert request */
    gatt_link_loss_alert_level_high        = 2,     /* High Alert request */
    gatt_link_loss_alert_level_reserved

}gatt_link_loss_alert_level;

/* This enum defines the different server service being provided.
*/
typedef enum __gatt_server_service_t
{
    gatt_server_service_gatt = 0,
    gatt_server_service_gap,
    gatt_server_service_bas_local,
    gatt_server_service_bas_remote,
    gatt_server_service_bas_peer,
    gatt_server_service_lls,
    gatt_server_service_hrs,
    gatt_server_service_max
} gatt_server_service_t;

typedef struct __gatt_server_t
{
    gatt_ccd_attributes_t client_config; /* Client Service Configuration set by by the client */
    gatt_server_gap_read_name_t gap_read_name;  /* Records the clients read of the local name using GAP */
    void           *servers_ptr;       /* Pointer to the memory block used to store the RUNDATA required by the GATT server tasks */
    GGATTS     *gatt_server;       /* Pointer to the "GATT Server" (library) RUNDATA */
    GGAPS       *gap_server;        /* Pointer to the "GAP Server" (library) RUNDATA */
    GBASS       *bas_server_local;  /* Pointer to the "Battery Server" (library) RUNDATA - internal local battery level */    
    GBASS       *bas_server_remote; /* Pointer to the "Battery Server" (library) RUNDATA - remote battery level */
    GBASS       *bas_server_peer;   /* Pointer to the "Battery Server" (library) RUNDATA - peer battery level */
    GLLSS_T    *lls_server;        /* Pointer to the "Link Loss Server" (library) RUNDATA - link loss service */   
    GHRS_T      *hrs_server;        /* Pointer to the "Heart rate Sensor Server" (library) RUNDATA - Heart rate service */         
    gatt_link_loss_alert_level alert_level;            /*! Virtual LLS Alert level veiw */
} gatt_server_t;


/****************************************************************************
NAME    
    gattServerConnectionAdd
    
DESCRIPTION
    Allocates resource for a GATT server connection.
    
PARAMETERS
    cid             The connection ID
    
RETURNS    
    TRUE if the server connection was successfully added, FALSE otherwise.
*/
bool gattServerConnectionAdd(uint16 cid, const typed_bdaddr *client_taddr);


/****************************************************************************
NAME    
    gattServerConnectionFindByCid
    
DESCRIPTION
    Finds the GATT server connection by connection ID.
    
PARAMETERS
    cid             The connection ID
    
RETURNS    
    TRUE if the server connection is found, NULL otherwise.
*/
bool gattServerConnectionFindByCid(uint16 cid);


/****************************************************************************
NAME    
    gattServerConnectionRemove
    
DESCRIPTION
    Frees resource for a GATT server connection.
    
PARAMETERS
    cid             The connection ID
    
RETURNS    
    TRUE if the connection was found and removed. FALSE otherwise.    
*/
bool gattServerConnectionRemove(uint16 cid);


#endif /* GATT_ENABLED */


/****************************************************************************
NAME    
    gattServerDisconnectAll
    
DESCRIPTION
    Disconnects all server role connections.
    
PARAMETERS
    None
    
RETURNS    
    TRUE if a disconnect request was sent. FALSE otherwise.
*/
#ifdef GATT_ENABLED
bool gattServerDisconnectAll(void);
#else
#define gattServerDisconnectAll() (FALSE)
#endif

/****************************************************************************
NAME    
    gattServerDisconnect
    
DESCRIPTION
    Disconnects server role connections based on CID.
    
PARAMETERS
    cid connection identifier of the server link
    
RETURNS    
    None
*/
#ifdef GATT_ENABLED
void gattServerDisconnect(uint16 cid);
#else
#define gattServerDisconnect(cid) (void(0))
#endif


/****************************************************************************
NAME    
    gattServerIsFullyDisconnected
    
DESCRIPTION
    Gets if there are no connections active.
    
PARAMETERS
    connection      The GATT connection
    
RETURNS
    TRUE if no connections. FALSE otherwise.

*/
#ifdef GATT_ENABLED
bool gattServerIsFullyDisconnected(void);
#else
#define gattServerIsFullyDisconnected() (TRUE)
#endif

/****************************************************************************
NAME    
    gattServerStoreConfigAttributes
    
DESCRIPTION
    Stores Client Characteristic Configuration attributes for bonded devices.
    
PARAMETERS
    cid The connection ID.
    client_service The service to store attributes for.

*/
#ifdef GATT_ENABLED
void gattServerStoreConfigAttributes(uint16 cid, gatt_attribute_service_t ccd_service);
#else
#define gattServerStoreConfigAttributes(cid, ccd_service) ((void)(0))
#endif

/****************************************************************************
NAME    
    gattServerSetServicePtr
    
DESCRIPTION
    Stores the Service pointers in the respective server structure.
    
PARAMETERS
    ptr pointer to service.
    service The service to store pointer for.

*/
#ifdef GATT_ENABLED
void gattServerSetServicePtr(uint16 **ptr, gatt_server_service_t service);
#else
#define gattServerSetServicePtr(ptr, service) ((void)(0))
#endif

#endif /* _SINK_GATT_SERVER_H_ */

