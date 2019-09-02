/****************************************************************************
Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    csr_common_example_if.h

DESCRIPTION
   
*/

#ifndef _CSR_COMMON_EXAMPLE_INTERFACE_H_
#define _CSR_COMMON_EXAMPLE_INTERFACE_H_
  
/* VM -> DSP messages */
#define MESSAGE_SETMODE                         0x0004
#define MESSAGE_REM_BT_ADDRESS                  0x2001
#define MESSAGE_EXAMPLE_PLUGIN_VOLUME           0x1002
/* DSP -> VM messages */
#define MESSAGE_EXAMPLE_PLUGIN_CODEC            0x1006
#define MESSAGE_EXAMPLE_PLUGIN_DSP_READY        0x1000
#define MESSAGE_TONE_COMPLETE                   0x1080

/* sending volume message directly after connecting ports causes
 * a loss of audio, a delayed send prevent this
 */
#define VOLUME_MESSAGE_SEND_DELAY               1000

/* Bit set in MESSAGE_EXAMPLE_PLUGIN_VOLUME message to indicate digital volume is to be used */
#define DSP_DIG_VOL_FLAG (1 << 4)

#define SYSMODE_PSTHRGH         0

#define MINUS_45dB              0x0      /* value used with SetOutputGainNow VM trap */

#define CODEC_MUTE              MINUS_45dB

/* The DAC gain must be limited to 0 dB so that no distortion occurs and so the echo canceller works. */
#define VOLUME_0DB              0x0F

#define BYTES_PER_MSBC_FRAME    60

#define DSP_INPUT_PORT_FOR_MIC_A        0
#define DSP_INPUT_PORT_FOR_MIC_B        2
#define DSP_INPUT_PORT_FOR_AG           1

#define DSP_OUTPUT_PORT_FOR_AG          DSP_OUTPUT_PORT_SUB_ESCO

#define SBC_DAC_RATE            16000


/* dsp message structure*/
typedef struct
{
    uint16 id;
    uint16 a;
    uint16 b;
    uint16 c;
    uint16 d;
} DSP_REGISTER_T;

/* Values for the selecting the plugin variant in the ExamplePluginTaskdata structure  */
typedef enum
{
   CVSD_8K_1_MIC  =  0,
   CVSD_8K_2_MIC  =  1,
   SBC_1_MIC      =  2,
   SBC_2_MIC      =  3
}EXAMPLE_PLUGIN_TYPE_T;

#endif

