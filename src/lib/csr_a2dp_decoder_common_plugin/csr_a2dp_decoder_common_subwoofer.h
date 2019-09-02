/****************************************************************************
Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    csr_a2dp_decoder_common_subwoofer.h

DESCRIPTION
    
    
NOTES
   
*/
 
#ifndef _CSR_A2DP_DECODER_COMMON_SUBWOOFER_H_
#define _CSR_A2DP_DECODER_COMMON_SUBWOOFER_H_

typedef enum
{
    sub_state_connected_esco,
    sub_state_connected_l2cap,
    sub_state_failed,
    sub_state_not_connected
} sub_state_t;

extern void CsrA2dpDecoderPluginSetSubWoofer(AUDIO_SUB_TYPE_T sub_type, Sink sub_sink);
extern void CsrA2dpDecoderPluginDisconnectSubwoofer(void);
extern void CsrA2dpDecoderPluginConnectSubwoofer(A2dpPluginConnectParams *codecData);
extern void SubConnectedNowUnmuteVolume(DECODER_t * DECODER);

#endif


