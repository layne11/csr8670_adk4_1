/****************************************************************************
Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    csr_dut_audio.c
    
DESCRIPTION
    Audio plugin for DUT mode

*/

#include <audio.h>
#include <gain_utils.h>
#include <stdlib.h>
#include <panic.h>
#include <stream.h>
#include <print.h>
#include <stream.h> 
#include <message.h>
#include <micbias.h>
#include <string.h>
#include <vmal.h>
                                                                                                                    
#include "audio_plugin_if.h"
#include "audio_plugin_common.h"
#include "csr_dut_audio.h"
#include "csr_dut_audio_plugin.h"
#include "audio_output.h"


typedef struct
{
    /*! The Audio sink in use */
    Sink audio_sink;    
    /*! The current mode */
    unsigned mode:8;
    /*! Unused */
    unsigned unused:8;
    /*! The current audio volume level*/
    uint16 volume;    
    /*! Indicates if stereo or mono */
    bool stereo;
    /*! Indicates rate of audio */
    uint32 rate;
    /*! The Tone sink in use */
    Sink tone_sink;
    /*! The current tone volume level*/
    uint16 tone_volume;
    /*! The current tone stereo flag*/
    uint16 tone_stereo;

    audio_output_t tone_channel;
    /*! The current tone */
    const ringtone_note *tone;
    /*! The plugin parameters */
    uint16 params;
    /*! which microphone to use */
    unsigned dut_microphone:2;
} DUT_AUDIO_T;

static DUT_AUDIO_T *DUT_AUDIO = NULL;

typedef enum
{
    test_channel_primary_left_and_right = ~((0x01 << audio_output_primary_left) | (0x01 << audio_output_primary_right)),
    test_channel_secondary_left_and_right = ~((0x01 << audio_output_secondary_left) | (0x01 << audio_output_secondary_right)),
    test_channel_subwoofer = ~(0x01 << audio_output_wired_sub),
    test_channel_aux_left_and_right = ~((0x01 << audio_output_aux_left) | (0x01 << audio_output_aux_right))
} csr_dut_audio_plugin_test_channel_masks;

typedef enum
{
    CSR_DUT_TEST_CHANNELS_PRIMARY,
    CSR_DUT_TEST_CHANNELS_SECONDARY,
    CSR_DUT_TEST_CHANNELS_SUB,
    CSR_DUT_TEST_CHANNELS_AUX
} csr_dut_audio_plugin_test_channels;

static void CsrDutAudioPluginDisconnectAudio(void)
{
    if (DUT_AUDIO->audio_sink)
    {
        PRINT(("DUT AUDIO: Disconnect Mic/Speaker\n"));
        DUT_AUDIO->audio_sink = 0;

        if (DUT_AUDIO->params)
        {
            common_mic_params *mic_params = (common_mic_params *)DUT_AUDIO->params;

            switch(DUT_AUDIO->dut_microphone)
            {
                case DUT_MIC_B:
                    AudioPluginSetMicPio(mic_params->mic_b, FALSE);
                    break;
                case DUT_MIC_A:
                default:
                    AudioPluginSetMicPio(mic_params->mic_a, FALSE);
                    break;
            }
        }
    }
    AudioOutputDisconnect();
    VmalMessageSinkTask( AudioOutputGetAudioSink() , NULL );
}

static void CsrDutAudioPluginConnectAudio(void)
{
    if(DUT_AUDIO->params)
    {
        Source mic_source;
        audio_output_params_t params;
        common_mic_params *mic_params = (common_mic_params *)DUT_AUDIO->params;

        CsrDutAudioPluginDisconnectAudio();

        switch (DUT_AUDIO->dut_microphone)
        {
            case (DUT_MIC_B):
            {
                mic_source = AudioPluginGetMic(AudioPluginGetInstance(mic_params->mic_b),
                                               AUDIO_CHANNEL_B, mic_params->mic_b.digital);
                AudioPluginSetMicRate(mic_source, mic_params->mic_b.digital, DUT_AUDIO->rate);           
                AudioPluginSetMicPio(mic_params->mic_b, TRUE);
                break;
            }
                    
            case (DUT_MIC_A):
            default:
            {
                mic_source = AudioPluginGetMic(AudioPluginGetInstance(mic_params->mic_a),
                                               AUDIO_CHANNEL_A, mic_params->mic_a.digital);
                AudioPluginSetMicRate(mic_source, mic_params->mic_a.digital, DUT_AUDIO->rate);           
                AudioPluginSetMicPio(mic_params->mic_a, TRUE);
                break;
            }
        }
        
        memset(&params, 0, sizeof(audio_output_params_t));
        params.sample_rate = DUT_AUDIO->rate;
        params.disable_mask = 0;
        PanicFalse(AudioOutputConnectStereoSource(mic_source, NULL, &params));

        CsrDutAudioPluginSetVolume(DUT_AUDIO->volume);
    }    
}

static void CsrDutAudioPluginPopulateFromAudioConnectData(const AUDIO_PLUGIN_CONNECT_MSG_T * const connect_message)
{
    DUT_AUDIO = PanicUnlessNew(DUT_AUDIO_T);

    DUT_AUDIO->volume = connect_message->volume;
    DUT_AUDIO->mode = connect_message->mode;
    DUT_AUDIO->stereo = connect_message->features.stereo;
    DUT_AUDIO->audio_sink = 0;
    DUT_AUDIO->tone_volume = 0;
    DUT_AUDIO->tone_stereo = 0;
    DUT_AUDIO->tone = NULL;
    DUT_AUDIO->tone_channel = audio_output_primary_left;
    DUT_AUDIO->rate = connect_message->rate;
    DUT_AUDIO->params = (uint16) connect_message->params;
    DUT_AUDIO->dut_microphone = connect_message->features.dut_microphone;

    PRINT(("DUT AUDIO: CsrDutAudioPluginConnect\n"));
}


void CsrDutAudioPluginConnect(const AUDIO_PLUGIN_CONNECT_MSG_T * const connect_message)
{
    if(DUT_AUDIO)
    {
        /* check incase AudioDisconnect wasn't called */
        CsrDutAudioPluginDisconnect();
    }

    CsrDutAudioPluginPopulateFromAudioConnectData(connect_message);
    
    CsrDutAudioPluginSetMode(connect_message->mode);
}


void CsrDutAudioPluginDisconnect(void)
{
    PanicNull(DUT_AUDIO);
    
    PRINT(("DUT AUDIO: CsrDutAudioPluginDisconnect\n")); 
    
    CsrDutAudioPluginSetMode(AUDIO_MODE_STANDBY);

    CsrDutAudioPluginStopTone();
    
    free(DUT_AUDIO);
    DUT_AUDIO = NULL;
}


void CsrDutAudioPluginSetMode(const AUDIO_MODE_T mode)
{
    PanicNull(DUT_AUDIO);

    DUT_AUDIO->mode = mode;
    
    PRINT(("DUT AUDIO: CsrDutAudioPluginSetMode [%d]\n", mode)); 

    if(mode == AUDIO_MODE_CONNECTED)
    {
        CsrDutAudioPluginConnectAudio();
    }
    else
    {
        CsrDutAudioPluginDisconnectAudio();
    }
}


void CsrDutAudioPluginSetVolume(const uint16 volume)
{    
    PanicNull(DUT_AUDIO);

    AudioOutputGainSetHardwareOnly(audio_output_group_main,
                                VolumeConvertDACGainToDB(volume));

    AudioOutputGainSetHardwareOnly(audio_output_group_aux,
                                    VolumeConvertDACGainToDB(volume));
}

static void CsrDutAudioPluginIncrementToneOutputChannel(void)
{
    audio_output_t current_channel = DUT_AUDIO->tone_channel;
    audio_output_t next_channel = current_channel;

    do
    {
        if(++next_channel == audio_output_max)
        {
            next_channel = audio_output_primary_left;
        }

        if(AudioOutputGetOutputType(next_channel) != OUTPUT_INTERFACE_TYPE_NONE)
        {
            DUT_AUDIO->tone_channel = next_channel;
            break;
        }
    } while(current_channel != next_channel);
}

#define ALL_CHANNELS_DISABLED   0xFFFF

static unsigned CsrDutAudioPluginGetEnableMaskForChannel(const audio_output_t channel)
{
    unsigned mask = ALL_CHANNELS_DISABLED;
    AudioOutputSetEnabled(mask, channel);

    return mask;
}

/****************************************************************************
DESCRIPTION
    plays a tone using the audio plugin    
*/
void CsrDutAudioPluginPlayTone(const AUDIO_PLUGIN_PLAY_TONE_MSG_T * const tone_message)
{    
    audio_output_params_t params;
    Sink speaker_sink;
    
    Source source = StreamRingtoneSource(tone_message->tone);

    if (!DUT_AUDIO || DUT_AUDIO->audio_sink)
    {
        /* tone audio should not be connected */
        SetAudioBusy(NULL);
        return;
    }
    
    CsrDutAudioPluginDisconnectAudio();


    DUT_AUDIO->tone_volume = tone_message->tone_volume;
    DUT_AUDIO->tone_stereo = tone_message->features.stereo;
    DUT_AUDIO->tone = tone_message->tone;

    memset(&params, 0, sizeof(audio_output_params_t));
    params.transform = audio_output_tansform_connect_and_dispose;
    params.sample_rate = 48000;
    params.disable_resample = FALSE;
    params.disable_mask = CsrDutAudioPluginGetEnableMaskForChannel(DUT_AUDIO->tone_channel);

    PanicFalse(AudioOutputConnectSource(source, DUT_AUDIO->tone_channel, &params));
   
    speaker_sink = AudioOutputGetAudioSink();
    DUT_AUDIO->tone_sink = speaker_sink;

    /*request an indication that the tone has completed / been disconnected*/
    VmalMessageSinkTask(DUT_AUDIO->tone_sink, (TaskData*)&csr_dut_audio_plugin);

    CsrDutAudioPluginSetVolume(DUT_AUDIO->tone_volume);
}

/****************************************************************************
DESCRIPTION
    Stop a tone from currently playing
*/
void CsrDutAudioPluginStopTone ( void ) 
{  
    PRINT(("DUT_AUDIO: Stop Tone\n"));
   
    if (DUT_AUDIO && DUT_AUDIO->tone_sink)
    {
        CsrDutAudioPluginDisconnectAudio();
        DUT_AUDIO->tone_sink = 0;
        PRINT(("DUT_AUDIO: Disconnect Tone\n"));
    }
}

/****************************************************************************
DESCRIPTION
    a tone has completed
    
*/
bool CsrDutAudioPluginToneComplete(void)
{
    PRINT(("DUT_AUDIO: Tone Complete\n"));
  
    CsrDutAudioPluginStopTone();
    
    VmalMessageSinkTask (DUT_AUDIO->tone_sink , NULL);

    /* if plugin is still connected and no other audio routed then restart tone */
    if (DUT_AUDIO && !DUT_AUDIO->audio_sink)
    {        
        return FALSE;        
    }
    return TRUE;
}


/****************************************************************************
DESCRIPTION
    repeat the tone
    
*/
void CsrDutAudioPluginRepeatTone(Task task)
{
    MAKE_AUDIO_MESSAGE( AUDIO_PLUGIN_PLAY_TONE_MSG, message ) ;
    
    PRINT(("DUT_AUDIO: Queue Repeat\n"));

    message->tone = DUT_AUDIO->tone;
    message->can_queue = FALSE;
    message->tone_volume = DUT_AUDIO->tone_volume;
    message->features.stereo = DUT_AUDIO->tone_stereo;
    
    CsrDutAudioPluginIncrementToneOutputChannel();

    MessageSendConditionallyOnTask(task, AUDIO_PLUGIN_PLAY_TONE_MSG, message, AudioBusyPtr());
}
