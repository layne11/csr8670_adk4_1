/****************************************************************************
Copyright (c) 2016 Qualcomm Technologies International, Ltd.

FILE NAME
     audio_decoder_aptx.h
DESCRIPTION
    Implement AptX specific functionality
NOTES
*/

#include <Transform.h>
#include <source.h>
#include <sink.h>

#ifndef _AUDIO_DECODER_APTX_H_
#define _AUDIO_DECODER_APTX_H_

Transform audioDecoderAptxConnectAptX(Source source, Sink sink, bool content_protection);
void audioDecoderAptxConfigureAptX(uint16 channel_mode, uint32 rate);
void audioDecoderAptxConfigureAptXLL(aptx_sprint_params_type* aptx_sprint_params);

#endif
