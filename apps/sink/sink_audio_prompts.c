/*
Copyright (c) 2004 - 2016 Qualcomm Technologies International, Ltd.
*/
/**
\file
\ingroup sink_app
\brief
    module responsible for audio (voice) prompts

*/

#include "sink_private.h"
#include "sink_debug.h"
#include "sink_audio_prompts.h"
#include "sink_events.h"
#include "sink_tones.h"
#include "sink_statemanager.h"
#include "sink_pio.h"
#include "sink_development.h"
#include "vm.h"


#include <stddef.h>
#include <csrtypes.h>
#include <audio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <partition.h>
#include <csr_voice_prompts_plugin.h>

#ifdef DEBUG_AUDIO_PROMPTS
    #define PROMPTS_DEBUG(x) DEBUG(x)
#else
    #define PROMPTS_DEBUG(x) 
#endif

#define xstr(s) str(s)
#define str(s) #s
#define NUMERIC_DIGITS_DISPLAY 6

/****************************************************************************
NAME
    AudioPromptPlay
    
DESCRIPTION
    Conditionally call text-to-speech plugin
*/
void AudioPromptPlay(Task plugin, uint16 id, bool can_queue, bool override)
{
    if (theSink.audio_prompts_enabled)
    {
        /* turn on audio amp */
        PioDrivePio(PIO_AUDIO_ACTIVE, TRUE);
        /* start check to turn amp off again if required */ 
        MessageSendLater(&theSink.task , EventSysCheckAudioAmpDrive, 0, 1000);    
        
#ifdef ENABLE_SQIFVP        
        /* If using multiple partitions for the voice prompt langages, mount the relevant partiton if required */

        /* find if the partition for this language is mounted */
        if (!((1<<theSink.audio_prompt_language) & theSink.rundata->partitions_mounted))
        {
            /* Mount the partition for this prompt */
            PROMPTS_DEBUG(("AudioPromptPlay mount SQIF partition %u (0x%x)\n", theSink.audio_prompt_language, theSink.rundata->partitions_mounted));
            if(!PartitionMountFilesystem(PARTITION_SERIAL_FLASH, theSink.audio_prompt_language , PARTITION_LOWER_PRIORITY))
                Panic();
            
            theSink.rundata->partitions_mounted |= (1<<theSink.audio_prompt_language);
            PROMPTS_DEBUG(("AudioPromptPlay SQIF partitions now mounted 0x%x\n", theSink.rundata->partitions_mounted ));
        }
#endif
        
        PROMPTS_DEBUG(("AudioPromptPlay %d  [lang:%u][q:%u][o.r.:%u]\n", id, theSink.audio_prompt_language,can_queue,override));
        AudioPlayAudioPrompt(plugin, id, theSink.audio_prompt_language, can_queue,
                     (int16)TonesGetToneVolume(FALSE), theSink.conf2->audio_routing_data.PluginFeatures, override, &theSink.task);
    }
}


/****************************************************************************
NAME 
DESCRIPTION
RETURNS    
*/

void AudioPromptConfigure( uint16 size_index )
{
    PROMPTS_DEBUG(("Setup AP Indexing: %d prompts\n",size_index));
    AudioVoicePromptsInit((TaskData *)&csr_voice_prompts_plugin, size_index, theSink.num_audio_prompt_languages);
}

/****************************************************************************
NAME 
    AudioPromptPlayNumString
DESCRIPTION
    Play a numeric string using the Audio Prompts plugin
RETURNS    
*/

void AudioPromptPlayNumString(uint16 size_num_string, const uint8* num_string)
{
    if(size_num_string)
    {
        const uint8 * pData = num_string;
        uint8 i;        

        /* check each character in the string is a numeric character */
        for(i=0;i<size_num_string;i++)
        {                    
            /* Check for non-numeric characters */
            if(*pData >= 0x30 || *pData <= 0x39)
            {
                PROMPTS_DEBUG(("AP: PlayDigit[%x]\n", pData[i]- 0x30 )) ;
                
                switch (pData[i] - 0x30)
                {
                    /* Send event corresponding to the digit, assign audio prompts to these
                       events in the normal manner */
                    case 0:
                        MessageSend(&theSink.task, EventSysToneDigit0, 0);
                        break;
                    case 1:
                        MessageSend(&theSink.task, EventSysToneDigit1, 0);
                        break;                    
                    case 2:
                        MessageSend(&theSink.task, EventSysToneDigit2, 0);
                        break;
                    case 3:
                        MessageSend(&theSink.task, EventSysToneDigit3, 0);
                        break;
                    case 4:
                        MessageSend(&theSink.task, EventSysToneDigit4, 0);
                        break;
                    case 5:
                        MessageSend(&theSink.task, EventSysToneDigit5, 0);
                        break;
                    case 6:
                        MessageSend(&theSink.task, EventSysToneDigit6, 0);
                        break;
                    case 7:
                        MessageSend(&theSink.task, EventSysToneDigit7, 0);
                        break;
                    case 8:
                        MessageSend(&theSink.task, EventSysToneDigit8, 0);
                        break;
                    case 9:
                        MessageSend(&theSink.task, EventSysToneDigit9, 0);
                        break;   
                    default:
                        ;
                }
            }                    
        }
    }
}

/****************************************************************************
NAME 
    AudioPromptPlayNumber
DESCRIPTION
    Play a uint32 using the Audio prompt plugin
RETURNS    
*/

void AudioPromptPlayNumber(uint32 number)
{
    /* A 32 bit number can be accommodated in 10 digits */
    char num_string[11];
    /* Convert number to at least a 6 digit string left padded with zeros */
    int16 len = sprintf(num_string, "%0" xstr(NUMERIC_DIGITS_DISPLAY) "lu", number);
    
    /* Play the final 6 digits of the string */
    if(len >= NUMERIC_DIGITS_DISPLAY)
    {
        AudioPromptPlayNumString(NUMERIC_DIGITS_DISPLAY, (uint8*)(num_string + len - NUMERIC_DIGITS_DISPLAY));
    }
    else
    {
        FATAL_ERROR(("Conversion of  number to string for Audio Prompts Play failed!\n"));
    }
}

/****************************************************************************
NAME 
DESCRIPTION
RETURNS    
*/

bool AudioPromptPlayCallerNumber( const uint16 size_number, const uint8* number ) 
{
	if(theSink.features.VoicePromptNumbers)
    {
        if(theSink.RepeatCallerIDFlag && size_number > 0) 
        { 
            theSink.RepeatCallerIDFlag = FALSE;
            AudioPromptPlayNumString(size_number, number);
            return TRUE;
        }
    }

    return FALSE;
}

/****************************************************************************
NAME 
DESCRIPTION
RETURNS    
*/
bool AudioPromptPlayCallerName( const uint16 size_name, const uint8* name ) 
{
#ifdef TEXT_TO_SPEECH_NAMES
	TaskData * task = (TaskData *) &INSERT_TEXT_TO_SPEECH_PLUGIN_HERE;
    
	if(size_name > 0) 
	{
        /* Hook point for Text to speech synthesizer */
        Panic();
	}
#else
    UNUSED(size_name);
#endif
    UNUSED(name);

    return FALSE;
}

/****************************************************************************
NAME    
    AudioPromptCancelNumString
    
DESCRIPTION
  	function to cancel any pending number string messages.
    
RETURNS
    
*/
void AudioPromptCancelNumString( void )
{
    /* Do nothing if Audio Prompt Terminate Disabled */
    if(!theSink.features.DisableAudioPromptTerminate)
    {
        PROMPTS_DEBUG(("PROMPTS: Cancel pending digits \n")) ;
        /* Cancel any digits that maybe pending */
        MessageCancelAll(&theSink.task,EventSysToneDigit0);
        MessageCancelAll(&theSink.task,EventSysToneDigit1);
        MessageCancelAll(&theSink.task,EventSysToneDigit2);
        MessageCancelAll(&theSink.task,EventSysToneDigit3);
        MessageCancelAll(&theSink.task,EventSysToneDigit4);
        MessageCancelAll(&theSink.task,EventSysToneDigit5);
        MessageCancelAll(&theSink.task,EventSysToneDigit6); 
        MessageCancelAll(&theSink.task,EventSysToneDigit7);        
        MessageCancelAll(&theSink.task,EventSysToneDigit8);        
        MessageCancelAll(&theSink.task,EventSysToneDigit9);        
    }
}  
      
/****************************************************************************
NAME    
    AudioPromptSelectLanguage
    
DESCRIPTION
  	function to select a audio prompt language.
    
RETURNS
    
*/
void AudioPromptSelectLanguage( void )
{
#ifdef ENABLE_SQIFVP     
    uint16 current_lang = theSink.audio_prompt_language;
#endif    
    uint16 delay = theSink.conf1->timeouts.LanguageConfirmTime_s;

#ifdef ENABLE_SQIFVP  
    /* if using Multiple partitions in SQIF for voice prompts make sure we choose one with prompts in it */
    do
    {
        theSink.audio_prompt_language++;
	    if(theSink.audio_prompt_language >= theSink.num_audio_prompt_languages)
            theSink.audio_prompt_language = 0;   
        
        PROMPTS_DEBUG(("AP: Select language [%u][%u][%u][0x%x]\n", theSink.audio_prompt_language,current_lang, theSink.num_audio_prompt_languages,theSink.rundata->partitions_free )) ;
    }
    while (((1<<theSink.audio_prompt_language) & theSink.rundata->partitions_free) && (theSink.audio_prompt_language != current_lang));

#else    
    theSink.audio_prompt_language++;
	if(theSink.audio_prompt_language >= theSink.num_audio_prompt_languages)
        theSink.audio_prompt_language = 0;
    
    PROMPTS_DEBUG(("AP: Select language [%u]\n", theSink.audio_prompt_language )) ;
#endif

    if(delay)
    {
        MessageCancelAll(&theSink.task, EventSysStoreAudioPromptLanguage);
        MessageSendLater(&theSink.task, EventSysStoreAudioPromptLanguage, 0, D_SEC(delay));
    }
}

/****************************************************************************
NAME    
    AudioPromptGetConfig
    
DESCRIPTION
    Returns audio prompt configuration for the event to caller if this
    event is configured.
 
PARAMETERS
    sink_event    - incoming sink event

RETURNS
    audio prompt configration if it is present otherwise returns NULL.
*/
static audio_prompts_config_type* AudioPromptGetConfig(sinkEvents_t sink_event)
{
    uint16 lEventIndex = sink_event;
    audio_prompts_config_type* ptr  = NULL;

    if(theSink.conf4)
    {
        ptr = theSink.conf4->audioPromptEvents;
    }
    else
    {
        /* no config */
        return NULL;
    }

    /* Check if this event has a configured audio prompt*/
    while(ptr->prompt_id != AUDIO_PROMPT_NOT_DEFINED)
    {
        if(ptr->event == lEventIndex)
        {
            PROMPTS_DEBUG(("AP: Event conf available\n"));
            return ptr;
        }
        ptr++;
    }
    return NULL;
}

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
bool IsAudioPromptConfiguredForEvent(sinkEvents_t sink_event)
{
    return (AudioPromptGetConfig(sink_event) != NULL);
}

/****************************************************************************
NAME    
    AudioPromptPlayEvent
    
DESCRIPTION
    Plyas audio prompt attached to the incoming sink event.
 
PARAMETERS
    sink_event           - incoming sink event
    event_can_be_queued  - Flag which indicate if this event
    					   is to be played imidiately or not

RETURNS
    void
*/
void AudioPromptPlayEvent(sinkEvents_t sink_event, bool event_can_be_queued)
{
    TaskData * task    = NULL;
    uint16 state_mask  = 1 << stateManagerGetState();

    audio_prompts_config_type* config = AudioPromptGetConfig(sink_event);

    if(config)
    {
        task = (TaskData *) &csr_voice_prompts_plugin;
		
        while(config->prompt_id != AUDIO_PROMPT_NOT_DEFINED)
        {
            /* Play Audio Prompt if  we're not in a blocked state or in streaming A2DP state */
            if((config->event == sink_event) && 
               ((config->state_mask & state_mask) && 
               (!(config->sco_block && theSink.routed_audio)||(state_mask & (1<<deviceA2DPStreaming)))))
            {
                PROMPTS_DEBUG(("AP: EvPl[%x][%x][%x][%x]\n", sink_event, config->event,\
                                          config->prompt_id, config->cancel_queue_play_immediate )) ;
                if (event_can_be_queued == FALSE)
                {
                    /* never queue mute reminders to protect against the case that the prompt is longer 
                       than the mute reminder timer */
                    AudioPromptPlay(task, (uint16) config->prompt_id, FALSE, config->cancel_queue_play_immediate);
			    }
		    	else
			    {
                    AudioPromptPlay(task, (uint16) config->prompt_id, TRUE, config->cancel_queue_play_immediate);
			    }
            }
            config++;
        }
    }
}
