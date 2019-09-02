/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.1

FILE NAME
    upgrade_msg_vm.h
    
DESCRIPTION

*/
#ifndef UPGRADE_MSG_VM_H_
#define UPGRADE_MSG_VM_H_

#ifndef UPGRADE_VM_MSG_BASE
#define UPGRADE_VM_MSG_BASE 0x400
#endif

typedef enum {
    UPGRADE_VM_PERMIT_UPGRADE = UPGRADE_VM_MSG_BASE,
    UPGRADE_VM_PROHIBIT_UPGRADE,
    UPGRADE_VM_PROHIBIT_STREAMING_IND,
    UPGRADE_VM_PERMIT_STREAMING_IND
} UpgradeMsgVM;

#endif /* UPGRADE_MSG_VM_H_ */
