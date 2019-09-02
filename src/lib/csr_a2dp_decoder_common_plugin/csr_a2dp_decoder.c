/****************************************************************************
Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    csr_a2dp_decoder_common.c
DESCRIPTION
    plugin implentation which routes the sco audio though the dsp
NOTES
*/


#include <print.h>
#include <kalimba.h>
#include <kalimba_standard_messages.h>
#include <kalimba_if.h>
#include <vmal.h>
#include <file.h>
#include <string.h>
#include <source.h>

#include "csr_a2dp_decoder_if.h"
#include "csr_a2dp_decoder_common.h"
#include "csr_a2dp_decoder_common_if.h"
#include "csr_a2dp_decoder_common_fm.h"
#include "csr_a2dp_decoder_common_sharing.h"
#include "csr_a2dp_decoder_common_cvc_back_channel.h"
#include "csr_a2dp_decoder_common_subwoofer.h"
#include "audio_decoder_aptx.h"
#include "csr_i2s_audio_plugin.h"
#include "audio_decoder_source_connection.h"
#include "gain_utils.h"
#include "audio.h"
#include "csr_i2s_audio_plugin.h"
#ifdef ANC
#include "anc.h"
#endif /* ANC */

#define MUTE_NONE   ((uint16)(0))
#define MUTE_PRIMARY_LEFT   ((uint16)(1<<0))
#define MUTE_PRIMARY_RIGHT  ((uint16)(1<<1))
#define MUTE_SECONDARY_LEFT ((uint16)(1<<2))
#define MUTE_SECONDARY_RIGHT ((uint16)(1<<3))
#define MUTE_WIRED_SUB      ((uint16)(1<<4))

#define MUTE_AUX_LEFT   ((uint16)(1<<0))
#define MUTE_AUX_RIGHT  ((uint16)(1<<1))

#ifdef ANC
/* ports used by ANC */
#define ANC_MIC_A_TO_DSP_PORT   5
#define ANC_MIC_B_TO_DSP_PORT   6
#endif /* ANC */

/* ports used by the low latency gaming applications */
#define CVC_1MIC_PORT           0
#define CVC_2MIC_PORT           1
#define CVC_BACK_CHANNEL_PORT   DSP_OUTPUT_PORT_SUB_ESCO

/* Port used by the DSP to route undecoded audio packets for transmission to another device */
#define DSP_FORWARDING_PORT     DSP_OUTPUT_PORT_RELAY_L2CAP
#define DSP_ESCO_SUB_PORT       DSP_OUTPUT_PORT_SUB_ESCO
#define DSP_L2CAP_SUB_PORT      DSP_OUTPUT_PORT_SUB_L2CAP

/* ports used by the USB audio app */
#define USB_AUDIO_TO_DSP_PORT   0
#define USB_MIC_TO_DSP_PORT     4
#define USB_DSP_TO_MIC_PORT     DSP_OUTPUT_PORT_USB

/* ports used by the wired input including FM input */
#define WIRED_LINE_A_TO_DSP_PORT 0
#define WIRED_LINE_B_TO_DSP_PORT 1

/* a2dp codec to dsp for standard latency apps */
#define CODEC_TO_DSP_PORT        0

/* a2dp codec to dsp for low latency apps */
#define LOW_LATENCY_CODEC_TO_DSP_PORT 2

/* spdif specific constants */

/* enable reporting of status messages from dsp */
#define ENABLE_SPDIF_MESSAGING 1
/* time in seconds before sending status messages */
#define SPDIF_MINIMUM_INACTIVITY_TIME_BEFORE_SENDING_MESSAGE 10

/* allow 250mS for outputs to mute before disconnecting dsp to prevent thuds/clicks */
#define MUTE_DISCONNECT_DELAY_WITH_SUB 250
/* allow 5mS for outputs to mute before disconnecting dsp to prevent thuds/clicks */
#define MUTE_DISCONNECT_DELAY_WITHOUT_SUB 5

/* This Macro is to set Input Sample Width in DSP Register r2 -> Bit 10 (Set 10th Bit if Input Sample Width is 24 Bit) */
#define MESSAGE_SET_SPDIF_CONFIG_MSG_INPUT_SAMPLE_WIDTH ((((getSpdifAudioInputBitResolution() == RESOLUTION_MODE_24BIT) ? 1:0)<<10))


static const uint16 dsp_variant[NUM_DECODER_PLUGINS] =
{
    0,
    DSP_SBC_DECODER,                /* SBC_DECODER              */
    DSP_MP3_DECODER,                /* MP3_DECODER              */
    DSP_AAC_DECODER,                /* AAC_DECODER              */
    DSP_FASTSTREAM_SINK,            /* FASTSTREAM_SINK          */
    DSP_USB_DECODER,                /* USB_DECODER              */
    DSP_APTX_DECODER,               /* APTX_DECODER             */
    DSP_APTX_ACL_SPRINT_DECODER,    /* APTX_ACL_SPRINT_DECODER  */
    DSP_ANALOG_DECODER,             /* WIRED_DECODER            */
    DSP_SPDIF_DECODER,              /* SPDIF_DECODER            */
    DSP_SBC_DECODER,                /* TWS_SBC_DECODER          */
    DSP_MP3_DECODER,                /* TWS_MP3_DECODER          */
    DSP_AAC_DECODER,                /* TWS_AAC_DECODER          */
    DSP_APTX_DECODER,               /* TWS_APTX_DECODER         */
    DSP_SBC_DECODER,                /* FM_DECODER               */
    DSP_I2S_DECODER,                 /* I2S_DECODER              */
    DSP_APTXHD_DECODER              /* APTXHD_DECODER           */
};


static const char* csrA2dpDecoderGetKapFile(A2DP_DECODER_PLUGIN_TYPE_T variant, AUDIO_SINK_T sink_type);
static uint16 estimateLatency (A2DP_DECODER_PLUGIN_TYPE_T variant);
static uint16 getInputAudioBitResolution(void);
static uint16 getOutputAudioBitResolution(void);

/****************************************************************************
DESCRIPTION
    Returns true if in TWS relay mode
*/
static bool is_tws_relay_mode(void)
{
    DECODER_t* decoder = CsrA2dpDecoderGetDecoderData();
    return (decoder && (decoder->stream_relay_mode == RELAY_MODE_TWS_MASTER
            || decoder->stream_relay_mode == RELAY_MODE_TWS_SLAVE));
}

/****************************************************************************
DESCRIPTION
    Load the requested dsp resources
*/
void audioDecodeLoadDSP(void)
{
    const char* kap_file;
    FILE_INDEX file_index;
    DECODER_t* decoder = CsrA2dpDecoderGetDecoderData();
    /* get the filename of the kap file to load */
    kap_file = csrA2dpDecoderGetKapFile(decoder->a2dp_plugin_variant, decoder->sink_type);

    /* attempt to obtain file handle and load kap file, panic if not achieveable */
    file_index = FileFind(FILE_ROOT, kap_file, (uint16)strlen(kap_file));
    PanicFalse(file_index != FILE_NONE);
    PanicFalse(KalimbaLoad(file_index));
}

/****************************************************************************
DESCRIPTION
    Configure the DSP
*/
void audioDecodeConfigureDSP(A2dpPluginTaskdata* task)
{
    UNUSED(task);
    /* update current dsp status */
    SetCurrentDspStatus( DSP_LOADED_IDLE );
}

/****************************************************************************
DESCRIPTION
    Kalimba specific Disconnect sequence
*/
void audioDecodeDisconnectAudio(Task task)
{
    DECODER_t* decoder = CsrA2dpDecoderGetDecoderData();

    /* sub is connected, use longer delay time to alow sub buffers to flush out to prevent pops */
    if ((decoder->stream_relay_mode == RELAY_MODE_TWS_MASTER)||(decoder->stream_relay_mode == RELAY_MODE_TWS_SLAVE))
    {
        /* disconnect immediately when using TWS due to the need to keep the volumes synchronised between
           master and slave devices */
        MessageSend( task, AUDIO_PLUGIN_DISCONNECT_DELAYED_MSG, 0);
    }
    /* not using TWS relay modes so apply soft volume ramp */
    else
    {
        MessageSendLater( task, AUDIO_PLUGIN_DISCONNECT_DELAYED_MSG, 0, MUTE_DISCONNECT_DELAY_WITH_SUB);
    }
}

/****************************************************************************
DESCRIPTION
    Shut down Kalimba dsp
*/
void audioDecodeDisconnectDsp()
{     /* dispose of any remaining messages in the queue */
    DECODER_t* decoder = CsrA2dpDecoderGetDecoderData();
    (void) MessageCancelAll( (TaskData*) decoder->task, MESSAGE_FROM_KALIMBA);

    /* turn off dsp and de-register the Kalimba Task */
    KalimbaPowerOff() ;
    MessageKalimbaTask(NULL);
}

/****************************************************************************
DESCRIPTION
    Translated VM variant id into something the DSP app understands
*/
static uint16 getDspVariant (uint16 variant)
{
    if (variant < NUM_DECODER_PLUGINS)
    {
        return dsp_variant[variant];
    }

    return 0xFFFF;
}

/****************************************************************************
DESCRIPTION
    Configures the DSP to start issuing audio latency measurements
*/
void audioDecodeEnableLatencyReporting (A2dpPluginTaskdata *audio_plugin)
{
    DECODER_t* decoder = CsrA2dpDecoderGetDecoderData();
    if (decoder != NULL)
    {
        uint16 initial_latency;
        A2dpPluginLatencyParams *latencyParams = &((A2dpPluginConnectParams *)decoder->params)->latency;

        if (latencyParams->period)
        {
            if (latencyParams->last)
            {
                initial_latency = (uint16)(latencyParams->last * LATENCY_LAST_MULTIPLIER);
            }
            else if (latencyParams->target)
            {
                initial_latency = (uint16)(latencyParams->target * LATENCY_TARGET_MULTIPLIER);
            }
            else
            {
                initial_latency = estimateLatency(audio_plugin->a2dp_plugin_variant);
            }

            /* Convert latency configuration parameters to single millisecond resolution before sending to DSP */
            PRINT(("DECODER: CONFIGURE_LATENCY_REPORTING Period=%ums, Change=%ums, Initial=%ums\n", (uint16)(latencyParams->period * LATENCY_PERIOD_MULTIPLIER), (uint16)(latencyParams->change * LATENCY_CHANGE_MULTIPLIER), initial_latency));
            KALIMBA_SEND_MESSAGE(CONFIGURE_LATENCY_REPORTING, 1, (uint16)(latencyParams->period * LATENCY_PERIOD_MULTIPLIER), (uint16)(latencyParams->change * LATENCY_CHANGE_MULTIPLIER), initial_latency);
        }
    }
}

/****************************************************************************
DESCRIPTION
    Provides an estimate of audio latency for a specific codec
*/
static uint16 estimateLatency (A2DP_DECODER_PLUGIN_TYPE_T variant)
{
    uint16 latency = 0;

    /* TODO: Temporary values atm, n.b. units are in 1/10th ms. */
    switch (variant)
    {
    case SBC_DECODER:
    case MP3_DECODER:
    case AAC_DECODER:
    case APTX_DECODER:
        latency = 150;
        break;
    case FASTSTREAM_SINK:
    case APTX_ACL_SPRINT_DECODER:
        latency = 45;
        break;
    case APTXHD_DECODER:
        latency = 250;
        break;
    case TWS_SBC_DECODER:
    case TWS_MP3_DECODER:
    case TWS_AAC_DECODER:
    case TWS_APTX_DECODER:
        latency = 300;
        break;
    case USB_DECODER:
    case ANALOG_DECODER:
    case SPDIF_DECODER:
    case FM_DECODER:
    case I2S_DECODER:
    case NUM_DECODER_PLUGINS:
    default:
        break;
    }

    PRINT(("DECODER: estimateLatency variant=%u latency=%ums\n", variant, latency));
    return latency;
}

/****************************************************************************
DESCRIPTION

    Obtains the mute mask value for the Main output group.
    Return Mute mask value.
*/
static uint16 audioGetMainMuteMask(AUDIO_MUTE_STATE_T state)
{
    uint16 mute_mask = MUTE_NONE;
    PRINT(("DECODER: audioGetMainMuteMask \n"));

    if ( state == AUDIO_MUTE_ENABLE)
    {
        mute_mask |= (MUTE_PRIMARY_LEFT | MUTE_PRIMARY_RIGHT |
                      MUTE_SECONDARY_LEFT | MUTE_SECONDARY_RIGHT |
                      MUTE_WIRED_SUB);
    }
    return mute_mask;
}

/****************************************************************************
DESCRIPTION

    Obtains the mute mask value for the Aux output group.
    Return Mute mask value.
*/
static uint16 audioGetAuxMuteMask(AUDIO_MUTE_STATE_T state)
{
    uint16 mute_mask = MUTE_NONE;
    PRINT(("DECODER: audioGetAuxMuteMask \n"));

    if ( state == AUDIO_MUTE_ENABLE)
    {
        mute_mask |= (MUTE_AUX_LEFT | MUTE_AUX_RIGHT);
    }
    return mute_mask;
}


/****************************************************************************
DESCRIPTION

    Obtains a value for audio latency for the specified plugin

    This function can be called before the DSP is loaded.  In which case, the reported
    latency value is an initial estimate based on the audio_plugin provided.

    If the DSP has been loaded and latency reporting has been enabled, then the value reported is
    the last latency value measured by the DSP.

    The latency is reported in units of one tenth of a ms.
*/
bool audioDecodePluginGetLatency (A2dpPluginTaskdata *audio_plugin, bool *estimated, uint16 *latency)
{
    DECODER_t* decoder = CsrA2dpDecoderGetDecoderData();
    PRINT(("DECODER: audioDecodePluginGetLatency plugin=%p ", (void*)audio_plugin));

    if (audio_plugin != NULL)
    {
        if (decoder != NULL)
        {   /* DSP is loaded */
            A2dpPluginLatencyParams *latencyParams = &((A2dpPluginConnectParams *)decoder->params)->latency;

            if ((latencyParams->period) && (latencyParams->last))
            {   /* Latency reporting enabled and DSP has provided a latency value */
                *estimated = FALSE;
                *latency = latencyParams->last;

                PRINT(("OK  estimated=%u latency=%u\n", *estimated, *latency));
                return TRUE;
            }
        }

        /* DSP not currently loaded or has not updated the latency */
        *estimated = TRUE;
        *latency = (uint16)(10 * estimateLatency(audio_plugin->a2dp_plugin_variant));

        PRINT(("OK  estimated=%u latency=%u(1/10th ms)\n", *estimated, *latency));
        return TRUE;
    }

    /* Invalid plugin, return something to indicate an unknown latency */
    *estimated = TRUE;
    *latency = 0xFFFF;

    PRINT(("INVALID  estimated=%u latency=%u\n", *estimated, *latency));
    return FALSE;
}

/****************************************************************************
DESCRIPTION

    Handles notification from DSP that the Stream Relay mode has been updated.

    Result will contain either the mode that has been set or an error code.

    Will issue a further request to change mode if unsuccessful or a new mode has been queued.

    A successful change will update the TWS Audio Routing mode, if appropriate
*/
static void streamRelayModeUpdated (uint16 result)
{
    DECODER_t* decoder = CsrA2dpDecoderGetDecoderData();

    PRINT(("streamRelayModeUpdated  result=%u  required mode=%u\n", result, decoder->stream_relay_mode));

    if (decoder)
    {
        if (result == decoder->stream_relay_mode)
        {
            decoder->relay_mode_change_pending = FALSE;

            if ((decoder->stream_relay_mode == RELAY_MODE_TWS_MASTER) && decoder->routing_mode_change_pending)
            {   /* Only set routing mode if operating as a TWS Master */
                PRINT(("DECODER: Issuing DSP with MESSAGE_SET_TWS_ROUTING=%u,%u\n",decoder->master_routing_mode, decoder->slave_routing_mode));

                KALIMBA_SEND_MESSAGE(MESSAGE_SET_TWS_ROUTING, decoder->master_routing_mode, decoder->slave_routing_mode, 0, 0);
                decoder->routing_mode_change_pending = FALSE;
            }

            if ((decoder->stream_relay_mode == RELAY_MODE_TWS_MASTER) && decoder->sbc_encoder_params_pending)
            {   /* Only set SBC encoder paramsif operating as a TWS Master */
                PRINT(("DECODER: Issuing DSP with SBC Encoder params  bitpool=%u  format=0x%X\n",decoder->sbc_encoder_bitpool, decoder->sbc_encoder_format));

                KALIMBA_SEND_MESSAGE(KALIMBA_MSG_SBCENC_SET_PARAMS, decoder->sbc_encoder_format, 0, 0, 0);
                KALIMBA_SEND_MESSAGE(KALIMBA_MSG_SBCENC_SET_BITPOOL, decoder->sbc_encoder_bitpool, 0, 0, 0);
                decoder->sbc_encoder_params_pending = FALSE;
            }

            if ((decoder->stream_relay_mode == RELAY_MODE_TWS_MASTER) && decoder->device_trims_pending)
            {   /* Only set device trims if operating as a TWS Master */
                PRINT(("DECODER: Issuing DSP with MESSAGE_SET_TWS_DEVICE_TRIMS  master_trim=%i  slave_trim=%i\n",decoder->volume.main.device_trim_master, decoder->volume.main.device_trim_slave));

                KALIMBA_SEND_MESSAGE(MESSAGE_SET_DEVICE_TRIMS, (uint16)decoder->volume.main.device_trim_master, (uint16)decoder->volume.main.device_trim_slave, 0, 0);
                decoder->device_trims_pending = FALSE;
            }

        }
        else
        {
            PRINT(("DECODER: Re-issuing DSP with MESSAGE_SET_RELAY_MODE=%u\n",decoder->stream_relay_mode));

            KALIMBA_SEND_MESSAGE(MESSAGE_SET_RELAY_MODE, decoder->stream_relay_mode, 0, 0, 0);
            decoder->relay_mode_change_pending = TRUE;
        }
    }
}

/****************************************************************************
DESCRIPTION
    This function returns the filename and path for the variant chosen
*/
static const char* csrA2dpDecoderGetKapFile(A2DP_DECODER_PLUGIN_TYPE_T variant, AUDIO_SINK_T sink_type)
{
    /* if using spdif input then use specific dsp app */
    if(sink_type == AUDIO_SINK_SPDIF)
    {
        return "spdif_sink/spdif_sink.kap";
    }
    /* not using wired mode with spdif input */
    else
    {
        /* determine required dsp app based on variant required */
        switch (variant)
        {
        case SBC_DECODER:
        case TWS_SBC_DECODER:
        case FM_DECODER:
            return "sbc_decoder/sbc_decoder.kap";

        case MP3_DECODER:
        case TWS_MP3_DECODER:
            return "mp3_decoder/mp3_decoder.kap";

        case AAC_DECODER:
        case TWS_AAC_DECODER:
            return "aac_decoder/aac_decoder.kap";

        case APTX_DECODER:
        case TWS_APTX_DECODER:
            return "aptx_decoder/aptx_decoder.kap";

        /* aptx and faststream apps are now part of the low latency with optional
           back channel dsp apps, function is selectable at runtime */
        case APTX_ACL_SPRINT_DECODER:
        case FASTSTREAM_SINK:
#ifdef CVC_BACK_CHANNEL
               /* determine which kap file to load, 1 and 2 mic back channels
                   have their own dedicated apps, digital volume control is not
                   available in these apps, 16 steps of volume only */
                if(CsrA2dpDecoderGetDecoderData()->features.use_one_mic_back_channel)
                    return "a2dp_low_latency_1mic/a2dp_low_latency_1mic.kap";
                else if(CsrA2dpDecoderGetDecoderData()->features.use_two_mic_back_channel)
                    return "a2dp_low_latency_2mic/a2dp_low_latency_2mic.kap";
                /* when not using a back channel revert to standard dsp apps without
                   back channel support to give enhanced digital volume control */
                else
#endif
                {
                    if(variant == FASTSTREAM_SINK)
                        return "faststream_decoder/faststream_decoder.kap";
                    else
                        return "aptx_acl_sprint_decoder/aptx_acl_sprint_decoder.kap";
                }

        case APTXHD_DECODER:
            return "aptxhd_decoder/aptxhd_decoder.kap";

        case USB_DECODER:
        case ANALOG_DECODER:
        case SPDIF_DECODER:
        case I2S_DECODER:
        case NUM_DECODER_PLUGINS:
        default:
            Panic();
            return NULL;
        }
    }
}

/****************************************************************************
DESCRIPTION
    Cancle all dsp related messages
*/
void audioDecodeCancelDspMessages(A2dpPluginTaskdata * task)
{
    MessageCancelAll( (TaskData*) task, MESSAGE_FROM_KALIMBA);
    MessageKalimbaTask( (TaskData*)task );
}

/******************************************************************************
DESCRIPTION

    Sends input, processing and output audio bit resolution configuration message
    to the DSP.
*/
void audioDecodeSendBitResolutionMessage(void)
{
    /* Set the resolution mode for input, processing and output */
    uint16 input_audio_resolution = getInputAudioBitResolution();
    uint16 processing_audio_resolution;
    uint16 output_audio_resolution = getOutputAudioBitResolution();

     /* Keep processing audio resolution same as input audio resolution */
     processing_audio_resolution = input_audio_resolution;
     /* Set the resolution mode for input, processing and output */
    KALIMBA_SEND_MESSAGE(MESSAGE_SET_BIT_RESOLUTION, input_audio_resolution, processing_audio_resolution, output_audio_resolution, 0);

}

/******************************************************************************
DESCRIPTION

    Sends input ('CODEC') and output ('DAC') sample rate configuration messages
    to the DSP, taking into account any mismatch.
*/
void audioDecodeSendDspSampleRateMessages(local_config_t* localConfig)
{
    DECODER_t* decoder = CsrA2dpDecoderGetDecoderData();
    A2dpPluginConnectParams * codecData = (A2dpPluginConnectParams *) decoder->params;


#ifdef ANC
    /* If using ANC then we need to give an indication of the sample rate required
       when the ANC mics are connected. */
    uint32 anc_sample_rate = AncGetDacSampleRate();
    uint16 anc_sample_rate_flag = DSP_ANC_SAMPLE_RATE_NONE;

    if (anc_sample_rate == ANC_SAMPLE_RATE_96K)
    {
        PRINT(("DECODER: Using ANC at 96k\n"));
        anc_sample_rate_flag = DSP_ANC_SAMPLE_RATE_96K;
    }
    else if (anc_sample_rate == ANC_SAMPLE_RATE_192K)
    {
        PRINT(("DECODER: Using ANC at 192k\n"));
        anc_sample_rate_flag = DSP_ANC_SAMPLE_RATE_192K;
    }

    /* Set the ANC Sample rate */
    KALIMBA_SEND_MESSAGE(MESSAGE_SET_ANC_MODE, anc_sample_rate_flag, 0, 0, 0);

#endif /* ANC */

    /* determine if there is a subwoofer compiled in the VM application, does not need to
       be connected or streaming at this point */
    if(codecData->sub_functionality_present)
    {
        /* Set the output sampling rate to a fixed rate of48KHz when subwoofer is being used to
           allow resampling of I2S outputs to be used */
        KALIMBA_SEND_MESSAGE(MESSAGE_SET_DAC_SAMPLE_RATE, FIXED_SUBWOOFER_DAC_SAMPLE_RATE/DSP_RESAMPLING_RATE_COEFFICIENT,
                               localConfig->mismatch, localConfig->val_clock_mismatch, 
                               CsrA2dpDecoderPluginIfLowLatencyGetOutputType());

    }
    /* no subwoofer in system */
    else
    {
        /* Set the output sampling rate (DAC/I2S if no resampling required) */
        KALIMBA_SEND_MESSAGE(MESSAGE_SET_DAC_SAMPLE_RATE, decoder->dsp_resample_rate, localConfig->mismatch, localConfig->val_clock_mismatch,
                               CsrA2dpDecoderPluginIfLowLatencyGetOutputType());
    }

    /* Set the codec sampling rate */
    KALIMBA_SEND_MESSAGE(MESSAGE_SET_CODEC_SAMPLE_RATE, (uint16)(decoder->rate/DSP_RESAMPLING_RATE_COEFFICIENT),  16000/DSP_RESAMPLING_RATE_COEFFICIENT, 0, 0);

    }

/****************************************************************************
DESCRIPTION
    Send SPDIF specific Messages
*/
void audioDecodeDspSendSpdifMessages(A2dpPluginConnectParams* codec_data)
{
    /* Send message to DSP to configure the spdif app, the app will then auto detect the sample rate
       and inform the vm app of the current rate via a message which will then be processed by DSP through
       general registers r1,r2,r3,r4 respectively. Bit 7 for r1 was previously in use and is now available. Refer to the module
       $config_spdif_sink_message_handler in spdif_sink.asm for further details on available bits for future reference */
    DECODER_t* decoder = CsrA2dpDecoderGetDecoderData();

    KALIMBA_SEND_MESSAGE(MESSAGE_SET_SPDIF_CONFIG_MSG,
                           (uint16)((decoder->features.spdif_supported_data_types) | (codec_data->spdif_config->target_latency << 8)),
                           ((ENABLE_SPDIF_MESSAGING|(SPDIF_MINIMUM_INACTIVITY_TIME_BEFORE_SENDING_MESSAGE<<1))|MESSAGE_SET_SPDIF_CONFIG_MSG_INPUT_SAMPLE_WIDTH),
                           decoder->dsp_resample_rate,
                           0);


    /* If AC3 is supported then send the configuration message */
    if (decoder->features.spdif_supported_data_types & INPUT_TYPE_AC3)
    {
        KALIMBA_SEND_MESSAGE(MESSAGE_SET_AC3_CONFIG_MSG,
                               codec_data->spdif_ac3_config->configuration_word_1,
                               codec_data->spdif_ac3_config->configuration_word_2,
                               0,
                               0);
    }
}

/****************************************************************************
DESCRIPTION
    Send AptX specific Messages
*/
void audioDecodeSendAptXMessages(uint16 channel_mode, uint32 rate)
{
    /* Send parameters that are to be displayed in Music Manager application */
    PRINT(("aptX: sampling rate=0x%lx channel mode=0x%x \n", rate, channel_mode));
    KALIMBA_SEND_MESSAGE(APTX_PARAMS_MSG, (uint16)rate, channel_mode, 0,0);
}

/****************************************************************************
DESCRIPTION
    Send AptXLL specific Messages
*/
void audioDecodeDspSendAptXLLparams(aptx_sprint_params_type* aptx_sprint_params)
{
/* Send parameters that configure the SRA and buffer settings */
 PRINT(("aptX LL params: initial level=%d target level=%d sra max rate=%d/10000 sra avg time=%d good working buffer level=%d \n",
        aptx_sprint_params->target_codec_level, aptx_sprint_params->initial_codec_level,
        aptx_sprint_params->sra_max_rate, aptx_sprint_params->sra_avg_time,
        aptx_sprint_params->good_working_level));
    KALIMBA_SEND_MESSAGE(MESSAGE_SET_APTX_LL_PARAMS1, aptx_sprint_params->target_codec_level,
                    aptx_sprint_params->initial_codec_level,
                    aptx_sprint_params->sra_max_rate,   /* Third field is scaled by 10000 */
                    aptx_sprint_params->sra_avg_time);
    KALIMBA_SEND_MESSAGE(MESSAGE_SET_APTX_LL_PARAMS2, aptx_sprint_params->good_working_level,
                    0, 0, 0);
}


/****************************************************************************
DESCRIPTION

    Sends configuration messages relating to multi-channel output to the DSP.
    This includes configuring the hardware type of each output, and the I2S
    master/slave mode.

    Note that these messages are not supported by the a2dp_low_latency_1mic or
    a2dp_low_latency_2mic DSP applications, but it is safe to send them anyway
    as the DSP will just ignore messages it doesn't recognise. This avoids the
    need for any complex logic to determine when to send the messages, we just
    send them in all cases.
*/
void audioDecodeSendDspMultiChannelMessages(void)
{
    /* Let the DSP know which outputs are connected to which hardware types */
    AUDIO_OUTPUT_TYPES_T message;

    PanicFalse(AudioOutputGetDspOutputTypesMsg(&message));

    /* Set the hardware type of each output */
    KALIMBA_SEND_MESSAGE(MESSAGE_SET_MULTI_CHANNEL_OUTPUT_TYPES_S, 
                         (uint16)(message.out_type[0]|(message.out_type[1]<<8)),
                         (uint16)(message.out_type[2]|(message.out_type[3]<<8)),
                         (uint16)(message.out_type[4]|0U),
                         (uint16)(message.out_type[5]|(message.out_type[6]<<8)));

    if (AudioOutputI2sActive())
    {
        /* Set the I2S master/slave mode */
        KALIMBA_SEND_MESSAGE(MESSAGE_SET_I2S_CONFIG_MSG, !CsrI2SMasterIsEnabled(), 0, 0, 0);
    }
}

/******************************************************************************
DESCRIPTION

    Returns the Audio input bit resolution.
*/
static uint16 getInputAudioBitResolution(void)
{
    DECODER_t* decoder = CsrA2dpDecoderGetDecoderData();
    uint16 input_audio_resolution = RESOLUTION_MODE_16BIT;

    /* Setting input audio bit resolution */
    switch(decoder->sink_type)
    {
        case AUDIO_SINK_ANALOG:
            {
                switch(decoder->features.audio_input_routing)
                {
                    /* using ADC for audio input */
                    case AUDIO_ROUTE_INTERNAL:
					case AUDIO_ROUTE_INTERNAL_AND_RELAY:
                        {
                            input_audio_resolution = getAnalogAudioInputBitResolution();
                        }
                        break;

                   default:
                    input_audio_resolution = RESOLUTION_MODE_16BIT;
                   break;
                }
            }
            break;

        /* using the I2S hardware for audio input */
        case AUDIO_SINK_I2S:
            {
                input_audio_resolution = getI2sAudioInputBitResolution();
            }
            break;

        case AUDIO_SINK_SPDIF:
            {
                input_audio_resolution = getSpdifAudioInputBitResolution();
            }
            break;

        case AUDIO_SINK_AV:
            {
                input_audio_resolution = getAptxAudioInputBitResolution();
            }
            break;

        default:
            /* All other sink_type works on 16 bit audio resolution input */
            PRINT(("DECODER: Default 16 bit audio resolution \n"));
            input_audio_resolution = RESOLUTION_MODE_16BIT;
        break;
    }

    return input_audio_resolution;
}

/******************************************************************************
DESCRIPTION

    Returns the Audio output bit resolution.
*/
static uint16 getOutputAudioBitResolution(void)
{
    uint16 output_audio_resolution = RESOLUTION_MODE_16BIT;

    if(AudioOutput24BitOutputEnabled())
    {
        output_audio_resolution = RESOLUTION_MODE_24BIT;
    }
    return output_audio_resolution;
}

/******************************************************************************
DESCRIPTION
    Play a tone
*/
void audioDecodePlayTone(A2dpPluginTaskdata *task, const ringtone_note * tone)
{
    DECODER_t* decoder = CsrA2dpDecoderGetDecoderData();
    Source lSource ;
    Sink lSink ;

    if (decoder)
    {
        PRINT(("DECODER: Tone Start\n")) ;

        /* update current tone playing status */
        SetTonePlaying(TRUE);

        /* Configure prompt playback, tone is mono*/
        KALIMBA_SEND_MESSAGE(MESSAGE_SET_TONE_RATE_MESSAGE_ID, 8000 , 0/*Mono Bit 0 =0, TONE BIT 1 = 0*/, 0, 0);

        /* mix the tone via the kalimba tone mixing port */
        lSink = StreamKalimbaSink(TONE_VP_MIXING_DSP_PORT) ;

        /*request an indication that the tone has completed / been disconnected*/
        VmalMessageSinkTask ( lSink , (TaskData*) task ) ;

        /*connect the tone*/
        lSource = StreamRingtoneSource (tone) ;

        /*mix the tone to the SBC*/
        StreamConnectAndDispose( lSource , lSink ) ;
        SetAudioBusy((TaskData *)task);
    }
    /* not valid to play tone at this time, clear busy flag */
    else
    {
        SetAudioBusy(NULL);
    }
}

/******************************************************************************
DESCRIPTION
    Send volume message to Kalimba
*/
void audioDecodeSendDspVolume(audio_output_group_t group, audio_output_gain_t* gain_info)
{
    /* Send the correct volume message for this output group */
    if(group == audio_output_group_main)
    {
        KALIMBA_SEND_MESSAGE(MUSIC_VOLUME_MSG_S, 1, (uint16)gain_info->trim.main.primary_left, (uint16)gain_info->trim.main.primary_right, 0);
        KALIMBA_SEND_MESSAGE(MUSIC_VOLUME_MSG_S, 2, (uint16)gain_info->trim.main.secondary_left, (uint16)gain_info->trim.main.secondary_right, (uint16)gain_info->trim.main.wired_sub);
        KALIMBA_SEND_MESSAGE(MUSIC_VOLUME_MSG_S, 0, (uint16)gain_info->common.system, (uint16)gain_info->common.master, (uint16)gain_info->common.tone);
    }
    else
    {
        KALIMBA_SEND_MESSAGE(MUSIC_VOLUME_AUX_MSG_S, 1, (uint16)gain_info->trim.aux.aux_left, (uint16)gain_info->trim.aux.aux_left, 0);
        KALIMBA_SEND_MESSAGE(MUSIC_VOLUME_AUX_MSG_S, 0, (uint16)gain_info->common.system, (uint16)gain_info->common.master, (uint16)gain_info->common.tone);
    }
}

/****************************************************************************
DESCRIPTION
    Apply mute state to an audio group
*/
void audioDecodeMuteOutput(audio_output_group_t group, AUDIO_MUTE_STATE_T state)
{
    DECODER_t* decoder = CsrA2dpDecoderGetDecoderData();

    PRINT(("DECODER: %smute ", (state == AUDIO_MUTE_DISABLE) ? "un-" : ""));

    if ((group == audio_output_group_main || group == audio_output_group_all))
    {
        PRINT(("main\n"));
        if(decoder->mute_state[audio_mute_group_main] != state)
        {
            KALIMBA_SEND_MESSAGE(MESSAGE_MULTI_CHANNEL_MUTE_MAIN_S, audioGetMainMuteMask(state), 0, 0, 0);
            decoder->mute_state[audio_mute_group_main] = state;
        }
    }
    
    if ((group == audio_output_group_aux || group == audio_output_group_all))
    {
        PRINT(("aux\n"));
        if(decoder->mute_state[audio_mute_group_aux] != state)
        {
            KALIMBA_SEND_MESSAGE(MESSAGE_MULTI_CHANNEL_MUTE_AUX_S, audioGetAuxMuteMask(state), 0, 0, 0);
        }
        decoder->mute_state[audio_mute_group_aux] = state;
    }
}

/******************************************************************************
DESCRIPTION
    handles messages from the dsp
*/
void audioDecodeHandleDspInternalMessage(A2dpPluginTaskdata* task, uint16 id, Message message)
{
    DECODER_t* decoder = CsrA2dpDecoderGetDecoderData();
    A2dpPluginConnectParams *codecData;

    /* determine message id */
    switch (id)
    {
       /* message validity check */
       case MESSAGE_FROM_KALIMBA:
       {
            const DSP_REGISTER_T *m = (const DSP_REGISTER_T *) message;
            PRINT(("DECODER: msg id[%x] a[%x] b[%x] c[%x] d[%x]\n", m->id, m->a, m->b, m->c, m->d));

            switch ( m->id )
            {
                /* indication that the dsp is loaded and ready to accept configuration data */
                case MUSIC_READY_MSG:
                    {
                        if (decoder)
                        {
                            A2DP_DECODER_PLUGIN_TYPE_T variant;
                            codecData = (A2dpPluginConnectParams *) decoder->params;

                            /* Override the variant if using a USB or wired connection */
                            switch(decoder->sink_type)
                            {
                                case AUDIO_SINK_USB:
                                    variant = USB_DECODER;
                                break;

                                case AUDIO_SINK_FM:
                                case AUDIO_SINK_ANALOG:
                                    variant = ANALOG_DECODER;
                                break;

                                case AUDIO_SINK_I2S:
                                     variant = I2S_DECODER;
                                break;

                                case AUDIO_SINK_SPDIF:
                                    variant = SPDIF_DECODER;
                                break;

                                default:
                                    /* Use the (default) variant from task structure */
                                    variant = task->a2dp_plugin_variant;
                                break;
                            }

                            /* Tell the DSP what plugin type is being used */
                            KALIMBA_SEND_MESSAGE(MUSIC_SET_PLUGIN_MSG, getDspVariant(variant), 0, 0, 0);

                            PRINT(("DECODER: Message MUSIC_SET_PLUGIN_MSG variant = %x\n",variant));

                            /* load configuration parameters from ps, different for low latency variants */
                            /* pskey base address */
                            KALIMBA_SEND_MESSAGE(MUSIC_LOADPARAMS_MSG, MUSIC_PS_BASE, 0, 0, 0);

                            if(codecData->silence_threshold)
                            {
                                /* Set silence detection params */
                                PRINT(("DECODER: Message SILENCE_DETECTION_PARAMS_MSG \n"));
                                PRINT(("Threshold %x, Timeout %x\n", codecData->silence_threshold, codecData->silence_trigger_time));
                                KALIMBA_SEND_MESSAGE(MUSIC_SILENCE_DETECTION_PARAMS_MSG, codecData->silence_threshold, codecData->silence_trigger_time, 0, 0);
                            }

                            /* update current dsp status */
                            SetCurrentDspStatus( DSP_LOADED_IDLE );

                            /* hold off setting VM app volume until dsp has full buffer of data to allow
                               a smooth transitional fade in */
                            if ((decoder->stream_relay_mode != RELAY_MODE_TWS_MASTER)&&(decoder->stream_relay_mode != RELAY_MODE_TWS_SLAVE))
                            {
                                /* delay the setting of the volume to allow smooth volume ramping */
                                codecData->delay_volume_message_sending = TRUE;
                            }
                            /* for TWS use case disable the delayed volume changes to maintain TWS volume synchronisation */
                            else
                            {
                                /* don't delay volume changes, smooth volume ramping is disabled for TWS */
                                codecData->delay_volume_message_sending = FALSE;
                            }
                        }
                        else
                        {
                            /* update current dsp status */
                            SetCurrentDspStatus( DSP_ERROR );
                        }
                    }
                    break;

                /* confirmation of the configuration data having been processed, ok to connect audio now */
                case MUSIC_PARAMS_LOADED_MSG:
                    {
                        csrA2dpDecoderDspLoadedAndConfigured(task);
                    }
                    break;

                /* dsp status information gets sent to the vm app */
                case KALIMBA_MSG_SOURCE_CLOCK_MISMATCH_RATE:
                    {
                        if (decoder)
                        {
                            MAKE_AUDIO_MESSAGE_WITH_LEN(AUDIO_PLUGIN_DSP_IND, 1, dsp_ind_message);
                            PRINT(("DECODER: Send CLOCK_MISMATCH_RATE\n"));
                            dsp_ind_message->id = KALIMBA_MSG_SOURCE_CLOCK_MISMATCH_RATE;
                            dsp_ind_message->size_value = 1;
                            dsp_ind_message->value[0] = m->a;
                            MessageSend(decoder->app_task, AUDIO_PLUGIN_DSP_IND, dsp_ind_message);
                        }
                    }
                    break;

                case MUSIC_CUR_EQ_BANK:
                    /* DSP tells plugin which EQ is active.  Send this value to the VM app
                       so the current EQ setting can be restored when the device is repowered.
                    */
                    {
                        if (decoder)
                        {
                            MAKE_AUDIO_MESSAGE_WITH_LEN(AUDIO_PLUGIN_DSP_IND, 1, dsp_ind_message);
                            PRINT(("DECODER: Current EQ setting: [%x][%x]\n", m->a, m->b));
                            dsp_ind_message->id = A2DP_MUSIC_MSG_CUR_EQ_BANK;
                            dsp_ind_message->size_value = 1;
                            dsp_ind_message->value[0] = m->a;
                            MessageSend(decoder->app_task, AUDIO_PLUGIN_DSP_IND, dsp_ind_message);
                        }
                    }
                    break;

                case MUSIC_SETCONFIG_RESP:
                    /* DSP tells plugin which audio enhancements are active.  Send this value
                       to the VM app so the current enhancements setting can be restored when
                       the device is repowered.
                    */
                    {
                        if (decoder)
                        {
                            MAKE_AUDIO_MESSAGE_WITH_LEN(AUDIO_PLUGIN_DSP_IND, 2, dsp_ind_message);
                            PRINT(("DECODER: Enhancements setting: [%x][%x]\n", m->a, m->b));
                            dsp_ind_message->id = A2DP_MUSIC_MSG_ENHANCEMENTS;
                            dsp_ind_message->size_value = 2;
                            dsp_ind_message->value[0] = m->a;
                            dsp_ind_message->value[1] = m->b;
                            MessageSend(decoder->app_task, AUDIO_PLUGIN_DSP_IND, dsp_ind_message);
                        }
                    }
                    break;

                case APTX_SECPASSED_MSG:
                    PRINT(("aptX: Security passed.\n"));
                    KALIMBA_SEND_MESSAGE(APTX_SECURITY_MSG, 1, 0, 0, 0);
                    break;

                case APTX_SECFAILED_MSG:
                  PRINT(("aptX: Security failed.\n"));
                  KALIMBA_SEND_MESSAGE(APTX_SECURITY_MSG, 0, 0, 0, 0);
                  break;

                case MUSIC_SIGNAL_DETECTOR_STATUS_RESP:
                    /* DSP tells when signal detector status has changed
                    Param1 == 0 => no audio - go into standby
                    Param1 == 1 => receiving audio - come out of standby
                    "no audio" message is sent when signal level has been below the
                    threshold level for the trigger time out period "receiving audio"
                    message is sent as soon as a signal is detected above the threshold level
                    */

                    {
                        uint16 signal = m->a;
                        PRINT(("SIGNAL_DETECTOR_STATUS_RESP: PARAM1 %x \n", signal));

                        {
                            MAKE_AUDIO_MESSAGE(AUDIO_PLUGIN_AUDIO_SIGNAL_DETECT_MSG, signal_detect_message);
                            signal_detect_message->signal_detected = signal;
                            MessageSend(decoder->app_task, AUDIO_PLUGIN_AUDIO_SIGNAL_DETECT_MSG, signal_detect_message);
                        }

                        csrA2dpDecoderPluginOutputMute(audio_output_group_all, (signal ? AUDIO_MUTE_DISABLE : AUDIO_MUTE_ENABLE));
                    }
                    break;

                case KALIMBA_MSG_LATENCY_REPORT:
                {   /* DSP has sent us an audio latency measurement */
                    if (decoder)
                    {
                        CsrA2dpDecoderPluginSetAudioLatency(task, m->a);
                    }
                }
                break;

                case KALIMBA_MSG_RELAY_MODE_RESP:
                {   /* DSP has acknowledged the Relay Mode request */
                    PRINT(("KALIMBA_MSG_RELAY_MODE_RESP: %x\n", m->a));
                    streamRelayModeUpdated(m->a);
                }
                break;

                case KALIMBA_MSG_CBOPS_SILENCE_CLIP_DETECT_CLIP_DETECTED_ID:
                {
                    PRINT(("\n\n\n\n\nInput level clipped.\n"));
                }
                break;


                case KALIMBA_MSG_CBOPS_SILENCE_CLIP_DETECT_SILENCE_DETECTED_ID:
                {
                    PRINT(("\n\n\n\n\nInput level silence.\n"));
                }
                break;

                /* message sent from DSP with SPDIF information including the calculated sample rate
                   of current stream, VM doesn't need to do anything with the sample rate as the DSP
                   talks directly to the firmware */
                case KALIMBA_MSG_DSP_SPDIF_EVENT_MSG:
                {
                    uint16 invalid = m->a;

                    /* check whether status information is valid before processing further */
                    if(!invalid)
                    {
                        PRINT(("SPDIF status message sample rate = %x \n", m->b));
                    }
                    else
                    {
                        PRINT(("SPDIF status message is invalid \n"));
                    }
                }
                break;

                case DSP_GAIA_MSG_GET_USER_PARAM_RESP:
                {
                    MAKE_AUDIO_MESSAGE_WITH_LEN(AUDIO_PLUGIN_DSP_GAIA_EQ_MSG, 2, gaia_eq_message);
                    PRINT(("DECODER: User EQ Param from DSP: [%x][%x]\n", m->a, m->b));
                    gaia_eq_message->size_value = 2;
                    gaia_eq_message->value[0] = m->a;
                    gaia_eq_message->value[1] = m->b;
                    MessageSend(decoder->app_task, AUDIO_PLUGIN_DSP_GAIA_EQ_MSG, gaia_eq_message);
                }
                break;

                /* message from DSP when tone or voice prompt has completed playing */
                case MUSIC_TONE_COMPLETE:
                {
                    /* stop tone and clear up status flags */
                    CsrA2dpDecoderPluginToneComplete() ;
                }
                break;

                case MUSIC_CODEC_MSG:
                {
                    /* This message is supported to allow the UFE to temporarily override
                       hardware gain. Force an update to the hardware gain. */
                    AudioOutputGainSetHardwareOnly(audio_output_group_main, VolumeConvertDACGainToDB((int16)m->a));
                    AudioOutputGainSetHardwareOnly(audio_output_group_aux, VolumeConvertDACGainToDB((int16)m->a));
                }
                break;

                default:
                break;
            }
        }
        break;

        case MESSAGE_FROM_KALIMBA_LONG:
        {
            /* recast message as this is a long message from the DSP */
            const uint16 *rcv_msg = (const uint16*) message;

            switch ( rcv_msg[0] )
            {
                case DSP_GAIA_MSG_GET_USER_GROUP_PARAM_RESP:
                    {
                        uint16 i;

                        MAKE_AUDIO_MESSAGE_WITH_LEN(AUDIO_PLUGIN_DSP_GAIA_GROUP_EQ_MSG, rcv_msg[1], gaia_group_eq_message);
                        PRINT(("DECODER: User EQ Group Param from DSP: [%x][%x]...\n", rcv_msg[2],rcv_msg[3]));
                        gaia_group_eq_message->size_value = rcv_msg[1];
                        for(i=0;i<rcv_msg[1];i++)
                            gaia_group_eq_message->value[i] = rcv_msg[i+2];
                        MessageSend(decoder->app_task, AUDIO_PLUGIN_DSP_GAIA_GROUP_EQ_MSG, gaia_group_eq_message);
                    }
                    break;

                case KALIMBA_MSG_EXTERNAL_VOLUME:
                {
                    PRINT(("KALIMBA_MSG_EXTERNAL_VOLUME\n"));
                    if (is_tws_relay_mode())
                    {
                        /* Apply the TWS system gain received from DSP. This is received unsolicited from DSP on TWS slave. On
                           TWS master this is echoed back from DSP to ensure synchronous volume change on both devices. */
                        const audio_output_gain_t* msg = (const audio_output_gain_t*)(&rcv_msg[1]);
                        AudioOutputGainSetHardware(audio_output_group_main, msg->common.master, msg);
                    }
                }
                break;
                default:
                    break;
            }
            break;
        }

        default:
        break ;
    }
}


/****************************************************************************
DESCRIPTION
    DSP is fully configured and ready for operation
*/
void audioDecodeEnableDsp()
{
    PRINT(("DECODER: Send Go message to DSP now\n"));
    KALIMBA_SEND_MESSAGE(KALIMBA_MSG_GO,0,0,0,0);
}

/****************************************************************************
DESCRIPTION
    Connect dsp to multi channel ouptuts
*/
void audioDecodeConnectDspOutputs(audio_output_params_t* params)
{
    AudioOutputAddSourceOrPanic(StreamKalimbaSource(DSP_OUTPUT_PORT_PRI_LEFT), audio_output_primary_left);
    AudioOutputAddSourceOrPanic(StreamKalimbaSource(DSP_OUTPUT_PORT_PRI_RIGHT), audio_output_primary_right);
    AudioOutputAddSourceOrPanic(StreamKalimbaSource(DSP_OUTPUT_PORT_SEC_LEFT), audio_output_secondary_left);
    AudioOutputAddSourceOrPanic(StreamKalimbaSource(DSP_OUTPUT_PORT_SEC_RIGHT), audio_output_secondary_right);
    AudioOutputAddSourceOrPanic(StreamKalimbaSource(DSP_OUTPUT_PORT_AUX_LEFT), audio_output_aux_left);
    AudioOutputAddSourceOrPanic(StreamKalimbaSource(DSP_OUTPUT_PORT_AUX_RIGHT), audio_output_aux_right);
    AudioOutputAddSourceOrPanic(StreamKalimbaSource(DSP_OUTPUT_PORT_SUB_WIRED), audio_output_wired_sub);
    AudioOutputConnectOrPanic(params);
}

/****************************************************************************
DESCRIPTION
    Standby mode
*/
void audioDecodeDspStandby(A2dpPluginModeParams *mode_params)
{
    KALIMBA_SEND_MESSAGE(MUSIC_SETMODE_MSG, MUSIC_SYSMODE_STANDBY, MUSIC_DO_NOT_CHANGE_EQ_BANK, 0, (uint16)((mode_params->external_mic_settings << 1)+(mode_params->mic_mute)));
}

/****************************************************************************
DESCRIPTION
    utility function to set the current EQ operating mode

    @return void
*/
void audioDecodePluginSetEqMode(uint16 operating_mode, A2DP_MUSIC_PROCESSING_T music_processing, A2dpPluginModeParams *mode_params)
{
    /* determine the music processing mode requirements, set dsp music mode appropriately */
    switch (music_processing)
    {
        case A2DP_MUSIC_PROCESSING_PASSTHROUGH:
            {
                KALIMBA_SEND_MESSAGE (MUSIC_SETMODE_MSG , MUSIC_SYSMODE_PASSTHRU , MUSIC_DO_NOT_CHANGE_EQ_BANK, 0, (uint16)((mode_params->external_mic_settings << 1)+(mode_params->mic_mute)) );
                PRINT(("DECODER: Set Music Mode SYSMODE_PASSTHRU\n"));
            }
            break;

        case A2DP_MUSIC_PROCESSING_FULL:
            {
                KALIMBA_SEND_MESSAGE (MUSIC_SETMODE_MSG , operating_mode , MUSIC_DO_NOT_CHANGE_EQ_BANK, 0, (uint16)((mode_params->external_mic_settings << 1)+(mode_params->mic_mute)));
                PRINT(("DECODER: Set Music Mode SYSMODE_FULLPROC\n"));
            }
            break;

        case A2DP_MUSIC_PROCESSING_FULL_NEXT_EQ_BANK:
            {
                KALIMBA_SEND_MESSAGE (MUSIC_SETMODE_MSG , operating_mode , MUSIC_NEXT_EQ_BANK, 0, (uint16)((mode_params->external_mic_settings << 1)+(mode_params->mic_mute)));
                PRINT(("DECODER: Set Music Mode %d and advance to next EQ bank\n", operating_mode));
            }
            break;

        case A2DP_MUSIC_PROCESSING_FULL_SET_EQ_BANK0:
            {
                KALIMBA_SEND_MESSAGE (MUSIC_SETMODE_MSG , operating_mode , MUSIC_SET_EQ_BANK, 0, (uint16)((mode_params->external_mic_settings << 1)+(mode_params->mic_mute)));
                PRINT(("DECODER: Set Music Mode %d and set EQ bank 0\n",operating_mode));
            }
            break;

        case A2DP_MUSIC_PROCESSING_FULL_SET_EQ_BANK1:
            {
                KALIMBA_SEND_MESSAGE (MUSIC_SETMODE_MSG , operating_mode , MUSIC_SET_EQ_BANK, 1, (uint16)((mode_params->external_mic_settings << 1)+(mode_params->mic_mute)));
                PRINT(("DECODER: Set Music Mode %d and set EQ bank 1\n",operating_mode));
            }
            break;

        case A2DP_MUSIC_PROCESSING_FULL_SET_EQ_BANK2:
            {
                KALIMBA_SEND_MESSAGE (MUSIC_SETMODE_MSG , operating_mode , MUSIC_SET_EQ_BANK, 2, (uint16)((mode_params->external_mic_settings << 1)+(mode_params->mic_mute)));
                PRINT(("DECODER: Set Music Mode %d and set EQ bank 2\n",operating_mode));
            }
            break;

        case A2DP_MUSIC_PROCESSING_FULL_SET_EQ_BANK3:
            {
                KALIMBA_SEND_MESSAGE (MUSIC_SETMODE_MSG , operating_mode , MUSIC_SET_EQ_BANK, 3, (uint16)((mode_params->external_mic_settings << 1)+(mode_params->mic_mute)));
                PRINT(("DECODER: Set Music Mode %d and set EQ bank 3\n",operating_mode));
            }
            break;
        case A2DP_MUSIC_PROCESSING_FULL_SET_EQ_BANK4:
            {
                KALIMBA_SEND_MESSAGE (MUSIC_SETMODE_MSG , operating_mode , MUSIC_SET_EQ_BANK, 4, (uint16)((mode_params->external_mic_settings << 1)+(mode_params->mic_mute)));
                PRINT(("DECODER: Set Music Mode %d and set EQ bank 4\n",operating_mode));
            }
            break;

        case A2DP_MUSIC_PROCESSING_FULL_SET_EQ_BANK5:
            {
                KALIMBA_SEND_MESSAGE (MUSIC_SETMODE_MSG , operating_mode , MUSIC_SET_EQ_BANK, 5, (uint16)((mode_params->external_mic_settings << 1)+(mode_params->mic_mute)));
                PRINT(("DECODER: Set Music Mode %d and set EQ bank 5\n",operating_mode));
            }
            break;

        case A2DP_MUSIC_PROCESSING_FULL_SET_EQ_BANK6:
            {
                KALIMBA_SEND_MESSAGE (MUSIC_SETMODE_MSG , operating_mode , MUSIC_SET_EQ_BANK, 6, (uint16)((mode_params->external_mic_settings << 1)+(mode_params->mic_mute)));
                PRINT(("DECODER: Set Music Mode %d and set EQ bank 6\n",operating_mode));
            }
            break;

        default:
            {
                PRINT(("DECODER: Set Music Mode Invalid [%x]\n" , music_processing ));
            }
            break;
    }
}

/****************************************************************************
DESCRIPTION
    utility function to set the current audio enhancments enables

    @return void
*/
void audioDecodePluginUpdateEnhancements(const A2dpPluginModeParams *mode_params)
{
    /* update the enables of any audio enhancements */
    if(mode_params)
    {
        /* if data is valid set the enhancement enables for bass boost and 3d effect, the enables are inverted logic */
        if(mode_params->music_mode_enhancements & MUSIC_CONFIG_DATA_VALID)
        {
            PRINT(("DECODER: Set Audio Enhancements Configuration [%x] inverted [%x]\n" , mode_params->music_mode_enhancements , (~mode_params->music_mode_enhancements & 0x0fff)));
            KALIMBA_SEND_MESSAGE(MUSIC_SETCONFIG , 0 , (MUSIC_CONFIG_USER_EQ_BYPASS|MUSIC_CONFIG_CROSSOVER_BYPASS|MUSIC_CONFIG_BASS_ENHANCE_BYPASS|MUSIC_CONFIG_SPATIAL_ENHANCE_BYPASS), 0, (mode_params->music_mode_enhancements & 0x0fff));
        }
    }
}


/****************************************************************************
DESCRIPTION
    Ste tws routing mode
*/
void a2dpDecoderDspSendTwsRoutingMode(uint16 master_routing, uint16 slave_routing)
{
    /* Only set routing mode if DSP is *currently* operating as a TWS Master */
    KALIMBA_SEND_MESSAGE(MESSAGE_SET_TWS_ROUTING, master_routing, slave_routing, 0, 0);
}

/****************************************************************************
DESCRIPTION
    Set up sbc encoding paramters
*/
void audioDecoderDspSendSbcEncodeParams(uint8 format, uint8 bitpool)
{
    /* Only set routing mode if DSP is *currently* operating as a TWS Master */
    KALIMBA_SEND_MESSAGE(KALIMBA_MSG_SBCENC_SET_PARAMS, format, 0, 0, 0);
    KALIMBA_SEND_MESSAGE(KALIMBA_MSG_SBCENC_SET_BITPOOL, bitpool, 0, 0, 0);
}

/****************************************************************************
DESCRIPTION
    Set up dsp device trims
*/
void audioDecodeDspSendDeviceTrims(int16 device_trim_master, int16 device_trim_slave)
{
    /* Only set routing mode if DSP is *currently* operating as a TWS Master */
    KALIMBA_SEND_MESSAGE(MESSAGE_SET_DEVICE_TRIMS, (uint16)device_trim_master, (uint16)device_trim_slave, 0, 0);
}

/****************************************************************************
DESCRIPTION
    set relay mode
*/
void audioDecodeDspSendRelayMode(unsigned stream_relay_mode)
{
    KALIMBA_SEND_MESSAGE(MESSAGE_SET_RELAY_MODE, (uint16)stream_relay_mode, 0, 0, 0);
}


/****************************************************************************
DESCRIPTION

    Check the version number of the peer device currently connected to,
    earlier 3.0.0 versions of TWS utilise an internal DSP buffering delay of
    250ms whereas versions 3.1.0 and later use a 350ms internal DSP buffering
    delay.
    A message must be sent to the DSP to select between 250 and 350ms internal
    delay to maintain backwards compatibility with previous ADK TWS versions.
*/
void audioDecodeDspSendTwsCompatibilityMode(peer_buffer_level buffer_level_required)
{
    KALIMBA_SEND_MESSAGE(MESSAGE_VM_TWS_COMPATIBILITY_MODE, buffer_level_required, 0, 0, 0);
}

/****************************************************************************
DESCRIPTION
    connect usb input
    Connects l2cap to dsp
    Also connect a usb mic back channel
*/
void audioDecodeDspConnectUSBToDsp(uint16 val_clock_mismatch, Source l_source,
                                    A2dpPluginConnectParams* codecData)
{
    DECODER_t* decoder = CsrA2dpDecoderGetDecoderData();
    UNUSED(val_clock_mismatch);
    /* connect the USB audio to the dsp */
    PanicNull(StreamConnect(StreamSourceFromSink(decoder->media_sink),
              StreamKalimbaSink(USB_AUDIO_TO_DSP_PORT)));
    /* determine if USB has been configured to include a microphone back channel */
    if (codecData)
    {
        /* Don't apply to USB - zero */
        val_clock_mismatch = 0;
        /* If we have a USB Mic */
        if (codecData->usb_params)
        {
            PRINT(("DECODER:         Back channel\n"));
            /* Connect Mic -> KalSnk4, KalSrc2 -> USB(mic)*/
            l_source = AudioPluginMicSetup(AUDIO_CHANNEL_A, codecData->mic_params->mic_a, decoder->rate);
            PanicNull(StreamConnect(l_source, StreamKalimbaSink(USB_MIC_TO_DSP_PORT)));
            PanicNull(StreamConnect(StreamKalimbaSource(USB_DSP_TO_MIC_PORT), codecData->usb_params->usb_sink));
        }
    }
}

/****************************************************************************
DESCRIPTION
    Analog in
    Connects mic to dsp
*/
void audioDecodeDspConnectMicToDsp(Source l_source, Source r_source, A2dpPluginConnectParams* codecData)
{
    DECODER_t* decoder = CsrA2dpDecoderGetDecoderData();
    /* Configure analogue input A */
    l_source = AudioPluginMicSetup(AUDIO_CHANNEL_A, codecData->mic_params->line_a, decoder->rate);
    /* Specify the bits per sample for the audio input */
    PanicFalse(SourceConfigure(l_source, STREAM_AUDIO_SAMPLE_SIZE, getAnalogAudioInputBitResolution() ));
    /* if set for stereo use, configure the second in input, mic/line B */
    if (decoder->features.stereo)
    {
        /* Configure analogue input B */
        r_source = AudioPluginMicSetup(AUDIO_CHANNEL_B, codecData->mic_params->line_b, decoder->rate);
        /* Specify the bits per sample for the audio input */
        PanicFalse(SourceConfigure(r_source, STREAM_AUDIO_SAMPLE_SIZE, getAnalogAudioInputBitResolution() ));

        /* synchronise the two channels */
        PanicFalse(SourceSynchronise(l_source, r_source));
        /* connect mic/line B to the dsp */
        PanicNull( StreamConnect(r_source, StreamKalimbaSink(WIRED_LINE_B_TO_DSP_PORT)));
        PRINT(("DECODER:         Stereo Input\n"));
    }
    /* connect mic/line A to the dsp */
    PanicNull( StreamConnect(l_source, StreamKalimbaSink(WIRED_LINE_A_TO_DSP_PORT)));
}

/****************************************************************************
DESCRIPTION
    I2s Input
    Connects I2s inputs to dsp
*/
uint16 audioDecodeDspConnectI2sToDsp(uint16 val_clock_mismatch, uint16 mismatch)
{
    /* connect the I2S input */
    DECODER_t* decoder = CsrA2dpDecoderGetDecoderData();
    UNUSED(val_clock_mismatch);
    CsrI2SAudioInputConnect(decoder->rate, decoder->features.stereo,
            StreamKalimbaSink(WIRED_LINE_A_TO_DSP_PORT),
            StreamKalimbaSink(WIRED_LINE_B_TO_DSP_PORT),
            getI2sAudioInputBitResolution());
    /* disable rate matching for wired input */
    mismatch |= MUSIC_RATE_MATCH_DISABLE;
    return mismatch;
}

/****************************************************************************
DESCRIPTION
    connect spdif
    Doesn't actually make connection due to bug B-213037
    Connection is made in audioDecodesendSpdifConfigMessages()
*/
uint16 audioDecodeDspConnectSpdifToDsp(Source r_source, Source l_source, uint16 val_clock_mismatch,
                                       uint16 mismatch, A2dpPluginConnectParams* codecData)
{
    DECODER_t* decoder = CsrA2dpDecoderGetDecoderData();
    /* obtain source to SPDIF hardware */
    r_source = StreamAudioSource(AUDIO_HARDWARE_SPDIF, AUDIO_INSTANCE_0, SPDIF_CHANNEL_B);
    l_source = StreamAudioSource(AUDIO_HARDWARE_SPDIF, AUDIO_INSTANCE_0, SPDIF_CHANNEL_A);
    UNUSED(r_source);
    UNUSED(l_source);
    UNUSED(val_clock_mismatch);

    /* when configured for tws audio sharing */
    switch (decoder->features.audio_input_routing)
    {
        case AUDIO_ROUTE_INTERNAL_AND_RELAY:
        { /* Encodes and then decodes audio locally, using SBC.  SBC encoded audio can also be relayed to another device */
            PRINT(("DECODER:     AUDIO_ROUTE_INTERNAL_AND_RELAY  format=0x%X  bitpool=%u\n",codecData->format,codecData->bitpool));
            csrA2dpDecoderSetSbcEncoderParams(codecData->bitpool, codecData->format);
        }
        break;

        default:
            break;
    };
    /* disable rate matching for wired input */
    mismatch |= MUSIC_RATE_MATCH_DISABLE;
    csrA2dpDecoderSetStreamRelayMode(RELAY_MODE_NONE);
    return mismatch;
}

/****************************************************************************
DESCRIPTION
    Disconnect Audio Output
*/
void audioDecodeDisconnectOutput(void)
{
    PanicFalse(AudioOutputDisconnect());
}

/****************************************************************************
DESCRIPTION
    Stop a tone from currently playing
*/
void CsrA2dpDecoderPluginStopTone ( void )
{
    PRINT(("DECODER: Stop Tone\n")) ;

    StreamDisconnect ( 0 , StreamKalimbaSink(TONE_VP_MIXING_DSP_PORT) ) ;
}

/****************************************************************************
DESCRIPTION
    Unregister for tone messages
*/
void audioDecodeUnregisterForToneMsg()
{
    /*we no longer want to receive stream indications*/
    VmalMessageSinkTask(StreamKalimbaSink(TONE_VP_MIXING_DSP_PORT), NULL);
}

/****************************************************************************
DESCRIPTION
    Return the dsp port constant
    Params
        port is either 0 or 1
*/
static uint16 GetDspPort(uint16 port)
{
    DECODER_t* decoder = CsrA2dpDecoderGetDecoderData();
    switch (decoder->sink_type)
    {
      case AUDIO_SINK_AV :
      {
            switch (decoder->a2dp_plugin_variant)
            {
                case APTX_ACL_SPRINT_DECODER :
                case DSP_FASTSTREAM_SINK :
                    return LOW_LATENCY_CODEC_TO_DSP_PORT;

                default :
                    return CODEC_TO_DSP_PORT;
                break;
            }
        }
        break;

        case AUDIO_SINK_USB :
        {
            return USB_AUDIO_TO_DSP_PORT;
        }
        break;

        case AUDIO_SINK_ANALOG :
        {
            return port ? WIRED_LINE_B_TO_DSP_PORT : WIRED_LINE_A_TO_DSP_PORT;
        }
        break;

        case AUDIO_SINK_SPDIF :
        {
            return port ? WIRED_LINE_B_TO_DSP_PORT : WIRED_LINE_A_TO_DSP_PORT;
        }
        break;

        case AUDIO_SINK_I2S :
        {
            return port ? WIRED_LINE_B_TO_DSP_PORT : WIRED_LINE_A_TO_DSP_PORT;
        }
        break;

        case AUDIO_SINK_FM :
        {
            return port ? WIRED_LINE_B_TO_DSP_PORT : WIRED_LINE_A_TO_DSP_PORT;
        }
        break;

        default :
            PRINT(("DECODER: unsupported source\n"));
            return 0;
        break;
    }
    return port;
}

/****************************************************************************
DESCRIPTION
  Connect AV specific sources and sinks
*/
static void connectAudioAV(Source source, bool content_protection)
{
    DECODER_t* decoder = CsrA2dpDecoderGetDecoderData();
    Sink sink = StreamKalimbaSink(GetDspPort(0));
    
    Transform transform = 0;

    switch (decoder->a2dp_plugin_variant)
    {
        case SBC_DECODER :
            transform = VmalTransformRtpSbcDecode(source, sink);
            break;

        case APTX_DECODER :
            transform = audioDecoderAptxConnectAptX(source, sink, content_protection);
            break;

        case MP3_DECODER :
            transform = VmalTransformRtpMp3Decode(source, sink);
            break;

        case AAC_DECODER :
            transform = VmalTransformRtpAacDecode(source, sink);
            break;

        case FASTSTREAM_SINK:
            PanicNull(StreamConnect(source, sink));
            break;

        case APTX_ACL_SPRINT_DECODER:
            transform = audioDecoderAptxConnectAptX(source, sink, content_protection);
            break;

        case APTXHD_DECODER:
            /* AptX HD always uses a rtpdecode transform irrespective of content protection.
                This is entirely different from the standard AptX/AptX-LL.*/
            transform = PanicNull(TransformRtpDecode(source, sink));
            break;

        /* TWS variants do not use transforms*/
        case TWS_SBC_DECODER :
        case TWS_MP3_DECODER :
        case TWS_AAC_DECODER :
        case TWS_APTX_DECODER :
            PanicNull(StreamConnect(source, sink));
            break;

        default :
            Panic();
            break;
    }
    audioDecoderStartTransformCheckScms(transform, content_protection);
}

/****************************************************************************
DESCRIPTION
    Connect a DSP port to the corresponding source in sources. For example
    connecting port 0 will connect sources->source[0] to port 0.
*/
static void audioDecodeConnectKalPort(const sources_t* sources, uint16 port)
{
    Source source;
    Sink sink;
    
    PanicFalse(port < sources->number_of_sources);
    source = PanicNull(sources->source[port]);
    sink = PanicNull(StreamKalimbaSink(GetDspPort(port)));
    PanicNull(StreamConnect(source, sink));
}

/****************************************************************************
DESCRIPTION
    Connect sources of a given type to the DSP. This function will panic if
    the number of sources does not match the expected number of sources for the
    type of audio used.
*/
void audioDecodeConnectSources(const sources_t* sources, AUDIO_SINK_T sink_type, bool content_protection)
{
    DECODER_t* decoder = CsrA2dpDecoderGetDecoderData();
    
    /* If 2 sources then synchronise them */
    if (sources->number_of_sources == 2)
        SourceSynchronise(sources->source[0], sources->source[1]);

    switch (sink_type)
    {
        case AUDIO_SINK_AV :
            /* There should always be one AV source only */
            PanicFalse(sources->number_of_sources == 1);
            /* Connection type depends upon other parameters*/
            connectAudioAV(sources->source[0], content_protection);
            break;

        case AUDIO_SINK_USB:
            audioDecodeConnectKalPort(sources, 0);
            break;
            
        case AUDIO_SINK_ANALOG:
            audioDecodeConnectKalPort(sources, 0);
            if(decoder->features.stereo)
                audioDecodeConnectKalPort(sources, 1);
        break;
        
        case AUDIO_SINK_SPDIF:
        case AUDIO_SINK_I2S:
        case AUDIO_SINK_FM:
            audioDecodeConnectKalPort(sources, 0);
            audioDecodeConnectKalPort(sources, 1);
            break;
            
        case AUDIO_SINK_INVALID:
        case AUDIO_SINK_SCO:
        case AUDIO_SINK_ESCO:
        default :
            Panic();
            break;
    }
}

/****************************************************************************
DESCRIPTION

*/
Sink audioDecoderGetUsbMicSink()
{
    return StreamKalimbaSink(USB_MIC_TO_DSP_PORT);
}

/****************************************************************************
DESCRIPTION

*/
Source audioDecoderGetUsbMicSource()
{
    return StreamKalimbaSource(USB_DSP_TO_MIC_PORT);
}

/*****************************************************************************/
Sink audioDecoderGetDspSink(dsp_sink_type_t type)
{
    switch(type)
    {
#ifdef ANC
        case dsp_sink_anc_mic_a:
            return StreamKalimbaSink(ANC_MIC_A_TO_DSP_PORT);
        
        case dsp_sink_anc_mic_b:
            return StreamKalimbaSink(ANC_MIC_B_TO_DSP_PORT);
#endif
        case dsp_sink_mic_a:
            return StreamKalimbaSink(CVC_1MIC_PORT);
        case dsp_sink_mic_b:
            return StreamKalimbaSink(CVC_2MIC_PORT);
        case dsp_sink_none:
        default:
            return (Sink)NULL;
    }
}

/*****************************************************************************/
Source audioDecoderGetDspSource(dsp_source_type_t type)
{
    switch(type)
    {
        case dsp_source_cvc_back_channel:
            return StreamKalimbaSource(CVC_BACK_CHANNEL_PORT);
        case dsp_source_forwarding:
            return StreamKalimbaSource(DSP_FORWARDING_PORT);
        case dsp_source_esco_sub:
            return StreamKalimbaSource(DSP_ESCO_SUB_PORT);
        case dsp_source_l2cap_sub:
            return StreamKalimbaSource(DSP_L2CAP_SUB_PORT);
        case dsp_source_none:
        default:
            return (Source)NULL;
    }
}

