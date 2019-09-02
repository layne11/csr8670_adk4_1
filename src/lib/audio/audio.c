/****************************************************************************
Copyright (c) 2004 - 2016 Qualcomm Technologies International, Ltd.

FILE NAME
    audio.c        
    
DESCRIPTION
     The main audio manager file
     
*/ 
#include "audio.h"

#include <stdlib.h>
#include <string.h>

#include <stream.h>
#include <audio_plugin_music_params.h>
#include <csr_voice_prompts_plugin.h>
#include <panic.h>
#include <print.h>



#if defined(__KALIMBA__) && !defined(__GNUC__)
# pragma unitsuppress OuterDefinition    /* MAKE_...MESSAGE macros used at different levels in same function */
#endif

typedef struct audio_lib_Tag
{
    Task active_plugin;                            /* main plugin in use */
    Task plugin;                    /* plugin for a connect that is pending, but is yet to be processed */
    Task relay_plugin;                      /* plugin used for managing stream relay operation */
    Task AUDIO_BUSY;                        /* audio routing currently in progress */
    AUDIO_PLUGIN_CONNECT_MSG_T message;     /* store of current connection params */
    DSP_STATUS_INFO_T dsp_status;           /* current dsp status */
    Sink forwarding_sink;
    Sink left_tone_mixing_sink;                  /* to be set by non VP plugins, so VP plugin can connect and mix together */
    Sink right_tone_mixing_sink;
    unsigned maximum_audio_connections;
    unsigned number_of_audio_connections;
    unsigned audio_in_use:1;                /* audio currently being routed */
    unsigned tone_playing:1;                /* tone currently being played */
    unsigned vp_playing:1;                  /* voice prompt currently being played */
    unsigned asr_running:1;                 /* asr is currently running/listening */
    unsigned content_protection:1;
    unsigned unused:11;     
}AUDIO_t ;

static Task audioGetPlugin(void);
static void audioSetPlugin(Task plugin);
static AUDIO_t * AUDIO;

/****************************************************************************
NAME    
    AudioLibraryInit

DESCRIPTION
    This function mallocs the memory to be used by the audio library 

RETURNS
    void
*/
void AudioLibraryInit (  void )
{
    if (AUDIO == NULL)
    {
        /* malloc one allocation for audio and plugin use */
        AUDIO = PanicUnlessMalloc(sizeof(AUDIO_t));  
    
        /* initialise to 0 */
        memset(AUDIO,0,sizeof(AUDIO_t));
        AUDIO->maximum_audio_connections = 1;
    }
}

/****************************************************************************
NAME
    audioCanConnect

DESCRIPTION
    This function returns a boolean to indicate whether resources are available to allow a connect
    request to succeed

RETURNS
    void
*/
static bool audioCanConnect(void)
{
    return (AUDIO->number_of_audio_connections < AUDIO->maximum_audio_connections);
}


/****************************************************************************
NAME    
    audioCancelMessage

DESCRIPTION
    This function sends cancel message requests to the active plugin, along with the plugin passed as a parameter

RETURNS
    void
*/
static void audioCancelMessage(Task plugin, audio_plugin_interface_message_type_t msg)
{
    Task audio_plugin = audioGetPlugin();

    MessageCancelAll(audio_plugin, msg);
    if (audio_plugin != plugin)
        MessageCancelAll(plugin, msg);
}

/****************************************************************************
NAME    
    AudioConnect

DESCRIPTION
    This function connects audio to the stream subsystem

RETURNS
    The Task associated with the audio connection if successful, else NULL
*/

Task AudioConnect ( Task audio_plugin,
                     Sink audio_sink,
                     AUDIO_SINK_T sink_type,
                     int16 volume,
                     uint32 rate,
                     AudioPluginFeatures features,
                     AUDIO_MODE_T mode, 
                     AUDIO_ROUTE_T route,
                     AUDIO_POWER_T power, 
                     void * params,
                     Task app_task )
{ 
    PanicNull(AUDIO);
    if (audioCanConnect())
    {
        MAKE_AUDIO_MESSAGE( AUDIO_PLUGIN_CONNECT_MSG, message ) ;
        PanicNull(audio_plugin);

        message->audio_sink = audio_sink ;
        message->sink_type  = sink_type ;
        message->volume     = volume ;
        message->rate       = rate;
        message->mode       = mode ;
        message->route      = route;
        message->features   = features ;
        message->params     = params ;
        message->app_task   = app_task ;
        message->power      = power ;

        audioSetPlugin(audio_plugin);
        AUDIO->message = *message;
    
        PRINT(("AUD: AudioConnect pl[%p] sk[%p] bsy[%p]\n", (void*)audio_plugin, (void*)audio_sink, (void*)AUDIO->AUDIO_BUSY ));
        MessageSendConditionallyOnTask ( audio_plugin, AUDIO_PLUGIN_CONNECT_MSG, message, AudioBusyPtr() ) ;
        AUDIO->number_of_audio_connections++;
        return audio_plugin;
    }
    return (Task)NULL;
}


/****************************************************************************
NAME    
    AudioDisconnect

DESCRIPTION
    Disconnect audio associated with the most recent AudioConnect message.
    Not to be used when multiple audio connections are supported
    (max available audio connections is >1)
    
RETURNS
    void
*/
void AudioDisconnect(void)
{
    Task audio_plugin = audioGetPlugin();
    PanicFalse(AUDIO->maximum_audio_connections == 1);
    if ( audio_plugin )
    {
        AudioDisconnectTask(audio_plugin);
    }
    else
    {
        PRINT(("AUD: AudioDisconnect ignored, not connected\n"));
    }
}

/****************************************************************************
NAME
    AudioDisconnectTask

DESCRIPTION
    Disconnect audio associated with the specified Task

RETURNS
    void
*/
void AudioDisconnectTask(Task audio_plugin)
{
    if (audio_plugin && AUDIO->number_of_audio_connections)
    {
        /*  Cancel all volume related messages immediately.
            This stops stale volume messages being sent to a new audio connection
            that uses the same plugin. */
        MessageCancelAll(audio_plugin, AUDIO_PLUGIN_SET_VOLUME_MSG);
        MessageCancelAll(audio_plugin, AUDIO_PLUGIN_SET_GROUP_VOLUME_MSG);
        MessageCancelAll(audio_plugin, AUDIO_PLUGIN_RESET_VOLUME_MSG);
        MessageCancelAll(audio_plugin, AUDIO_PLUGIN_DELAY_VOLUME_SET_MSG);
        MessageCancelAll(audio_plugin, AUDIO_PLUGIN_SET_SOFT_MUTE_MSG);
        MessageCancelAll(audio_plugin, AUDIO_PLUGIN_ALLOW_VOLUME_CHANGES_MSG);
        
        MessageSendConditionallyOnTask (audio_plugin, AUDIO_PLUGIN_DISCONNECT_MSG, 0, AudioBusyPtr()) ;
        AUDIO->number_of_audio_connections--;
    }

    if (audio_plugin == audioGetPlugin())
    {
    audioSetPlugin(NULL);
    AUDIO->relay_plugin = NULL;
}
}


/****************************************************************************
NAME    
    AudioSetVolume

DESCRIPTION
    update the volume of any currently connected audio

RETURNS
    void
*/
void AudioSetVolume( int16 volume , int16 tone_volume)
{
    Task plugin = audioGetPlugin();

    if ( plugin )
    {
        MAKE_AUDIO_MESSAGE( AUDIO_PLUGIN_SET_VOLUME_MSG, message ) ;
    
        message->volume     = volume ;
        message->tone_volume = tone_volume;
        
        MessageSendConditionallyOnTask ( plugin, AUDIO_PLUGIN_SET_VOLUME_MSG, message, AudioBusyPtr() ) ;
        
        AUDIO->message.volume = volume;
    }
}

/****************************************************************************
NAME    
    AudioSetMode

DESCRIPTION
    Set the audio connection mode

RETURNS
    void
*/
bool AudioSetMode ( AUDIO_MODE_T mode, void * params )
{
    bool lResult = FALSE ;
    Task plugin = audioGetPlugin();
    
    if (plugin)
    {
        MAKE_AUDIO_MESSAGE( AUDIO_PLUGIN_SET_MODE_MSG, message ) ;
    
        message->mode = mode ;
        message->params = params ;
        
        MessageSendConditionallyOnTask ( plugin, AUDIO_PLUGIN_SET_MODE_MSG, message, AudioBusyPtr() ) ;
        
        lResult = TRUE ;
        AUDIO->message.mode   = mode;
    }
    
    return lResult ;   
}

/****************************************************************************
NAME    
    AudioSetInputAudioMute 

DESCRIPTION
    Mute / unmute the input audio port for all the audio sources except tones.

RETURNS
    void
*/
void AudioSetInputAudioMute (bool enable)
{    
    Task plugin = audioGetPlugin();
    if (plugin)
    {
        MAKE_AUDIO_MESSAGE( AUDIO_PLUGIN_SET_INPUT_AUDIO_MUTE_MSG, message ) ;
        memset(message, 0, sizeof(message));
    
        message->input_audio_port_mute_active = BITFIELD_CAST(1, enable) ;
        
        MessageSendConditionallyOnTask ( plugin, AUDIO_PLUGIN_SET_INPUT_AUDIO_MUTE_MSG, message, AudioBusyPtr() );
    } 
}


/****************************************************************************
NAME    
    AudioConfigureSubWoofer

DESCRIPTION
    Set the operating mode of the sub woofer

RETURNS
    void
*/
bool AudioConfigureSubWoofer(AUDIO_SUB_TYPE_T  sub_woofer_type, Sink sub_sink )
{
    bool lResult = FALSE ;
    Task plugin = audioGetPlugin();
    
    if (plugin)
    {
        MAKE_AUDIO_MESSAGE( AUDIO_PLUGIN_SET_SUB_WOOFER_MSG, message ) ;
    
        message->sub_woofer_type = sub_woofer_type ;
        message->sub_sink = sub_sink ;
        
        MessageSendConditionallyOnTask ( plugin, AUDIO_PLUGIN_SET_SUB_WOOFER_MSG, message, AudioBusyPtr() ) ;
        
        lResult = TRUE ;
    }
    
    return lResult ;       
}

/****************************************************************************
NAME	
    AudioSetSoftMute

DESCRIPTION
    Apply soft mute / unmute to either (or both) sink and subwoofer

RETURNS
    void
*/
bool AudioSetSoftMute(AUDIO_PLUGIN_SET_SOFT_MUTE_MSG_T* mute_message)
{
    bool lResult = FALSE;
    Task plugin = audioGetPlugin();
    
    if (plugin)
    {
        MAKE_AUDIO_MESSAGE( AUDIO_PLUGIN_SET_SOFT_MUTE_MSG, message );
    
        memcpy(message, mute_message, sizeof(AUDIO_PLUGIN_SET_SOFT_MUTE_MSG_T));
        
        MessageSendConditionallyOnTask ( plugin, AUDIO_PLUGIN_SET_SOFT_MUTE_MSG, message, AudioBusyPtr() );
        
        lResult = TRUE ;
    }
    
    return lResult ;     
}

/****************************************************************************
NAME    
    AudioSetRoute

DESCRIPTION
    Set the audio connection route

RETURNS
    void
*/
bool AudioSetRoute(AUDIO_ROUTE_T route, const void * params)
{
    bool lResult = FALSE ;
    Task plugin = audioGetPlugin();
    
    if (plugin)
    {
        MAKE_AUDIO_MESSAGE( AUDIO_PLUGIN_SET_ROUTE_MSG, message ) ;
    
        message->route = route ;
        message->params = params ;
        
        MessageSendConditionallyOnTask ( plugin, AUDIO_PLUGIN_SET_ROUTE_MSG, message, AudioBusyPtr() ) ;
        
        lResult = TRUE ;
        AUDIO->message.route  = route;
    }
    
    return lResult ;   
}

/****************************************************************************
NAME    
    AudioStartForwarding

DESCRIPTION
    Start forwarding undecoded audio frames to specified sink

RETURNS
    void
*/
bool AudioStartForwarding(Task relay_plugin, Sink forwarding_sink, bool content_protection, peer_buffer_level peer_dsp_required_buffering_level)
{
    bool lResult = FALSE ;
    Task plugin = audioGetPlugin();
    
    if ( plugin && ((AUDIO->relay_plugin == NULL) || (AUDIO->relay_plugin == relay_plugin)))
    {
        MAKE_AUDIO_MESSAGE( AUDIO_PLUGIN_START_FORWARDING_MSG, message ) ;
    
        message->forwarding_sink = forwarding_sink ;
        message->content_protection = content_protection ;
        message->peer_dsp_required_buffering_level = peer_dsp_required_buffering_level ;

        AUDIO->relay_plugin = relay_plugin;
        AUDIO->forwarding_sink = forwarding_sink;
        AUDIO->content_protection = BITFIELD_CAST(1, content_protection);

        MessageSendConditionallyOnTask ( AUDIO->relay_plugin, AUDIO_PLUGIN_START_FORWARDING_MSG, message, AudioBusyPtr() ) ;
        
        lResult = TRUE ;
    }
    
    return lResult ;   
}


/****************************************************************************
NAME    
    IsAudioRelaying

DESCRIPTION
    Query whether the  plugin used for the stream relay operation is available

RETURNS
    void
*/
bool IsAudioRelaying(void)
{
    if(AUDIO->relay_plugin != NULL)
        return TRUE;
    else
        return FALSE;
}

/****************************************************************************
NAME    
    AudioStopForwarding

DESCRIPTION
    Stop forwarding of undecoded audio frames

RETURNS
    void
*/
void AudioStopForwarding(void)
{
    Task plugin = audioGetPlugin();

    if ( plugin && AUDIO->relay_plugin)
    {
        MessageSendConditionallyOnTask ( AUDIO->relay_plugin, AUDIO_PLUGIN_STOP_FORWARDING_MSG, NULL, AudioBusyPtr() ) ;
        
        AUDIO->relay_plugin = NULL ;
    }
}

/****************************************************************************
NAME    
    AudioPlayTone

DESCRIPTION

    queues the tone if can_queue is TRUE and there is already a tone playing

RETURNS 
    
*/
void AudioPlayTone ( const ringtone_note * tone , bool can_queue ,  int16 tone_volume , AudioPluginFeatures features )
{       
    if (!IsAudioBusy() || can_queue)
    {
        Task plugin = audioGetPlugin();

        MAKE_AUDIO_MESSAGE( AUDIO_PLUGIN_PLAY_TONE_MSG, message ) ;
    
        message->tone        = tone;
        message->can_queue   = can_queue ;
        message->tone_volume = tone_volume  ;
        message->features    = features ;
    
        if(plugin)
        {
            PRINT(("AUDIO: play tone, plugin = %p \n",(void*)plugin));
            MessageSendConditionallyOnTask ( plugin, AUDIO_PLUGIN_PLAY_TONE_MSG, message, AudioBusyPtr() ) ;
        }
        else
        {
            PRINT(("AUDIO: play tone, no plugin \n"));
            /* Forward message to the Voice Prompts plugin as the DSP is required for multi-channel tones */
            MessageSendConditionallyOnTask( (TaskData*)&csr_voice_prompts_plugin, AUDIO_PLUGIN_PLAY_TONE_MSG, message, AudioBusyPtr() ) ;
        }
    }
    else
    {
        PRINT(("AUDIO: discard tone \n"));
    }
}

/****************************************************************************
NAME    
    AudioStopToneAndPrompt

DESCRIPTION
    Stop a tone from playing

RETURNS
    
*/
void AudioStopToneAndPrompt (bool prompt_terminate) 
{
    Task audio_plugin = AudioGetActivePlugin();
    Task tone_plugin = AudioGetTonePlugin();
    Task current_tone_handler = audio_plugin ? audio_plugin : tone_plugin;

    PRINT(("AUDIO: STOP tone, busy = %p\n",(uint16)AUDIO->AUDIO_BUSY));
    if(IsTonePlaying() || IsVpPlaying())
    {
        /* Cancel any pending tones */
        MessageCancelAll(audio_plugin ,AUDIO_PLUGIN_PLAY_TONE_MSG);
        MessageCancelAll(tone_plugin ,AUDIO_PLUGIN_PLAY_TONE_MSG);
        
        /* Cancel audio prompts if prompt_terminate is true */
        if(prompt_terminate)
        {                   
            /* Make sure we cancel any pending Audio Prompts */
            MessageCancelAll(audio_plugin, AUDIO_PLUGIN_PLAY_AUDIO_PROMPT_MSG);
            MessageCancelAll(tone_plugin, AUDIO_PLUGIN_PLAY_AUDIO_PROMPT_MSG);
        }
        
        /* Send the stop message when tone is playing or prompt_terminate is true */
        if( prompt_terminate || IsTonePlaying())
        {
            MessageSend(current_tone_handler, AUDIO_PLUGIN_STOP_TONE_AND_PROMPT_MSG, 0);
        }
    }
}

/****************************************************************************
NAME    
    AudioPlayAudioPrompt
    
DESCRIPTION

    plays / queues the Audio Prompt if already Playing or busy. 

RETURNS
    
*/
void AudioPlayAudioPrompt ( Task plugin , uint16 id , uint16 language , bool can_queue , int16 ap_volume , AudioPluginFeatures features, bool override, Task app_task )
{   
    if ((uint16*)AUDIO->AUDIO_BUSY == NULL || can_queue) 
    {
        Task audio_plugin = audioGetPlugin();
        MAKE_AUDIO_MESSAGE( AUDIO_PLUGIN_PLAY_AUDIO_PROMPT_MSG, message ) ;
        
        message->id          = id ;
        message->language    = language ;
        message->can_queue   = can_queue ;
        message->app_task    = app_task ;
        message->ap_volume   = ap_volume  ;
        message->features    = features ;
        
        if (audio_plugin)
        {
            /* if this voice prompt is set to play immediately, cancel any queued prompts */
            if(override)
            {
                audioCancelMessage(plugin, AUDIO_PLUGIN_PLAY_AUDIO_PROMPT_MSG);
                MessageCancelAll( plugin, AUDIO_PLUGIN_CONNECT_MSG);
            }
    
            /* determine whether voice prompts can be mixed with dsp audio, to do this 
               voice prompts must be in adpcm or pcm format and the audio plugin must be capable
               of supporting tone mixing via kalimba port 3 (tone port) */
            /* OR if the audio plugin connect message is in the queue but not yet actioned, queue
               the Audio Prompt message until the audio plugin is loaded, then decide what to do with it */
            if((GetCurrentDspStatus() || !IsAudioInUse()) &&
               (CsrVoicePromptsIsMixable(id, language)))
            {
                PRINT(("AUD play Audio Prompt via DSP mixing\n"));
                MessageSendConditionallyOnTask ( plugin , AUDIO_PLUGIN_PLAY_AUDIO_PROMPT_MSG ,message , AudioBusyPtr() ) ;        
            }
            /* audio is connected via audio plugin and not using the DSP or not ADPCM voice prompts
               so need to disconnect current plugin prior to playing voice prompts */
            else
            {
            
                PRINT(("AUD play -Audio Prompt disconnect audio\n"));
                /*if we have a plugin connected, then perform the disconnect*/
                MessageSendConditionallyOnTask ( audio_plugin, AUDIO_PLUGIN_DISCONNECT_MSG , 0 , AudioBusyPtr() ) ;
                MessageSendConditionallyOnTask ( plugin , AUDIO_PLUGIN_PLAY_AUDIO_PROMPT_MSG , message , AudioBusyPtr() ) ;           
        
                /* Queue message to reconnect plugin */
                {
                    MAKE_AUDIO_MESSAGE( AUDIO_PLUGIN_CONNECT_MSG, connect_message ) ;
                    *connect_message = AUDIO->message;
                    MessageSendConditionallyOnTask ( audio_plugin, AUDIO_PLUGIN_CONNECT_MSG , connect_message , AudioBusyPtr() ) ;
                }    
    
                /*  Start the reconnected plugin with all audio groups muted.
                    When the application receives the following AUDIO_PLUGIN_REFRESH_VOLUME message it should
                    restore the the correct mute status for all the groups using AudioSetSoftMute().
                    If, for example, the mic was muted in the audio plugin before the audio prompt was played,
                    this ensures the mic remains muted until the application has chance to update the mute status.
                */
                {
                    MAKE_AUDIO_MESSAGE( AUDIO_PLUGIN_SET_SOFT_MUTE_MSG, soft_mute_message ) ;
                    soft_mute_message->mute_states = AUDIO_MUTE_ENABLE_ALL;
                    soft_mute_message->unused = 0;
                    MessageSendConditionallyOnTask ( audio_plugin, AUDIO_PLUGIN_SET_SOFT_MUTE_MSG , soft_mute_message , AudioBusyPtr() ) ;
                } 
                
                /* Request that the app refreshes the volume and mute status after reconnect */
                {
                    PRINT(("AUD:Vol refresh\n"));
                    MessageSendConditionallyOnTask(app_task, AUDIO_PLUGIN_REFRESH_VOLUME, NULL, AudioBusyPtr());
                }
    
                if ( NULL != AUDIO->relay_plugin)
                {
                    MAKE_AUDIO_MESSAGE( AUDIO_PLUGIN_START_FORWARDING_MSG, start_forwarding_message ) ;
    
                    start_forwarding_message->forwarding_sink = AUDIO->forwarding_sink ;
                    start_forwarding_message->content_protection = AUDIO->content_protection ;
    
                    MessageSendConditionallyOnTask ( AUDIO->relay_plugin, AUDIO_PLUGIN_START_FORWARDING_MSG, start_forwarding_message , AudioBusyPtr()) ;
                }
            }
        }
        else
        {
            PRINT(("AUD play Audio Prompt no plugin\n"));
            /* if this voice prompt is set to play immediately, cancel any queued prompts */
            if(override)
                audioCancelMessage(plugin, AUDIO_PLUGIN_PLAY_AUDIO_PROMPT_MSG);
            
            MessageSendConditionallyOnTask ( plugin , AUDIO_PLUGIN_PLAY_AUDIO_PROMPT_MSG ,message , AudioBusyPtr() ) ;        
        }   
    }
    else
    {
        PRINT(("AUDIO: discard Audio Prompt\n"));
    }
}

/****************************************************************************
NAME    
    AudioVoicePromptsInit

DESCRIPTION
    Initialises the voice prompts indexing. The data block pointed to by index should *NOT* be freed
    after calling this function, the plugin will refer to this block once it has been 
    initialised.

RETURNS
*/
void AudioVoicePromptsInit( TaskData * plugin, uint16 number_of_prompts, uint16 number_of_languages)
{
    MAKE_AUDIO_MESSAGE( AUDIO_PLUGIN_VOICE_PROMPTS_INIT_MSG, message );
    message->number_of_prompts   = number_of_prompts;
    message->number_of_languages = number_of_languages;
    MessageSend (plugin, AUDIO_PLUGIN_VOICE_PROMPTS_INIT_MSG, message) ;
}

/****************************************************************************
NAME    
    AudioSetPower

DESCRIPTION
        This can be used to change the power level of an underlying plugin and this 
        behaviour is not supported by all plugin vendors.

RETURNS
    
*/
void AudioSetPower(AUDIO_POWER_T power)
{
    Task plugin = audioGetPlugin();

    if (plugin)
    {
        MAKE_AUDIO_MESSAGE( AUDIO_PLUGIN_SET_POWER_MSG, message ) ;
    
        message->power = power ;
        /* NB. Don't update AUDIO.message or we may end up with no DSP and metadata enabled */
        MessageSend (plugin, AUDIO_PLUGIN_SET_POWER_MSG, message ) ;
    }

}

/****************************************************************************
NAME    
    AudioMicSwitch

DESCRIPTION
    Swap between the two microphone inputs in Production Test

RETURNS
	
*/
static void sendProductionTestCommand(audio_plugin_interface_message_type_t message_type)
{
    Task plugin = audioGetPlugin();
    if(plugin)
    {
        MessageSendConditionallyOnTask ( plugin, message_type, 0, AudioBusyPtr() ) ;
    }
}

void AudioMicSwitch ( void ) 
{
    PRINT(("AUD: MicSwitch\n") );
	
    sendProductionTestCommand(AUDIO_PLUGIN_MIC_SWITCH_MSG);
}

void AudioOutputSwitch(void)
{
    PRINT(("AUD: OutputSwitch\n"));

    sendProductionTestCommand(AUDIO_PLUGIN_OUTPUT_SWITCH_MSG);
}

/****************************************************************************
NAME    
    AudioStartASR

DESCRIPTION
    Start or restart the ASR algorithm

RETURNS
    
*/
bool AudioStartASR ( AUDIO_MODE_T mode ) 
{
    bool lResult = FALSE ;
    Task plugin = audioGetPlugin();

    PRINT(("AUD: AudioStartASR"));

    if (plugin)
    {
        MAKE_AUDIO_MESSAGE( AUDIO_PLUGIN_START_ASR, message ) ;
    
        message->mode = mode ;

        MessageSendConditionallyOnTask ( plugin, AUDIO_PLUGIN_START_ASR, message, AudioBusyPtr() ) ;

        lResult = TRUE ;
    }
    return lResult;
}


/****************************************************************************
NAME    
    IsAudioBusy

DESCRIPTION
    query the current status of the audio library

RETURNS
    A boolean indicating if audio_busy task exists
*/
bool IsAudioBusy(void)
{
    if(AUDIO == NULL)
        return FALSE;
    
    PRINT(("AUD: IsAudioBusy = %s \n",AUDIO->AUDIO_BUSY?"TRUE":"FALSE")) ;
    return AUDIO->AUDIO_BUSY == NULL ? FALSE : TRUE;
}


/****************************************************************************
NAME
    AudioBusyTask

DESCRIPTION
    query the current status of the audio library

RETURNS
    the audio_busy task
*/
Task AudioBusyTask(void)
{
    PRINT(("AUD: IsAudioBusyTask = %s \n",AUDIO->AUDIO_BUSY?"TRUE":"FALSE"));
    return AUDIO->AUDIO_BUSY;
}

/****************************************************************************
NAME    
    AudioBusyPtr

DESCRIPTION
    get pointer to AUDIO_BUSY for use with messageSendConditionally
RETURNS
    pointer to current audio_busy task value
*/
const Task * AudioBusyPtr(void)
{   
    PRINT(("AUD: AudioBusyPtr = %s \n",AUDIO->AUDIO_BUSY?"TRUE":"FALSE") );
    return &AUDIO->AUDIO_BUSY;
}

static bool CallbackCheckRequired(TaskData *oldtask, TaskData *newtask);
static void MakeBusyCallback(TaskData *task);

/****************************************************************************
NAME    
    SetAudioBusy

DESCRIPTION
	update the current status of the audio library
        callback is required to the task losing its lock

RETURNS

*/

void SetAudioBusy(TaskData* newtask)
{
    TaskData *oldtask;

    oldtask = (TaskData *)AUDIO->AUDIO_BUSY;

    PRINT(("AUD: SetAudioBusy = %p -> %p Plugin = %p\n",
            (void *) oldtask, (void *) newtask, (void *)audioGetPlugin()));

    if (CallbackCheckRequired(oldtask, newtask))
    {
        MakeBusyCallback(oldtask);
    }

    AUDIO->AUDIO_BUSY = newtask;
}


/****************************************************************************
NAME	
	CallbackCheckRequired

DESCRIPTION
	Looks to see whether a task no longer has an audio lock

RETURNS
        TRUE if a task used to have the lock but doesn't any more
        FALSE otherwise

*/

static bool CallbackCheckRequired(TaskData *oldtask, TaskData *newtask)
{
    return (oldtask && (newtask != oldtask)) ? TRUE : FALSE ;
}


/****************************************************************************
NAME	
	MakeBusyCallback

DESCRIPTION
	Makes a callback to the task losing the audio lock.

        This is useful because we can't always rely on all tasks to play nicely
        with the audio lock.  This mechanism provides a means for a task to
        reset its state when another task either grabs the busy lock or frees
        it when it shouldn't have.

        Hard coded callback functions have to be used instead of more elegant
        options involving either a table of function pointers or getting tasks
        to register their own callback.  The reason is that the ADK stack-size
        calculator assumes that any function pointer can hold the value of
        _any_ function whose address has ever been taken, which inevitably
        leads to the assumed possibility of recursion and a failure in the
        stack size calculation.

RETURNS

*/

static void MakeBusyCallback(TaskData *oldtask)
{
    UNUSED(oldtask);

    if (oldtask == &csr_voice_prompts_plugin)
    {
        PRINT(("AUD: Calling CsrVoicePromptsPluginCleanup()\n"));
        CsrVoicePromptsPluginCleanup();
    }
}

/****************************************************************************
NAME    
    GetCurrentDspStatus

DESCRIPTION
    query the current dsp status of the audio library

RETURNS
    pointer to current dsp status value
*/
DSP_STATUS_INFO_T GetCurrentDspStatus(void)
{
    return AUDIO->dsp_status;
}

/****************************************************************************
NAME    
    SetCurrentDspStatus

DESCRIPTION
    update the current dsp status of the audio library

RETURNS
    
*/
void SetCurrentDspStatus(DSP_STATUS_INFO_T status)
{
    AUDIO->dsp_status = status;
}

/****************************************************************************
NAME    
    IsAudioInUse

DESCRIPTION
    query whether the audio sub system is currently in use

RETURNS
    true or false status
*/
bool IsAudioInUse(void)
{
    PRINT(("AUD: IsAudioInUse = %s \n",AUDIO->audio_in_use?"TRUE":"FALSE"));
    return AUDIO->audio_in_use;
}

/****************************************************************************
NAME    
    SetAudioInUse

DESCRIPTION
    update the current audio in use state

RETURNS
    
*/
void SetAudioInUse(bool status)
{
    AUDIO->audio_in_use = BITFIELD_CAST(1, status);
    PRINT(("AUD: SetAudioInUse = %s \n",AUDIO->audio_in_use?"TRUE":"FALSE"));
}

/****************************************************************************
NAME    
    AudioSetActivePlugin

DESCRIPTION
    It is the responsibility of the plugin libraries to call this function when handling
    AUDIO_CONNECT_MSG to set its value, and, when handling AUDIO_DISCONNECT_MSG to clear its value.
    The corresponding accessor function can then be used to check the current plugin in use.
    If a plugin library does check the state of the currently active plugin then there is no
    requirement to call this function.

RETURNS
    void

*/
void AudioSetActivePlugin(Task plugin)
{
    if (AUDIO)
        AUDIO->active_plugin = plugin;

}

/****************************************************************************
NAME
    AudioSetRelayPlugin

DESCRIPTION
    update the current relay_plugin variable

RETURNS

*/
void AudioSetRelayPlugin(Task plugin)
{
    if (AUDIO)
        AUDIO->relay_plugin = plugin;
}



/****************************************************************************
NAME
    IsTonePlaying

DESCRIPTION
    query whether the tone playing is currently in use

RETURNS
    true or false status
*/
bool IsTonePlaying(void)
{
    return AUDIO->tone_playing;
}

/****************************************************************************
NAME    
    SetTonePlaying

DESCRIPTION
    update the current tone playing in use state

RETURNS
    
*/
void SetTonePlaying(bool status)
{
    AUDIO->tone_playing = BITFIELD_CAST(1, status);
}

/****************************************************************************
NAME    
    IsVpPlaying

DESCRIPTION
    query whether the voice prompt system is currently in use

RETURNS
    true or false status
*/
bool IsVpPlaying(void)
{
    return AUDIO->vp_playing;
}

/****************************************************************************
NAME    
    SetVpPlaying

DESCRIPTION
    update the current voice playing state 

RETURNS
    
*/
void SetVpPlaying(bool status)
{
    AUDIO->vp_playing = BITFIELD_CAST(1, status);
}

/****************************************************************************
NAME    
    IsAsrPlaying

DESCRIPTION
    query whether the asr system is currently in use

RETURNS
    true or false status
*/
bool IsAsrPlaying(void)
{
    return AUDIO->asr_running;
}

/****************************************************************************
NAME    
    SetAsrPlaying

DESCRIPTION
    update the current asr state 

RETURNS
    
*/
void SetAsrPlaying(bool status)
{
    AUDIO->asr_running = BITFIELD_CAST(1, status);
}

/****************************************************************************
NAME    
    AudioGetPlugin

DESCRIPTION
    This method returns the audio plugin associated with the most recent AudioConnect message.
    The message has not necessarily been processed, so it may be that it does not point to the
    currently active plugin. It is intentionally made static as only the audio library should
    be accessing this data. If an audio plugin requires to know the value of audio plugin then
    a the function AudioGetActivePlugin() should be used.
    The primary use of this call is to get the plugin to be used for sending messages to.

RETURNS
    
*/
static Task audioGetPlugin(void)
{
    if (AUDIO)
        return AUDIO->plugin;
    else
        return (Task)NULL;
}

static void audioSetPlugin(Task plugin)
{
    if (AUDIO)
        AUDIO->plugin = plugin;
}

/****************************************************************************
NAME
    AudioIsLastConnectedPlugin

DESCRIPTION
    This method compares the supplied parameter with the plugin that the last AudioConnect()
    call was made to, and returns true if they match.

RETURNS
    bool

*/
bool AudioIsLastConnectedPlugin(Task plugin_to_check)
{
    if(AUDIO && audioGetPlugin() == plugin_to_check)
        return TRUE;
    return FALSE;
}

/****************************************************************************
NAME
    AudioGetActivePlugin

DESCRIPTION
    This method returns the audio plugin associated with the currently active audio plugin.
    This function should be used by external libraries to determine the currently active plugin.

RETURNS

*/
Task AudioGetActivePlugin(void)
{
    if (AUDIO)
        return AUDIO->active_plugin;
    else
        return NULL;
}

/****************************************************************************
NAME
    AudioGetTonePlugin

DESCRIPTION
   Returns tone/voice prompt plugin

RETURNS

*/
Task AudioGetTonePlugin(void)
{
    return (Task)&csr_voice_prompts_plugin;
}

/****************************************************************************
NAME
	AudioSetGroupVolume

DESCRIPTION
	method to send A2DP volume info to DSP

RETURNS

*/
void AudioSetGroupVolume(AUDIO_PLUGIN_SET_GROUP_VOLUME_MSG_T *msg)
{
    Task plugin = audioGetPlugin();
    if(plugin)
    {
        MAKE_AUDIO_MESSAGE( AUDIO_PLUGIN_SET_GROUP_VOLUME_MSG, message ) ;
        memcpy(message, msg, sizeof(AUDIO_PLUGIN_SET_GROUP_VOLUME_MSG_T));
        MessageSendConditionallyOnTask ( plugin, AUDIO_PLUGIN_SET_GROUP_VOLUME_MSG, message, AudioBusyPtr() ) ;
        
        AUDIO->message.volume = msg->main.master;
    }
}

/****************************************************************************
NAME
    AudioSetMaximumConcurrentAudioConnections

DESCRIPTION
    Sets the number of concurrent audio connections the application supports.
    Note: the library must also be able to support this number.

RETURNS

*/
void AudioSetMaximumConcurrentAudioConnections(unsigned connections)
{
    PanicNull(AUDIO);
    AUDIO->maximum_audio_connections = connections;
}

/****************************************************************************
NAME    
    AudioLibraryTestReset

DESCRIPTION
    Reset the audio library an audio_plugin if requested. Note that plug-in must
    support the message AUDIO_PLUGIN_TEST_RESET_MSG for this to work. This is
    only intended for unit test and will panic if called in a release build.

RETURNS
    void
*/
void AudioLibraryTestReset(Task audio_plugin)
{
#ifndef AUDIO_TEST_BUILD
    (void)audio_plugin;
    Panic();
#else
    if(audio_plugin != NULL)
    {
        PRINT(("Reset Plugin %lx\n", (uint32)audio_plugin));
        MessageFlushTask(audio_plugin);
        MessageSend(audio_plugin, AUDIO_PLUGIN_TEST_RESET_MSG, NULL);
    }
    
    if(AUDIO)
    {
        PRINT(("Reset Audio\n"));
        free(AUDIO);
        AUDIO = NULL;
    }
#endif
}
