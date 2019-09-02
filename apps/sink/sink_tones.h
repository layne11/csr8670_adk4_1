/*
Copyright (c) 2004 - 2016 Qualcomm Technologies International, Ltd.
*/

/*!
@file
@ingroup sink_app
@brief
    header file which defines all of the tones which can be used in the 
    sink device application
*/

#ifndef SINK_TONES_H
#define SINK_TONES_H

/****************************************************************************
NAME    
    TonesPlayTone
    
DESCRIPTION
  	Works out the correct volume to play tones or Audio Prompts at
    
RETURNS
    void
*/
uint16 TonesGetToneVolume(bool PlayToneAtDefaultLevel);

/****************************************************************************
NAME    
    TonesPlayTone
    
DESCRIPTION
  	Playsback the tone given by the heasdsetTone_t index
    
RETURNS
    void
*/
void TonesPlayTone ( uint16 pTone , bool pCanQueue , bool PlayToneAtDefaultLevel) ;

/****************************************************************************
NAME    
    ToneTerminate
    
DESCRIPTION
  	function to terminate a ring tone prematurely.
    
RETURNS
    
*/
void ToneTerminate ( void ) ;

/****************************************************************************
NAME
    TonesGetToneVolumeDb
    
DESCRIPTION
    Works out the correct volume to play tones or Audio Prompts at in dB/60
    
RETURNS
    Volume in dB/60
*/
int16 TonesGetToneVolumeInDb(audio_output_group_t group);

/****************************************************************************
NAME    
    IsToneConfiguredForEvent
    
DESCRIPTION
    Checks if audio tone is configured for the given sink event.
 
PARAMETERS
    sink_event    - incoming sink event

RETURNS
    TRUE if audio tone is configured for the sink event
    FALSE if audio tone is not configured for the sink event
*/
bool IsToneConfiguredForEvent(sinkEvents_t sink_event);

/****************************************************************************
NAME    
    AudioTonePlayEvent
    
DESCRIPTION
    Plyas audio tone attached to the incoming sink event.
 
PARAMETERS
    sink_event           - incoming sink event
    event_can_be_queued  - Flag which indicate if this event
    					   is to be played imidiately or not

RETURNS
    void
*/
void AudioTonePlayEvent(sinkEvents_t sink_event, bool event_can_be_queued);


#endif

