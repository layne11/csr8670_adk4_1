/****************************************************************************
Copyright (c) 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.1

FILE NAME
    sink_upgrade.h
    
DESCRIPTION
    Interface to the upgrade library.

*/

#ifndef _SINK_UPGRADE_H_
#define _SINK_UPGRADE_H_


/*************************************************************************
NAME
    sinkUpgradeInit

DESCRIPTION
    Initialise the Upgrade library
*/
#ifdef ENABLE_UPGRADE
void sinkUpgradeInit(Task task);
#else
#define sinkUpgradeInit(task) ((void)0)
#endif

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
#ifdef ENABLE_UPGRADE
bool sinkUpgradeIsUpgradeMsg(uint16 id);
#else
#define sinkUpgradeIsUpgradeMsg(id) (FALSE)
#endif

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
#ifdef ENABLE_UPGRADE
void sinkUpgradeMsgHandler(Task task, MessageId id, Message message);
#else
#define sinkUpgradeMsgHandler(task, id, message) ((void)0)
#endif

/*******************************************************************************
NAME
    sinkUpgradePowerEventHandler

DESCRIPTION
    Handle power events and pass relevant ones to the Upgrade library.

PARAMETERS
    void

RETURNS
    void
*/
#ifdef ENABLE_UPGRADE
void sinkUpgradePowerEventHandler(void);
#else
#define sinkUpgradePowerEventHandler() ((void)0)
#endif

#endif /* _SINK_UPGRADE_H_ */
