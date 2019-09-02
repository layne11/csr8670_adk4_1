/****************************************************************************
Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    csr_simple_text_to_speech_plugin.c
DESCRIPTION
    an audio plugin
NOTES
*/
#include "print.h"

#include "audio_plugin_if.h" /*the messaging interface*/
#include "voice_prompts_dsp_if.h"
#include "csr_voice_prompts.h"
#include "csr_voice_prompts_plugin.h"

/*the task message handler*/
static void message_handler (Task task, MessageId id, Message message);

/*the local message handling functions*/
static void handleAudioMessage (Task task , MessageId id, Message message);
static void handleInternalMessage (Task task , MessageId id, Message message);
    
/*the plugin task*/
const TaskData csr_voice_prompts_plugin = { message_handler };

/****************************************************************************
DESCRIPTION
    The main task message handler
*/
static void message_handler ( Task task, MessageId id, Message message ) 
{
    if ( (id >= AUDIO_DOWNSTREAM_MESSAGE_BASE ) && (id < AUDIO_DOWNSTREAM_MESSAGE_TOP) )
    {
        handleAudioMessage (task , id, message ) ;
    }
    else if (VoicePromptsIsItDspMessage(id))
    {
        VoicePromptsDspMessageHandler(VoicePromptsGetContext(), task , message) ;
    }
    else
    {
        handleInternalMessage (task , id , message ) ;
    }
}    

/****************************************************************************
DESCRIPTION

    messages from the audio library are received here. 
    and converted into function calls to be implemented in the 
    plugin module
*/ 
static void handleAudioMessage ( Task task , MessageId id, Message message )     
{
    UNUSED(task);
    switch (id)
    {
        case (AUDIO_PLUGIN_VOICE_PROMPTS_INIT_MSG ):
        {
            AUDIO_PLUGIN_VOICE_PROMPTS_INIT_MSG_T * init_message = (AUDIO_PLUGIN_VOICE_PROMPTS_INIT_MSG_T *)message;
            CsrVoicePromptsPluginInit(init_message->number_of_prompts, init_message->number_of_languages);
        }
        break;
        
        case (AUDIO_PLUGIN_PLAY_AUDIO_PROMPT_MSG ):
        {
            AUDIO_PLUGIN_PLAY_AUDIO_PROMPT_MSG_T * prompt_message = (AUDIO_PLUGIN_PLAY_AUDIO_PROMPT_MSG_T *)message ;

            CsrVoicePromptsPluginPlayPhrase(prompt_message->id,
                                            prompt_message->language,
                                            prompt_message->ap_volume, /* Here its used for playing audio without DSP */
                                            prompt_message->features,
                                            prompt_message->app_task);
        }
        break ;
        
        case (AUDIO_PLUGIN_PLAY_TONE_MSG ):
        {
            AUDIO_PLUGIN_PLAY_TONE_MSG_T * tone_message = (AUDIO_PLUGIN_PLAY_TONE_MSG_T *)message ;
            
            CsrVoicePromptsPluginPlayTone(tone_message->tone,
                tone_message->tone_volume,
                tone_message->features) ;
        }
        break ;        
        
        case (AUDIO_PLUGIN_STOP_TONE_AND_PROMPT_MSG ):
                CsrVoicePromptsPluginStopPhrase() ;
        break ;             
        
        case (AUDIO_PLUGIN_TEST_RESET_MSG):
            CsrVoicePromptsPluginTestReset();
        break;

        default:
            /*other messages do not need to be handled by the voice prompts plugin*/
        break;
    }
}

/****************************************************************************
DESCRIPTION
    Internal messages to the task are handled here
*/ 
static void handleInternalMessage ( Task task , MessageId id, Message message )     
{
    UNUSED(task);
    UNUSED(message);
    switch (id)
    {
        case MESSAGE_STREAM_DISCONNECT:
            CsrVoicePromptsPluginHandleStreamDisconnect();
        break ;
        
        case MESSAGE_MORE_SPACE:
        break;
        
        default:
            Panic() ;
    }
}
