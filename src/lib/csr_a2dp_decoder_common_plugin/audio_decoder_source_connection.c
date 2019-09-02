/*
 * audioDecodeSourceConn.c
 *
 * Copyright (c) 2016 Qualcomm Technologies International, Ltd. 
 * Part of ADK 4.1
 *
 *  Created on: 23 Feb 2016
 *      Author: stephenf
 */

#include <source.h>
#include <vmal.h>
#include <print.h>
#include <stdlib.h>

#include "csr_a2dp_decoder_if.h"
#include "csr_a2dp_decoder_common.h"
#include "csr_i2s_audio_plugin.h"
#include "audio_decoder_source_connection.h"
#include "audio_decoder_aptx.h"
#include "csr_a2dp_decoder_common_fm.h"
#include "csr_i2s_audio_plugin.h"

/****************************************************************************
DESCRIPTION
    Returns the sources for a specifed sink type
*/
sources_t audioDecoderGetSources(AUDIO_SINK_T sink_type)
{
    DECODER_t* decoder = CsrA2dpDecoderGetDecoderData();
    sources_t sources;
    int16 i;

    sources.number_of_sources = 1;

    for (i = MAX_SOURCES; i < MAX_SOURCES; i++)
        sources.source[i] = NULL;

    switch(sink_type)
    {
      case AUDIO_SINK_AV :
        {
            sources.source[0] = StreamSourceFromSink(decoder->media_sink);
        }
        break;

        case AUDIO_SINK_USB :
        {
            sources.source[0] = StreamSourceFromSink(decoder->media_sink);
        }
        break;

        case AUDIO_SINK_ANALOG :
        {
            A2dpPluginConnectParams* codecData = (A2dpPluginConnectParams *) decoder->params;
            sources.source[0] = AudioPluginMicSetup(AUDIO_CHANNEL_A, codecData->mic_params->line_a, decoder->rate);
            /* if set for stereo use, disconnect the second input, mic/line B */
            if(decoder->features.stereo)
            {
            /* Configure analogue input B */
                sources.source[1] = AudioPluginMicSetup(AUDIO_CHANNEL_B, codecData->mic_params->line_b, decoder->rate);
                sources.number_of_sources = 2;
            }
        }
        break;

        case AUDIO_SINK_SPDIF :
        {
            sources.source[0] = StreamAudioSource(AUDIO_HARDWARE_SPDIF, AUDIO_INSTANCE_0, SPDIF_CHANNEL_A );
            sources.source[1] = StreamAudioSource(AUDIO_HARDWARE_SPDIF, AUDIO_INSTANCE_0, SPDIF_CHANNEL_B );
            sources.number_of_sources = 2;
        }
        break;

        case AUDIO_SINK_I2S :
        {
            /* Note: Why are the channel slots the wrong way round from convention? */
            sources.source[0] = StreamAudioSource(AUDIO_HARDWARE_I2S, AUDIO_INSTANCE_0, AUDIO_CHANNEL_SLOT_1 );
            sources.source[1] = StreamAudioSource(AUDIO_HARDWARE_I2S, AUDIO_INSTANCE_0, AUDIO_CHANNEL_SLOT_0 );
            sources.number_of_sources = 2;
        }
        break;

        case AUDIO_SINK_FM :
        {
            /* The FM receiver uses the I2S hardware, not FM */
            sources.source[0] = StreamAudioSource(AUDIO_HARDWARE_I2S, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A);
            sources.source[1] = StreamAudioSource(AUDIO_HARDWARE_I2S, AUDIO_INSTANCE_0, AUDIO_CHANNEL_B);
            sources.number_of_sources = 2;
        }
        break;

        case AUDIO_SINK_SCO:
        case AUDIO_SINK_ESCO:
        case AUDIO_SINK_INVALID:
        default :
            PRINT(("DECODER: unsupported source\n"));
            sources.number_of_sources = 0;
        break;

    }
    return sources;
}

/****************************************************************************
DESCRIPTION
    Are we in tws mode?
*/
static bool csrA2dpDecoderIsTwsMode(void)
{
    DECODER_t* decoder = CsrA2dpDecoderGetDecoderData();
    switch (decoder->a2dp_plugin_variant)
    {
        case TWS_SBC_DECODER:
        case TWS_MP3_DECODER:
        case TWS_AAC_DECODER:
        case TWS_APTX_DECODER:
            return TRUE;

        default:
            return FALSE;
    }
}

/****************************************************************************
DESCRIPTION
    Configure an spdif source
*/
static void configureSpdifSource(const Source source)
{
    /* configure the SPDIF interface operating mode, run in auto rate detection mode */
    PanicFalse(SourceConfigure(source, STREAM_SPDIF_AUTO_RATE_DETECT, TRUE));
    PanicFalse(SourceConfigure(source, STREAM_SPDIF_CHNL_STS_REPORT_MODE, TRUE));
	PanicFalse(SourceConfigure(source, STREAM_AUDIO_SAMPLE_SIZE, getSpdifAudioInputBitResolution()));
}

/****************************************************************************
DESCRIPTION
    Sets the operating mode for the a2dp plugin
*/
void audioDecoderSetRelayMode(void)
{
    if (csrA2dpDecoderIsTwsMode())
        csrA2dpDecoderSetStreamRelayMode(RELAY_MODE_TWS_SLAVE);
    else
        csrA2dpDecoderSetStreamRelayMode(RELAY_MODE_NONE);
}

/****************************************************************************
DESCRIPTION
    Ensure AV specific configuration messages are sent
*/
static void generalAVConfig(A2DP_DECODER_PLUGIN_TYPE_T variant, A2dpPluginConnectParams* codec_data, uint32 rate)
{
    switch (variant)
    {
        case APTX_DECODER:
        {
            audioDecoderAptxConfigureAptX(codec_data->channel_mode, rate);
        }
        break;

        case APTX_ACL_SPRINT_DECODER:
        {
            audioDecoderAptxConfigureAptXLL(&codec_data->aptx_sprint_params);
        }
        break;

        case APTXHD_DECODER:
        {
            audioDecoderAptxConfigureAptX(codec_data->channel_mode, rate);
        }
        break;

        case SBC_DECODER:
        case MP3_DECODER:
        case AAC_DECODER:
        case FASTSTREAM_SINK:
        case USB_DECODER:
        case ANALOG_DECODER:
        case SPDIF_DECODER:
        case TWS_SBC_DECODER:
        case TWS_MP3_DECODER:
        case TWS_AAC_DECODER:
        case TWS_APTX_DECODER:
        case FM_DECODER:
        case I2S_DECODER:
        case NUM_DECODER_PLUGINS:
        default:
            break;
    }
}

/****************************************************************************
DESCRIPTION
    If internal routing is required then set sbc encoding parameters
*/
static void setSbcEncodingParams(AUDIO_ROUTE_T audio_route, A2dpPluginConnectParams *codec_data)
{
    switch (audio_route)
    {
        case AUDIO_ROUTE_INTERNAL_AND_RELAY:
            csrA2dpDecoderSetSbcEncoderParams(codec_data->bitpool, codec_data->format);
            break;

        case AUDIO_ROUTE_INTERNAL:
        case AUDIO_ROUTE_I2S:
        case AUDIO_ROUTE_SPDIF:
        default:
            break;
    }
}

/****************************************************************************
DESCRIPTION
    Configure non-source specific features - mostly dsp specific
*/
static void generalConfiguration(local_config_t* local_config)
{
    DECODER_t* decoder = CsrA2dpDecoderGetDecoderData();
    switch(decoder->sink_type)
    {
        case AUDIO_SINK_SPDIF:
        {
            /* Must configure dsp before the sources are configured */
            audioDecodeDspSendSpdifMessages(local_config->codec_data);
            setSbcEncodingParams(decoder->features.audio_input_routing, local_config->codec_data);
            local_config->mismatch |= MUSIC_RATE_MATCH_DISABLE;
            local_config->val_clock_mismatch = 0;
        }
        break;

        case AUDIO_SINK_ANALOG:
        {
            /* disable rate matching for wired and spdif input */
            setSbcEncodingParams(decoder->features.audio_input_routing, local_config->codec_data);
            local_config->mismatch |= MUSIC_RATE_MATCH_DISABLE;
            local_config->val_clock_mismatch = 0;
        }
        break;

        case AUDIO_SINK_USB :
        {
            setSbcEncodingParams(decoder->features.audio_input_routing, local_config->codec_data);
            /* don't apply the stored values to a WIRED input */
            local_config->val_clock_mismatch = 0;
        }
        break;

        case AUDIO_SINK_I2S :
        {
            CsrInitialiseI2SDevice(decoder->rate);
            local_config->val_clock_mismatch = 0;
            local_config->mismatch |= MUSIC_RATE_MATCH_DISABLE;
        }
        break;

        case AUDIO_SINK_FM :
        {
            local_config->val_clock_mismatch = 0;
            local_config->mismatch |= MUSIC_RATE_MATCH_DISABLE;
        }
        break;

        case AUDIO_SINK_AV :
        {
            generalAVConfig(decoder->a2dp_plugin_variant, local_config->codec_data, decoder->rate);
        }
        break;

        default :
            PRINT(("DECODER: unsupported source\n"));
        break;
    }
}


/****************************************************************************
DESCRIPTION
    Configure analog source
*/
static void configureAnalogSource(Source source)
{
    PanicFalse(SourceConfigure(source, STREAM_AUDIO_SAMPLE_SIZE, getAnalogAudioInputBitResolution()));
    /* The ramaining configuration is completed in AudioPluginMicSetup(), which is called when creating sources*/
}


/****************************************************************************
DESCRIPTION
    Call functions to ensure each source type is corretcly configured
*/
static void configureSource(const Source source)
{
    DECODER_t* decoder = CsrA2dpDecoderGetDecoderData();
    switch(decoder->sink_type)
    {
        case AUDIO_SINK_AV :
        case AUDIO_SINK_USB :
            break;

        case AUDIO_SINK_ANALOG:
            configureAnalogSource(source);
            break;

        case AUDIO_SINK_SPDIF:
            /* Must configure sources after the dsp has been configured*/
            configureSpdifSource(source);
            break;

        case AUDIO_SINK_I2S :
            /* configure the I2S interface operating mode, run in master mode */
            CsrI2SConfigureSource(source, decoder->rate, getI2sAudioInputBitResolution());
            break;

        case AUDIO_SINK_FM :
            AudioDecodeFMConfigure(source);
            break;

        default :
            PRINT(("DECODER: unsupported source\n"));
            break;
    }
}

/****************************************************************************
DESCRIPTION
    Iterate through each source and request its configuration
*/
static void configureSources(const sources_t* sources)
{
    uint16 i;
    for (i = 0; i < sources->number_of_sources; i++)
    {
        configureSource(sources->source[i]);
    }
}

/****************************************************************************
DESCRIPTION
    Connect a usb microphone back channel
*/
static void connectUSBBackChannel(A2dpPluginConnectParams *codec_data)
{
    if (codec_data && codec_data->usb_params)
    {
        DECODER_t* decoder = CsrA2dpDecoderGetDecoderData();
        Source source = AudioPluginMicSetup(AUDIO_CHANNEL_A, codec_data->mic_params->mic_a, decoder->rate);
        Source mic_source = audioDecoderGetUsbMicSource();
        Sink mic_sink = audioDecoderGetUsbMicSink();
        PanicNull(StreamConnect(source, mic_sink));
        PanicNull(StreamConnect(mic_source, codec_data->usb_params->usb_sink));
    }
}

/****************************************************************************
DESCRIPTION
    If any backchannel connections are required for a specific sink type then connect them
*/
static void connectBackChannel(AUDIO_SINK_T sink_type, A2dpPluginConnectParams* codec_data)
{
    switch(sink_type)
    {
        case AUDIO_SINK_USB :
        {
            connectUSBBackChannel(codec_data);
        }
        break;

        case AUDIO_SINK_INVALID:
        case AUDIO_SINK_SCO:
        case AUDIO_SINK_ESCO:
        case AUDIO_SINK_AV:
        case AUDIO_SINK_ANALOG:
        case AUDIO_SINK_SPDIF:
        case AUDIO_SINK_I2S:
        case AUDIO_SINK_FM:
        default :
            PRINT(("DECODER: No backchannel for this source\n"));
        break;
    }
}

/****************************************************************************
DESCRIPTION
    Configure and connect all input sources to the appropriate dsp port
*/
void audioDecoderConnectInputSources(AUDIO_SINK_T sink_type, local_config_t* localConfig)
{
    sources_t sources;

    sources = audioDecoderGetSources(sink_type);
    generalConfiguration(localConfig);
    configureSources(&sources);
    audioDecodeConnectSources(&sources, sink_type, localConfig->content_protection);
    connectBackChannel(sink_type, localConfig->codec_data);
}

/******************************************************************************
DESCRIPTION
    Configure content protection for a transform, and then start it.
*/
void audioDecoderStartTransformCheckScms(Transform transform, bool content_protection)
{
    if (transform)
    {
        /* Start or stop SCMS content protection */
        TransformConfigure(transform, VM_TRANSFORM_RTP_SCMS_ENABLE, (uint16)content_protection);

        /* Start the transform decode */
        TransformStart(transform);

        PRINT(("DECODER: Transform started\n"));
    }
}
