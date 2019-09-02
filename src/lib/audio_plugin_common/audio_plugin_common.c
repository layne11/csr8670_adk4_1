/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    audio_plugin_common.c
    
DESCRIPTION
    Implementation of audio plugin common library.
*/

#include <stream.h>
#include <source.h>
#include <micbias.h>
#include <pio.h>
#include <pio_common.h>

#include "audio_plugin_if.h"
#include "audio_plugin_common.h"

/****************************************************************************
DESCRIPTION
    Get hardware instance from mic parameters
*/
audio_instance AudioPluginGetInstance(const audio_mic_params params)
{
    if (params.instance == 0) return AUDIO_INSTANCE_0;
    if (params.instance == 1) return AUDIO_INSTANCE_1;
    if (params.instance == 2) return AUDIO_INSTANCE_2;

    /* Trying to use unsupported audio_instance */
    Panic();
    return AUDIO_INSTANCE_0; /* Satisfy compiler, should never get here */
}
/****************************************************************************
DESCRIPTION
    Set up of a couple of common microphone levels
*/
const T_mic_gain MIC_MUTE = {0,0,8,0};  /* -45db, -24db, preamp=off */
const T_mic_gain MIC_DEFAULT_GAIN = {1,0,0x1,0xa}; /* +3db for digital and analog, preamp=in */


/****************************************************************************
DESCRIPTION
    Get analogue or digital mic
*/
Source AudioPluginGetMic(audio_instance instance, audio_channel channel, bool digital)
{
    return StreamAudioSource(digital ? AUDIO_HARDWARE_DIGITAL_MIC : AUDIO_HARDWARE_CODEC, instance, channel);
}


/****************************************************************************
DESCRIPTION
    Configure Mic channel
*/
void AudioPluginSetMicRate(Source mic_source, bool digital, uint32 adc_rate)
{
    PanicFalse(SourceConfigure(mic_source, digital ? STREAM_DIGITAL_MIC_INPUT_RATE : STREAM_CODEC_INPUT_RATE, adc_rate));
}


/****************************************************************************
DESCRIPTION
    Set mic gain
*/
void AudioPluginSetMicGain(Source mic_source, bool digital, uint16 gain, bool preamp)
{
    if (digital)
    {
        SourceConfigure(mic_source, STREAM_DIGITAL_MIC_INPUT_GAIN, gain);
    }
    else
    {
        SourceConfigure(mic_source, STREAM_CODEC_INPUT_GAIN, gain);
        SourceConfigure(mic_source, STREAM_CODEC_MIC_INPUT_GAIN_ENABLE, preamp);
    }
}

/****************************************************************************
DESCRIPTION
    Helper to set microphone PIO state
*/
static bool audioPluginSetMicPio(const audio_mic_params params, bool set)
{
    if(params.bias_config != BIAS_CONFIG_DISABLE)
    {
        if(params.bias_config == BIAS_CONFIG_PIO)
        {
            return PioCommonSetPin(params.pio, pio_drive, set);
        }
        else
        {
            mic_bias_id id = (params.bias_config == BIAS_CONFIG_MIC_BIAS_0 ? 
                                                    MIC_BIAS_0 : 
                                                    MIC_BIAS_1);
            uint16 value = (set ? MIC_BIAS_FORCE_ON : MIC_BIAS_OFF);
            return MicbiasConfigure(id, MIC_BIAS_ENABLE, value);
        }
    }
    /* Nothing to do, success! */
    return TRUE;
}

/****************************************************************************
DESCRIPTION
    Set mic bias or digital mic PIO on or off
*/
void AudioPluginSetMicPio(const audio_mic_params params, bool set)
{
    PanicFalse(audioPluginSetMicPio(params, set));
}

/****************************************************************************
DESCRIPTION
    Set mic bias or digital mic PIO to default state (off)
*/
void AudioPluginInitMicPio(const audio_mic_params params)
{
    /* Failure here most likely indicates "already off" so ignore it */
    (void)audioPluginSetMicPio(params, FALSE);
}

/****************************************************************************
DESCRIPTION
    Apply mic configuration and set mic PIO
*/
Source AudioPluginMicSetup(audio_channel channel, const audio_mic_params params, uint32 rate)
{
    Source mic_source = AudioPluginGetMic(AudioPluginGetInstance(params), channel, params.digital);
    AudioPluginSetMicRate(mic_source, params.digital, rate);
    AudioPluginSetMicGain(mic_source, params.digital, params.gain, params.pre_amp);
    AudioPluginSetMicPio(params, TRUE);
    return mic_source;
}

