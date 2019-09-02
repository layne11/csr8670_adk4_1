/****************************************************************************
Copyright (c) 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.1
 
FILE NAME
    voice_prompts_dsp_if.h
 
DESCRIPTION
    DSP interface. It's implementation depends on underlying hardware.
*/

#ifndef LIBS_CSR_VOICE_PROMPTS_PLUGIN_VOICE_PROMPTS_DSP_IF_H_
#define LIBS_CSR_VOICE_PROMPTS_PLUGIN_VOICE_PROMPTS_DSP_IF_H_

#include <csrtypes.h>
#include "voice_prompts_defs.h"

/*******************************************************************************
DESCRIPTION
    Play voice prompt or tone.
*/
void VoicePromptsDspPlay(vp_context_t *context);

/*******************************************************************************
DESCRIPTION
    Stop playing voice prompt or tone and initiate clean up procedure.
*/
void VoicePromptsDspStop(void);

/*******************************************************************************
    Clean up functions.
*/

/*******************************************************************************
DESCRIPTION
    Operations needed to be executed before actual disconnecting
    and freeing resources.
*/
void VoicePromptsDspPrepareForClose(void);

/*******************************************************************************
DESCRIPTION
    Decide if clean up shall proceed in the case when it was initiated by
    stream disconnect message.
*/
bool VoicePromptsDspShallHandleStreamDisconnect(vp_context_t *context);

/*******************************************************************************
DESCRIPTION
    Free resources and clean up.
*/
void VoicePromptsDspCleanup(vp_context_t *context);

/*******************************************************************************
    Helper functions.
*/

/*******************************************************************************
DESCRIPTION
    Determine if messages was sent by DSP.
*/
bool VoicePromptsIsItDspMessage(MessageId id);

/*******************************************************************************
DESCRIPTION
    Handler for messages sent by DSP.
*/
void VoicePromptsDspMessageHandler(vp_context_t *context, Task task, Message message);

/*******************************************************************************
DESCRIPTION
    Determine if DSP has internal tone generator.
    When this is a case then external tone source is not used.
*/
bool VoicePromptsDspAreTonesGeneratedOnDsp(void);

#endif /* LIBS_CSR_VOICE_PROMPTS_PLUGIN_VOICE_PROMPTS_DSP_IF_H_ */
