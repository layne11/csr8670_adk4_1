/****************************************************************************
Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    csr_ag_audio_if.h

DESCRIPTION
   
*/

#ifndef _CSR_AG_AUDIO_INTERFACE_H_
#define _CSR_AG_AUDIO_INTERFACE_H_


#include <kalimba_if.h>

  
/* Message ID for sending mode from VM to DSP */
#define MESSAGE_SETMODE         0x0004 

/* Message ID for sending warp values from VM to DSP */
#define MESSAGE_SEND_WARP       0x1026

/* Message ID for warp values sent from DSP to VM */
#define KALIMBA_MSG_WARP_VALUES 0x1025

/* Message ID for sending Bluetooth address from VM to DSP */
#define MESSAGE_REM_BT_ADDRESS  0x2001
 
#define SYSMODE_PSTHRGH         0

#define MINUS_45dB              0x0      /* value used with SetOutputGainNow VM trap */

#define CODEC_MUTE              MINUS_45dB

#define BYTES_PER_MSBC_FRAME    60


/* Port numbers used by the DSP applications */
/* SCO <-> DSP */
#define SCO_TO_DSP_PORT         1
#define DSP_TO_SCO_PORT         DSP_OUTPUT_PORT_SUB_ESCO
/* USB <-> DSP */
#define USB_TO_DSP_PORT         0
#define DSP_TO_USB_PORT         DSP_OUTPUT_PORT_USB
/* WIRED <-> DSP */
#define ADC_TO_DSP_L_PORT       0
#define ADC_TO_DSP_R_PORT       2
#define DSP_TO_DAC_L_PORT       DSP_OUTPUT_PORT_PRI_LEFT
#define DSP_TO_DAC_R_PORT       DSP_OUTPUT_PORT_PRI_RIGHT
   

/* dsp message structure*/
typedef struct
{
    uint16 id;
    uint16 a;
    uint16 b;
    uint16 c;
    uint16 d;
} DSP_REGISTER_T;

/* Values for the selecting the plugin variant in the CsrAgAudioPluginTaskData structure  */
typedef enum
{
   CSR_AG_AUDIO_CVSD_8K_1_MIC  =  0,
   CSR_AG_AUDIO_CVSD_8K_2_MIC  =  1,
   CSR_AG_AUDIO_SBC_16K_1_MIC  =  2,
   CSR_AG_AUDIO_SBC_2_MIC      =  3,
   CSR_AG_AUDIO_CVSD_48K_1_MIC =  4,
   CSR_AG_AUDIO_SBC_48K_1_MIC  =  5
}CSR_AG_AUDIO_PLUGIN_TYPE_T;

#endif

