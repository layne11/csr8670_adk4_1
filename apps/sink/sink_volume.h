/*
Copyright (c) 2004 - 2016 Qualcomm Technologies International, Ltd.
*/

/*!
@file
@ingroup sink_app
*/

#ifndef SINK_VOLUME_H
#define SINK_VOLUME_H


#include "sink_volume.h"
#include "sink_events.h"
#include <hfp.h>
#include <audio_plugin_if.h>

#define VOL_NUM_VOL_SETTINGS     (16)


typedef enum  
{
    increase_volume,
    decrease_volume,
    same_volume
}volume_direction;

typedef enum
{
    tws_none,
    tws_master,
    tws_slave
} tws_device_type;

typedef struct
{
    int16 main_volume;
    int16 aux_volume;
}volume_info;

#define MAX_A2DP_CONNECTIONS        2        


typedef struct
{
    volume_info a2dp_volume[MAX_A2DP_CONNECTIONS];
    volume_info analog_volume; 
    volume_info spdif_volume; 
    volume_info usb_volume; 
    volume_info fm_volume; 
}volume_levels_t;   
    

#define MICROPHONE_MUTE_ON        0
#define MICROPHONE_MUTE_OFF       10

#define VOLUME_A2DP_MIN_LEVEL 0

#define VOLUME_HFP_MAX_LEVEL 15
#define VOLUME_HFP_MIN_LEVEL 0

#define VOLUME_NUM_VOICE_STEPS 16

/* Wait period between USB Audio event creation */
#define VOLUME_USB_EVENT_WAIT (1500)

#define sinkVolumeGetGroupConfig(group) theSink.conf6->volume_config.volume_control_config.group_config[group]
#define sinkVolumeGetTwsSlaveTrim()     theSink.conf6->volume_config.volume_control_config.device_trim_slave
#define sinkVolumeGetTwsMasterTrim()    theSink.conf6->volume_config.volume_control_config.device_trim_master
#define sinkVolumeGetCvcVol(idx)        theSink.conf6->volume_config.gVolMaps[idx].VolGain

/****************************************************************************
NAME
 volumeInit

DESCRIPTION
 malloc memory for storing of current volume levels for a2dp, usb, wired and fm
 interfaces

RETURNS
 none

*/
void volumeInit(void);

/****************************************************************************
NAME 
 VolumeCheckA2dpMute

DESCRIPTION
 check whether any a2dp connections are at minimum volume and mutes them properly if they are

RETURNS
 bool   Returns true if stream muted
    
*/
bool VolumeCheckA2dpMute(void);


/****************************************************************************
NAME 
 VolumeSetHeadsetVolume

DESCRIPTION
 sets the internal speaker gain to the level corresponding to the phone volume level
    
RETURNS
 void
    
*/
void VolumeSetHeadsetVolume( uint16 pNewVolume , bool pPlayTone, hfp_link_priority priority );


/****************************************************************************
NAME 
 VolumeSendAndSetHeadsetVolume

DESCRIPTION
    sets the vol to the level corresponding to the phone volume level
    In addition - send a response to the AG indicating new volume level

RETURNS
 void
    
*/
void VolumeSendAndSetHeadsetVolume( uint16 pNewVolume , bool pPlayTone , hfp_link_priority priority );

/****************************************************************************
NAME 
 VolumeIfAudioRoutedModifyAndUpdateTWSDeviceTrim

DESCRIPTION
 check for an active A2DP, Wired or USB streaming currently routing audio to the device and adjust the trim volume up or down
 as appropriate

RETURNS
 bool   TRUE if volume adjusted, FALSE if no streams found
    
*/
bool VolumeIfAudioRoutedModifyAndUpdateTWSDeviceTrim(const volume_direction dir, const tws_device_type tws_device);

/****************************************************************************
NAME 
 VolumeModifyAndUpdateTWSDeviceTrim

DESCRIPTION
 Updates the trim volume changes locally in the device.

RETURNS
void
    
*/
void VolumeModifyAndUpdateTWSDeviceTrim(const volume_direction dir, const tws_device_type tws_device);

/****************************************************************************
NAME 
 VolumeHandleSpeakerGainInd

DESCRIPTION
 Handle speaker gain change indication from the AG

RETURNS
 void
    
*/
void VolumeHandleSpeakerGainInd(const HFP_VOLUME_SYNC_SPEAKER_GAIN_IND_T* ind);

/******************************************************************************
NAME
    VolumeApplySoftMuteStates

DESCRIPTION
    Send the stored soft mute state to the audio library, overriding the output
    mute states if necessary.

RETURNS
    void

*/
void VolumeApplySoftMuteStates(void);

/******************************************************************************
NAME 
    VolumeAllOutputsMuted

DESCRIPTION
    Checks if both main and aux outputs are in a muted state.

RETURNS
    bool
    
*/
bool VolumeAllOutputsMuted(void);

/******************************************************************************
NAME
    VolumeUpdateMuteState

DESCRIPTION
    Update the stored mute state for a specific mute group

RETURNS
    void

*/
void VolumeUpdateMuteState(AUDIO_MUTE_GROUP_T mute_group, AUDIO_MUTE_STATE_T state);

/******************************************************************************
NAME 
    VolumeUpdateMuteStatusAllOutputs

DESCRIPTION
    Manual override to temporarily force muting of all outputs or restore them
    to their previous mute states. This is so that other modules do not need to
    be aware of the distinction between multi-channel output groups ('main' and
    'aux'). They can simply mute everything as they always have done, but using
    the new soft mute interface rather than through mode setting. The stored
    multi-channel mute states can then be automatically restored later.

PARAMETERS
    bool enable_mute    If TRUE, mute all outputs, else restore previous states.

RETURNS
    void

*/
void VolumeUpdateMuteStatusAllOutputs(bool enable_mute);

/******************************************************************************
NAME 
    VolumeUpdateMuteStatusMicrophone

DESCRIPTION
    Mute or unmute the microphone. Convenience function which would have the
    same effect as calling VolumeUpdateMuteState(audio_mute_group_mic, state);
    the only difference being that it takes a bool parameter. Complementary
    function to VolumeUpdateMuteStatusAllOutputs(), for older code that used to
    use AUDIO_PLUGIN_SET_MODE_MSG to control the mute state of the speaker/mic.

PARAMETERS
    bool enable_mute    If TRUE, mute microphone, else unmute it.

RETURNS
    void

*/
void VolumeUpdateMuteStatusMicrophone(bool enable_mute);

/******************************************************************************
NAME 
    VolumeSetMicrophoneMute

DESCRIPTION
    Update the stored mute state for the HFP microphone and send it to the DSP.
    Will also mute all outputs if this feature is enabled.

RETURNS
    void

*/
void VolumeSetMicrophoneMute(AUDIO_MUTE_STATE_T state);


#if defined (APTX_LL_BACK_CHANNEL) || defined(INCLUDE_FASTSTREAM)
/****************************************************************************
DESCRIPTION
    Set mute or unmute of A2DP low latency back channel mic.
*/
void VolumeSetA2dpMicrophoneGain(AUDIO_MUTE_STATE_T state);

#endif /* defined (APTX_LL_BACK_CHANNEL) || defined(INCLUDE_FASTSTREAM)*/


/****************************************************************************
DESCRIPTION
    Set mute or unmute (mic gain of MICROPHONE_MUTE_ON - 0 is mute, all other
    gain settings unmute) of HFP mic.
*/
void VolumeSetHfpMicrophoneGain(hfp_link_priority priority, uint8 mic_gain);


/****************************************************************************
DESCRIPTION
    Set mute or unmute remotely from AG if SyncMic feature bit is enabled
    (mic gain of MICROPHONE_MUTE_ON - 0 is mute, all other gain settings unmute).
*/
void VolumeSetHfpMicrophoneGainCheckMute(hfp_link_priority priority, uint8 mic_gain);


/****************************************************************************
DESCRIPTION
    Sends the current HFP microphone volume to the AG on connection.
*/
void VolumeSendHfpMicrophoneGain(hfp_link_priority priority, uint8 mic_gain);


/****************************************************************************
DESCRIPTION
    Determine whether the mute reminder tone should be played in the device, e.g. #
    if AG1 is in mute state but AG2 is not muted and is the active AG then the mute reminder
    tone will not be played, when AG1 becomes the active AG it will be heard.
*/
bool VolumePlayMuteToneQuery(void);


/****************************************************************************
    Functions to increment/decrement or resend stored group volumes to the
    current audio plugin
*/
void VolumeModifyAndUpdateRoutedAudioMainVolume(const volume_direction direction);
void VolumeModifyAndUpdateRoutedA2DPAudioMainVolume(const volume_direction direction);
void VolumeUpdateRoutedAudioMainAndAuxVolume(void);

/*************************************************************************
NAME    
    storeCurrentSinkVolume
    
DESCRIPTION
    Stores the current volume level of the sink which is streaming audio (HFP or A2DP)

RETURNS

*/
void storeCurrentSinkVolume( void );

/****************************************************************************
    Send new main group volume level to currently loaded plugin
    - updates display, sub and uses previous volume to update dsp
    operating mode
*/
void VolumeSetNewMainVolume(const volume_info * const volumes, const int16 previousVolume);

/*****************************************
    Set the mute status and volume levels.
*/
void VolumeSetupInitialMutesAndVolumes(const volume_info * const volumes);

/****************************************************************************
    Set the initial mute states for audio inputs and outputs
*/
void VolumeSetInitialMuteState(void);

/****************************************************************************
    Processes user and system events specific to volume
*/
bool sinkVolumeProcessEventVolume(const MessageId EventVolume);

/****************************************************************************
    Queries whether the given event requires a reset of the source and
    volume data save timer
*/
bool sinkVolumeEventRequiresSessionDataToBeStored(const MessageId EventVolume);

/****************************************************************************
    Returns a modified event based on the volume orientation setting
*/
MessageId sinkVolumeModifyEventAccordingToVolumeOrientation(const MessageId EventVolume);

/****************************************************************************
    Resets the timer that triggers a saving of the session data
*/
void sinkVolumeResetVolumeAndSourceSaveTimeout(void);

#define sinkVolumeGetNumberOfVolumeSteps(group) (sinkVolumeGetGroupConfig(group).no_of_steps)
#define sinkVolumeGetMaxVolumeStep(group) ((sinkVolumeGetNumberOfVolumeSteps(group) - 1))

#endif

