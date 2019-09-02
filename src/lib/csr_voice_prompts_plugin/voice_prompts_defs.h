/****************************************************************************
Copyright (c) 2016 Qualcomm Technologies International, Ltd.
Part of ADK 4.1
 
FILE NAME
    voice_prompts_defs.h
 
DESCRIPTION
    Write a short description about what the sub module does and how it 
    should be used.
*/

#ifndef LIBS_CSR_VOICE_PROMPTS_PLUGIN_VOICE_PROMPTS_DEFS_H_
#define LIBS_CSR_VOICE_PROMPTS_PLUGIN_VOICE_PROMPTS_DEFS_H_

#include "audio_plugin_if.h"

typedef struct
{
    /*! If this is a tone, pointer to the ringtone_note */
    const ringtone_note       *tone;

    /*! prompt index Id. */
    uint16              prompt_id;
    /*! The language */
    uint16              language;

    /*! Decompression to use */
    voice_prompts_codec_t codec_type;
    /*! Playback rate */
    uint16              playback_rate;
    /*! stereo or mono */
    bool                stereo;

    /*! The volume at which to play the tone */
    int16               prompt_volume;
    /* */
    AudioPluginFeatures features;

    /*! VP source */
    Source              source;
    /*! 2nd source needed for right channel when not using DSP */
    Source              duplicate_source;

    /*! flag that indicates if the prompt is mixing with existing audio */
    bool                mixing;
    
    /*! Application task */
    Task                app_task;
} vp_context_t ;

#endif /* LIBS_CSR_VOICE_PROMPTS_PLUGIN_VOICE_PROMPTS_DEFS_H_ */
