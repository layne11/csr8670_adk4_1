/****************************************************************************
Copyright (c) 2005 - 2016 Qualcomm Technologies International, Ltd.

FILE NAME
    csr_a2dp_decoder_common_if.h

DESCRIPTION
    Message definitions for A2DP plugins.

*/

/*!
@file csr_a2dp_decoder_common_if.h

@brief
   Definitions for A2DP plugins.

   @description


*/
#include <audio_output.h>

#ifndef _CSR_A2DP_DECODER_COMMON_INTERFACE_H_
#define _CSR_A2DP_DECODER_COMMON_INTERFACE_H_

/* I/O configuration enum for DSP which are used by various plugin types in VM */
typedef enum
{
     SBC_IO                = 1
    ,MP3_IO                = 2
    ,AAC_IO                = 3
    ,FASTSTREAM_IO         = 4
    ,USB_IO                = 5
    ,APTX_IO               = 6
    ,APTX_ACL_SPRINT_IO    = 7
    ,ANALOG_IO             = 8
    ,SPDIF_IO              = 9
    ,I2S_IO                = 10
    ,APTXHD_IO             = 11
}DSP_IO;

typedef enum
{
     DSP_SBC_DECODER                = SBC_IO                /*SBC_DECODER*/
    ,DSP_MP3_DECODER                = MP3_IO                /*MP3_DECODER*/
    ,DSP_AAC_DECODER                = AAC_IO                /* AAC_DECODER*/
    ,DSP_FASTSTREAM_SINK            = FASTSTREAM_IO         /*FASTSTREAM_SINK*/
    ,DSP_USB_DECODER                = USB_IO                /* USB_DECODER*/
    ,DSP_APTX_DECODER               = APTX_IO               /* APTX_DECODER*/
    ,DSP_APTX_ACL_SPRINT_DECODER    = APTX_ACL_SPRINT_IO    /* APTX_ACL_SPRINT_DECODER*/
    ,DSP_ANALOG_DECODER             = ANALOG_IO             /* ANALOG_DECODER */
    ,DSP_SPDIF_DECODER              = SPDIF_IO              /* SPDIF_DECODER */
    ,DSP_I2S_DECODER                = I2S_IO
    ,DSP_APTXHD_DECODER             = APTXHD_IO
} DSP_PLUGIN_TYPE;

/* dsp message structure*/
typedef struct
{
    uint16 id;
    uint16 a;
    uint16 b;
    uint16 c;
    uint16 d;
} DSP_REGISTER_T;



#endif

