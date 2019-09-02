/****************************************************************************
Copyright (c) 2016 Qualcomm Technologies International, Ltd.
Part of ADK 4.1
 
FILE NAME
    voice_prompts_utils.h
 
DESCRIPTION
    Utility functions.
*/

#ifndef LIBS_CSR_VOICE_PROMPTS_PLUGIN_VOICE_PROMPTS_UTILS_H_
#define LIBS_CSR_VOICE_PROMPTS_PLUGIN_VOICE_PROMPTS_UTILS_H_

#include <csrtypes.h>
#include "audio_plugin_if.h"

/****************************************************************************
DESCRIPTION
    Validates codec type.
*/
bool VoicePromptsIsCodecTypeValid(voice_prompts_codec_t codec_type);

/****************************************************************************
DESCRIPTION
    Check if it is a tone.
*/
bool VoicePromptsIsItTone(voice_prompts_codec_t codec_type);

/****************************************************************************
DESCRIPTION
    Check if voice prompt is compressed.
*/
bool VoicePromptsIsCompressed(voice_prompts_codec_t codec_type);

/****************************************************************************
DESCRIPTION
    Check if voice prompt's sampling rate is fixed and it is 8 kHz.
*/
bool VoicePromptsIsSampleRateFixedAt8khz(voice_prompts_codec_t codec_type);

/****************************************************************************
DESCRIPTION
    Register for messages from given sink.
*/
void VoicePromptsRegisterForMessagesFromSink(Sink sink);

/****************************************************************************
DESCRIPTION
    Deregister from messages from given sink.
*/
void VoicePromptsDeregisterForMessagesFromSink(Sink sink);

#endif /* LIBS_CSR_VOICE_PROMPTS_PLUGIN_VOICE_PROMPTS_UTILS_H_ */
