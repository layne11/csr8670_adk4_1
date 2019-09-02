/****************************************************************************
Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.1

FILE NAME
    upgrade_sm.c

DESCRIPTION

NOTES

*/

#include <stdlib.h>

#include <print.h>
#include <boot.h>
#include <loader.h>
#include <ps.h>
#include <panic.h>
#ifdef UPGRADE_USE_CSR_SIGN_VALIDATION
#include <sqif.h>
#endif

#include <upgrade.h>

#include "upgrade_ctx.h"
#include "upgrade_host_if_data.h"
#include "upgrade_partition_data.h"
#include "upgrade_psstore.h"
#include "upgrade_partitions.h"
#include "upgrade_partition_validation.h"
#include "upgrade_partitions.h"

#include "upgrade_sm.h"
#include "upgrade_msg_vm.h"
#include "upgrade_msg_host.h"
#include "upgrade_msg_fw.h"
#include "upgrade_msg_internal.h"
#include "upgrade_fw_if.h"

#define VALIDATAION_BACKOFF_TIME_MS 100

static bool HandleCheckStatus(MessageId id, Message message);
static bool HandleSync(MessageId id, Message message);
static bool HandleReady(MessageId id, Message message);
static bool HandleProhibited(MessageId id, Message message);
static bool HandleAborting(MessageId id, Message message);
static bool HandleDataReady(MessageId id, Message message);
static bool HandleDataTransfer(MessageId id, Message message);
static bool HandleDataTransferSuspended(MessageId id, Message message);
static bool HandleValidating(MessageId id, Message message);
static bool HandleWaitForValidate(MessageId id, Message message);
static bool HandleValidated(MessageId id, Message message);
static bool HandleRestartedForCommit(MessageId id, Message message);
static bool HandleCommitHostContinue(MessageId id, Message message);
static bool HandleCommitVerification(MessageId id, Message message);
static bool HandleCommitConfirm(MessageId id,Message message);
static bool HandleCommit(MessageId id, Message message);
static bool HandlePsJournal(MessageId id, Message message);
static bool HandleRebootToResume(MessageId id,Message message);
static bool HandleBatteryLow(MessageId id, Message message);

static bool DefaultHandler(MessageId id, Message message, bool handledAlready);

static void SetState(UpgradeState nextState);
static UpgradeState GetState(void);
static void MoveToState(UpgradeState nextState);

static void FatalError(UpgradeHostErrorCode ec);
static void PsSpaceError(void);
static void Erase(void);
static void Abort(void);

static void CommitConfirmYes(void);
static uint16 LoaderErrorCheck(void);

/* permission related functions */
static bool UpgradeSMHavePermissionToProceed(MessageId id);
static void UpgradeSMBlockingOpIsDone(void);
static void CallLoaderOrReboot(void);
static void PsFloodAndReboot(void);
static void InformAppsCompleteGotoSync(void);
static void UpgradeSendUpgradeStatusInd(Task task, upgrade_state_t state);

/***************************************************************************
NAME
    UpgradeSMInit  -  Initialise the State Machine

DESCRIPTION
    This function performs relevant initialisation of the state machine,
    currently just setting the initial state.

    This is currently determined by checking whether an upgraded
    application is running.

*/
void UpgradeSMInit(void)
{
    switch(UpgradeCtxGetPSKeys()->upgrade_in_progress_key)
    {
    /* UPGRADE_RESUME_POINT_PRE_VALIDATE:
        @todo: What do we do in this case ? */
    /* UPGRADE_RESUME_POINT_POST_REBOOT:
       UPGRADE_RESUME_POINT_COMMIT:
        @todo: Are these right, we want host to chat */
    default:
        SetState(UPGRADE_STATE_CHECK_STATUS);
        break;

    /*case UPGRADE_RESUME_POINT_PRE_REBOOT:
        SetState(UPGRADE_STATE_VALIDATED);
        break;*/

    case UPGRADE_RESUME_POINT_POST_REBOOT:
        SetState(UPGRADE_STATE_COMMIT_HOST_CONTINUE);
        MessageSendLater(UpgradeGetUpgradeTask(), UPGRADE_INTERNAL_RECONNECTION_TIMEOUT, NULL,
                D_SEC(UPGRADE_WAIT_FOR_RECONNECTION_TIME_SEC));
        break;

    /*case UPGRADE_RESUME_POINT_ERASE:
        MoveToState(UPGRADE_STATE_COMMIT);
        break;*/

    case UPGRADE_RESUME_POINT_ERROR:
        SetState(UPGRADE_STATE_ABORTING);
        break;
    }
}

UpgradeState UpgradeSMGetState(void)
{
    return GetState();
}

void UpgradeSMHandleMsg(MessageId id, Message message)
{
    bool handled=FALSE;

    PRINT(("curr state 0x%x msg id 0x%x\n", GetState(), id));

    switch(GetState())
    {
    case UPGRADE_STATE_BATTERY_LOW:
        handled = HandleBatteryLow(id, message);
        break;

    case UPGRADE_STATE_CHECK_STATUS:
        handled = HandleCheckStatus(id, message);
        break;

    case UPGRADE_STATE_SYNC:
        handled = HandleSync(id, message);
        break;

    case UPGRADE_STATE_READY:
        handled = HandleReady(id, message);
        break;

    case UPGRADE_STATE_PROHIBITED:
        handled = HandleProhibited(id, message);
        break;

    case UPGRADE_STATE_ABORTING:
        handled = HandleAborting(id, message);
        break;

    case UPGRADE_STATE_DATA_READY:
        handled = HandleDataReady(id, message);
        break;

    case UPGRADE_STATE_DATA_TRANSFER:
        handled = HandleDataTransfer(id, message);
        break;

    case UPGRADE_STATE_DATA_TRANSFER_SUSPENDED:
        handled = HandleDataTransferSuspended(id, message);
        break;

    case UPGRADE_STATE_VALIDATING:
        handled = HandleValidating(id, message);
        break;

    case UPGRADE_STATE_WAIT_FOR_VALIDATE:
        handled = HandleWaitForValidate(id, message);
        break;

    case UPGRADE_STATE_VALIDATED:
        handled = HandleValidated(id, message);
        break;

    case UPGRADE_STATE_RESTARTED_FOR_COMMIT:
        handled = HandleRestartedForCommit(id, message);
        break;

    case UPGRADE_STATE_COMMIT_HOST_CONTINUE:
        handled = HandleCommitHostContinue(id, message);
        break;

    case UPGRADE_STATE_COMMIT_VERIFICATION:
        handled = HandleCommitVerification(id, message);
        break;

    case UPGRADE_STATE_COMMIT_CONFIRM:
        handled = HandleCommitConfirm(id, message);
        break;

    case UPGRADE_STATE_COMMIT:
        handled = HandleCommit(id, message);
        break;

    case UPGRADE_STATE_PS_JOURNAL:
        handled = HandlePsJournal(id, message);
        break;

    case UPGRADE_STATE_REBOOT_TO_RESUME:
        handled = HandleRebootToResume(id,message);
        break;

    default:
        PRINT(("ERROR: Unhandled SM state %d",GetState()));
        break;
    }

    if(GetState() != UPGRADE_STATE_CHECK_STATUS)
    {
        handled = DefaultHandler(id, message, handled);
    }

    if (!handled)
    {
        PRINT(("Msg %d not handled",id));
    }

    PRINT(("next state 0x%x\n", GetState()));
}

bool HandleCheckStatus(MessageId id, Message message)
{
    UNUSED(message);
    switch(id)
    {
    case UPGRADE_VM_PERMIT_UPGRADE:
        SetState(UPGRADE_STATE_SYNC);
        break;

    case UPGRADE_INTERNAL_IN_PROGRESS:
        SetState(UPGRADE_STATE_RESTARTED_FOR_COMMIT);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

bool HandleBatteryLow(MessageId id, Message message)
{
    if(UpgradeCtxGet()->power_state != upgrade_battery_low)
    {
        /* this forces DefaultHandle to be called to handle sync request */
        return FALSE;
    }

    /* in this state always send low battery error message */
    if (id != UPGRADE_HOST_ERRORWARN_RES)
    {
        if (id == UPGRADE_HOST_DATA)
        {
            UPGRADE_HOST_DATA_T *msg = (UPGRADE_HOST_DATA_T *)message;

            /* There may be several of these messages in a row, and
             * we only want to send on error message.
             * Part-process the message. If we get an error response
             * then we can assume that we have already sent an error 
             * and do not send another */
            if (UPGRADE_PARTITION_DATA_XFER_ERROR ==
                          UpgradePartitionDataParseDataCopy((uint8 *)&msg->data[0], msg->length))
            {
                return TRUE;
            }
            UpgradePartitionDataStopData();
        }

        UpgradeHostIFDataSendErrorInd(UPGRADE_HOST_ERROR_BATTERY_LOW);
    }

    return TRUE;
}

bool HandleSync(MessageId id, Message message)
{
    UNUSED(message);
#ifdef UPGRADE_SYNC_WILL_FORCE_COMMIT_PHASE
    switch(id)
    {
    /* @todo
     * This is a temporary solution to force state machine to
     * proceed to commit phase after reboot.
     */
    case UPGRADE_HOST_SYNC_AFTER_REBOOT_REQ:
        UpgradeHostIFDataSendShortMsg(UPGRADE_HOST_IN_PROGRESS_IND);
        SetState(UPGRADE_STATE_COMMIT_HOST_CONTINUE);
        break;

    default:
        return FALSE;
    }

    return TRUE;
#else
    UNUSED(id);
    return FALSE;
#endif
}

bool HandleReady(MessageId id, Message message)
{
    UNUSED(message);
    switch(id)
    {
    case UPGRADE_HOST_START_REQ:
        {
            bool error = FALSE;
            bool newUpgrade = TRUE;

            PRINT(("UPGRADE_HOST_START_REQ handled\n"));
            PRINT(("P&R: going to jump to resume point %d\n", UpgradeCtxGetPSKeys()->upgrade_in_progress_key));

            switch(UpgradeCtxGetPSKeys()->upgrade_in_progress_key)
            {
            case UPGRADE_RESUME_POINT_START:
                SetState(UPGRADE_STATE_DATA_READY);
                break;
            case UPGRADE_RESUME_POINT_PRE_VALIDATE:
                UpgradePartitionValidationInit();
                MoveToState(UPGRADE_STATE_VALIDATING);
                break;
            case UPGRADE_RESUME_POINT_PRE_REBOOT:
                SetState(UPGRADE_STATE_VALIDATED);
                newUpgrade = FALSE;
                break;
            case UPGRADE_RESUME_POINT_POST_REBOOT:
                SetState(UPGRADE_STATE_COMMIT_HOST_CONTINUE);
                newUpgrade = FALSE;
                break;
            /*case UPGRADE_RESUME_POINT_COMMIT:
                SetState(UPGRADE_STATE_COMMIT_CONFIRM);
                newUpgrade = FALSE;
                break;*/
            case UPGRADE_RESUME_POINT_ERASE:
                MoveToState(UPGRADE_STATE_COMMIT);
                newUpgrade = FALSE;
                break;
            case UPGRADE_RESUME_POINT_ERROR:
                SetState(UPGRADE_STATE_ABORTING);
                /* @todo do we need to set error = TRUE here? We need to send an error to the host
                 * or we'll get stuck in the ABORTING state */
                break;
            default:
                PRINT(("UPGRADE_HOST_START_REQ unexpected in progress key %d\n", UpgradeCtxGetPSKeys()->upgrade_in_progress_key));
                error = TRUE;
            }

            if(!error)
            {
                UpgradeHostIFDataSendStartCfm(0, 0x666);
            }
            else
            {
                FatalError(UPGRADE_HOST_ERROR_INTERNAL_ERROR_4);
            }

            /* We are starting/resuming an upgrade so update target partitions */
            if (newUpgrade)
            {
                UpgradePartitionsUpgradeStarted();
            }
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

bool HandleProhibited(MessageId id, Message message)
{
    UNUSED(id);
    UNUSED(message);
    return FALSE;
}

bool HandleAborting(MessageId id, Message message)
{
    UNUSED(message);
    switch(id)
    {
    case UPGRADE_HOST_ERRORWARN_RES:
    case UPGRADE_HOST_ABORT_REQ:
        Abort();
        break;
    case UPGRADE_HOST_SYNC_REQ:
        /* TODO: Maybe we also should send something like last error here */
        UpgradeHostIFDataSendErrorInd(UPGRADE_HOST_ERROR_IN_ERROR_STATE);
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

bool HandleDataReady(MessageId id, Message message)
{
    UNUSED(message);
    switch(id)
    {
    case UPGRADE_HOST_START_DATA_REQ:
        {
            if (UpgradePartitionDataInit())
            {
                uint16 req_size = UpgradePartitionDataGetNextReqSize();
                PRINT(("UPGRADE_HOST_START_DATA_REQ req size %d\n", req_size));
                UpgradeHostIFDataSendBytesReq(req_size, 0);
                SetState(UPGRADE_STATE_DATA_TRANSFER);
            }
            else
            {
                FatalError(UPGRADE_HOST_ERROR_NO_MEMORY);
            }
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

bool HandleDataTransfer(MessageId id, Message message)
{
    switch(id)
    {
    case UPGRADE_HOST_DATA:
        if(message)
        {
            UPGRADE_HOST_DATA_T *msg = (UPGRADE_HOST_DATA_T *)message;
            UpgradeHostErrorCode rc;

            PRINT(("UPGRADE_HOST_DATA handled msg len %d\n", msg->length));

            /*{
                uint16 i;
                for(i = 0; i < msg->length; ++i)
                {
                    PRINT(("data tran data[%d] 0x%x\n", i, msg->data[i]));
                }
            }*/
            rc = UpgradePartitionDataParse((uint8 *)&msg->data[0], msg->length);

            /* Check for upgrade file size errors */
            if(rc == UPGRADE_HOST_SUCCESS && msg->moreData)
            {
                rc = UPGRADE_HOST_ERROR_FILE_TOO_SMALL;
            }
            else if(rc == UPGRADE_HOST_OEM_VALIDATION_SUCCESS && !msg->moreData)
            {
                rc = UPGRADE_HOST_ERROR_FILE_TOO_BIG;
            }

            if(rc == UPGRADE_HOST_SUCCESS)
            {
                uint32 req_size = UpgradePartitionDataGetNextReqSize();
                if(req_size)
                {
                    uint32 offset = UpgradePartitionDataGetNextOffset();
                    UpgradeHostIFDataSendBytesReq(req_size, offset);
                }
                else
                {
                    PRINT(("0 size req size\n"));
                }
            }
            else if(rc == UPGRADE_HOST_OEM_VALIDATION_SUCCESS)
            {
                /* change the resume point, now that all data has been
                 * downloaded, and ensure we remember it. */
                UpgradeCtxGetPSKeys()->upgrade_in_progress_key = UPGRADE_RESUME_POINT_PRE_VALIDATE;
                UpgradeSavePSKeys();
                PRINT(("P&R: UPGRADE_RESUME_POINT_PRE_VALIDATE saved\n"));

                UpgradePartitionValidationInit();
                MoveToState(UPGRADE_STATE_VALIDATING);
            }
            else
            {
                if(rc == UPGRADE_HOST_ERROR_PARTITION_CLOSE_FAILED_PS_SPACE)
                {
                    PsSpaceError();
                }
                else
                {
                    FatalError(rc);
                }
            }
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

bool HandleDataTransferSuspended(MessageId id, Message message)
{
    UNUSED(id);
    UNUSED(message);
    return FALSE;
}

bool HandleValidating(MessageId id, Message message)
{
    UNUSED(message);
    switch(id)
    {
    case UPGRADE_INTERNAL_CONTINUE:
        {
            UpgradePartitionValidationResult res;
            res = UpgradePartitionValidationValidate();
            if(res == UPGRADE_PARTITION_VALIDATION_IN_PROGRESS)
            {
                MoveToState(UPGRADE_STATE_WAIT_FOR_VALIDATE);
            }
            else
            {
                /* Validation completed, and now waiting for UPGRADE_TRANSFER_COMPLETE_RES
                 * protocol message. Update resume point and ensure we remember it. */
                UpgradeCtxGetPSKeys()->upgrade_in_progress_key = UPGRADE_RESUME_POINT_PRE_REBOOT;
                UpgradeSavePSKeys();
                PRINT(("P&R: UPGRADE_RESUME_POINT_PRE_REBOOT saved\n"));

                MoveToState(UPGRADE_STATE_VALIDATED);
            }
        }
        break;

    case UPGRADE_HOST_IS_CSR_VALID_DONE_REQ:
        UpgradeHostIFDataSendIsCsrValidDoneCfm(VALIDATAION_BACKOFF_TIME_MS);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

bool HandleWaitForValidate(MessageId id, Message message)
{
    UNUSED(message);
    switch(id)
    {
#ifdef UPGRADE_USE_CSR_SIGN_VALIDATION
    case MESSAGE_EXE_FS_VALIDATION_STATUS:
        {
            MessageExeFsValidationResult *msg = (MessageExeFsValidationResult*)message;

            PRINT(("MESSAGE_EXE_FS_VALIDATION_STATUS received result is %s\n", (msg->result == VALIDATE_PARTITION_PASS) ? "PASS" : "FAIL"));
            if(msg->result == VALIDATE_PARTITION_PASS)
            {
                    MoveToState(UPGRADE_STATE_VALIDATING);
            }
            else
            {
                FatalError(UPGRADE_HOST_ERROR_SFS_VALIDATION_FAILED);
            }
        }
        break;
#endif

    case UPGRADE_HOST_IS_CSR_VALID_DONE_REQ:
        UpgradeHostIFDataSendIsCsrValidDoneCfm(VALIDATAION_BACKOFF_TIME_MS);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

/*
NAME
    CallLoaderOrReboot

DESCRIPTION
    Process the required actions from HandleValidated.

    Depending on the type of upgrade being performed either call
    the loader to execute a DFU from SQIF, or warm reboot to change
    the partitions in SQIF that are mounted.
    
    Called either immediately or once we receive permission from
    the VM application.
*/
static void CallLoaderOrReboot(void)
{
    if (UpgradePartitionDataIsDfuUpdate())
    {
        uint16 partition = UpgradePartitionDataGetDfuPartition();

        /* This will reboot into the loader to apply the dfu */
        LoaderPerformDfuFromSqif(partition);

        /* If we get here then the dfu file failed the initial
           validation checks and the device was not rebooted by
           the firmware. We still need to reboot to reconnect to
           the host though. */
        UpgradeCtxGetPSKeys()->loader_msg = UPGRADE_LOADER_MSG_INVALID;
        UpgradeSavePSKeys();
    }
    
    BootSetMode(BootGetMode());
}

/* This is the last state before reboot */
bool HandleValidated(MessageId id, Message message)
{
    switch(id)
    {
    case UPGRADE_HOST_TRANSFER_COMPLETE_RES:
        {
            UPGRADE_HOST_TRANSFER_COMPLETE_RES_T *msg = (UPGRADE_HOST_TRANSFER_COMPLETE_RES_T *)message;
            if(msg->action == 0)
            {
                UpgradeCtxGetPSKeys()->upgrade_in_progress_key = UPGRADE_RESUME_POINT_POST_REBOOT;
                UpgradeSavePSKeys();
                PRINT(("P&R: UPGRADE_RESUME_POINT_POST_REBOOT saved\n"));

                UpgradeSetToTryUpgrades();

                /* if we have permission, go ahead and call loader/reboot */
                if (UpgradeSMHavePermissionToProceed(UPGRADE_APPLY_IND))
                {
                    CallLoaderOrReboot();
                }
            }
            else
            {
                MoveToState(UPGRADE_STATE_SYNC);
            }
        }
        break;

    case UPGRADE_HOST_IS_CSR_VALID_DONE_REQ:
        UpgradeHostIFDataSendShortMsg(UPGRADE_HOST_TRANSFER_COMPLETE_IND);
        break;

    /* application finally gave permission, warm reboot */
    case UPGRADE_INTERNAL_REBOOT:
        {
            CallLoaderOrReboot();
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

bool HandleRestartedForCommit(MessageId id, Message message)
{
    UNUSED(message);
    switch(id)
    {
    case UPGRADE_HOST_SYNC_AFTER_REBOOT_REQ:
        UpgradeHostIFDataSendShortMsg(UPGRADE_HOST_IN_PROGRESS_IND);
        SetState(UPGRADE_STATE_COMMIT_HOST_CONTINUE);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

/* We end up here after reboot */
bool HandleCommitHostContinue(MessageId id, Message message)
{
    switch(id)
    {
    case UPGRADE_HOST_IN_PROGRESS_RES:
        {
            UPGRADE_HOST_IN_PROGRESS_RES_T *msg = (UPGRADE_HOST_IN_PROGRESS_RES_T *)message;

            MessageCancelFirst(UpgradeGetUpgradeTask(), UPGRADE_INTERNAL_RECONNECTION_TIMEOUT);

            if(msg->action == 0)
            {
                MoveToState(UPGRADE_STATE_COMMIT_VERIFICATION);
            }
            else
            {
                MoveToState(UPGRADE_STATE_SYNC);
            }
        }
        break;

    case UPGRADE_INTERNAL_RECONNECTION_TIMEOUT:
        {
            bool dfu = UpgradePartitionDataIsDfuUpdate();
            uint16 err = LoaderErrorCheck();

            if(dfu && !err)
            {
                /* Carry on */
                CommitConfirmYes();
            }
            else
            {
                /* Revert */
                UpgradeRevertUpgrades();
                UpgradeCtxGetPSKeys()->upgrade_in_progress_key = UPGRADE_RESUME_POINT_ERROR;
                UpgradeSavePSKeys();
                PRINT(("P&R: UPGRADE_RESUME_POINT_ERROR saved\n"));
                SetState(UPGRADE_STATE_SYNC);
                BootSetMode(BootGetMode());
            }
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

bool HandleCommitVerification(MessageId id, Message message)
{
    UNUSED(message);
    switch(id)
    {
    case UPGRADE_INTERNAL_CONTINUE:
        {
#ifdef UPGRADE_USE_CSR_SIGN_VALIDATION
            sqif_app_validate_status status;
            status = SqifGetAppValidateStatus();
            if(status == SQIF_APP_VALIDATION_RUNNING)
            {
                MoveToState(UPGRADE_STATE_COMMIT_VERIFICATION);
            }
            else
#else
            {
                bool dfu = UpgradePartitionDataIsDfuUpdate();
                uint16 err = LoaderErrorCheck();

                if(err)
                {
                    /* TODO: Delete transient store = revert upgrade */
                    FatalError(err);
                }
                else
                {
                    /*UpgradeCtxGetPSKeys()->loader_msg = UPGRADE_LOADER_MSG_NONE;
                    UpgradeCtxGetPSKeys()->dfu_partition_num = 0;
                    UpgradeCtxGetPSKeys()->upgrade_in_progress_key = UPGRADE_RESUME_POINT_COMMIT;

                    UpgradeSavePSKeys();
                    PRINT(("P&R: UPGRADE_RESUME_POINT_COMMIT saved\n"));*/

                    if(dfu)
                    {
                        MoveToState(UPGRADE_STATE_COMMIT_CONFIRM);
                    }
                    else
                    {
                        UpgradeHostIFDataSendShortMsg(UPGRADE_HOST_COMMIT_REQ);
                        SetState(UPGRADE_STATE_COMMIT_CONFIRM);
                    }
                }
            }
#endif
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

bool HandleCommitConfirm(MessageId id, Message message)
{
    switch(id)
    {
    case UPGRADE_HOST_COMMIT_CFM:
        {
            UPGRADE_HOST_COMMIT_CFM_T *cfm = (UPGRADE_HOST_COMMIT_CFM_T *)message;
            switch (cfm->action)
            {
            case UPGRADE_HOSTACTION_YES:
                CommitConfirmYes();
                break;

            case UPGRADE_HOSTACTION_NO:
                /** @todo Call to UpgradeRevertUpgrades here */
                UpgradeRevertUpgrades();
                UpgradeCtxGetPSKeys()->upgrade_in_progress_key = UPGRADE_RESUME_POINT_PRE_REBOOT;
                UpgradeSavePSKeys();
                PRINT(("P&R: UPGRADE_RESUME_POINT_PRE_REBOOT saved\n"));
                SetState(UPGRADE_STATE_SYNC);
                BootSetMode(BootGetMode());
                break;

            /** default:
               @todo: Error case. Should we handle ? Who cancels timeout */
            }
        }
        break;

    case UPGRADE_INTERNAL_CONTINUE:
        CommitConfirmYes();
        break;

    default:
        /** @todo We don't handle unexpected messages in most cases, error messages
          are dealt with in the default handler.

           BUT how do we deal with timeout ?  It *should* be in the state
          machine */
        return FALSE;
    }

    return TRUE;
}

/*
NAME
    InformAppsCompleteGotoSync

DESCRIPTION
    Process the required actions from HandleCommit.

    Send messages to both the Host and VM applications to inform them that
    the upgrade is complete.

    Called either immediately or once we receive permission from
    the VM application.
*/
static void InformAppsCompleteGotoSync(void)
{
    /* tell host application we're complete */
    UpgradeHostIFDataSendShortMsg(UPGRADE_HOST_COMPLETE_IND);
    /* tell VM application we're complete */
    UpgradeSendUpgradeStatusInd(UpgradeGetAppTask(), upgrade_state_done);
    /* go back to SYNC state to be ready to start again */
    SetState(UPGRADE_STATE_SYNC);
}

/*
NAME
    HandleCommit

DESCRIPTION
    Handle event messages in the Commit state of the FSM.
*/
bool HandleCommit(MessageId id, Message message)
{
    UNUSED(message);
    switch(id)
    {
    case UPGRADE_INTERNAL_CONTINUE:
        {
            UpgradeCtxGetPSKeys()->upgrade_in_progress_key = UPGRADE_RESUME_POINT_ERASE;
            UpgradeSavePSKeys();
            PRINT(("P&R: UPGRADE_RESUME_POINT_ERASE saved\n"));

            /* We erase all partitions at this point.
             * We have taken the hit of disrupting audio etc. by doing a reboot
             * to get here.
             * @todo An error code would indicate that we couldn't erase all
             * the partitions we expected to so we need to set a state of WTF ?
             */
            UpgradeCtxGetPSKeys()->version = UpgradeCtxGetPSKeys()->version_in_progress;
            UpgradeCtxGetPSKeys()->config_version = UpgradeCtxGetPSKeys()->config_version_in_progress;

            /* only erase if we already have permission, get permission if we don't */
            if (UpgradeSMHavePermissionToProceed(UPGRADE_BLOCKING_IND))
            {
                Erase();
                InformAppsCompleteGotoSync();
            }
        }
        break;

    case UPGRADE_HOST_ABORT_REQ:
        /* ignore abort from host when we are in the commit state */
        break;

        /* VM application permission granted for erase */
    case UPGRADE_INTERNAL_ERASE:
        {
            Erase();
            InformAppsCompleteGotoSync();
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

bool HandlePsJournal(MessageId id, Message message)
{
    UNUSED(id);
    UNUSED(message);
    return FALSE;
}

/*
NAME
    PsFloodAndReboot

DESCRIPTION
    Process the required actions from HandleRebootToResume.

    Flood PS to force a defrag on next boot, then do a warm reboot.

    Called either immediately or once we receive permission from
    the VM application.
*/
static void PsFloodAndReboot(void)
{
    PsFlood();
    BootSetMode(BootGetMode());
}

/*
NAME
    HandleRebootToResume - In UPGRADE_STATE_REBOOT_TO_RESUME

DESCRIPTION
    Unable to continue with an upgrade as we cannot guarantee the required
    PSKEY operations.

    Once the error that got us to this state is acknowledged we will
    reboot if the VM application permits, otherwise remaining in this state
    handling all messages that might otherwise cause an activity.
*/
bool HandleRebootToResume(MessageId id, Message message)
{
    UPGRADE_HOST_ERRORWARN_RES_T *errorwarn_res = (UPGRADE_HOST_ERRORWARN_RES_T *)message;

    switch(id)
    {
    case UPGRADE_HOST_ERRORWARN_RES:
        if (errorwarn_res->errorCode == UPGRADE_HOST_ERROR_PARTITION_CLOSE_FAILED_PS_SPACE)
        {
            if (UpgradeSMHavePermissionToProceed(UPGRADE_APPLY_IND))
            {
                PsFloodAndReboot();
            }
        }
        else
        {
            /* Message not handled */
            return FALSE;
        }
        break;
    
        /* got permission from the application, go ahead with reboot */
    case UPGRADE_INTERNAL_REBOOT:
        {
            PsFloodAndReboot();
        }
        break;

    case UPGRADE_HOST_SYNC_REQ:
    case UPGRADE_HOST_START_REQ:
    case UPGRADE_HOST_ABORT_REQ:
        /*! @todo: Should we be sending the respective CFM messages back in this case ?
         *
         * In most cases there is no error code.
         */
        UpgradeHostIFDataSendErrorInd(UPGRADE_HOST_ERROR_PARTITION_CLOSE_FAILED_PS_SPACE);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}


/*
NAME
    DefaultHandler - Deal with messages which we want to handle in all states

DESCRIPTION
    Default processing of messages which may be handled at any time.

    These are NOT normally processed if they have been dealt with already in
    the state machine, but this can be done.
 */
bool DefaultHandler(MessageId id, Message message, bool handled)
{
    if (!handled)
    {
        switch(id)
        {
        case UPGRADE_HOST_SYNC_REQ:
            {
                UPGRADE_HOST_SYNC_REQ_T *req = (UPGRADE_HOST_SYNC_REQ_T *)message;
                PRINT(("UPGRADE_HOST_SYNC_REQ handled 0x%lx %ld\n", req->inProgressId, req->inProgressId));

                /* refuse to sync if upgrade is not permitted */
                if (UpgradeCtxGet()->perms == upgrade_perm_no)
                {
                    UpgradeHostIFDataSendErrorInd(UPGRADE_HOST_ERROR_APP_NOT_READY);
                }
                /* Check upgrade ID */
                else if(req->inProgressId == 0)
                {
                    UpgradeHostIFDataSendErrorInd(UPGRADE_HOST_ERROR_INVALID_SYNC_ID);
                }
                else if(UpgradeCtxGetPSKeys()->id_in_progress == 0
                    || UpgradeCtxGetPSKeys()->id_in_progress == req->inProgressId)
                {
                    UpgradeHostIFDataSendSyncCfm(UpgradeCtxGetPSKeys()->upgrade_in_progress_key, req->inProgressId);

                    UpgradeCtxGetPSKeys()->id_in_progress = req->inProgressId;
                    /*!
                        @todo Need to minimise the number of times that we write to the PS
                              so this may not be the optimal place. It will do for now.
                    */
                    UpgradeSavePSKeys();

                    SetState(UPGRADE_STATE_READY);
                }
                else
                {
                    /* Send a warning to a host, which then can force upgrade with this
                       file by sending ABORT_REQ and SYNC_REQ again.
                     */
                    UpgradeHostIFDataSendErrorInd(UPGRADE_HOST_WARN_SYNC_ID_IS_DIFFERENT);
                }
            }
            break;

        case UPGRADE_HOST_ABORT_REQ:
            PRINT(("UPGRADE_HOST_ABORT_REQ handled\n"));
            UpgradeHostIFDataSendShortMsg(UPGRADE_HOST_ABORT_CFM);
            Abort();
            break;

        case UPGRADE_HOST_VERSION_REQ:
            PRINT(("UPGRADE_HOST_VERSION_REQ\n"));
            /* reply to UPGRADE_VERSION_REQ with UPGRADE_VERSION_CFM
             * sending our current upgrade and PS config version numbers */
            UpgradeHostIFDataSendVersionCfm(UpgradeCtxGetPSKeys()->version.major,
                                            UpgradeCtxGetPSKeys()->version.minor,
                                            UpgradeCtxGetPSKeys()->config_version);
            break;

        case UPGRADE_HOST_VARIANT_REQ:
            PRINT(("UPGRADE_HOST_VARIANT_REQ\n"));
            UpgradeHostIFDataSendVariantCfm(UpgradeFWIFGetDeviceVariant());
            break;

        case UPGRADE_INTERNAL_BATTERY_LOW:
            PRINT(("UPGRADE_INTERNAL_BATTERY_LOW\n"));
            SetState(UPGRADE_STATE_BATTERY_LOW);
            break;

        /* got required permission from VM app, erase and return to SYNC state */
        case UPGRADE_INTERNAL_ERASE:
            {
                Erase();
                SetState(UPGRADE_STATE_SYNC);
            }
            break;

        default:
            return FALSE;
        }
        handled = TRUE;
    }

    return handled;
}

void SetState(UpgradeState nextState)
{
    UpgradeCtxGet()->smState = nextState;
}

UpgradeState GetState(void)
{
    return UpgradeCtxGet()->smState;
}

void MoveToState(UpgradeState nextState)
{
    SetState(nextState);
    MessageSend(UpgradeGetUpgradeTask(), UPGRADE_INTERNAL_CONTINUE, NULL);
}

void FatalError(UpgradeHostErrorCode ec)
{
    UpgradeHostIFDataSendErrorInd((uint16)ec);
    SetState(UPGRADE_STATE_ABORTING);
    UpgradeCtxGetPSKeys()->upgrade_in_progress_key = UPGRADE_RESUME_POINT_ERROR;
    UpgradeSavePSKeys();
}

void PsSpaceError(void)
{
    UpgradeHostIFDataSendErrorInd((uint16)UPGRADE_HOST_ERROR_PARTITION_CLOSE_FAILED_PS_SPACE);
    SetState(UPGRADE_STATE_REBOOT_TO_RESUME);
}

/*
NAME
    Erase - Clean up after an upgrade, even if it was aborted.

DESCRIPTION
    Erase any partitions that will be needed for any future upgrade.
    Clear the transient data in the upgrade PS key data.

    Note: Before calling this function make sure it is ok to erase
          partitions on the external flash because it will block other
          services during the erase.
*/
void Erase(void)
{
    /* If a partition is currently open, close it now.
       Otherwise the f/w will think it is in use and not erase it. */
    UpgradePartitionDataCtx *ctx = UpgradeCtxGetPartitionData();
    if (ctx)
    {
        if (ctx->partitionHdl)
        {
             UpgradeFWIFPartitionClose(ctx->partitionHdl);
        }
    }

    /* Erase any partitions that require it. */
    UpgradeCtxGetPSKeys()->state_of_partitions = UpgradePartitionsEraseAllManaged();

    /* Reset the transient state data in upgrade PS key. */
    UpgradeCtxGetPSKeys()->version_in_progress.major = 0;
    UpgradeCtxGetPSKeys()->version_in_progress.minor = 0;
    UpgradeCtxGetPSKeys()->config_version_in_progress = 0;
    UpgradeCtxGetPSKeys()->id_in_progress = 0;

    UpgradeCtxGetPSKeys()->upgrade_in_progress_key = UPGRADE_RESUME_POINT_START;
    UpgradeCtxGetPSKeys()->last_closed_partition = 0;
    UpgradeCtxGetPSKeys()->dfu_partition_num = 0;
    UpgradeCtxGetPSKeys()->loader_msg = UPGRADE_LOADER_MSG_NONE;
    UpgradePartitionsUpgradeStarted();
    UpgradeSavePSKeys();
    PRINT(("P&R: UPGRADE_RESUME_POINT_START saved\n"));

    /* Let application know that erase is done */
    UpgradeSMBlockingOpIsDone();
}

/*
NAME
    Abort - Clean everything and go to the sync state.

DESCRIPTION
    Common handler for clearing up an upgrade after an abort
    and going back to a state that is ready for a new upgrade.
*/
void Abort(void)
{
    /* if we have permission erase immediately and return to the SYNC state
     * to start again if required */
    if (UpgradeSMHavePermissionToProceed(UPGRADE_BLOCKING_IND))
    {
        Erase();
        SetState(UPGRADE_STATE_SYNC);
    }
}

void CommitConfirmYes(void)
{
    UpgradeCommitUpgrades();
    MoveToState(UPGRADE_STATE_COMMIT);
}

uint16 LoaderErrorCheck(void)
{
    uint16 err = 0;

    if(UpgradePartitionDataIsDfuUpdate())
    {
        switch(UpgradeCtxGetPSKeys()->loader_msg)
        {
        case UPGRADE_LOADER_MSG_SUCCESS:
            break;

        case UPGRADE_LOADER_MSG_ERROR:
        case UPGRADE_LOADER_MSG_INVALID:
            err = UPGRADE_HOST_ERROR_LOADER_ERROR;
            break;

        default:
            err = UPGRADE_HOST_ERROR_MISSING_LOADER_MSG;
        }
    }
    else if(UpgradeCtxGetPSKeys()->loader_msg != UPGRADE_LOADER_MSG_NONE)
    {
        err = UPGRADE_HOST_ERROR_UNEXPECTED_LOADER_MSG;
    }

    return err;
}

/***************************************************************************
NAME
    UpgradeSMUpgradeInProgress

DESCRIPTION
    Return boolean indicating if an upgrade is currently in progress.
*/
bool UpgradeSMUpgradeInProgress(void)
{
    return (GetState() >= UPGRADE_STATE_READY);
}

/***************************************************************************
NAME
    UpgradeSMHavePermissionToProceed

DESCRIPTION
    Decide if we should perform an action now, not at all, or ask the
    application for permission.
    
*/
static bool UpgradeSMHavePermissionToProceed(MessageId id)
{
    switch (UpgradeCtxGet()->perms)
    {
        /* we currently have no permission to upgrade, so no permission
         * to do anything */
        case upgrade_perm_no:
            {
                /* @todo if we're here, for reboot or erase, but have no
                 * permission to do so, should we clean up/abort?
                 */
                return FALSE;
            }

            /* we have permission to do anything without question */
        case upgrade_perm_assume_yes:
            return TRUE;

            /* we have permission, but must ask the application, send
             * a message AND return FALSE so the caller doesn't do anything
             * yet, but waits until the application responds. */
        case upgrade_perm_always_ask:
            MessageSend(UpgradeCtxGet()->mainTask, id, NULL);
            return FALSE;
    }

    return FALSE;
}

/***************************************************************************
NAME
    UpgradeSMBlockingOpIsDone

DESCRIPTION
    Let know application that blocking operation is finished.
*/
static void UpgradeSMBlockingOpIsDone(void)
{
    if(UpgradeCtxGet()->perms == upgrade_perm_always_ask)
    {
        /* Cancelling messages shouldn't be needed but do it anyway */
        MessageCancelAll(UpgradeCtxGet()->mainTask, UPGRADE_BLOCKING_IND);
        MessageSend(UpgradeCtxGet()->mainTask, UPGRADE_BLOCKING_IS_DONE_IND, NULL);
    }
}

/***************************************************************************
NAME
    UpgradeSendUpgradeStatusInd

DESCRIPTION
    Build and send an UPGRADE_STATUS_IND message to the VM application.
*/
static void UpgradeSendUpgradeStatusInd(Task task, upgrade_state_t state)
{
    UPGRADE_STATUS_IND_T *upgradeStatusInd = (UPGRADE_STATUS_IND_T *)PanicUnlessMalloc(sizeof(UPGRADE_STATUS_IND_T));
    upgradeStatusInd->state = state;
    MessageSend(task, UPGRADE_STATUS_IND, upgradeStatusInd);
}
