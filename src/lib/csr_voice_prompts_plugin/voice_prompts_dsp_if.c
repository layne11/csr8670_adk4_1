/****************************************************************************
Copyright (c) 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.1
 
FILE NAME
    voice_prompts_dsp_if.c
 
DESCRIPTION
    Monolithic DSP implementation of voice prompt plug-in.
*/

#include <stdlib.h>
#include <string.h>

#include <kalimba.h>
#include <kalimba_standard_messages.h>
#include <kalimba_if.h>
#include <gain_utils.h>
#include <file.h>
#include <transform.h>
#include <sink.h>
#include <print.h>

#include "audio.h"
#include "audio_plugin_if.h"
#include "audio_output.h"
#include "csr_i2s_audio_plugin.h"

#include "csr_voice_prompts.h"
#include "voice_prompts_utils.h"
#include "voice_prompts_dsp_if.h"
#include "voice_prompts_no_dsp.h"

#ifdef ANC
#include "anc.h"
#define ANC_MIC_A_DSP_PORT   5
#define ANC_MIC_B_DSP_PORT   6
#endif

#define FORCED_TONE_PLAYBACK_RATE   (48000)

/* DSP message structure */
typedef struct
{
    uint16 id;
    uint16 a;
    uint16 b;
    uint16 c;
    uint16 d;
} DSP_REGISTER_T;

/* Decoder type to send to DSP app */
typedef enum
{
    PHRASE_NO_DECODER = 0,
    PHRASE_SBC_DECODER = 1,
    PHRASE_MP3_DECODER = 2,
    PHRASE_AAC_DECODER = 3
} PHRASE_DECODER_T;

extern const TaskData csr_voice_prompts_plugin;

static void playStandalone(vp_context_t *context);
static void mixWithOtherPlugin(vp_context_t *context);
static void stopPhraseStandalone(void);
static void stopPhraseMixable(void);

static void loadDsp(voice_prompts_codec_t codec_type);
static bool connectPromptSourceToDsp(voice_prompts_codec_t codec_type, Source prompt_source);
/* used by Kalimba message handler */
static void handleMusicReadyMessage(vp_context_t *context);

static void setVolume(int16 prompt_volume, bool using_tone_port);
static void sendDspSampleRateMessages(voice_prompts_codec_t codec_type, bool is_stereo,
        unsigned playback_rate, unsigned resample_rate_with_coefficient_applied);
static void sendDspMultiChannelMessages(void);
/* used by mixVoicePrompt() */
static void setupRunningDspToPlayAudioPrompt(unsigned playback_rate, bool is_stereo);
static void setupRunningDspToPlayTone(unsigned playback_rate);

static PHRASE_DECODER_T getDecoderType(voice_prompts_codec_t codec_type);
static const char* getKapFileName(PHRASE_DECODER_T decoder);
static bool isOtherPluginPresent(void);

/*******************************************************************************
DESCRIPTION
    Entry point for DSP specific playback of tones and voice prompts.
*/
void VoicePromptsDspPlay(vp_context_t *context)
{
    if(!VoicePromptsIsCompressed(context->codec_type))
    {
        /* If DSP is already running, the voice prompt can be mixed with the DSP audio
           via the kalimba mixing port (3), either the CVC plugin or the music plugin
           will have already determined the output source and connected the appropriate
           ports to the hardware, and the volume will already have been set. */
        if(isOtherPluginPresent())
        {
            mixWithOtherPlugin(context);
        }
        /* DSP not currently loaded, cannot mix */
        /* Load the DSP to playback this prompt if we are in multi-channel mode, as filtering / aux output may be required. */
        else if(!VoicePromptsIsSampleRateFixedAt8khz(context->codec_type) ||
                VoicePromptsNoDspIsDspRequiredForPlayback(context->features.force_resampling_of_tones))
        {
            PRINT(("VP: Use DSP for prompt (multi-channel)\n"));
            playStandalone(context);
        }
        /* Play the prompt directly without the DSP */
        else
        {
            context->duplicate_source = VoicePromptsNoDspObtainAnotherPromptSource(context);
            VoicePromptsNoDspPlay(context->codec_type, context->playback_rate, context->prompt_volume,
                    context->source, context->duplicate_source);
        }
    }
    else
    {
        /* For SBC, AAC or MP3 voice prompts, it is necessary to load the DSP with the appropriate application
           to perform the decoding */
        PRINT(("VP: Play DSP prompt\n"));
        playStandalone(context);
    }
}

/*******************************************************************************
DESCRIPTION
    Stop of tone or voice prompt. It is used for both forced stop and
    for handling end of voice prompt message.
    Triggers CsrVoicePromptsPluginCleanup().
*/
void VoicePromptsDspStop(void)
{
    SetAudioBusy(NULL);
}

/*******************************************************************************
DESCRIPTION
    Tells upper layer if closing down procedure should continue.
    Continue only if voice prompt plugin is in control of hardware outputs.
*/
bool VoicePromptsDspShallHandleStreamDisconnect(vp_context_t *context)
{
    if(context)
    {
        return !context->mixing;
    }
    else
    {
        return FALSE;
    }
}

/*******************************************************************************
DESCRIPTION
    Nothing to do for monolithic DSP.
*/
void VoicePromptsDspPrepareForClose(void)
{

}

/*******************************************************************************
DESCRIPTION
    Select clean up variant depending on presence of other plugin.
*/
void VoicePromptsDspCleanup(vp_context_t *context)
{
    if(context->mixing)
    {
        /* If DSP already loaded and the prompt was mixed */
        stopPhraseMixable();
    }
    else
    {
        /* DSP not previously loaded or the prompt was not mixable type*/
        stopPhraseStandalone();
    }
}

/*******************************************************************************
DESCRIPTION
    Helper function to determine if messages was sent by DSP.
*/
bool VoicePromptsIsItDspMessage(MessageId id)
{
    return (id == MESSAGE_FROM_KALIMBA) ? TRUE : FALSE;
}

/*******************************************************************************
DESCRIPTION
    Message handler for DSP messages.
*/
void VoicePromptsDspMessageHandler(vp_context_t *context, Task task, Message message)
{
    const DSP_REGISTER_T *m = (const DSP_REGISTER_T *) message;
    PRINT(("handleKalimbaMessage: msg id[%x] a[%x] b[%x] c[%x] d[%x]\n", m->id, m->a, m->b, m->c, m->d));

    UNUSED(task);
    switch ( m->id )
    {
        case MUSIC_READY_MSG:
        {
            PRINT(("VP: KalMsg MUSIC_READY_MSG\n"));
            handleMusicReadyMessage(context);
        }
        break;

        case MUSIC_CODEC_MSG:
        break;

        case MUSIC_TONE_COMPLETE:
        {
            PRINT(("VP: KalMsg MUSIC_TONE_COMPLETE\n"));
            CsrVoicePromptsPluginStopPhrase();
            }
        break;

        default:
        {
            PRINT(("handleKalimbaMessage: unhandled %X\n", m->id));
        }
        break;
    }
}

/*******************************************************************************
DESCRIPTION
    Tells upper layer that DSP doesn't have internal tone generator.
    External tone source have to be used.
*/
bool VoicePromptsDspAreTonesGeneratedOnDsp(void)
{
    return FALSE;
}

/*******************************************************************************
DESCRIPTION
    Initiate standalone tone/voice prompt playback.
    It starts with loading VP DSP application,
    upon receiving load confirmation from DSP the rest of setup will be done.
*/
static void playStandalone(vp_context_t *context)
{
    Task kal_task = MessageKalimbaTask((TaskData*)&csr_voice_prompts_plugin);
    /* If plug-in other that voice prompts were registered load voice prompts plug-in.
       Otherwise continue setup.
    */
    if(kal_task != &csr_voice_prompts_plugin)
    {
        loadDsp(context->codec_type);
    }
    else
    {
        (void)connectPromptSourceToDsp(context->codec_type, context->source);
    }
}

/*******************************************************************************
DESCRIPTION
    Connect tone or voice prompt source to the other already loaded plugin.
*/
static void mixWithOtherPlugin(vp_context_t *context)
{
    Sink mixing_port_sink;

    context->mixing = TRUE;

    /* Configure DSP tone or prompt playback */
    if(context->codec_type == voice_prompts_codec_tone)
    {
        PRINT(("VP: play tone\n"));
        setupRunningDspToPlayTone(context->playback_rate);
    }
    else
    {
        bool is_stereo = (context->codec_type == voice_prompts_codec_pcm) ? context->stereo : FALSE;
        PRINT(("VP: play adpcm/pcm\n"));
        setupRunningDspToPlayAudioPrompt(context->playback_rate, is_stereo);
    }

    /* Configure sink */
    mixing_port_sink = StreamKalimbaSink(TONE_VP_MIXING_DSP_PORT);

    PRINT(("VP: play dsp mix lSink = %p lSource = %p\n", (void*)mixing_port_sink, 
                                                         (void*)context->source));

    /* Get messages when source has finished */
    VoicePromptsRegisterForMessagesFromSink(mixing_port_sink);

    /* Stream voice prompt data to the DSP tone mixing port */
    if(context->codec_type == voice_prompts_codec_ima_adpcm)
        PanicFalse(TransformStart(TransformAdpcmDecode(context->source, mixing_port_sink)));
    else
        PanicNull(StreamConnect(context->source, mixing_port_sink));

    /* No need to set volume as host plugin has already done so */
}

/****************************************************************************
DESCRIPTION
    Stop prompt where DSP has not been loaded by the plugin, e.g. (adpcm or pcm)
    Prompt is either mixing in an existing DSP app or not using the DSP.
*/
static void stopPhraseMixable ( void )
{
    Sink lSink = NULL;

    /* Check for DSP mixing */
    if(isOtherPluginPresent())
    {
        lSink = StreamKalimbaSink(TONE_VP_MIXING_DSP_PORT);
    }
    else    /* Must be ADPCM not mixing */
    {
        PanicFalse(AudioOutputDisconnect());
    }

    /* Close sink and cancel any messages if valid */
    if(lSink)
    {
        /* Cancel all the messages relating to VP that have been sent */
        VoicePromptsDeregisterForMessagesFromSink(lSink);
        SinkClose(lSink);
    }

    MessageCancelAll((TaskData*) &csr_voice_prompts_plugin, MESSAGE_STREAM_DISCONNECT);
}

/****************************************************************************
DESCRIPTION
    Stop dsp phrase playing where VP plugin has loaded the DSP
*/
static void stopPhraseStandalone ( void )
{
    /* Turn off DSP and de-register the Kalimba Task */
    KalimbaPowerOff();
    (void)MessageKalimbaTask(NULL);

    /* Cancel all the messages relating to VP that have been sent */
    MessageCancelAll((TaskData*) &csr_voice_prompts_plugin, MESSAGE_FROM_KALIMBA);

    /* Disconnect PCM sources/sinks */
    (void)AudioOutputDisconnect();

#ifdef ANC
        /* Ensure ANC microphones are disconnected */
        AncDisconnectMics();
#endif /* ANC */
}

/****************************************************************************
DESCRIPTION
    Load DSP application for given codec type.
*/
static void loadDsp(voice_prompts_codec_t codec_type)
{
    const char *kap_file = getKapFileName(getDecoderType(codec_type));
    FILE_INDEX file_index = FileFind(FILE_ROOT,(const char *) kap_file ,(uint16)strlen(kap_file));

    /* Load DSP */
    if (!KalimbaLoad(file_index))
        Panic();
}


/****************************************************************************
DESCRIPTION
    Helper function for the handleMusicReadyMessage().
    It connects tone or voice prompt source to the DSP.

*/
static bool connectPromptSourceToDsp(voice_prompts_codec_t codec_type, Source prompt_source)
{
    bool using_tone_port;

#ifdef ANC
    /* Attempt to connect ANC microphones */
    AncConnectMics(StreamKalimbaSink(ANC_MIC_A_DSP_PORT), 
                   StreamKalimbaSink(ANC_MIC_B_DSP_PORT));
#endif /* ANC */

    if (!VoicePromptsIsCompressed(codec_type))
    {
        /* Connect ADPCM, PCM prompts and tones to the tone mixing port */
        Sink lSink = NULL;
        lSink = StreamKalimbaSink(TONE_VP_MIXING_DSP_PORT);

        if(codec_type == voice_prompts_codec_ima_adpcm)
            PanicFalse(TransformStart(TransformAdpcmDecode(prompt_source, lSink)));
        else
            PanicNull(StreamConnect(prompt_source, lSink));

        VoicePromptsRegisterForMessagesFromSink(lSink);

        using_tone_port = TRUE;
    }
    else
    {
        /* All other prompt codec types */
        PanicNull(StreamConnect(prompt_source, StreamKalimbaSink(DSP_CODEC_INPUT_PORT)));
        using_tone_port = FALSE;
    }

    return using_tone_port;
}

/****************************************************************************
DESCRIPTION
    Connect all the DSP output ports to hardware
*/
static void connectKalimbaOutputs(audio_output_params_t* mch_params)
{
    AudioOutputAddSourceOrPanic(StreamKalimbaSource(DSP_OUTPUT_PORT_PRI_LEFT), audio_output_primary_left);
    AudioOutputAddSourceOrPanic(StreamKalimbaSource(DSP_OUTPUT_PORT_PRI_RIGHT), audio_output_primary_right);
    AudioOutputAddSourceOrPanic(StreamKalimbaSource(DSP_OUTPUT_PORT_SEC_LEFT), audio_output_secondary_left);
    AudioOutputAddSourceOrPanic(StreamKalimbaSource(DSP_OUTPUT_PORT_SEC_RIGHT), audio_output_secondary_right);
    AudioOutputAddSourceOrPanic(StreamKalimbaSource(DSP_OUTPUT_PORT_AUX_LEFT), audio_output_aux_left);
    AudioOutputAddSourceOrPanic(StreamKalimbaSource(DSP_OUTPUT_PORT_AUX_RIGHT), audio_output_aux_right);
    AudioOutputAddSourceOrPanic(StreamKalimbaSource(DSP_OUTPUT_PORT_SUB_WIRED), audio_output_wired_sub);
    AudioOutputConnectOrPanic(mch_params);
}

/****************************************************************************
DESCRIPTION
    Handler for message sent after DSP application loading process is finished.
    This is actual implementation of 'playStandalone'.
*/
static void handleMusicReadyMessage(vp_context_t *context)
{
    bool using_tone_port;
    /* Set up multi-channel parameters. */
    audio_output_params_t mch_params;
    memset(&mch_params, 0, sizeof(audio_output_params_t));

    mch_params.sample_rate = FORCED_TONE_PLAYBACK_RATE;

    PRINT(("VP: Using multi-channel mapping (%luHz)\n", mch_params.sample_rate));

    using_tone_port = connectPromptSourceToDsp(context->codec_type, context->source);

    /* Connect outputs */
    connectKalimbaOutputs(&mch_params);

    /* Set the digital volume before playing the prompt */
    setVolume(context->prompt_volume, using_tone_port);

    /* Set the playback rate */
    KalimbaSendMessage(MESSAGE_SET_SAMPLE_RATE, context->playback_rate, 0, 0, 1);

    /* Set the codec in use */
    KalimbaSendMessage(MUSIC_SET_PLUGIN_MSG, getDecoderType(context->codec_type), 0, 0, 0);

    /* Set sample rates */
    /* If re-sampling was required, multi-channel library will have overridden supplied rate. */
    sendDspSampleRateMessages(context->codec_type, context->stereo,
            context->playback_rate, mch_params.sample_rate/DSP_RESAMPLING_RATE_COEFFICIENT);

    /* Let the DSP know which outputs are connected to which hardware types */
    sendDspMultiChannelMessages();

    /* Ready to go... */
    if (!KalimbaSendMessage(KALIMBA_MSG_GO, 0, 0, 0, 0))
    {
        PRINT(("VP: DSP failed to send go to kalimba\n"));
        Panic();
    }
}

/******************************************************************************
DESCRIPTION

    Sends input ('CODEC'), required output ('DAC'), and tone sample rate
    configuration messages to the DSP, taking into account any re-sampling.
*/
static void sendDspSampleRateMessages(voice_prompts_codec_t codec_type, bool is_stereo,
        unsigned playback_rate, unsigned resample_rate_with_coefficient_applied)
{
#ifdef ANC
    /* If using ANC then we need to give an indication of the sample rate required
       when the ANC mics are connected. */
    uint32 anc_sample_rate = AncGetDacSampleRate();
    uint16 anc_sample_rate_flag = DSP_ANC_SAMPLE_RATE_NONE;

    if (anc_sample_rate == ANC_SAMPLE_RATE_96K)
    {
        PRINT(("VP: Using ANC at 96k\n"));
        anc_sample_rate_flag = DSP_ANC_SAMPLE_RATE_96K;
    }
    else if (anc_sample_rate == ANC_SAMPLE_RATE_192K)
    {
        PRINT(("VP: Using ANC at 192k\n"));
        anc_sample_rate_flag = DSP_ANC_SAMPLE_RATE_192K;
    }

    /* Set the ANC Sample rate */
    if(!KalimbaSendMessage(MESSAGE_SET_ANC_MODE, anc_sample_rate_flag, 0, 0, 0))
    {
        PRINT(("VP: Message MESSAGE_SET_ANC_MODE failed!\n"));
        Panic();
    }
#endif /* ANC */

    if (codec_type == voice_prompts_codec_pcm)
    {
        /* PCM specific messages */
        /* Set that we are sending a PCM prompt in tone port */
        KalimbaSendMessage(MESSAGE_SET_DAC_SAMPLE_RATE, (uint16)resample_rate_with_coefficient_applied, 0, 0, (LOCAL_FILE_PLAYBACK|PCM_PLAYBACK));

        /* Configure prompt playback */
        KalimbaSendMessage(MESSAGE_SET_TONE_RATE_MESSAGE_ID, (uint16)playback_rate, (is_stereo?PROMPT_STEREO:0)|PROMPT_ISPROMPT, 0, 0);

        KalimbaSendMessage(MESSAGE_SET_CODEC_SAMPLE_RATE, (uint16)resample_rate_with_coefficient_applied, 0, 0, 0);
    }
    else if(codec_type == voice_prompts_codec_tone)
    {
        /* Tone specific messages */
        /* Set that we are sending a tone in tone port */
        KalimbaSendMessage(MESSAGE_SET_DAC_SAMPLE_RATE, (uint16)resample_rate_with_coefficient_applied, 0, 0, (PCM_PLAYBACK));

        /* Configure prompt playback */
        KalimbaSendMessage(MESSAGE_SET_TONE_RATE_MESSAGE_ID, (uint16)playback_rate, 0, 0, 0);

        KalimbaSendMessage(MESSAGE_SET_CODEC_SAMPLE_RATE, (uint16)resample_rate_with_coefficient_applied, 0, 0, 0);
    }
    else if (codec_type == voice_prompts_codec_ima_adpcm)
    {
        PRINT(("VP: Play ADPCM with DSP\n"));
        /* Set that we are sending a ADPCM prompt in tone port */
        KalimbaSendMessage(MESSAGE_SET_DAC_SAMPLE_RATE, (uint16)resample_rate_with_coefficient_applied, 0, 0, (LOCAL_FILE_PLAYBACK|PCM_PLAYBACK));

        KalimbaSendMessage(MESSAGE_SET_TONE_RATE_MESSAGE_ID, (uint16)playback_rate, PROMPT_ISPROMPT, 0, 0);

        KalimbaSendMessage(MESSAGE_SET_CODEC_SAMPLE_RATE,(uint16)resample_rate_with_coefficient_applied, 0, 0, 0);
    }
    else
    {
        /* Set the codec sampling rate (DSP needs to know this for resampling) */
        KalimbaSendMessage(MESSAGE_SET_DAC_SAMPLE_RATE, (uint16)resample_rate_with_coefficient_applied, 0, 0, (LOCAL_FILE_PLAYBACK));

        KalimbaSendMessage(MESSAGE_SET_CODEC_SAMPLE_RATE, (uint16)(playback_rate/DSP_RESAMPLING_RATE_COEFFICIENT), 0, 0, 0);
    }
}

/****************************************************************************
DESCRIPTION

    Sends configuration messages relating to multi-channel output to the DSP.
    This includes configuring the hardware type of each output, and the I2S
    master/slave mode.
*/
static void sendDspMultiChannelMessages(void)
{
    AUDIO_OUTPUT_TYPES_T message;

    PanicFalse(AudioOutputGetDspOutputTypesMsg(&message));

    /* Set the hardware type of each output */
    /* Set the hardware type of each output */
    KalimbaSendMessage(MESSAGE_SET_MULTI_CHANNEL_OUTPUT_TYPES_S, (uint16)(message.out_type[0]|(message.out_type[1]<<8)),
                                                                 (uint16)(message.out_type[2]|(message.out_type[3]<<8)),
                                                                 (uint16)(message.out_type[4]|0U),
                                                                 (uint16)(message.out_type[5]|(message.out_type[6]<<8)));

    if (AudioOutputI2sActive())
    {
        /* Set the I2S master/slave mode */
        if (!KalimbaSendMessage(MESSAGE_SET_I2S_CONFIG_MSG, !CsrI2SMasterIsEnabled(), 0, 0, 0))
        {
            PRINT(("DECODER: Message MESSAGE_SET_I2S_CONFIG_MSG failed!\n"));
            Panic();
        }
    }
}

/******************************************************************************
DESCRIPTION
    Voice prompt specific setup for other already running plugin
    voice prompt connects to.
*/
static void setupRunningDspToPlayAudioPrompt(unsigned playback_rate, bool is_stereo)
{
    uint16 flags = (is_stereo ? PROMPT_STEREO : 0) | PROMPT_ISPROMPT;
    KalimbaSendMessage(MESSAGE_SET_TONE_RATE_MESSAGE_ID, (uint16)playback_rate , flags, 0, 0);
}

/******************************************************************************
DESCRIPTION
    Tone specific setup for other already running plugin tone connects to.
*/
static void setupRunningDspToPlayTone(unsigned playback_rate)
{
    KalimbaSendMessage(MESSAGE_SET_TONE_RATE_MESSAGE_ID, (uint16)playback_rate , 0, 0, 0);
}

/******************************************************************************
DESCRIPTION
    Set the digital and hardware gain as appropriate. The multi-channel library
    takes care of checking which volume control mode has been configured and
    setting either the hardware or digital gain to a fixed level if applicable.
*/
static void setVolume(int16 prompt_volume, bool using_tone_port)
{
    audio_output_gain_t main_vol_msg;
    audio_output_gain_t aux_vol_msg;

    int16 master_volume = MAXIMUM_DIGITAL_VOLUME_0DB;

    /* Input is in DAC levels, multi-channel library expects dB/60 values */
    prompt_volume = VolumeConvertDACGainToDB(prompt_volume);

    if (!using_tone_port)
    {
        /* Adjust volume level by -6dB */
        prompt_volume = (int16)(prompt_volume + DSP_VOICE_PROMPTS_LEVEL_ADJUSTMENT);
        /* Have to use master volume to control prompt level */
        master_volume = prompt_volume;
    }

    /* Fill in and then send DSP volume messages */
    AudioOutputGainGetDigital(audio_output_group_main, master_volume, prompt_volume, (audio_output_gain_t*)&main_vol_msg);
    AudioOutputGainGetDigital(audio_output_group_aux, master_volume, prompt_volume, (audio_output_gain_t*)&aux_vol_msg);

    KalimbaSendMessage(MUSIC_VOLUME_MSG_S, 1, (uint16)main_vol_msg.trim.main.primary_left, (uint16)main_vol_msg.trim.main.primary_right, 0);
    KalimbaSendMessage(MUSIC_VOLUME_MSG_S, 2, (uint16)main_vol_msg.trim.main.secondary_left, (uint16)main_vol_msg.trim.main.secondary_right, (uint16)main_vol_msg.trim.main.wired_sub);
    KalimbaSendMessage(MUSIC_VOLUME_MSG_S, 0, (uint16)main_vol_msg.common.system, (uint16)main_vol_msg.common.master, (uint16)main_vol_msg.common.tone);

    KalimbaSendMessage(MUSIC_VOLUME_AUX_MSG_S, 1, (uint16)aux_vol_msg.trim.aux.aux_left, (uint16)aux_vol_msg.trim.aux.aux_left, 0);
    KalimbaSendMessage(MUSIC_VOLUME_AUX_MSG_S, 0, (uint16)aux_vol_msg.common.system, (uint16)aux_vol_msg.common.master, (uint16)aux_vol_msg.common.tone);

    /* Set hardware gains */
    AudioOutputGainSetHardware(audio_output_group_main, master_volume, NULL);
    AudioOutputGainSetHardware(audio_output_group_aux, master_volume, NULL);
}

/******************************************************************************
DESCRIPTION
    Get appropriate decoder name for given codec type.
*/
static PHRASE_DECODER_T getDecoderType(voice_prompts_codec_t codec_type)
{
    switch (codec_type)
    {
        /* if no DSP loaded, use SBC decoder to playback variable rate PCM and tones */
        case voice_prompts_codec_sbc:
        case voice_prompts_codec_pcm:
        case voice_prompts_codec_pcm_8khz:
        case voice_prompts_codec_ima_adpcm: /* use when resampling */
        case voice_prompts_codec_tone:
            return PHRASE_SBC_DECODER;
        case voice_prompts_codec_mp3:
            return PHRASE_MP3_DECODER;
        case voice_prompts_codec_aac:
            return PHRASE_AAC_DECODER;
        default:
            return PHRASE_NO_DECODER;
    }
}

/******************************************************************************
DESCRIPTION
    Get appropriate decoder kap file for given decoder name.
*/
static const char* getKapFileName(PHRASE_DECODER_T decoder)
{
    const char *kap_file_name = NULL;

    switch(decoder)
    {
    case PHRASE_SBC_DECODER:
        kap_file_name = "sbc_decoder/sbc_decoder.kap";
        break;
    case PHRASE_MP3_DECODER:
        kap_file_name =  "mp3_decoder/mp3_decoder.kap";
        break;
    case PHRASE_AAC_DECODER:
        kap_file_name = "aac_decoder/aac_decoder.kap";
        break;
    case PHRASE_NO_DECODER:
    default:
        Panic();
    }

    return kap_file_name;
}

/******************************************************************************
DESCRIPTION
    Is other plugin (like a2dp or cvc) already loaded.
*/
static bool isOtherPluginPresent(void)
{
    DSP_STATUS_INFO_T status = GetCurrentDspStatus();
    if(DSP_NOT_LOADED == status || DSP_ERROR == status)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}
