/****************************************************************************
Copyright (c) 2005 - 2016 Qualcomm Technologies International, Ltd.

FILE NAME
    csr_cvc_common.c

DESCRIPTION
NOTES
*/

#ifndef SCO_DSP_BOUNDARIES
#define SCO_DSP_BOUNDARIES
#endif

#include <audio.h>
#include <gain_utils.h>
#include <stdlib.h>
#include <panic.h>
#include <stream.h>
#include <sink.h>
#include <print.h>
#include <file.h>
#include <stream.h>     /*for the ringtone_note*/
#include <connection.h> /*for the link_type */
#include <string.h>
#include <source.h>
#include <transform.h>
#include <app/vm/vm_if.h>
#include <ps.h>
#include <vmal.h>

#include "audio_plugin_if.h"        /*for the audio_mode*/
#include "audio_plugin_common.h"
#include "csr_cvc_common_ctx.h"
#include "csr_cvc_common_plugin.h"
#include "csr_cvc_common.h"
#include "csr_cvc_common_dsp_if.h"
#include "csr_cvc_common_io_if.h"
#include "csr_cvc_common_state_machine.h"
#include "audio_output.h"
#include <csr_i2s_audio_plugin.h>

#ifdef ANC
#include "anc.h"
#endif /* ANC */

/* Macro to check if link type is USB */
#define CVC_IS_LINK_TYPE_USB()     (CVC->link_type == AUDIO_SINK_USB)

/* The CVC task instance pointer*/
static CVC_t * CVC = NULL;

/*******************************************************************************
DESCRIPTION
    Connect plug-in for production test of hardware. This function handles link
    configuration and kicks off connection of (e)SCO
*/
static void pluginConnectForProductionTest(CvcPluginTaskdata * task)
{
    CVC->incoming_rate = 8000;

    if(CVC->no_dsp)
    {
        PRINT(("CVC: connect No DSP\n"));

        /* Disable MetaData */
        SourceConfigure(CVC->audio_source,VM_SOURCE_SCO_METADATA_ENABLE,0);

        csrCvcCommonConnectAudio(task) ;

        SetAudioBusy( NULL );
        csrCvcCommonStateMachineHandleEvent(cvc_event_no_dsp_setup, NULL);
        return;
    }

    CVC->processing_mode = cvc_passthrough;

    /* Enable MetaData - not supported for USB*/
    if((CVC->link_type != AUDIO_SINK_USB)&&(CVC->audio_source))
    {
        SourceConfigure(CVC->audio_source,VM_SOURCE_SCO_METADATA_ENABLE,1);
    }

    csrCvcCommonDspLoadDsp(task);
    csrCvcCommonStateMachineHandleEvent(cvc_event_dsp_loaded, NULL);
}

/*******************************************************************************
DESCRIPTION
    Connect CVC for production test of hardware. Depending on configuration this
    will either connect (e)SCO directly to hardware OR will connect CVC in pass-
    through mode.
*/
static void connectAudioForProductionTest(const CvcPluginTaskdata * const task)
{
   if(CVC->audio_sink)
    {
        /* Use plugin default encoder or overwrite if link type is USB */
        link_encoding_t encoding = CVC_IS_LINK_TYPE_USB() ?
                                   link_encoding_usb_pcm : task->encoder;

        /* update the current audio state */
        SetAudioInUse(TRUE);

        CsrCvcIoConnectMicNoCvc();
        CsrCvcIoConnectOutputNoCvc();

        if(CVC->no_dsp)
        {
            /* DSP not required so no need to continue */
            return;
        }

        /* Flag DSP is up and running */
        CVC->cvc_running = TRUE ;

        csrCvcCommonDspConfigureTransport(encoding, task->adc_dac_16kHz ? 16 : 8);

        /* check whether SCO is present */
        if(CVC->audio_sink)
        {
            /* connect sco in/out to dsp ports */
            csrCvcCommonDspConnectScoSource(CVC->audio_source);
            csrCvcCommonDspConnectScoSink(CVC->audio_sink);
        }

        /* Set passthrough mode */
        csrCvcCommonDspConfigureProcessing(cvc_mode_pass_through, cvc_call_active);
    }
}

/*******************************************************************************
DESCRIPTION
    Get the correct audio sink from the AUDIO_PLUGIN_CONNECT_MSG_T. This may 
    either be the USB sink or the SCO sink, depending on connection type.
*/
static Sink getAudioSinkFromConnectMessage(AUDIO_PLUGIN_CONNECT_MSG_T* connect_msg)
{
    if(connect_msg->sink_type == AUDIO_SINK_USB)
    {
        hfp_common_plugin_params_t* params = connect_msg->params;
        return params->usb_params.usb_sink;
    }
    
    return connect_msg->audio_sink;
}

/******************************************************************************/
void CsrCvcPluginConnect( CvcPluginTaskdata *task,
                          AUDIO_PLUGIN_CONNECT_MSG_T* connect_msg)
{
    hfp_common_plugin_params_t* params = (hfp_common_plugin_params_t*)connect_msg->params;

    AudioSetActivePlugin((Task)task);
    /*signal that the audio is busy until the kalimba / parameters are fully loaded so that no tone messages etc will arrive*/
    SetAudioBusy((TaskData*) task);

    PanicNotNull(CVC);
    CVC = PanicUnlessNew(CVC_t);
    PanicNull(params);

    /* Ensure DSP is powered on. Failure to do this may cause failure of 
       StreamSourceFromSink on some platforms. */
    csrCvcCommonDspPowerOn();
    
    /* The DAC gain must be limited to 0 dB so that no distortion occurs and so the echo canceller works. */
    if (connect_msg->volume > 0xf)
        connect_msg->volume = 0xf;

    CVC->app_task        = (TaskData *) connect_msg->app_task;
    CVC->cvc_running     = FALSE;
    CVC->digital         = params->digital;
    CVC->link_type       = connect_msg->sink_type;
    CVC->volume          = BITFIELD_CAST(8, connect_msg->volume);
    CVC->audio_sink      = getAudioSinkFromConnectMessage(connect_msg);
    CVC->audio_source    = StreamSourceFromSink(connect_msg->audio_sink);
    CVC->mode            = connect_msg->mode;
    CVC->processing_mode = cvc_hands_free_kit;
    CVC->tone_volume     = BITFIELD_CAST(8, connect_msg->volume);
    CVC->audio_rate      = connect_msg->rate;
    CVC->features        = connect_msg->features;
    CVC->mic_for_no_cvc  = microphone_input_id_voice_a;
    CVC->output_for_no_cvc = audio_output_primary_left;
    CVC->mic_muted       = FALSE;
    CVC->speaker_muted   = FALSE;
    /* Check if this is the no dsp plugin or should be started in low power mode */
    CVC->production_testing = CVC_PLUGIN_IS_CVC_DISABLED(task);
    CVC->no_dsp = FALSE;

    if(CVC->production_testing)
    {
        if(csrCvcCommonDspSupportsNoDspMode(task))
            CVC->no_dsp = TRUE;
        pluginConnectForProductionTest(task);
        return;
    }

    /* Set clear mic pin */
    AudioPluginSetMicPio(CVC->digital->mic_a, TRUE);
    if(task->two_mic)
        AudioPluginSetMicPio(CVC->digital->mic_b, TRUE);

    /* Don't drop to no DSP if meta data cannot be disabled */
    if(connect_msg->power <= LPIBM_THRESHOLD)
    {
        if(csrCvcCommonDspSupportsNoDspMode(task))
            CVC->no_dsp = TRUE;
        else
            CVC->processing_mode = cvc_passthrough;
    }

    PRINT(("CVC: connect [%x] [%x] [%x]\n", CVC->cvc_running, (int)CVC->audio_sink, (int)CVC->audio_source));

    /* Calculate the DAC rate based on the over-the-air rate value passed in from VM */
    if(CVC->no_dsp)
    {     /* force DAC rate to 8k if in low power mode and not WBS */
        CVC->incoming_rate = 8000;
    }
    else
    {
        /* Set ADC & DAC to 16 or 8 kHz  */
        CVC->incoming_rate = (task->adc_dac_16kHz)?16000:8000;
    }

    /* Fow WBS set SBC Frame size, else sample-based */
    if(task->encoder == link_encoding_msbc)
    {
        SinkConfigure(CVC->audio_sink,VM_SINK_SCO_SET_FRAME_LENGTH,60);
    }

    /* If in no DSP mode then just connect the ports, if this is cVc then continue and load the DSP */
    if(CVC->no_dsp)
    {
        PRINT(("CVC: connect No DSP\n"));

        /* Disable MetaData */
        SourceConfigure(CVC->audio_source,VM_SOURCE_SCO_METADATA_ENABLE,0);

        csrCvcCommonConnectAudio (task) ;

        CsrCvcPluginSetVolume(CVC->volume);
        SetAudioBusy( NULL );
        csrCvcCommonStateMachineHandleEvent(cvc_event_no_dsp_setup, NULL);
        return;
    }

    /* Enable MetaData - not supported for USB*/
    if((CVC->link_type != AUDIO_SINK_USB)&&(CVC->audio_source))
    {
        SourceConfigure(CVC->audio_source,VM_SOURCE_SCO_METADATA_ENABLE,1);
    }

    csrCvcCommonDspLoadDsp(task);
    csrCvcCommonStateMachineHandleEvent(cvc_event_dsp_loaded, NULL);
}

/******************************************************************************/
void CsrCvcPluginDisconnect( CvcPluginTaskdata *task )
{
    PanicNull(CVC);

    if(CVC->no_dsp)
    {
        PRINT(("CVC: NO DSP: disconnect\n"));
        /* Disconnect speakers */
        CsrCvcIoDisconnectOutputNoCvc();
        /* disconnect the microphones */
        CsrCvcIoDisconnectMicNoCvc();
    }
    else
    {
        Source mic_source_a = NULL;
        Source mic_source_b = NULL;

        csrCvcCommonDspStop();

        /* check cvc running */
        PanicFalse(CVC->cvc_running);

        /* Disconnect speakers */
        csrCvcCommonDspDisconnectSpeakers();

#ifdef ANC
        /* Ensure ANC microphones are disconnected */
        AncDisconnectMics();
#endif /* ANC */

        PRINT(("CVC: Destroy transforms\n"));

        /* Disconnect and close the left mic channel */
        mic_source_a = AudioPluginGetMic(AudioPluginGetInstance(CVC->digital->mic_a),
                                         AUDIO_CHANNEL_A,
                                         CVC->digital->mic_a.digital);

        StreamDisconnect(mic_source_a, NULL);
        SourceClose(mic_source_a);

        /* for the two mic variant disconnect the second microphone */
        if( task->two_mic )
        {
            mic_source_b = AudioPluginGetMic(AudioPluginGetInstance(CVC->digital->mic_b),
                                             AUDIO_CHANNEL_B,
                                             CVC->digital->mic_b.digital);

            StreamDisconnect(mic_source_b, NULL);
            SourceClose(mic_source_b);
        }

        /* disconnect the in and out sco ports */
        StreamDisconnect(CVC->audio_source, NULL); /* SCO->DSP */
        StreamDisconnect(NULL, CVC->audio_sink ); /* DSP->SCO/USB */
        
        PRINT(("CVC: Disconnected\n"));
    }

    CVC->cvc_running = FALSE;
    CVC->audio_sink = NULL;
    CVC->link_type = 0;

    /* Cancel any firmware messages */
    MessageCancelAll((Task)task, MESSAGE_STREAM_DISCONNECT);

    /* Cancel any messages to the application task */
    MessageCancelAll( CVC->app_task, CSR_SR_WORD_RESP_UNKNOWN);
    MessageCancelAll( CVC->app_task, CSR_SR_WORD_RESP_YES);
    MessageCancelAll( CVC->app_task, CSR_SR_WORD_RESP_NO);
    MessageCancelAll( CVC->app_task, CSR_SR_WORD_RESP_FAILED_YES);
    MessageCancelAll( CVC->app_task, CSR_SR_WORD_RESP_FAILED_NO);

    /* Turn off Digital Mic PIO */
    AudioPluginSetMicPio(CVC->digital->mic_a, FALSE);
    if(task->two_mic)
        AudioPluginSetMicPio(CVC->digital->mic_b, FALSE);

    /* Wait until micbias has been turned off before powering down DSP */
    if(!CVC->no_dsp)
        csrCvcCommonDspPowerOff((Task)task);
    
    free (CVC);
    CVC = NULL;


    /* update current dsp status */
    csrCvcCommonStateMachineHandleEvent(cvc_disconnected, NULL);
    /* update the current audio state */
    SetAudioInUse(FALSE);
    SetAsrPlaying(FALSE);
    AudioSetActivePlugin(NULL);
    AudioSetRelayPlugin(NULL);
}

/******************************************************************************/
void CsrCvcPluginSetVolume(int16 volume )
{
    PanicNull(CVC);

    /* The DAC gain must be limited to 0 dB so that no distortion occurs and so the echo canceller works. */
    if (volume > 0xf)
        volume = 0xf;

    CVC->volume = BITFIELD_CAST(8, volume);

    if(CVC->no_dsp)
    {
        int16 master_vol = VolumeConvertDACGainToDB(CVC->volume);
        PRINT(("CVC: NO DSP: Set volume %d (%d dB/60)\n", volume, master_vol));
        /*Set the output Gain immediately*/
        AudioOutputGainSetHardwareOnly(audio_output_group_main, master_vol);
        if(CVC->production_testing)
        {
            AudioOutputGainSetHardwareOnly(audio_output_group_aux, master_vol);
        }
        return;
    }

    PRINT(("CVC: DAC GAIN SET[%x]\n", CVC->volume ));

    /* Only update the volume if not in a mute mode */
    if ( CVC->cvc_running && !(CVC->speaker_muted) )
    {
        csrCvcCommonDspConfigureVolume(CVC->volume);
    }
}

/******************************************************************************/
void CsrCvcPluginResetVolume( void )
{
    /* Only update the volume if not in a mute mode */
    if ( CVC && CVC->cvc_running && !(CVC->speaker_muted) )
    {
        csrCvcCommonDspConfigureVolume(CVC->volume);
    }
}

/******************************************************************************/
void CsrCvcPluginSetModeNoDsp ( AUDIO_MODE_T mode )
{
    PRINT(("CsrCvcPluginSetModeNoDsp mode = %d\n",mode));

    switch (mode)
    {
        case AUDIO_MODE_CONNECTED :
        {
            PRINT(("CVC: NODSP: Set Mode CONNECTED\n"));
            CsrCvcIoConnectMicNoCvc();
            CsrCvcIoConnectOutputNoCvc();
        }
        break ;
        case AUDIO_MODE_STANDBY:
        {
            PRINT(("CVC: NODSP: Set Mode STANDBY\n"));
        }
        break ;
        case AUDIO_MODE_MUTE_MIC:
        case AUDIO_MODE_MUTE_SPEAKER:
        case AUDIO_MODE_MUTE_BOTH:
        case AUDIO_MODE_UNMUTE_SPEAKER:
        {
            PRINT(("CVC: NODSP: *** Muting via SET_MODE_MSG is deprecated ***\n"));
            PRINT(("CVC: NODSP: Use SET_SOFT_MUTE_MSG instead\n"));
            Panic();
        }
        case AUDIO_MODE_LEFT_PASSTHRU:
        case AUDIO_MODE_RIGHT_PASSTHRU:
        case AUDIO_MODE_LOW_VOLUME:
        default :
        {
            PRINT(("CVC: NODSP: Set Mode Invalid [0x%x]\n", mode )) ;
        }
        break ;
    }
}

/******************************************************************************/
void CsrCvcPluginSetSoftMuteNoDsp(AUDIO_PLUGIN_SET_SOFT_MUTE_MSG_T* message)
{
    bool mute_mic = FALSE;
    bool mute_speaker = FALSE;

    if (message->mute_states & AUDIO_MUTE_MASK(audio_mute_group_main))
        mute_speaker = TRUE;

    if (message->mute_states & AUDIO_MUTE_MASK(audio_mute_group_mic))
        mute_mic = TRUE;

    if (CVC->speaker_muted != mute_speaker)
    {
        if (mute_speaker)
            CsrCvcIoDisconnectOutputNoCvc();
        else
            CsrCvcIoConnectOutputNoCvc();

        CVC->speaker_muted = BITFIELD_CAST(1, mute_speaker);
    }

    if (CVC->mic_muted != mute_mic)
    {
        if (mute_mic)
            CsrCvcIoDisconnectMicNoCvc();
        else
            CsrCvcIoConnectMicNoCvc();

        CVC->mic_muted = BITFIELD_CAST(1, mute_mic);
    }
}

/******************************************************************************/
void CsrCvcPluginSetMode ( CvcPluginTaskdata *task, AUDIO_MODE_T mode, const void * params )
{
    /* pre-initialise with the most common parameters and adjust below as necessary */
    cvc_mode_t sysmode;
    cvc_call_state_t call_state = cvc_call_active;
    int16 volume;
    
    PRINT(("CsrCvcPluginSetMode mode = %d\n",mode));

    /* ensure CVC is valid and this is not a stale message left in the queue at disconnect */
    if(CVC)
    {
        if (params)
        {
            /* When using aptx dsp app with wbs back channel check for external mic support */
            CVCPluginModeParams * mode_params = (CVCPluginModeParams *)params;
            bool mic_attached = (mode_params->external_mic_settings == EXTERNAL_MIC_FITTED);
            csrCvcCommonDspSendExternalMicrophoneUpdate(task, mic_attached);
        }

        /* Store current mode */
        CVC->mode = mode;

        /* check if in no dsp mode */
        if(CVC->no_dsp)
        {
            CsrCvcPluginSetModeNoDsp(mode) ;
            return;
        }

        PanicFalse(CVC->cvc_running);

        volume  = CVC->volume;
        sysmode = (CVC->processing_mode == cvc_hands_free_kit) ? cvc_mode_hands_free_kit : cvc_mode_pass_through;

        /* if using ASR feature, start it running */
        if(CVC_PLUGIN_IS_ASR(task))
        {
            PRINT(("CVC: SETMODE ASR: vol=%d\n", volume));

            /*send update to kalimba */
            csrCvcCommonDspConfigureProcessing(cvc_mode_speech_recognition, cvc_call_active);
            csrCvcCommonDspConfigureVolume(volume);
        }
        /* for all modes excluding ASR mode */
        else
        {
            switch (CVC->mode)
            {
                case AUDIO_MODE_CONNECTED:
                {

                }
                break ;
                case AUDIO_MODE_STANDBY:
                {
                    sysmode = cvc_mode_stand_by;
                    call_state = cvc_call_none;
                    volume = DAC_MUTE;
                }
                break;
                case AUDIO_MODE_MUTE_MIC:
                case AUDIO_MODE_MUTE_SPEAKER:
                case AUDIO_MODE_MUTE_BOTH:
                case AUDIO_MODE_UNMUTE_SPEAKER:
                {
                    PRINT(("CVC: *** Muting via SET_MODE_MSG is deprecated ***\n"));
                    PRINT(("CVC: Use SET_SOFT_MUTE_MSG instead\n"));
                    Panic();
                }
                break;
                default:
                {
                /*do not send a message*/
                    PRINT(("CVC: Set Mode Invalid [0x%x]\n", mode ));
                    return;
                }
            }

            /*send update to kalimba */
            PRINT(("CVC: SYSMODE 0x%x, CALLSTATE 0x%x, VOL 0x%x\n", sysmode, call_state, volume));
            csrCvcCommonDspConfigureProcessing(sysmode, call_state);
            csrCvcCommonDspConfigureVolume(volume);
        }
    }
}

/******************************************************************************/
void CsrCvcPluginSetSoftMute(AUDIO_PLUGIN_SET_SOFT_MUTE_MSG_T* message)
{
    if (!CVC)
        return;

    /* Check if in no dsp mode */
    if(CVC->no_dsp)
    {
        CsrCvcPluginSetSoftMuteNoDsp(message);
    }
    else
    {
        bool mute_mic = FALSE;
        bool mute_speaker = FALSE;

        PanicFalse(CVC->cvc_running);

        if (message->mute_states & AUDIO_MUTE_MASK(audio_mute_group_main))
            mute_speaker = TRUE;

        if (message->mute_states & AUDIO_MUTE_MASK(audio_mute_group_mic))
            mute_mic = TRUE;

        /* Notify DSP if mute mode needs updating */
        csrCvcCommonDspConfigureMute(mute_mic, mute_speaker);
        
        /* Update CVC state(s). This must always be after mute is configured */
        CVC->mic_muted = BITFIELD_CAST(1, mute_mic);
        CVC->speaker_muted = BITFIELD_CAST(1, mute_speaker);
    }
}

/*******************************************************************************
DESCRIPTION
    Get the resampling rate required for voice audio if using I2S output
*/
static uint32 getVoiceOutputSampleRate(void)
{
    if(AudioOutputConfigRequiresI2s())
    {
        uint32 output_rate = CsrI2SVoiceResamplingFrequency();
        if(output_rate != I2S_NO_RESAMPLE)
        {
            return output_rate;
        }
    }
    return CVC->incoming_rate;
}

/*******************************************************************************
DESCRIPTION
    Connect a microphone when configured to use internal routing. This means
    either ADC or digital microphones.
*/
static void connectMicrophonesRouteInternal(bool two_mic, uint32 adc_rate)
{
    const common_mic_params* mics = CVC->digital;
    Source mic_source_b = NULL;
    Source mic_source_a = AudioPluginGetMic(AudioPluginGetInstance(mics->mic_a),
                                            AUDIO_CHANNEL_A,
                                            mics->mic_a.digital);

    /* match input and output rates when using resampling */
    AudioPluginSetMicRate(mic_source_a, mics->mic_a.digital, adc_rate);

    if(two_mic)
    {
        /* Configure mic B */
        mic_source_b = AudioPluginGetMic(AudioPluginGetInstance(mics->mic_b),
                                         AUDIO_CHANNEL_B,
                                         mics->mic_b.digital);

        /* match input and output rates when using resampling */
        AudioPluginSetMicRate(mic_source_b, mics->mic_b.digital, adc_rate);

        /* synchronise both mics A and B */
        SourceSynchronise(mic_source_a, mic_source_b);
    }

    csrCvcCommonDspConnectMicrophones(mic_source_a, mic_source_b);
}

/*******************************************************************************
DESCRIPTION
    Connect a microphone when configured to use I2S routing. This means
    external I2S ADC(s)
*/
static void connectMicrophonesRouteI2s(bool two_mic)
{
    uint32 i2s_rate = CsrI2SVoiceResamplingFrequency();
    csrCvcCommonDspConnectI2sMicrophones(two_mic, i2s_rate);
}

/*******************************************************************************
DESCRIPTION
    Determine microphone type required
*/
static void connectMicrophones(CvcPluginTaskdata *task, unsigned adc_rate)
{
    if(!CVC->no_dsp)
    {
        PRINT(("CVC: Input "));
        switch(CVC->features.audio_input_routing)
        {
            case AUDIO_ROUTE_INTERNAL:
            {
                PRINT(("ADC\n"));
                connectMicrophonesRouteInternal(task->two_mic, adc_rate);
                break;
            }
            case AUDIO_ROUTE_I2S:
            {
                PRINT(("I2S\n"));
                connectMicrophonesRouteI2s(task->two_mic);
                break;
            }
            case AUDIO_ROUTE_SPDIF:
            case AUDIO_ROUTE_INTERNAL_AND_RELAY:
            default:
            {
                PRINT(("Unsupported\n"));
                Panic();
                break;
            }
        }
    }
}

/******************************************************************************/
void csrCvcCommonConnectAudio (CvcPluginTaskdata *task)
{
    if(CVC->production_testing)
    {
        connectAudioForProductionTest(task);
    }
    else if ((CVC->audio_sink != (Sink)NULL) || CVC_PLUGIN_IS_ASR(task))
    {
        uint32 adc_rate;
        audio_output_params_t params;

        /* Default to hardware rate matching for DAC outputs */
        rate_matching_t rate_matching = rate_matching_hardware;
        hardware_type_t hardware_type = hardware_type_dac;

        /* Use plugin default encoder or overwrite if link type is USB */
        link_encoding_t encoding = CVC_IS_LINK_TYPE_USB() ?
                                   link_encoding_usb_pcm : task->encoder;

        /* Set up params, initially to work out ADC rate */
        CsrCvcIoSetupMicParams(&params, FALSE);

        /* bandwidth extension variants require different adc rate to output sample rate */
        if(CVC_PLUGIN_IS_2MIC_BEX(task))
            params.sample_rate = 8000;

        adc_rate = AudioOutputGetSampleRate(&params);

        /* update the current audio state */
        SetAudioInUse(TRUE);

    #ifdef ANC
        /* ANC requires digital volume control and therefore the DSP to be running */
        PanicFalse(!CVC->no_dsp);

        /* Attempt to connect ANC microphones */
        AncConnectMics(csrCvcCommonDspGetSink(dsp_sink_anc_mic_a), 
                       csrCvcCommonDspGetSink(dsp_sink_anc_mic_b));
    #else /* ANC */

        /* Handle no DSP as special case */
        if(CVC->audio_sink && CVC->no_dsp)
        {
            /* Call set mode to connect DSP */
            CsrCvcPluginSetModeNoDsp(CVC->mode);

            /* Set hardware gain */
            AudioOutputGainSetHardwareOnly(audio_output_group_main,
                                            VolumeConvertDACGainToDB(CVC->volume));
            return;
        }
    #endif /* ANC */

        /* Now set the DAC rate */
        params.sample_rate = getVoiceOutputSampleRate();

        /* Connect DSP - note sample rate may be updated if re-sampling
           is required for outputs used */
        csrCvcCommonDspConnectSpeakers(&params);

        /* Flag DSP is up and running */
        CVC->cvc_running = TRUE ;

        csrCvcCommonDspConfigureTransport(encoding, task->adc_dac_16kHz ? 16 : 8);

        /* check whether SCO is present */
        if(CVC->audio_sink)
        {
            /* connect sco in/out to dsp ports */
            csrCvcCommonDspConnectScoSource(CVC->audio_source); /* SCO->DSP */
            connectMicrophones(task, adc_rate);
            csrCvcCommonDspConnectScoSink(CVC->audio_sink);
        }

#ifdef ANC
        if (AncGetDacSampleRate() != 0)
        {
            PRINT(("CVC: ANC in use\n"));
            params.sample_rate = AncGetDacSampleRate();

            /* Always use use software rate matching when ANC in use */
            rate_matching = rate_matching_software;
        }
#endif /* ANC */

        /* Always use use software rate matching when I2S in use */
        if(AudioOutputI2sActive())
        {
            rate_matching = rate_matching_software;
            hardware_type = hardware_type_i2s;
        }

        /* The DSP must know the sampling rate for the ADC and DAC */
        csrCvcCommonDspConfigureHardware(params.sample_rate, adc_rate,
                                         rate_matching, hardware_type);

        /* Set the mode */
        CsrCvcPluginSetMode ( task, CVC->mode, NULL );

        CsrCvcPluginStartSpeechRecognitionIfSupported(task);
    }
    else
    {
        /* We should never get here, state machine should prevent it */
        Panic();
    }
}

/******************************************************************************/
void csrCvcCodecMessage (CvcPluginTaskdata *task, T_mic_gain input_gain_l, T_mic_gain input_gain_r, int16 output_gain )
{
    PRINT(("CVC: Output gain = 0x%x\n", output_gain ));
    PRINT(("CVC: Input gain L:R = 0x%x : 0x%x \n", *(uint16*)&input_gain_l,*(uint16*)&input_gain_r));

    /* check pointer validity as there is a very small window where a message arrives
       as the result of playing a tone after CVC has been powered down */
    if(CVC)
    {
        /* Set output volume level */
        AudioOutputGainSetHardwareOnly(audio_output_group_main, VolumeConvertDACGainToDB(output_gain));
        if(CVC->production_testing)
        {
            AudioOutputGainSetHardwareOnly(audio_output_group_aux, VolumeConvertDACGainToDB(output_gain));
        }

        /* Set input gain(s) */
        CsrCvcIoSetMicGain(AudioPluginGetInstance(CVC->digital->mic_a),
                      AUDIO_CHANNEL_A, CVC->digital->mic_a.digital, input_gain_l);
        if( task->two_mic )
            CsrCvcIoSetMicGain(AudioPluginGetInstance(CVC->digital->mic_b),
                          AUDIO_CHANNEL_B, CVC->digital->mic_b.digital, input_gain_r);
    }
}

/******************************************************************************/
bandwidth_t csrCvcGetBandwidth(CvcPluginTaskdata *task)
{
    switch(task->cvc_plugin_variant)
    {
        case cvc_1_mic_headset_cvsd_bex:
        case cvc_2_mic_headset_cvsd_bex:
        case cvc_1_mic_handsfree_cvsd_bex:
        case cvc_2_mic_handsfree_cvsd_bex:
            return bandwidth_expanded;

        case cvc_1_mic_headset_cvsd_asr:
        case cvc_2_mic_headset_cvsd_asr:
        case cvc_1_mic_handsfree_cvsd_asr:
        case cvc_2_mic_handsfree_cvsd_asr:
        case cvc_1_mic_headset_cvsd:
        case cvc_2_mic_headset_cvsd:
        case cvc_1_mic_handsfree_cvsd:
        case cvc_2_mic_handsfree_cvsd:
            return bandwidth_narrow;

        case cvc_1_mic_headset_msbc:
        case cvc_2_mic_headset_msbc:
        case cvc_1_mic_handsfree_msbc:
        case cvc_2_mic_handsfree_msbc:
            return bandwidth_wide;

        case cvc_disabled:
        default:
            break;
    }
    Panic();
    return (bandwidth_t)-1;
}

/******************************************************************************/
const audio_mic_params * CsrCvcGetMicParamsFromMicId(const microphone_input_id_t mic_id)
{
    if(mic_id == microphone_input_id_voice_b)
    {
        return &CVC->digital->mic_b;
    }
#ifdef ANC
    else if(mic_id == microphone_input_id_anc_a)
    {
        return AncGetMicAParams();
    }
    else if(mic_id == microphone_input_id_anc_b)
    {
        return AncGetMicBParams();
    }
#endif
    return &CVC->digital->mic_a;
}

/******************************************************************************/
#ifdef CVC_ALL
void CsrCvcPluginMicSwitch(void)
{
    PRINT(("CVC: NODSP: MicSwitch [%x]\n", (int)CVC->audio_sink )) ;
    if ( CVC->audio_sink )
    {
        CsrCvcIoDisconnectMicId(CVC->mic_for_no_cvc);

        CVC->mic_for_no_cvc++;
        if(CVC->mic_for_no_cvc >= microphone_input_id_max)
        {
            CVC->mic_for_no_cvc = microphone_input_id_voice_a;
        }

        CsrCvcIoConnectMicId(CVC->mic_for_no_cvc);
    }
}

/*******************************************************************************
DESCRIPTION
    Get the next speaker output which is enabled in the multi-channel
    configuration
*/
static audio_output_t getNextValidMultiChannelOutput(const audio_output_t current_channel)
{
    audio_output_t next_channel = current_channel;

    do
    {
        if(++next_channel == audio_output_max)
        {
            next_channel = audio_output_primary_left;
        }

        if(AudioOutputGetOutputType(next_channel))
        {
            break;
        }
    } while(current_channel != (const audio_output_t)next_channel);

    return next_channel;
}

/******************************************************************************/
void CsrCvcPluginOutputSwitch(void)
{
    PRINT(("CVC: NODSP: OutputSwitch [%x]\n", (int)CVC->audio_sink )) ;
    if(CVC->audio_sink)
    {
        CsrCvcIoDisconnectOutputNoCvc();

        CVC->output_for_no_cvc = getNextValidMultiChannelOutput(CVC->output_for_no_cvc);

        CsrCvcIoConnectOutputNoCvc();
    }
}

#endif

/******************************************************************************/
void CsrCvcPluginSetPower( CvcPluginTaskdata *task,  AUDIO_POWER_T power)
{

    PRINT(("CVC : Set Power [0x%x]\n", power));

    /* If actually using the NO DSP plugin disregard set power requests */
    if(!CVC || CVC_PLUGIN_IS_CVC_DISABLED(task))
    {
        PRINT(("CVC : Set Power ignored\n"));
        return;
    }

    /*   These are the state transitions possible during an active SCO:

    CVC -> (low power) -> DSP Passthrough
    DSP Passthrough -> (normal power) -> CVC
    No DSP -> (normal power) -> CVC

    It's not possible to switch CVC or DSP Pass through to No DSP with an active SCO due to Metadata issues */
    if(!CVC->no_dsp)
    {
        /* Default to normal power */
        CVC->processing_mode = cvc_hands_free_kit;
        /* Back to low power unless above threshold */
        if(power <= LPIBM_THRESHOLD)
            CVC->processing_mode = cvc_passthrough;

        if (IsAudioBusy())
        {
            /* Update mode once busy condition clear */
            MAKE_AUDIO_MESSAGE ( AUDIO_PLUGIN_SET_MODE_MSG, message) ;
            message->mode   = CVC->mode ;
            message->params = NULL ;
            MessageSendConditionallyOnTask ( (Task)task, AUDIO_PLUGIN_SET_MODE_MSG, message, AudioBusyPtr() ) ;
        }
        else
        {
            /* Update mode now */
            CsrCvcPluginSetMode((CvcPluginTaskdata*)task, CVC->mode, NULL) ;
        }
    }
}

/******************************************************************************/
void CvcConfigureSpeechRecognitionIfSupported(CvcPluginTaskdata *task)
{
    /* Note that configuration is set for headset only */
    if(CVC_PLUGIN_IS_ASR_HEADSET(task))
    {
        PanicFalse(csrCvcCommonDspConfigureSpeechRecognition());
    }
}

/******************************************************************************/
void CsrCvcPluginStartSpeechRecognitionIfSupported(CvcPluginTaskdata *task)
{
    /* if using ASR feature, start it running */
    if(CVC_PLUGIN_IS_ASR(task))
    {
        PRINT(("Start Speech Recognition\n"));
        PanicFalse(csrCvcCommonDspStartSpeechRecognition());
        /* update the current audio state */
        SetAsrPlaying(TRUE);
    }
}

/******************************************************************************/
void CsrCvcPluginReset(void)
{
    if(CVC)
    {
        csrCvcCommonStateMachineReset();
        free(CVC);
        CVC = NULL;
    }
}

/******************************************************************************/
bool CsrCvcIsHandsFreeKitEnabled(void)
{
    return (CVC->processing_mode == cvc_hands_free_kit);
}

/******************************************************************************/
int16 CsrCvcGetVolume(void)
{
    return CVC->volume;
}

/******************************************************************************/
void CsrCvcSetToneVolume(int16 tone_volume)
{
    CVC->tone_volume = BITFIELD_CAST(8, tone_volume);
}

/******************************************************************************/
int16 CsrCvcGetToneVolume(void)
{
    return CVC->tone_volume;
}

/******************************************************************************/
bool CsrCvcIsUsbAudio(void)
{
    return CVC_IS_LINK_TYPE_USB();
}

/******************************************************************************/
void CsrCvcSendApplicationMessage(MessageId id, void* payload)
{
    if(CVC->app_task)
        MessageSend(CVC->app_task, id, payload);
}

/******************************************************************************/
CVC_t *CsrCvcGetCtx(void)
{
    return CVC;
}

CvcPluginTaskdata *CsrCvcGetActivePlugin(void)
{
    return (CvcPluginTaskdata *) AudioGetActivePlugin();
}

/*******************************************************************************
DESCRIPTION
    Handle playing of a tone when plug-in is not using the DSP
*/
static void toneStartNoDsp(CvcPluginTaskdata *task, AUDIO_PLUGIN_PLAY_TONE_MSG_T * tone_message)
{
    Source tone_source = StreamRingtoneSource(tone_message->tone);
    audio_output_params_t params;
    /* Use the tone volume if present */
    int16 sys_vol = tone_message->tone_volume ? tone_message->tone_volume : CVC->volume;

    if ( CVC->audio_sink )
        CsrCvcIoDisconnectOutputNoCvc();

    /* Connect the tone source direct to left primary output */
    CsrCvcIoSetupMicParams(&params, TRUE);
    /* Automatically dispose tone source on early disconnect */
    params.transform = audio_output_tansform_connect_and_dispose;
    PanicFalse(AudioOutputConnectStereoSource(tone_source, NULL, &params));

    VmalMessageSinkTask( AudioOutputGetAudioSink(), (TaskData*) task );

    /* Set gain using multi-channel lib */
    AudioOutputGainSetHardwareOnly(audio_output_group_main,
                                    VolumeConvertDACGainToDB(sys_vol));
}

/*******************************************************************************
DESCRIPTION
    Handle completion of a tone when plug-in is not using the DSP
*/
static void toneCompleteNoDsp(CvcPluginTaskdata *task)
{
    CsrCvcIoDisconnectOutputNoCvc();

    if (!(CVC->speaker_muted))
    {
        /* reconnect sco audio if present */
        CsrCvcIoConnectOutputNoCvc();

        /* check to see if the sco is still valid, if it is not then we will have received the
           message before the tone has completed playing due to some other issue, therefore
           allow tone to continue playing for an additional 1.5 seconds to allow the power off
           tone to be played to completion */
        if(SinkIsValid(CVC->audio_sink))
        {
            SetAudioBusy(NULL) ;
            CsrCvcPluginSetVolume(CVC->volume );
            PRINT(("CVC: Tone Complete SCO exists\n"));
        }
        else
        {
            MessageSendLater((TaskData*) task, MESSAGE_FORCE_TONE_COMPLETE, 0, 1500);
            PRINT(("CVC: Tone Complete SCO not exists [%p]\n", (void*)CVC->audio_sink));
        }
    }
    else
    {
        SetAudioBusy(NULL) ;
        CsrCvcPluginSetVolume(CVC->volume );
    }
}

/******************************************************************************/
void CsrCvcCommonTonePlay (CvcPluginTaskdata *task, AUDIO_PLUGIN_PLAY_TONE_MSG_T * tone_message)
{
    PanicNull(CVC);

    PRINT(("CVC: Tone Start\n"));

    SetAudioBusy((TaskData*) task);
    SetTonePlaying(TRUE);

    if(CVC->no_dsp)
    {
        toneStartNoDsp(task, tone_message);
        return;
    }

    /*check cvc running */
    PanicFalse(CVC->cvc_running);

    /* when mixing tones with ASR, mute the microphone as currently no echo removal */
    if(CVC_PLUGIN_IS_ASR(task))
    {
        PRINT(("CVC: Mute Mic for tone play with ASR\n"));

        CsrCvcIoMuteMic(task->two_mic);
    }

    /* The DAC gain must be limited to 0 dB so that no distortion occurs and so the echo canceller works. */
    if (tone_message->tone_volume > 0xf)
        tone_message->tone_volume = 0xf;

    /* Configure the tone volume */
    if(CsrCvcGetToneVolume() != tone_message->tone_volume)
    {
        CsrCvcSetToneVolume(tone_message->tone_volume);
        csrCvcCommonDspConfigureVolume(CsrCvcGetVolume());
    }

    csrCvcCommonDspRegisterToneNotifications((Task)task);
    csrCvcCommonDspConnectTone(tone_message);
}

/******************************************************************************/
void CsrCvcCommonToneStop ( CvcPluginTaskdata *task )
{
    PRINT(("CVC: Stop Tone\n"));
    PanicNull(CVC);

    if(CVC->no_dsp)
    {
        CsrCvcIoDisconnectOutputNoCvc() ;
        CsrCvcPluginSetModeNoDsp(CVC->mode);
    }
    else
    {
        csrCvcCommonDspDisconnectTone();

        /* when mixing tones with ASR, restore mic volume settins once tone is complete */
        if(CVC_PLUGIN_IS_ASR(task))
        {
            /* restore microphone gain */
            CsrCvcPluginResetVolume();
        }

    }
    /* update current tone playing status */
    SetTonePlaying(FALSE);
}

/******************************************************************************/
void CsrCvcCommonToneComplete( CvcPluginTaskdata *task )
{
    PRINT(("CVC: Tone Complete\n"));
    SetAudioBusy(NULL);

    if(CVC->no_dsp)
    {
        toneCompleteNoDsp(task);
    }
    else
    {
        /* Restore mute if in mute mode */
        if ( CVC->cvc_running && CVC->speaker_muted )
            csrCvcCommonDspConfigureMute(CVC->mic_muted, CVC->speaker_muted);

        /* We no longer want to receive stream indications */
        csrCvcCommonDspRegisterToneNotifications(NULL);
    }
    /* update current tone playing status */
    SetTonePlaying(FALSE);
}

/******************************************************************************/
void csrCvcCommonToneCompleteMessage(CvcPluginTaskdata* task)
{
    /* Make sure tone port is disconnected. Tone source will be
       cleared up as it was connected with StreamConnectAndDispose.
       Do this after CsrCvcPluginToneComplete has cleared the sink
       task so we don't get any MESSAGE_STREAM_DISCONNECTs. */
    CsrCvcCommonToneComplete(task);
    csrCvcCommonDspDisconnectTone();
}

/******************************************************************************/
void CsrCvcCommonToneForceCompleteNoDsp (void)
{
    PRINT(("CVC: Force Tone Complete No DSP\n"));

    if(CVC)
    {
        if(IsAudioBusy())
        {
            SetAudioBusy(NULL);

            CsrCvcIoDisconnectOutputNoCvc();
            CsrCvcIoDisconnectMicNoCvc();
        }
        /* ensure volume is set to correct level after playing tone */
        CsrCvcPluginSetVolume(CVC->volume );
        /* update current tone playing status */
        SetTonePlaying(FALSE);
    }
}
