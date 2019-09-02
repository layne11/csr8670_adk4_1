// *****************************************************************************
// Copyright (c) 2015 - 2016 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
//
// *****************************************************************************

#ifndef MV_CODEC_ENCODER_H
#define MV_CODEC_ENCODER_H

#ifdef WOODSTOCK
    #define USES_PEQ
#endif // WOODSTOCK

// Port definitions
.CONST $USB_IN_PORT                                         ($cbuffer.READ_PORT_MASK  + 0);
.CONST $USB_OUT_PORT                                        ($cbuffer.WRITE_PORT_MASK + 1);

.CONST $AUDIO_LEFT_IN_PORT                                  ($cbuffer.READ_PORT_MASK + 0);
.CONST $AUDIO_RIGHT_IN_PORT                                 ($cbuffer.READ_PORT_MASK + 1);

.CONST $CODEC_OUT_PORT                                      ($cbuffer.WRITE_PORT_MASK + 2);
.CONST $CODEC_OUT_PORT_TWO                                  ($cbuffer.WRITE_PORT_MASK + 3);
.CONST $CODEC_IN_PORT                                       ($cbuffer.READ_PORT_MASK  + 2);

// ** defining timer interrupt periods **
#define $TMR_PERIOD_USB_COPY                                625     // Timer period to read the USB port data
#define $TMR_PERIOD_AUDIO_COPY                              1000    // Timer period to read the audio data in analogue/i2s mode (for MP3 and SBC)
#define $TMR_PERIOD_HI_RATE_AUDIO_COPY                      500     // Timer period to read the audio data in analogue/i2s mode (for MP3 and SBC) for 88.2kHz/96kHz
#define $FASTSTREAM_TMR_PERIOD_AUDIO_COPY                   500     // Timer period to read the audio data in analogue mode (for faststream only)
#define $TMR_PERIOD_USB_OUT_AUDIO_COPY                      1000    // Timer period to write audio data into USB port
#define $TMR_PERIOD_CODEC_COPY                              8000    // Timer period to write codec output to codec port (for MP3 and SBC)
#define $FASTSTREAM_TMR_PERIOD_CODEC_COPY                   1000    // Timer period to write codec output to codec port (for faststream only)

#define $BLOCK_SIZE                                         16
#define $RESAMPLER_READ_BACK                                50
#define $USB_SAMPLE_RATE                                    48000
#define $DEFAULT_MINIMIMUM_CODEC_DATA_TO_COPY               1
#define $FASTSTREAM_MINIMUM_CODEC_DATA_TO_COPY              100     // This limit is for faststream copying codec data into the port (slightly less than 3 frames = 3*36=108 words
#define $USB_STALL_TIME_BEFORE_SILENCE_INSERTION            5       // Minimum no activity time(in ms) in usb in port before sending silence

#ifdef SELECTED_ENCODER_SBC
   #define SELECTED_CODEC_FRAME_ENCODE_FUNCTION             $sbcenc.frame_encode
   #define SELECTED_CODEC_RESET_ENCODER_FUNCTION            $sbcenc.reset_encoder
   #define SELECTED_CODEC_INITIALISE_ENCODER_FUNCTION       $sbcenc.init_static_encoder
   #define SELECTED_ENCODER_LIBRARY_HEADER                  "sbc_library.h"
   #define SBC_BITPOOL_CHANGE_MESSAGE                       0x7070
   #define SBC_BITPOOL_SET_LOW_MESSAGE                      0x7080
   #define $PAUSE_SILENCE_DURATION_TO_SEND_MS               200     // Amount of silence (in ms) streamed when inactivity in USB port is detected
   #define ENABLE_FASTSTREAM_VOICE                                  // Define this option to enable bidirectional faststream
   #ifdef ENABLE_FASTSTREAM_VOICE
      #define SELECTED_DECODER_FRAME_DECODE_FUNCTION        $sbcdec.frame_decode
      #define SELECTED_DECODER_RESET_DECODER_FUNCTION       $sbcdec.reset_decoder
      #define SELECTED_DECODER_SILENCE_DECODER_FUNCTION     $sbcdec.silence_decoder
      #define SELECTED_DECODER_INITIALISE_DECODER_FUNCTION  $sbcdec.init_static_decoder
      #define MONITOR_CODEC_BUFFER_LEVEL                    253
      #define FAST_STREAM_FRAME_LENGTH                      36
   #endif
#endif

#ifdef SELECTED_ENCODER_MP3
   #define SELECTED_CODEC_FRAME_ENCODE_FUNCTION             $mp3enc.frame_encode
   #define SELECTED_CODEC_RESET_ENCODER_FUNCTION            $mp3enc.reset_encoder
   #define SELECTED_CODEC_INITIALISE_ENCODER_FUNCTION       $mp3enc.init_encoder
   #define SELECTED_ENCODER_LIBRARY_HEADER                  "mp3enc_library.h"
    .CONST $MESSAGE_MP3ENC_UPDATE_SETTING                       0x7060;
   #define $PAUSE_SILENCE_DURATION_TO_SEND_MS               700     // Amount of silence (in ms) streamed when inactivity in USB port is detected
#endif

#ifdef SELECTED_ENCODER_APTX
   #define SELECTED_CODEC_FRAME_ENCODE_FUNCTION             $aptx.encode
   #define SELECTED_CODEC_RESET_ENCODER_FUNCTION            $aptx.reset_encoder
   #define SELECTED_CODEC_INITIALISE_ENCODER_FUNCTION       0
   .CONST  $MESSAGE_SET_SAMPLE_RATE                         0x7050;
   #define $PAUSE_SILENCE_DURATION_TO_SEND_MS               200     // Amount of silence (in ms) streamed when inactivity in USB port is detected
#endif

#ifdef WOODSTOCK
   #define WOODSTOCK_PEQ_SELECT_CURVE_MESSAGE               0x7090
   #define WOODSTOCK_MUTE_SPEAKER_MESSAGE                   0x70a0
   #define WOODSTOCK_MUTE_MICROPHONE_MESSAGE                0x70b0
#endif // WOODSTOCK

#define $PAUSE_SILENCE_DURATION_TO_SEND ($PAUSE_SILENCE_DURATION_TO_SEND_MS*1000/$TMR_PERIOD_USB_COPY)

#include "core_library.h"
#include "cbops_library.h"
#include "codec_library.h"
#include "mv_codec_encoder_common.h"
#include "src.h"
#include "sbc_library.h"

#ifndef SELECTED_ENCODER_APTX
    #include SELECTED_ENCODER_LIBRARY_HEADER
#endif
#ifdef WOODSTOCK
    #include "kalimba_standard_messages.h"
#endif // WOODSTOCK

#ifdef SPDIF_ENABLE
   #include "spdif_library.h"

   #define VM_CONFIG_SPDIF_APP_MESSAGE_ID                   0x1073  // VM to DSP, to configure the spdif sink app
   .CONST $SPDIF_EVENT_MSG                                  0x1075; // DSP to VM. to notify events in the input stream
   #define $TMR_PERIOD_SPDIF_COPY                           1000    // Timer period to reads the spdif data
   #define SPDIF_PAUSE_THRESHOLD_US                         3000    // Pause detection threshold in micro seconds

   // Port used for SPDIF inputs
   .CONST $SPDIF_IN_LEFT_PORT                               ($cbuffer.READ_PORT_MASK  + 0 + $cbuffer.FORCE_NO_SIGN_EXTEND);
   .CONST $SPDIF_IN_RIGHT_PORT                              ($cbuffer.READ_PORT_MASK  + 1 + $cbuffer.FORCE_NO_SIGN_EXTEND);

#endif // SPDIF_ENABLE

#ifdef SELECTED_ENCODER_APTX
   // Port limit to prevent "lone" byte issue in the sink side
   .CONST  $OUT_PORT_FILL_LIMIT                             425;    // Limit on the output port fill level (in words)
#endif

#endif // MV_CODEC_ENCODER_H
