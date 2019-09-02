/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    audio_plugin_common.h
    
DESCRIPTION
    Header file for the audio plugin common library.
*/

/*!
\defgroup audio_plugin_common audio_plugin_common
\ingroup vm_libs

\brief  Header file for the audio plugin common library.
    
\section audio_plugin_common_intro INTRODUCTION
        This defines functionality common to all audio plugins.
*/

/*@{*/

#ifndef _AUDIO_PLUGIN_COMMON_H_
#define _AUDIO_PLUGIN_COMMON_H_

#include "audio_plugin_if.h"

/****************************************************************************
DESCRIPTION
	Data Description of Mic Gain Levels
*/
typedef struct tag_mic_type
{
   unsigned preamp_enable:1; /* high bit */
   unsigned unused:6;
   unsigned digital_gain:4;
   unsigned analogue_gain:5;   /* low bits */
} T_mic_gain;

/****************************************************************************
DESCRIPTION
	Data Description of a couple of common mic gain levels
*/
extern const T_mic_gain MIC_MUTE;         /* -45db, -24db, preamp=off */
extern const T_mic_gain MIC_DEFAULT_GAIN; /* +3db for digital and analog, preamp=in */

/****************************************************************************
DESCRIPTION
    Get hardware instance from mic parameters
*/
audio_instance AudioPluginGetInstance(const audio_mic_params params);

/****************************************************************************
DESCRIPTION
    Get analogue or digital mic
*/
Source AudioPluginGetMic(audio_instance instance, audio_channel channel, bool digital);


/****************************************************************************
DESCRIPTION
    Configure Mic channel
*/
void AudioPluginSetMicRate(Source mic_source, bool digital, uint32 adc_rate);


/****************************************************************************
DESCRIPTION
    Set mic gain
*/
void AudioPluginSetMicGain(Source mic_source, bool digital, uint16 gain, bool preamp);


/****************************************************************************
DESCRIPTION
    Set mic bias or digital mic PIO on or off
*/
void AudioPluginSetMicPio(const audio_mic_params params, bool set);


/****************************************************************************
DESCRIPTION
    Set mic bias or digital mic PIO to default state (off)
*/
void AudioPluginInitMicPio(const audio_mic_params params);


/****************************************************************************
DESCRIPTION
    Apply mic configuration and set mic PIO
*/
Source AudioPluginMicSetup(audio_channel channel, const audio_mic_params params, uint32 rate);

#endif /* _AUDIO_PLUGIN_COMMON_H_ */

/*@}*/
