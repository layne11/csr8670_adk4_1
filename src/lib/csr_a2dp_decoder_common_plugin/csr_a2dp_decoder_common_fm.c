/****************************************************************************
Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    csr_decoder_common_fm.c
DESCRIPTION
    plugin implentation which routes the sco audio though the dsp
NOTES
*/

#include <audio.h>
#include <source.h>
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
#include "csr_a2dp_decoder_common_fm.h"


/****************************************************************************
DESCRIPTION
    This function disconnects the FM I2S ports
    

****************************************************************************/
void AudioDecodeFMConfigure(Source source)
{
    DECODER_t * DECODER = CsrA2dpDecoderGetDecoderData();

    PanicFalse(SourceConfigure(source, STREAM_I2S_MASTER_MODE, TRUE));
    PanicFalse(SourceConfigure(source, STREAM_I2S_SYNC_RATE, DECODER->rate));
}
