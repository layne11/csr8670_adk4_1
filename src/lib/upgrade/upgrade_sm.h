/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.1

FILE NAME
    upgrade_sm.h
    
DESCRIPTION
    Interface to the state machine module of the upgrade library.

*/
#ifndef UPGRADE_SM_H_
#define UPGRADE_SM_H_

#include <message.h>

/*!
    @brief Time wait for reconnection after mid-upgrade reboot. In seconds.

    Time in seconds for how long upgrade library will for reconnection after
    mid-upgrade reboot. After that time the library will apply or cancel
    upgrade automatically.
 */
#define UPGRADE_WAIT_FOR_RECONNECTION_TIME_SEC 60

/*!
    @brief Enumeration of the states in the machine.
 */
typedef enum {
    /*! TODO and all the other items too... */
    UPGRADE_STATE_CHECK_STATUS,
    UPGRADE_STATE_SYNC,
    UPGRADE_STATE_READY,
    UPGRADE_STATE_PROHIBITED,
    UPGRADE_STATE_ABORTING,
    UPGRADE_STATE_BATTERY_LOW,

    UPGRADE_STATE_DATA_READY,
    UPGRADE_STATE_DATA_TRANSFER,
    UPGRADE_STATE_DATA_TRANSFER_SUSPENDED,
    UPGRADE_STATE_VALIDATING,
    UPGRADE_STATE_WAIT_FOR_VALIDATE,
    UPGRADE_STATE_VALIDATED,

    UPGRADE_STATE_RESTARTED_FOR_COMMIT,
    UPGRADE_STATE_COMMIT_HOST_CONTINUE,
    UPGRADE_STATE_COMMIT_VERIFICATION,
    UPGRADE_STATE_COMMIT_CONFIRM,
    UPGRADE_STATE_COMMIT,

    UPGRADE_STATE_PS_JOURNAL,
    UPGRADE_STATE_REBOOT_TO_RESUME  /*! Unable to continue until a reboot */
} UpgradeState;

/*!
    @brief Enumeration of resume points in the upgrade process.

    These are key places in the upgrade process, where the host and device
    can synchronise in order to resume an interrupted upgrade process.
 */
typedef enum {
    /*! Resume from the beginning, includes download phase. */
    UPGRADE_RESUME_POINT_START,

    /*! Resume from the start of the validation phase, i.e. download is complete. */
    UPGRADE_RESUME_POINT_PRE_VALIDATE,

    /*! Resume after the validation phase, but before the device has rebooted
     *  to action the upgrade. */
    UPGRADE_RESUME_POINT_PRE_REBOOT,

    /*! Resume after the reboot */
    UPGRADE_RESUME_POINT_POST_REBOOT,

    /*! Resume at final stage of an upgrade, ready for host to commit */
    UPGRADE_RESUME_POINT_COMMIT, 

    /*! Final stage of an upgrade, partition erase still required */
    UPGRADE_RESUME_POINT_ERASE,

    /*! Resume in error handling, before reset unhandled error message have been sent */
    UPGRADE_RESUME_POINT_ERROR

} UpdateResumePoint;

/*!
    @brief TODO
    @param
    @return
*/
void UpgradeSMInit(void);

/*!
    @brief TODO
    @param
    @return
*/
UpgradeState UpgradeSMGetState(void);

/*!
    @brief TODO
    @param
    @return
*/
void UpgradeSMHandleMsg(MessageId id, Message message);

/*!
    @brief Determine if an upgrade is currently in progress.
    @return bool TRUE upgrade is in progress FALSE upgrade is not in progress.
*/
bool UpgradeSMUpgradeInProgress(void);

#endif /* UPGRADE_SM_H_ */
