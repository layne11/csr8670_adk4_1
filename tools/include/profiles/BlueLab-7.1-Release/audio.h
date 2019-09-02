/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    audio.h
    
DESCRIPTION
    header file for the audio library	
*/

/*!
\defgroup audio audio
\ingroup vm_libs

\brief  Header file for the audio library.
\section audio_intro INTRODUCTION
    This defines the Application Programming interface to the audio library.
    
    i.e. the interface between the VM application and the audio library.
        
    see the audio_plugin_if documentation for details of the API between the 
    audio library and an underlying audio plugin.
*/

/*@{*/

#ifndef _AUDIO_H_
#define _AUDIO_H_

#include <message.h>
#include <stream.h> /*for the ringtone_note*/
#include "audio_plugin_if.h"

/****************************************************************************
NAME	
	AudioLibraryInit

DESCRIPTION
	This function mallocs the memory to be used by the audio library 

RETURNS
	void
*/
void AudioLibraryInit (  void );


/*!
    @brief Connects an audio stream to the underlying audio plugin 
    
    @param audio_plugin The audio plugin to use for the audio connection
    @param audio_sink The Sink to connect (may be synchronous AV or other) 
    @param sink_type The type of audio connection required - see AUDIO_SINK_T
    @param volume The volume at which to connect the audio plugin. 
                  The plugin vendor is responsible for the use of this value    
    @param stereo Whether or not a stereo connection is to be used (channel A or channel A & B)
    @param mode The mode to set the audio connection to
                This behaviour is plugin dependent
    @param route The route the audio will take (internal, i2s, spif)
                This behaviour is plugin dependent
    @param power The power level the plugin will use
                This behaviour is plugin dependent
    @param params Used to specify plugin specific parameters - See plugin vendor for details 
    @param app_task The main application task that should receive messages sent from the plugin.
    
    The underlying audio plugin is responsible for connecting up the audio sink
    as requested
    
    Calls to AudioConnect will be queued if the AUDIO_BUSY flag is set. 
    i.e. if a tone is currently being played, then the audio connection will occur
    once the tone has completed.
      
*/
Task AudioConnect(Task audio_plugin,
                  Sink audio_sink , 
				  AUDIO_SINK_T sink_type , 
				  int16 volume,
				  uint32 rate  , 
				  AudioPluginFeatures features , 
				  AUDIO_MODE_T mode , 
				  AUDIO_ROUTE_T route , 
				  AUDIO_POWER_T power ,
				  void * params , 
				  Task app_task);

/*!
    @brief Disconnects the audio stream most recently connected using AudioConnect
    
    The most recent audio_plugin connected using AudioConnect()
    will be asked to perform the disconnect.
    
    Calls to AudioDisconnect when no plugin is connected will be ignored
    
*/
void AudioDisconnect(void);

/*!
    @brief Disconnects the audio stream associated with the specified Task

    The audio_plugin associated with the specified Task will be asked
    to perform the disconnect.

    Calls to AudioDisconnectTask when no plugin is connected will be ignored

*/
void AudioDisconnectTask(Task audio_plugin);

/*!
    @brief Updates the volume of any currently connected audio connection
    
    @param volume The new volume to pass to the currently connected audio plugin
    @param volume The new tone/prompt volume to pass to the currently connected audio plugin    
    
    The volume is set by the underlying audio plugin and the behaviour is specific
    to the implementation of the plugin.
    
    Some plugins may interpret this as a codec gain
    Others may choose to ignore this value etc 
    
    Note : the initial volume setting is passed in as part of AudioConnect 
*/    
void AudioSetVolume(int16 volume , int16 tone_volume);

/*!
    @brief Updates the mode of any currently connected audio connection
    
    @param mode The mode to set the audio connection to
                This behaviour is plugin dependent
    @param params Used to specify plugin specific parameters - See plugin vendor for details
    
    The mode can be used to change the connection behaviour of an underlying plugin and this 
    behaviour is not supported by all plugin vendors.
    
    This call is ignored if no plugin is currently connected
    
    Note : The mode & params are passed in as part of AudioConnect   
*/
bool AudioSetMode(AUDIO_MODE_T mode , void * params);

/****************************************************************************
NAME	
	AudioConfigureSubWoofer

DESCRIPTION
	Set the operating mode of the sub woofer

RETURNS
	void
*/
bool AudioConfigureSubWoofer(AUDIO_SUB_TYPE_T sub_woofer_type, Sink sub_sink );

/*!
    @brief Updates the mute state of the audio using soft mute
    
    @param mute_type Defines which mute type to apply
        
    This call is ignored if no plugin is currently connected
*/
bool AudioSetSoftMute(AUDIO_PLUGIN_SET_SOFT_MUTE_MSG_T* mute_message);

/*!
    @brief Updates the route of any currently connected audio connection
    
    @param route The route the audio will take (internal, i2s, spif)
                This behaviour is plugin dependent
    @param params Used to specify plugin specific parameters - See plugin vendor for details
        
    This call is ignored if no plugin is currently connected
    
    Note : The mode & params are passed in as part of AudioConnect   
*/
bool AudioSetRoute(AUDIO_ROUTE_T route, const void * params);

/*!
    @brief Starts forwarding undecoded audio frames to the specified sink
    
	@param relay_plugin Plugin that manages the relay mechanism
    @param forwarding_sink The media sink to a remote device
                           This behaviour is plugin dependent
    @param content_protection Flag indicating if content protection is enabled
    
    This call is ignored if a main plugin is not currently connected
*/
bool AudioStartForwarding(Task relay_plugin, Sink forwarding_sink, bool content_protection, peer_buffer_level peer_dsp_required_buffering_level);

/*!
    @brief Stops forwarding of undecoded audio frames
           This behaviour is plugin dependent
        
    This call is ignored if no plugin is currently connected
*/
void AudioStopForwarding(void);

/*!
    @brief Plays a tone 
    
    @param tone The tone to be played 
    @param can_queue Whether or not to queue the requested tone
    @param tone_volume The volume at which to play the tone (0 = play tone at current volume)
    @param features including Whether or not a stereo connection is to be used (channel A or channel A & B)                    
		
    Tone playback can be used when an audio connection is present or not. (plugin connected or not)
    
    When a plugin is not connected, csr_voice_prompts_plugin is used for standalone tone playback
    
    When a plugin is connected, the tone playback request will be passed to the currently connected plugin
    The underlying plugin is then responsible for connecting up the audio tone.
    Some plugins may choose to ignore tone requests in certain modes 
    
    Tone queuing can be achieved using the can_queue parameter. If this is selected, then
    tones will be queued using the audio flag AUDIO_BUSY.
    Tones will be played back in the order they are requested.       
*/

void AudioPlayTone ( const ringtone_note * tone , bool can_queue , int16 tone_volume , AudioPluginFeatures features ) ;

/*!
    @brief Stops a currently playing tone or audio prompt
    
    @param prompt_terminate If true, pending audio prompts will be cancelled and active audio prompts
                            will be ended prematurely. If false, audio prompt early termination is disabled.
                            Tones are always cancelled and ended prematurely.
    
    If a tone or audio prompt is currently connected to either the default tone plugin (csr_tone_plugin)
    or to any other connected plugin then, it can be stopped part way through.
    
    In general, this is used to end ring tones prematurely to allow fast call setup times.
    
    Note : The implementation of AudioStopToneAndPrompt is plugin specific.
    Some plugins may choose to ignore the request to stop playback of a tone
*/
void AudioStopToneAndPrompt ( bool prompt_terminate ) ;

/*!
    @brief Plays text-to-speech

    @param plugin Audio Prompt plugin to use

    @param id Identifier used by the Audio Prompt plug-in to determine which Audio Prompt to
            play, eg name caller ID

    @param language This gives us an offest from the id determined by
                language * (size_index / no_languages), where size_index and
                no_languages are taken from AudioVoicePromptsInit. 
                
    @param can_queue If this is TRUE, and Audio Prompt cannot currently be played 
                (eg tone is playing), then it will be played on 
                completion of the currently playing tone

    @param ap_volume The volume at which to play the Audio Prompt. A non-zero value will 
                      cause the Audio Prompt to played at this volume level.

    @param features including Whether or not a stereo connection is to be used (channel A or channel A & B)     						

    @param app_task The main application task that should receive messages sent from audio lib.		

    @param override  prompt is set to play immediately, cancel any queued prompts
*/
void AudioPlayAudioPrompt ( Task plugin , uint16 id ,  uint16 language , bool can_queue , int16 ap_volume , AudioPluginFeatures features, bool override, Task app_task );

/*!
    @brief Initialises the voice prompts indexing, telling the plugin where in EEPROM voice 
                prompts are located.

    @param plugin The voice prompts plugin

    @param number_of_prompts The number of voice prompts per language.
                  Each language must have the same number of voice prompts.
                  
    @param number_of_languages The number of languages supported. Voice
                  prompts are stored in the memory sequentially. For example 
                  if there are two voice prompts and three languages, the 
                  voice prompt sequence in memory will be:

                  language0_prompt0, language0_prompt1, language1_prompt0,
                  language1_prompt1, language2_prompt0, language2_prompt1
*/
void AudioVoicePromptsInit( TaskData * plugin, uint16 number_of_prompts, uint16 number_of_languages);

/*!
    @brief Updates the Power level of any currently connected audio connection
    
    @param power The power to set the audio connection to
                This behaviour is plugin dependent
    
    This can be used to change the power level of an underlying plugin and this 
    behaviour is not supported by all plugin vendors.
    
    This call is ignored if no plugin is currently connected
*/
void AudioSetPower(AUDIO_POWER_T power);

/*!
    @brief Swap between microphone inputs in Production Test
*/
void AudioMicSwitch ( void );

void AudioOutputSwitch(void);

/****************************************************************************
NAME	
	AudioStartASR

DESCRIPTION
	Start or restart the ASR algorithm

RETURNS
	
*/
bool AudioStartASR ( AUDIO_MODE_T mode ) ;
        
/****************************************************************************
NAME	
	IsAudioBusy

DESCRIPTION
	query the current status of the audio library

RETURNS
	bool indicating if Audio task exists
*/
bool IsAudioBusy(void);


/****************************************************************************
NAME
    IsAudioBusy

DESCRIPTION
    query the current status of the audio library

RETURNS
	pointer to current audio_busy task value
*/
Task AudioBusyTask(void);


/****************************************************************************
NAME	
	SetAudioBusy

DESCRIPTION
	update the current status of the audio library

RETURNS

*/
void SetAudioBusy(TaskData* task);

/****************************************************************************
NAME	
	GetCurrentDspStatus

DESCRIPTION
	query the current dsp status of the audio library

RETURNS
	pointer to current dsp status value
*/
DSP_STATUS_INFO_T GetCurrentDspStatus(void);

/****************************************************************************
NAME	
	SetCurrentDspStatus

DESCRIPTION
	update the current dsp status of the audio library

RETURNS
	
*/
void SetCurrentDspStatus(DSP_STATUS_INFO_T status);

/****************************************************************************
NAME	
	IsAudioInUse

DESCRIPTION
	query whether the audio sub system is currently in use

RETURNS
    true or false status
*/
bool IsAudioInUse(void);

/****************************************************************************
NAME	
	SetAudioInUse

DESCRIPTION
	update the current audio in use state

RETURNS
	
*/
void SetAudioInUse(bool status);

/****************************************************************************
NAME
    SetAudioPlugin

DESCRIPTION
    update the current plugin variable

RETURNS

*/
void AudioSetActivePlugin(Task plugin);

/****************************************************************************
NAME
    SetAudioRelayPlugin

DESCRIPTION
    update the current relay_plugin variable

RETURNS

*/
void AudioSetRelayPlugin(Task plugin);

/****************************************************************************
NAME	
	IsTonePlaying

DESCRIPTION
	query whether the tone playing is currently in use

RETURNS
    true or false status
*/
bool IsTonePlaying(void);

/****************************************************************************
NAME	
	SetTonePlaying

DESCRIPTION
	update the current tone playing in use state

RETURNS
	
*/
void SetTonePlaying(bool status);

/****************************************************************************
NAME	
	IsVpPlaying

DESCRIPTION
	query whether the voice prompt system is currently in use

RETURNS
    true or false status
*/
bool IsVpPlaying(void);

/****************************************************************************
NAME	
	IsAudioRelaying

DESCRIPTION
	Query whether the  plugin used for the stream relay operation is available

RETURNS
	void
*/
bool IsAudioRelaying(void);

/****************************************************************************
NAME	
	SetVpPlaying

DESCRIPTION
	update the current voice playing state 

RETURNS
	
*/
void SetVpPlaying(bool status);

/****************************************************************************
NAME	
	IsAsrPlaying

DESCRIPTION
	query whether the asr system is currently in use

RETURNS
    true or false status
*/
bool IsAsrPlaying(void);

/****************************************************************************
NAME	
	SetAsrPlaying

DESCRIPTION
	update the current asr state 

RETURNS
	
*/
void SetAsrPlaying(bool status);

/****************************************************************************
NAME	
	AudioBusyPtr

DESCRIPTION
	get pointer to AUDIO_BUSY for use with messageSendConditionally
RETURNS
	pointer to current audio_busy task value
*/
const Task * AudioBusyPtr(void);

/****************************************************************************
NAME
    AudioGetActivePlugin

DESCRIPTION
    This method should be called by the audio plugins when they process the Audio Connect message.
    A complementary accessor function is provided to allow a plugin to check which plugin is currently
    active.

RETURNS

*/
Task AudioGetActivePlugin(void);

/****************************************************************************
NAME
    AudioIsLastConnectedPlugin

DESCRIPTION
    This method compares the supplied parameter with the plugin that the last AudioConnect()
    call was made to, and returns true if they match.

RETURNS
    bool

*/
bool AudioIsLastConnectedPlugin(Task plugin_to_check);

/****************************************************************************
NAME
    AudioGetTonePlugin

DESCRIPTION
   Returns tone/voice prompt plugin

RETURNS

*/
Task AudioGetTonePlugin(void);

/****************************************************************************
NAME
    AudioSetGroupVolume

DESCRIPTION
    Set group volume in increments of 60th of dB.

RETURNS

*/
void AudioSetGroupVolume(AUDIO_PLUGIN_SET_GROUP_VOLUME_MSG_T *msg);

/****************************************************************************
NAME    
    AudioSetInputAudioMute

DESCRIPTION
    Mute / unmute the input audio port for all the audio sources except tones.

RETURNS
    void
*/
void AudioSetInputAudioMute (bool enable);


/****************************************************************************
NAME
    AudioGetLatency

DESCRIPTION
    Reads estimated or actual link latency.

RETURNS
    True if read of latency was sucessful.
*/
bool AudioGetLatency(Task audio_plugin, bool *estimated, uint16 *latency);

/****************************************************************************
NAME
    AudioGetA2DPSampleRate

DESCRIPTION
    Get A2DP smapling rate.

RETURNS
    Sampling rate.
*/
uint32 AudioGetA2DPSampleRate(void);

/****************************************************************************
NAME
    AudioGetA2DPSubwooferSampleRate

DESCRIPTION
    Get subwoofer smapling rate.

RETURNS
    Sampling rate.
*/
uint32 AudioGetA2DPSubwooferSampleRate(void);

/****************************************************************************
NAME
    AudioSetMaximumConcurrentAudioConnections

DESCRIPTION
    Sets the number of concurrent audio connections the application supports.
    Note: the library must also be able to support this number.

RETURNS

*/
void AudioSetMaximumConcurrentAudioConnections(unsigned connections);

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
void AudioLibraryTestReset(Task audio_plugin);

#endif /* _AUDIO_H_ */
