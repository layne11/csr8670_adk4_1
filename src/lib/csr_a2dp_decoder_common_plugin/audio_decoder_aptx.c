/****************************************************************************
Copyright (c) 2016 Qualcomm Technologies International, Ltd.

FILE NAME
     audio_decoder_aptx.c
DESCRIPTION
    Implement AptX specific functionality
NOTES
*/
#include <stdlib.h>
#include <stream.h>

#include "csr_a2dp_decoder_if.h"
#include "audio_decoder_aptx.h"
 
/****************************************************************************
DESCRIPTION
    Connect AptX or AptXLL source to sink
*/
 Transform audioDecoderAptxConnectAptX(Source source, Sink sink, bool content_protection)
{
    /* AptX uses rtp headers only if content protection is enabled */
    if (content_protection)
    {
        return PanicNull(TransformRtpDecode(source, sink));
    }
    else
    {
        PanicNull(StreamConnect(source, sink));
        return (Transform)NULL;
    }
}

 /****************************************************************************
 DESCRIPTION
     Send AptX specific messages to dsp
 */
void audioDecoderAptxConfigureAptX(uint16 channel_mode, uint32 rate)
{
    audioDecodeSendAptXMessages(channel_mode, rate);
}

/****************************************************************************
DESCRIPTION
     Send AptX LL specific messages to dsp
*/
void audioDecoderAptxConfigureAptXLL(aptx_sprint_params_type* aptx_sprint_params)
{
    audioDecodeDspSendAptXLLparams(aptx_sprint_params);
}

