/****************************************************************************
Copyright (c) 2016 Qualcomm Technologies International, Ltd.
Part of ADK 4.1
 
FILE NAME
    csr_cvc_common_io_if.c
 
DESCRIPTION
    Microphone and outputs (e.g. DAC) related functions.
*/
#include <string.h>

#include <audio_plugin_common.h>
#include <vmal.h>
#include <print.h>

#include "csr_cvc_common_ctx.h"
#include "csr_cvc_common_dsp_if.h"
#include "csr_cvc_common_io_if.h"

/* Disable all but left and right primary outputs for standard CVC */
#define CVC_DISABLED_OUTPUTS ((unsigned)~(AudioOutputGetMask(audio_output_primary_left) | AudioOutputGetMask(audio_output_primary_right)))
/* Disable all but left primary output for no DSP mode */
#define CVC_NO_DSP_DISABLED_OUTPUTS ((unsigned)~(AudioOutputGetMask(audio_output_primary_left)))

#define ALL_OUTPUTS_DISABLED    0xFFFF

static unsigned getEnableMaskForChannel(const audio_output_t channel);

/*******************************************************************************
DESCRIPTION
    Set up multi-channel parameters as required by CVC. This will configure
    parameters to connect only the primary left output and disable re-sampling
    if the DSP is not being used, otherwise it will connect the primary left and
    primary right outputs.
*/
void CsrCvcIoSetupMicParams(audio_output_params_t* params, bool no_dsp)
{
    CVC_t *CVC = CsrCvcGetCtx();
    memset(params, 0, sizeof(audio_output_params_t));

    if(no_dsp)
    {
        /* In no DSP mode route only the primary left output for I2S/SPDIF */
        params->disable_mask = BITFIELD_CAST(audio_output_max, CVC_NO_DSP_DISABLED_OUTPUTS);

        /* Set sample rate. Can't re-sample if DSP is not being used */
        params->sample_rate  = CVC->incoming_rate;
        params->disable_resample = TRUE;
    }
    else
    {
        params->disable_mask = BITFIELD_CAST(audio_output_max, CVC_DISABLED_OUTPUTS);
        params->sample_rate  = CVC->incoming_rate;
    }
}

/*******************************************************************************
DESCRIPTION
    Set mic gain
*/
void CsrCvcIoSetMicGain(audio_instance instance, audio_channel channel, bool digital, T_mic_gain gain)
{
    Source mic_source = AudioPluginGetMic(instance, channel, digital);
    uint8 mic_gain = (digital ? gain.digital_gain : gain.analogue_gain);
    AudioPluginSetMicGain(mic_source, digital, mic_gain, gain.preamp_enable);
}

void CsrCvcIoMuteMic(bool is_mic_two_present)
{
    CVC_t *CVC = CsrCvcGetCtx();
    /* set gain to 0 */
    T_mic_gain input_gain;
    memset(&input_gain,0,sizeof(T_mic_gain));

    /* Set input gain(s) */
    CsrCvcIoSetMicGain(AudioPluginGetInstance(CVC->digital->mic_a),
                 AUDIO_CHANNEL_A, CVC->digital->mic_a.digital, input_gain);
    if(is_mic_two_present)
        CsrCvcIoSetMicGain(AudioPluginGetInstance(CVC->digital->mic_b),
                     AUDIO_CHANNEL_B, CVC->digital->mic_b.digital, input_gain);
}

/*******************************************************************************
DESCRIPTION
    Connect an ADC or digital microphone input to the DSP
*/
void CsrCvcIoConnectMic(const audio_channel channel, const audio_mic_params * const params)
{
    CVC_t *CVC = CsrCvcGetCtx();
    audio_output_params_t mch_params;

    CsrCvcIoSetupMicParams(&mch_params, CVC->no_dsp);

    if(CVC->no_dsp)
    {
        Source mic_source = AudioPluginMicSetup(channel, *params, mch_params.sample_rate);
        PanicNull(StreamConnect(mic_source, CVC->audio_sink));
    }
    else
    {
        Source mic_source = AudioPluginMicSetup(channel, *params, AudioOutputGetSampleRate(&mch_params));
        csrCvcCommonDspConnectMicrophones(mic_source, NULL);
    }
}

/*******************************************************************************
DESCRIPTION
    Disonnect an ADC or digital microphone input from the DSP
*/
void CsrCvcIoDisconnectMic(const audio_channel channel, const audio_mic_params * const params)
{
    Source mic_source = AudioPluginGetMic(AudioPluginGetInstance(*params),
                                        channel, params->digital);
    PRINT(("CVC: NODSP: Disconnect Mic\n")) ;

    StreamDisconnect(mic_source, NULL);

    SourceClose(mic_source);
}

/*******************************************************************************
DESCRIPTION
    Get the audio channel (A or B) used for a particular microphone
*/
audio_channel CsrCvcIoGetAudioChannelFromMicId(const microphone_input_id_t mic_id)
{
    if(mic_id == microphone_input_id_voice_b
#ifdef ANC
            || mic_id == microphone_input_id_anc_b
#endif
            )
    {
        return AUDIO_CHANNEL_B;
    }
    return AUDIO_CHANNEL_A;
}

/*******************************************************************************
DESCRIPTION
    Connect (e)SCO with CVC processing disabled. This may be either no DSP mode
    when the (e)SCO is connected directly to the primary left speaker OR it may
    be passthrough mode where CVC is loaded and processing disabled.
*/
void CsrCvcIoConnectOutputNoCvc(void)
{
    CVC_t *CVC = CsrCvcGetCtx();
    audio_output_params_t params;

    /* Don't try to reconnect if sink has closed (e.g. audio transferred to
    the phone while tone playing). */
    if(!SinkIsValid(CVC->audio_sink))
    {
        return;
    }

    PRINT(("CVC: NODSP: Connect Output\n")) ;

    CsrCvcIoSetupMicParams(&params, CVC->no_dsp);

    if(CVC->no_dsp)
    {
        params.disable_mask = BITFIELD_CAST(audio_output_max, getEnableMaskForChannel(CVC->output_for_no_cvc));

        AudioOutputConnectSource(CVC->audio_source,
                                                      CVC->output_for_no_cvc,
                                                      &params);
    }
    else
    {
        params.sample_rate = AudioOutputGetSampleRate(&params);
        params.disable_mask = BITFIELD_CAST(audio_output_max, getEnableMaskForChannel(CVC->output_for_no_cvc));

        csrCvcCommonDspConnectMonoSpeaker(CVC->output_for_no_cvc, &params);

        csrCvcCommonDspConfigureHardware(params.sample_rate, params.sample_rate,
                                         rate_matching_software,
                                         AudioOutputI2sActive() ?  hardware_type_i2s : hardware_type_dac);
    }
    CsrCvcPluginSetVolume(CVC->volume);
}

/*******************************************************************************
DESCRIPTION
    Disconnect speakers with CVC processing disabled.
*/
void CsrCvcIoDisconnectOutputNoCvc ( void )
{
    PRINT(("CVC: NODSP: Disconnect Speaker\n")) ;

    /* Possible to have a race between remote close of the link and app disconnecting
       audio. Audio may already be disconnected so ignore return value */
    (void)AudioOutputDisconnect();

    /* Clear message task for the audio sink */
    VmalMessageSinkTask( AudioOutputGetAudioSink(), NULL );
}

/*******************************************************************************
DESCRIPTION
    Calculate the mask required to only enable a specific output channel
*/
static unsigned getEnableMaskForChannel(const audio_output_t channel)
{
    unsigned mask = ALL_OUTPUTS_DISABLED;
    AudioOutputSetEnabled(mask, channel);
    return mask;
}
