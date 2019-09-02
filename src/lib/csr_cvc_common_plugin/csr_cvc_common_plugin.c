/****************************************************************************
Copyright (c) 2005 - 2016 Qualcomm Technologies International, Ltd.

FILE NAME
    csr_cvc_common_plugin.c
DESCRIPTION
    Interface file for an audio_plugin
NOTES
*/
#ifndef SCO_DSP_BOUNDARIES
#define SCO_DSP_BOUNDARIES
#endif

#include <audio.h>
#include <stdlib.h>
#include <print.h>
#include <stream.h> /*for the ringtone_note*/
#include <string.h> /*for memcpy*/

#include "audio_plugin_if.h" /*the messaging interface*/
#include "csr_cvc_common_plugin.h"
#include "csr_cvc_common.h"
#include "csr_cvc_common_dsp_if.h"
#include "csr_cvc_common_dsp_message_handler.h"

    /*the task message handler*/
static void message_handler (Task task, MessageId id, Message message) ;

    /*the local message handling functions*/
static void handleAudioMessage ( Task task , MessageId id, Message message )     ;
static void handleInternalMessage ( Task task , MessageId id, Message message )     ;
    
    /*the plugin task*/
const CvcPluginTaskdata csr_cvsd_cvc_1mic_headset_plugin = {{message_handler},cvc_1_mic_headset_cvsd, link_encoding_cvsd, 0, 0, BITFIELD_CAST(6, 0)};
const CvcPluginTaskdata csr_wbs_cvc_1mic_headset_plugin = {{message_handler},cvc_1_mic_headset_msbc, link_encoding_msbc, 0, 1, BITFIELD_CAST(6, 0)};
const CvcPluginTaskdata csr_cvsd_cvc_1mic_headset_bex_plugin = {{message_handler},cvc_1_mic_headset_cvsd_bex, link_encoding_cvsd, 0, 1, BITFIELD_CAST(6, 0)};

const CvcPluginTaskdata csr_cvsd_cvc_2mic_headset_plugin = {{message_handler},cvc_2_mic_headset_cvsd, link_encoding_cvsd, 1, 0, BITFIELD_CAST(6, 0)};
const CvcPluginTaskdata csr_wbs_cvc_2mic_headset_plugin = {{message_handler},cvc_2_mic_headset_msbc, link_encoding_msbc, 1, 1, BITFIELD_CAST(6, 0)};
const CvcPluginTaskdata csr_cvsd_cvc_2mic_headset_bex_plugin = {{message_handler},cvc_2_mic_headset_cvsd_bex, link_encoding_cvsd, 1, 1, BITFIELD_CAST(6, 0)};

/* Handsfree variants */
const CvcPluginTaskdata csr_cvsd_cvc_1mic_handsfree_plugin = {{message_handler},cvc_1_mic_handsfree_cvsd, link_encoding_cvsd, 0, 0, BITFIELD_CAST(6, 0)};
const CvcPluginTaskdata csr_wbs_cvc_1mic_handsfree_plugin = {{message_handler},cvc_1_mic_handsfree_msbc, link_encoding_msbc, 0, 1, BITFIELD_CAST(6, 0)};
const CvcPluginTaskdata csr_cvsd_cvc_1mic_handsfree_bex_plugin = {{message_handler},cvc_1_mic_handsfree_cvsd_bex, link_encoding_cvsd, 0, 1, BITFIELD_CAST(6, 0)};

const CvcPluginTaskdata csr_cvsd_cvc_2mic_handsfree_plugin = {{message_handler},cvc_2_mic_handsfree_cvsd, link_encoding_cvsd, 1, 0, BITFIELD_CAST(6, 0)};
const CvcPluginTaskdata csr_wbs_cvc_2mic_handsfree_plugin = {{message_handler},cvc_2_mic_handsfree_msbc, link_encoding_msbc, 1, 1, BITFIELD_CAST(6, 0)};
const CvcPluginTaskdata csr_cvsd_cvc_2mic_handsfree_bex_plugin = {{message_handler},cvc_2_mic_handsfree_cvsd_bex, link_encoding_cvsd, 1, 1, BITFIELD_CAST(6, 0)};

const CvcPluginTaskdata csr_cvsd_no_dsp_plugin = {{message_handler},cvc_disabled, link_encoding_cvsd, 0, 0, BITFIELD_CAST(6, 0)};

const CvcPluginTaskdata csr_cvsd_cvc_1mic_asr_plugin = {{message_handler},cvc_1_mic_headset_cvsd_asr, link_encoding_cvsd, 0, 0, BITFIELD_CAST(6, 0)};
const CvcPluginTaskdata csr_cvsd_cvc_2mic_asr_plugin = {{message_handler},cvc_2_mic_headset_cvsd_asr, link_encoding_cvsd, 1, 0, BITFIELD_CAST(6, 0)};

const CvcPluginTaskdata csr_cvsd_cvc_1mic_hf_asr_plugin = {{message_handler},cvc_1_mic_handsfree_cvsd_asr, link_encoding_cvsd, 0, 0, BITFIELD_CAST(6, 0)};
const CvcPluginTaskdata csr_cvsd_cvc_2mic_hf_asr_plugin = {{message_handler},cvc_2_mic_handsfree_cvsd_asr, link_encoding_cvsd, 1, 0, BITFIELD_CAST(6, 0)};

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
    bool audio_busy = (bool)IsAudioBusy();
    
    switch (id)
    {
        case AUDIO_PLUGIN_CONNECT_MSG:
        {
            AUDIO_PLUGIN_CONNECT_MSG_T * connect_message = (AUDIO_PLUGIN_CONNECT_MSG_T *)message ;
            CvcPluginTaskdata* cvc_task = (CvcPluginTaskdata*)task;
            PRINT(("CVC: audio connect actioned\n"));
            CsrCvcPluginConnect(cvc_task, connect_message);
        }
        break ;
        
        case AUDIO_PLUGIN_DISCONNECT_MSG:
        {
            PRINT(("CVC: audio disconnect actioned\n"));
            CsrCvcPluginDisconnect((CvcPluginTaskdata*)task);
        }    
        break ;
        
        case AUDIO_PLUGIN_SET_MODE_MSG:
        {
            AUDIO_PLUGIN_SET_MODE_MSG_T * mode_message = (AUDIO_PLUGIN_SET_MODE_MSG_T *)message ;            
            
            if ((GetCurrentDspStatus() && (GetCurrentDspStatus() != DSP_RUNNING)))
            {
                MAKE_AUDIO_MESSAGE ( AUDIO_PLUGIN_SET_MODE_MSG, set_mode_message) ;
                set_mode_message->mode   = mode_message->mode ;
                set_mode_message->params = mode_message->params ;
                MessageSendConditionallyOnTask ( task, AUDIO_PLUGIN_SET_MODE_MSG , set_mode_message ,AudioBusyPtr()) ;
            }
            else
            {
                CsrCvcPluginSetMode((CvcPluginTaskdata*)task, mode_message->mode , mode_message->params) ;
            }
        }
        break ;
        
        /* Message from VM application via audio lib to configure the mute state of the microphone / outputs */
        case AUDIO_PLUGIN_SET_SOFT_MUTE_MSG:
        {
            AUDIO_PLUGIN_SET_SOFT_MUTE_MSG_T* mute_message = (AUDIO_PLUGIN_SET_SOFT_MUTE_MSG_T*)message;
            CsrCvcPluginSetSoftMute(mute_message);
        }
        break ;
        
        case AUDIO_PLUGIN_SET_VOLUME_MSG:
        {
            AUDIO_PLUGIN_SET_VOLUME_MSG_T * volume_message = (AUDIO_PLUGIN_SET_VOLUME_MSG_T *)message ;            
            CsrCvcPluginSetVolume(volume_message->volume);
        }
        break ;
        
        case AUDIO_PLUGIN_RESET_VOLUME_MSG:
        {
            CsrCvcPluginResetVolume();
        }        
        break ;

        case AUDIO_PLUGIN_PLAY_TONE_MSG:
        {
            AUDIO_PLUGIN_PLAY_TONE_MSG_T * tone_message = (AUDIO_PLUGIN_PLAY_TONE_MSG_T *)message ;
            
            if(IsTonePlaying() || IsVpPlaying()) 
            {
                if(tone_message->can_queue) /*then re-queue the tone*/
                {                
                    MAKE_AUDIO_MESSAGE( AUDIO_PLUGIN_PLAY_TONE_MSG, play_tone_message ) ;
                    
                    play_tone_message->tone        = tone_message->tone       ;
                    play_tone_message->can_queue   = tone_message->can_queue  ;
                    play_tone_message->tone_volume = tone_message->tone_volume;
                    play_tone_message->features    = tone_message->features   ;
    
                    PRINT(("TONE:Q\n"));
                    
                    MessageSendConditionallyOnTask ( task , AUDIO_PLUGIN_PLAY_TONE_MSG, play_tone_message ,AudioBusyPtr() ) ;
                }
            }
            else
            {
                PRINT(("TONE CVC:start\n"));
                CsrCvcCommonTonePlay ((CvcPluginTaskdata*)task, tone_message) ;
            }

        }
        break ;
        
        case AUDIO_PLUGIN_STOP_TONE_AND_PROMPT_MSG:
        {
            if (audio_busy)
            {
                PRINT(("CVC: stop tone\n"));
                CsrCvcCommonToneStop((CvcPluginTaskdata*)task) ;
            }
        }
        break ;

        case AUDIO_PLUGIN_TONE_END_NOTIFICATION_MSG:
        {
            CsrCvcCommonToneComplete((CvcPluginTaskdata*)task);
        }
        break;
        
#ifdef CVC_ALL
        case AUDIO_PLUGIN_MIC_SWITCH_MSG:
        {
            CsrCvcPluginMicSwitch();
        }
        break ;

        case AUDIO_PLUGIN_OUTPUT_SWITCH_MSG:
        {
            CsrCvcPluginOutputSwitch();
        }
        break;
#endif
        
        case AUDIO_PLUGIN_SET_POWER_MSG:
        {
            AUDIO_PLUGIN_SET_POWER_MSG_T * power_message = (AUDIO_PLUGIN_SET_POWER_MSG_T *)message ;
            CsrCvcPluginSetPower((CvcPluginTaskdata*)task, power_message->power) ;
        }
        break ;
        
        /* start or restart the ASR engine */
        case AUDIO_PLUGIN_START_ASR:
        {
            AUDIO_PLUGIN_START_ASR_T * asr_message = (AUDIO_PLUGIN_START_ASR_T *)message ;            
            
            if (GetCurrentDspStatus() && (GetCurrentDspStatus() != DSP_RUNNING))
            {
                MAKE_AUDIO_MESSAGE ( AUDIO_PLUGIN_START_ASR, start_asr_message) ;
                start_asr_message->mode   = asr_message->mode ;
        
                MessageSendConditionallyOnTask ( task, AUDIO_PLUGIN_START_ASR , start_asr_message ,AudioBusyPtr()) ;
            }
            else
            {
                CsrCvcPluginStartSpeechRecognitionIfSupported((CvcPluginTaskdata*)task) ;
            }
        }
        break ;

        case AUDIO_PLUGIN_TEST_RESET_MSG:
            CsrCvcPluginReset();
        break;
        
        default:
            /* Audio messages unsupported by the CVC plug-in */
        break ;
    }
}

/****************************************************************************
DESCRIPTION
    Internal messages to the task are handled here
*/ 
static void handleInternalMessage ( Task task , MessageId id, Message message )     
{
    switch (id)
    {
        case MESSAGE_STREAM_DISCONNECT: /*a tone has completed*/
        {
            PRINT(("CVC: Tone End\n"));
            CsrCvcCommonToneComplete((CvcPluginTaskdata*)task) ;
        }    
        break ;

        case MESSAGE_FORCE_TONE_COMPLETE:
        {
            CsrCvcCommonToneForceCompleteNoDsp();
        }
        break;
    
        default:
        {
            /* Handle DSP messages */
            CsrCvcCommonDspMessageHandler((CvcPluginTaskdata*)task , id , message ) ;
        }
        break ;
    }
}
