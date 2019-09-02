/*
Copyright (c) 2005 - 2016 Qualcomm Technologies International, Ltd.
*/
/**
\file
\ingroup sink_app
\brief
    This file handles all Synchronous connection messages
*/


/****************************************************************************
    Header files
*/
#include "sink_private.h"
#include "sink_audio.h"
#include "sink_statemanager.h"
#include "sink_pio.h"
#include "sink_tones.h"
#include "sink_volume.h"
#include "sink_speech_recognition.h"
#include "sink_wired.h"
#ifndef HYDRACORE_TODO
#include "sink_dut.h"
#endif /* HYDRACORE_TODO */
#include "sink_display.h"
#include "sink_audio_routing.h"
#include "sink_devicemanager.h"
#include "sink_debug.h"
#include "sink_partymode.h"
#include "sink_anc.h"
#include "sink_fm.h"
#include "sink_peer.h"
#include "sink_callmanager.h"
#include "sink_swat.h"
#include "sink_avrcp.h"

#include <connection.h>
#include <a2dp.h>
#include <hfp.h>
#include <stdlib.h>
#include <audio.h>
#include <audio_plugin_if.h>
#include <sink.h>
#include <bdaddr.h>
#include <vm.h>
#ifndef HYDRACORE_TODO
#include <swat.h>
#endif /* HYDRACORE_TODO */

#ifdef DEBUG_AUDIO
#define AUD_DEBUG(x) DEBUG(x)
#else
#define AUD_DEBUG(x) 
#endif   

static audio_sources audioGetHighestPriorityAudioSourceAvailable(void);
static bool audioRouteAncIfAvailable(void);
static bool audioRouteScoOutsideOfCallIfAvailable(void);
static bool audioRouteFMIfAvailable(void);
static bool audioRouteActiveCallScoIfAvailable(void);
static bool audioRouteA2dpStreamIfAvailable(a2dp_link_priority priority);
static bool audioRouteSource(audio_sources source);
static bool audioRouteAnalogIfAvailable(void);
static bool audioRouteSpdifIfAvailable(void);
static bool audioRouteI2SIfAvailable(void);
static bool audioRouteUsbIfAvailable(void);
static audio_sources getNextSourceInSequence(audio_sources source);
static audio_sources getAudioSourceFromEventUsrSelectAudioSource(const MessageId EventUsrSelectAudioSource);
static bool audioIsAudioSourceRoutable(audio_sources source);
static bool audioIsA2dpAudioRoutable(a2dp_link_priority priority);
static audio_sources audioGetAudioSourceToRoute(void);
static bool audioIsAudioSourceEnabledByUser(audio_sources source);
static audio_sources getNextRoutableSource(audio_sources current_source);
static bool audioRouteScoSink(Sink sco_sink_to_route);
static int16 audioGetVolumeForSink(Sink sink);
static void audioRouteVoiceSource(void);
static bool isAudioSourceA2dp(audio_sources source);
static bool audioIsCallWithoutSco(void);

/*************************************************************************
NAME
    isAudioGated

DESCRIPTION
    Determines whether we should block the given source because either the
    input is gated for all states or the current state does not allow playing
    from this source.

    The state test is necessary for directly attached audio inputs because we
    have the option of whether to play them while in limbo.

INPUTS
    audio_gated_mask

RETURNS
    TRUE if the source should be gated
    FALSE otherwise

*/
bool isAudioGated (audio_gating audio_gated_mask)
{
    bool gated_in_all_states = !!(theSink.gated_audio & audio_gated_mask);
    bool is_direct_connection = (audio_gated_mask & (audio_gate_usb | audio_gate_wired));
    bool in_limbo = (stateManagerGetState() == deviceLimbo);
    bool gated;
    
    AUD_DEBUG(("isAudioGated(0x%X)  0x%X\n",audio_gated_mask,theSink.gated_audio));
    /* For audio sources which are capable of playing in Limbo we must first check
       the current status of VM.*/
    if((audio_gate_wired == audio_gated_mask) || (audio_gate_usb == audio_gated_mask))
    {
        if (gated_in_all_states)
        {
            gated = TRUE;
        }
        else if (is_direct_connection)
        {
            gated = (in_limbo && (!theSink.features.PlayUsbAndWiredInLimbo));
        }
        else
        {
            /*
             * As far as routing logic is concerned, source can be played.
             * (Note that whether or not the source is available is a separate
             * question - e.g. remote sources will not be connected in limbo)
             */
            gated = FALSE;
        }
        
        return gated;

    }
    else
        return gated_in_all_states;
}

audio_gating audioGateAudio (uint16 audio_gated_mask)
{
    theSink.gated_audio |= audio_gated_mask;
    
    AUD_DEBUG(("audioGateAudio(0x%X)  0x%X\n",audio_gated_mask,theSink.gated_audio));
    return theSink.gated_audio;
}

audio_gating audioUngateAudio (uint16 audio_ungated_mask)
{
    theSink.gated_audio &= ~audio_ungated_mask;
    
    AUD_DEBUG(("audioUngateAudio(0x%X)  0x%X\n",audio_ungated_mask,theSink.gated_audio));
    return theSink.gated_audio;
}

/****************************************************************************
NAME    
    audioSwitchToAudioSource
    
DESCRIPTION
    Switch audio routing to the source passed in, it may not be possible
    to actually route the audio at that point if audio sink for that source
    is not available at that time.

RETURNS
    None
*/
void audioSwitchToAudioSource(audio_sources source)
{   
    if(audioIsAudioSourceEnabledByUser(source))
    {
        theSink.rundata->requested_audio_source = source;
        audioUpdateAudioRouting();
    }
}

/****************************************************************************
NAME
    audioIsReadyForAudioRouting

DESCRIPTION
    Fundamental checks before we continue with routing an audio source.

RETURNS
    TRUE if checks passed, FALSE otherwise.
*/
static bool audioIsReadyForAudioRouting(void)
{
    /* invalidate at-limit status */
    theSink.vol_at_min = FALSE;
    theSink.vol_at_max = FALSE;

    /* ensure initialisation is complete before attempting to route audio */
    if(theSink.SinkInitialising)
    {
        return FALSE;
    }

    /* is the headset in device under test mode with active audio, if so
       disconnect the audio */
    if(stateManagerGetState() == deviceTestMode)
    {
        /* if audio is active in DUT mode then must disconnect this first to avoid any Panics,
           as device is still allowed to operate in DUT mode */
        AUD_DEBUG(("AUD: disconnect DUT audio\n"));
#ifndef HYDRACORE_TODO
        dutDisconnect();
#endif
        return FALSE;
    }

    return TRUE;
}

static bool audioRouteAudioFromTWSPeer(void)
{
    if(peerIsRemotePeerInCall())
    {
        if(theSink.features.TwsSingleDeviceOperation)
        {
            audioSuspendDisconnectAudioSource();
        }
        return TRUE;
    }
    if(peerIsDeviceSlaveAndStreaming() && (peerIsRemotePeerInCall() == FALSE))
    {
        audioRouteA2dpStreamIfAvailable(a2dp_pri_sec);
        return TRUE;
    }
    return FALSE;
}

static void audioRouteAudioSource(void)
{
    if(sinkCheckPartyModeAudio())
    {
        AUD_DEBUG(("AUD: party mode\n"));
    }
    else if(audioRouteAudioFromTWSPeer())
    {
        AUD_DEBUG(("AUD: Device is in TWS, is a Slave and is connected to a streaming Peer\n"));
    }
    else if(audioRouteSource(audioGetAudioSourceToRoute()))
    {
        AUD_DEBUG(("AUD: New audio source was successfully routed\n"));
    }
    else if(audioRouteScoOutsideOfCallIfAvailable())
    {
        AUD_DEBUG(("AUD: SCO found\n"));
    }
    else if(audioRouteAncIfAvailable())
    {
        AUD_DEBUG(("AUD: ANC found\n"));
    }
}

static void audioUpdateAudioActivePio(void)
{
    PioDrivePio(PIO_AUDIO_ACTIVE, (theSink.routed_audio ? TRUE : FALSE));
}

static void audioPostRoutingAudioConfiguration(void)
{
    /* Configure subwoofer audio if the correct connection is available for the current audio source,
    otherwise make correct connection to the subwoofer */
    if(theSink.routed_audio)
    {
        subwooferCheckConnection(TRUE);
    }

    /* If AG1 or AG2, deteremine if AVRCP is available for that source and
        update the currently active AVRCP connection appropriately */
    avrcpSetInstanceBasedOnSourceRequested(theSink.rundata->requested_audio_source);

    /* Make sure soft mute is correct */
    VolumeApplySoftMuteStates();

    /* Relay any AV Source stream to a connected Peer */
    peerRelaySourceStream();

    if(theSink.routed_audio)
    {
        int16 volume = audioGetVolumeForSink(theSink.routed_audio);
        displayUpdateVolume((VOLUME_NUM_VOICE_STEPS * volume)/sinkVolumeGetNumberOfVolumeSteps(audio_output_group_main));
        updateSwatVolume(volume);
    }

    audioUpdateAudioActivePio();
}

static bool activeFeaturesOverrideRouting(void)
{
    return speechRecognitionIsActive();
}


static void audioRouteVoiceSource(void)
{
    if(!audioRouteActiveCallScoIfAvailable())
    {
        audioDisconnectActiveVoiceSink();
    }
}

/****************************************************************************
NAME
    audioUpdateAudioRouting

DESCRIPTION
    Handle the routing of audio sources based on current status and other
    priority features like Speech Recognition, TWS and others.

RETURNS
    None
*/
void audioUpdateAudioRouting(void)
{
    if(!audioIsReadyForAudioRouting())
    {
        return;
    }

    AUD_DEBUG(("AUD: audioUpdateAudioRouting %x", (uint16)theSink.routed_audio));
    AUD_DEBUG((", Allocations remaining %d\n", VmGetAvailableAllocations()));

    if(activeFeaturesOverrideRouting())
    {
        audioSuspendDisconnectAudioSource();
        audioDisconnectActiveVoiceSink();
        AUD_DEBUG(("AUD: routing overridden, suspend/disconnect voice & audio\n"));
        return;
    }

    audioRouteVoiceSource();
    audioRouteAudioSource();
    
    audioPostRoutingAudioConfiguration();
}

/*************************************************************************
NAME
    audioGetHighestPriorityAudioSourceAvailable

DESCRIPTION
    Helper function to retrieve the next available source from the user
    defined sequence, without user's input.

INPUTS
    None

RETURNS
    TRUE when successful otherwise FALSE.

*/
static audio_sources audioGetHighestPriorityAudioSourceAvailable(void)
{

    audio_sources source = theSink.features.seqSourcePriority1;
    
    while(FALSE == audioIsAudioSourceRoutable(source))
    {
        source = getNextSourceInSequence(source);
        
        if(theSink.features.seqSourcePriority1 == source)
        {
            source = audio_source_none;
            break;
        }
    }
    
    return source;
}

static bool isAudioSourceA2dp(audio_sources source)
{
    return (source == audio_source_AG1 || source == audio_source_AG2);
}

static bool isAudioSourceMixableWithVoice(audio_sources source)
{
    /* simultaneous voice & a2dp unsupported */
    if(isAudioSourceA2dp(source) || (!theSink.features.enableMixingOfVoiceAndAudio))
    {
        return FALSE;
    }
    return TRUE;
}

static bool isAudioRoutingBlockedByCallIndication(void)
{
    return (audioIsCallWithoutSco() && (theSink.features.enableMixingOfVoiceAndAudio == FALSE));
}

static bool isAudioRoutingBlockedByRoutedVoice(audio_sources source)
{
    return (theSink.routed_voice && (isAudioSourceMixableWithVoice(source) == FALSE));
}

static bool isAudioRoutingPermitted(audio_sources source)
{
    return ((isAudioRoutingBlockedByRoutedVoice(source)
            || isAudioRoutingBlockedByCallIndication()) == FALSE);
}

/****************************************************************************
NAME    
    audioRouteSource
    
DESCRIPTION
    attempt to route the audio for the passed in source

RETURNS
    TRUE if audio routed correctly, FALSE if no audio available yet to route 
 */
static bool audioRouteSource(audio_sources source)
{
    bool routing_success = FALSE;

    if(isAudioRoutingPermitted(source))
    {
        switch (source)
        {
            case audio_source_FM:
                routing_success = audioRouteFMIfAvailable();
                break;

            case audio_source_ANALOG:
                routing_success = audioRouteAnalogIfAvailable();
                break;

            case audio_source_SPDIF:
                routing_success = audioRouteSpdifIfAvailable();
                break;

            case audio_source_I2S:
                routing_success = audioRouteI2SIfAvailable();
                break;

            case audio_source_USB:
                routing_success = audioRouteUsbIfAvailable();
                break;

            case audio_source_AG1:
                routing_success = audioRouteA2dpStreamIfAvailable(a2dp_primary);
                break;

            case audio_source_AG2:
                routing_success = audioRouteA2dpStreamIfAvailable(a2dp_secondary);
                break;

            case audio_source_none:
            default:
                break;
        }
    }

    /* Disconnect any currently routed sources in case requested source didn't make it.*/
    if ((routing_success == FALSE) && (theSink.routed_audio))
    {
        AUD_DEBUG(("AUD: routing, request to suspend/disconnect current source\n"));
        audioSuspendDisconnectAudioSource();
    }

    displayUpdateAudioSourceText(theSink.rundata->routed_audio_source);

    /* indicate whether audio was successfully routed or not */
    return routing_success;
}
    

static Sink audioGetActiveScoSink(void)
{
    Sink priority_sink = 0;
    
    Sink sco_sink_primary = sinkCallManagerGetHfpSink(hfp_primary_link);
    Sink sco_sink_secondary = sinkCallManagerGetHfpSink(hfp_secondary_link);
    hfp_call_state call_state_primary = sinkCallManagerGetHfpCallState(hfp_primary_link);
    hfp_call_state call_state_secondary = sinkCallManagerGetHfpCallState(hfp_secondary_link);

    /* determine number of scos and calls if any */
    if((sco_sink_primary && (call_state_primary > hfp_call_state_idle))&&
           (sco_sink_secondary && (call_state_secondary > hfp_call_state_idle)))
    {
        audio_priority primary_priority = getScoPriorityFromHfpPriority(hfp_primary_link);
        audio_priority secondary_priority = getScoPriorityFromHfpPriority(hfp_secondary_link);

        AUD_DEBUG(("AUD: two SCOs\n"));

        /* two calls and two SCOs exist, determine which sco has the highest priority */
        if(primary_priority == secondary_priority)
        {
            priority_sink = sco_sink_primary;
            /* There are two SCOs and both have the same priority, determine which was first and prioritise that */
            if(HfpGetFirstIncomingCallPriority() == hfp_secondary_link)
            {
                AUD_DEBUG(("AUD: route sec - pri = sec = [%d] [%d] :\n" , primary_priority, secondary_priority)) ;
                priority_sink = sco_sink_secondary;
            }
        }
        else
        {
            priority_sink = sco_sink_primary;
            if(secondary_priority > primary_priority)
            {
                AUD_DEBUG(("AUD: route - sec > pri = [%d] [%d] :\n" , primary_priority, secondary_priority)) ;
                priority_sink = sco_sink_secondary;
            }
        }
    }
    /* primary AG call + sco only? or pri sco and voice dial is active? */    
    else if( sco_sink_primary &&
            ((call_state_primary > hfp_call_state_idle)||(theSink.VoiceRecognitionIsActive)))
    {
        AUD_DEBUG(("AUD: AG1 sco\n"));
        /* AG1 (primary) call with sco only */
        priority_sink = sco_sink_primary;
    }
    /* secondary AG call + sco only? or sec sco and voice dial is active? */    
    else if( sco_sink_secondary &&
            ((call_state_secondary > hfp_call_state_idle)||(theSink.VoiceRecognitionIsActive)))
    {
        AUD_DEBUG(("AUD: AG2 sco\n"));
        /* AG2 (secondary) call with sco only */
        priority_sink = sco_sink_secondary;
    }
    return priority_sink;
}

static bool audioIsCallWithoutSco(void)
{
    if(sinkCallManagerIsCallWithoutSco(hfp_primary_link))
    {
        return TRUE;
    }
    if(sinkCallManagerIsCallWithoutSco(hfp_secondary_link))
    {
        return TRUE;
    }

    return FALSE;
}

/****************************************************************************
NAME
    audioRouteActiveCallIfAvailable

DESCRIPTION
    checks for any sco being present, check whether there is a corresponding
    active call and route it based on its priority. check whether sco is already
    routed or whether a different audio source needs to be suspended/disconnected

RETURNS
    true if sco routed, false if no sco routable
*/
bool audioRouteActiveCallScoIfAvailable(void)
{
    Sink sco_sink_to_route = audioGetActiveScoSink();

    AUD_DEBUG(("AUD: Sco Status state p[%d] s[%d] sink p[%d] s[%d]\n" , sinkCallManagerGetHfpCallState(hfp_primary_link), sinkCallManagerGetHfpCallState(hfp_secondary_link),(uint16)sinkCallManagerGetHfpSink(hfp_primary_link), (uint16)sinkCallManagerGetHfpSink(hfp_secondary_link))) ;

    if(sco_sink_to_route)
    {
        if(audioRouteScoSink(sco_sink_to_route))
        {
            hfp_link_priority hfp_priority = HfpLinkPriorityFromAudioSink(theSink.routed_voice);
            uint16 hfp_volume;
            bdaddr ag_addr;

            if( HfpLinkGetBdaddr(hfp_priority, &ag_addr) &&
                    deviceManagerGetAttributeHfpVolume(&ag_addr, &hfp_volume) )
            {
                AUD_DEBUG(("AUD: btAdd : %x %x %x\n",(uint16)ag_addr.nap,(uint16)ag_addr.uap,(uint16)ag_addr.lap ));
                AUD_DEBUG(("AUD: hfp Attribute Volume set: [%d][%d]\n",hfp_priority,hfp_volume));
                theSink.profile_data[PROFILE_INDEX(hfp_priority)].audio.gSMVolumeLevel = hfp_volume;
            }
            return TRUE;
        }
    }
    return FALSE;
}

static volume_info * audioGetVolumeConfigFromSource(audio_sources source)
{
    volume_info * volume = NULL;
    switch(source)
    {  
        case audio_source_FM:
            volume = &theSink.volume_levels.fm_volume;
            break;
        case audio_source_ANALOG:
        case audio_source_I2S:
            volume = &theSink.volume_levels.analog_volume;
            break;
        case audio_source_SPDIF:
            volume = &theSink.volume_levels.spdif_volume;
            break;
        case audio_source_USB:
            volume = &theSink.volume_levels.usb_volume;
            break;
        case audio_source_AG1:
            volume = &theSink.volume_levels.a2dp_volume[a2dp_primary];
            break;
        case audio_source_AG2:
            volume = &theSink.volume_levels.a2dp_volume[a2dp_secondary];
            break;
        case audio_source_none:
        default:
            break;
    }
    return volume;
}

static bool populateConnectParameters(audio_sources source, audio_connect_parameters *connect_parameters)
{
    switch(source)
    {
        case audio_source_FM:
            return sinkFmRxPopulateConnectParameters(connect_parameters);
        case audio_source_ANALOG:
            return analoguePopulateConnectParameters(connect_parameters);
        case audio_source_SPDIF:
            return spdifPopulateConnectParameters(connect_parameters);
        case audio_source_I2S:
            return i2sPopulateConnectParameters(connect_parameters);
        case audio_source_USB:
            return usbAudioPopulateConnectParameters(connect_parameters);
        case audio_source_AG1:
            return A2dpPopulateConnectParameters(a2dp_primary, connect_parameters);
        case audio_source_AG2:
            return A2dpPopulateConnectParameters(a2dp_secondary, connect_parameters);
        case audio_source_none:
        default:
            return FALSE;
    }
}

static void postAudioConnectSourceSpecificConfiguration(audio_sources source)
{
    switch(source)
    {
        case audio_source_FM:
            sinkFmRxPostConnectConfiguration();
            break;
        case audio_source_AG1:
            A2dpPostConnectConfiguration(a2dp_primary);
            break;
        case audio_source_AG2:
            A2dpPostConnectConfiguration(a2dp_secondary);
            break;
        default:
            break;
    }
}

static Task ConnectWithParameters(audio_connect_parameters *connect_parameters)
{
    return AudioConnect(    connect_parameters->audio_plugin,
                            connect_parameters->audio_sink,
                            connect_parameters->sink_type,
                            connect_parameters->volume,
                            connect_parameters->rate,
                            connect_parameters->features,
                            connect_parameters->mode,
                            connect_parameters->route,
                            connect_parameters->power,
                            connect_parameters->params,
                            connect_parameters->app_task);
}

static Task connectVoiceSink(Sink sink)
{
    Task connected_plugin = NULL;
    audio_connect_parameters *connect_parameters = mallocPanic(sizeof(audio_connect_parameters));

    if(audioHfpPopulateConnectParameters(sink,connect_parameters))
    {
        connected_plugin = ConnectWithParameters(connect_parameters);
        if(connected_plugin)
        {
            theSink.routed_voice = connect_parameters->audio_sink;
            theSink.routed_voice_task = connected_plugin;
            AUD_DEBUG(("Audio Connect[%d][%x]\n", theSink.features.audio_plugin , theSink.profile_data[index].audio.gSMVolumeLevel )) ;
        }
    }

    free(connect_parameters);

    return connected_plugin;
}

static Task connectAudioSource(audio_sources source)
{
    audio_connect_parameters *connect_parameters = mallocPanic(sizeof(audio_connect_parameters));
    Task connected_plugin = NULL;

    if(populateConnectParameters(source,connect_parameters))
    {
        connected_plugin = ConnectWithParameters(connect_parameters);
        if(connected_plugin)
        {
            theSink.routed_audio_task = connected_plugin;
            theSink.routed_audio = connect_parameters->audio_sink;
        }
    }

    free(connect_parameters);

    return connected_plugin;
}

static void routeVoiceSink(Sink voice_sink)
{
    if(theSink.routed_voice != voice_sink)
    {
        hfp_link_priority link_priority = hfp_invalid_link;

        audioDisconnectActiveVoiceSink();

        if((!isAudioSourceMixableWithVoice(theSink.rundata->routed_audio_source))
                || (!connectVoiceSink(voice_sink)))
        {
            audioSuspendDisconnectAudioSource();
            PanicNull(connectVoiceSink(voice_sink));
        }

        audioControlLowPowerCodecs(TRUE);
        link_priority = HfpLinkPriorityFromAudioSink(voice_sink);
        AudioSetVolume((int16)sinkVolumeGetCvcVol(theSink.profile_data[PROFILE_INDEX(link_priority)].audio.gSMVolumeLevel),
                                                    (int16)TonesGetToneVolume(FALSE));
        VolumeSetHfpMicrophoneGain(link_priority,
                    (theSink.profile_data[PROFILE_INDEX(link_priority)].audio.gMuted ? MICROPHONE_MUTE_ON : MICROPHONE_MUTE_OFF));
    }
}


static bool routeAudioSource(audio_sources source)
{
    subwooferResetSubwooferLinkType();

    audioSuspendDisconnectAudioSource();

    if(connectAudioSource(source))
    {
        postAudioConnectSourceSpecificConfiguration(source);
        audioControlLowPowerCodecs(FALSE);
        VolumeSetupInitialMutesAndVolumes(audioGetVolumeConfigFromSource(source));
        peerUpdateMuteState();
        updateSwatVolume(audioGetVolumeConfigFromSource(source)->main_volume);
        theSink.rundata->routed_audio_source = source;
        return TRUE;
    }
    return FALSE;
}

static audio_sources getA2dpSourceFromPriority(a2dp_link_priority priority)
{
    audio_sources source = audio_source_none;
    if(priority == a2dp_primary)
    {
        source = audio_source_AG1;
    }
    else if(priority == a2dp_secondary)
    {
        source = audio_source_AG2;
    }
    return source;
}

/****************************************************************************
NAME
    audioRouteA2dpStreamIfAvailable

DESCRIPTION
    routes specified a2dp priority

RETURNS
    TRUE if routed, else FALSE
*/
static bool audioRouteA2dpStreamIfAvailable(a2dp_link_priority priority)
{
    bool routed = FALSE;
    a2dp_link_priority linkPri = priority;

#if (defined ENABLE_AVRCP) && (MAX_AVRCP_CONNECTIONS >=2) && (MAX_A2DP_CONNECTIONS >=2)        
    a2dp_link_priority avrcp_channel = a2dp_primary;
#endif
    
    if(isAudioGated(audio_gate_a2dp))
    {
        return FALSE;
    }
    
    /* are both primary and secondary a2dp sources streaming streaming audio and one of them
        currently routed?, if so use AVRCP play status to decide if it needs to be swapped
        or left alone, if neither is routed then drop through to connecting the primary sink */
    if(((priority == a2dp_pri_sec)&&(sinkA2dpGetStreamState(a2dp_primary) == a2dp_stream_streaming)&&(sinkA2dpGetStreamState(a2dp_secondary) == a2dp_stream_streaming))&&
       ((sinkA2dpGetRoleType(a2dp_primary) == a2dp_sink)&&(sinkA2dpGetRoleType(a2dp_secondary) == a2dp_sink))&&
       ((a2dpAudioSinkMatch(a2dp_primary, theSink.routed_audio))||(a2dpAudioSinkMatch(a2dp_secondary, theSink.routed_audio))))     
    {
#if (defined ENABLE_AVRCP) && (MAX_AVRCP_CONNECTIONS >=2) && (MAX_A2DP_CONNECTIONS >=2)        
        uint16 avrcpIndex;
        /* default play status to Playing as in a streaming state */
        avrcp_play_status priPlayStatus = avrcp_play_status_playing;
        avrcp_play_status secPlayStatus = avrcp_play_status_playing;
        
        if(!theSink.features.avrcp_enabled ||
           !theSink.features.EnableAvrcpAudioSwitching)
        {
            AUD_DEBUG(("AUD: ignore AVRCP in audio switching\n"));
            return TRUE;
        }
        
        AUD_DEBUG(("AUD: play status avrcp 0 (%u", theSink.avrcp_link_data->connected[0]));
        AUD_DEBUG((",%u)", theSink.avrcp_link_data->play_status[0]));
        AUD_DEBUG((", avrcp 1 (%u", theSink.avrcp_link_data->connected[1]));
        AUD_DEBUG((",%u) \n", theSink.avrcp_link_data->play_status[1]));
                       
        /* if AVRCP is connected get the play status, if it's not then assume Playing as at this point there is
            an A2DP stream active */ 
        if (theSink.avrcp_link_data->connected[0])
        {
            priPlayStatus = theSink.avrcp_link_data->play_status[0];
        }
            
        if (theSink.avrcp_link_data->connected[1])
        {
            secPlayStatus = theSink.avrcp_link_data->play_status[1];
        }
            
        /* Compare play status to see if one is in a play state (or fwd or rwd seek) while the other is not.
           If there is no AVRCP connections this will fall through to making no changes as there is nothing to make a better 
           decision with (both will be set to 'Playing') */
        if((priPlayStatus == avrcp_play_status_playing || priPlayStatus == avrcp_play_status_fwd_seek || priPlayStatus == avrcp_play_status_rev_seek) &&
           (secPlayStatus != avrcp_play_status_playing && secPlayStatus != avrcp_play_status_fwd_seek && secPlayStatus != avrcp_play_status_rev_seek))
        { 
            avrcpIndex = 0;
        }
        else if((secPlayStatus == avrcp_play_status_playing || secPlayStatus == avrcp_play_status_fwd_seek || secPlayStatus == avrcp_play_status_rev_seek) && 
                (priPlayStatus != avrcp_play_status_playing && priPlayStatus != avrcp_play_status_fwd_seek && priPlayStatus != avrcp_play_status_rev_seek))
        {                
            avrcpIndex = 1;
        }
        else
        {
            /* leave routing as it is, both are probably in Playing state or neither have AVRCP
               and we don't know any better */
            AUD_DEBUG(("AUD: ignore AVRCP in audio switching leave alone\n"));
            return TRUE;
        }
        
        /* Check if the selected AVRCP channel is connected */
        avrcp_channel = BdaddrIsSame(&theSink.a2dp_link_data->bd_addr[a2dp_primary],
                                   &theSink.avrcp_link_data->bd_addr[avrcpIndex]) ? a2dp_primary : a2dp_secondary;

        if (theSink.avrcp_link_data->connected[avrcpIndex])
        {
            /* AVRCP channel is connected so give it priority */
            linkPri = avrcp_channel;
        }
        else
        {
            /* AVRCP channel is not connected so pick the other one */
            linkPri = (avrcp_channel == a2dp_primary) ? a2dp_secondary : a2dp_primary;
        }

        AUD_DEBUG(("AUD: select avrcp %u", avrcpIndex));
        AUD_DEBUG((", a2dp %u\n", linkPri));
 
#else       
        /* with no AVRCP leave audio routing as is*/
        return TRUE;   
#endif           
    }        
    
    /* no active streams, are there any suspended streams? */    
    if((linkPri != a2dp_secondary)&&(a2dpSuspended(a2dp_primary)==a2dp_local_suspended)&&(sinkA2dpGetRoleType(a2dp_primary) == a2dp_sink))
    {
        AUD_DEBUG(("AUD: route - a2dp suspended, resume primary\n" )) ;
        /* send avrcp play or a2dp start to resume audio and connect it */        
        if(ResumeA2dpStream(a2dp_primary))
        {
            if(!a2dpAudioSinkMatch(a2dp_primary, theSink.routed_audio))
            {
                return routeAudioSource(getA2dpSourceFromPriority(a2dp_primary));
            }
        }
        /* successfull issued restart of audio */
        routed = TRUE;
    }
    else if((linkPri != a2dp_primary)&&(a2dpSuspended(a2dp_secondary)==a2dp_local_suspended)&&(sinkA2dpGetRoleType(a2dp_secondary) == a2dp_sink))
    {
        AUD_DEBUG(("AUD: route - a2dp suspended, resume secondary\n" )) ;
        if(ResumeA2dpStream(a2dp_secondary))
        {
            if(!a2dpAudioSinkMatch(a2dp_secondary, theSink.routed_audio))
            {
                return routeAudioSource(getA2dpSourceFromPriority(a2dp_secondary));
            }
        }
        /* successfull issued restart of audio */
        routed = TRUE;
    }
    /* is primary a2dp streaming audio? */
    else if((linkPri != a2dp_secondary)&&((sinkA2dpGetStreamState(a2dp_primary) == a2dp_stream_starting)||(sinkA2dpGetStreamState(a2dp_primary) == a2dp_stream_streaming))&&(a2dpSuspended(a2dp_primary)==a2dp_not_suspended)&&(sinkA2dpGetRoleType(a2dp_primary) == a2dp_sink))
    {
        AUD_DEBUG(("AUD: pri a2dp streaming\n"));
        if(!a2dpAudioSinkMatch(a2dp_primary, theSink.routed_audio))
        {
            return routeAudioSource(getA2dpSourceFromPriority(a2dp_primary));
        } 
        
        /* successfully routed audio */
        routed = TRUE;
    }
    /* or a2dp secondary streaming audio */    
    else if((linkPri != a2dp_primary)&&((sinkA2dpGetStreamState(a2dp_secondary) == a2dp_stream_starting)||(sinkA2dpGetStreamState(a2dp_secondary) == a2dp_stream_streaming))&&(a2dpSuspended(a2dp_secondary)==a2dp_not_suspended)&&(sinkA2dpGetRoleType(a2dp_secondary) == a2dp_sink))
    {
        AUD_DEBUG(("AUD: sec a2dp streaming\n"));
        if(!a2dpAudioSinkMatch(a2dp_secondary, theSink.routed_audio))
        {
            return routeAudioSource(getA2dpSourceFromPriority(a2dp_secondary));
        } 
        
        /* successfully routed audio */
        routed = TRUE;
    }
    else
    {
        AUD_DEBUG(("AUD: failed to route any a2dp audio\n"));        
    }
    
    return routed;
}

/****************************************************************************
NAME    
    audioRouteUsbIfAvailable
    
DESCRIPTION
    checks for a usb stream being present and routes it if present and allowed
    in the current state
    
RETURNS
    true if usb routed, false if no usb routable
*/
static bool audioRouteUsbIfAvailable(void)
{
    if((usbGetAudioSink()
            || sinkUsbAudioIsSuspendedLocal()
            || sinkUsbIsMicrophoneActive())
        && (!isAudioGated(audio_gate_usb)))
    {
        if(sinkUsbAudioIsSuspendedLocal())
        {
            usbAudioResume();
        }

        if(!usbAudioSinkMatch(theSink.routed_audio))
        {
            return routeAudioSource(audio_source_USB);
        }
        return TRUE;
    }

    AUD_DEBUG(("AUD: USB audio NOT routed\n"));
    return FALSE;
}


/****************************************************************************
NAME    
    audioRouteAnalogIfAvailable
    
DESCRIPTION
    checks for an analog audio stream being present and routes it if present
    and allowed in current state
    
RETURNS
    true if analog audio routed, false if no analog audio routable
*/
static bool audioRouteAnalogIfAvailable(void)
{
    if(analogAudioConnected() && (!isAudioGated(audio_gate_wired)))
    {
        if(!analogAudioSinkMatch(theSink.routed_audio))
        {
            return routeAudioSource(audio_source_ANALOG);
        }
        return TRUE;
    }
    return FALSE;
}

/****************************************************************************
NAME    
    audioRouteSpdifIfAvailable
    
DESCRIPTION
    checks for an spdif audio stream being present and routes it if present
    and allowed in the current state
    
RETURNS
    true if spdif audio routed, false if no spdif audio routable
*/
static bool audioRouteSpdifIfAvailable(void)
{
    if(spdifAudioConnected() && (!isAudioGated(audio_gate_wired)))
    {
        if(!spdifAudioSinkMatch(theSink.routed_audio))
        {
            return routeAudioSource(audio_source_SPDIF);
        }
        return TRUE;
    }
    return FALSE;
}
/****************************************************************************
NAME    
    audioRouteI2SIfAvailable
    
DESCRIPTION
    checks for an I2S audio stream being present and routes it if present
    and allowed in the current state
    
RETURNS
    true if I2S audio routed, false if no I2S audio routable
*/
static bool audioRouteI2SIfAvailable(void)
{
    if(i2sAudioConnected() && (!isAudioGated(audio_gate_wired)))
    {
        if(!i2sAudioSinkMatch(theSink.routed_audio))
        {
            return routeAudioSource(audio_source_I2S);
        }
        return TRUE;
    }
    return FALSE;
}

/****************************************************************************
NAME    
    audioRouteFMIfAvailable
    
DESCRIPTION
    checks for an fm audio stream being present and routes it if present  
    
RETURNS
    true if fm routed, false if no fm routed
*/
bool audioRouteFMIfAvailable(void)
{
    if(sinkFmIsFmRxOn() && (!isAudioGated(audio_gate_fm)))
    {
        if(!sinkFmAudioSinkMatch(theSink.routed_audio))
        {
            return routeAudioSource(audio_source_FM);
        }
        return TRUE;
    }
    return FALSE;
}


static bool audioRouteScoSink(Sink sco_sink_to_route)
{
    if(sco_sink_to_route && (!isAudioGated(audio_gate_call)))
    {
        routeVoiceSink(sco_sink_to_route);
        return TRUE;
    }
    return FALSE;
}

/****************************************************************************
NAME    
    audioRouteScoIfAvailable
    
DESCRIPTION
    checks for any sco being present without any call indications
    
RETURNS
    true if sco routed, false if no sco routable
*/
bool audioRouteScoOutsideOfCallIfAvailable(void)
{
    if(!theSink.routed_voice && !theSink.routed_audio)
    {
        Sink sink_to_route = sinkCallManagerGetHfpSink(hfp_primary_link);
        if(!sink_to_route)
        {
            sink_to_route = sinkCallManagerGetHfpSink(hfp_secondary_link);
        }
        AUD_DEBUG(("AUD: Sco Status state p[%d] s[%d] sink p[%d] s[%d]\n" , sinkCallManagerGetHfpCallState(hfp_primary_link), sinkCallManagerGetHfpCallState(hfp_secondary_link),(uint16)sinkCallManagerGetHfpSink(hfp_primary_link), (uint16)sinkCallManagerGetHfpSink(hfp_secondary_link))) ;
        return audioRouteScoSink(sink_to_route);
    }
    return FALSE;
}

static hfp_link_priority getHfpLinkIndexFromA2dpPriority(a2dp_link_priority priority)
{
    hfp_link_priority hfp_link_index = hfp_invalid_link;
    if(deviceManagerIsSameDevice(priority, hfp_primary_link))
    {
        hfp_link_index = hfp_primary_link;
    }
    else if(deviceManagerIsSameDevice(priority, hfp_secondary_link))
    {
        hfp_link_index = hfp_secondary_link;
    }
    return hfp_link_index;
}

static bool isActiveCallOnTheSameDevice(a2dp_link_priority priority)
{
    hfp_link_priority hfp_priority = getHfpLinkIndexFromA2dpPriority(priority);

    if(hfp_priority)
    {
        return (sinkCallManagerGetHfpCallState(hfp_priority) > hfp_call_state_idle);
    }
    return FALSE;
}

static void a2dpSuspendIfRequired(a2dp_link_priority priority)
{
    if((!isActiveCallOnTheSameDevice(priority)) || (!theSink.features.AssumeAutoSuspendOnCall))
    {
        AUD_DEBUG(("AUD: suspend a2dp audio \n"));
        SuspendA2dpStream(priority);
    }
}

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
void audioSuspendDisconnectAudioSource(void)
{
    if(theSink.routed_audio)
    {
        audio_route_available current_route = sinkAudioRouteAvailable();
        audioDisconnectActiveAudioSink();
        switch(current_route)
        {
            case audio_route_a2dp_primary:
                a2dpSuspendIfRequired(a2dp_primary);
                break;
            case audio_route_a2dp_secondary:
                a2dpSuspendIfRequired(a2dp_secondary);
                break;
            case audio_route_usb:
                usbAudioPostDisconnectConfiguration();
                break;
            case audio_route_fm:
                sinkFmRxAudioPostDisconnectConfiguration();
                break;
            default:
                break;
        }

        sendMuteToSubwoofer();
        theSink.rundata->routed_audio_source = audio_source_none;
    }
}

static int16 audioGetVolumeForSink(Sink sink)
{
    int16 volume = theSink.features.DefaultVolume;

    if(a2dpAudioSinkMatch(a2dp_primary, sink))
    {
        AUD_DEBUG(("AUD: disp vol a2dp1\n")) ;
        volume = theSink.volume_levels.a2dp_volume[a2dp_primary].main_volume;
    }
    else if(a2dpAudioSinkMatch(a2dp_secondary, sink))
    {
        AUD_DEBUG(("AUD: disp vol a2dp2\n")) ;
        volume = theSink.volume_levels.a2dp_volume[a2dp_secondary].main_volume;
    }
    else if (sink == sinkCallManagerGetHfpSink(hfp_primary_link))
    {
        AUD_DEBUG(("AUD: disp vol hfp1\n")) ;
        volume = theSink.profile_data[PROFILE_INDEX(hfp_primary_link)].audio.gSMVolumeLevel;
    }
    else if (sink == sinkCallManagerGetHfpSink(hfp_secondary_link))
    {
        AUD_DEBUG(("AUD: disp vol hfp2\n")) ;
        volume = theSink.profile_data[PROFILE_INDEX(hfp_secondary_link)].audio.gSMVolumeLevel;
    }
    else if (usbAudioSinkMatch(sink))
    {
#ifdef ENABLE_USB_AUDIO
        usb_device_class_audio_levels *levels = mallocPanic(sizeof (usb_device_class_audio_levels));

        AUD_DEBUG(("AUD: disp vol usb\n")) ;
        if (levels)
        {
            UsbDeviceClassGetValue(USB_DEVICE_CLASS_GET_VALUE_AUDIO_LEVELS, (uint16 *) levels);

            /* convert to signed 16 */
            levels->out_l_vol = (int16)(int8)(levels->out_l_vol>>8);
            levels->out_r_vol = (int16)(int8)(levels->out_r_vol>>8);

            /* convert to volume steps */
            volume = (sinkVolumeGetNumberOfVolumeSteps(audio_output_group_main) - ((levels->out_l_vol * sinkVolumeGetNumberOfVolumeSteps(audio_output_group_main))/-60));

            /* limit check */
            if(volume > sinkVolumeGetMaxVolumeStep(audio_output_group_main))
            {
                volume = sinkVolumeGetMaxVolumeStep(audio_output_group_main);
            }
            if(volume < 0)
            {
                volume = 0;
            }

            if (!sinkUsbIsUsbPluginTypeStereo())
            {
                volume = levels->out_l_vol;
            }

            freePanic(levels);
        }
#endif
    }
    else if (analogAudioSinkMatch(sink))
    {
        AUD_DEBUG(("AUD: disp vol wired\n")) ;
        volume = theSink.volume_levels.analog_volume.main_volume;
    }
    else if (spdifAudioSinkMatch(sink))
    {
        AUD_DEBUG(("AUD: disp vol wired\n")) ;
        volume = theSink.volume_levels.spdif_volume.main_volume;
    }
    else if(i2sAudioSinkMatch(sink))
    {
        AUD_DEBUG(("AUD: disp vol wired\n")) ;
        volume = theSink.volume_levels.analog_volume.main_volume;
    }
    return volume;
}

/****************************************************************************
NAME    
    audioDisconnectActiveSink
    
DESCRIPTION
    Disconnect the active audio sink

RETURNS
    
*/
static void audioDisconnectActiveSink(Task task_to_disconnect)
{
    if(task_to_disconnect)
    {
        AUD_DEBUG(("AUD: route - audio disconnect = [%X] :\n" , (uint16)sink_to_disconnect)) ;
        AudioDisconnectTask(task_to_disconnect);
    }
}

void audioDisconnectActiveAudioSink(void)
{
    audioDisconnectActiveSink(theSink.routed_audio_task);
    theSink.routed_audio_task = 0;
    theSink.routed_audio = 0;
}

void audioDisconnectActiveVoiceSink(void)
{
    audioDisconnectActiveSink(theSink.routed_voice_task);
    theSink.routed_voice_task = 0;
    theSink.routed_voice = 0;
}

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
bool audioSuspendDisconnectAllA2dpMedia(void)
{
    bool disconnected = FALSE;

    AUD_DEBUG(("AUD: suspend any a2dp due to esco sub use \n"));
            
    /* primary a2dp currently routed? */    
    if((theSink.rundata->routed_audio_source)&&(a2dpAudioSinkMatch(a2dp_primary, theSink.routed_audio)))
    {
        AUD_DEBUG(("AUD: suspend a2dp primary audio \n"));
        SuspendA2dpStream(a2dp_primary);
        /* and disconnect the routed sink */
        AUD_DEBUG(("AUD: disconnect a2dp primary audio \n"));
        /* disconnect audio */
        audioDisconnectActiveAudioSink();
        /* update currently routed source */
        theSink.rundata->routed_audio_source = audio_source_none;
        /* successfully disconnected audio */        
        disconnected = TRUE;
    }
    /* secondary a2dp currently routed? */
    else if((theSink.rundata->routed_audio_source)&&(a2dpAudioSinkMatch(a2dp_secondary, theSink.routed_audio)))
    {
        AUD_DEBUG(("AUD: suspend a2dp secondary audio \n"));
        SuspendA2dpStream(a2dp_secondary);
        /* and disconnect the routed sink */
        AUD_DEBUG(("AUD: disconnect a2dp secondary audio \n"));
        /* disconnect audio */
        audioDisconnectActiveAudioSink();
        /* update currently routed source */
        theSink.rundata->routed_audio_source = audio_source_none;
        /* successfully disconnected audio */        
        disconnected = TRUE;
    }
    /* are there any a2dp media streams not currently routed that need to be suspended? */
    else
    {
        if(sinkA2dpGetStreamState(a2dp_primary) == a2dp_stream_streaming)
        {
            AUD_DEBUG(("AUD: suspend pri a2dp\n"));
            /* suspend or disconnect the a2dp media stream */
            SuspendA2dpStream(a2dp_primary);       
        }
        if(sinkA2dpGetStreamState(a2dp_secondary) == a2dp_stream_streaming)
        {
            AUD_DEBUG(("AUD: suspend sec a2dp\n"));
            /* suspend or disconnect the a2dp media stream */
            SuspendA2dpStream(a2dp_secondary);       
        }
    }                
    return disconnected;
}

/****************************************************************************
NAME    
    sinkAudioRouteAvailable
    
DESCRIPTION
    returns which audio source is routed. Only the route of highest priority is 
    returned. The priority starting at the top of the enum audio_route_available
    
RETURNS
    audio_route_available
*/
audio_route_available sinkAudioRouteAvailable(void)
{

    audio_route_available route = audio_route_none;

    if(theSink.routed_audio)
    {
        if ( a2dpAudioSinkMatch( a2dp_primary, theSink.routed_audio) )
        {
            AUD_DEBUG(("AUD ROUTE: A2DP primary\n"));
            route = audio_route_a2dp_primary;
        }
        else if ( a2dpAudioSinkMatch( a2dp_secondary, theSink.routed_audio) )
        {
            AUD_DEBUG(("AUD ROUTE: A2DP secondary\n"));
            route = audio_route_a2dp_secondary;
        } 
        
#ifndef HYDRACORE_TODO
        else if( usbAudioSinkMatch(theSink.routed_audio))    
        {
            AUD_DEBUG(("AUD ROUTE: usb\n"));
            route = audio_route_usb;
        }
#endif
        else if (analogAudioSinkMatch(theSink.routed_audio))
        {
    
            AUD_DEBUG(("AUD ROUTE: analogue\n"));
            route = audio_route_analog;
        }
        else if (spdifAudioSinkMatch(theSink.routed_audio))
        {
            AUD_DEBUG(("AUD ROUTE: spdif\n"));
            route = audio_route_spdif;
        }
        else if(i2sAudioSinkMatch(theSink.routed_audio))
        {
            AUD_DEBUG(("AUD ROUTE: I2S\n"));
            route = audio_route_i2s;
        }
        else if(sinkFmAudioSinkMatch(theSink.routed_audio))
        {
            AUD_DEBUG(("AUD ROUTE: FM\n"));
            route = audio_route_fm;
        }
    }
    else
    {
        hfp_link_priority hfp_priority = HfpLinkPriorityFromAudioSink(theSink.routed_voice);

        if( hfp_priority == hfp_primary_link )
        {
            AUD_DEBUG(("AUD ROUTE: HFP primary\n"));
            route = audio_route_hfp_primary;
        }
        else if(  hfp_priority == hfp_secondary_link )
        {
            AUD_DEBUG(("AUD ROUTE: HFP secondary\n"));
            route = audio_route_hfp_secondary;
        }
    }
    return route;
}

/****************************************************************************/
bool audioRouteAncIfAvailable(void)
{
    if(sinkAncIsEnabled() && (!theSink.routed_voice) && (!theSink.routed_audio))
    {
        if(theSink.routed_audio != ANC_SINK)
        {
            if(!sinkAncAudioRoute())
            {
                PanicFalse(sinkAncAudioRoute());
            }
        }
        return TRUE;
    }
    return FALSE;
}

/****************************************************************************/
void audioA2dpStartStream(void)
{
    if(sinkA2dpGetStreamState(a2dp_primary) == a2dp_stream_open && a2dpSuspended(a2dp_primary)==a2dp_not_suspended && sinkA2dpGetRoleType(a2dp_primary) == a2dp_sink)
    {
        if(A2dpSignallingGetState(theSink.a2dp_link_data->device_id[a2dp_primary]) == a2dp_signalling_connected)
        {
            A2dpMediaStartRequest(theSink.a2dp_link_data->device_id[a2dp_primary], theSink.a2dp_link_data->stream_id[a2dp_primary]);
        }
    }
    else if(sinkA2dpGetStreamState(a2dp_secondary) == a2dp_stream_open && a2dpSuspended(a2dp_secondary)==a2dp_not_suspended && sinkA2dpGetRoleType(a2dp_secondary) == a2dp_sink)
    {
        if(A2dpSignallingGetState(theSink.a2dp_link_data->device_id[a2dp_secondary]) == a2dp_signalling_connected)
        {
            A2dpMediaStartRequest(theSink.a2dp_link_data->device_id[a2dp_secondary], theSink.a2dp_link_data->stream_id[a2dp_secondary]);
        }
    }
}

/*************************************************************************
NAME
    getAudioSourceFromEventUsrSelectAudioSource

DESCRIPTION
    Maps a user event to the relevant audio source.

INPUTS
    User event related to audio sources.

RETURNS
    The matching audio source.

*/
static audio_sources getAudioSourceFromEventUsrSelectAudioSource(const MessageId EventUsrSelectAudioSource)
{
    audio_sources audio_source = audio_source_none;

    switch(EventUsrSelectAudioSource)
    {
        case EventUsrSelectAudioSourceAnalog:
            audio_source = audio_source_ANALOG;
            break;

        case EventUsrSelectAudioSourceSpdif:
            audio_source = audio_source_SPDIF;
            break;

        case EventUsrSelectAudioSourceI2S:
            audio_source = audio_source_I2S;
            break;
            
        case EventUsrSelectAudioSourceUSB:
            audio_source = audio_source_USB;
            break;

        case EventUsrSelectAudioSourceAG1:
            audio_source = audio_source_AG1;
            break;

        case EventUsrSelectAudioSourceAG2:
            audio_source = audio_source_AG2;
            break;

        case EventUsrSelectAudioSourceFM:
            audio_source = audio_source_FM;
            break;
            
        case EventUsrSelectAudioSourceNextRoutable:
            audio_source = getNextRoutableSource(theSink.rundata->requested_audio_source);
            break;

        case EventUsrSelectAudioSourceNext:
            audio_source = getNextSourceInSequence(theSink.rundata->requested_audio_source);
            break;

        default:
            Panic(); /* Unexpected event */
            break;
    }

    if(audioIsAudioSourceEnabledByUser(audio_source) == FALSE)
    {
        /* In case requested source is not enabled, remain with the current routed
         * rather than disconnect.*/
        audio_source = theSink.rundata->routed_audio_source;
    }
    
    return audio_source;
}

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
bool processEventUsrSelectAudioSource(const MessageId EventUsrSelectAudioSource)
{
    bool result = TRUE;

    /* No need to attempt routing a source while in deviceLimbo state. */
    if(stateManagerGetState() == deviceLimbo)
    {
        result = FALSE;
    }
    else
    {
        if(theSink.conf2->audio_routing_data.PluginFeatures.manual_source_selection)
        {
            theSink.rundata->requested_audio_source = getAudioSourceFromEventUsrSelectAudioSource(EventUsrSelectAudioSource);

            result = audioIsAudioSourceRoutable(theSink.rundata->requested_audio_source);

            peerUpdateRelaySource(theSink.rundata->requested_audio_source);
            audioUpdateAudioRouting();

            if(result)
            {
                sinkVolumeResetVolumeAndSourceSaveTimeout();
            }

        }
        else
        {
            result = FALSE;
        }
    }

    return result;
}

/*************************************************************************
NAME
    getNextSourceInSequence

DESCRIPTION
    Helper function to retrieve the next available source from the user
    defined sequence.

INPUTS
    Initial audio source to start from in order to identify next.

RETURNS
    Audio sources enums.

*/
static audio_sources getNextSourceInSequence(audio_sources source)
{
    audio_sources next_source = audio_source_none;
    
    if((theSink.features.seqSourcePriority1 == source) &&
            (theSink.features.seqSourcePriority2))
    {
        next_source = (audio_sources)theSink.features.seqSourcePriority2;
    }
    else if((theSink.features.seqSourcePriority2 == source) &&
            (theSink.features.seqSourcePriority3))
    {
        next_source = (audio_sources)theSink.features.seqSourcePriority3;
    }
    else if((theSink.features.seqSourcePriority3 == source) &&
            (theSink.features.seqSourcePriority4))
    {
        next_source = (audio_sources)theSink.features.seqSourcePriority4;
    }
    else if((theSink.features.seqSourcePriority4 == source) &&
            (theSink.features.seqSourcePriority5))
    {
        next_source = (audio_sources)theSink.features.seqSourcePriority5;
    }    
    else if((theSink.features.seqSourcePriority5 == source) &&
            (theSink.features.seqSourcePriority6))
    {
        next_source = (audio_sources)theSink.features.seqSourcePriority6;
    }    
    else if((theSink.features.seqSourcePriority6 == source) &&
            (theSink.features.seqSourcePriority7))
    {
        next_source = (audio_sources)theSink.features.seqSourcePriority7;
    } 
    else if(theSink.features.seqSourcePriority1)
    {
        next_source = (audio_sources)theSink.features.seqSourcePriority1;
    }

    return next_source;
}

/*************************************************************************
NAME
    audioIsAudioSourceRoutable

DESCRIPTION
    Helper call to confirm the route-ability of an audio source

INPUTS
    Audio source enum

RETURNS
    TRUE when source passed is routable, otherwise FALSE.

*/
static bool audioIsAudioSourceRoutable(audio_sources source)
{
    bool is_routable = FALSE;

    switch(source)
    {
        case audio_source_none:
            is_routable = TRUE;
            break;
        case audio_source_FM:
            is_routable = sinkFmIsFmRxOn();
            break;
        case audio_source_ANALOG:
            is_routable = analogAudioConnected();
            break;
        case audio_source_SPDIF:
            is_routable = spdifAudioConnected();
            break;
        case audio_source_I2S:
            is_routable = i2sAudioConnected();
            break;
        case audio_source_USB:
            is_routable = ( usbGetAudioSink() ||
                    sinkUsbAudioIsSuspendedLocal() ||
                    sinkUsbIsMicrophoneActive() );
            break;
        case audio_source_AG1:
            is_routable = audioIsA2dpAudioRoutable(a2dp_primary);
            break;
        case audio_source_AG2:
            is_routable = audioIsA2dpAudioRoutable(a2dp_secondary);
            break;
        case audio_source_end_of_list:
        default:
            break;
    }

    return is_routable;

}

/*************************************************************************
NAME
    audioIsAudioSourceEnabledByUser

DESCRIPTION
    Helper call in order to verify requested source is enabled by user.

INPUTS
    Audio source enum.

RETURNS
    TRUE if enabled, otherwise FALSE.

*/
static bool audioIsAudioSourceEnabledByUser(audio_sources source)
{
    return ((theSink.features.seqSourcePriority1 == source) ||
            (theSink.features.seqSourcePriority2 == source) ||
            (theSink.features.seqSourcePriority3 == source) ||
            (theSink.features.seqSourcePriority4 == source) ||
            (theSink.features.seqSourcePriority5 == source) ||
            (theSink.features.seqSourcePriority6 == source) ||
            (theSink.features.seqSourcePriority7 == source) );
}

/*************************************************************************
NAME
    audioGetAudioSourceToRoute

DESCRIPTION
    Final call of routing an audio source.

INPUTS
    None

RETURNS
    Audio source to route.

*/
static audio_sources audioGetAudioSourceToRoute(void)
{
    audio_sources source_to_route = theSink.rundata->requested_audio_source;

    if(!theSink.conf2->audio_routing_data.PluginFeatures.manual_source_selection)
    {
        source_to_route = audioGetHighestPriorityAudioSourceAvailable();
    }

    return source_to_route;
}

/*************************************************************************
NAME
    audioIsA2dpAudioRoutable

DESCRIPTION
    Initial check to confirm that A2DP is routable, without actually
    routing at this stage.

INPUTS
    a2dp_link_priority type.

RETURNS
    TRUE if routable, otherwise FALSE.

*/
static bool audioIsA2dpAudioRoutable(a2dp_link_priority priority)
{
    bool result = FALSE;

    /* Case covering A2DP only involved. */
    if( (priority == a2dp_primary) && 
        (sinkA2dpGetRoleType(a2dp_primary) == a2dp_sink) &&
        (a2dpSuspended(a2dp_primary) != a2dp_remote_suspended) &&
        ((sinkA2dpGetStreamState(a2dp_primary) == a2dp_stream_starting) || (sinkA2dpGetStreamState(a2dp_primary) == a2dp_stream_streaming)) )
    {  
        result = TRUE;
    }    
    /* Case covering A2DP only involved. */    
    else if( (priority == a2dp_secondary) && 
             (sinkA2dpGetRoleType(a2dp_secondary) == a2dp_sink) &&             
             (a2dpSuspended(a2dp_secondary) != a2dp_remote_suspended) &&
             ((sinkA2dpGetStreamState(a2dp_secondary) == a2dp_stream_starting) || (sinkA2dpGetStreamState(a2dp_secondary) == a2dp_stream_streaming)) )
    {       
        result = TRUE;   
    }
    /* Case covering A2DP and Call involved where stream is suspended. */    
    else if( (priority != a2dp_secondary) &&
             (sinkA2dpGetRoleType(a2dp_primary) == a2dp_sink) &&
             (a2dpSuspended(a2dp_primary) == a2dp_local_suspended) && 
             ((sinkA2dpGetStreamState(a2dp_primary) == a2dp_stream_open) && (sinkA2dpGetStreamState(a2dp_secondary) != a2dp_stream_streaming)) )
    {        
        result = TRUE;          
    }
    /* Case covering A2DP and Call involved where stream is suspended. */        
    else if( (priority != a2dp_primary) &&
             (sinkA2dpGetRoleType(a2dp_secondary) == a2dp_sink) &&
             (a2dpSuspended(a2dp_secondary) == a2dp_local_suspended) &&
             ((sinkA2dpGetStreamState(a2dp_secondary) == a2dp_stream_open) && (sinkA2dpGetStreamState(a2dp_primary) != a2dp_stream_streaming)) )
    {       
        result = TRUE;          
    }

    return result;    
}

/*************************************************************************
NAME
    getNextRoutableSource

DESCRIPTION
    Gets the next available but also routable audio source.

INPUTS
    audio_sources current audio source

RETURNS
    The routable audio source

*/
static audio_sources getNextRoutableSource(audio_sources current_source)
{
    audio_sources new_source = getNextSourceInSequence(current_source);

    if(audio_source_none == current_source)
    {
        current_source = theSink.features.seqSourcePriority1;
    }

    while(new_source != current_source && (!audioIsAudioSourceRoutable(new_source)))
    {
        new_source = getNextSourceInSequence(new_source);
    }

    return new_source;
}

void audioRouteSpecificA2dpSource(audio_sources a2dp_source)
{
    if(isAudioSourceA2dp(a2dp_source))
    {
        routeAudioSource(a2dp_source);
    }
}
