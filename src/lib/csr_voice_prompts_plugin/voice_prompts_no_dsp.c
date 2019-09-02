/****************************************************************************
Copyright (c) 2016 Qualcomm Technologies International, Ltd.
Part of ADK 4.1
 
FILE NAME
    voice_prompts_no_dsp.c
 
DESCRIPTION
    Handling a case when DSP is not used for playback.
*/
#include <string.h>

#include "audio_output.h"
#include "gain_utils.h"
#include "print.h"

#include "voice_prompts_utils.h"
#include "voice_prompt_file.h"
#include "voice_prompts_no_dsp.h"

/****************************************************************************
DESCRIPTION
    Check if hardware or software configuration forces use of DSP.
*/
bool VoicePromptsNoDspIsDspRequiredForPlayback(bool force_resampling_of_tones)
{
    /* DSP re-sampler is required for I2S outputs to up-sample unsupported 8kHz tones */
    if(AudioOutputConfigRequiresI2s())
    {
        return TRUE;
    }
    /* Multichannel setup */
    if(AudioOutputGetOutputType(audio_output_aux_left) != OUTPUT_INTERFACE_TYPE_NONE)
    {
        return TRUE;
    }
    /* forced from sink config */
    if(force_resampling_of_tones)
    {
        return TRUE;
    }

    return FALSE;
}

/****************************************************************************
DESCRIPTION
    Connect and play tone or voice prompt without DSP.
*/
void VoicePromptsNoDspPlay(voice_prompts_codec_t codec_type,
        unsigned sample_rate, int16 volume, Source left_source, Source right_source)
{
    Sink lSink;

    audio_output_params_t mch_params;
    memset(&mch_params, 0, sizeof(audio_output_params_t));

    /* Not using DSP so can't re-sample */
    mch_params.disable_resample = TRUE;
    mch_params.sample_rate = sample_rate;
    mch_params.transform = (codec_type == voice_prompts_codec_ima_adpcm) ? audio_output_tansform_adpcm : audio_output_tansform_connect;

    PRINT(("VP: Play prompt direct (%u Hz) vol[%d]\n", sample_rate, volume));

    /* Connect prompt source(s) to appropriate output sink(s) */
    PanicFalse(AudioOutputConnectStereoSource(left_source, right_source, &mch_params));
    lSink = AudioOutputGetAudioSink();

    /* Request an indication that the prompt has completed */
    VoicePromptsRegisterForMessagesFromSink(lSink);

    /* Set gain (no DSP so set main hardware gains only) */
    AudioOutputGainSetHardwareOnly(audio_output_group_main, VolumeConvertDACGainToDB(volume));
}

/****************************************************************************
DESCRIPTION
    Get another source for voice prompt.
    Used to get stereo playback without DSP.
*/
Source VoicePromptsNoDspObtainAnotherPromptSource(vp_context_t *context)
{
    Source another_source = NULL;
    /* Obtain another source to the prompt if stereo, as each one can only be connected to one output sink */
    if(AudioOutputGetOutputType(audio_output_primary_right) != OUTPUT_INTERFACE_TYPE_NONE)
    {
        voice_prompt_t prompt;
        another_source = VoicePromptsFileGetToneOrPrompt(context, &prompt, TRUE);
        if (another_source == (Source)NULL)
        {
            PRINT(("VP: Failed obtain second prompt source\n"));
            Panic();
        }
    }

    return another_source;
}
