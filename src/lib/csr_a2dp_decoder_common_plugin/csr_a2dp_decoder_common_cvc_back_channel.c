/****************************************************************************
Copyright (c) 2005 - 2016 Qualcomm Technologies International, Ltd.

FILE NAME
    csr_decoder_cvc_back_channel.c
DESCRIPTION
    cvc back channel specific functions
NOTES
*/

#include <audio.h>
#include <source.h>
#include <gain_utils.h>
#include <stdlib.h>
#include <panic.h>
#include <print.h>
#include <file.h>
#include <stream.h> /*for the ringtone_note*/
#include <kalimba.h>
#include <kalimba_standard_messages.h>
#include <message.h>
#include <ps.h>
#include <transform.h>
#include <string.h>
#include <pio_common.h>
#include <pblock.h>

#include "csr_i2s_audio_plugin.h"
#include "audio_plugin_if.h" /*for the audio_mode*/
#include "audio_plugin_common.h"
#include "csr_a2dp_decoder_common.h"
#include "csr_a2dp_decoder_common_if.h"
#include "csr_a2dp_decoder_common_cvc_back_channel.h"
#include "csr_a2dp_decoder_if.h"


/****************************************************************************
DESCRIPTION
    This function disconnects any CVC microphone back channels
****************************************************************************/    
void CsrA2dpDecoderPluginDisconnectMic(A2dpPluginConnectParams *codecData)
{
    DECODER_t * DECODER = CsrA2dpDecoderGetDecoderData();
    Source l_src = AudioPluginGetMic(AudioPluginGetInstance(codecData->mic_params->mic_a),
                                     AUDIO_CHANNEL_A, codecData->mic_params->mic_a.digital);
            
    /* if microphone back channel is connected then disconnect it */
    if (l_src)
    {
        /* disconnect and close the source */
        StreamDisconnect(l_src, 0);
        SourceClose(l_src);

        /* reset the mic bias pio drive */
        AudioPluginSetMicPio(codecData->mic_params->mic_a, FALSE);
    }

    /* one or two mic back channel ? */
    if(DECODER->features.use_two_mic_back_channel)
    {
        Source r_src = AudioPluginGetMic(AudioPluginGetInstance(codecData->mic_params->mic_b),
                                         AUDIO_CHANNEL_B, codecData->mic_params->mic_b.digital);

    	/* if right mic channel is connected then disconnect it */
        if(r_src)
        {
            /* disconnect and close the source */
            StreamDisconnect(r_src, 0);
            SourceClose(r_src);
    
            /* reset the mic bias pio for mic B */
            AudioPluginSetMicPio(codecData->mic_params->mic_b, FALSE);
        }
    }
}


/* Bit set in DSP volume message to indicate digital volume is to be used */
#define DSP_DIG_VOL_FLAG (1 << 4)

/****************************************************************************
DESCRIPTION
    Send the volume to the DSP
*/
static void csrA2dpDecoderPluginSendLlDspGain(int16 master, int16 tone)
{
    uint16 digital_dsp = 0;
    
    if (AudioOutputGainGetType(audio_output_group_main) == audio_output_gain_digital)
    {
        audio_output_gain_t gain;
        
        /* DSP needs to know the fixed system gain, no need to pass master/tone gain
           so set them to zero and call function to get the fixed system gain. */
        AudioOutputGainGetDigital(audio_output_group_main, 0, 0, &gain);
        
        digital_dsp = (uint16)(DSP_DIG_VOL_FLAG | (gain.common.system & 0xF));
    }
    
    /* Notify DSP of new volume */
    KALIMBA_SEND_MESSAGE(LOW_LATENCY_VOLUME_MSG, 0, digital_dsp, (uint16)master, (uint16)tone);
}

/****************************************************************************
DESCRIPTION
    Set hardware volume when codec message received from DSP
*/
static void csrA2dpDecoderPluginSetLlHwGain(int16 master)
{
    int16 master_dB;
    master_dB = VolumeConvertDACGainToDB(master);

    if (AudioOutputGainGetType(audio_output_group_main) == audio_output_gain_digital)
    {
        /* Set main hw volume, back channel apps don't support multi-channel */
        AudioOutputGainSetHardware(audio_output_group_main, master_dB, NULL);
    }
    else
    {
        /* Hybrid/Hardware mode - set the gain in hardware only */
        AudioOutputGainSetHardwareOnly(audio_output_group_main, master_dB);
    }
}


/****************************************************************************
DESCRIPTION
    Sets the audio mode for low latency back channel applications. Should only
    be called if isCodecLowLatencyBackChannel().
*/
void csrA2dpDecoderPluginSetLowLatencyMode(AUDIO_MODE_T mode, const A2dpPluginModeParams *mode_params, A2DP_MUSIC_PROCESSING_T music_processing)
{
    DECODER_t * DECODER = CsrA2dpDecoderGetDecoderData();
    /* pre-initialise with the most common parameters and adjust below as necessary */
    A2dpPluginConnectParams *codecData = (A2dpPluginConnectParams *) DECODER->params;
    uint16 sysmode;
    uint16 call_state = CALLST_CONNECTED;
    int16 volume;

    if (!DECODER)
        Panic() ;

    /* update the current operating mode stored value */
    DECODER->mode = mode;

    /* set current volume level */
    volume  = DECODER->volume.main.master;
    
    /* default operating mode of full processing */
    sysmode = SYSMODE_HFK;

    /* determine current operating mode */
    switch (mode)
    {
        case AUDIO_MODE_STANDBY:
        {
            /* Always standby */
            sysmode = SYSMODE_STANDBY;
            volume = DAC_MUTE;
            KALIMBA_SEND_MESSAGE(LOW_LATENCY_SETMODE_MSG, MUSIC_SYSMODE_STANDBY, 1, 0, (uint16)((mode_params->external_mic_settings << 1)+(mode_params->mic_mute)));
        }
        break;
        /* left/1st mic pass through mode */
        case AUDIO_MODE_LEFT_PASSTHRU:
            sysmode = SYSMODE_LEFT_PASSTHRU;        
        break;

        /* right/2nd mic pass through mode */        
        case AUDIO_MODE_RIGHT_PASSTHRU:
            sysmode = SYSMODE_RIGHT_PASSTHRU;        
        break;
        
        /* low volume operating mode */
        case AUDIO_MODE_LOW_VOLUME:
            sysmode = SYSMODE_LOW_VOLUME;
        break;
        
        /* no mute required, full processing as default mode */        
        case AUDIO_MODE_CONNECTED:                   
            
           /* set the current EQ mode of operation */
           CsrA2dpDecoderLowLatencyPluginSetEqMode(music_processing);

            /* update the audio enhancements settings */
           audioDecodePluginUpdateEnhancements(mode_params);

        break;
        
        case AUDIO_MODE_MUTE_MIC:
        case AUDIO_MODE_MUTE_SPEAKER:
        case AUDIO_MODE_MUTE_BOTH:
        case AUDIO_MODE_UNMUTE_SPEAKER:
        {
            PRINT(("DECODER: *** Muting via SET_MODE_MSG is deprecated ***\n"));
            PRINT(("DECODER: Use SET_SOFT_MUTE_MSG instead\n"));
            Panic();
        }
        return;
            
        default:
        {
            PRINT(("DECODER: Set Audio Mode Invalid [%x]\n" , mode ));
        }
        break;
    }
	/*send update to kalimba */
    PRINT(("CVC: SYSMODE 0x%x, CALLSTATE 0x%x, VOL 0x%x\n", sysmode, call_state, volume));
    csrA2dpDecoderPluginSendLlDspGain(volume, DECODER->volume.main.tone);
    KALIMBA_SEND_MESSAGE(LOW_LATENCY_SETMODE_MSG, sysmode, 0, call_state, (uint16)(( codecData->mic_params->mic_a.digital << 1 ) | codecData->mic_params->mic_b.digital) );
}

/******************************************************************************
DESCRIPTION
    Send the soft mute message for low latency applications to the DSP.
    Should only be called if isCodecLowLatencyBackChannel().
*/
void csrA2dpDecoderPluginLowLatencyOutputMute(AUDIO_MUTE_STATE_T state)
{
    DECODER_t* DECODER = CsrA2dpDecoderGetDecoderData();
    
    if (DECODER == NULL)
        return;
    
    if (DECODER->mute_state[audio_mute_group_main] != state)
    {
        AUDIO_LL_SOFT_MUTE_TYPE_T mute_type;
        
        /* Separate sub control deprecated, mute or unmute both together */
        if (state & AUDIO_MUTE_MASK(audio_mute_group_main))
            mute_type = mute_sink_and_sub;
        else
            mute_type = unmute_sink_and_sub;
            
        PRINT(("MUTE TYPE [%x] applied (low latency)\n", mute_type));
        KALIMBA_SEND_MESSAGE(LOW_LATENCY_SOFT_MUTE, mute_type, 0, 0, 0);
        
        DECODER->mute_state[audio_mute_group_main] = state;
    }
}

/******************************************************************************
DESCRIPTION
    Set the soft mute state for the sink/subwoofer in low latency applications.
    Should only be called if isCodecLowLatencyBackChannel().
*/
void csrA2dpDecoderPluginSetLowLatencySoftMute(const AUDIO_PLUGIN_SET_SOFT_MUTE_MSG_T* message)
{
    AUDIO_MUTE_STATE_T new_main_state = AUDIO_MUTE_DISABLE;
    AUDIO_MUTE_STATE_T new_mic_state = AUDIO_MUTE_DISABLE;
    
    if (message->mute_states & AUDIO_MUTE_MASK(audio_mute_group_main))
        new_main_state = AUDIO_MUTE_ENABLE;
    
    if (message->mute_states & AUDIO_MUTE_MASK(audio_mute_group_mic))
        new_mic_state = AUDIO_MUTE_ENABLE;
    
    csrA2dpDecoderPluginLowLatencyOutputMute(new_main_state);
    csrA2dpDecoderPluginMicMute(new_mic_state);
}

/****************************************************************************
DESCRIPTION
    Utility function to set output gains for the CVC back channel variants.
    Low latency apps using 16 levels of DAC volume control or I2S/SPDIF 
    output.

    @return void
*/
void CsrA2dpDecoderPluginSetLowLatencyGain(int16 master_gain, int16 tone_gain)
{
    /* convert from dB values back to DAC gains */
    int16 volume = (int16)(CODEC_STEPS + (master_gain / DB_TO_DAC));
    int16 tone_volume = (int16)(CODEC_STEPS + (tone_gain / DB_TO_DAC));
    
    /* DAC gain only goes down to -45dB, dsp volume control goes to -60dB */
    if(volume < 0) volume = 0;
    if(tone_volume < 0) tone_volume = 0;
    
    /* Send volume change message */
    csrA2dpDecoderPluginSendLlDspGain(volume, tone_volume);
    
    PRINT(("DSP LowLat Gains: Volume = %d TonesVol = %d\n", volume, tone_volume));
}

/****************************************************************************
DESCRIPTION
    utility function to set microphone gains for the CVC back channel variants

    @return void
*/
void CvcMicSetGain(audio_instance instance, audio_channel channel, bool digital, T_mic_gain gain)
{
    Source mic_source = AudioPluginGetMic(instance, channel, digital);
    uint8 mic_gain = (digital ? gain.digital_gain : gain.analogue_gain);
    AudioPluginSetMicGain(mic_source, digital, mic_gain, gain.preamp_enable);
}

/****************************************************************************
DESCRIPTION
    utility function to connect the 1 or two mics to the cvc back channel app

    @return void
*/
void CsrA2dpDecoderConnectBackChannel(A2dpPluginConnectParams * codecData, bool use_two_mic_back_channel)
{
    DECODER_t * DECODER = CsrA2dpDecoderGetDecoderData();

    /****************************************/
    /* connect the back channel microphones */
    /****************************************/

    /* obtain source for mic a */
    Source mic_source_a = AudioPluginGetMic(AudioPluginGetInstance(codecData->mic_params->mic_a),
                                            AUDIO_CHANNEL_A, codecData->mic_params->mic_a.digital);
    /* set microphone sample rate */
    AudioPluginSetMicRate(mic_source_a, codecData->mic_params->mic_a.digital, BACK_CHANNEL_SAMPLE_RATE); 
    /* Set mic bias pin */
    AudioPluginSetMicPio(codecData->mic_params->mic_a, TRUE);

    /* set mic bias pin for second microphone if 2 mic variant is chosen */    
    if(use_two_mic_back_channel)
    {
        /* obtain source for microphone b */
        Source mic_source_b = AudioPluginGetMic(AudioPluginGetInstance(codecData->mic_params->mic_b),
                                                AUDIO_CHANNEL_B, codecData->mic_params->mic_b.digital);
        /* set mic b input sample rate */
        AudioPluginSetMicRate(mic_source_b, codecData->mic_params->mic_b.digital, BACK_CHANNEL_SAMPLE_RATE); 
        /* set mic b bias pin */
        AudioPluginSetMicPio(codecData->mic_params->mic_b, TRUE);
        /* synchronise the two microphone inputs */
        SourceSynchronise(mic_source_a, mic_source_b);
        /* connect 1st mic to dsp */
        PanicNull(StreamConnect(mic_source_a, audioDecoderGetDspSink(dsp_sink_mic_a)));
        PRINT(("A2DP: connect 1-mic back channel\n"));
        /* connect 2nd mic to dsp */
        PanicNull(StreamConnect(mic_source_b, audioDecoderGetDspSink(dsp_sink_mic_b)));
        PRINT(("A2DP: connect 2-mic back channel\n"));
    }
    else
    {
        /* connect 1st mic to dsp */
        PanicNull(StreamConnect(mic_source_a, audioDecoderGetDspSink(dsp_sink_mic_a)));
        PRINT(("A2DP: connect 1-mic back channel\n"));
    }

    /* stream currently disposed, disconnect and */
    StreamDisconnect(0, DECODER->media_sink);
    /* connect the back channel to the l2cap link */
    PanicNull(StreamConnect(audioDecoderGetDspSource(dsp_source_cvc_back_channel),DECODER->media_sink));

}

/****************************************************************************
DESCRIPTION
    utility function to determine which low latency application is running and 
    whether it supports back channel operation or not

    @return back channel support as true or false
*/
bool isCodecLowLatencyBackChannel(void)
{
#ifdef CVC_BACK_CHANNEL    
    DECODER_t * DECODER = CsrA2dpDecoderGetDecoderData();

    /* determine if the app running supports back channel */
    if(DECODER && ((DECODER->task->a2dp_plugin_variant == FASTSTREAM_SINK)||(DECODER->task->a2dp_plugin_variant == APTX_ACL_SPRINT_DECODER))&&
        (DECODER->features.use_one_mic_back_channel || DECODER->features.use_two_mic_back_channel))
    {
        return TRUE;
    }
#endif        
    return FALSE;
}

/****************************************************************************
DESCRIPTION
    This function handles the messages for the low latency codec types including 
    FASTSTREAM and APTX LL, the message ID's overlap between the standard and 
    low latency dsp apps when the back channel is enabled and hence a separate 
    message handler is required   
    
****************************************************************************/
void CsrA2dpDecoderPluginLowLatencyInternalMessage(A2dpPluginTaskdata *task ,uint16 id , Message message)
{
    A2DP_DECODER_PLUGIN_TYPE_T variant = task->a2dp_plugin_variant;
    DECODER_t * DECODER = CsrA2dpDecoderGetDecoderData();
    A2dpPluginConnectParams* codecData;

    /* dsp message handler, determine message id */
    switch(id)
    {
       /* message validity check */
       case MESSAGE_FROM_KALIMBA:
       {
            const DSP_REGISTER_T *m = (const DSP_REGISTER_T *) message;
            PRINT(("LOW_LATENCY DECODER: msg id[%x] a[%x] b[%x] c[%x] d[%x]\n", m->id, m->a, m->b, m->c, m->d));

            switch ( m->id )
            {
               case MUSIC_TONE_COMPLETE:
                   if (IsTonePlaying() || IsVpPlaying())
                   {
                       CsrA2dpDecoderPluginToneComplete() ;
                   }
                break;
                /* get the pskey configuration data repsonse */
                case LOW_LATENCY_LOAD_PERSIST:
                {
                    /* a=sysid, b=len */
                    const pblock_entry* entry = PblockGet(m->a);
                    KalimbaSendLongMessage(LOW_LATENCY_LOAD_PERSIST_RESP, entry->size, entry->data);                       
                }
                break;                
                
                /* indication that the dsp is loaded and ready to accept configuration data */
                case LOW_LATENCY_READY_MSG:
                {
                    if (DECODER)
                    {
                        codecData = (A2dpPluginConnectParams *) DECODER->params;

                        /* load configuration parameters from ps, different for low latency variants */
                        /* different pskey base address for cvc back channel support */
                        KALIMBA_SEND_MESSAGE(LOW_LATENCY_LOADPARAMS_MSG, MUSIC_PS_BASE_WBS_BACK_CHANNEL, CVC_SR_WB, 0, 0);
                        
                        if(codecData->silence_threshold)
                        {
                            /* Set silence detection params */
                            PRINT(("DECODER: Message SILENCE_DETECTION_PARAMS_MSG \n"));
                            PRINT(("Threshold %x, Timeout %x\n", codecData->silence_threshold, codecData->silence_trigger_time));
                            KALIMBA_SEND_MESSAGE(LOW_LATENCY_SIGNAL_DET_SET_PARAMS_MSG, codecData->silence_threshold, codecData->silence_trigger_time, 0, 0);
                        }

                        /* Tell the DSP what plugin type is being used */
                        KALIMBA_SEND_MESSAGE(LOW_LATENCY_SET_PLUGIN_MSG, variant, 0, 0, 0);
                        
                        PRINT(("DECODER: Message MUSIC_SET_PLUGIN_MSG variant = %x\n",variant));

                        /* update current dsp status */
                        {
                            /* send status message to app to indicate dsp is ready to accept data,
                               applicable to A2DP media streams only */
                            MAKE_AUDIO_MESSAGE(AUDIO_PLUGIN_DSP_READY_FOR_DATA, dsp_ready_message);
                            PRINT(("DECODER: Send CLOCK_MISMATCH_RATE\n"));
                            dsp_ready_message->plugin = (TaskData *)task;
                            dsp_ready_message->AUDIO_BUSY = AudioBusyTask();
                            dsp_ready_message->dsp_status = GetCurrentDspStatus();
                            dsp_ready_message->media_sink = DECODER->media_sink;
                            MessageSend(DECODER->app_task, AUDIO_PLUGIN_DSP_READY_FOR_DATA, dsp_ready_message);
    
                            /*A2dp is now loaded, signal that tones etc can be scheduled*/
                            SetAudioBusy( NULL ) ;
                            PRINT(("DECODER: DECODER_READY \n"));

                            /* connect the dsp ports to the audio streams */
                            MusicConnectAudio();
                            /* update current dsp status */
                            SetCurrentDspStatus( DSP_RUNNING );
                        }
                    }
                    else
                    {
                        /* update current dsp status */
                        SetCurrentDspStatus( DSP_ERROR );
                    }
                }
                break;

                /* dsp confirmation that a volume change has been processed */
                case LOW_LATENCY_CODEC_MSG:
                {
                    if (DECODER)
                    {  
                        /* low latency codecs return the mic gain values and a combined left/right output level */
                        T_mic_gain lInput_gain_l;
                        T_mic_gain lInput_gain_r;
                        codecData = (A2dpPluginConnectParams *) DECODER->params;

                        /* if using the microphone or spdif/i2s back channel */   
                        if(isCodecLowLatencyBackChannel())
                        {
                            lInput_gain_l = *(const T_mic_gain*)&m->b;
                            lInput_gain_r = *(const T_mic_gain*)&m->c;                       

                            /* set the input gain values */
                            CvcMicSetGain(AudioPluginGetInstance(codecData->mic_params->mic_a),
                                          AUDIO_CHANNEL_A, codecData->mic_params->mic_a.digital, lInput_gain_l);

                            if(DECODER->features.use_two_mic_back_channel)
                                CvcMicSetGain(AudioPluginGetInstance(codecData->mic_params->mic_b),
                                              AUDIO_CHANNEL_B, codecData->mic_params->mic_b.digital, lInput_gain_r);
                        }

                        csrA2dpDecoderPluginSetLlHwGain((int16)m->a);
                    }
                }
                break;

                /* dsp status information gets sent to the vm app */
                case KALIMBA_MSG_SOURCE_CLOCK_MISMATCH_RATE:
                {
                    if (DECODER)
                    {
                        MAKE_AUDIO_MESSAGE_WITH_LEN(AUDIO_PLUGIN_DSP_IND, 1, dsp_ind_message);
                        PRINT(("DECODER: Send CLOCK_MISMATCH_RATE\n"));
                        dsp_ind_message->id = KALIMBA_MSG_SOURCE_CLOCK_MISMATCH_RATE;
                        dsp_ind_message->size_value = 1;
                        dsp_ind_message->value[0] = m->a;
                        MessageSend(DECODER->app_task, AUDIO_PLUGIN_DSP_IND, dsp_ind_message);
                    }
                }
                break;

                case LOW_LATENCY_CUR_EQ_BANK:
                /* DSP tells plugin which EQ is active.  Send this value to the VM app
                   so the current EQ setting can be restored when the device is repowered.
                */
                {
                    if (DECODER)
                    {
                        MAKE_AUDIO_MESSAGE_WITH_LEN(AUDIO_PLUGIN_DSP_IND, 1, dsp_ind_message);
                        PRINT(("DECODER: Current EQ setting: [%x][%x]\n", m->a, m->b));
                        dsp_ind_message->id = A2DP_MUSIC_MSG_CUR_EQ_BANK;
                        dsp_ind_message->size_value = 1;
                        dsp_ind_message->value[0] = m->a;
                        MessageSend(DECODER->app_task, AUDIO_PLUGIN_DSP_IND, dsp_ind_message);
                    }
                }
                break;

                case LOW_LATENCY_APTX_SEC_PASSED:
                {
                    PRINT(("aptX: Security passed.\n"));
/*                    KalimbaSendMessage(LOW_LATENCY_APTX_SECURITY_MSG, 1, 0, 0, 0);*/
                }
                break;

                case LOW_LATENCY_APTX_SEC_FAILED:
                {
                    PRINT(("aptX: Security failed.\n"));
/*                    KalimbaSendMessage(LOW_LATENCY_APTX_SECURITY_MSG, 0, 0, 0, 0);*/
                }
                break;

                case LOW_LATENCY_SIGNAL_DET_STATUS:
                /* DSP tells when signal detector status has changed
                Param1 == 0 => no audio - go into standby
                Param1 == 1 => receiving audio - come out of standby
                "no audio" message is sent when signal level has been below the
                threshold level for the trigger time out period "receiving audio"
                message is sent as soon as a signal is detected above the threshold level
                */

                {
                    uint16 signal = m->a;
                    PRINT(("SIGNAL_DETECTOR_STATUS_RESP: PARAM1 %x \n", signal));

                    {
                        MAKE_AUDIO_MESSAGE(AUDIO_PLUGIN_AUDIO_SIGNAL_DETECT_MSG, signal_detect_message);
                        signal_detect_message->signal_detected = signal;
                        MessageSend(DECODER->app_task, AUDIO_PLUGIN_AUDIO_SIGNAL_DETECT_MSG, signal_detect_message);
                    }
                }
                break;

                case LOW_LATENCY_CVC_SEC_PASSED:
                    PRINT(("CVC:  Sec passed.\n"));
                     /*cvc is now loaded, signal that tones etc can be scheduled*/
                    SetAudioBusy(NULL) ;
                break;

                case LOW_LATENCY_CVC_SEC_FAILED:
                    PRINT(("CVC: Security has failed.\n"));
                    /*cvc is now loaded, signal that tones etc can be scheduled*/
                    SetAudioBusy(NULL) ;
                break;

                case KALIMBA_MSG_CBOPS_SILENCE_CLIP_DETECT_CLIP_DETECTED_ID:
                {
                    PRINT(("\n\n\n\n\nInput level clipped.\n"));
                }
                break;                      

                case KALIMBA_MSG_CBOPS_SILENCE_CLIP_DETECT_SILENCE_DETECTED_ID:
                {
                    PRINT(("\n\n\n\n\nInput level silence.\n"));
                }
                break;   

                case KALIMBA_MSG_LATENCY_REPORT:
                {   /* DSP has sent us an audio latency measurement */
                    if (DECODER)
                    {
                    	CsrA2dpDecoderPluginSetAudioLatency(task, m->a);
                    }
                }
                break;

                default:
                    PRINT(("unhandled message id [%x]\n",m->id));
                break;
            }
        }
        break;

        case MESSAGE_FROM_KALIMBA_LONG:
        {
            const DSP_LONG_REGISTER_T *m = (const DSP_LONG_REGISTER_T*) message;
            PRINT(("CVC: LONG_MESSAGE_FROM_KALIMBA id[0x%x] l[0x%x] \n", m->id, m->size));
            switch (m->id)
            {
                case LOW_LATENCY_STOREPERSIST_MSG:
                    /* Set the DSP app's pblock */
                   PRINT(("CVC: StorePersist key[0x%x], data[0x%x] \n", m->buf[0], m->buf[1]));
                   PblockSet((uint16)m->buf[0], (uint16)(m->size-1), (uint16*)m->buf+1);
                break;
                
                default:
                break;
            }
        }
        break;
        default:
        break ;
    }    
}



/****************************************************************************
DESCRIPTION
    utility function to set the current EQ operating mode of the low latency bac channel apps

    @return void
*/
void CsrA2dpDecoderLowLatencyPluginSetEqMode(A2DP_MUSIC_PROCESSING_T music_processing)
{
    /* determine the music processing mode requirements, set dsp music mode appropriately */
    switch (music_processing)
    {
        case A2DP_MUSIC_PROCESSING_PASSTHROUGH:
            {
                KALIMBA_SEND_MESSAGE (LOW_LATENCY_SET_EQ_MSG , MUSIC_SYSMODE_PASSTHRU , MUSIC_DO_NOT_CHANGE_EQ_BANK, 0, 0 );
                PRINT(("DECODER: Set Low Latency Mode SYSMODE_PASSTHRU\n"));
            }
            break;

        case A2DP_MUSIC_PROCESSING_FULL:
            {
                KALIMBA_SEND_MESSAGE (LOW_LATENCY_SET_EQ_MSG , MUSIC_DO_NOT_CHANGE_EQ_BANK, 0, 0, 0 );
                PRINT(("DECODER: Set Low Latency Mode SYSMODE_FULLPROC\n"));
            }
            break;

        case A2DP_MUSIC_PROCESSING_FULL_NEXT_EQ_BANK:
            {
                KALIMBA_SEND_MESSAGE (LOW_LATENCY_SET_EQ_MSG , MUSIC_NEXT_EQ_BANK, 0, 0 ,0);
                PRINT(("DECODER: Set Low Latency Mode SYSMODE_FULLPROC and advance to next EQ bank\n"));
            }
            break;

        case A2DP_MUSIC_PROCESSING_FULL_SET_EQ_BANK0:
        case A2DP_MUSIC_PROCESSING_FULL_SET_EQ_BANK1:
        case A2DP_MUSIC_PROCESSING_FULL_SET_EQ_BANK2:
        case A2DP_MUSIC_PROCESSING_FULL_SET_EQ_BANK3:
        case A2DP_MUSIC_PROCESSING_FULL_SET_EQ_BANK4:
        case A2DP_MUSIC_PROCESSING_FULL_SET_EQ_BANK5:
        case A2DP_MUSIC_PROCESSING_FULL_SET_EQ_BANK6:
            {
                KALIMBA_SEND_MESSAGE(LOW_LATENCY_SET_EQ_MSG , MUSIC_SET_EQ_BANK, (uint16)(music_processing - A2DP_MUSIC_PROCESSING_FULL_SET_EQ_BANK0), 0, 0);
                PRINT(("DECODER: Set Low Latency Mode SYSMODE_FULLPROC and set EQ bank\n"));
            }
            break;

        default:
            {
                PRINT(("DECODER: Set Low Latency Mode Invalid [%x]\n" , music_processing ));
            }
            break;
    }
}

AUDIO_OUTPUT_TYPE_T CsrA2dpDecoderPluginIfLowLatencyGetOutputType(void)
{
    if(isCodecLowLatencyBackChannel())
    {
        return AudioOutputGetOutputType(audio_output_primary_left);
    }
    return OUTPUT_INTERFACE_TYPE_NONE;
}
