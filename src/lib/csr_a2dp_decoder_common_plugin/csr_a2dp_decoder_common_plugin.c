/****************************************************************************
Copyright (c) 2005 - 2016 Qualcomm Technologies International, Ltd.

FILE NAME
    csr_a2dp_decoder_common_plugin.c
DESCRIPTION
    an audio plugin
NOTES
*/
#include <audio.h>
#include <stdlib.h>
#include <print.h>
#include <stream.h> /*for the ringtone_note*/
#include <string.h>
#include <panic.h>

#include "audio_plugin_if.h"
#include "audio_plugin_music_variants.h"
#include "audio_plugin_common.h"
#include "csr_a2dp_decoder_common.h"
#include "csr_a2dp_decoder_common_if.h"
#include "csr_a2dp_decoder_common_subwoofer.h"
#include "csr_a2dp_decoder_common_sharing.h"
#include "csr_a2dp_decoder_if.h"
#include "csr_a2dp_decoder_common_plugin.h"

const A2dpPluginTaskdata csr_sbc_decoder_plugin = {{AudioPluginMusicMessageHandler}, SBC_DECODER, BITFIELD_CAST(8, 0)};
const A2dpPluginTaskdata csr_mp3_decoder_plugin = {{AudioPluginMusicMessageHandler}, MP3_DECODER, BITFIELD_CAST(8, 0)};
const A2dpPluginTaskdata csr_aac_decoder_plugin = {{AudioPluginMusicMessageHandler}, AAC_DECODER, BITFIELD_CAST(8, 0)};
const A2dpPluginTaskdata csr_faststream_sink_plugin = {{AudioPluginMusicMessageHandler}, FASTSTREAM_SINK, BITFIELD_CAST(8, 0)};
const A2dpPluginTaskdata csr_aptx_decoder_plugin = {{AudioPluginMusicMessageHandler}, APTX_DECODER, BITFIELD_CAST(8, 0)};
const A2dpPluginTaskdata csr_aptx_acl_sprint_decoder_plugin = {{AudioPluginMusicMessageHandler}, APTX_ACL_SPRINT_DECODER, BITFIELD_CAST(8, 0)};
const A2dpPluginTaskdata csr_aptxhd_decoder_plugin = {{AudioPluginMusicMessageHandler}, APTXHD_DECODER, BITFIELD_CAST(8, 0)};
const A2dpPluginTaskdata csr_tws_sbc_decoder_plugin = {{AudioPluginMusicMessageHandler}, TWS_SBC_DECODER, BITFIELD_CAST(8, 0)};
const A2dpPluginTaskdata csr_tws_mp3_decoder_plugin = {{AudioPluginMusicMessageHandler}, TWS_MP3_DECODER, BITFIELD_CAST(8, 0)};
const A2dpPluginTaskdata csr_tws_aac_decoder_plugin = {{AudioPluginMusicMessageHandler}, TWS_AAC_DECODER, BITFIELD_CAST(8, 0)};
const A2dpPluginTaskdata csr_tws_aptx_decoder_plugin = {{AudioPluginMusicMessageHandler}, TWS_APTX_DECODER, BITFIELD_CAST(8, 0)};
const A2dpPluginTaskdata csr_fm_decoder_plugin = {{AudioPluginMusicMessageHandler}, FM_DECODER, BITFIELD_CAST(8, 0)};
const A2dpPluginTaskdata csr_analogue_decoder_plugin = {{AudioPluginMusicMessageHandler}, SBC_DECODER, BITFIELD_CAST(8, 0)};
const A2dpPluginTaskdata csr_i2s_decoder_plugin = {{AudioPluginMusicMessageHandler}, SBC_DECODER, BITFIELD_CAST(8, 0)};
const A2dpPluginTaskdata csr_spdif_decoder_plugin = {{AudioPluginMusicMessageHandler}, SBC_DECODER, BITFIELD_CAST(8, 0)};


    /*the local message handling functions*/
static void handleAudioMessage ( Task task , MessageId id, Message message );
static void handleInternalMessage ( Task task , MessageId id, Message message );

/****************************************************************************
DESCRIPTION
    The main task message handler
*/
void AudioPluginMusicMessageHandler(Task task, MessageId id, Message message)
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
    switch (id)
    {
        case (AUDIO_PLUGIN_CONNECT_MSG ):
        {
            AUDIO_PLUGIN_CONNECT_MSG_T * connect_message = (AUDIO_PLUGIN_CONNECT_MSG_T *)message ;
            CsrA2dpDecoderPluginConnect((A2dpPluginTaskdata*)task,
                                        connect_message->audio_sink ,
                                        connect_message->sink_type ,
                                        connect_message->volume ,
                                        connect_message->rate ,
                                        connect_message->features ,
                                        connect_message->mode   ,
                                        connect_message->params ,
                                        connect_message->app_task ) ;
        }   
        break ;

        /* start the disconnect procedure, issue mute then unload dsp */
        case (AUDIO_PLUGIN_DISCONNECT_MSG ):
        {
            /* begin muting the output to disconnect the audio */
            CsrA2dpDecoderPluginStartDisconnect(task);

        }   
        break ;
        
        /* disconnect the streams and unload kalimba once muted */
        case AUDIO_PLUGIN_DISCONNECT_DELAYED_MSG:
        {
            CsrA2dpDecoderPluginDisconnect((A2dpPluginTaskdata*)task) ;
        }   
        break;
        case (AUDIO_PLUGIN_SET_MODE_MSG ):
        {
            AUDIO_PLUGIN_SET_MODE_MSG_T * mode_message = (AUDIO_PLUGIN_SET_MODE_MSG_T *)message ;
            CsrA2dpDecoderPluginSetMode(mode_message->mode , mode_message->params) ;
        }
        break ;        
        
        /* message from VM application via audio lib to configure the sub woofer, this could be
           an enable or disable indication along with sub woofer link type, esco or l2cap */
        case (AUDIO_PLUGIN_SET_SUB_WOOFER_MSG ):
        {
            AUDIO_PLUGIN_SET_SUB_WOOFER_MSG_T * sub_message = (AUDIO_PLUGIN_SET_SUB_WOOFER_MSG_T *)message ;
            CsrA2dpDecoderPluginSetSubWoofer(sub_message->sub_woofer_type , sub_message->sub_sink) ;
        }
        break ;    
        
        /* Message from VM application via audio lib to configure the mute state of the sink, this could be to either:
            mute the sink output but not subwoofer
            mute the subwoofer output but not sink
            mute both sink and subwoofer
            unmute both sink and subwoofer */
        case (AUDIO_PLUGIN_SET_SOFT_MUTE_MSG ):
        {
            AUDIO_PLUGIN_SET_SOFT_MUTE_MSG_T* mute_message = (AUDIO_PLUGIN_SET_SOFT_MUTE_MSG_T*)message;
            CsrA2dpDecoderPluginSetSoftMute(mute_message);
        }
        break ;

        /* allow volume messages to be processed by releasing the audio busy flag lock */
        case (AUDIO_PLUGIN_ALLOW_VOLUME_CHANGES_MSG) :
        {
            /* release the lock on setting volume changes to allow smooth volume ramping */
            CsrA2dpDecoderPluginAllowVolChanges();
        }
        break;

        /* should the subwoofer fail to open a media channel within a timeout period begin the unmuting
           procedure anyway to prevent stalled audio connection */
        case (AUDIO_PLUGIN_SUBWOOFER_CONNECTION_TIMEOUT_MSG):
        {
            /* check if sub connected port within timeout period */
            CsrA2dpDecoderPluginSubCheckForConnectionFailure();
        }
        break;
        
        case (AUDIO_PLUGIN_SET_GROUP_VOLUME_MSG ): 
        {
            AUDIO_PLUGIN_SET_GROUP_VOLUME_MSG_T * volume_message = (AUDIO_PLUGIN_SET_GROUP_VOLUME_MSG_T *)message ;
            CsrA2dpDecoderPluginSetVolume (volume_message) ;
        }
        break ;
        
        case (AUDIO_PLUGIN_RESET_VOLUME_MSG ): 
        {
            /* Not used any longer used */
            if (IsAudioBusy())
            {
                 MessageSendConditionallyOnTask ( task, AUDIO_PLUGIN_RESET_VOLUME_MSG , 0, AudioBusyPtr() ) ;
            }
            else
            {
                CsrA2dpDecoderPluginResetVolume () ;
            }           
        }       
        break ;
        
        /* Message from VM application via audio lib to configure the mute state of the input audio port */
        case (AUDIO_PLUGIN_SET_INPUT_AUDIO_MUTE_MSG):
        {
            AUDIO_PLUGIN_SET_INPUT_AUDIO_MUTE_MSG_T *mute_message = (AUDIO_PLUGIN_SET_INPUT_AUDIO_MUTE_MSG_T *)message ;
            CsrA2dpDecoderPluginSetInputAudioMute(mute_message) ;
        }
        break ;        
        
        case (AUDIO_PLUGIN_SET_ROUTE_MSG ):
            /* Routing is fixed in this plugin - ignore */              
        break ;
        
        case (AUDIO_PLUGIN_START_FORWARDING_MSG ):
        {
            AUDIO_PLUGIN_START_FORWARDING_MSG_T * fwd_msg = (AUDIO_PLUGIN_START_FORWARDING_MSG_T *)message ;
            CsrA2dpDecoderPluginForwardUndecoded((A2dpPluginTaskdata *)task, TRUE, fwd_msg->forwarding_sink, fwd_msg->content_protection, fwd_msg->peer_dsp_required_buffering_level) ;
        }
        break ;     
        
        case (AUDIO_PLUGIN_STOP_FORWARDING_MSG ):
        {
            CsrA2dpDecoderPluginForwardUndecoded((A2dpPluginTaskdata *)task, FALSE, NULL, 0, 0) ;
        }
        break ;     
        
        case (AUDIO_PLUGIN_PLAY_TONE_MSG ):
        {
            /* Let tone plugin do the job */
            AUDIO_PLUGIN_PLAY_TONE_MSG_T * tone_message = (AUDIO_PLUGIN_PLAY_TONE_MSG_T *)message;
            MAKE_AUDIO_MESSAGE(AUDIO_PLUGIN_PLAY_TONE_MSG, new_message);

            memcpy(new_message, tone_message, sizeof(AUDIO_PLUGIN_PLAY_TONE_MSG_T));

            MessageSend(AudioGetTonePlugin(), AUDIO_PLUGIN_PLAY_TONE_MSG, new_message);

            /* Set audio busy here otherwise other pending tone messages will be sent */
            SetAudioBusy(AudioGetTonePlugin());
        }
        break ;
        
        case (AUDIO_PLUGIN_STOP_TONE_AND_PROMPT_MSG ):
        {
            MessageSend(AudioGetTonePlugin(), AUDIO_PLUGIN_STOP_TONE_AND_PROMPT_MSG, NULL);
        }
        break ;     
        
        case (AUDIO_PLUGIN_DELAY_VOLUME_SET_MSG):
        {
            AUDIO_PLUGIN_DELAY_VOLUME_SET_MSG_T * volume_message = (AUDIO_PLUGIN_DELAY_VOLUME_SET_MSG_T *)message ;
            CsrA2dpDecoderPluginSetHardwareLevels(volume_message);
        }
        break;
        
        case (AUDIO_PLUGIN_SET_VOLUME_MSG ): 
        {
            PRINT(("A2DP: Set volume not used in this plugin\n"));
        }
        break;
        case AUDIO_PLUGIN_TEST_RESET_MSG :
        {
            CsrA2dpDecoderPluginTestReset();
        }
        break;

        default:
        {
        }
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
      case MESSAGE_STREAM_DISCONNECT:
      {
      }
      break;
      
      default:
      {
            /*route the dsp messages to the relevant handler*/
            CsrA2dpDecoderPluginInternalMessage( (A2dpPluginTaskdata*)task , id , message ) ;        
      }
      break ;
    }   
      
}

/****************************************************************************
DESCRIPTION
    get the current sample rate if available
*/ 
uint32 AudioGetA2DPSampleRate(void)
{
    return CsrA2DPGetDecoderSampleRate();
}

/****************************************************************************
DESCRIPTION
    get the current subwoofer sample rate if available
*/ 
uint32 AudioGetA2DPSubwooferSampleRate(void)
{
    return CsrA2DPGetDecoderSubwooferSampleRate();
}     
        
/****************************************************************************
DESCRIPTION
    Get current/estimated latency measurement for specified plugin
*/ 
bool AudioGetLatency (Task audio_plugin, bool *estimated, uint16 *latency)
{
    return audioDecodePluginGetLatency((A2dpPluginTaskdata *)audio_plugin, estimated, latency);
}
