/****************************************************************************
Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    csr_voice_prompts.h

DESCRIPTION
    
    
NOTES
   
*/

#ifndef _CSR_SIMPLE_TESXT_TO_SPEECH_H_
#define _CSR_SIMPLE_TESXT_TO_SPEECH_H_

/* Workaround GCC error where -DANC in command line becomes anc define */
#ifdef anc
#define ANC
#endif

#include "voice_prompts_defs.h"

void CsrVoicePromptsPluginInit(uint16 number_of_prompts, uint16 number_of_languages);
vp_context_t *VoicePromptsGetContext(void);


void CsrVoicePromptsPluginPlayPhrase(uint16 id , uint16 language, int16 no_dsp_prompt_volume , AudioPluginFeatures features, Task app_task);
void CsrVoicePromptsPluginStopPhrase ( void ) ;
void CsrVoicePromptsPluginPlayTone (const ringtone_note * tone, int16 tone_volume, AudioPluginFeatures features);
void CsrVoicePromptsPluginHandleStreamDisconnect(void);
void CsrVoicePromptsPluginTestReset(void);

#endif


