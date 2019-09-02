// *****************************************************************************
// Copyright (c) 2007 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
//
// $Change: 2612975 $  $DateTime: 2016/08/04 16:34:47 $
// $Change: 2612975 $  $DateTime: 2016/08/04 16:34:47 $
// *****************************************************************************


// *****************************************************************************
// DESCRIPTION
//    This is a passthrough application. Project files use compile time
//    flags to provide the following applications:
//
//    1) one_mic_example_16k (for use with headset sink application)
//       ADC @ 16kHz -> DSP with WBS encoder -> eSCO
//       DAC @ 16kHz <- DSP with WBS decoder <- eSCO
//
//    2) one_mic_example_cvsd (for use with headset sink application)
//       ADC @ 8kHz -> DSP -> SCO
//       DAC @ 8kHz <- DSP <- SCO
//
//    3) usb_dongle_8k_mono (for use with a dongle source application)
//       USB mono @ 8kHz -> DSP -> SCO
//       USB mono @ 8kHz <- DSP <- SCO
//
//    4) usb_dongle_16k_mono (for use with a dongle source application)
//       USB mono @ 16kHz -> DSP with WBS encoder -> eSCO
//       USB mono @ 16kHz <- DSP with WBS decoder <- eSCO
//
//    5) usb_dongle_48_to_16k_stereo (for use with a dongle source application)
//       USB stereo @ 48kHz -> DSP with WBS encoder -> eSCO
//       USB mono   @ 48kHz <- DSP with WBS decoder <- eSCO
//       note: The right channel of the USB input is discarded.
//
//    6) usb_dongle_48_to_8k_stereo (for use with a dongle source application)
//       USB mono @ 48kHz -> DSP with WBS encoder -> eSCO
//       USB mono @ 48kHz <- DSP with WBS decoder <- eSCO
//       note: The right channel of the USB input is discarded.
//
//   OTHER NOTES:
//
//    The ADC/DAC gain levels are set in the plugin (csr_common_example_plugin)
//
//    To enable/disable PLC, modify the following variable :
//      M[$sco_in.decoder_obj + $sco_pkt_handler.ENABLE_FIELD]
//      (0 to disable, 0x2000 to enable)
//
// *****************************************************************************

// #define FRAME_SYNC_DEBUG  // Define this if using frame_sync_debug library.
                             // It must be defined before the following #include
                             // statements.

#include "../common/ports.h"
#include "core_library.h"
#include "cbops_multirate_library.h"
#include "kalimba_messages.h"
#include "frame_sync_library.h"
#include "frame_codec.h"
#include "usbio.h"
#include "plc100_library.h"
#include "one_mic_example.h"
#include "frame_sync_stream_macros.h"
#include "flash.h"
#include "stream_copy.h"
#include "sbc_library.h"
#include "codec_library.h"
#include "cvc_system_library.h"
#include "frame_sync_tsksched.h"

#define uses_PLC                 1


   // System defines
   // --------------
#if uses_16kHz

#if uses_48_to_16kHz
   .CONST $SAMPLE_RATE           48000;
   .CONST $BLOCK_SIZE            120;
#else
   .CONST $SAMPLE_RATE           16000;
   .CONST $BLOCK_SIZE            120;
#endif

#else // 8kHz

#if uses_48_to_8kHz
   .CONST $SAMPLE_RATE           48000;
   .CONST $BLOCK_SIZE            60;
#else
   .CONST $SAMPLE_RATE           8000;
   .CONST $BLOCK_SIZE            60;
#endif

#endif

#if defined(USB_DONGLE)
   #define $SAMPLE_SIZE                               2        // 2 bytes per sample
   #define $PACKET_RATE                               1000     // Number of packets in 1 second
#ifdef STEREO_USB_INPUT
   #define $NUM_CHANNELS                              2        // Number of channels (Mono: 1, Stereo: 2)
#else
   #define $NUM_CHANNELS                              1        // Number of channels (Mono: 1, Stereo: 2)
#endif
   #define $USB_PACKET_LEN          (($SAMPLE_RATE * $SAMPLE_SIZE * $NUM_CHANNELS) / $PACKET_RATE)   // Number of audio data bytes in a USB packet (for all channels)
#endif

   .CONST $WARP_PARAMS_FROM_VM_MESSAGE_ID       0x1026;
   .CONST $SAMPLE_RATE_FROM_VM_MESSAGE_ID       0x1070;
   .CONST $READY_TO_VM_MESSAGE_ID               0x1000;
   // 1ms is chosen as the interrupt rate for the audio input/output to minimize latency.
   .CONST $TIMER_PERIOD_AUDIO_COPY_MICROS       625;
   .CONST $BUFFER_SCALING                       4;
   .CONST $TONE_BUFFER_SIZE                     ($BLOCK_SIZE * 2);
   .CONST $ONE_MIC_EXAMPLE_BUFFER_SIZE          $BLOCK_SIZE;


   .CONST  $SCO_IN_PORT          ($cbuffer.READ_PORT_MASK  + 1);
   .CONST  $SCO_OUT_PORT         ($cbuffer.WRITE_PORT_MASK + $CODEC_ESCO_OUT_PORT_NUMBER);

#ifdef USB_DONGLE
   .CONST  $USB_IN_PORT          ($cbuffer.READ_PORT_MASK + 0);
   .CONST  $USB_OUT_PORT         ($cbuffer.WRITE_PORT_MASK + $USB_OUT_PORT_NUMBER);
#else
   .CONST  $ADC_PORT             (($cbuffer.READ_PORT_MASK  +  0 ) | $cbuffer.FORCE_PCM_AUDIO);
   .CONST  $DAC_PORT_L           (($cbuffer.WRITE_PORT_MASK + $PRIMARY_LEFT_OUT_PORT_NUMBER) | $cbuffer.FORCE_PCM_AUDIO);
   .CONST  $DAC_PORT_R           (($cbuffer.WRITE_PORT_MASK + $PRIMARY_RIGHT_OUT_PORT_NUMBER) | $cbuffer.FORCE_PCM_AUDIO);
#endif
   .CONST  $TONE_PORT            (($cbuffer.READ_PORT_MASK  +  3 ) | $cbuffer.FORCE_PCM_AUDIO);

.MODULE $cbops.scratch;
   .DATASEGMENT DM;

    .VAR BufferTable[6*$cbops_multirate.BufferTable.ENTRY_SIZE];

    // Scratch sufficient for 96kHz sample rate @ 625 usec
    DeclareCBuffer(cbuffer_struc1,mem1,120);
    DeclareCBuffer(cbuffer_struc2,mem2,120);
    DeclareCBuffer(cbuffer_struc3,mem3,120);

.ENDMODULE;

#if defined(USB_DONGLE)

.MODULE $usb_in;
   .DATASEGMENT DM;
   DeclareCBuffer (cbuffer_struc,mem, (2* $SAMPLE_RATE / 1000));

#ifdef STEREO_USB_INPUT
   DeclareCBuffer(right.cbuffer_struc,right.mem,(2* $SAMPLE_RATE / 1000));
   .VAR/DM1 copy_struc[$frame_sync.USB_IN_STEREO_COPY_STRUC_SIZE] =
      $USB_IN_PORT,                 /* $USB_IN.STEREO_COPY_SOURCE_FIELD */
      &cbuffer_struc,       /* $USB_IN.STEREO_COPY_LEFT_SINK_FIELD */
      &right.cbuffer_struc, /* $USB_IN.STEREO_COPY_RIGHT_SINK_FIELD */
      $USB_PACKET_LEN,             /* $USB_IN.STEREO_COPY_PACKET_LENGTH_FIELD */
      8,                   /* $USB_IN.STEREO_COPY_SHIFT_AMOUNT_FIELD */
      0 ...;
#else
   // ** allocate memory for USB input (non-cbops) copy routine **
   .VAR/DM copy_struc[$frame_sync.USB_IN_MONO_COPY_STRUC_SIZE] =
      $USB_IN_PORT,                             // USB source port
      &cbuffer_struc,                           // Sink buffer
      $USB_PACKET_LEN,                          // Packet length (Number of audio data bytes in a USB packet for all channels)
      8,                                        // Shift amount
      0 ...;
#endif

.ENDMODULE;

#define USBIN_INPUT_INDEX       0
#define USBIN_OUTPUT_INDEX      1
#define USBIN_INTERNAL_INDEX    2

.MODULE $usb_in_rm;
   .DATASEGMENT DM;

//          usbin       internal         usbin_rm
//    cBuffer --> RESAMPLE -+-> RATE_MATCH --> cBuffer
//                          |
//                      RATE_MONITOR

   DeclareCBuffer (cbuffer_struc,mem,$BLOCK_SIZE * $BUFFER_SCALING);

  .VAR/DM1CIRC sr_hist_left[$cbops.rate_adjustment_and_shift.SRA_COEFFS_SIZE];

   .VAR copy_struc[] =
        $cbops.scratch.BufferTable,     // BUFFER_TABLE_FIELD
        &copy_op,                       // MAIN_FIRST_OPERATOR_FIELD
        &sw_copy_op,                    // MTU_FIRST_OPERATOR_FIELD
        1,                              // NUM_INPUTS_FIELD
        &$usb_in.cbuffer_struc,
        1,                              // NUM_OUTPUTS_FIELD
        &cbuffer_struc,
        1,                              // NUM_INTERNAL_FIELD
        &$cbops.scratch.cbuffer_struc2;

     //  Copy/Resampler Operator
    .BLOCK copy_op;
        .VAR copy_op.mtu_next  = $cbops.NO_MORE_OPERATORS;
        .VAR copy_op.main_next = &sw_rate_op;
        .VAR copy_op.func = $cbops_iir_resamplev2;
        .VAR copy_op.param[$iir_resamplev2.OBJECT_SIZE] =
            USBIN_INPUT_INDEX,               // Input index
            USBIN_INTERNAL_INDEX,            // Output index
#if uses_48_to_16kHz
            &$M.iir_resamplev2.Up_1_Down_3.filter,// FILTER_DEFINITION_PTR_FIELD
#elif uses_48_to_8kHz
            &$M.iir_resamplev2.Up_1_Down_6.filter,// FILTER_DEFINITION_PTR_FIELD
#else
            0,                               // Pass-Through (Copy)
#endif
            -8,                              // INPUT_SCALE_FIELD   (input Q15)
            8,                               // OUTPUT_SCALE_FIELD  (output Q23)
            &$cbops.scratch.mem1,            // INTERMEDIATE_CBUF_PTR_FIELD
            LENGTH($cbops.scratch.mem1),     // INTERMEDIATE_CBUF_LEN_FIELD
            0 ...;
    .ENDBLOCK;

    .BLOCK sw_rate_op;
        .VAR sw_rate_op.mtu_next  = &copy_op;
        .VAR sw_rate_op.main_next = &sw_copy_op;
        .VAR sw_rate_op.func = &$cbops.rate_monitor_op;
        .VAR sw_rate_op.param[$cbops.rate_monitor_op.STRUC_SIZE] =
            USBIN_INTERNAL_INDEX,         // MONITOR_INDEX_FIELD
            1600,                       // PERIODS_PER_SECOND_FEILD
            10,                         // SECONDS_TRACKED_FIELD
#if uses_16kHz
            16000,                      // TARGET_RATE_FIELD
#else
            8000,
#endif
            10,                         // ALPHA_LIMIT_FIELD (controls the size of the averaging window)
            0.5,                        // AVERAGE_IO_RATIO_FIELD - initialize to 1.0 in q.22
            11,                         // WARP_MSG_LIMIT_FIELD
            0 ...;
    .ENDBLOCK;

    .BLOCK sw_copy_op;
        .VAR sw_copy_op.mtu_next  = &sw_rate_op;
        .VAR sw_copy_op.main_next = $cbops.NO_MORE_OPERATORS;
        .VAR sw_copy_op.func = &$cbops.rate_adjustment_and_shift;
        .VAR sw_copy_op.param[$cbops.rate_adjustment_and_shift.STRUC_SIZE] =
            USBIN_INTERNAL_INDEX,       // INPUT1_START_INDEX_FIELD
            USBIN_OUTPUT_INDEX,         // OUTPUT1_START_INDEX_FIELD
            0,                          // SHIFT_AMOUNT_FIELD
            0,                          // MASTER_OP_FIELD
            &$sra_coeffs,               // FILTER_COEFFS_FIELD
            &sr_hist_left,              // HIST1_BUF_FIELD
            &sr_hist_left,              // HIST1_BUF_START_FIELD
            &sw_rate_op.param + $cbops.rate_monitor_op.WARP_VALUE_FIELD, // SRA_TARGET_RATE_ADDR_FIELD
            0,                          // ENABLE_COMPRESSOR_FIELD
            0 ...;
    .ENDBLOCK;

.ENDMODULE;

.MODULE $usb_out;
   .DATASEGMENT DM;

  // Only allow this buffer to be two USB packets so frame_sync can drop/add samples
   DeclareCBuffer (cbuffer_struc,mem, (2* $SAMPLE_RATE / 1000));

   // ** allocate memory for USB input (non-cbops) copy routine **
   .VAR/DM copy_struc[$frame_sync.USB_OUT_MONO_STRUC_SIZE] =
      &cbuffer_struc,                           // Source buffer
      $USB_OUT_PORT,                            // USB sink port
      -8,                                       // Shift amount
      ($SAMPLE_RATE / 1000),                    // Transfer per period (number of samples transferred on each timer interrupt)
      0;                                        // Stall counter
.ENDMODULE;

#define USBOUT_INPUT_INDEX       0
#define USBOUT_OUTPUT_INDEX      1
#define USBOUT_INTERNAL_INDEX    2

.MODULE $usb_out_rm;
   .DATASEGMENT DM;

//          usbout_rm     internal       usbout
//    cBuffer --> RATE_MATCH ---> RESAMPLE --> cBuffer

    DeclareCBuffer (cbuffer_struc,mem,$BLOCK_SIZE * $BUFFER_SCALING);

    .VAR/DM1CIRC sr_hist_left[$cbops.rate_adjustment_and_shift.SRA_COEFFS_SIZE];

    .VAR copy_struc[] =
        $cbops.scratch.BufferTable,     // BUFFER_TABLE_FIELD
        &sw_copy_op,                    // MAIN_FIRST_OPERATOR_FIELD
        &copy_op,                       // MTU_FIRST_OPERATOR_FIELD
        1,                              // NUM_INPUTS_FIELD
        &cbuffer_struc,
        1,                              // NUM_OUTPUTS_FIELD
        &$usb_out.cbuffer_struc,
        1,                              // NUM_INTERNAL_FIELD
        &$cbops.scratch.cbuffer_struc2;

    .BLOCK sw_copy_op;
        .VAR sw_copy_op.mtu_next  = $cbops.NO_MORE_OPERATORS;
        .VAR sw_copy_op.main_next = &copy_op;
        .VAR sw_copy_op.func = &$cbops.rate_adjustment_and_shift;
        .VAR sw_copy_op.param[$cbops.rate_adjustment_and_shift.STRUC_SIZE] =
            USBOUT_INPUT_INDEX,         // INPUT1_START_INDEX_FIELD
            USBOUT_INTERNAL_INDEX,      // OUTPUT1_START_INDEX_FIELD
            0,                          // SHIFT_AMOUNT_FIELD
            0,                          // MASTER_OP_FIELD
            &$sra_coeffs,               // FILTER_COEFFS_FIELD
            &sr_hist_left,              // HIST1_BUF_FIELD
            &sr_hist_left,              // HIST1_BUF_START_FIELD
            &$usb_in_rm.sw_rate_op.param + $cbops.rate_monitor_op.INVERSE_WARP_VALUE_FIELD, // SRA_TARGET_RATE_ADDR_FIELD
            0,                          // ENABLE_COMPRESSOR_FIELD
            0 ...;
    .ENDBLOCK;

    // Copy/Resampler Operator
    .BLOCK copy_op;
        .VAR copy_op.mtu_next  = &sw_copy_op;
        .VAR copy_op.main_next = $cbops.NO_MORE_OPERATORS;
        .VAR copy_op.func = $cbops_iir_resamplev2;
        .VAR copy_op.param[$iir_resamplev2.OBJECT_SIZE] =
            USBOUT_INTERNAL_INDEX,              // Input index
            USBOUT_OUTPUT_INDEX,                // Output index
#if uses_48_to_16kHz
            $M.iir_resamplev2.Up_3_Down_1.filter,// FILTER_DEFINITION_PTR_FIELD
#elif uses_48_to_8kHz
            $M.iir_resamplev2.Up_6_Down_1.filter,// FILTER_DEFINITION_PTR_FIELD
#else
            0,                                  // Pass-Through (Copy)
#endif
            -8,                          // INPUT_SCALE_FIELD   (input Q15)
            8,                                  // OUTPUT_SCALE_FIELD  (output Q23)
            &$cbops.scratch.mem1,               // INTERMEDIATE_CBUF_PTR_FIELD
            LENGTH($cbops.scratch.mem1),        // INTERMEDIATE_CBUF_LEN_FIELD
            0 ...;
    .ENDBLOCK;

.ENDMODULE;

#else
// INPUT/OUTPUT streams
// --------------------------------------------------------------------------

#define ADCINDEX_PORT           0
#define ADCINDEX_CBUFFER        1
#define ADCINDEX_SIDETONE       2
#define ADCINDEX_INTERNAL       3

.MODULE $adc_in;
   .DATASEGMENT DM;

   DeclareCBuffer (cbuffer_struc,mem,$BLOCK_SIZE * $BUFFER_SCALING);
   DeclareCBuffer (sidetone_cbuffer_struc,sidetone_mem,4*($SAMPLE_RATE / 1000));

   .VAR/DM1CIRC    sr_hist[$cbops.rate_adjustment_and_shift.SRA_COEFFS_SIZE];

   .VAR copy_struc[] =
      $cbops.scratch.BufferTable,     // BUFFER_TABLE_FIELD
      &resample_op,                   // MAIN_FIRST_OPERATOR_FIELD
      &sidetone_copy_op,              // MTU_FIRST_OPERATOR_FIELD
      1,                              // NUM_INPUTS_FIELD
      $ADC_PORT,
      2,                              // NUM_OUTPUTS_FIELD
      &cbuffer_struc,
      &sidetone_cbuffer_struc,
      1,                              // NUM_INTERNAL_FIELD
      $cbops.scratch.cbuffer_struc2;

    .BLOCK resample_op;
        .VAR resample_op.mtu_next  = $cbops.NO_MORE_OPERATORS;
        .VAR resample_op.main_next = &sw_rate_op;
        .VAR resample_op.func = $cbops_iir_resamplev2;
        .VAR resample_op.param[$iir_resamplev2.OBJECT_SIZE] =
            ADCINDEX_PORT,              // Input index
            ADCINDEX_INTERNAL,          // Output index
            0,                          // FILTER_DEFINITION_PTR_FIELD  [---CONFIG---]
            0,                          // INPUT_SCALE_FIELD   (input Q15)
            8,                          // OUTPUT_SCALE_FIELD  (output Q23)
            &$cbops.scratch.mem1,       // INTERMEDIATE_CBUF_PTR_FIELD
            LENGTH($cbops.scratch.mem1),// INTERMEDIATE_CBUF_LEN_FIELD
            0 ...;
    .ENDBLOCK;

    // Software rate matching uses internal buffers
    .BLOCK sw_rate_op;
        .VAR sw_rate_op.mtu_next  = &resample_op;
        .VAR sw_rate_op.main_next = &sw_copy_op;
        .VAR sw_rate_op.func = &$cbops.rate_monitor_op;
        .VAR sw_rate_op.param[$cbops.rate_monitor_op.STRUC_SIZE] =
            ADCINDEX_INTERNAL,          // MONITOR_INDEX_FIELD
            1600,                       // PERIODS_PER_SECOND_FIELD
            5,                          // SECONDS_TRACKED_FIELD
            0,                          // TARGET_RATE_FIELD    [---CONFIG---]
            2,                          // ALPHA_LIMIT_FIELD (controls the size of the averaging window)
            0.5,                        // AVERAGE_IO_RATIO_FIELD - initialize to 1.0 in q.22
            0,                          // WARP_MSG_LIMIT_FIELD
            120,
            0 ...;
    .ENDBLOCK;

    .BLOCK sw_copy_op;
        .VAR sw_copy_op.mtu_next  = &sw_rate_op;
        .VAR sw_copy_op.main_next = &sidetone_copy_op;
        .VAR sw_copy_op.func = &$cbops.rate_adjustment_and_shift;
        .VAR sw_copy_op.param[$cbops.rate_adjustment_and_shift.STRUC_SIZE] =
            ADCINDEX_INTERNAL,          // INPUT1_START_INDEX_FIELD
            ADCINDEX_CBUFFER,           // OUTPUT1_START_INDEX_FIELD
            0,                          // SHIFT_AMOUNT_FIELD
            0,                          // MASTER_OP_FIELD
            &$sra_coeffs,               // FILTER_COEFFS_FIELD
            &sr_hist,                   // HIST1_BUF_FIELD
            &sr_hist,                   // HIST1_BUF_START_FIELD
            &sw_rate_op.param + $cbops.rate_monitor_op.WARP_VALUE_FIELD, // SRA_TARGET_RATE_ADDR_FIELD
            0,                          // ENABLE_COMPRESSOR_FIELD
            0 ...;
    .ENDBLOCK;

    .BLOCK sidetone_copy_op;
       .VAR sidetone_copy_op.mtu_next  = &sw_copy_op;
       .VAR sidetone_copy_op.main_next = $cbops.NO_MORE_OPERATORS;
       .VAR sidetone_copy_op.func = &$cbops.shift;
       .VAR sidetone_copy_op.param[$cbops.shift.STRUC_SIZE] =
          ADCINDEX_CBUFFER,             // Input index field (Input cbuffer)
          ADCINDEX_SIDETONE,            // Output index field (SIDE TONE cbuffer)
          0;                            // Shift parameter field
   .ENDBLOCK;

.ENDMODULE;

#define DACINDEX_CBUFFER        0
#define DACINDEX_SIDETONE       1
#define DACINDEX_PORT_L         2
#define DACINDEX_PORT_R         3
#define DACINDEX_INTERNAL_1     4
#define DACINDEX_INTERNAL_2     5

.MODULE $dac_out;
   .DATASEGMENT DM;

   DeclareCBuffer (cbuffer_struc,mem,$BLOCK_SIZE * $BUFFER_SCALING);

    .VAR RightPortEnabled = 0;
    .VAR aux_tone_peak = 0;
    .VAR $sidetone_gain = 0.0;
    .VAR/DM1CIRC    sr_hist[$cbops.rate_adjustment_and_shift.SRA_COEFFS_SIZE];

    .VAR copy_struc[] =
        $cbops.scratch.BufferTable,     // BUFFER_TABLE_FIELD
        &insert_op,                     // MAIN_FIRST_OPERATOR_FIELD
        &insert_op,                     // MTU_FIRST_OPERATOR_FIELD
        2,                              // NUM_INPUTS_FIELD
        &cbuffer_struc,                 // from frame processing
        &$adc_in.sidetone_cbuffer_struc,// from adc chain
        2,                              // NUM_OUTPUTS_FIELD
        $DAC_PORT_L,                    //
        $DAC_PORT_R,                    // Index 3
        2,                              // NUM_INTERNAL_FIELD
        $cbops.scratch.cbuffer_struc2,
        $cbops.scratch.cbuffer_struc3;

    // cBuffer Insertion Operator (Keep DAC chain Fed)
    .BLOCK insert_op;
        .VAR insert_op.mtu_next  = &dac_wrap_op;
        .VAR insert_op.main_next = &auxillary_mix_op;
        .VAR insert_op.func = &$cbops.insert_op;
        .VAR insert_op.param[$cbops.insert_op.STRUC_SIZE] =
          DACINDEX_CBUFFER,   // BUFFER_INDEX_FIELD
          0,                  // MAX_ADVANCE_FIELD
          0 ...;
    .ENDBLOCK;

    // Mix in Auxillary Audio
    .BLOCK auxillary_mix_op;
        .VAR auxillary_mix_op.mtu_next = $cbops.NO_MORE_OPERATORS;
        .VAR auxillary_mix_op.main_next = &sidetone_mix_op;
        .VAR auxillary_mix_op.func = &$cbops.aux_audio_mix_op;
        .VAR auxillary_mix_op.param[$cbops.aux_audio_mix_op.STRUC_SIZE] =
            DACINDEX_CBUFFER,                /* Input index (Output cbuffer) */
            DACINDEX_CBUFFER,                /* Output index (Output cbuffer) */
            $TONE_PORT,                      /* Auxillary Audio Port */
            $tone_in.cbuffer_struc,          /* Auxillary Audio CBuffer */
            0,                               /* Hold Timer */
            -154,                            /* Hold count.  0.625 msec (ISR rate) * 154 = ~ 96 msec */
            0x80000,   /*(0db) */            /* Auxillary Gain   */
            0x80000,   /*(0db) */            /* Main Gain            (Q5.18) */
            0x008000,  /*(0db) */            /* OFFSET_INV_DAC_GAIN  (Q8.15) */
            1.0,                             /* Volume Independent Clip Point (Q23)*/
            1.0,                               /* Absolute Clip Point  (Q23)*/
            0x40000,                         /* Boost (Q4.19)*/
            0,                               /* Auxillary Audio Peak Statistic */
            1.0,                             /* Inverse gain difference between Main & Tone Volume (Q23) */
            0;                               /* Internal Data */
    .ENDBLOCK;

     // Mix in Sidetone
    .BLOCK sidetone_mix_op;
        .VAR sidetone_mix_op.mtu_next  = &auxillary_mix_op;
        .VAR sidetone_mix_op.main_next = &resample_op;
        .VAR sidetone_mix_op.func = &$cbops.sidetone_mix_op;
        .VAR sidetone_mix_op.param[$cbops.sidetone_mix_op.STRUC_SIZE] =
            DACINDEX_CBUFFER,       // INPUT_START_INDEX_FIELD
            DACINDEX_CBUFFER,       // OUTPUT_START_INDEX_FIELD
            DACINDEX_SIDETONE,      // SIDETONE_START_INDEX_FIELD
#if uses_16kHz
            12,                     // SIDETONE_MAX_SAMPLES_FIELD
#else
            6,                      // SIDETONE_MAX_SAMPLES_FIELD
#endif
            &$sidetone_gain;        // ATTENUATION_PTR_FIELD
    .ENDBLOCK;

    // Resample before rate monitor. works with l_to_r_copy for dual mono support
    .BLOCK resample_op;
        .VAR resample_op.mtu_next = &sidetone_mix_op;
        .VAR resample_op.main_next = &audio_out_volume;
        .VAR resample_op.func = $cbops_iir_resamplev2;
        .VAR resample_op.param[$iir_resamplev2.OBJECT_SIZE] =
            DACINDEX_CBUFFER,             // Input index
            DACINDEX_INTERNAL_1,          // Output index
            0,                            // FILTER_DEFINITION_PTR_FIELD  [---CONFIG---]
           -8,                            // INPUT_SCALE_FIELD   (input Q15)
            8,                            // OUTPUT_SCALE_FIELD  (output Q23)
            &$cbops.scratch.mem1,         // INTERMEDIATE_CBUF_PTR_FIELD
            LENGTH($cbops.scratch.mem1),  // INTERMEDIATE_CBUF_LEN_FIELD
            0 ...;
    .ENDBLOCK;

    .BLOCK audio_out_volume;
        .VAR audio_out_volume.mtu_next  = &resample_op;
        .VAR audio_out_volume.main_next = &sw_copy_op;
        .VAR audio_out_volume.func = &$cbops.volume;
        .VAR audio_out_volume.param[$cbops.volume.STRUC_SIZE] =
            DACINDEX_INTERNAL_1,             // INPUT_START_INDEX_FIELD
            DACINDEX_INTERNAL_1,             // OUTPUT_START_INDEX_FIELD
            1.0,                             // Final Gain Value
            0,                               // Current Gain Value
            0,                               // samples per step,
            -5,                              // step shift field
            0.01,                            // delta field
            0;                               // current step field
    .ENDBLOCK;

    // Apply rate adjustment in software
    .BLOCK sw_copy_op;
        .VAR sw_copy_op.mtu_next  = &audio_out_volume;
        .VAR sw_copy_op.main_next = &sw_rate_op;
        .VAR sw_copy_op.func = &$cbops.rate_adjustment_and_shift;
        .VAR sw_copy_op.param[$cbops.rate_adjustment_and_shift.STRUC_SIZE] =
            DACINDEX_INTERNAL_1,        // INPUT1_START_INDEX_FIELD
            DACINDEX_INTERNAL_2,        // OUTPUT1_START_INDEX_FIELD
            0,                          // SHIFT_AMOUNT_FIELD
            0,                          // MASTER_OP_FIELD
            &$sra_coeffs,               // FILTER_COEFFS_FIELD
            &sr_hist,                   // HIST1_BUF_FIELD
            &sr_hist,                   // HIST1_BUF_START_FIELD
            &sw_rate_op.param + $cbops.rate_monitor_op.INVERSE_WARP_VALUE_FIELD, // SRA_TARGET_RATE_ADDR_FIELD
            0,                          // ENABLE_COMPRESSOR_FIELD
            0 ...;
    .ENDBLOCK;

    // Software rate monitor dac chain independently
    .BLOCK sw_rate_op;
        .VAR sw_rate_op.mtu_next  = &sw_copy_op;
        .VAR sw_rate_op.main_next = &left_copy_op;
        .VAR sw_rate_op.func = &$cbops.rate_monitor_op;
        .VAR sw_rate_op.param[$cbops.rate_monitor_op.STRUC_SIZE] =
            DACINDEX_INTERNAL_2,        // MONITOR_INDEX_FIELD
            1600,                       // PERIODS_PER_SECOND_FIELD
            5,                          // SECONDS_TRACKED_FIELD
            0,                          // TARGET_RATE_FIELD    [---CONFIG---]
            2,                          // ALPHA_LIMIT_FIELD (controls the size of the averaging window)
            0.5,                        // AVERAGE_IO_RATIO_FIELD - initialize to 1.0 in q.22
            0,                          // WARP_MSG_LIMIT_FIELD
            120,                        // IDLE_PERIODS_AFTER_STALL_FIELD (empirical)
            0 ...;
    .ENDBLOCK;

    .BLOCK left_copy_op;
        .VAR left_copy_op.mtu_next  = &sw_rate_op;
        .VAR left_copy_op.main_next = &dual_mono_mux;
        .VAR left_copy_op.func = &$cbops.shift;
        .VAR left_copy_op.param[$cbops.shift.STRUC_SIZE] =
            DACINDEX_INTERNAL_2,        // INPUT_INDEX_FIELD
            DACINDEX_PORT_L,            // OUTPUT_INDEX_FIELD
            -8;                         // SHIFT_AMOUNT_FIELD
    .ENDBLOCK;

    // Mux has only main part. Used as a single operator bypass
    .BLOCK dual_mono_mux;
        .VAR dual_mono_mux.mtu_next = &left_copy_op;
        .VAR dual_mono_mux.main_next = 0; // Set by mux
        .VAR dual_mono_mux.func    = &$cbops.mux_1to2_op;
        .VAR dual_mono_mux.param[$cbops.mux_1to2_op.STRUC_SIZE] =
            &RightPortEnabled,        // PTR_STATE_FIELD
            &left_to_right_copy,      // MAIN_TRUE_FIELD
            &dual_mono_demux;         // MAIN_FALSE_FIELD
    .ENDBLOCK;

    // Dual mono support. Resampled upstream
    .BLOCK left_to_right_copy;
        .VAR left_to_right_copy.mtu_next  = &dual_mono_mux;
        .VAR left_to_right_copy.main_next = &dual_mono_demux;
        .VAR left_to_right_copy.func = &$cbops.shift;
        .VAR left_to_right_copy.copy_param[$cbops.shift.STRUC_SIZE] =
            DACINDEX_INTERNAL_2,       // INPUT_START_INDEX_FIELD
            DACINDEX_PORT_R,           // OUTPUT_START_INDEX_FIELD
            -8;
    .ENDBLOCK;

    // Demux has only mtu part. Used as a single operator bypass
    .BLOCK dual_mono_demux;
        .VAR dual_mono_demux.mtu_next = 0; // Set by demux
        .VAR dual_mono_demux.main_next = &dac_wrap_op;
        .VAR dual_mono_demux.func    = &$cbops.demux_2to1_op;
        .VAR dual_mono_demux.param[$cbops.demux_2to1_op.STRUC_SIZE] =
            &RightPortEnabled,        // PTR_STATE_FIELD
            &left_to_right_copy,      // MTU_TRUE_FIELD
            &dual_mono_mux;           // MTU_FALSE_FIELD
    .ENDBLOCK;

    // Check DAC for wrap.  Always last operator
    .BLOCK dac_wrap_op;
        .VAR dac_wrap_op.mtu_next  = &dual_mono_demux;
        .VAR dac_wrap_op.main_next = $cbops.NO_MORE_OPERATORS;
        .VAR dac_wrap_op.func = &$cbops.port_wrap_op;
        .VAR dac_wrap_op.param[$cbops.port_wrap_op.STRUC_SIZE] =
            DACINDEX_PORT_L,     // LEFT_PORT_FIELD
            DACINDEX_PORT_R,     // RIGHT_PORT_FIELD
            3,                   // BUFFER_ADJUST_FIELD
#if uses_16kHz
            12,                  // MAX_ADVANCE_FIELD
#else
            6,                   // MAX_ADVANCE_FIELD
#endif
            1,                   // SYNC_INDEX_FIELD
            0;                   // internal : WRAP_COUNT_FIELD
    .ENDBLOCK;

   .ENDMODULE;

.MODULE $tone_in;
    .DATASEGMENT DM;

   .VAR $set_tone_rate_from_vm_message_struc[$message.STRUC_SIZE];      // Message structure for $one_mic_example.VM_SET_TONE_RATE_MESSAGE_ID message
   .VAR aux_input_stream_available;                                     // Flag indicating tones/prompts are being mixed

    // SP.  Need to add a little headroom above a frame to
    //      handle conversion ratio plus maximum fill is size-1
    DeclareCBuffer(cbuffer_struc,mem, ($BLOCK_SIZE * 2)+3 );

   .VAR copy_struc[] =
      $cbops.scratch.BufferTable,     // BUFFER_TABLE_FIELD
      &copy_op,                       // MAIN_FIRST_OPERATOR_FIELD
      &copy_op,                       // MTU_FIRST_OPERATOR_FIELD
      1,                              // NUM_INPUTS_FIELD
      $TONE_PORT,
      1,                              // NUM_OUTPUTS_FIELD
      &cbuffer_struc,
      0;                              // NUM_INTERNAL_FIELD

   .BLOCK copy_op;
      .VAR copy_op.mtu_next  = $cbops.NO_MORE_OPERATORS;
      .VAR copy_op.main_next = $cbops.NO_MORE_OPERATORS;
      .VAR copy_op.func      = $cbops_iir_resamplev2;
      .VAR copy_op.param[$iir_resamplev2.OBJECT_SIZE_SNGL_STAGE] =
         0,                                  // Input index
         1,                                  // Output index
#if uses_16kHz
         &$M.iir_resamplev2.Up_2_Down_1.filter,    // FILTER_DEFINITION_PTR_FIELD
#else
         0,
#endif
         0,                                  // INPUT_SCALE_FIELD
         8,                                  // OUTPUT_SCALE_FIELD,
         0 ...;
   .ENDBLOCK;

.ENDMODULE;

#endif

.MODULE $sco_data;
    .DATASEGMENT DM;

    DeclareCBuffer(port_in.cbuffer_struc ,mem1,3*93);          // Big enough for 2 SCO packets with headers
    DeclareCBuffer(sco_in.cbuffer_struc  ,mem2,3*90);          // Big enough for 2 SCO packets
    DeclareCBuffer(port_out.cbuffer_struc,mem3,3*90);          // Big enough for 2 SCO packets
#if uses_16kHz
    DeclareCBuffer(encoder.cbuffer_struc ,mem4,3*90);          // Big enough for 2 SCO packets
#endif

    // declare all plc memory for testing plc module
#if !uses_16kHz
    .VAR/DM sco_decoder[$sco_decoder.STRUC_SIZE] =
            $frame_sync.sco_decoder.pcm.validate,        // VALIDATE_FUNC
            $frame_sync.sco_decoder.pcm.process,         // DECODE_FUNC
            $frame_sync.sco_decoder.pcm.initialize,      // RESET_FUNC
            0,                                // DATA_PTR
            0.3;                              // THRESHOLD (between 0 and 1) //wmsn: this field is for PLC only

#if uses_PLC
   .VAR/DM1 buffer_speech[$plc100.SP_BUF_LEN_NB];
   .VAR/DM buffer_ola[$plc100.OLA_LEN_NB];
#endif

#else
    .VAR/DM sco_decoder[$sco_decoder.STRUC_SIZE] =
            $sco_decoder.wbs.validate,     // VALIDATE_FUNC
            $sco_decoder.wbs.process,      // DECODE_FUNC
            $sco_decoder.wbs.initialize,   // RESET_FUNC
            0,                             // DATA_PTR
            1.0;                           // THRESHOLD (between 0 and 1) //wmsn: this field is for PLC only

#if uses_PLC
   .VAR/DM1CIRC buffer_speech[$plc100.SP_BUF_LEN_WB];
   .VAR/DM buffer_ola[$plc100.OLA_LEN_WB];
#endif

#endif // uses_16kHz

    .VAR/DM plc_obj[$plc100.STRUC_SIZE] =
                   &buffer_speech,                    // SPEECH_BUF_START_ADDR_FIELD: 0
                   &buffer_speech,                    // SPEECH_BUF_PTR_FIELD: 1
                   &buffer_ola,                       // OLA_BUF_PTR_FIELD:     2
                   &sco_in.cbuffer_struc,             // OUTPUT_PTR_FIELD:   3

#if uses_16kHz
                   &$plc100.wb_consts,                // CONSTS_FIELD:  4
#else
                   &$plc100.nb_consts,                // CONSTS_FIELD:  4
#endif
                   $plc100.INITIAL_ATTENUATION,       // ATTENUATION_FIELD: 5
#if uses_16kHz
                   1.0,                               // PER_THRESHOLD_FIELD:  6
#else
                   0.3,                               // PER_THRESHOLD_FIELD:  6
#endif
                   0 ...;

    .VAR/DM object[$sco_pkt_handler.STRUC_SIZE ] =
                   $SCO_IN_PORT | $cbuffer.FORCE_LITTLE_ENDIAN,    // SCO PORT (header)
#if uses_16kHz
                   $SCO_IN_PORT | $cbuffer.FORCE_16BIT_DATA_STREAM,// SCO port (payload)
#else
                   $SCO_IN_PORT | $cbuffer.FORCE_PCM_AUDIO,        // SCO port (payload)
#endif
                   &port_in.cbuffer_struc,             // INPUT_PTR_FIELD
                   &sco_in.cbuffer_struc,              // OUTPUT_PTR_FIELD:   2
#if uses_PLC
                   0x2000,                             // ENABLE_FIELD
                   0x2000,                             // CONFIG_FIELD, assigned statically in example apps, but gets assigned
#else
                   0,                             // ENABLE_FIELD
                   0,                             // CONFIG_FIELD
#endif
                                                       // by CVC app's if any processing blocks are used.
                   0,                                  // STAT_LIMIT_FIELD
                   0,                                  // PACKET_IN_LEN_FIELD
                   0,                                  // PACKET_OUT_LEN_FIELD: 6
                   &sco_decoder,                       // DECODER_PTR
#if uses_PLC
                   $plc100.process,                    // PLC_PROCESS_PTR
                   $plc100.initialize,                 // PLC_RESET_PTR
#else
                   0,
                   0,
#endif
                   0,                                  // BFI_FIELD:          7
                   0,                                  // PACKET_LOSS_FIELD
                   0,                                  // INV_STAT_LIMIT_FIELD
                   0,                                  // PACKET_COUNT_FIELD
                   0,                                  // BAD_PACKET_COUNT_FIELD
#if uses_PLC
                   &plc_obj,                           // PLC_DATA_PTR_FIELD
#else
                   0,
#endif
#if uses_16kHz
                   $SCO_OUT_PORT | $cbuffer.FORCE_16BIT_DATA_STREAM,   // SCO_OUT_PORT_FIELD (payload)
                   0,                                                  // SCO_OUT_SHIFT_FIELD
#else
                   $SCO_OUT_PORT | $cbuffer.FORCE_PCM_AUDIO,           // SCO_OUT_PORT_FIELD (payload)
                   -8,                                                 // SCO_OUT_SHIFT_FIELD
#endif
                   &port_out.cbuffer_struc,                 // SCO_OUT_BUFFER_FIELD
                   30,                                      // SCO_OUT_PKTSIZE_FIELD
                   0,                                       // SCO_PARAM_TESCO_FIELD
                   0,                                       // SCO_PARAM_SLOT_LS_FIELD
                   0,                                       // SCO_NEW_PARAMS_FLAG
                   0,                                       // JITTER_PTR_FIELD
#if uses_16kHz
                   &encoder.cbuffer_struc,                  // ENCODER_BUFFER_FIELD
#else
                   0,
#endif
                   120,                                     // ENCODER_INPUT_SIZE_FIELD
                   30,                                      // ENCODER_OUTPUT_SIZE_FIELD
#if uses_16kHz
                   &$wbsenc.set_up_frame_encode,            // ENCODER_SETUP_FUNC_FIELD
                   &$wbsenc.process_frame;                  // ENCODER_PROC_FUNC_FIELD
#else
                   0,
                   0;
#endif

.ENDMODULE;

     // Scheduleing Task.   COUNT_FIELD counts modulous 12 (0-11) in 625 usec intervals

     // SCO Send on COUNT_FIELD=0, SCO Xmit on COUNT_FIELD=3,
     // SCO Good Receive on COUNT_FIELD=2, with Posibility of Re-Xmits
     // Need to accept 1 packet of latency due to re-Xmit
     //     The rcv_in jitter needs to be one sco packet

     // Send processing should be scheduled based on the peaks SEND_MIPS_FIELD (0-8000).
     //     Trigger equals 11 - round[11*(SEND_MIPS_FIELD/8000)]

#if uses_16kHz
    #define SND_PROCESS_TRIGGER     10
#else
    #define SND_PROCESS_TRIGGER     10
#endif

.MODULE $M.App.scheduler;
   .DATASEGMENT DM;

   // ** Memory Allocation for CVC config structure
    .VAR    tasks[]=
        0,                      //    COUNT_FIELD
        12,                     //    MAX_COUNT_FIELD
        (Length(tasks) - $FRM_SCHEDULER.TASKS_FIELD)/2, // NUM_TASKS_FIELD
        0,                      //    TOTAL_MIPS_FIELD
        0,                      //    SEND_MIPS_FIELD
        0,                      //    TOTALTM_FIELD
        0,                      //    TOTALSND_FIELD
        0,                      //    TIMER_FIELD
        0,                      //    TRIGGER_FIELD
        // Task List (highest to lowest priority) - Limit 23 tasks (Modulous 12)
        $main_send,              SND_PROCESS_TRIGGER,
        $main_receive,           0,
        $main_housekeeping,      1;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.Main
//
// DESCRIPTION:
//    This is the main routine.  It initializes the libraries and then loops
//    "forever" in the processing loop.
//
// *****************************************************************************
.MODULE $M.main;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   // These scratch "registers" are used by various libraries (e.g. SBC)
   .VAR $scratch.s0;
   .VAR $scratch.s1;
   .VAR $scratch.s2;

   // ** allocate memory for timer structures **
   .VAR $audio_copy_timer_struc[$timer.STRUC_SIZE];

$main:

   // initialise the stack library
   call $stack.initialise;
   // initialise the interrupt library
   call $interrupt.initialise;
   // initialise the message library
   call $message.initialise;
   // initialise the cbuffer library
   call $cbuffer.initialise;
   // initialise the pskey library
   call $pskey.initialise;

#ifdef PROFILER_ON
   // initialise the profiler library
   call $profiler.initialise;
#endif

#ifdef USB_DONGLE
#else

   r2 = $READY_TO_VM_MESSAGE_ID;
   call $message.send_short;

   r1 = &$set_adcdac_rate_message_struc;
   r2 = $SAMPLE_RATE_FROM_VM_MESSAGE_ID;
   r3 = &$sample_rate_message_handler;
   call $message.register_handler;

#endif

   // Initialize Synchronized Timing for SCO   (Requires wall clock)
   call $wall_clock.initialise;
   r8 = $sco_data.object;
   call $sco_timing.initialize;

#ifdef USB_DONGLE
#else

   r8 = &$one_mic_example.config_struct;
   call $CVC.PowerUpReset;

#endif

#if uses_16kHz

   r5 = M[&$sco_data.object + $sco_pkt_handler.DECODER_PTR];
   push r5;

   r5 = r5 + ($sco_decoder.DATA_PTR - $codec.ENCODER_DATA_OBJECT_FIELD);
   call $sbcenc.init_static_encoder;  // will also reset it

   // Setup Decoder
   // new merged SBC library needs full initialisation
   // prepare R5 pointer to encoder/decoder data structs
   pop r5;
   r5 = r5 + ($sco_decoder.DATA_PTR - $codec.DECODER_DATA_OBJECT_FIELD);
   call $sbcdec.init_static_decoder;  // also resets and silences

   // static init functions filled data object pointer field in the encoder/decoder data structs
   // set up the DATA_PTR field of sco_data.object, so that this is subsequently found by
   // used framework.

   r7 = &$sco_data.object;
   call $frame_sync.sco_initialize;

#endif

   // send message saying we're up and running!
   r2 = $MESSAGE_KALIMBA_READY;
   call $message.send_short;

#ifdef USB_DONGLE
   // register codec type message handler
   r1 = &$persistant_warp_message_struc;
   r2 = $WARP_PARAMS_FROM_VM_MESSAGE_ID;
   r3 = &$persistant_warp_message_handler;
   call $message.register_handler;
#endif

#ifdef USB_DONGLE
#else
   // Set up a message handler for the $one_mic_example.VM_SET_TONE_RATE_MESSAGE_ID message
   r1 = $set_tone_rate_from_vm_message_struc;
   r2 = $one_mic_example.VM_SET_TONE_RATE_MESSAGE_ID;
   r3 = $set_tone_rate_from_vm;
   call $message.register_handler;

   call $ConfigureFrontEnd;

#endif

   // start timer that copies audio samples
   r1 = &$audio_copy_timer_struc;
   r2 = $TIMER_PERIOD_AUDIO_COPY_MICROS;
   r3 = &$audio_copy_handler;
   call $timer.schedule_event_in;

   // Start a loop to filter the data
main_loop:
    r8 = &$M.App.scheduler.tasks;
    call $frame_sync.task_scheduler_run;
    jump main_loop;

     // ******* Houskeeping Event ********************
$main_housekeeping:
     $push_rLink_macro;
     // General system processing each frame period

     jump $pop_rLink_and_rts;

     // ********  Receive Processing ************
$main_receive:
    $push_rLink_macro;

    // Check for Initialization
    NULL = M[$one_mic_example.reinit];
    if NZ call $one_mic_example_reinitialize;

    // Run PLC and Decoder
    r7 = &$sco_data.object;
    call $frame_sync.sco_decode;

    // Get Current System Mode
    r0 = M[$one_mic_example.sys_mode];
    // Call processing table that corresponds to the current mode
    r4 = M[$M.system_config.data.receive_mode_table + r0];
    call $frame_sync.run_function_table;

    jump $pop_rLink_and_rts;

      // ***************  Send Processing **********************
$main_send:
    $push_rLink_macro;

    // Track time in process
    r0 = M[$TIMER_TIME];
    push r0;

    M[$ARITHMETIC_MODE] = NULL;

    // Check for Initialization
    NULL = M[$one_mic_example.reinit];
    if NZ call $one_mic_example_reinitialize;

    // Get Current System Mode
    r0 = M[$one_mic_example.sys_mode];
    // Call processing table that corresponds to the current mode
    r4 = M[$M.system_config.data.send_mode_table + r0];
    call $frame_sync.run_function_table;

#if uses_16kHz

    r7 = &$sco_data.object;
    r0 = M[r7 + $sco_pkt_handler.DECODER_PTR];
    r9 = M[r0 + $sco_decoder.DATA_PTR];

    call $frame_sync.sco_encode;

#endif

    // Track Time in Process
    pop r0;
    r0 = r0 - M[$TIMER_TIME];
    r1 = M[$M.App.scheduler.tasks + $FRM_SCHEDULER.TOTALSND_FIELD];
    r1 = r1 - r0;
    M[$M.App.scheduler.tasks + $FRM_SCHEDULER.TOTALSND_FIELD]=r1;

    jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.audio_copy_handler
//
// DESCRIPTION:
//    This routine copies data between firmware MMU buffers and dsp processing
//    buffers.  It is called every msec.
//
// *****************************************************************************
.MODULE $M.audio_copy_handler;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

$audio_copy_handler:

   // push rLink onto stack
   $push_rLink_macro;

   // Call operators
#ifdef USB_DONGLE
   r8 = &$usb_in.copy_struc;

#ifdef STEREO_USB_INPUT
   call $frame_sync.usb_in_stereo_audio_copy;
   // consume right channel
   r0 = &$usb_in.right.cbuffer_struc;
   call $cbuffer.get_write_address_and_size;
   r1 = r0;
   r0 = &$usb_in.right.cbuffer_struc;
   call $cbuffer.set_read_address;
#else
   call $frame_sync.usb_in_mono_audio_copy;
#endif

   r8 = &$usb_in_rm.copy_struc;
   call $cbops_multirate.copy;

   r8 = &$usb_out_rm.copy_struc;
   call $cbops_multirate.copy;

   r8 = &$usb_out.copy_struc;
   call $frame_sync.usb_out_mono_copy;

#else // USB_DONGLE

   Null = M[$one_mic_example.sr_flag];
   if NZ call $ConfigureFrontEnd;

   r8 = &$adc_in.copy_struc;
   call $cbops_multirate.copy;

   // Set up DAC operation
   call $DAC_CheckConnectivity;

   r8 = &$dac_out.copy_struc;
   call $cbops_multirate.copy;

   // Store the write pointer for tone/prompt handling
   r0 = M[$tone_in.cbuffer_struc + $cbuffer.WRITE_ADDR_FIELD];
   M[$M.detect_end_of_aux_stream.last_write_address] = r0;

   r8 = &$tone_in.copy_struc;
   call $cbops_multirate.copy;

   // Check for Volume Change due to Auxillary Audio Play
   call $CVC.VolumeControl.Check_Aux_Volume;

   // Detect the end of a tone/prompt stream
   call $detect_end_of_aux_stream;
#endif

irq_sco:

   // SP - SCO synchronization may reset frame counter
   r7 = &$M.App.scheduler.tasks + $FRM_SCHEDULER.COUNT_FIELD;
   r1 = &$audio_copy_timer_struc;
   r3 = &$audio_copy_handler;
   call $sco_timing.SyncClock;

   r8 = &$M.App.scheduler.tasks;
   call $frame_sync.task_scheduler_isr;

   // pop rLink from stack
   jump $pop_rLink_and_rts;

.ENDMODULE;

#if !defined(USB_DONGLE)
// *****************************************************************************
// MODULE:
//    $set_tone_rate_from_vm
//
// DESCRIPTION: message handler for receiving tone rate from VM
//    (note: run-time tone rate and mono/stereo configuration is
//    not currently supported in this app)
//
// INPUTS:
//  r1 = (tone rate)
//  r2 = bit 0: mono / stereo
//       bit 1: tone / prompt
//  r3 = Not used
//  r4 = Not used
//
// OUTPUTS:
//    none
// *****************************************************************************
.MODULE $M.set_tone_rate_from_vm;
   .CODESEGMENT PM;

   $set_tone_rate_from_vm:

   $push_rLink_macro;

   // clear tone buffer
   r0 = $tone_in.cbuffer_struc;
   call $cbuffer.empty_buffer;

   // auxiliary input expected now
   r0 = 1;
   M[$tone_in.aux_input_stream_available] = r0;

   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $detect_end_of_aux_stream
//
// DESCRIPTION:
//    detects end of pcm tone/prompts and notifies vm
//
// INPUTS:
//    none
//
// OUTPUTS:
//    none
// *****************************************************************************

.MODULE $M.detect_end_of_aux_stream;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   .VAR last_write_address = $tone_in.mem;
   .VAR write_move_counter = 0;

   $detect_end_of_aux_stream:

   $push_rLink_macro;

   // detect end of tone/prompt auxiliary pcm input stream
   r3 = M[$tone_in.aux_input_stream_available];
   if Z jump $pop_rLink_and_rts;

   // see if the input is active
   r0 = $tone_in.cbuffer_struc;
   call $cbuffer.calc_amount_data;

   // check if input activity has been seen before
   Null = r3 AND 0x2;
   if NZ jump input_has_received;

   // input hasn't started yet, so no end check
   Null = r0;
   if Z jump $pop_rLink_and_rts;

   // input just received
   r3 = r3 OR 0x2;
   M[$tone_in.aux_input_stream_available] = r3;
   jump $pop_rLink_and_rts;

   input_has_received:

   // Get the write address for the output buffer
   r0 = $tone_in.cbuffer_struc;
   call $cbuffer.get_write_address_and_size;
   r4 = M[last_write_address];
   r3 = M[write_move_counter];
   r5 = 0;

   Null = r0 - r4;
   if Z r5 = r3 + 1;
   M[write_move_counter] = r5;

   Null = r5 - $one_mic_example.$PCM_END_DETECTION_TIME_OUT;
   if NEG jump $pop_rLink_and_rts;

   // inactive more than a threshold
   // notify VM about end of play back of aux input
   r2 = $one_mic_example.PLAY_BACK_FINISHED_MSG;
   r3 = 0;
   r4 = 0;
   r5 = 0;
   r6 = 0;
   call $message.send_short;
   M[$tone_in.aux_input_stream_available] = 0;
   M[write_move_counter] = 0;

   // Purge tone buffers
   r0 = M[$tone_in.cbuffer_struc + $cbuffer.WRITE_ADDR_FIELD];
   M[$tone_in.cbuffer_struc + $cbuffer.READ_ADDR_FIELD] = r0;

   // pop rLink from stack
   jump $pop_rLink_and_rts;

.ENDMODULE;
#endif
