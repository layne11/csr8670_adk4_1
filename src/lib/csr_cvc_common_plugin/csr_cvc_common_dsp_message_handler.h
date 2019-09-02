/****************************************************************************
Copyright (c) 2016 Qualcomm Technologies International, Ltd.

FILE NAME
    csr_cvc_common_dsp_message_handler.h

DESCRIPTION
    Interface header to DSP message handler
    
NOTES
   
*/

#ifndef _CSR_CVC_COMMON_DSP_MESSAGE_HANDLER_H_
#define _CSR_CVC_COMMON_DSP_MESSAGE_HANDLER_H_

#include "csr_cvc_common_plugin.h"

/****************************************************************************
DESCRIPTION
    Handles messages from the DSP
*/
void CsrCvcCommonDspMessageHandler(CvcPluginTaskdata *task ,uint16 id, Message message);

#endif /* _CSR_CVC_COMMON_DSP_MESSAGE_HANDLER_H_ */

