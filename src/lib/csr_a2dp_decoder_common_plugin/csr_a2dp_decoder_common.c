/****************************************************************************
Copyright (c) 2005 - 2016 Qualcomm Technologies International, Ltd.

FILE NAME
    csr_a2dp_decoder_common.c
DESCRIPTION
    plugin implentation which routes the sco audio though the dsp
NOTES
*/

#include <audio.h>
#include <source.h>
#include <gain_utils.h>
#include <stdlib.h>
#include <panic.h>
#include <print.h>
#include <file.h>
#include <stream.h> /*for the ringtone_note*/

#include <message.h>
#include <ps.h>
#include <transform.h>
#include <string.h>
#include <pio_common.h>
#include <pblock.h>
#include <vmal.h>


#include "csr_i2s_audio_plugin.h"
#include "audio_output.h"
#include "audio_plugin_if.h" /*for the audio_mode*/
#include "audio_plugin_common.h"
#include "csr_a2dp_decoder_common.h"
#include "csr_a2dp_decoder_common_if.h"
#include "csr_a2dp_decoder_if.h"

#include "csr_a2dp_decoder_common_fm.h"
#include "csr_a2dp_decoder_common_subwoofer.h"
#include "csr_a2dp_decoder_common_sharing.h"
#include "csr_a2dp_decoder_common_cvc_back_channel.h"
#include "audio_decoder_source_connection.h"

#ifdef ANC
#include "anc.h"
#endif /* ANC */

/*the synchronous audio data structure*/
static DECODER_t * DECODER = NULL ;

static bool pskey_read = FALSE;
static uint16 val_pskey_max_mismatch = 0;
/*static uint16 val_clock_mismatch = 0;*/

/****************************************************************************
DESCRIPTION

    Test if tws relay mode is enabled
*/
static bool is_tws_relay_mode(void)
{
    return (bool)(DECODER && (DECODER->stream_relay_mode == RELAY_MODE_TWS_MASTER
                  || DECODER->stream_relay_mode == RELAY_MODE_TWS_SLAVE));
}

/****************************************************************************
DESCRIPTION

    Read PSKeys
*/
static void ReadPskey(void)
{
    /* Only need to read the PS Key value once */
    if (!pskey_read)
    {
        if (PsFullRetrieve(PSKEY_MAX_CLOCK_MISMATCH, &val_pskey_max_mismatch, sizeof(uint16)) == 0)
             val_pskey_max_mismatch = 0;
        pskey_read = TRUE;
    }
}

/****************************************************************************
DESCRIPTION

    Workaround for aac sample rate behaviour
*/
static void AacSampleRateWorkaround(A2dpPluginTaskdata *task)
{
    if (((A2DP_DECODER_PLUGIN_TYPE_T)task->a2dp_plugin_variant == AAC_DECODER) ||
        ((A2DP_DECODER_PLUGIN_TYPE_T)task->a2dp_plugin_variant == TWS_AAC_DECODER))
    {
        /* Workaround for AAC+ sources that negotiate sampling frequency at half the actual value */
        if (DECODER->rate < 32000)
            DECODER->rate = DECODER->rate * 2;
    }
}


/****************************************************************************
DESCRIPTION
*/
static void a2dpInitLevels(int16 volume)
{
/*  We use default values to mute volume on A2DP audio connection
    VM application is expected to send volume control right
    after attempting to establish A2DP media connection with the correct
    system and trim volume information along with master and tone volume
*/
    DECODER->volume.group = audio_output_group_all; /* Always update all groups */
    DECODER->volume.main.tone = volume; /* set the initial tones volume level */
    DECODER->volume.main.master = DIGITAL_VOLUME_MUTE; /* -120dB , literally mute */
    DECODER->volume.main.device_trim_master = 0;
    DECODER->volume.main.device_trim_slave = 0;
    DECODER->volume.aux.tone = volume; /* set the initial tones volume level */
    DECODER->volume.aux.master = DIGITAL_VOLUME_MUTE; /* -120dB , literally mute */
            
    DECODER->device_trims_pending = FALSE;
    DECODER->mute_state[audio_mute_group_main] = AUDIO_MUTE_DISABLE;
    DECODER->mute_state[audio_mute_group_aux] = AUDIO_MUTE_DISABLE;
    DECODER->mute_state[audio_mute_group_mic] = AUDIO_MUTE_DISABLE;

    DECODER->external_volume_enabled = FALSE;

    /* keep DAC gain in mute state until ports are connected to prevent pops and clicks */
    DECODER->input_audio_port_mute_active = FALSE;
}

/****************************************************************************
DESCRIPTION
*/
static void a2dpInitTwsParams(void)
{
    DECODER->master_routing_mode = TWS_ROUTING_STEREO;
    DECODER->slave_routing_mode = TWS_ROUTING_STEREO;
    DECODER->routing_mode_change_pending = FALSE;
    DECODER->stream_relay_mode = RELAY_MODE_NONE;
    DECODER->relay_mode_change_pending = FALSE;
}
/****************************************************************************
DESCRIPTION
*/
static void a2dpInitSbcParams(void)
{
    DECODER->sbc_encoder_bitpool = 0;
    DECODER->sbc_encoder_format = 0;
    DECODER->sbc_encoder_params_pending = FALSE;
}
            
static void a2dpInitSubwooferParams(void)
{
    ((A2dpPluginConnectParams *)DECODER->params)->sub_connection_state = sub_state_not_connected;
}

/****************************************************************************
DESCRIPTION

    Helper function to check a2dp plugin variant is of TWS type or not.

    Returns TRUE for TWS plugin otherwise,FALSE.
*/
static bool isTwsVariant(A2DP_DECODER_PLUGIN_TYPE_T variant)
{
    bool result = FALSE;

    switch(variant)
    {
        case TWS_SBC_DECODER:
        case TWS_MP3_DECODER:
        case TWS_AAC_DECODER:
        case TWS_APTX_DECODER:
            result = TRUE; /*Incase of TWS Slave only*/
        break;

        case SBC_DECODER:
        case MP3_DECODER:
        case AAC_DECODER:
        case FASTSTREAM_SINK:
        case USB_DECODER:
        case APTX_DECODER:
        case APTX_ACL_SPRINT_DECODER:
        case ANALOG_DECODER:
        case SPDIF_DECODER:
        case FM_DECODER:
        case I2S_DECODER:
        case APTXHD_DECODER:
        case NUM_DECODER_PLUGINS:
        default:
            result = FALSE;
        break;
    }
    return result;
}

/****************************************************************************
DESCRIPTION
*/
static void ResetDecoderParams(void)
{
    DECODER->rate = 0;
    /* start disconnect by muting output */
    DECODER->volume.main.master = DIGITAL_VOLUME_MUTE;
    DECODER->volume.aux.master  = DIGITAL_VOLUME_MUTE;
    DECODER->volume.main.device_trim_master = 0;
    DECODER->volume.main.device_trim_slave = 0;
    DECODER->device_trims_pending = FALSE;
    DECODER->input_audio_port_mute_active = FALSE;
}

/****************************************************************************
DESCRIPTION

    Sends a latency report to the application
*/
static void sendLatencyReport (Task app_task, A2dpPluginTaskdata *audio_plugin, bool estimated, uint16 latency)
{
    MAKE_AUDIO_MESSAGE(AUDIO_PLUGIN_LATENCY_REPORT, message);
    PRINT(("DECODER: Latency report estimated:%u latency:%ums\n", estimated, latency));
    message->audio_plugin = (Task)audio_plugin;
    message->estimated = estimated;
    message->latency = latency;
    MessageSend(app_task, AUDIO_PLUGIN_LATENCY_REPORT, message);
}

/****************************************************************************
DESCRIPTION

    Helper function to disconnect a Source, flush buffers then close
*/
static void sourceDisconnectFlushClose(Source disconnectSource)
{
    if (disconnectSource)
    {
        /* The calling order of these functions is critical */
        /* Moving StreamDisconnect beneath StreamConnectDispose */
        /* causes B-182712 */
        StreamDisconnect(disconnectSource, 0);

        /* flush buffer */
        StreamConnectDispose(disconnectSource);

        /* disconnect and close */
        SourceClose(disconnectSource);
    }
}

/****************************************************************************
DESCRIPTION

    Stores latency reported by DSP and, if required, informs application
*/
void CsrA2dpDecoderPluginSetAudioLatency (A2dpPluginTaskdata *audio_plugin, uint16 latency)
{
    if (DECODER != NULL)
    {
        A2dpPluginLatencyParams * latencyParams = &((A2dpPluginConnectParams *)DECODER->params)->latency;
        latencyParams->last = BITFIELD_CAST(12, latency);

        if (latencyParams->period || latencyParams->change)
        {
            sendLatencyReport(DECODER->app_task, audio_plugin, FALSE, latency);
        }
    }
}

/******************************************************************************
DESCRIPTION

    Returns the ADC input audio bit resolution.
*/
uint16 getAnalogAudioInputBitResolution(void)
{
    DECODER_t* decoder = CsrA2dpDecoderGetDecoderData();
    uint16 analog_audio_input_resolution = RESOLUTION_MODE_16BIT;

    A2dpPluginConnectParams * codecData = (A2dpPluginConnectParams *) decoder->params;

    if(codecData->mic_params->enable_24_bit_resolution)
    {
        analog_audio_input_resolution = RESOLUTION_MODE_24BIT;
    }
    return analog_audio_input_resolution;
}

/******************************************************************************
DESCRIPTION

    Returns the I2S input audio bit resolution.
*/
uint16 getI2sAudioInputBitResolution(void)
{
    uint16 i2s_audio_input_resolution = RESOLUTION_MODE_16BIT;

    if(CsrI2SIs24BitAudioInputEnabled())
    {
        i2s_audio_input_resolution = RESOLUTION_MODE_24BIT;
    }
    return i2s_audio_input_resolution;
}

/******************************************************************************
DESCRIPTION

    Returns the SPDIF input audio bit resolution.
*/
uint16 getSpdifAudioInputBitResolution(void)
{
    DECODER_t* decoder = CsrA2dpDecoderGetDecoderData();
    uint16 spdif_audio_input_resolution = RESOLUTION_MODE_16BIT;

    A2dpPluginConnectParams * codecData = (A2dpPluginConnectParams *) decoder->params;
    if(codecData->spdif_config->enable_24_bit_audio_input)
    {
        spdif_audio_input_resolution = RESOLUTION_MODE_24BIT;
    }
    return spdif_audio_input_resolution;
}

/******************************************************************************
DESCRIPTION

    Returns the APTX input audio bit resolution.
*/
uint16 getAptxAudioInputBitResolution(void)
{
    DECODER_t* decoder = CsrA2dpDecoderGetDecoderData();
    uint16 aptx_audio_input_resolution = RESOLUTION_MODE_16BIT;
    
    if(decoder->a2dp_plugin_variant == APTXHD_DECODER)
    {
        aptx_audio_input_resolution = RESOLUTION_MODE_24BIT;
    }
    return aptx_audio_input_resolution;
}
/****************************************************************************
DESCRIPTION

    Issues a request to the DSP to change the TWS Audio Routing mode.
    
    The audio routing change will be queued if the DSP is not in the appropriate 
    state to accept a request.
*/
void csrA2dpDecoderSetTwsRoutingMode (uint16 master_routing, uint16 slave_routing)
{
    PRINT(("DECODER: SetTwsRoutingMode  master=%u  slave=%u\n", master_routing, slave_routing));
    if (DECODER != NULL)
    {
        DECODER->master_routing_mode = BITFIELD_CAST(2, master_routing);
        DECODER->slave_routing_mode = BITFIELD_CAST(2, slave_routing);
        DECODER->routing_mode_change_pending = TRUE;

        if ((DECODER->stream_relay_mode == RELAY_MODE_TWS_MASTER) && !DECODER->relay_mode_change_pending)
        {   /* Only set routing mode if DSP is *currently* operating as a TWS Master */
            a2dpDecoderDspSendTwsRoutingMode(master_routing, slave_routing);
            DECODER->routing_mode_change_pending = FALSE;
        }
    }
}


/****************************************************************************
DESCRIPTION

    Issues a request to the DSP to change the SBC Encoder parameters used for the TWS wired modes.
    
    The SBC encoder params change will be queued if the DSP is not in the appropriate 
    state to accept a request.
*/
void csrA2dpDecoderSetSbcEncoderParams (uint8 bitpool, uint8 format)
{
    PRINT(("DECODER: csrA2dpDecoderSetSbcEncoderParams  bitpool=%u  format=0x%X\n", bitpool, format));
    if (DECODER != NULL)
    {
        DECODER->sbc_encoder_bitpool = bitpool;
        DECODER->sbc_encoder_format = format;
        DECODER->sbc_encoder_params_pending = TRUE;
        
        if ((DECODER->stream_relay_mode == RELAY_MODE_TWS_MASTER) && !DECODER->relay_mode_change_pending)
        {   /* Only set routing mode if DSP is *currently* operating as a TWS Master */
            audioDecoderDspSendSbcEncodeParams(format, bitpool);
            DECODER->sbc_encoder_params_pending = FALSE;
        }
    }
}


/****************************************************************************
DESCRIPTION

    Issue DSP request to update TWS device trims if the device is currently the
    master.

    If not the master, saves the trim change for later.
*/
void csrA2dpDecoderSetTWSDeviceTrims (int16 device_trim_master, int16 device_trim_slave)
{
    PRINT(("DECODER: csrA2dpDecoderSetTWSDeviceTrims  master_trim=%i  slave_trim=%i\n", device_trim_master, device_trim_slave));
    if (DECODER != NULL)
    {
        DECODER->volume.main.device_trim_master = device_trim_master;
        DECODER->volume.main.device_trim_slave = device_trim_slave;
        DECODER->device_trims_pending = TRUE;

        if ((DECODER->stream_relay_mode == RELAY_MODE_TWS_MASTER) && !DECODER->relay_mode_change_pending)
        {   /* Only set routing mode if DSP is *currently* operating as a TWS Master */
            audioDecodeDspSendDeviceTrims(device_trim_master, device_trim_slave);
            DECODER->device_trims_pending = FALSE;
        }
    }
}


/****************************************************************************
DESCRIPTION

    Issues a request to the DSP to change the Stream Relay mode.  
    Any outstanding request will cause this new one to be queued.
    
*/
void csrA2dpDecoderSetStreamRelayMode (uint16 mode)
{
    PRINT(("DECODER: SetStreamRelayMode mode=%u\n", mode));

    if (DECODER != NULL)
    {
        PRINT(("DECODER:      last mode=%u  pending=%u\n", DECODER->stream_relay_mode, DECODER->relay_mode_change_pending));

        if (DECODER->stream_relay_mode != mode)
        {   /* Requested mode is different to current/queued mode */
            DECODER->stream_relay_mode = BITFIELD_CAST(2, mode);

            if (!DECODER->relay_mode_change_pending)
            {   /* Not currently updating relay mode, so go ahead and issue a request */
                PRINT(("DECODER: Issuing DSP with MESSAGE_SET_RELAY_MODE=%u\n",DECODER->stream_relay_mode));

                audioDecodeDspSendRelayMode(DECODER->stream_relay_mode);
                DECODER->relay_mode_change_pending = TRUE;
            }
        }
    }
}


/****************************************************************************
DESCRIPTION
*/
DECODER_t * CsrA2dpDecoderGetDecoderData(void)
{
    return DECODER;
}



/****************************************************************************
DESCRIPTION
    This function connects a synchronous audio stream to the pcm subsystem
*/
void CsrA2dpDecoderPluginConnect( A2dpPluginTaskdata *task,
                                  Sink audio_sink ,
                                  AUDIO_SINK_T sink_type,
                                  int16 volume ,
                                  uint32 rate ,
                                  AudioPluginFeatures features ,
                                  AUDIO_MODE_T mode ,
                                  void * params,
                                  Task app_task)
{
    /* create malloc'd copy of audio connect parameters, free'd when audio plugin unloads */
    DECODER = (DECODER_t*)PanicUnlessMalloc (sizeof (DECODER_t) ) ;

    DECODER->task = task;
    DECODER->media_sink = audio_sink ;
    DECODER->a2dp_plugin_variant = task->a2dp_plugin_variant;
    DECODER->forwarding_sink = NULL ;
    DECODER->packet_size = 0;
    DECODER->mode       = mode;
    DECODER->mode_params = 0;
    DECODER->features   = features;
    DECODER->params     = params;
    DECODER->rate       = rate;
    DECODER->app_task   = app_task;
    DECODER->sink_type  = sink_type;
    DECODER->dsp_ports_connected = FALSE;

    a2dpInitLevels(volume);
    a2dpInitTwsParams();
    a2dpInitSbcParams();
    a2dpInitSubwooferParams();

    ReadPskey();

     /* For sinks disconnect the source in case its currently being disposed. */
    StreamDisconnect(StreamSourceFromSink(audio_sink), 0);

    AudioSetActivePlugin((Task)task);

    /* audio busy until DSP returns ready message */
    SetAudioBusy((TaskData*)task);

    /* update current dsp status */
    SetCurrentDspStatus( DSP_LOADING );

    audioDecodeCancelDspMessages(task);

    audioDecodeLoadDSP();
    audioDecodeConfigureDSP(task);

    AacSampleRateWorkaround(task);

    PRINT(("DECODER: CsrA2dpDecoderPluginConnect completed\n"));
}

/****************************************************************************
DESCRIPTION
    Disconnect Sync audio
*/
void CsrA2dpDecoderPluginStartDisconnect(TaskData * task)
{

    PRINT(("DECODER: CsrA2dpDecoderPluginDisconnect start mute\n"));
    /* sample rate no longer valid as plugin is unloading, set to 0 to ensure subwoofer doesn't use it */

    /* ensure nothing interrupts this sequence of events */
    SetAudioBusy((TaskData*) task);    

    ResetDecoderParams();
    /* set mute volume levels */
    CsrA2dpDecoderPluginSetLevels(&DECODER->volume, TRUE);

    audioDecodeDisconnectAudio(task);
}

/****************************************************************************
DESCRIPTION
    Disconnect Sync audio
*/
void CsrA2dpDecoderPluginDisconnect( A2dpPluginTaskdata *task )
{

    A2dpPluginConnectParams *codecData = NULL;

    if (!DECODER)
    {
        PRINT(("DECODER: CsrA2dpDecoderPluginDisconnect, nothing to disconnect\n"));
        return; /* nothing to disconnect */
    }

    codecData = (A2dpPluginConnectParams *) DECODER->params;

    /* disconnect the subwoofer if currently connected */
    CsrA2dpDecoderPluginDisconnectSubwoofer();

    /* if using the microphone or spdif/i2s back channel */

    if(isCodecLowLatencyBackChannel())
    {
        /* check whether microphone back channel needs to be disconnected */
        CsrA2dpDecoderPluginDisconnectMic(codecData);
        /* reset the mic bias pio drive */
        AudioPluginSetMicPio(codecData->mic_params->mic_a, FALSE);
    }

    audioDecodeDisconnectOutput();
    CsrA2dpDisconnectSource(task);

    SetCurrentDspStatus( DSP_NOT_LOADED );
    SetAudioInUse(FALSE);
    SetAudioBusy(NULL);
    AudioSetActivePlugin(NULL);

    free (DECODER);
    DECODER = NULL ;

    PRINT(("DECODER: CsrA2dpDecoderPluginDisconnect completed\n"));
}

static void DisconnectSources(const sources_t sources)
{
    uint8 i;
    for (i = 0; i < sources.number_of_sources; i++)
    {
        sourceDisconnectFlushClose(sources.source[i]);
    }
}


static void audioDecodeCancelMsgs(A2dpPluginTaskdata* task)
{
     /* dispose of any outstanding volume/muting/fading messages */
    (void) MessageCancelAll((TaskData*) task, AUDIO_PLUGIN_ALLOW_VOLUME_CHANGES_MSG);    
    (void) MessageCancelAll((TaskData*) task, AUDIO_PLUGIN_SUBWOOFER_CONNECTION_TIMEOUT_MSG);
    (void) MessageCancelAll((TaskData*) task, AUDIO_PLUGIN_DISCONNECT_DELAYED_MSG);  
    (void) MessageCancelAll((TaskData*) task, AUDIO_PLUGIN_DELAY_VOLUME_SET_MSG);  
}

/****************************************************************************
DESCRIPTION
    This function
*/
void CsrA2dpDisconnectSource(A2dpPluginTaskdata *task)
{
#ifdef ANC
    /* Ensure ANC microphones are disconnected */
    AncDisconnectMics();
#endif /* ANC */

    DisconnectSources(audioDecoderGetSources(DECODER->sink_type));
    /* disconnect forwarding sink if connected */
    CsrA2dpDecoderPluginDisconnectForwardingSink();
    audioDecodeCancelMsgs(task);    
    audioDecodeDisconnectDsp();
}


/****************************************************************************
DESCRIPTION
    function to set the volume levels of the dsp after a preset delay
*/
void CsrA2dpDecoderPluginSetHardwareLevels(AUDIO_PLUGIN_DELAY_VOLUME_SET_MSG_T * message)
{
    PRINT(("DSP Hybrid Delayed Gains: Group = %d Master Gain = %d\n", message->group, message->master));
    AudioOutputGainSetHardware(message->group, message->master, NULL);
}

/****************************************************************************
DESCRIPTION
    Update the volume info stored in DECODER->volume
*/
static void csrA2dpUpdateStoredVolume(AUDIO_PLUGIN_SET_GROUP_VOLUME_MSG_T* volume_msg)
{
    if(volume_msg->group == audio_output_group_main || volume_msg->group == audio_output_group_all)
        DECODER->volume.main = volume_msg->main;

    if(volume_msg->group == audio_output_group_aux || volume_msg->group == audio_output_group_all)
        DECODER->volume.aux = volume_msg->aux;
}

/****************************************************************************
DESCRIPTION
    Determine whether volume levels can be set immediately or whether there
    a pending action must complete before volume can be updated.
*/
static bool csrA2dpDelaySetLevels(AUDIO_PLUGIN_SET_GROUP_VOLUME_MSG_T* volume_msg)
{
    A2dpPluginConnectParams *codecData = (A2dpPluginConnectParams *) DECODER->params;

    /* determine if there is a subwoofer present, if it is present and the media stream is not
       yet present then leave volume in muted state, it will be unmuted when the subwoofer is present */
    if(codecData->sub_is_available && codecData->sub_connection_state == sub_state_not_connected)
    {
        /* a sub is available but it has yet to establish its media channel, hold the app in muted state
        until the sub channel is available and has been connected */
        PRINT(("DSP Set Levels - Sub available - wait for sub media\n"));
        csrA2dpUpdateStoredVolume(volume_msg);
        return TRUE;
    }

    /* if no subwoofer connected and this is an initial volume set message sent from the vm then
    delay the setting of the volume to allow the dsp buffers to fill which allows a smooth
    fade in transistion of the audio, also don't change volume if in the middle of playing a tone otherwise
    a discrepancy will be heard */
    if(IsTonePlaying() || (codecData->sub_is_available == FALSE && codecData->delay_volume_message_sending))
    {
        MAKE_AUDIO_MESSAGE( AUDIO_PLUGIN_SET_GROUP_VOLUME_MSG, message ) ;
         /* create message containing requested volume settings */
        memmove(message, volume_msg, sizeof(AUDIO_PLUGIN_SET_GROUP_VOLUME_MSG_T));
         /* reschedule the volume setting to allow a full buffer of dsp data */
        MessageSendLater ( (TaskData *)DECODER->task , AUDIO_PLUGIN_SET_GROUP_VOLUME_MSG, message, ALLOW_FULL_DSP_BUFFER_DELAY_FOR_SOFT_MUTE) ;
        PRINT(("DSP Set Levels - delay volume set - reschedule main = %d aux = %d\n", DECODER->volume.main.master,
                                                                                      DECODER->volume.aux.master));

        return TRUE;
    }
    return FALSE;
}

/****************************************************************************
DESCRIPTION
    Function to set the hardware volume (used to set hardware volume after
    a delay when using hybrid volume control).
*/
static void csrA2dpSetHardwareGainDelayed(audio_output_group_t group, int16 master_gain, uint16 delay_ms)
{
    TaskData* task = AudioGetActivePlugin();
    if(task)
    {
        MAKE_AUDIO_MESSAGE(AUDIO_PLUGIN_DELAY_VOLUME_SET_MSG, message);
        message->group  = group;
        message->master = master_gain;
        MessageSendLater(task, AUDIO_PLUGIN_DELAY_VOLUME_SET_MSG, message, delay_ms);
    }
    else
    {
        AudioOutputGainSetHardware(group, master_gain, NULL);
    }
}

/****************************************************************************
DESCRIPTION
    Set the volume levels for a group (either main or aux, not all)
*/
static void csrA2dpDecoderPluginSetGroupLevels(audio_output_group_t group, int16 master, int16 tone)
{
    audio_output_gain_t gain_info;

    /* Get the previous gain for this group */
    int16 prev_gain = (group == audio_output_group_main) ? DECODER->volume.main.master :
                                                            DECODER->volume.aux.master;

    if(DECODER->input_audio_port_mute_active)
    {
        PRINT(("Input Mute Active, overriding volume"));
        master = DIGITAL_VOLUME_MUTE;
    }

    PRINT(("%s vol %d dB/60\n", (group == audio_output_group_main) ? "Main" : "Aux", master));

    /* Get the digital gain settings to apply */
    AudioOutputGainGetDigital(group, master, tone, &gain_info);

    switch(AudioOutputGainGetType(group))
    {
        case audio_output_gain_hardware:
        {
            if(is_tws_relay_mode() && (group == audio_output_group_main))
            {
                /* To sync TWS volume changes we use this function to get all gain
                   info (ignoring control type) and send it to the DSP. DSP will 
                   echo it back to VM at a time synchronised between the TWS devices
                   via KALIMBA_MSG_EXTERNAL_VOLUME where we will set hardware gain.
                   This only applies to main output, aux is treated as normal */
                AudioOutputGainGetDigitalOnly(group, master, tone, &gain_info);

                /* Break without setting hardware volume */
                break;
            }

            /* Apply hardware gain immediately for non-TWS usage */
            AudioOutputGainSetHardware(group, master, NULL);
        }
        break;

        case audio_output_gain_hybrid:
        {
            uint16 hw_delay_ms = (master >= prev_gain) ? MIXED_MODE_INCREASING_DELAY : 
                                                         MIXED_MODE_DECREASING_DELAY;
            /* Set hardware gain after a delay (delay is tuned to ensure digital and hardware gains happen simultaneously) */
            csrA2dpSetHardwareGainDelayed(group, master, hw_delay_ms);
        }
        break;

        case audio_output_gain_digital:
        case audio_output_gain_invalid:
        default:
            /* Set hardware gain to fixed level */
            AudioOutputGainSetHardware(group, master, NULL);
        break;
    }

    /* Set digital gain in DSP */
    audioDecodeSendDspVolume(group, &gain_info);
}

/****************************************************************************
DESCRIPTION
    Function to set the volume levels using the appropriate volume control 
    mechanism
*/
void CsrA2dpDecoderPluginSetLevels(AUDIO_PLUGIN_SET_GROUP_VOLUME_MSG_T * volume_msg, bool force_set_volume)
{
    /* Delay setting volume for some cases (unless overridden by caller) */
    if(!force_set_volume && csrA2dpDelaySetLevels(volume_msg))
    {
        /* Return here so volume is not updated */
        return;
    }

    if(isCodecLowLatencyBackChannel())
    {
        /* Applications with CVC back channel do not support digital gain messages */
        CsrA2dpDecoderPluginSetLowLatencyGain(volume_msg->main.master, volume_msg->main.tone);
    }
    else
    {
        /* Set the volume parameters for the main group including TWS per-device trims */
        if(volume_msg->group == audio_output_group_main || volume_msg->group == audio_output_group_all)
        {
            csrA2dpDecoderPluginSetGroupLevels(audio_output_group_main, volume_msg->main.master, volume_msg->main.tone);
            csrA2dpDecoderSetTWSDeviceTrims(volume_msg->main.device_trim_master, volume_msg->main.device_trim_slave);
        }
        /* Set the volume parameters for the aux group */
        if(volume_msg->group == audio_output_group_aux || volume_msg->group == audio_output_group_all)
        {
            csrA2dpDecoderPluginSetGroupLevels(audio_output_group_aux, volume_msg->aux.master, volume_msg->aux.tone);
        }
    }

    /* Update stored volume once level has been set to ensure correct 
       detection of increase/decrease when setting hybrid volume level */
    csrA2dpUpdateStoredVolume(volume_msg);
}

/****************************************************************************
DESCRIPTION
    Indicate the volume has changed
*/
void CsrA2dpDecoderPluginSetVolume(AUDIO_PLUGIN_SET_GROUP_VOLUME_MSG_T *volumeDsp)
{
    if (DECODER && volumeDsp)
    {
        /* set the volume levels according to volume control type */
        CsrA2dpDecoderPluginSetLevels(volumeDsp, FALSE);
    }
}

/****************************************************************************
DESCRIPTION
    reset the volume levels to that stored in the plugin
*/
void CsrA2dpDecoderPluginResetVolume(void)
{
    /* Reset gains following voice prompt */
    CsrA2dpDecoderPluginSetVolume(&DECODER->volume);
}

/****************************************************************************
DESCRIPTION
    Mute/un-mute the input audio port for all audio sources using this plugin except the tones.
*/
void CsrA2dpDecoderPluginSetInputAudioMute(const AUDIO_PLUGIN_SET_INPUT_AUDIO_MUTE_MSG_T *mute_message)
{
    if(DECODER && mute_message)
    {
        DECODER->input_audio_port_mute_active = mute_message->input_audio_port_mute_active;

        /* set dsp volume levels */
        CsrA2dpDecoderPluginSetLevels(&DECODER->volume, TRUE);
    }
}

/******************************************************************************
DESCRIPTION
    Get the AUDIO_MUTE_STATE_T from a mute mask for a group. NB. group must not
    be audio_mute_group_all
*/
static AUDIO_MUTE_STATE_T getMuteState(audio_output_group_t group, uint16 mute_mask)
{
    AUDIO_MUTE_STATE_T state = AUDIO_MUTE_DISABLE;

    PanicFalse(group != audio_output_group_all);
    
    if(mute_mask & AUDIO_MUTE_MASK(group))
        state = AUDIO_MUTE_ENABLE;
        
    return state;
}

/******************************************************************************
DESCRIPTION
    Take a mute mask and apply the setting for the main or aux group.
*/
static void applyMuteMask(audio_output_group_t group, uint16 mute_mask)
{
    /* Can only apply main or aux here */
    PanicFalse(group == audio_output_group_main || group == audio_output_group_aux);

    /* Convert mask to AUDIO_MUTE_STATE_T and apply */
    audioDecodeMuteOutput(group, getMuteState(group, mute_mask));
}

/******************************************************************************
DESCRIPTION
    Set the soft mute state for outputs using the DSP, inputs (microphones) are
    muted using hardware (for low latency back channel DSP applications).
*/
void CsrA2dpDecoderPluginSetSoftMute(AUDIO_PLUGIN_SET_SOFT_MUTE_MSG_T* message)
{
    /* Determine if this is the low latency back channel application using either aptX LL or faststream */
    if (isCodecLowLatencyBackChannel())
    {
        /* Handle separately */
        csrA2dpDecoderPluginSetLowLatencySoftMute(message);
    }
    /* Standard latency codecs including USB and WIRED (analogue/spdif) inputs */
    else
    {
        uint16 mute_mask = message->mute_states;
        /* Apply mute mask for main group. */
        applyMuteMask(audio_output_group_main, mute_mask);

        /* Apply mute mask for aux group. */
        applyMuteMask(audio_output_group_aux, mute_mask);
    }
}

/****************************************************************************
DESCRIPTION
    Set the mute state for the main or aux output group (or both)
*/
void csrA2dpDecoderPluginOutputMute(audio_output_group_t group, AUDIO_MUTE_STATE_T state)
{
    if (isCodecLowLatencyBackChannel())
        csrA2dpDecoderPluginLowLatencyOutputMute(state);
    else
        audioDecodeMuteOutput(group, state);
}

void csrA2dpDecoderModeConnected(A2DP_MUSIC_PROCESSING_T music_processing, A2dpPluginModeParams* mode_params)
{
    /* ensure mode_params has been set before use */
    if (mode_params)
    {
        /* Setup routing mode for both Master and Slave TWS devices */
        csrA2dpDecoderSetTwsRoutingMode(mode_params->master_routing_mode,
                mode_params->slave_routing_mode);

        /* Update DSP mode if necessary */
        audioDecodePluginSetEqMode(MUSIC_SYSMODE_FULLPROC, music_processing,
                mode_params);

        /* update the audio enhancements settings */
        audioDecodePluginUpdateEnhancements(mode_params);
    }
}

/****************************************************************************
DESCRIPTION
    Sets the audio mode
*/
void CsrA2dpDecoderPluginSetMode ( AUDIO_MODE_T mode , const void * params )
{
    /* set the dsp into the correct operating mode with regards to mute and enhancements */
    A2dpPluginModeParams *mode_params = NULL;
    A2DP_MUSIC_PROCESSING_T music_processing = A2DP_MUSIC_PROCESSING_PASSTHROUGH;

    if (!DECODER)
        Panic() ;

    /* mode not already set so set it */
    DECODER->mode = mode;

    /* check whether any operating mode parameters were passed in via the audio connect */
    if (params)
    {
        /* if mode parameters supplied then use these */
        mode_params = (A2dpPluginModeParams *)params;
        music_processing = mode_params->music_mode_processing;
        DECODER->mode_params = mode_params;
    }
    /* no operating mode params were passed in, use previous ones if available */
    else if (DECODER->mode_params)
    {
        /* if previous mode params exist then revert to back to use these */
        mode_params = (A2dpPluginModeParams *)DECODER->mode_params;
        music_processing = mode_params->music_mode_processing;
    }

    /* determine if this is the low latency back channel application using either aptx ll or faststream */
    if(isCodecLowLatencyBackChannel())
    {
        /* Handle separately */
        csrA2dpDecoderPluginSetLowLatencyMode(mode, mode_params, music_processing);
    }
    /* standard latency codecs including USB and WIRED (analogue/spdif) inputs */
    else
    {
        /* determine current operating mode */
        switch (mode)
        {
            case AUDIO_MODE_STANDBY:
            {
                audioDecodeDspStandby(mode_params);
            }
            break;
            
            case AUDIO_MODE_CONNECTED:
            {
                csrA2dpDecoderModeConnected(music_processing, mode_params);
            }
            break;
            case AUDIO_MODE_MUTE_MIC:
            case AUDIO_MODE_MUTE_SPEAKER:
            case AUDIO_MODE_MUTE_BOTH:
            case AUDIO_MODE_UNMUTE_SPEAKER:
            {
                PRINT(("DECODER: *** Muting via SET_MODE_MSG is deprecated ***\n"));
                PRINT(("DECODER: Use SET_SOFT_MUTE_MSG instead\n"));
                Panic();
            }
            break;

            case AUDIO_MODE_LEFT_PASSTHRU:
            case AUDIO_MODE_RIGHT_PASSTHRU:
            case AUDIO_MODE_LOW_VOLUME:
            default:
            {
                PRINT(("DECODER: Set Audio Mode Invalid [%x]\n", mode));
            }
            break;
        }
    }
}


/****************************************************************************
DESCRIPTION
    plays a tone using the audio plugin
*/
void CsrA2dpDecoderPluginPlayTone ( A2dpPluginTaskdata *task, const ringtone_note * tone)
{
    audioDecodePlayTone(task, tone);
}


/****************************************************************************
DESCRIPTION
    Reconnects the audio after a tone has completed
*/
void CsrA2dpDecoderPluginToneComplete ( void )
{
    PRINT(("DECODER: Tone Complete\n")) ;

    /* ensure plugin hasn't unloaded before dsp message was received */
    if(DECODER)
    {
        /*we no longer want to receive stream indications*/
        audioDecodeUnregisterForToneMsg();
    }
    if (IsTonePlaying() || IsVpPlaying())
    {
        /*
         * unblock messages waiting for tone to complete and trigger a voice
         * prompt cleanup if required
         */
        SetTonePlaying(FALSE);
        SetAudioBusy(NULL) ;
    }
}


/****************************************************************************
DESCRIPTION
    handles the internal cvc messages /  messages from the dsp
*/
void CsrA2dpDecoderPluginInternalMessage( A2dpPluginTaskdata *task ,uint16 id , Message message )
{
    /* determine codec type as message id's are different */
    if(isCodecLowLatencyBackChannel())
    {
        /* different message handler for low latency codec types as message id's are different */
        CsrA2dpDecoderPluginLowLatencyInternalMessage(task , id , message);
    }
    /* non low latency codec types use different message id's */
    else
    {
        /* dsp message handler */
        audioDecodeHandleDspInternalMessage(task, id, message);
    }
}

static void csrA2dpDecoderNotifyAppDspReady(A2dpPluginTaskdata* task)
{
    /* send status message to app to indicate dsp is ready to accept data,
     applicable to A2DP media streams only */
    MAKE_AUDIO_MESSAGE(AUDIO_PLUGIN_DSP_READY_FOR_DATA, message);
    message->plugin = (TaskData*) task;
    message->AUDIO_BUSY = AudioBusyTask();
    message->dsp_status = GetCurrentDspStatus();
    message->media_sink = DECODER->media_sink;
    MessageSend(DECODER->app_task, AUDIO_PLUGIN_DSP_READY_FOR_DATA, message);
}

/******************************************************************************
DESCRIPTION
    Dsp has loaded. Connect up system
*/
void csrA2dpDecoderDspLoadedAndConfigured(A2dpPluginTaskdata* task)
{
    A2dpPluginConnectParams *codecData;

    if (DECODER)
    {
        /* send status message to app to indicate dsp is ready to accept data,
           applicable to A2DP media streams only */
        codecData = (A2dpPluginConnectParams *) DECODER->params; 

        /*In case of TWS slave delay volume changes need to be updated */
        if(isTwsVariant(task->a2dp_plugin_variant) == TRUE)
        {
            /*disable delay volume changes in case of TWS*/
            codecData->delay_volume_message_sending = FALSE;
        }

        csrA2dpDecoderNotifyAppDspReady(task);
        SetAudioBusy( NULL ) ;
        PRINT(("DECODER: DECODER_READY \n"));
        /* set initial volume levels to mute, ensure this happens regardless of sub connection state */
        DECODER->volume.main.master = DIGITAL_VOLUME_MUTE;
        DECODER->volume.aux.master  = DIGITAL_VOLUME_MUTE;
        CsrA2dpDecoderPluginSetLevels(&DECODER->volume, TRUE);
        /* connect the dsp ports to the audio streams */
        MusicConnectAudio ();
        /* update current dsp status */
        SetCurrentDspStatus( DSP_RUNNING );
        /* If correctly configured, turn on latency reporting */
        audioDecodeEnableLatencyReporting(task);
        /* disable soft volume ramping for TWS as needs too maintain volume level syncronisation
           between master and slave units */
        if ((DECODER->stream_relay_mode != RELAY_MODE_TWS_MASTER)&&(DECODER->stream_relay_mode != RELAY_MODE_TWS_SLAVE))
        {
            /* send message to release the lock on processing VM volume changes */
            MessageSendLater((TaskData *)DECODER->task, AUDIO_PLUGIN_ALLOW_VOLUME_CHANGES_MSG, 0 , ALLOW_FULL_DSP_BUFFER_DELAY_FOR_SOFT_MUTE);
            /* set a subwoofer port connection failure timeout */
            MessageSendLater((TaskData *)DECODER->task, AUDIO_PLUGIN_SUBWOOFER_CONNECTION_TIMEOUT_MSG, 0 , SUBWOOFER_CONNECTION_FAILURE_TIMEOUT);
        }
    }
    else
    {
        /* update current dsp status */
        SetCurrentDspStatus( DSP_ERROR );
    }
}

/****************************************************************************
DESCRIPTION

    @return void
*/
static void InitMultiChannelParams(audio_output_params_t* mch_params)
{
    A2dpPluginConnectParams *codecData = (A2dpPluginConnectParams *) DECODER->params;
    memset(mch_params, 0, sizeof(audio_output_params_t));

    /* determine if there is a subwoofer compiled in the VM application, does not need to
       be connected or streaming at this point */
    if(codecData->sub_functionality_present)
    {
        /* Set the output sampling rate to a fixed rate of 48KHz when subwoofer is being used to
           allow resampling of I2S outputs to be used */
        mch_params->sample_rate = FIXED_SUBWOOFER_DAC_SAMPLE_RATE;
        /* disable the output rate resampling */
        mch_params->disable_resample = TRUE;
    }
    else
    {
        /* Consider output sampling rate set in case of wired audio routing (analog, I2S and SPDIF) except USB */
        if((DECODER->sink_type == AUDIO_SINK_ANALOG) ||(DECODER->sink_type == AUDIO_SINK_I2S) ||(DECODER->sink_type == AUDIO_SINK_SPDIF))
        {
            /* Set the output sampling rate to that of the configured output rate ?? need to take care of USB and SPDIF here??*/
            mch_params->sample_rate = codecData->wired_audio_output_rate;
        }
        else
        {
            /* Set the output sampling rate to that of the codec rate, no resampling */
            mch_params->sample_rate = DECODER->rate;
        }
    }
}

/****************************************************************************
DESCRIPTION
    Connect the encoded audio input and pcm audio output streams
*/
void MusicConnectAudio (void)
{
    local_config_t* local_config = malloc(sizeof(local_config_t));
    memset(local_config, 0, sizeof(local_config_t));
    local_config->codec_data = (A2dpPluginConnectParams *) DECODER->params;
    /* Set up multi-channel parameters */
    InitMultiChannelParams(&(local_config->mch_params));

    /* Store adjusted sample rate returned from multi-channel plugin */
    DECODER->dsp_resample_rate = (uint16)(AudioOutputGetSampleRate(&local_config->mch_params)/DSP_RESAMPLING_RATE_COEFFICIENT);

    /* initialise the content protection and clock mismatch settings from pstore if available */
    if(local_config->codec_data != NULL)
    {
        DECODER->packet_size = BITFIELD_CAST(10, local_config->codec_data->packet_size);
        local_config->val_clock_mismatch = local_config->codec_data->clock_mismatch;
        local_config->content_protection = local_config->codec_data->content_protection;
        if (local_config->codec_data->mode_params != NULL)
            local_config->mode_params = local_config->codec_data->mode_params;
    }

    audioDecoderConnectInputSources(DECODER->sink_type, local_config);

#ifdef ANC
    /* Attempt to connect ANC microphones */
    AncConnectMics(audioDecoderGetDspSink(dsp_sink_anc_mic_a), 
                   audioDecoderGetDspSink(dsp_sink_anc_mic_b));
#endif /* ANC */

    /* Connect up DSP output ports to hardware sinks */
    audioDecodeConnectDspOutputs(&(local_config->mch_params));

    /* For the low latency apps, connect the CVC back channel if 1 or two mic back channel has been defined */
    if (isCodecLowLatencyBackChannel())
    {
        /* connect the one or two mics for back channel operation, if two mic is not defined it is assummed 1 mic */
        CsrA2dpDecoderConnectBackChannel(local_config->codec_data, DECODER->features.use_two_mic_back_channel);
    }

    /* check to see if the subwoofer can be connected */
    CsrA2dpDecoderPluginConnectSubwoofer(local_config->codec_data);

    /* DSP port now connected, ok to set DAC_GAIN, this eliminates the pop/clicks associated with port connection */
    DECODER->dsp_ports_connected = TRUE;

    CsrA2dpDecoderPluginSetMode(DECODER->mode, local_config->mode_params);
    /* SPDIF sample rate messages must be sent before the sources are configured, so have already been sent*/
    if (DECODER->sink_type != AUDIO_SINK_SPDIF)
        audioDecodeSendDspSampleRateMessages(local_config);

    audioDecodeSendDspMultiChannelMessages();
    audioDecodeSendBitResolutionMessage();
    audioDecoderSetRelayMode();

    audioDecodeEnableDsp();

    /* Update the current audio state */
    SetAudioInUse(TRUE);
    free(local_config);
}

/****************************************************************************
DESCRIPTION
    @Param  state
            TRUE: Mic will be muted
            FALSE : Unmute Mic
    @return void
*/
void csrA2dpDecoderPluginMicMute(AUDIO_MUTE_STATE_T state)
{
    A2dpPluginConnectParams* codecData = (A2dpPluginConnectParams*)DECODER->params;

    if (codecData == NULL)
        return;

    if (DECODER->mute_state[audio_mute_group_mic] != state)
    {
        bool digital = (DECODER->sink_type == AUDIO_SINK_ANALOG ? FALSE : codecData->mic_params->mic_a.digital);
        Source src = AudioPluginGetMic(AudioPluginGetInstance(codecData->mic_params->mic_a), AUDIO_CHANNEL_A, digital);

        PRINT(("DECODER: Set Mic Mute to [%x]\n", state));

        /* update the muteEnable status , according to mute value */
        if(src)
        {
            SourceConfigure(src, STREAM_AUDIO_MUTE_ENABLE, state);
        }
        if(DECODER->features.use_two_mic_back_channel)
        {
            src = AudioPluginGetMic(AudioPluginGetInstance(codecData->mic_params->mic_b),
                                    AUDIO_CHANNEL_B, codecData->mic_params->mic_b.digital);

            /* Do mute/unmute on channel B */
            if(src)
            {
                SourceConfigure(src, STREAM_AUDIO_MUTE_ENABLE, state);
            }
        }

        DECODER->mute_state[audio_mute_group_mic] = state;
    }
}

/****************************************************************************
DESCRIPTION
    Utility function to obtain current plugin sample rate 

    @return current sample rate
*/
uint32 CsrA2DPGetDecoderSampleRate(void)
{
    /* if a2dp plugin loaded and rate is valid */
    if((DECODER)&&(DECODER->rate))
    {
        /* return current sample rate */
        PRINT(("DECODER: Sample Rate = %ld , decoder = %d\n",DECODER->rate,DECODER->sink_type));
        return DECODER->rate;
    }
    /* not yet loaded, sample rate unavailable so return 0 */
    else
    {
        PRINT(("DECODER: Sample Rate Not Available\n"));
        return 0;
    }
}

/****************************************************************************
DESCRIPTION
    Utility function to obtain current plugin sample rate for subwoofer use

    @return current sample rate
*/
uint32 CsrA2DPGetDecoderSubwooferSampleRate(void)
{
    /* Subwoofer rate is now fixed at 48KHz to prevent issues with resampling 
       I2S outputs */
    return 48000;
}


/****************************************************************************
DESCRIPTION
    utility function to reset the VM volume change request block now that the 
    dsp has a full buffer of data 

    @return void
*/
void CsrA2dpDecoderPluginAllowVolChanges(void)
{
    A2dpPluginConnectParams *codecData = (A2dpPluginConnectParams *) DECODER->params;

    /* ensure plugin loaded and we are not waiting for the subwoofer to connect */
    if((DECODER)&&(codecData->sub_is_available == FALSE))
    {
        PRINT(("DECODER: Allow Volume Changes\n"));
        codecData->delay_volume_message_sending = FALSE;
    }
    else
    {
        PRINT(("DECODER: Allow Volume Changes Ignored Due To Sub\n"));
    }
}

/****************************************************************************
DESCRIPTION
    utility function to check whether the subwoofer port got connected within the 
    5 second timeout period, if it didn't the audio will still be in muted state
    so will need to be unmuted 

    @return void
*/
void CsrA2dpDecoderPluginSubCheckForConnectionFailure(void)
{
    /* ensure plugin loaded */
    if(DECODER)
    {
        A2dpPluginConnectParams *codecData = (A2dpPluginConnectParams *) DECODER->params;

        /* check whether subwoofer is available, if not exit */
        if((codecData->sub_is_available == TRUE)&&(codecData->sub_connection_state == sub_state_not_connected))
        {
            PRINT(("DECODER: SUB Failed to Connect, unmute\n"));

            /* a sub is available but it has failed to establish its media channel, 
               unmute audio and set a failed sub channel to allow volume changes to 
               get processed */       
            codecData->sub_connection_state = sub_state_failed;

            /* release tone playing/volume setting lock */
            codecData->delay_volume_message_sending = FALSE;

            /* set volume levels */
            CsrA2dpDecoderPluginSetLevels(&DECODER->volume, TRUE);
        }
    }
}

/****************************************************************************
DESCRIPTION
    Accessor for val_pskey_max_mismatch
*/
uint16 a2dpGetPskeyMaxMismatch(void)
{
    return val_pskey_max_mismatch;
}

/****************************************************************************
DESCRIPTION
    Reset any static variables
    This is only intended for unit test and will panic if called in a release build.
*/
void CsrA2dpDecoderPluginTestReset(void)
{
#ifndef AUDIO_TEST_BUILD
    Panic();
#else
    pskey_read = FALSE;
    val_pskey_max_mismatch = 0;
    if (DECODER)
    {
        memset(DECODER, 0, sizeof(DECODER_t));
        free(DECODER);
        DECODER = NULL;
    }
#endif
}

