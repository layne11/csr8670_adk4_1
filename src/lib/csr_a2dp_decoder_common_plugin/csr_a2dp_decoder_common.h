/****************************************************************************
Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    csr_a2dp_decoder_common.h

DESCRIPTION
    
    
NOTES
   
*/
 
#ifndef _CSR_A2DP_DECODER_COMMON_H_
#define _CSR_A2DP_DECODER_COMMON_H_

#include <audio_plugin_if.h>
#include <audio_plugin_common.h>
#include <audio_plugin_music_params.h>
#include <audio_plugin_music_variants.h>
#include <audio_output.h>

#ifdef DEBUG_PRINT_ENABLED
#define KALIMBA_SEND_MESSAGE(id, a, b, c, d) \
if(!KalimbaSendMessage(id, a, b, c, d)) \
{\
    PRINT(("KalimbaSendMessageFailed %d\n", id)); \
    Panic(); \
}
#else
#define KALIMBA_SEND_MESSAGE(id, a, b, c, d) \
PanicFalse(KalimbaSendMessage(id, a, b, c, d));
#endif

/*  The following PS Key can be used to define a non-default maximum clock mismatch between SRC and SNK devices.
    If the PS Key is not set, the default maximum clock mismatch value will be used.
    The default value has been chosen to have a very good THD performance and to avoid audible pitch shifting
    effect even during harsh conditions (big jitters, for example). While the default covers almost all phones
    and other streaming sources by a big margin, some phones could prove to have a larger percentage clock drift.
*/
#define PSKEY_MAX_CLOCK_MISMATCH    0x2258 /* PSKEY_DSP0 */

#define MIXED_MODE_INCREASING_DELAY 42 /* 42 ms optimum delay for increasing volume */
#define MIXED_MODE_DECREASING_DELAY 25 /* 25 ms optimum delay for decreasing volume */

/* allow 100ms for dsp buffers to fill to allow soft mute functionality to work */
#define ALLOW_FULL_DSP_BUFFER_DELAY_FOR_SOFT_MUTE 100
/* if subwoofer port hasn't connected within 5 seconds then assume it won't */
#define SUBWOOFER_CONNECTION_FAILURE_TIMEOUT 5000

typedef struct
{
    uint16 id;
    uint16 size;
    char   buf[1];
} DSP_LONG_REGISTER_T;

typedef struct sync_Tag
{
    A2dpPluginTaskdata *task;
    Sink media_sink ;
    Sink forwarding_sink ;
    /*! mono or stereo*/
    AudioPluginFeatures features;

    /* Selects the A2DP plugin variant */
    A2DP_DECODER_PLUGIN_TYPE_T  a2dp_plugin_variant:8 ;
    /* type of audio source, used to determine what port to connect to dsp */
    AUDIO_SINK_T sink_type:8;

    /*! The current mode */
    unsigned mode:8 ;
    unsigned master_routing_mode:2;
    unsigned slave_routing_mode:2;
    unsigned routing_mode_change_pending:1;
    unsigned stream_relay_mode:2;
    unsigned relay_mode_change_pending:1;
    
    unsigned sbc_encoder_bitpool:8;
    unsigned sbc_encoder_format:8;
    
    unsigned sbc_encoder_params_pending:1;
    unsigned external_volume_enabled:1;
    unsigned device_trims_pending:1;
    unsigned dsp_ports_connected:1;
    unsigned input_audio_port_mute_active:1;
    unsigned :1;
    unsigned packet_size:10;                /* Used to configure RTP transform when forwarding undecoded audio frames */

    /* Additional mode parameters */
    void * mode_params;
    /* digital volume structure including trim gains */    
    AUDIO_PLUGIN_SET_GROUP_VOLUME_MSG_T volume;
    void * params;
    uint32 rate;                /* Codec sample rate (input rate to DSP) */
    uint16 dsp_resample_rate;   /* Output sample rate (required output rate from DSP, divided by DSP_RESAMPLING_RATE_COEFFICIENT ready to send in Kalimba message) */
    Task app_task;
    AUDIO_MUTE_STATE_T mute_state[audio_mute_group_max];
}DECODER_t ;

typedef struct LocalConfig
{
    uint16 mismatch;
    uint16 val_clock_mismatch;
    bool content_protection;
    A2dpPluginModeParams *mode_params;
    A2dpPluginConnectParams* codec_data;
    audio_output_params_t mch_params;
} local_config_t;


/*plugin functions*/
void csrA2dpDecoderEnableExternalVolume (bool enabled);
void csrA2dpDecoderSetTwsRoutingMode (uint16 master_routing, uint16 slave_routing);
void csrA2dpDecoderSetSbcEncoderParams (uint8 bitpool, uint8 format);
void csrA2dpDecoderSetTWSDeviceTrims (int16 device_trim_master, int16 device_trim_slave);
void csrA2dpDecoderSetStreamRelayMode (uint16 mode);
void CsrA2dpDecoderPluginConnect( A2dpPluginTaskdata *task, 
                                  Sink audio_sink , 
                                  AUDIO_SINK_T sink_type,
                                  int16 volume ,
                                  uint32 rate , 
                                  AudioPluginFeatures features,
                                  AUDIO_MODE_T mode , 
                                  void * params,
                                  Task app_task );
void CsrA2dpDecoderPluginDisconnect(A2dpPluginTaskdata *task);
void CsrA2dpDecodePluginFreeResources(void);
void CsrA2dpDisconnectSource(A2dpPluginTaskdata *task);
void CsrA2dpDecoderPluginSetVolume(AUDIO_PLUGIN_SET_GROUP_VOLUME_MSG_T *volumeDsp) ;
void CsrA2dpDecoderPluginResetVolume(void);
void CsrA2dpDecoderPluginSetSoftMute(AUDIO_PLUGIN_SET_SOFT_MUTE_MSG_T* message);
void csrA2dpDecoderPluginOutputMute(audio_output_group_t group, AUDIO_MUTE_STATE_T state);
void csrA2dpDecoderModeConnected(A2DP_MUSIC_PROCESSING_T music_processing, A2dpPluginModeParams* mode_params);
void CsrA2dpDecoderPluginSetMode (AUDIO_MODE_T mode , const void * params);
void CsrA2dpDecoderPluginPlayTone ( A2dpPluginTaskdata *task, const ringtone_note * tone);
void CsrA2dpDecoderPluginToneComplete ( void ) ;
void CsrA2dpDecoderPluginInternalMessage( A2dpPluginTaskdata *task ,uint16 id , Message message ) ;
DECODER_t * CsrA2dpDecoderGetDecoderData(void);
void csrA2dpDecoderPluginMicMute(AUDIO_MUTE_STATE_T mute);
void csrA2dpDecoderConnectL2cap(uint8 content_protection, A2dpPluginTaskdata* task, A2dpPluginConnectParams* codecData);
void MusicConnectAudio(void);
void CsrA2dpDecoderPluginSetHardwareLevels(AUDIO_PLUGIN_DELAY_VOLUME_SET_MSG_T * message);
void CsrA2dpDecoderPluginStartDisconnect(TaskData * task);
void CsrA2dpDecoderPluginSetLevels(AUDIO_PLUGIN_SET_GROUP_VOLUME_MSG_T * VolumeMsg, bool ForceSetVolume);
void CsrA2dpDecoderPluginAllowVolChanges(void);
void CsrA2dpDecoderPluginSubCheckForConnectionFailure(void);
void CsrA2dpDecoderPluginSetAudioLatency (A2dpPluginTaskdata *audio_plugin, uint16 latency);
uint32 CsrA2DPGetDecoderSampleRate(void);
uint32 CsrA2DPGetDecoderSubwooferSampleRate(void);
void CsrA2dpDecoderPluginSetInputAudioMute(const AUDIO_PLUGIN_SET_INPUT_AUDIO_MUTE_MSG_T *mute_message);
uint16 a2dpGetPskeyMaxMismatch(void);
void CsrA2dpDecoderPluginTestReset(void);
void csrA2dpDecoderDspLoadedAndConfigured(A2dpPluginTaskdata* task);
uint16 getI2sAudioInputBitResolution(void);
uint16 getSpdifAudioInputBitResolution(void);
uint16 getAnalogAudioInputBitResolution(void);
uint16 getAptxAudioInputBitResolution(void);


#endif


