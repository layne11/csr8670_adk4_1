/****************************************************************************
Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.1

FILE NAME
    upgrade.c
    
DESCRIPTION
    Upgrade library API implementation.
*/

#include <string.h>
#include <stdlib.h>

#include <boot.h>
#include <message.h>
#include <byte_utils.h>
#include <print.h>
#include <panic.h>

#include "upgrade_ctx.h"
#include "upgrade_private.h"
#include "upgrade_sm.h"
#include "upgrade_host_if.h"
#include "upgrade_psstore.h"
#include "upgrade_partitions.h"

#include "upgrade_msg_vm.h"
#include "upgrade_msg_internal.h"
#include "upgrade_host_if_data.h"

static void SMHandler(Task task, MessageId id, Message message);
static void SendUpgradeInitCfm(Task task, upgrade_status_t status);
static void RequestApplicationReconnectIfNeeded(void);

/****************************************************************************
NAME
    UpgradeInit

DESCRIPTION
    Perform initialisation for the upgrade library. This consists of fixed
    initialisation as well as taking account of the information provided
    by the application.
*/

void UpgradeInit(Task appTask,uint16 dataPskey,uint16 dataPskeyStart,
    const UPGRADE_UPGRADABLE_PARTITION_T * logicalPartitions,
    uint16 numPartitions,
    upgrade_power_management_t power_mode,
    const char * dev_variant,
    upgrade_permission_t init_perm,
    const upgrade_version *init_version,
    uint16 init_config_version)
{
    UpgradeCtx *upgradeCtx;
    uint16 available_space;

    PRINT(("UPGRADE: Init\n"));

    upgradeCtx = PanicUnlessMalloc(sizeof(*upgradeCtx));
    memset(upgradeCtx, 0, sizeof(*upgradeCtx));
    upgradeCtx->mainTask = appTask;
    upgradeCtx->smTaskData.handler = SMHandler;

    UpgradeCtxSet(upgradeCtx);

    /* handle permission initialisation, must be an "enabled" state */
    if (   (init_perm != upgrade_perm_assume_yes)
        && (init_perm != upgrade_perm_always_ask))
    {
        Panic();
    }
    UpgradeCtxGet()->perms = init_perm;

    /* set the initial power management mode */
    UpgradeCtxGet()->power_mode = power_mode;

    /* set the initial state to battery ok, expecting sink powermanagement to soon update the state */
    UpgradeCtxGet()->power_state = upgrade_battery_ok;

    /* store the device variant */
    if(dev_variant != NULL)
    {
        strncpy(UpgradeCtxGet()->dev_variant, dev_variant, UPGRADE_HOST_VARIANT_CFM_BYTE_SIZE );
    }

    available_space = PSKEY_MAX_STORAGE_LENGTH - dataPskeyStart;
    if (   (dataPskeyStart >= PSKEY_MAX_STORAGE_LENGTH)
        || (available_space < UPGRADE_PRIVATE_PSKEY_USAGE_LENGTH_WORDS)
        || !UpgradePartitionsSetMappingTable(logicalPartitions,numPartitions))
    {
        SendUpgradeInitCfm(appTask, upgrade_status_unexpected_error);
        free(upgradeCtx);
        return;
    }

    UpgradeLoadPSStore(dataPskey,dataPskeyStart);

    /* @todo Need to deal with two things here
     * Being called when the PSKEY has already been set-up
     * being called for the first time. should we/can we verify partition
     * mapping 
     */

    /* Initial version setting */
    if (UpgradeCtxGetPSKeys()->version.major == 0
        && UpgradeCtxGetPSKeys()->version.minor == 0)
    {
        UpgradeCtxGetPSKeys()->version = *init_version;
    }

    if (UpgradeCtxGetPSKeys()->config_version == 0)
    {
        UpgradeCtxGetPSKeys()->config_version = init_config_version;
    }

    /* Make this call before initialising the state machine so that the
       SM cannot cause the initial state to change */
    RequestApplicationReconnectIfNeeded();

    /* initialise the state machine and pass in the event that enables upgrades
     * @todo this UPGRADE_VM_PERMIT_UPGRADE event can be removed if we're always
     * starting in an upgrade enabled state, just need to initialise the state
     * machine in the correct state. */
    UpgradeSMInit();
    UpgradeSMHandleMsg(UPGRADE_VM_PERMIT_UPGRADE, 0);
    UpgradeHostIFClientConnect(&upgradeCtx->smTaskData);

    SendUpgradeInitCfm(appTask, upgrade_status_success);
}

/****************************************************************************
NAME
    UpgradePowerManagementSetState

DESCRIPTION
    Receives the current state of the power management from the Sink App

RETURNS

*/
upgrade_status_t UpgradePowerManagementSetState(upgrade_power_state_t state)
{
    /* if initially the power management was set to disabled, don't accept any change */
    /* we need to make sure this is called AFTER UpgradeInit is called */
    if(UpgradeCtxGet()->power_mode == upgrade_power_management_disabled)
    {
        return upgrade_status_invalid_power_state;
    }
    
    UpgradeCtxGet()->power_state = state;

    if(UpgradeCtxGet()->power_state == upgrade_battery_low)
    {
        MessageSend(UpgradeGetUpgradeTask(), UPGRADE_INTERNAL_BATTERY_LOW, NULL);
    }

    return upgrade_status_success;
}

/****************************************************************************
NAME
    UpgradeGetPartitionInUse 

DESCRIPTION
    Find out current physical partition for a logical partition.

RETURNS
    uint16 representing the partition that is active. 
    UPGRADE_PARTITION_NONE_MAPPED is returned for an invalid partition.
*/
uint16 UpgradeGetPartitionInUse(uint16 logicalPartition)
{

    return (uint16)UpgradePartitionsPhysicalPartition(logicalPartition,UpgradePartitionActive);
}

/****************************************************************************
NAME
    UpgradeGetAppTask

DESCRIPTION
    Returns the VM application task registered with the library at
    initialisation in #UpgradeInit

RETURNS
    Task VM application task
*/
Task UpgradeGetAppTask(void)
{
    return UpgradeCtxGet()->mainTask;
}

/****************************************************************************
NAME
    UpgradeGetUpgradeTask

DESCRIPTION
    Returns the upgrade library main task.

RETURNS
    Task Upgrade library task.
*/
Task UpgradeGetUpgradeTask(void)
{
    return &UpgradeCtxGet()->smTaskData;
}

/****************************************************************************
NAME
    UpgradeHandleMsg

DESCRIPTION
    Main message handler for messages to the upgrade library from VM
    applications.
*/
void UpgradeHandleMsg(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UpgradeSMHandleMsg(id, message);
}

/****************************************************************************
NAME
    UpgradePermit

DESCRIPTION
    Control the permission the upgrade has for upgrade operations.

RETURNS
    upgrade_status_t Success or failure of requested permission type.
*/
upgrade_status_t UpgradePermit(upgrade_permission_t perm)
{
    switch (perm)
    {
        case upgrade_perm_no:
            /* if we already have an upgrade in progress, return an
             * error and do not modify our permissions */
            if (UpgradeSMUpgradeInProgress())
            {
                return upgrade_status_in_progress;
            }
            break;

        case upgrade_perm_assume_yes:
            /* fall-thru - both cases are permitting an upgrade */
        case upgrade_perm_always_ask:
            UpgradeSMHandleMsg(UPGRADE_VM_PERMIT_UPGRADE, 0);
            break;

        default:
            return upgrade_status_unexpected_error;
    }

    /* remember the permission setting */
    UpgradeCtxGet()->perms = perm;

    return upgrade_status_success;
}

/****************************************************************************
NAME
    UpgradeTransportConnectRequest

DESCRIPTION
    When a client wants to initiate an upgrade, the transport
    must first connect to the upgrade library so that it knows
    which Task to use to send messages to a client.

    The Upgrade library will respond by sending
    UPGRADE_TRANSPORT_CONNECT_CFM to transportTask.

*/
void UpgradeTransportConnectRequest(Task transportTask)
{
    PRINT(("UPGRADE: UpgradeTransportConnect 0x%p\n", (void *)transportTask));
    UpgradeHostIFTransportConnect(transportTask);
}

/****************************************************************************
NAME
    UpgradeProcessDataRequest

DESCRIPTION
    All data packets from a client should be sent to the Upgrade library
    via this function. Data packets must be in order but do not need
    to contain a whole upgrade message.

    The Upgrade library will respond by sending
    UPGRADE_TRANSPORT_DATA_CFM to the Task set in
    UpgradeTransportConnectRequest().

*/
void UpgradeProcessDataRequest(uint16 size_data, uint8 *data)
{
    PRINT(("UPGRADE: UpgradeProcessDataRequest\n"));
    UpgradeHostIFProcessDataRequest(data, size_data);
}

/****************************************************************************
NAME
    UpgradeTransportDisconnectRequest

DESCRIPTION
    When a transport no longer needs to use the Upgrade
    library it must disconnect.

    The Upgrade library will respond by sending
    UPGRADE_TRANSPORT_DISCONNECT_CFM to the Task set in
    UpgradeTransportConnectRequest().

*/
void UpgradeTransportDisconnectRequest(void)
{
    PRINT(("UPGRADE: UpgradeTransportDisconnect\n"));
    UpgradeHostIFTransportDisconnect();
}

void SMHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UpgradeSMHandleMsg(id, message);
}

/****************************************************************************
NAME
    UpgradeDfuStatus

DESCRIPTION
    Inform the Upgrade library of the result of an attempt to upgrade
    internal flash using DFU file from a serial flash partition.

RETURNS
    n/a
*/
void UpgradeDfuStatus(MessageDFUFromSQifStatus *message)
{
    /* TODO: Make sure that this structure already exists */
    switch(message->status)
    {
    case DFU_SQIF_STATUS_SUCCESS:
        {
            UpgradeCtxGetPSKeys()->loader_msg = UPGRADE_LOADER_MSG_SUCCESS;
            
            /* If there are one or more data partitions to update,
               we need to (re)calculate new FSTAB and warm reboot.
               (before reconnecting with the host.) */
            if (UpgradeSetToTryUpgrades())
            {
                UpgradeSavePSKeys();
                BootSetMode(BootGetMode());
            }
        }
        break;

    case DFU_SQIF_STATUS_ERROR:
        UpgradeCtxGetPSKeys()->loader_msg = UPGRADE_LOADER_MSG_ERROR;
        break;
    }

    UpgradeSavePSKeys();
}

/****************************************************************************
NAME
    UpgradeApplyResponse

DESCRIPTION
    Handle application decision on applying (reboot) an upgrade.

    If the application wishes to postpone the reboot, resend the message to
    the application after the requested delay. Otherwise, push a reboot
    event into the state machine.

    @todo do we want protection against these being called by a bad application
     at the wrong time? The state machine *should* cover this.

RETURNS
    n/a
*/
void UpgradeApplyResponse(uint32 postpone)
{
    if (!postpone)
    {
        MessageSend(UpgradeGetUpgradeTask(), UPGRADE_INTERNAL_REBOOT, NULL);
    }
    else
    {
        MessageSendLater(UpgradeCtxGet()->mainTask, UPGRADE_APPLY_IND, 0, postpone);
    }
}

/****************************************************************************
NAME
    UpgradeBlockingResponse

DESCRIPTION
    Handle application decision on blocking the system (erase). 

    If the application wishes to postpone the blocking erase, resend the
    message to the application after the requested delay. Otherwise, push an
    erase event into the state machine.

    @todo do we want protection against these being called by a bad application
     at the wrong time? The state machine *should* cover this.

RETURNS
    n/a
*/
void UpgradeBlockingResponse(uint32 postpone)
{
    if (!postpone)
    {
        MessageSend(UpgradeGetUpgradeTask(), UPGRADE_INTERNAL_ERASE, NULL);
    }
    else
    {
        MessageSendLater(UpgradeCtxGet()->mainTask, UPGRADE_BLOCKING_IND, 0, postpone);
    }
}

/****************************************************************************
NAME
    SendUpgradeInitCfm

DESCRIPTION
    Build and send an UPGRADE_INIT_CFM message and send to the specified task.

RETURNS
    n/a
*/
static void SendUpgradeInitCfm(Task task, upgrade_status_t status)
{
    MESSAGE_MAKE(upgradeInitCfm, UPGRADE_INIT_CFM_T);
    upgradeInitCfm->status = status;
    MessageSend(task, UPGRADE_INIT_CFM, upgradeInitCfm);
}

/****************************************************************************
NAME
    RequestApplicationReconnectIfNeeded

DESCRIPTION
    Check the upgrade status and decide if the application needs to consider 
    restarting communication / UI so that it can connect to a host.

    If needed, builds and send an UPGRADE_RESTARTED_IND_T message and sends to 
    the application task.

NOTE 
    Considered implementing this as part of UpgradeSMInit() which also looks at
    the resume point information, but it is not really related to the SM.
*/
static void RequestApplicationReconnectIfNeeded(void)
{
    upgrade_reconnect_recommendation_t reconnect = upgrade_reconnect_not_required;

    switch(UpgradeCtxGetPSKeys()->upgrade_in_progress_key)
    {
        /* Resume from the beginning, includes download phase. */
    case UPGRADE_RESUME_POINT_START:
    case UPGRADE_RESUME_POINT_ERROR:
        if (UpgradeCtxGetPSKeys()->id_in_progress)
        {
            /* Not in a critical operation, but there is an upgrade in progress,
               (in the case of _ERROR) presumably was an upgrade in progress.
               So the application may want to restart operations to allow it to
               resume */
            reconnect = upgrade_reconnect_recommended_in_progress;
        }
        break;

    case UPGRADE_RESUME_POINT_ERASE:
            /* Not in a critical operation, but there is an upgrade in progress.
               Separated from the two cases above as there is a lesser argument
               for the reconnect - so may change in future. */
        reconnect = upgrade_reconnect_recommended_in_progress;
        break;

    case UPGRADE_RESUME_POINT_PRE_VALIDATE:
    case UPGRADE_RESUME_POINT_PRE_REBOOT:
            /* We don't strictly need a reconnect at this point but 
               some interaction with the host is required to complete 
               the upgrade */
        reconnect = upgrade_reconnect_required_for_confirm;
        break;
        
    case UPGRADE_RESUME_POINT_POST_REBOOT:
    case UPGRADE_RESUME_POINT_COMMIT:
        if (UpgradeCtxGetPSKeys()->dfu_partition_num == 0)
        {
            /* We are in the middle of an upgrade that requires the host/app to
               confirm its success. */
            reconnect = upgrade_reconnect_required_for_confirm;
        }
        else
        {
            /* There is a DFU to be finished off. No host interaction is
               needed but won't hurt. */
            reconnect = upgrade_reconnect_recommended_as_completed;
        }
        break;
    }

    if (reconnect != upgrade_reconnect_not_required)
    {
        UPGRADE_RESTARTED_IND_T *restarted = (UPGRADE_RESTARTED_IND_T*)
                                                PanicUnlessMalloc(sizeof(*restarted));
        restarted->reason = reconnect;
        MessageSend(UpgradeCtxGet()->mainTask, UPGRADE_RESTARTED_IND, restarted);
    }

}
