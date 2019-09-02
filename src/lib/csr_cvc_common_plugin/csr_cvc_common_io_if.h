/****************************************************************************
Copyright (c) 2016 Qualcomm Technologies International, Ltd.
Part of ADK 4.1
 
FILE NAME
    csr_cvc_common_io_if.h
 
DESCRIPTION
    Microphone and outputs (e.g. DAC) related functions.
*/

#ifndef LIBS_CSR_CVC_COMMON_PLUGIN_CSR_CVC_COMMON_IO_IF_H_
#define LIBS_CSR_CVC_COMMON_PLUGIN_CSR_CVC_COMMON_IO_IF_H_

#include <app/audio/audio_if.h>
#include <gain_utils.h>

/* Define for mute volume */
#define DAC_MUTE    MIN_CODEC_GAIN_STEPS

void CsrCvcIoSetupMicParams(audio_output_params_t* params, bool no_dsp);
void CsrCvcIoSetMicGain(audio_instance instance, audio_channel channel, bool digital, T_mic_gain gain);
void CsrCvcIoMuteMic(bool is_mic_two_present);

void CsrCvcIoConnectMic(const audio_channel channel, const audio_mic_params * params);
void CsrCvcIoDisconnectMic(const audio_channel channel, const audio_mic_params * params);
audio_channel CsrCvcIoGetAudioChannelFromMicId(const microphone_input_id_t mic_id);
#define CsrCvcIoConnectMicId(mic_id) CsrCvcIoConnectMic(CsrCvcIoGetAudioChannelFromMicId(mic_id), CsrCvcGetMicParamsFromMicId(mic_id))
#define CsrCvcIoDisconnectMicId(mic_id) CsrCvcIoDisconnectMic(CsrCvcIoGetAudioChannelFromMicId(mic_id), CsrCvcGetMicParamsFromMicId(mic_id))

#define CsrCvcIoConnectMicNoCvc() CsrCvcIoConnectMicId(CVC->mic_for_no_cvc)
#define CsrCvcIoDisconnectMicNoCvc() CsrCvcIoDisconnectMicId(CVC->mic_for_no_cvc)
void CsrCvcIoConnectOutputNoCvc(void);
void CsrCvcIoDisconnectOutputNoCvc(void);

#endif /* LIBS_CSR_CVC_COMMON_PLUGIN_CSR_CVC_COMMON_IO_IF_H_ */
