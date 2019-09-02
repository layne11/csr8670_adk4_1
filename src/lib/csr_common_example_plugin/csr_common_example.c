/****************************************************************************
Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
   csr_cvsd_8k_cvc_1mic_headset.c
    
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
#include <kalimba.h>
#include <file.h>
#include <stream.h>     /*for the ringtone_note*/
#include <connection.h> /*for the link_type */
#include <string.h>
#include <kalimba_standard_messages.h>
#include <kalimba_if.h>
#include <source.h>
#include <app/vm/vm_if.h>
#include <vmal.h>
#include "audio_plugin_if.h"        /*for the audio_mode*/
#include "audio_plugin_common.h"
#include "csr_common_example_if.h"  /*for things common to all CSR_COMMON_EXAMPLE systems*/
#include "csr_common_example_plugin.h"
#include "csr_common_example.h"
#include "audio_output.h"

typedef struct audio_Tag
{
   /*! Whether or not CSR_COMMON_EXAMPLE is running */
   unsigned running:1;
   unsigned reserved:4;
   /*! mono or stereo*/
   unsigned stereo:1;
   /*! The current CSR_COMMON_EXAMPLE mode */
   unsigned mode:8;
   /*! Mute states */
   unsigned mic_muted:1;
   unsigned speaker_muted:1;
   /*! The audio sink being used*/
   Sink audio_sink;
   /*! The link_type being used*/
   sync_link_type link_type;
   /*! The current volume level*/
   uint16 volume;
   /*! The current tone volume level*/
   uint16 tone_volume;
   /*! Over the air rate  */
   uint32  dac_rate;	
   /*! Left microphone gain */
   T_mic_gain mic_gain_left;
   /*! Right microphone gain */
   T_mic_gain mic_gain_right;
   /*! Microphone configuration */
   const common_mic_params* digital;
}EXAMPLE_t ;

/* The task instance pointer*/
static EXAMPLE_t * CSR_COMMON_EXAMPLE = NULL;

static FILE_INDEX selectKapFile(const EXAMPLE_PLUGIN_TYPE_T example_plugin_variant)
{		
    FILE_INDEX index = 0;
    char* kap_file = NULL;
	
    switch(example_plugin_variant)
    {
        case CVSD_8K_1_MIC:
            kap_file = "one_mic_example_cvsd/one_mic_example_cvsd.kap";
            break;

        case CVSD_8K_2_MIC:
            kap_file = "two_mic_example_cvsd/two_mic_example_cvsd.kap";
            break;
    	
        case SBC_1_MIC:
            /* Tell firmware the size of the SBC frame */
            kap_file = "one_mic_example_16k/one_mic_example_16k.kap";
            break;
      
        case SBC_2_MIC:
            /* Tell firmware the size of the SBC frame */
            kap_file = "two_mic_example_16k/two_mic_example_16k.kap";
            break;
      
        default:
            PRINT(("CSR_COMMON_EXAMPLE: No Corresponding Kap file\n")) ;
            Panic() ;
            break;
    }

    index = FileFind(FILE_ROOT,(const char *) kap_file ,strlen(kap_file));
    
    if( index == FILE_NONE )
    {
        PRINT(("CSR_COMMON_EXAMPLE: No File\n"));
        Panic();
    }
   
    return(index);
}

static bool isPluginRunning(void)
{
    if((!CSR_COMMON_EXAMPLE) || (!CSR_COMMON_EXAMPLE->running))
    {
        return FALSE;
    }
    return TRUE;
}

static bool isPluginVariantSbc(const ExamplePluginTaskdata * const task)
{
    if((task->example_plugin_variant == SBC_1_MIC) || (task->example_plugin_variant == SBC_2_MIC))
    {
        return TRUE;
    }
    return FALSE;
}

static uint32 calculateDacRate (const ExamplePluginTaskdata * const task, const uint32 rate)
{
    if(isPluginVariantSbc(task))
    {
        return SBC_DAC_RATE;
    }
    return rate;
}

static void populatePluginFromAudioConnectData(const AUDIO_PLUGIN_CONNECT_MSG_T * const connect_message)
{
    hfp_common_plugin_params_t *params = (hfp_common_plugin_params_t*)connect_message->params;

    CSR_COMMON_EXAMPLE = PanicUnlessNew(EXAMPLE_t);

    CSR_COMMON_EXAMPLE->running         = FALSE;
    CSR_COMMON_EXAMPLE->link_type       = connect_message->sink_type ;
    CSR_COMMON_EXAMPLE->volume          = ((connect_message->volume > 0xf) ? VOLUME_0DB : connect_message->volume);
    CSR_COMMON_EXAMPLE->audio_sink      = connect_message->audio_sink;
    CSR_COMMON_EXAMPLE->mode            = connect_message->mode;
    CSR_COMMON_EXAMPLE->stereo          = connect_message->features.stereo;
    CSR_COMMON_EXAMPLE->tone_volume     = connect_message->volume;
    CSR_COMMON_EXAMPLE->mic_gain_left   = MIC_DEFAULT_GAIN;
    CSR_COMMON_EXAMPLE->mic_gain_right  = MIC_DEFAULT_GAIN;
    CSR_COMMON_EXAMPLE->digital         = (params ? params->digital : NULL);

    PRINT(("CSR_COMMON_EXAMPLE: connect [%x] [%x]\n", CSR_COMMON_EXAMPLE->running , (int)CSR_COMMON_EXAMPLE->audio_sink));
}

static void setMicrophoneInstanceGain(const audio_instance instance, const audio_channel channel, const bool digital, const T_mic_gain gain)
{
    Source mic_source = AudioPluginGetMic(instance, channel, digital);
    uint8 mic_gain = (digital ? gain.digital_gain : gain.analogue_gain);
    AudioPluginSetMicGain(mic_source, digital, mic_gain, gain.preamp_enable);
}

static void updateMicrophoneGains(const ExamplePluginTaskdata * const task)
{
    setMicrophoneInstanceGain(AudioPluginGetInstance(CSR_COMMON_EXAMPLE->digital->mic_a),
                    AUDIO_CHANNEL_A, CSR_COMMON_EXAMPLE->digital->mic_a.digital, CSR_COMMON_EXAMPLE->mic_gain_left);
    if(task->two_mic)
    {
        setMicrophoneInstanceGain(AudioPluginGetInstance(CSR_COMMON_EXAMPLE->digital->mic_b),
                    AUDIO_CHANNEL_B, CSR_COMMON_EXAMPLE->digital->mic_b.digital, CSR_COMMON_EXAMPLE->mic_gain_right);
    }
}

static void setHardwareGain(const uint16 output_gain)
{
    PRINT(("CSR_COMMON_EXAMPLE: Output gain = 0x%x\n" , output_gain));

    /* check pointer validity as there is a very small window where a message arrives
       as the result of playing a tone after CSR_COMMON_EXAMPLE has been powered down */
    if(CSR_COMMON_EXAMPLE)
    {
        AudioOutputGainSetHardwareOnly(audio_output_group_main,
                                                    VolumeConvertDACGainToDB(output_gain));
    }
}

static void sendBluetoothAddressMessage(void)
{
    tp_bdaddr rem_addr;

    SinkGetBdAddr(CSR_COMMON_EXAMPLE->audio_sink, &rem_addr);
    KALIMBA_SEND_MESSAGE(MESSAGE_REM_BT_ADDRESS, rem_addr.taddr.addr.nap, (rem_addr.taddr.addr.lap >> 16) | (((unsigned int)rem_addr.taddr.addr.uap) << 8), rem_addr.taddr.addr.lap & 0xffff, 0 );
}

static void sendVolumeMessage(const uint16 volume)
{
    uint16 digital_dsp = 0;
    if(AudioOutputGainGetType(audio_output_group_main) == audio_output_gain_digital)
    {
        audio_output_gain_t gain;

        /* DSP needs to know the fixed system gain, no need to pass master/tone gain
           so set them to zero and call function to get the fixed system gain. */
        AudioOutputGainGetDigital(audio_output_group_main, 0, 0, &gain);

        digital_dsp = DSP_DIG_VOL_FLAG | (gain.common.system & 0xF);
    }

    KALIMBA_SEND_MESSAGE(MESSAGE_EXAMPLE_PLUGIN_VOLUME, 0, digital_dsp, volume, CSR_COMMON_EXAMPLE->tone_volume);
}

static void sendDelayedVolumeMessage(const ExamplePluginTaskdata * const task, const uint16 volume)
{
    MAKE_AUDIO_MESSAGE (AUDIO_PLUGIN_SET_VOLUME_MSG, message ) ;
    message->volume = volume;
    MessageSendLater((TaskData *)task, AUDIO_PLUGIN_SET_VOLUME_MSG , message, VOLUME_MESSAGE_SEND_DELAY ) ;
}

static void connectMicrophones(const ExamplePluginTaskdata * const task, const uint16 mic_rate)
{
    Source mic_src = AudioPluginGetMic(AudioPluginGetInstance(CSR_COMMON_EXAMPLE->digital->mic_a),
                                AUDIO_CHANNEL_A, CSR_COMMON_EXAMPLE->digital->mic_a.digital);
    AudioPluginSetMicRate(mic_src, CSR_COMMON_EXAMPLE->digital->mic_a.digital, mic_rate);

    if(task->two_mic)
    {
        Source mic_src2 = AudioPluginGetMic(AudioPluginGetInstance(CSR_COMMON_EXAMPLE->digital->mic_b),
                                    AUDIO_CHANNEL_B, CSR_COMMON_EXAMPLE->digital->mic_b.digital);
        AudioPluginSetMicRate(mic_src2, CSR_COMMON_EXAMPLE->digital->mic_b.digital, mic_rate);

        SourceSynchronise(mic_src,mic_src2);

        PanicFalse(StreamConnect(mic_src2, StreamKalimbaSink(DSP_INPUT_PORT_FOR_MIC_B)));
    }

    PanicFalse(StreamConnect(mic_src,StreamKalimbaSink(DSP_INPUT_PORT_FOR_MIC_A)));
}

static void disconnectMicrophones(const ExamplePluginTaskdata * const task)
{
    Source mic_src = AudioPluginGetMic(AudioPluginGetInstance(CSR_COMMON_EXAMPLE->digital->mic_a),
                                            AUDIO_CHANNEL_A, CSR_COMMON_EXAMPLE->digital->mic_a.digital);
    StreamDisconnect(mic_src, StreamKalimbaSink(DSP_INPUT_PORT_FOR_MIC_A));
    SourceClose(mic_src);

    if(task->two_mic)
    {
        mic_src = AudioPluginGetMic(AudioPluginGetInstance(CSR_COMMON_EXAMPLE->digital->mic_b),
                                                AUDIO_CHANNEL_B, CSR_COMMON_EXAMPLE->digital->mic_b.digital);
        StreamDisconnect(mic_src, StreamKalimbaSink(DSP_INPUT_PORT_FOR_MIC_B));
        SourceClose(mic_src);
    }
}

static void setMicrophonePioPins(const ExamplePluginTaskdata * const task, const bool set)
{
    AudioPluginSetMicPio(CSR_COMMON_EXAMPLE->digital->mic_a, set);
    if(task->two_mic)
    {
        AudioPluginSetMicPio(CSR_COMMON_EXAMPLE->digital->mic_b, set);
    }
}

/****************************************************************************
DESCRIPTION
    Connect primary DSP output ports to hardware. Other outputs are not used
    by the example DSP apps.
*/
static void connectKalimbaOutputs(audio_output_params_t* mch_params)
{
    AudioOutputAddSourceOrPanic(StreamKalimbaSource(DSP_OUTPUT_PORT_PRI_LEFT), 
                                audio_output_primary_left);
    AudioOutputAddSourceOrPanic(StreamKalimbaSource(DSP_OUTPUT_PORT_PRI_RIGHT), 
                                audio_output_primary_right);
    AudioOutputConnectOrPanic(mch_params);
}

/****************************************************************************
DESCRIPTION
   Connect the audio stream (Speaker and Microphone)
*/
static void connectAudio(const ExamplePluginTaskdata * const task)
{
    bool r1, r2 =0;

    if(CSR_COMMON_EXAMPLE->audio_sink)
    {
        uint16 adc_rate;
        audio_output_params_t params;
        memset(&params, 0, sizeof(audio_output_params_t));

        /* DSP is up and running */
        CSR_COMMON_EXAMPLE->running = TRUE ;

        /* update the current audio state */
        SetAudioInUse(TRUE);

        params.disable_resample = FALSE;
        params.sample_rate = CSR_COMMON_EXAMPLE->dac_rate;
        adc_rate = AudioOutputGetSampleRate(&params);

        connectMicrophones(task, adc_rate);

        connectKalimbaOutputs(&params);

        r1 = (bool) StreamConnect(StreamSourceFromSink( CSR_COMMON_EXAMPLE->audio_sink ),StreamKalimbaSink(DSP_INPUT_PORT_FOR_AG)); /* SCO->DSP */
        r2 = (bool) StreamConnect( StreamKalimbaSource(DSP_OUTPUT_PORT_FOR_AG), CSR_COMMON_EXAMPLE->audio_sink ); /* DSP->SCO */
        PRINT(("CSR_COMMON_EXAMPLE: connect_sco %d %d \n",r1,r2));

        KALIMBA_SEND_MESSAGE(MESSAGE_SET_DAC_SAMPLE_RATE,
                               params.sample_rate / 10, params.sample_rate/10,
                               0, 0);

        CsrExamplePluginSetMode(CSR_COMMON_EXAMPLE->mode);

        sendDelayedVolumeMessage(task, CSR_COMMON_EXAMPLE->volume);

        updateMicrophoneGains(task);
    }
    else
    {
        CsrExamplePluginDisconnect(task);
    }

    SetCurrentDspStatus(DSP_RUNNING);
}

void CsrExamplePluginConnect(const ExamplePluginTaskdata * const task, const AUDIO_PLUGIN_CONNECT_MSG_T * const connect_msg)
{
    FILE_INDEX index;

    if(CSR_COMMON_EXAMPLE)
    {
        CsrExamplePluginDisconnect(task);
    }

    populatePluginFromAudioConnectData(connect_msg);

    setMicrophonePioPins(task, TRUE);

    CSR_COMMON_EXAMPLE->dac_rate = calculateDacRate(task, connect_msg->rate);

    /* For WBS set SBC Frame size, else sample-based */
    if(isPluginVariantSbc(task))
    {
        SinkConfigure(CSR_COMMON_EXAMPLE->audio_sink, VM_SINK_SCO_SET_FRAME_LENGTH, BYTES_PER_MSBC_FRAME);
    }

    /* Enable MetaData */
    SourceConfigure(StreamSourceFromSink( CSR_COMMON_EXAMPLE->audio_sink ),VM_SOURCE_SCO_METADATA_ENABLE,1);

    /*ensure that the messages received are from the correct kap file*/
    (void) MessageCancelAll( (TaskData*) task, MESSAGE_FROM_KALIMBA);
    MessageKalimbaTask( (TaskData*) task );

    index = selectKapFile((EXAMPLE_PLUGIN_TYPE_T)task->example_plugin_variant);
    
    if( !KalimbaLoad( index ) )
    {
        PRINT(("Kalimba load fail\n"));
        Panic();
    }
    
    SetCurrentDspStatus(DSP_LOADING);
 }


void CsrExamplePluginDisconnect(const ExamplePluginTaskdata * const task)
{
    if(isPluginRunning() == FALSE)
    {
        return;
    }

    /* Disconnect speakers */
    PanicFalse(AudioOutputDisconnect());

    PRINT(("CSR_COMMON_EXAMPLE: Streams disconnected (left)\n"));

    disconnectMicrophones(task);
    
    StreamDisconnect(StreamSourceFromSink( CSR_COMMON_EXAMPLE->audio_sink ),StreamKalimbaSink(DSP_INPUT_PORT_FOR_AG)); /* SCO->DSP */
    StreamDisconnect( StreamKalimbaSource(DSP_OUTPUT_PORT_FOR_AG), CSR_COMMON_EXAMPLE->audio_sink );                    /* DSP->SCO */
    
    CSR_COMMON_EXAMPLE->running = FALSE;
    CSR_COMMON_EXAMPLE->audio_sink = NULL;
    CSR_COMMON_EXAMPLE->link_type = 0;
    
    PRINT(("CSR_COMMON_EXAMPLE: Disconnect\n"));

    /* Turn off DSP and de-register the Kalimba Task */
    KalimbaPowerOff();
    MessageKalimbaTask(NULL);

    /* Cancel any outstanding messages */
    MessageCancelAll( (TaskData*)task , MESSAGE_FROM_KALIMBA);
    MessageCancelAll( (TaskData*)task , MESSAGE_STREAM_DISCONNECT);

    setMicrophonePioPins(task, FALSE);
    
    free(CSR_COMMON_EXAMPLE);
    CSR_COMMON_EXAMPLE = NULL;
    
   
    /* update current dsp status */
    SetCurrentDspStatus( DSP_NOT_LOADED );
    /* update the current audio state */
    SetAudioInUse(FALSE);
}

void CsrExamplePluginSetVolume(const uint16 volume)
{
    PanicNull(CSR_COMMON_EXAMPLE);

    CSR_COMMON_EXAMPLE->volume = ((volume > VOLUME_0DB) ? VOLUME_0DB : volume);
    PRINT(("CSR_COMMON_EXAMPLE: DAC GAIN SET[%x]\n", CSR_COMMON_EXAMPLE->volume ));
    
    /* Only update the volume if not in a mute mode */
    if(CSR_COMMON_EXAMPLE->running && !(CSR_COMMON_EXAMPLE->speaker_muted))
    {
        sendVolumeMessage(CSR_COMMON_EXAMPLE->volume);
    }
}

void CsrExamplePluginSetMode(const AUDIO_MODE_T mode)
{    
    if(isPluginRunning() == FALSE)
    {
        return;
    }
    
    CSR_COMMON_EXAMPLE->mode = mode;
    
    /* Nothing to do, only single mode available for example plugin */

    KALIMBA_SEND_MESSAGE(MESSAGE_SETMODE, SYSMODE_PSTHRGH, 0, 0,
            (CSR_COMMON_EXAMPLE->digital->mic_a.digital << 1) | CSR_COMMON_EXAMPLE->digital->mic_b.digital);
}


void CsrExamplePluginSetSoftMute(const AUDIO_PLUGIN_SET_SOFT_MUTE_MSG_T * const message)
{
    bool mute_mic = FALSE;
    bool mute_speaker = FALSE;

    if(isPluginRunning() == FALSE)
    {
        return;
    }

    if(message->mute_states & AUDIO_MUTE_MASK(audio_mute_group_main))
    {
        mute_speaker = TRUE;
    }

    if(message->mute_states & AUDIO_MUTE_MASK(audio_mute_group_mic))
    {
        mute_mic = TRUE;
    }

    if(CSR_COMMON_EXAMPLE->mic_muted != mute_mic)
    {
        /* mute/unmute mic here */
        CSR_COMMON_EXAMPLE->mic_muted = mute_mic;
    }

    if(CSR_COMMON_EXAMPLE->speaker_muted != mute_speaker)
    {
        /* Speaker mute state changing, need to set volume */
        sendVolumeMessage(mute_speaker? CODEC_MUTE : CSR_COMMON_EXAMPLE->volume);

        CSR_COMMON_EXAMPLE->speaker_muted = mute_speaker;
    }
}


void CsrExamplePluginPlayTone(const ExamplePluginTaskdata * const task,
                                const AUDIO_PLUGIN_PLAY_TONE_MSG_T * const tone_message)
{   
    Sink lSink;
    Source lSource = StreamRingtoneSource(tone_message->tone);

    uint16 volume = ((tone_message->tone_volume > VOLUME_0DB) ?
                                                VOLUME_0DB : tone_message->tone_volume);
        
    if(isPluginRunning() == FALSE)
    {
        return;
    }

    PRINT(("CSR_COMMON_EXAMPLE: Tone Start\n"));
   
    SetTonePlaying(TRUE);

    /* Configure prompt playback, indicate that all firmware tones are mono*/
    KALIMBA_SEND_MESSAGE(MESSAGE_SET_TONE_RATE_MESSAGE_ID, 8000 , 0, 0, 0);

    /* set DAC gain to a suitable level for tone play */
    if (volume != CSR_COMMON_EXAMPLE->tone_volume)
    {
        CSR_COMMON_EXAMPLE->tone_volume = volume;
        sendVolumeMessage(CSR_COMMON_EXAMPLE->tone_volume);
    }

    lSink = StreamKalimbaSink(TONE_VP_MIXING_DSP_PORT);
    
    /*request an indication that the tone has completed / been disconnected*/
    VmalMessageSinkTask (lSink, (TaskData*)task);
 
    /*mix the tone to the CSR_COMMON_EXAMPLE*/
    PanicFalse(StreamConnectAndDispose(lSource, lSink));
}


void CsrExamplePluginStopTone(void)
{
    PanicNull(CSR_COMMON_EXAMPLE);
        
    StreamDisconnect(0, StreamKalimbaSink(TONE_VP_MIXING_DSP_PORT)) ;
   
    SetTonePlaying(FALSE);
}

/****************************************************************************
DESCRIPTION
   handles the internal cvc messages /  messages from the dsp
*/
void CsrExamplePluginInternalMessage(const ExamplePluginTaskdata * const task,
                                            const uint16 id, const Message message)
{
    switch(id)
    {
        case MESSAGE_FROM_KALIMBA:
        {
            const DSP_REGISTER_T *m = (const DSP_REGISTER_T *) message;
            PRINT(("CSR_CVSD_8K_1MIC: msg id[%x] a[%x] b[%x] c[%x] d[%x]\n", m->id, m->a, m->b, m->c, m->d));

            switch( m->id )
            {
                /* Case statements for messages from the kalimba can be added here.
                   The example application as shipped does not send any messages
                   to the DSP. */
                case MESSAGE_EXAMPLE_PLUGIN_DSP_READY:
                    SetCurrentDspStatus(DSP_LOADED_IDLE);
                    sendBluetoothAddressMessage();
                    connectAudio(task);
                    break;
                case MESSAGE_EXAMPLE_PLUGIN_CODEC:
                    setHardwareGain(m->a);
                    SetCurrentDspStatus(DSP_RUNNING);
                    break;
                case MESSAGE_TONE_COMPLETE:
                    SetAudioBusy(NULL);
                    CsrExamplePluginToneComplete(task);
                    break;
                default:
                    break;
            }
        }
        break;
	
        /* Message is not from DSP.  The example plugin as shipped does not
         send any messages to this handler. */

        default:
            break;
    }
}	

/****************************************************************************
DESCRIPTION
   a tone has completed
*/
void CsrExamplePluginToneComplete(const ExamplePluginTaskdata * const task)
{
    sendVolumeMessage((CSR_COMMON_EXAMPLE->running && CSR_COMMON_EXAMPLE->speaker_muted) ?
            CODEC_MUTE : CSR_COMMON_EXAMPLE->volume);
   
    /* We no longer want to receive stream indications */
    VmalMessageSinkTask (StreamKalimbaSink(TONE_VP_MIXING_DSP_PORT) , NULL);
   
    /* update current tone playing status */
    SetTonePlaying(FALSE);
}
