/****************************************************************************
Copyright (c) 2016 Qualcomm Technologies International, Ltd.

FILE NAME
    csr_a2dp_decoder_if.h

DESCRIPTION


NOTES

*/

#ifndef _CSR_A2DP_DECODER_IF_H_
#define _CSR_A2DP_DECODER_IF_H_

#include "audio_plugin_if.h" /*for the audio_mode*/
#include "audio_plugin_common.h"
#include "audio_output.h"
#include "csr_a2dp_decoder_common.h"

#define MAX_SOURCES 2

typedef enum
{
    dsp_sink_none,
#ifdef ANC
    dsp_sink_anc_mic_a,
    dsp_sink_anc_mic_b,
#endif
    dsp_sink_mic_a,
    dsp_sink_mic_b
} dsp_sink_type_t;

typedef enum
{
    dsp_source_none,
    dsp_source_cvc_back_channel,
    dsp_source_forwarding,
    dsp_source_esco_sub,
    dsp_source_l2cap_sub
} dsp_source_type_t;

typedef struct Sources
{
    Source source[MAX_SOURCES];
    uint8 number_of_sources;
} sources_t;

void audioDecodeLoadDSP(void);
void audioDecodeConfigureDSP(A2dpPluginTaskdata* task);
void audioDecodeCancelDspMessages(A2dpPluginTaskdata * task);
void audioDecodeSendBitResolutionMessage(void);
bool audioDecodePluginGetLatency (A2dpPluginTaskdata *audio_plugin, bool *estimated, uint16 *latency);
void audioDecodeHandleDspInternalMessage(A2dpPluginTaskdata *task, uint16 id, Message message);
void audioDecodePlayTone(A2dpPluginTaskdata *task, const ringtone_note * tone);
void audioDecodeSendDspVolume(audio_output_group_t group, audio_output_gain_t* gain_info);
void audioDecodeMuteOutput(audio_output_group_t group, AUDIO_MUTE_STATE_T state);
void audioDecodeDisconnectAudio(TaskData * task);
void audioDecodeDisconnectDsp(void);
void audioDecodeEnableDsp(void);
void audioDecodeConnectDspOutputs(audio_output_params_t* params);
void audioDecodeDspSendSpdifMessages(A2dpPluginConnectParams* codec_data);
void audioDecodeSendAptXMessages(uint16 channel_mode, uint32 rate);
void audioDecodeDspSendAptXLLparams(aptx_sprint_params_type* aptx_sprint_params);
void audioDecodeSendDspSampleRateMessages(local_config_t* localConfig);
void audioDecodeSendDspMultiChannelMessages(void);
void audioDecodeEnableLatencyReporting (A2dpPluginTaskdata *audio_plugin);
void audioDecodeDspStandby(A2dpPluginModeParams *mode_params);
void audioDecodePluginSetEqMode(uint16 operating_mode, A2DP_MUSIC_PROCESSING_T music_processing, A2dpPluginModeParams *mode_params);
void audioDecodePluginUpdateEnhancements(const A2dpPluginModeParams *mode_params);
void a2dpDecoderDspSendTwsRoutingMode(uint16 master_routing, uint16 slave_routing);
void audioDecoderDspSendSbcEncodeParams(uint8 format, uint8 bitpool);
void audioDecodeDspSendDeviceTrims(int16 device_trim_master, int16 device_trim_slave);
void audioDecodeDspSendRelayMode(unsigned stream_relay_mode);
void audioDecodeDspSendTwsCompatibilityMode(peer_buffer_level buffer_level_required);
void audioDecodeDspConnectUSBToDsp(uint16 val_clock_mismatch, Source l_source, A2dpPluginConnectParams* codecData);
void audioDecodeDspConnectMicToDsp(Source l_source, Source r_source, A2dpPluginConnectParams* codecData);
uint16 audioDecodeDspConnectI2sToDsp(uint16 val_clock_mismatch, uint16 mismatch);
uint16 audioDecodeDspConnectSpdifToDsp(Source r_source, Source l_source, uint16 val_clock_mismatch, uint16 mismatch, A2dpPluginConnectParams* codecData);
void audioDecodeDisconnectOutput(void);
void CsrA2dpDecoderPluginStopTone (void);
void audioDecodeUnregisterForToneMsg(void);
void audioDecodeConnectSources(const sources_t* sources, AUDIO_SINK_T sink_type, bool content_protection);
Sink audioDecoderGetUsbMicSink(void);
Source audioDecoderGetUsbMicSource(void);

/****************************************************************************
DESCRIPTION
    Get the sink for a given function
*/
Sink audioDecoderGetDspSink(dsp_sink_type_t type);

/****************************************************************************
DESCRIPTION
    Get the source for a given function
*/
Source audioDecoderGetDspSource(dsp_source_type_t type);

#endif
