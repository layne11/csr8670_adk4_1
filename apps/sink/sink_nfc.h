/**
@file

@ingroup sink_app

Copyright (c) 2016 - 2016 Qualcomm Technologies International, Ltd.

*/

/*****************************************************************************/
/*!

\brief  This header files provide function prototype to configure basic NFC 
functionality.  It contains function illustration how to handle Rx messages
from nfc connection library.

@{        
*/
#ifndef SINK_NFC_H
#define SINK_NFC_H

/** Important Note:   <library.h> defines  NFC_MESSAGE_BASE and
 *  should be included before nfc_prim.h as nfc_prim.h
 *  will redefine that value for the purpose unit testing. */
#include <library.h>

/**
 * "ENABLE_ADK_NFC" used to enable the NFC subsystem and 
 * configure the NFC TAG with an NFC Bluetooth Static 
 * Handover. It should be defined in the ADK for product with 
 * NFC enabled */ 
#if defined(ENABLE_ADK_NFC)

#include "nfc/nfc_prim.h"
#include "nfc_cl.h" /* !< contains NFC connection lib prototype use in macros below */

/* PUBLIC MACROS *************************************************************/
#define NFC_CL_TAG_INIT(p_task) \
{ \
    NFC_CL_INIT mainNfcClInit; \
    mainNfcClInit.nfcClientRecvTask = (p_task); \
    mainNfcClInit.send_carrier_on_ind   = FALSE; \
    mainNfcClInit.send_carrier_loss_ind = FALSE; \
    mainNfcClInit.send_selected_ind     = FALSE; \
    NfcClTagInit(&mainNfcClInit); \
}

#define NFC_CL_HDL_LOCAL_NAME(local_name_msg) \
{ \
    const uint8 *local_name = ((const CL_DM_LOCAL_NAME_COMPLETE_T *)local_name_msg)->local_name; \
    uint8 size_local_name = (uint8) ((const CL_DM_LOCAL_NAME_COMPLETE_T *)local_name_msg)->size_local_name; \
    NfcClTagSetLocalName(local_name, size_local_name); \
}

#define NFC_CL_HDL_LOCAL_BDADDR(local_bdaddr_msg) \
{ \
    const bdaddr local_bdaddr = ((const CL_DM_LOCAL_BD_ADDR_CFM_T *)local_bdaddr_msg)->bd_addr; \
    NfcClTagSetBDAddr(&local_bdaddr); \
}

#define NFC_APP "NFC_APP: "

/* PUBLIC FUNCTION DECLARATIONS **********************************************/
/*!
    @brief Disable Fast Connect 
    @Warning Experimental code
*/
extern void NfcDisableFastConnect(void);
/*!
    @brief Enable Fast Connect 
    @Warning Experimental code
*/
extern void NfcEnableFastConnect(void);

/*************************************************************************/
/*!    
    @brief Function to handle the NFC CL Lib messages - these are independent
    of state.

    @param task the Sink App task
    @param id NFC message Id
    @param message Body of the message.  NULL when there is no message body.

    @return void

*/
extern void handleNfcClMessage(Task task, MessageId id, Message message);

/*!
    @brief This function is called when the NFC TAG has been activated by the 
    NFC reader and is in the process of being read. An imminent BT connection
    of disconnection will occur.

    @param task the Sink App task

    @return void
*/
extern void handleNfcClTagReadStarted(Task task);

#else
#define NFC_CL_TAG_INIT(p_task)
#define NFC_CL_HDL_LOCAL_NAME(local_name_msg)
#define NFC_CL_HDL_LOCAL_BDADDR(local_bdaddr_msg)


#endif /* ENABLE_ADK_NFC */

/*!
    @brief This function is called when the NFC RF field has been detected
    It is provided for ADK4.0 backward compatibility.

    @param task the Sink App task
    @return void
*/
void handleUsrNFCTagDetected(Task task,  Message message);

/** @}*/
#endif

