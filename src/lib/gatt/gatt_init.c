/****************************************************************************
Copyright (c) 2011 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.1

FILE NAME
    gatt_init.c        

DESCRIPTION
    GATT library initialisation.

NOTES

*/

/***************************************************************************
    Header Files
*/
#include "gatt.h"
#include "gatt_private.h"

#include <vm.h>
#include <string.h>
#include <util.h>
#include <stdlib.h>
#include <bdaddr.h>

/***************************************************************************
    Local and static 
*/
static gattState    *theGatt;

#define CID_MAP     theGatt->u.cid_map

/*************************************************************************** 
    Sanity check that GATT_CID_LOCAL as defined in gatt.h matches the 
    corresponding value in the firmware API 
*/
#if GATT_CID_LOCAL != ATT_CID_LOCAL
#error "ATT_CID_LOCAL has changed. Update GATT_CID_LOCAL."
#endif

/****************************************************************************
NAME
    GattInitEx

DESCRIPTION
    Initialise the GATT library - initialises the ATT protocol.

RETURNS

*/

void GattInitEx(Task theAppTask, uint16 size_database, uint16* database, uint16 flags)
{
    if (theGatt)
    {
        GATT_DEBUG(("ERROR: Gatt Library already initialised.\n"));
    }
    else
    {
        theGatt = PanicUnlessNew(gattState);
        memset(theGatt, 0, sizeof(gattState));
        
        if (MessageAttTask(&theGatt->task))
            GATT_DEBUG(("ERROR: ATT Task already registered\n"));
    }
        

    theGatt->theAppTask         = theAppTask;
    theGatt->task.handler       = gattMessageHandler;
    theGatt->state              = gatt_state_initialising;
    theGatt->flags              = flags;

    theGatt->u.database.ptr     = database;
    theGatt->u.database.size    = size_database;

    /* Register with ATT protocol to start intialisation. */
    {
        MAKE_ATT_PRIM(ATT_REGISTER_REQ);
        VmSendAttPrim(prim);
    }
}

/****************************************************************************
NAME
    GattInit

DESCRIPTION
    Initialise the GATT library - initialises the ATT protocol.

RETURNS

*/

void GattInit(Task theAppTask, uint16 size_database, uint16* database)
{
    GattInitEx(theAppTask, size_database, database, GATT_FLAGS_DEFAULT);
}


/****************************************************************************
NAME
    gattGetTask

DESCRIPTION
    Returns the GATT library task so that the GATT library can post messages 
    to itself.

RETURNS

*/
Task gattGetTask(void)
{
    return &theGatt->task;
}


/****************************************************************************
NAME
    gattGetAppTask

DESCRIPTION
    Returns the Task that registered the the GATT library.

RETURNS

*/
Task gattGetAppTask(void)
{
    return theGatt->theAppTask;
}

/****************************************************************************
NAME
    gattInitCidTaskMap

DESCRIPTION
    Initialise the array that maps connection Cid to Tasks.

RETURNS

*/
void gattInitCidTaskMap(void)
{
    uint16 i;

    /* First clear the array map. */
    memset(theGatt->u.cid_map, 0, sizeof(cid_map_t) * MAX_ATT_CONNECTIONS);

    /* First CID is 0, this is the local ATT CID and used for scenario
     * locking for local ATT DB operations. All other CID must be set
     * to FF (invalid CID).
     */
    for (i=1; i<MAX_ATT_CONNECTIONS; i++)
    {
        theGatt->u.cid_map[i].cid = INVALID_CID;
    }
}


/****************************************************************************
NAME
    gattMaxConnections

DESCRIPTION
    If the number of MAX_ATT_CONNECTIONS has been meet then returns TRUE,
    otherwise false.

RETURNS

*/
bool gattMaxConnections(void)
{
    return (MAX_ATT_CONNECTIONS > theGatt->connect_count) ? FALSE : TRUE;
}
 
/*************************************************************************
NAME    
    gattFindConn
    
DESCRIPTION
    Find GATT connection entry in task_cid_map by the cid.

RETURNS
    Ptr of cid_map_t structure in table, if found, or 0, if not.
    
*/
cid_map_t *gattFindConn(uint16 cid)
{
    return (cid_map_t*)UtilFind(0xffff,                 /* mask */
                                cid,                    /* value */
                                (const uint16*)CID_MAP, /* data_start */
                                0,                      /* offset */
                                sizeof(cid_map_t),      /* size */
                                MAX_ATT_CONNECTIONS);   /* count */
}

/*************************************************************************
NAME    
    gattFindConnOrPanic
    
DESCRIPTION
    Find GATT connection entry in task_cid_map by the cid.

RETURNS
    Ptr of cid_map_t structure in table, if found, or Panics if not.
    
*/
cid_map_t *gattFindConnOrPanic(uint16 cid)
{
    cid_map_t *found = (cid_map_t*)PanicNull(gattFindConn(cid));

    /* Never reach here if we panic */
    return found;

}

#if (GATT_FEATURES & GATT_PRIMARY_DISCOVERY)
/*************************************************************************
NAME    
    gattFindConnAddr
    
DESCRIPTION
    Find GATT connection entry in task_cid_map by the address. This only done 
    for SDP searches and so the transport will always be BREDR and the address
    type PUBLIC.

RETURNS
    
*/
cid_map_t *gattFindConnAddr(const bdaddr *addr)
{
    cid_map_t *found = NULL;
    cid_map_t *conn;
    tp_bdaddr tpaddr;
    uint16 i;

    /* go through active connections */
    for (i = 0; i < MAX_ATT_CONNECTIONS; i++)
    {
        conn = &theGatt->u.cid_map[i];
        
        /* fake instance for SDP search */
        if ((conn->data.scenario == gatt_ms_discover_all_bredr_services ||
             conn->data.scenario == gatt_ms_discover_bredr_service) &&
            BdaddrIsSame(
                addr,
                &conn->data.req.discover_all_bredr_services.addr))
        {
            return conn;
        }
        
        /* get address from cid */
        else if (VmGetBdAddrtFromCid(conn->cid, &tpaddr) &&
                 tpaddr.transport == TRANSPORT_BREDR_ACL &&
                 tpaddr.taddr.type == TBDADDR_PUBLIC &&
                 BdaddrIsSame(addr, &tpaddr.taddr.addr))
        {
            found = conn;
        }
    }

    return found;
}
#endif /* GATT_PRIMARY_DISCOVERY */

/****************************************************************************
NAME
    gattCidIsValid

DESCRIPTION
    Check the CID is in the list of GATT connections.

RETURNS
    TRUE if found, otherwise FALSE.
*/
bool gattCidIsValid(uint16 cid)
{
    return gattFindConn(cid) ? TRUE : FALSE;
}

/****************************************************************************
NAME
    gattAddCidTask

DESCRIPTION
    Add a CID and Task map for an established connection. The cid is used to map data
    associated with the connection and lock message scenarios.

RETURNS

*/
cid_map_t *gattAddCid(uint16 cid, Task task)
{
    if (MAX_ATT_CONNECTIONS == theGatt->connect_count)
    {
        GATT_DEBUG(("Maximum number of GATT connections reached\n"));
        
    }
    else
    {
        /* Find the first empty table entry (Invalid CID) */
        cid_map_t *ptr = gattFindConn(INVALID_CID);
        
        ptr->cid =  cid;
        ptr->task = task;

        theGatt->connect_count++;

        return ptr;
    }

    return NULL;
}

/****************************************************************************
NAME
    gattDeleteCid

DESCRIPTION
    Delete a cid map. To be used when the connection is closed and the
    pending command queue has been cleared.  

RETURNS

*/
void gattDeleteCid(uint16 cid)
{
    cid_map_t *ptr = gattFindConn(cid);

    if (cid && ptr)
    {
        /* clear associated connection data */
        free(ptr->data.stash);
        
        /* set the whole entry to 0 */
        memset( ptr, 0, sizeof(cid_map_t) );

        /* set the CID to invalid */
        ptr->cid = INVALID_CID;

        /* decrement number of connections */
        theGatt->connect_count--;        
    }
}

/****************************************************************************
NAME
    gattGetCidData

DESCRIPTION
    Get the connection data associated with a cid. May return NULL so 
    caller must check that returned pointer is valid

RETURNS
    conn_data_t pointer.
*/
conn_data_t *gattGetCidData(uint16 cid)
{
    cid_map_t *cid_map = (cid_map_t *)gattFindConn(cid);
    
    if(cid_map)
        return &cid_map->data;
    
    return NULL;
}

/****************************************************************************
NAME
    gattLockCid

DESCRIPTION
    Lock on CID for an active message scenario. New scenarios can't start 
    until the current one is complete and the CID unlocked.

RETURNS

*/
bool gattLockCid(uint16 cid,  gatt_msg_scen_t scenario)
{
    cid_map_t *ptr = gattFindConnOrPanic(cid);

    if (ptr->data.scenario)
    {
        return FALSE;   /* CID is already mapped to a scenario */
    }
    else
    {
        ptr->data.scenario = scenario;
    }
    return TRUE;
}

/****************************************************************************
NAME
    gattUnlockCid

DESCRIPTION
    Clear data associated with a message scenario locking on a CID. 

RETURNS

*/
void gattUnlockCid(uint16 cid)
{
    cid_map_t *ptr = gattFindConnOrPanic(cid);

#if GATT_DEBUG_LIB
    if (!ptr->data.scenario)
    {
        printf("gattUnlockCid: No scenario associated with CID\n");
        Panic();
    }
#endif

    /* clear associated connection data. */
    free(ptr->data.stash);
    memset( &ptr->data, 0, sizeof(conn_data_t) );
}

/****************************************************************************
NAME
    gattGetCidMappedTask

DESCRIPTION
    Returns the the task associated with the cid, the task that created the
    connection.

RETURNS
    Task ptr
*/
Task gattGetCidMappedTask(uint16 cid)
{
    cid_map_t *ptr = gattFindConnOrPanic(cid);
    return ptr->task;
}


/****************************************************************************
NAME
    gattSendGattInitCfm

DESCRIPTION
    Sends the GATT_INIT_CFM message to theAppTask.

RETURNS

*/
static void gattSendInitCfm(Task task, gatt_status_t status)
{
    MAKE_GATT_MESSAGE(GATT_INIT_CFM);
    message->status = status;
    MessageSend(task, GATT_INIT_CFM, message);
}


/****************************************************************************
NAME
    gattHandleAttRegisterCfm

DESCRIPTION
    Handles the ATT_INIT_CFM from BlueStack. If there is an ATT DB to set up,
    send ATT_ADD_DB_REQ, otherwise indicate GATT is intialised.

RETURNS

*/

void gattHandleAttRegisterCfm(gattState *theGatt, const ATT_REGISTER_CFM_T  *cfm)
{
    if (cfm->result)
    {
        GATT_DEBUG_INFO(("GATT_INIT_CFM status %0x02X\n", cfm->result));
        theGatt->state = gatt_state_uninitialised;
        gattSendInitCfm(theGatt->theAppTask, gatt_status_att_reg_failure);

        if (!(theGatt->flags & GATT_FLAGS_CONST_DB))
        {
            if (theGatt->u.database.size)
            {
                free(theGatt->u.database.ptr);
            }
        }
    }
    else
    {
        if (theGatt->u.database.size)
        {   
            MAKE_ATT_PRIM(ATT_ADD_DB_REQ);
            prim->size_db = theGatt->u.database.size;
            prim->db      = (uint16_t*)VmGetHandleFromPointer(theGatt->u.database.ptr);
            prim->flags   = 0;      /* Not applicable for only DB. */
            VmSendAttPrim(prim);
        }
        else
        {
            theGatt->state = gatt_state_initialised;
            gattSendInitCfm(theGatt->theAppTask, gatt_status_success);
            gattInitCidTaskMap();
        }
    }        
}


/****************************************************************************
NAME
    gattHandleAttAddDbCfm

DESCRIPTION
    Handles the ATT_INIT_CFM from BlueStack. If there is an ATT DB to set up,
    send ATT_ADD_DB_REQ, otherwise indicate GATT is intialised.

RETURNS

*/
void gattHandleAttAddDbCfm(gattState *theGatt, const ATT_ADD_DB_CFM_T *cfm)
{
    if (cfm->result)
    {
        GATT_DEBUG_INFO(("gattHandleAttAddDbCfm: Result 0x%x\n", cfm->result));

        theGatt->state = gatt_state_uninitialised;
        gattSendInitCfm(theGatt->theAppTask, gatt_status_att_db_failure);
    }
    else 
    {
        theGatt->state = gatt_state_initialised;
        gattSendInitCfm(theGatt->theAppTask, gatt_status_success);
        gattInitCidTaskMap();
    }    
}


/****************************************************************************
NAME
    GattGetCidForBdaddr

DESCRIPTION
    Returns the CID associated for the address passed as a parameter or
    0 if no matching address is found.

RETURNS
    uint16 containing the cid.

*/
uint16 GattGetCidForBdaddr(const typed_bdaddr *taddr)
{
    uint16 i;
    tp_bdaddr    cid_tpaddr;

    for (i=0; i<MAX_ATT_CONNECTIONS; i++)
    {
        /* ignore CID = 0 (own database) or INVALID_CID */
        if (
            theGatt->u.cid_map[i].cid  &&
            theGatt->u.cid_map[i].cid != INVALID_CID
            )
        {
            const cid_map_t *conn = &theGatt->u.cid_map[i];
            
            if (
                VmGetBdAddrtFromCid(conn->cid, &cid_tpaddr)  &&
                BdaddrTypedIsSame(taddr, &cid_tpaddr.taddr)
               )
            {
               return conn->cid;
            }
        }
    }
    return 0;
}

