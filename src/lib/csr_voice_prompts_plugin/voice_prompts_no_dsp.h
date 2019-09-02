/****************************************************************************
Copyright (c) 2016 Qualcomm Technologies International, Ltd.
Part of ADK 4.1
 
FILE NAME
    voice_prompts_no_dsp.h
 
DESCRIPTION
    Handling a case when DSP is not used for playback.
    Main use case is uncompressed voice prompt.
*/

#ifndef LIBS_CSR_VOICE_PROMPTS_PLUGIN_VOICE_PROMPTS_NO_DSP_H_
#define LIBS_CSR_VOICE_PROMPTS_PLUGIN_VOICE_PROMPTS_NO_DSP_H_

#include <csrtypes.h>
#include "audio_plugin_if.h"
#include "voice_prompts_defs.h"

/****************************************************************************
DESCRIPTION
    Check if hardware or software configuration forces use of DSP.
*/
bool VoicePromptsNoDspIsDspRequiredForPlayback(bool force_resampling_of_tones);

/****************************************************************************
DESCRIPTION
    Connect and play tone or voice prompt without DSP.
*/
void VoicePromptsNoDspPlay(voice_prompts_codec_t codec_type,
        unsigned sample_rate, int16 volume, Source left_source, Source right_source);

/****************************************************************************
DESCRIPTION
    Get another source for voice prompt.
    Used to get stereo playback without DSP.
*/
Source VoicePromptsNoDspObtainAnotherPromptSource(vp_context_t *context);

#endif /* LIBS_CSR_VOICE_PROMPTS_PLUGIN_VOICE_PROMPTS_NO_DSP_H_ */
