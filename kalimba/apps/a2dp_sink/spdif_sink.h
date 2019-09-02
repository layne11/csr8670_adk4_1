// *****************************************************************************
// Copyright (c) 2003 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
//
// *****************************************************************************

#ifndef SPDIF_SINK_HEADER_INCLUDED
#define SPDIF_SINK_HEADER_INCLUDED

   #define TMR_PERIOD_SPDIF_COPY           1000   // spdif data copy runs per 1ms
   #define MAX_SPDIF_SRA_RATE              0.015  // maximum mismatch rate between input and output clocks
   #define SPDIF_PACKET_BOUNDARY_INFO_SIZE 40     // size of cbuffer for packets' boundary info
   #define SPDIF_DEFAULT_LATENCY_MS        40     // default latency
   #define MIN_SPDIF_LATENCY_MS            25     // minimum target latency
   #define MIN_SPDIF_LATENCY_WITH_SUB_MS   40     // minimum target latency when sub is connected
   #define MAX_SPDIF_LATENCY_CODED_MS      150    // maximum target latency for coded input
   #define MAX_SPDIF_LATENCY_PCM_MS        70     // maximum target latency for pcm input

   #define SPDIF_LATENCY_CONVERGE_US       150    // in us, assumed converged when latency diff reaches to this value
   #define OUTPUT_INTERFACE_INIT_ACTIVE           // output is connected acive not inactive
   #define SPDIF_PAUSE_THRESHOLD_US        5000   // pause detection threshold in micro seconds
   #define SPDIF_PAUSE_SILENCE_TO_ADD_MS   50     // maximum amount of silence to insert during pause

   // define this only for multi channel apps
   // it can be undefined for stereo output
   #define SPDIF_DISABLE_OUTPUT_RATE_CONTROL

   // Port used for SPDIF inputs
   .CONST $SPDIF_IN_LEFT_PORT        ($cbuffer.READ_PORT_MASK  + 0 + $cbuffer.FORCE_NO_SIGN_EXTEND);
   .CONST $SPDIF_IN_RIGHT_PORT       ($cbuffer.READ_PORT_MASK  + 1 + $cbuffer.FORCE_NO_SIGN_EXTEND);

   .CONST $spdif_sra.RATE_CALC_HIST_SIZE 32;

   .CONST $spdif_sra.MAX_RATE_FIELD                  0;  // maximum mismatch rate to compensate
   .CONST $spdif_sra.TARGET_LATENCY_MS_FIELD         1;  // target latency in ms
   .CONST $spdif_sra.CURRENT_LATENCY_PTR_FIELD       2;  // current latency in the system
   .CONST $spdif_sra.OFFSET_LATENCY_US_FIELD         3;  // target latency in ms
   .CONST $spdif_sra.SRA_RATE_FIELD                  4;  // calculated mismatch rate
   .CONST $spdif_sra.FIX_RATE_FIELD                  5;  // a small fix for rate when the buffer gets full or empty
   .CONST $spdif_sra.SAMPLES_DELAY_AVERAGE_FIELD     6;  // Average delay in samples
   .CONST $spdif_sra.AVERAGE_LATENCY_FIELD           7;  // current average latency
   .CONST $spdif_sra.AVERAGE_LATENCY_LEFT_FIELD      8;  // latency can be added by this value
   .CONST $spdif_sra.LATENCY_CONVERGED_FIELD         9;  // flag showing latemcy converged
   .CONST $spdif_sra.RATE_BEFORE_FIX_FIELD           10; // actual clock mismatch rate (x16)
   .CONST $spdif_sra.EXPECTED_SAMPLE_RATE_FIELD      11; // Samples consumed by output port (slave operation only)
   .CONST $spdif_sra.EXPECTED_SAMPLE_RATE_INV_FIELD  12; // Inverse of above. Number of microseconds in 1 sample
   .CONST $spdif_sra.STRUC_SIZE                      13;

   .CONST $RESOLUTION_MODE_16BIT                     16; // 16bit resolution mode
   .CONST $RESOLUTION_MODE_24BIT                     24; // 24bit resolution mode

   // define SPDIF related message IDs
   #define VM_CONFIG_SPDIF_APP_MESSAGE_ID             0x1073  // VM to DSP, to configure the spdif sink app
   #define VM_AC3_DECODER_CONFIG_MESSAGE_ID           0x1074  // VM to DSP, to configure ac-3 decoder
   .CONST $SPDIF_EVENT_MSG                            0x1075; // DSP to VM. to notify events in the input stream
   #define VM_AC3_USER_INFO_REQUEST_MESSGE_ID         0x1076  // VM to DSP, to request for latest AC-3 stream status
   #define AC3_USER_INFO_MSG                          0x1077  // DSP to VM, to send the latest status of AC-3 stream
   .CONST $SPDIF_FULL_CHSTS_WORDS_MESSAGE_ID          0x1078;  // DSP to VM, to senf the whole channel status bits
   
   
   // channel status alternating period in micro seconds
   // 0 means no alternating required
   #define SPDIF_CHSTS_ALTERNATING_PERIOD_US  100000

   // bitmask, which channel status bits to report
   // b0: channel A, b1: channel B
   #define SPDIF_CHSTS_REPORTING_CHANNELS     0x3

#endif // #ifndef   SPDIF_SINK_HEADER_INCLUDED
