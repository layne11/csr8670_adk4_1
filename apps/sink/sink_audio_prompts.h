/*
Copyright (c) 2004 - 2016 Qualcomm Technologies International, Ltd.
*/
/**
\file
\ingroup sink_app
\brief
    header file which defines the interface between the audio (voice) prompts and the application

*/

#ifndef SINK_AUDIO_PROMPTS_H
#define SINK_AUDIO_PROMPTS_H
#include "sink_debug.h"


#define AUDIO_PROMPT_NOT_DEFINED (0xFF)



/****************************************************************************

*/
void AudioPromptConfigure( uint16 size_index );

/****************************************************************************

*/
void AudioPromptPlay(Task plugin, uint16 id, bool can_queue, bool override);

/****************************************************************************
NAME 
    AudioPromptPlayNumString
DESCRIPTION
    Play a numeric string using the Audio Prompt plugin
RETURNS    
*/
void AudioPromptPlayNumString(uint16 size_num_string, const uint8* num_string);

/****************************************************************************
NAME 
    AudioPromptPlayNumber
DESCRIPTION
    Play a uint32 using the audio prompt plugin
RETURNS    
*/
void AudioPromptPlayNumber(uint32 number);

/* **************************************************************************
   */


bool AudioPromptPlayCallerNumber( const uint16 size_number, const uint8* number );

/****************************************************************************
NAME    
    AudioPromptPlayCallerName
    
DESCRIPTION
  	function to play caller name
    
RETURNS
    
*/
bool AudioPromptPlayCallerName( const uint16 size_name, const uint8* name );
   
/****************************************************************************
NAME    
    AudioPromptCancelNumString
    
DESCRIPTION
  	function to cancel any pending number string messages.
    
RETURNS
    
*/
void AudioPromptCancelNumString( void );

/****************************************************************************
NAME    
    AudioPromptSelectLanguage
    
DESCRIPTION
  	Move to next language
    
RETURNS
    
*/
void AudioPromptSelectLanguage( void );

/****************************************************************************
NAME    
    IsAudioPromptConfiguredForEvent
    
DESCRIPTION
    Checks if audio prompt is configured for the given sink event.
 
PARAMETERS
    sink_event    - incoming sink event

RETURNS
    TRUE if audio prompt is configured for the sink event
    FALSE if audio proompt is not configured for the sink event
*/
bool IsAudioPromptConfiguredForEvent(sinkEvents_t sink_event);

/****************************************************************************
NAME    
    AudioPromptsPlayEvent
    
DESCRIPTION
    Plyas audio prompt attached to the incoming sink event.
 
PARAMETERS
    sink_event           - incoming sink event
    event_can_be_queued  - Flag which indicate if this event
    					   is to be played imidiately or not
 
RETURNS
    void
*/
void AudioPromptPlayEvent(sinkEvents_t sink_event, bool event_can_be_queued);
#endif

