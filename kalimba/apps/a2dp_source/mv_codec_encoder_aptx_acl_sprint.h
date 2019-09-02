// *****************************************************************************
// Copyright (c) 2015 - 2016 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
//
// *****************************************************************************

#ifndef MV_CODEC_ENCODER_APTX_ACL_SPRINT_H
#define MV_CODEC_ENCODER_APTX_ACL_SPRINT_H

#ifdef WOODSTOCK
    #define USES_PEQ
#endif // WOODSTOCK

// Port definitions
.CONST $USB_IN_PORT                                         ($cbuffer.READ_PORT_MASK  + 0);
.CONST $USB_OUT_PORT                                        ($cbuffer.WRITE_PORT_MASK + 1);

.CONST $AUDIO_LEFT_IN_PORT                                  ($cbuffer.READ_PORT_MASK  + 0);
.CONST $AUDIO_RIGHT_IN_PORT                                 ($cbuffer.READ_PORT_MASK  + 1);

.CONST $CODEC_OUT_PORT                                      ($cbuffer.WRITE_PORT_MASK + 2);

#ifdef ENABLE_BACK_CHANNEL
    .CONST $CODEC_IN_PORT                                   ($cbuffer.READ_PORT_MASK  + 2);
    #ifdef ANALOGUE_VOICE_OUT
        .CONST $AUDIO_OUT_PORT                              ($cbuffer.WRITE_PORT_MASK + 0);
    #endif // ANALOGUE_VOICE_OUT
#endif // ENABLE_BACK_CHANNEL

// Defining timer interrupt periods
#define $TMR_PERIOD_USB_COPY                                625     // Timer period to read the USB port data
#define $TMR_PERIOD_AUDIO_COPY                              625     // Timer period to read the audio data in analogue/i2s mode
#define $TMR_PERIOD_HI_RATE_AUDIO_COPY                      500     // Timer period to read the audio data in analogue/i2s mode (for aptX-ll) for 88.2kHz/96kHz
#define $TMR_PERIOD_USB_OUT_AUDIO_COPY                      1000    // Timer period to write audio data into USB port
#define $TMR_PERIOD_ANALOGUE_OUT_AUDIO_COPY                 5000
#define $TMR_PERIOD_CODEC_IN_COPY                           1000

#define $BLOCK_SIZE                                         16
#define $RESAMPLER_READ_BACK                                50
#define $USB_SAMPLE_RATE                                    48000
#define $USB_STALL_TIME_BEFORE_SILENCE_INSERTION            5       // Minimum no activity time(in ms) in usb in port before sending silence

#define SELECTED_CODEC_FRAME_ENCODE_FUNCTION                $aptx_sprint.encode
#define SELECTED_CODEC_RESET_ENCODER_FUNCTION               $aptx_sprint.reset_encoder
#define SELECTED_CODEC_INITIALISE_ENCODER_FUNCTION          0
#define $PAUSE_SILENCE_DURATION_TO_SEND_MS                  200     // Amount of silence (in ms) streamed when inactivity in USB port is detected

#ifdef ENABLE_BACK_CHANNEL
    #define SELECTED_DECODER_FRAME_DECODE_FUNCTION          $wbsdec.wbs_frame_decode
    #define SELECTED_DECODER_RESET_DECODER_FUNCTION         $sbcdec.reset_decoder
    #define SELECTED_DECODER_SILENCE_DECODER_FUNCTION       $sbcdec.silence_decoder
    #define SELECTED_DECODER_INITIALISE_DECODER_FUNCTION    $wbsdec.init_decoder
    #define SELECTED_CODEC_LIBRARY_INITIALISATION           $sbcdec.init_static_decoder
    #define MONITOR_CODEC_BUFFER_LEVEL                      204
    #define VOICE_FRAME_LENGTH                              29
#endif // ENABLE_BACK_CHANNEL

#define $MESSAGE_SET_SAMPLE_RATE                            0x7050
#ifdef WOODSTOCK
    #define WOODSTOCK_PEQ_SELECT_CURVE_MESSAGE              0x7090
    #define WOODSTOCK_MUTE_SPEAKER_MESSAGE                  0x70a0
    #define WOODSTOCK_MUTE_MICROPHONE_MESSAGE               0x70b0
#endif // WOODSTOCK

#define $PAUSE_SILENCE_DURATION_TO_SEND ($PAUSE_SILENCE_DURATION_TO_SEND_MS*1000/$TMR_PERIOD_USB_COPY)

#include "core_library.h"
#include "cbops_library.h"
#include "codec_library.h"
#include "mv_codec_encoder_common.h"
#include "src.h"
#include "aptx_api.h"

#ifdef WOODSTOCK
    #include "kalimba_standard_messages.h"
#endif // WOODSTOCK

#ifdef SPDIF_ENABLE
   #include "spdif_library.h"

   #define VM_CONFIG_SPDIF_APP_MESSAGE_ID                   0x1073      // VM to DSP, to configure the spdif sink app
   .CONST $SPDIF_EVENT_MSG                                  0x1075;     // DSP to VM. to notify events in the input stream
   #define $TMR_PERIOD_SPDIF_COPY                           1000        // timer period to reads the spdif data
   #define SPDIF_PAUSE_THRESHOLD_US                         3000        // pause detection threshold in micro seconds

   // Port used for SPDIF inputs
   .CONST $SPDIF_IN_LEFT_PORT                               ($cbuffer.READ_PORT_MASK  + 0 + $cbuffer.FORCE_NO_SIGN_EXTEND);
   .CONST $SPDIF_IN_RIGHT_PORT                              ($cbuffer.READ_PORT_MASK  + 1 + $cbuffer.FORCE_NO_SIGN_EXTEND);

#endif // SPDIF_ENABLE

#endif // MV_CODEC_ENCODER_APTX_ACL_SPRINT_H
