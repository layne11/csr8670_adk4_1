/****************************************************************************
Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    csr_a2dp_decoder_common_sharing.h

DESCRIPTION
    Functionality to share audio with a second device.

*/
 
#ifndef _CSR_A2DP_DECODER_COMMON_SHARING_H_
#define _CSR_A2DP_DECODER_COMMON_SHARING_H_

extern void CsrA2dpDecoderPluginForwardUndecoded(A2dpPluginTaskdata *task , bool enable, Sink sink, bool content_protection, peer_buffer_level buffer_level_required);
extern void CsrA2dpDecoderPluginDisconnectForwardingSink(void);
extern void A2DPConnectAndConfigureTWSAudio(peer_buffer_level buffer_level_required);

#endif


