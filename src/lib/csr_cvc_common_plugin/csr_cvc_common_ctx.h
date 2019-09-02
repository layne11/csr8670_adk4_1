/****************************************************************************
Copyright (c) 2016 Qualcomm Technologies International, Ltd.
Part of ADK 4.1
 
FILE NAME
    csr_cvc_common_ctx.h
 
DESCRIPTION
    Definition of the context structure.
*/

#ifndef LIBS_CSR_CVC_COMMON_PLUGIN_CSR_CVC_COMMON_CTX_H_
#define LIBS_CSR_CVC_COMMON_PLUGIN_CSR_CVC_COMMON_CTX_H_

#include <message.h>
#include <sink.h>
#include <source.h>

#include "audio_plugin_if.h"
#include "audio_plugin_common.h"
#include "audio_output.h"

#include "csr_cvc_common.h"

typedef enum
{
    cvc_hands_free_kit,
    cvc_passthrough
} cvc_processing_mode_t;

typedef struct audio_Tag
{
    /* vm task to send messages back to */
    TaskData* app_task;

    /*! The audio sink being used*/
    Sink audio_sink ;
    /*! The audio source being used*/
    Source audio_source;
    /*! Over the air rate  */
    uint32 incoming_rate;
    /*! Audio rate - used for mic switch */
    uint32 audio_rate;

    const common_mic_params* digital;

    /*! The current volume level*/
    unsigned volume:8;
    /*! The current tone volume level*/
    unsigned tone_volume:8;

    /*! Indicates current mic/output for no_dsp */
    microphone_input_id_t mic_for_no_cvc:3;
    audio_output_t output_for_no_cvc:3;
    unsigned unused_2:2;

    /*! The current CVC mode */
    AUDIO_MODE_T mode:4 ;
    /*! The link_type being used*/
    AUDIO_SINK_T link_type:4 ;
    /*! CVC processing mode */
    cvc_processing_mode_t processing_mode:1;
    /*! Whether or not CVC is running */
    unsigned cvc_running:1 ;
    /*! Set if in low power(CVC off) or no DSP mode*/
    unsigned no_dsp:1 ;
    unsigned production_testing:1;
    /*! is tone mono or stereo*/
    unsigned tone_stereo:1;
    /*! Mute states */
    unsigned mic_muted:1;
    unsigned speaker_muted:1;
    unsigned unused:1;

    /*! mono or stereo/use i2s output */
    AudioPluginFeatures features;
}CVC_t ;

CVC_t *CsrCvcGetCtx(void);

#endif /* LIBS_CSR_CVC_COMMON_PLUGIN_CSR_CVC_COMMON_CTX_H_ */
