/****************************************************************************
Copyright (c) 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.1

FILE NAME
    sink_upgrade.c
    
DESCRIPTION
    Interface to the upgrade library.

*/

#include <message.h>
#include <gaia.h>
#include <print.h>
#include <upgrade.h>

#include "sink_private.h"
#include "sink_configmanager.h"
#include "sink_volume.h"
#include "audio.h"
#include "sink_upgrade.h"

#ifdef ENABLE_UPGRADE


#ifdef DEBUG_UPGRADE
#define UPGRADE_INFO(x) DEBUG(x)
#define UPGRADE_ERROR(x) DEBUG(x) TOLERATED_ERROR(x)
#else
#define UPGRADE_INFO(x)
#define UPGRADE_ERROR(x)
#endif


/* The files sent to the upgrade library work using logical partitions
 * rather than physical ones.
 * In most cases a partition partitions are paired in such a way that
 * upgrades occur into a partition that is not being used and,
 * following a successful upgrade, the old partition can then be erased.
 *
 * The library is passed a table that gives the logical partition
 * structure.
 *
 * The example passed here, matches this partition table.
 *
 * upgrade_demo_ADK.ptn
    0, 512k, RS, (erase)     # Partition for DFU
    1, 32K, RO, VP_two.xuv  # Audio Prompt partition #1,1
    2, 32K, RO, (erase)      # Audio Prompt partition #1,2
    3, 16K, RO, (erase)      # Audio Prompt partition #2,1
    4, 16K, RO, (erase)      # Audio Prompt partition #2,2
    5, 64K, RS, (erase)     # Test partition #1
    6, 64K, RS, (erase)     # Test partition #2
    7, 8K, RS, (erase)      # Single-banked test partition
*/

static const UPGRADE_UPGRADABLE_PARTITION_T logicalPartitions[]
                    = {UPGRADE_PARTITION_SINGLE(0x1000,DFU),
                       UPGRADE_PARTITION_DOUBLE(0x1001,0x1002,MOUNTED),
                       UPGRADE_PARTITION_DOUBLE(0x1003,0x1004,MOUNTED),
                       UPGRADE_PARTITION_DOUBLE(0x1005,0x1006,UNMOUNTED),
                       UPGRADE_PARTITION_SINGLE(0x1007,KEEP_ERASED)
                      };

/* The factory-set upgrade version and PS config version.

   After a successful upgrade the values from the upgrade header
   will be written to the upgrade PS key and used in future.
*/
static const upgrade_version init_version = { 1, 0 };
static const uint16 init_config_version = 1;

#define NOW 0

#define UPGRADE_RESTARTED_DELAY D_SEC(1)


/*************************************************************************
NAME
    sinkUpgradeInit

DESCRIPTION
    Initialise the Upgrade library
*/
void sinkUpgradeInit(Task task)
{
    /* Allow storage of info at end of PSKEY 49 (CONFIG_SOFTWARE_VERSION_ID) */
    UpgradeInit(task,CONFIG_SOFTWARE_VERSION_ID,3,
                    logicalPartitions,
                    sizeof(logicalPartitions)/sizeof(logicalPartitions[0]),
                    UPGRADE_INIT_POWER_MANAGEMENT,
                    NULL,
                    upgrade_perm_always_ask,
                    &init_version,
                    init_config_version);
}

/*******************************************************************************
NAME
    sinkUpgradeIsUpgradeMsg

DESCRIPTION
    Check if a message should be handled by sinkUpgradeMsgHandler

PARAMETERS
    id      The ID for the message

RETURNS
    bool TRUE if it is an upgrade message, FALSE otherwise.
*/
bool sinkUpgradeIsUpgradeMsg(uint16 id)
{
    return ( (id >= UPGRADE_UPSTREAM_MESSAGE_BASE ) && (id < UPGRADE_UPSTREAM_MESSAGE_TOP) );
}

/*******************************************************************************
NAME
    sinkUpgradeMsgHandler
    
DESCRIPTION
    Handle messages specific to the Upgrade library.
    
PARAMETERS
    task    The task the message is delivered
    id      The ID for the Upgrade message
    payload The message payload
    
RETURNS
    void
*/
void sinkUpgradeMsgHandler(Task task, MessageId id, Message message)
{
    switch (id)
    {
    case MESSAGE_DFU_SQIF_STATUS:
        {
            MessageDFUFromSQifStatus *msg = (MessageDFUFromSQifStatus *)message;
            UPGRADE_INFO(("MESSAGE_DFU_SQIF_STATUS: %u\n", msg->status));
            UpgradeDfuStatus(msg);
        }
        break;

    case UPGRADE_APPLY_IND:
        {
            UPGRADE_INFO(("UPGRADE_APPLY_IND:\n"));
            UpgradeApplyResponse(NOW);
        }
        break;

    case UPGRADE_BLOCKING_IND:
        {
            UPGRADE_INFO(("UPGRADE_BLOCKING_IND:\n"));
            if(IsTonePlaying() || IsVpPlaying())
            {
                UpgradeBlockingResponse(100);
            }
            else
            {
                VolumeUpdateMuteStatusAllOutputs(TRUE);
                UpgradeBlockingResponse(NOW);
            }
        }
        break;

    case UPGRADE_BLOCKING_IS_DONE_IND:
        {
            UPGRADE_INFO(("UPGRADE_BLOCKING_IS_DONE_IND:\n"));
            VolumeUpdateMuteStatusAllOutputs(FALSE);
        }
        break;

    case UPGRADE_INIT_CFM:
        {
            UPGRADE_INFO(("UPGRADE_INIT_CFM: status %u\n", ((UPGRADE_INIT_CFM_T *)message)->status));
        }
        break;

    case UPGRADE_RESTARTED_IND:
        {
            UPGRADE_RESTARTED_IND_T *ind = (UPGRADE_RESTARTED_IND_T *)message;
            UPGRADE_INFO(("UPGRADE_RESTARTED_IND: reason %u\n", ind->reason));

            if (theSink.SinkInitialising)
            {
                /* if not in a state to be able to power on yet, 
                    re-send this message with a delay. */
                MESSAGE_MAKE(restarted, UPGRADE_RESTARTED_IND_T);
                restarted->reason = ind->reason;
                UPGRADE_INFO((" sink not initialised; delaying\n"));
                MessageSendLater(task, UPGRADE_RESTARTED_IND, restarted, UPGRADE_RESTARTED_DELAY);
            }
            else
            {
                /* The upgrade library is letting the application know that a restart
                   of some sort has occurred.

                   The reason indicates how important it is to make ourselves 
                   connectable.

                   For our purposes we use any indication of an upgrade being in progress
                   to send ourselves a power on event. */
                if (ind->reason != upgrade_reconnect_not_required)
                {
                    gaia_transport_type transport_type;

                    transport_type = (gaia_transport_type)configManagerGetUpgradeTransportType();

                    UPGRADE_INFO(("transport type used some time ago was 0x%x\n", transport_type));

                    if ((transport_type == gaia_transport_spp) ||
                            (transport_type == gaia_transport_rfcomm))
                    {
                        UPGRADE_INFO(("sending EventUsrPowerOn\n"));
                        MessageSend(&theSink.task, EventUsrPowerOn, NULL);
                    }
                    else
                    {
                        UPGRADE_INFO(("sending nothing\n"));
                    }
                }
            }
        }
        break;

    default:
        UPGRADE_INFO(("Unhandled 0x%04X\n", id));
        break;
    }
}

/*******************************************************************************
NAME
    sinkUpgradePowerEventHandler

DESCRIPTION
    Handle power events and pass relevant ones to the Upgrade library.
    The upgrade library is interested in two kinds of events:
    1. When the library is in the 'normal' state then low battery when
       a charger is not connected triggers transition to
       the 'low battery error' state.
    2. When the library is in the 'low battery error' state, then connecting
       a charger (or when battery level will magically change from low to ok
       without connecting a charger) enables transition back to
       the 'normal' state.

PARAMETERS
    void

RETURNS
    void
*/
void sinkUpgradePowerEventHandler(void)
{
    upgrade_power_state_t power_state = upgrade_battery_ok;

    if(powerManagerIsChargerConnected())
    {
        power_state = upgrade_charger_connected;
    }
    else if(powerManagerIsVbatLow() || powerManagerIsVbatCritical())
    {
        /* Only when charger is not connected and battery level is low notify
         * the upgrade library. This is because purpose the upgrade library's
         * low battery handling is to prevent draining battery to much when
         * charger is not connected.
         */
        power_state = upgrade_battery_low;
    }

    UpgradePowerManagementSetState(power_state);
}

#endif /* ENABLE_UPGRADE */
