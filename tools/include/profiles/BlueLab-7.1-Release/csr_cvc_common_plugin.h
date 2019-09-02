/****************************************************************************
Copyright (c) 2005 - 2016 Qualcomm Technologies International, Ltd.

FILE NAME
    csr_cvc_common_plugin.h

DESCRIPTION
    
    
NOTES
   
*/
#ifndef _CSR_CVC_COMMON_PLUGIN_H_
#define _CSR_CVC_COMMON_PLUGIN_H_

#include <message.h> 
#include <audio_plugin_if.h>

/*!  CSR_CVC_COMMON plugin

    This is an cVc plugin that can be used with the cVc DSP library.
*/

/*the messages sent to the application task registered with the SR plugin as 
part of the call to AudioConnect() 

These messages are CSR_SPEECH_RECOGNITION_MESSAGE_BASE greater than the messages 
sent from Kalimba to the SR plugin task*/
typedef enum 
{
    CSR_SR_MESSAGE_BASE            = CSR_SPEECH_RECOGNITION_MESSAGE_BASE , 
    CSR_SR_WORD_RESP_YES     ,
    CSR_SR_WORD_RESP_NO      ,
    CSR_SR_WORD_RESP_FAILED_YES,
    CSR_SR_WORD_RESP_FAILED_NO,
    CSR_SR_WORD_RESP_UNKNOWN
} CsrSpeechRecognitionId ;

/* External mic settings, applicable to APTX ll + back channel application only */
#define EXTERNAL_MIC_NOT_FITTED 1
#define EXTERNAL_MIC_FITTED     2

typedef struct
{
    unsigned external_mic_settings:2;
} CVCPluginModeParams;

typedef struct
{
    TaskData    data;
    unsigned    cvc_plugin_variant:5 ;  /* Selects the CVC plugin variant */
    unsigned    encoder:3 ;             /* Sets if its CVSd, Auri or SBC */
    unsigned    two_mic:1;              /* Set the bit if using 2mic plugin */
    unsigned    adc_dac_16kHz:1;        /* Set ADC/DAC sample rates to 16kHz */
    unsigned    reserved:6 ;            /* Set the reserved bits to zero */
} CvcPluginTaskdata;

CvcPluginTaskdata *CsrCvcGetActivePlugin(void);

extern const CvcPluginTaskdata csr_cvsd_cvc_1mic_handsfree_plugin ;
extern const CvcPluginTaskdata csr_wbs_cvc_1mic_handsfree_plugin ;
extern const CvcPluginTaskdata csr_cvsd_cvc_1mic_handsfree_bex_plugin ;

extern const CvcPluginTaskdata csr_cvsd_cvc_2mic_handsfree_plugin ;
extern const CvcPluginTaskdata csr_wbs_cvc_2mic_handsfree_plugin ;
extern const CvcPluginTaskdata csr_cvsd_cvc_2mic_handsfree_bex_plugin ;

extern const CvcPluginTaskdata csr_cvsd_cvc_1mic_headset_plugin ;
extern const CvcPluginTaskdata csr_wbs_cvc_1mic_headset_plugin ;
extern const CvcPluginTaskdata csr_cvsd_cvc_1mic_headset_bex_plugin ;

extern const CvcPluginTaskdata csr_cvsd_cvc_2mic_headset_plugin ;
extern const CvcPluginTaskdata csr_wbs_cvc_2mic_headset_plugin ;
extern const CvcPluginTaskdata csr_cvsd_cvc_2mic_headset_bex_plugin ;

extern const CvcPluginTaskdata csr_cvsd_no_dsp_plugin;

extern const CvcPluginTaskdata csr_cvsd_cvc_1mic_asr_plugin;
extern const CvcPluginTaskdata csr_cvsd_cvc_2mic_asr_plugin;

extern const CvcPluginTaskdata csr_cvsd_cvc_1mic_hf_asr_plugin;
extern const CvcPluginTaskdata csr_cvsd_cvc_2mic_hf_asr_plugin;

#endif

