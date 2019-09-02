/*
Copyright (c) 2004 - 2016 Qualcomm Technologies International, Ltd.
*/
/**
\file
\ingroup sink_app
*/

#ifndef _SINK_AUDIO_ROUTING_H_
#define _SINK_AUDIO_ROUTING_H_


typedef enum
{
    audio_source_none = 0,
    audio_source_FM,
    audio_source_ANALOG,
    audio_source_SPDIF,
    audio_source_I2S,
    audio_source_USB,
    audio_source_AG1,
    audio_source_AG2,
    audio_source_end_of_list
}audio_sources;

typedef enum
{
    audio_gate_none       = 0x00,
    audio_gate_call       = 0x01,
    audio_gate_party      = 0x02,
    audio_gate_a2dp       = 0x04,
    audio_gate_usb        = 0x08,
    audio_gate_wired      = 0x10,
    audio_gate_fm         = 0x20,
    audio_gate_multimedia = 0x3E,   /* All but audio_gate_call, audio_gate_sco and audio_gate_relay */
    audio_gate_sco        = 0x40,
    audio_gate_relay      = 0x80,
    audio_gate_noncall    = 0x8E,   /* All but audio_gate_call */
    audio_gate_all        = 0xFF
}audio_gating;

/* intended as a clean mechanism to determine what audio is routed */
typedef enum
{
    audio_route_none            	= 0x00,
    audio_route_a2dp_primary		= 0x01,
    audio_route_a2dp_secondary		= 0x02,
    audio_route_hfp_primary			= 0x04,
    audio_route_hfp_secondary		= 0x08,
    audio_route_usb					= 0x10,
    audio_route_analog				= 0x20,
    audio_route_spdif				= 0x40,
    audio_route_fm					= 0x80,
    audio_route_i2s                 = 0x100
} audio_route_available;

audio_gating audioGateAudio (uint16 audio_gated_mask);
audio_gating audioUngateAudio (uint16 audio_ungated_mask);

typedef struct
{
    Task audio_plugin;
    Sink audio_sink;
    AUDIO_SINK_T sink_type;
    int16 volume;
    uint32 rate;
    AudioPluginFeatures features;
    AUDIO_MODE_T mode;
    AUDIO_ROUTE_T route;
    AUDIO_POWER_T power;
    void * params;
    Task app_task;
} audio_connect_parameters;

/****************************************************************************
NAME    
    audioSwitchToAudioSource
    
DESCRIPTION
	Switch audio routing to the source passed in, it may not be possible
    to actually route the audio at that point until the audio sink becomes
    available. 
    
RETURNS
    None
*/
void audioSwitchToAudioSource(audio_sources source);

/****************************************************************************
NAME
    audioUpdateAudioRouting

DESCRIPTION
	Handle the routing of audio sources based on current status and other
	priority features like Speech Recognition, TWS and others.

RETURNS
    None
*/
void audioUpdateAudioRouting(void);

/****************************************************************************
NAME    
    audioDisconnectActiveSink
    
DESCRIPTION
    Disconnect the active audio sink

RETURNS
    
*/
void audioDisconnectActiveAudioSink(void);

void audioDisconnectActiveVoiceSink(void);

/****************************************************************************
NAME    
    sinkAudioRouteAvailable
    
DESCRIPTION
    returns which audio source is routed. Only the route of highest priority is 
    returned. The priority starting at the top of the enum audio_route_available
    
RETURNS
    audio_route_available
*/
audio_route_available sinkAudioRouteAvailable(void);

/****************************************************************************
NAME    
    audioSuspendDisconnectAllA2dpMedia
    
DESCRIPTION
    called when the SUB link wants to use an ESCO link, there is not enough
    link bandwidth to support a2dp media and esco links so suspend or disconnect
    all a2dp media streams
    
RETURNS
    true if audio disconnected, false if no action taken
*/
bool audioSuspendDisconnectAllA2dpMedia(void);

/****************************************************************************
NAME    
    audioA2dpStartStream
    
DESCRIPTION
    Start the A2DP Audio stream request from the application.
    
RETURNS
    None
*/
void audioA2dpStartStream(void);

/*************************************************************************
NAME
    processEventUsrSelectAudioSource

DESCRIPTION
    Function to handle the user's source selection. Follow up calls and
    configuration settings status will determine the outcome and proceed
    by indicating the event or not.

INPUTS
    Source selection user events (i.e. EventUsrSelectAudioSourceAnalog).

RETURNS
	TRUE if routing was successful and VM isn't in deviceLimbo state.
	FALSE if routing was not possible or VM is in deviceLimbo state.

*/
bool processEventUsrSelectAudioSource(const MessageId EventUsrSelectAudioSource);

/*************************************************************************
NAME
	isAudioGated

DESCRIPTION


INPUTS
	audio_gated_mask

RETURNS
	bool

*/
bool isAudioGated (audio_gating audio_gated_mask);

/****************************************************************************
NAME    
    audioSuspendDisconnectAudioSource
    
DESCRIPTION
    determines source of sink passed in and decides whether to issue a suspend or
    not based on source type, an audio disconnect is performed thereafter regardless
    of wether or not the source requires a suspend
    
RETURNS
    true if audio disconnected, false if no action taken
*/
void audioSuspendDisconnectAudioSource(void);

void audioRouteSpecificA2dpSource(audio_sources a2dp_source);
        
#endif /* _SINK_AUDIO_ROUTING_H_ */

