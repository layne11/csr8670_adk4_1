/****************************************************************************
Copyright (c) 2016 Qualcomm Technologies International, Ltd.

FILE NAME
    csr_cvc_common_dsp_if.h

DESCRIPTION
    Interface header to DSP operations

NOTES

*/

#ifndef _CSR_CVC_COMMON_DSP_IF_H_
#define _CSR_CVC_COMMON_DSP_IF_H_

#include "csr_cvc_common_plugin.h"

#include <audio_output.h>

typedef enum
{
    link_encoding_cvsd,
    link_encoding_msbc,
    link_encoding_usb_pcm
} link_encoding_t;

typedef enum
{
    rate_matching_software,
    rate_matching_hardware
} rate_matching_t;

typedef enum
{
    hardware_type_dac,
    hardware_type_i2s
} hardware_type_t;

typedef enum
{
    cvc_mode_hands_free_kit,        /* CVC enabled */
    cvc_mode_speech_recognition,    /* CVC with speech recognition */
    cvc_mode_pass_through,          /* Pass through mode (no CVC) */
    cvc_mode_stand_by               /* Stand by (no output, low power) */
} cvc_mode_t;

typedef enum
{
    cvc_call_none,              /* Call is inactive */
    cvc_call_active             /* Call is active */
} cvc_call_state_t;

typedef enum
{
    dsp_sink_none,
#ifdef ANC
    dsp_sink_anc_mic_a,
    dsp_sink_anc_mic_b,
#endif
    dsp_sink_top
} dsp_sink_type_t;

/*******************************************************************************
DESCRIPTION
    Power on the DSP
*/
void csrCvcCommonDspPowerOn(void);

/*******************************************************************************
DESCRIPTION
    Load the required DSP functionality for a given CVC plugin task
*/
void csrCvcCommonDspLoadDsp(CvcPluginTaskdata *task);

/*******************************************************************************
DESCRIPTION
    Connect an (e)SCO source to the DSP
*/
void csrCvcCommonDspConnectScoSource(Source sco_source);

/*******************************************************************************
DESCRIPTION
    Connect an (e)SCO sink to the DSP
*/
void csrCvcCommonDspConnectScoSink(Sink sco_sink);

/*******************************************************************************
DESCRIPTION
    Connect all DSP outputs to speakers (as per multi-channel configuration)
*/
void csrCvcCommonDspConnectSpeakers(audio_output_params_t* params);

/*******************************************************************************
DESCRIPTION
    Disconnect all DSP outputs from speakers (as per multi-channel configuration)
*/
void csrCvcCommonDspDisconnectSpeakers(void);

/*******************************************************************************
DESCRIPTION
    Connect only the primary left DSP output to a speaker
*/
void csrCvcCommonDspConnectMonoSpeaker(audio_output_t output, audio_output_params_t* params);

/*******************************************************************************
DESCRIPTION
    Connect ADC or Digital microphone sources to the DSP microphone inputs
*/
void csrCvcCommonDspConnectMicrophones(Source mic_source_a, Source mic_source_b);

/*******************************************************************************
DESCRIPTION
    Connect external I2S microphone sources to the DSP microphone inputs
*/
void csrCvcCommonDspConnectI2sMicrophones(bool two_mic, uint32 i2s_rate);

/*******************************************************************************
DESCRIPTION
    Send the DSP the transport configuration data for the incoming audio link
*/
void csrCvcCommonDspConfigureTransport(link_encoding_t encoding, uint16 rate_khz);

/*******************************************************************************
DESCRIPTION
    Send the DSP the bandwidth configuration as specified in the plug-in task
*/
void csrCvcCommonDspConfigureBandwidth(CvcPluginTaskdata *task);

/*******************************************************************************
DESCRIPTION
    Notify the DSP that an external microphone has been attached or detached
*/
#ifdef REMOVABLE_MIC
void csrCvcCommonDspSendExternalMicrophoneUpdate(CvcPluginTaskdata *task, bool mic_attached);
#else
#define csrCvcCommonDspSendExternalMicrophoneUpdate(task, mic_attached) ((void)mic_attached)
#endif

/*******************************************************************************
DESCRIPTION
    Send the DSP the hardware configuration for microphones and speakers
*/
void csrCvcCommonDspConfigureHardware(uint32 dac_rate_hz, uint32 adc_rate_hz, rate_matching_t rate_matching, hardware_type_t hardware_type);

/*******************************************************************************
DESCRIPTION
    Send the DSP a message to change processing mode
*/
void csrCvcCommonDspConfigureProcessing(cvc_mode_t mode, cvc_call_state_t call_state);

/*******************************************************************************
DESCRIPTION
    Send the DSP a message to mute or unmute microphone and/or speakers
*/
void csrCvcCommonDspConfigureMute(bool mute_microphone, bool mute_speaker);

/*******************************************************************************
DESCRIPTION
    Send the DSP a message to set the speaker volume
*/
void csrCvcCommonDspConfigureVolume(const int16 volume);

/*******************************************************************************
DESCRIPTION
    Send the DSP the speech recognition configuration (as read from PS)
*/
bool csrCvcCommonDspConfigureSpeechRecognition(void);

/*******************************************************************************
DESCRIPTION
    Send the DSP a message to start (or re-start) speech recognition
*/
bool csrCvcCommonDspStartSpeechRecognition(void);

/*******************************************************************************
DESCRIPTION
    Register a task to receive stream notifications from the DSP tone mixing
    input
*/
void csrCvcCommonDspRegisterToneNotifications(Task task);

/*******************************************************************************
DESCRIPTION
    Connect a tone source to the DSP tone mixing input
*/
void csrCvcCommonDspConnectTone(AUDIO_PLUGIN_PLAY_TONE_MSG_T * tone_message);

/*******************************************************************************
DESCRIPTION
    Disconnect a tone source from the DSP tone mixing input
*/
void csrCvcCommonDspDisconnectTone(void);

/*******************************************************************************
DESCRIPTION
    Stop all DSP activity
*/
void csrCvcCommonDspStop(void);

/*******************************************************************************
DESCRIPTION
    Power off the DSP
*/
void csrCvcCommonDspPowerOff(Task task);

/*******************************************************************************
DESCRIPTION
    Get the Sink for a DSP task
*/
Sink csrCvcCommonDspGetSink(dsp_sink_type_t type);

/*******************************************************************************
DESCRIPTION
    Can a no DSP mode be supported
*/
bool csrCvcCommonDspSupportsNoDspMode(CvcPluginTaskdata *task);

#endif

