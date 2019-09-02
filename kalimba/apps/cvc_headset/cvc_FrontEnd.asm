// *****************************************************************************
// Copyright (c) 2007 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.0
//
// *****************************************************************************

// *****************************************************************************
// DESCRIPTION
//    This file defines the front end prcoessing for the CVC 1-MIC Headset
//
// External References:
//      &$M.CVC.data.CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_ST_CLIP_POINT
//      &$M.CVC.data.ndvc_dm1 + $M.NDVC_Alg1_0_0.OFFSET_FILTSUMLPDNZ
//      &$M.CVC.data.ZeroValue
//      &$M.CVC.data.CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_ST_PEQ_CONFIG
//      &$M.CVC.data.stream_map_sndin + $framesync_ind.DROP_INSERT_FIELD
//
//      &$M.CVC.data.stream_map_refin  + $framesync_ind.FRAME_SIZE_FIELD
//      &$M.CVC.data.stream_map_sndin  + $framesync_ind.FRAME_SIZE_FIELD
//      &$M.CVC.data.stream_map_rcvout + $framesync_ind.FRAME_SIZE_FIELD
//      &$M.CVC.data.stream_map_rcvout + $framesync_ind.JITTER_FIELD
//      &$M.CVC.data.stream_map_refin  + $framesync_ind.JITTER_FIELD
//      &$M.CVC.data.stream_map_sndin  + $framesync_ind.JITTER_FIELD
//
// *****************************************************************************
#include "../common/ports.h"
#include "core_library.h"
#include "cbops_multirate_library.h"
#include "frame_sync_stream_macros.h"
#include "frame_sync_library.h"
#include "cvc_modules.h"
#include "cvc_headset.h"

.CONST  $ADC_PORT             (($cbuffer.READ_PORT_MASK  + 0) | $cbuffer.FORCE_PCM_AUDIO);
.CONST  $DAC_PORT_L           (($cbuffer.WRITE_PORT_MASK + $PRIMARY_LEFT_OUT_PORT_NUMBER) | $cbuffer.FORCE_PCM_AUDIO);
.CONST  $DAC_PORT_R           (($cbuffer.WRITE_PORT_MASK + $PRIMARY_RIGHT_OUT_PORT_NUMBER) | $cbuffer.FORCE_PCM_AUDIO);
.CONST  $TONE_PORT            (($cbuffer.READ_PORT_MASK  + 3) | $cbuffer.FORCE_PCM_AUDIO);

#define $TONE_EXTRA_BOOST_BITS        2
#define $PROMPT_EXTRA_BOOST_BITS     (-1)
#define $PCM_END_DETECTION_TIME_OUT   40
#define PLAY_BACK_FINISHED_MSG        0x1080

// --------------------------------------------------------------------------
// INPUT/OUTPUT streams
// --------------------------------------------------------------------------
.MODULE $cbops.scratch;
   .DATASEGMENT DM;

    // if NUM_INPUTS_FIELD + NUM_OUTPUTS_FIELD + NUM_INTERNAL_FIELD > 6,increase
    // size of this table
    .VAR BufferTable[8*$cbops_multirate.BufferTable.ENTRY_SIZE];
    // scratch sufficient for 192kHz sample rate @ 625 usec
    // scratch 1 for resampler temp
    DeclareCBuffer(cbuffer_struc1,mem1,240);
    // scratch 2,3 for changing consumed/produced
    DeclareCBuffer(cbuffer_struc2,mem2,240);
    DeclareCBuffer(cbuffer_struc3,mem3,240);

.ENDMODULE;

#define ADCINDEX_PORT           0
#define ADCINDEX_CBUFFER        1
#define ADCINDEX_SIDETONE       2
#define ADCINDEX_INTERNAL       3
#define ADCINDEX_NONE          -1

#define   SIDETONE_PEQ_STAGES   3

.MODULE $adc_in;
   .DATASEGMENT DM;

//                   (internal)               (cBuffer)
//   PORT ---- (RESAMPLE) ----- RATEMATCH ---+---->
//                                         |
//                                       SIDETONE
//                                         |
//                                      (ST cBuffer)
//

    DeclareCBuffer (cbuffer_struc,mem,$BLOCK_SIZE_ADC_DAC * 2);
    DeclareCBuffer (sidetone_cbuffer_struc,sidetone_mem,(4*$SAMPLE_RATE_ADC_DAC)/1000);

    .VAR/DM1CIRC    sr_hist[$cbops.rate_adjustment_and_shift.SRA_COEFFS_SIZE];

   .VAR copy_struc[] =
      $cbops.scratch.BufferTable,     // BUFFER_TABLE_FIELD
      &copy_op,                       // MAIN_FIRST_OPERATOR_FIELD
      &sidetone_copy_op,              // MTU_FIRST_OPERATOR_FIELD
      1,                              // NUM_INPUTS_FIELD
      $ADC_PORT,                      // port->scratch2 (resample)
      2,                              // NUM_OUTPUTS_FIELD
      &cbuffer_struc,                 // scratch2->cbuffer (rate adjust->cbuffer)
      &sidetone_cbuffer_struc,        // cbuffer->sidetone cbuffer
      1,                              // NUM_INTERNAL_FIELD
      $cbops.scratch.cbuffer_struc2;  // from port


    // Port Copy/Resampler Operator
    .BLOCK copy_op;
        .VAR copy_op.mtu_next  = $cbops.NO_MORE_OPERATORS;
        .VAR copy_op.main_next = &rate_match_mux;
        .VAR copy_op.func = $cbops_iir_resamplev2;
        .VAR copy_op.param[$iir_resamplev2.OBJECT_SIZE] =
            ADCINDEX_PORT,                      // Input index
            ADCINDEX_INTERNAL,                  // Output index
            0,                                  // FILTER_DEFINITION_PTR_FIELD  [---CONFIG---]
            0,                                  // INPUT_SCALE_FIELD   (input Q15)
            8,                                  // OUTPUT_SCALE_FIELD  (output Q23)
            &$cbops.scratch.mem1,               // INTERMEDIATE_CBUF_PTR_FIELD
            LENGTH($cbops.scratch.mem1),        // INTERMEDIATE_CBUF_LEN_FIELD
            0 ...;
    .ENDBLOCK;

    // Mux has only main part
    .BLOCK rate_match_mux;
        .VAR rate_match_mux.mtu_next = &copy_op;
        .VAR rate_match_mux.main_next = 0; // Set by mux
        .VAR rate_match_mux.func    = &$cbops.mux_1to2_op;
        .VAR rate_match_mux.param[$cbops.mux_1to2_op.STRUC_SIZE] =
            &$sw_ratematching_adc,    // PTR_STATE_FIELD
            &sw_rate_op,              // MAIN_TRUE_FIELD
            &hw_copy_op;              // MAIN_FALSE_FIELD
    .ENDBLOCK;

    // Unlike USB rate monitor in the backend, this monitor uses a smaller
    // "seconds tracked window" and a signficantly smaller "alpha limit" value
    // The combination of these two values allows the rate adjustment to react
    // faster and more accurately
    .BLOCK sw_rate_op;
        .VAR sw_rate_op.mtu_next  = &rate_match_mux;
        .VAR sw_rate_op.main_next = &sw_copy_op;
        .VAR sw_rate_op.func = &$cbops.rate_monitor_op;
        .VAR sw_rate_op.param[$cbops.rate_monitor_op.STRUC_SIZE] =
            ADCINDEX_INTERNAL,          // MONITOR_INDEX_FIELD
            1600,                       // PERIODS_PER_SECOND_FIELD
            5,                          // SECONDS_TRACKED_FIELD
            0,                          // TARGET_RATE_FIELD    [---CONFIG---]
            2,                          // ALPHA_LIMIT_FIELD (controls the size of the averaging window)
            0.5,                        // AVERAGE_IO_RATIO_FIELD - initialize to 1.0 in q.22
            11,                         // WARP_MSG_LIMIT_FIELD
            120,                        // IDLE_PERIODS_AFTER_STALL_FIELD (empirical)
            0 ...;  
    .ENDBLOCK;

    .BLOCK sw_copy_op;
        .VAR sw_copy_op.mtu_next  = &sw_rate_op;
        .VAR sw_copy_op.main_next = &rate_match_demux;
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

    // Hardware Rate matching uses copy_op to simplify routing
    //  Otherwise output of copy_op and input to sidetone would need to change
    .BLOCK hw_copy_op;
        .VAR hw_copy_op.mtu_next  = &rate_match_mux;
        .VAR hw_copy_op.main_next = &hw_rate_op;
        .VAR hw_copy_op.func = &$cbops.copy_op;
        .VAR hw_copy_op.param[$cbops.copy_op.STRUC_SIZE] =
            ADCINDEX_INTERNAL,           // INPUT_START_INDEX_FIELD
            ADCINDEX_CBUFFER;            // OUTPUT_START_INDEX_FIELD
    .ENDBLOCK;

    // Hardware Rate matching monitors cBuffer
    .BLOCK hw_rate_op;
        .VAR hw_rate_op.mtu_next  = &hw_copy_op;
        .VAR hw_rate_op.main_next = &rate_match_demux;
        .VAR hw_rate_op.func = &$cbops.hw_warp_op;
        .VAR hw_rate_op.param[$cbops.hw_warp_op.STRUC_SIZE] =
            $ADC_PORT,         // PORT_OFFSET - Tracks Connectivity
            ADCINDEX_INTERNAL, // MONITOR_INDEX_OFFSET - Monitors throughput
            0x3F,              // WHICH_PORTS_OFFSET
            0,                 // TARGET_RATE_OFFSET    [---CONFIG---]
            1600,              // PERIODS_PER_SECOND_OFFSET
            3,                 // COLLECT_SECONDS_OFFSET
            1,                 // ENABLE_DITHER_OFFSET  [---CONFIG---]
            0 ...;
    .ENDBLOCK;

    // Demux has only mtu part
    .BLOCK rate_match_demux;
        .VAR rate_match_demux.mtu_next = 0; // Set by demux
        .VAR rate_match_demux.main_next = &sidetone_copy_op;
        .VAR rate_match_demux.func    = &$cbops.demux_2to1_op;
        .VAR rate_match_demux.param[$cbops.demux_2to1_op.STRUC_SIZE] =
            &$sw_ratematching_adc,     // PTR_STATE_FIELD
            &sw_copy_op,               // MTU_TRUE_FIELD
            &hw_rate_op;               // MTU_FALSE_FIELD
    .ENDBLOCK;

        // sidetone PEQ
    .BLOCK sidetone_copy_op;
        .VAR sidetone_copy_op.mtu_next  = &rate_match_demux;
        .VAR sidetone_copy_op.main_next = $cbops.NO_MORE_OPERATORS;
        .VAR sidetone_copy_op.func = &$cbops.sidetone_filter_op;
        .VAR sidetone_copy_op.param[CBOPS_SIDETONE_FILTER_OBJECT_SIZE(SIDETONE_PEQ_STAGES)] =
            ADCINDEX_CBUFFER,                   // Input index field
            ADCINDEX_SIDETONE,                  // Output index field
            $M.CVC_HEADSET.CONFIG.SIDETONEENA,  // SideTone Enable Mask
            &$M.CVC.data.CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_ST_CLIP_POINT, // Pointer to Sidetone Parameters
            0,                                  // Apply Filter-Filter
            0.0,                                // APPLY_GAIN
            // noise switching is never enabled by UFE
#if uses_NSVOLUME                               // NOISE_LEVEL_PTR_FIELD
            &$M.CVC.data.ndvc_dm1 + $M.NDVC_Alg1_0_0.OFFSET_FILTSUMLPDNZ,
#else
            &$M.CVC.data.ZeroValue,
#endif
            0,                                  // Config
            &$dac_out.auxillary_mix_op.param+$cbops.aux_audio_mix_op.OFFSET_INV_DAC_GAIN,// OFFSET_PTR_INV_DAC_GAIN
            0,                                  // OFFSET_CURRENT_SIDETONE_GAIN
            0,                                  // OFFSET_PTR_PEAK_ST

            // Below starts sidetone PEQ fields
            0,                                  // PTR_INPUT_DATA_BUFF_FIELD  - Not Used
            0,                                  // PTR_OUTPUT_DATA_BUFF_FIELD - Not Used
            SIDETONE_PEQ_STAGES,                // MAX_STAGES_FIELD
            &$M.CVC.data.CurParams + $M.CVC_HEADSET.PARAMETERS.OFFSET_ST_PEQ_CONFIG,       // Pointer to PEQ parameters
            0 ...;
    .ENDBLOCK;

.ENDMODULE;

/************************** Data Structure***************************************/

#define DACINDEX_CBUFFER        0
#define DACINDEX_SIDETONE       1
#define DACINDEX_PORT_LEFT      2
#define DACINDEX_PORT_RIGHT     3
#define DACINDEX_REFERENCE      4
#define DACINDEX_INTERNAL_1     5
#define DACINDEX_INTERNAL_2     6
#define DACINDEX_NONE          -1

.MODULE $dac_out;
   .DATASEGMENT DM;

//
//          (ST cBuffer)                                   (Ref cBuffer)
//          ----------> SIDETONE MIX  ---+   +-- REFERENCE --->
//                                       |   |
//                 (internal)            |   |                      (DAC cBuffer)
// PORT <--- RESAMPLE ----- RATEMATCH ---+---+----+------ INSERTION -----<
//                                                |
//                                              TONE MIX
//                                                |
//                                             (Aux cBuffer)
//
    .VAR RightPortEnabled = 0;
    .VAR spkr_out_pk_dtct = 0;
    .VAR/DM1CIRC    sr_hist[$cbops.rate_adjustment_and_shift.SRA_COEFFS_SIZE];

    DeclareCBuffer (cbuffer_struc,mem,$BLOCK_SIZE_ADC_DAC * 2);
    DeclareCBuffer (reference_cbuffer_struc,reference_mem,$BLOCK_SIZE_ADC_DAC * 2);

    .VAR copy_struc[] =
        $cbops.scratch.BufferTable,     // BUFFER_TABLE_FIELD
        &insert_op,                     // MAIN_FIRST_OPERATOR_FIELD
        &insert_op,                     // MTU_FIRST_OPERATOR_FIELD
        2,                              // NUM_INPUTS_FIELD
        &cbuffer_struc,                 // buffer 1, buffer 1-> TONE PORT
        &$adc_in.sidetone_cbuffer_struc,// sidetone buffer -> buffer 1
        3,                              // NUM_OUTPUTS_FIELD
        $DAC_PORT_L,                    // scratch 2 (RM)-> PORT L
        $DAC_PORT_R,                    // scratch 2 (RM)-> PORT R
        reference_cbuffer_struc,        // buffer 1 -> reference
        2,                              // NUM_INTERNAL_FIELD
        $cbops.scratch.cbuffer_struc2,  // buffer 1 -> scratch 1 (resample)
        $cbops.scratch.cbuffer_struc3;  // scratch 1 (resample) -> scratch 2 (RM)

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
        .VAR auxillary_mix_op.main_next = &reference_copy_op;
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
            0,                               /* Absolute Clip Point  (Q23)*/
            0x40000,                         /* Boost (Q4.19)*/
            0,                               /* Auxillary Audio Peak Statistic */
            1.0,                             /* Inverse gain difference between Main & Tone Volume (Q23) */
            0;                               /* Internal Data */
    .ENDBLOCK;

    // Tap for AEC reference
    .BLOCK reference_copy_op;
        .VAR reference_copy_op.mtu_next = &auxillary_mix_op;
        .VAR reference_copy_op.main_next = &sidetone_mix_op;
        .VAR reference_copy_op.func = &$cbops.shift;
        .VAR reference_copy_op.param[$cbops.shift.STRUC_SIZE] =
             DACINDEX_CBUFFER,     // Input index
             DACINDEX_REFERENCE,   // Output index
             0;                    // SHIFT_AMOUNT_FIELD
    .ENDBLOCK;

    // Mix in Sidetone
    .BLOCK sidetone_mix_op;
        .VAR sidetone_mix_op.mtu_next  = &reference_copy_op;
        .VAR sidetone_mix_op.main_next = &resample_op;
        .VAR sidetone_mix_op.func = &$cbops.sidetone_mix_op;
        .VAR sidetone_mix_op.param[$cbops.sidetone_mix_op.STRUC_SIZE] =
            DACINDEX_CBUFFER,       // INPUT_START_INDEX_FIELD
            DACINDEX_CBUFFER,       // OUTPUT_START_INDEX_FIELD
            DACINDEX_SIDETONE,      // SIDETONE_START_INDEX_FIELD
            0,                      // SIDETONE_MAX_SAMPLES_FIELD     [---CONFIG---]
            0;                      // ATTENUATION_PTR_FIELD
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
        .VAR audio_out_volume.main_next = &pk_out_dac;
        .VAR audio_out_volume.func = &$cbops.volume;
        .VAR audio_out_volume.param[$cbops.volume.STRUC_SIZE] =
            DACINDEX_INTERNAL_1,                // INPUT_START_INDEX_FIELD
            DACINDEX_INTERNAL_1,                // OUTPUT_START_INDEX_FIELD
            1.0,                             // Final Gain Value
            0,                               // Current Gain Value
            0,                               // samples per step,
            -5,                              // step shift field
            0.01,                            // delta field
            0;                               // current step field
      .ENDBLOCK;

    .BLOCK pk_out_dac;
        .VAR pk_out_dac.mtu_next = &audio_out_volume;
        .VAR pk_out_dac.main_next= &rate_match_mux;
        .VAR pk_out_dac.func    = &$cbops.peak_monitor_op;
        .VAR pk_out_dac.param[$cbops.peak_monitor_op.STRUC_SIZE] =
            DACINDEX_INTERNAL_1,  // PTR_INPUT_BUFFER_FIELD
            &spkr_out_pk_dtct;    // PEAK_LEVEL_PTR - UFE stat
    .ENDBLOCK;

    // Mux has only main part
    .BLOCK rate_match_mux;
        .VAR rate_match_mux.mtu_next = &pk_out_dac;
        .VAR rate_match_mux.main_next = 0; // Set by mux
        .VAR rate_match_mux.func    = &$cbops.mux_1to2_op;
        .VAR rate_match_mux.param[$cbops.mux_1to2_op.STRUC_SIZE] =
            &$sw_ratematching_dac,    // PTR_STATE_FIELD
            &sw_rate_adjust_op,       // MAIN_TRUE_FIELD
            &hw_copy_op;              // MAIN_FALSE_FIELD
    .ENDBLOCK;

    // Apply rate adjustment in software
    .BLOCK sw_rate_adjust_op;
        .VAR sw_rate_adjust_op.mtu_next  = &rate_match_mux;
        .VAR sw_rate_adjust_op.main_next = &sw_rate_monitor_op;
        .VAR sw_rate_adjust_op.func = &$cbops.rate_adjustment_and_shift;
        .VAR sw_rate_adjust_op.param[$cbops.rate_adjustment_and_shift.STRUC_SIZE] =
            DACINDEX_INTERNAL_1,        // INPUT1_START_INDEX_FIELD
            DACINDEX_INTERNAL_2,        // OUTPUT1_START_INDEX_FIELD
            0,                          // SHIFT_AMOUNT_FIELD
            0,                          // MASTER_OP_FIELD
            &$sra_coeffs,               // FILTER_COEFFS_FIELD
            &sr_hist,                   // HIST1_BUF_FIELD
            &sr_hist,                   // HIST1_BUF_START_FIELD
            &sw_rate_monitor_op.param + $cbops.rate_monitor_op.INVERSE_WARP_VALUE_FIELD, // SRA_TARGET_RATE_ADDR_FIELD
            0,                          // ENABLE_COMPRESSOR_FIELD
            0 ...;
    .ENDBLOCK;
    
    // Software rate monitor dac chain independently. We mirror the same params
    // as the monitor on the ADC chain to keep the behavior identical irrespective
    // of the external interface 
    .BLOCK sw_rate_monitor_op;
        .VAR sw_rate_monitor_op.mtu_next  = &sw_rate_adjust_op;
        .VAR sw_rate_monitor_op.main_next = &rate_match_demux;
        .VAR sw_rate_monitor_op.func = &$cbops.rate_monitor_op;
        .VAR sw_rate_monitor_op.param[$cbops.rate_monitor_op.STRUC_SIZE] =
            DACINDEX_INTERNAL_2,        // MONITOR_INDEX_FIELD
            1600,                       // PERIODS_PER_SECOND_FIELD
            5,                          // SECONDS_TRACKED_FIELD
            0,                          // TARGET_RATE_FIELD    [---CONFIG---]
            2,                          // ALPHA_LIMIT_FIELD (controls the size of the averaging window)
            0.5,                        // AVERAGE_IO_RATIO_FIELD - initialize to 1.0 in q.22
            11,                         // WARP_MSG_LIMIT_FIELD
            120,                        // IDLE_PERIODS_AFTER_STALL_FIELD (empirical)
            0 ...;
    .ENDBLOCK;

    // Keep mtu and main paths of mux/demux in symmetry
    .BLOCK hw_copy_op;
        .VAR hw_copy_op.mtu_next  = &rate_match_mux; 
        .VAR hw_copy_op.main_next = &hw_copy_op2; 
        .VAR hw_copy_op.func = &$cbops.copy_op;
        .VAR hw_copy_op.param[$cbops.copy_op.STRUC_SIZE] =
            DACINDEX_INTERNAL_1,  // INPUT_START_INDEX_FIELD
            DACINDEX_INTERNAL_2;  // OUTPUT_START_INDEX_FIELD 
    .ENDBLOCK; 
    
    // Hardware rate monitor and send message dac chain independently 
    .BLOCK hw_copy_op2;
        .VAR hw_copy_op2.mtu_next  = &hw_copy_op;
        .VAR hw_copy_op2.main_next = &rate_match_demux;
        .VAR hw_copy_op2.func = &$cbops.copy_op;
        .VAR hw_copy_op2.param[$cbops.copy_op.STRUC_SIZE] =
            DACINDEX_INTERNAL_2,  // INPUT_START_INDEX_FIELD
            DACINDEX_INTERNAL_2;  // OUTPUT_START_INDEX_FIELD 
    .ENDBLOCK;  
    
    // Demux has only mtu part
    .BLOCK rate_match_demux;
        .VAR rate_match_demux.mtu_next = 0; // Set by demux
        .VAR rate_match_demux.main_next = &left_copy_op;
        .VAR rate_match_demux.func    = &$cbops.demux_2to1_op;
        .VAR rate_match_demux.param[$cbops.demux_2to1_op.STRUC_SIZE] =
            &$sw_ratematching_dac,     // PTR_STATE_FIELD
            &sw_rate_monitor_op,       // MTU_TRUE_FIELD 
            &hw_copy_op2;              // MTU_FALSE_FIELD
    .ENDBLOCK;

    // Using shift here instead of copy. Not shifting in rate adjust and shift
    .BLOCK left_copy_op;
        .VAR left_copy_op.mtu_next  = &rate_match_demux;
        .VAR left_copy_op.main_next = &dual_mono_mux;
        .VAR left_copy_op.func = &$cbops.shift;
        .VAR left_copy_op.param[$cbops.shift.STRUC_SIZE] =
            DACINDEX_INTERNAL_2,  // INPUT_START_INDEX_FIELD
            DACINDEX_PORT_LEFT,   // OUTPUT_START_INDEX_FIELD
            -8;
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
            DACINDEX_PORT_RIGHT,       // OUTPUT_START_INDEX_FIELD
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

    // Check DAC for wrap.  Always last operator. This operator is needed to keep
    // the DAC MMU buffer trimmed to within 2 periods. It is needed for preventing
    // large latency swings between the DAC and ADC buffers needed for AEC's
    .BLOCK dac_wrap_op;
        .VAR dac_wrap_op.mtu_next  = &dual_mono_demux;
        .VAR dac_wrap_op.main_next = $cbops.NO_MORE_OPERATORS;
        .VAR dac_wrap_op.func = &$cbops.port_wrap_op;
        .VAR dac_wrap_op.param[$cbops.port_wrap_op.STRUC_SIZE] =
            DACINDEX_PORT_LEFT, // LEFT_PORT_FIELD
            DACINDEX_PORT_RIGHT,// RIGHT_PORT_FIELD
            3,                  // BUFFER_ADJUST_FIELD
            0,                  // MAX_ADVANCE_FIELD         [---CONFIG---]
            1,                  // SYNC_INDEX_FIELD
            0;                  // internal : WRAP_COUNT_FIELD
    .ENDBLOCK;

.ENDMODULE;


.MODULE $tone_in;
    .DATASEGMENT DM;
    // Need to add a little headroom above a frame to
    //      handle conversion ratio plus maximum fill is size-1
    DeclareCBuffer(cbuffer_struc,mem, $BLOCK_SIZE_ADC_DAC+3 );
    
   .VAR aux_input_stream_available;    // local pcm file is being mixed

   .VAR copy_struc[] =
      $cbops.scratch.BufferTable,      // BUFFER_TABLE_FIELD
      deinterleave_mix_op,             // MAIN_FIRST_OPERATOR_FIELD
      copy_op,                         // MTU_FIRST_OPERATOR_FIELD
      1,                               // NUM_INPUTS_FIELD
      $TONE_PORT,
      1,                               // NUM_OUTPUTS_FIELD
      &cbuffer_struc,
      1,                               // NUM_INTERNAL_FIELD
      $cbops.scratch.cbuffer_struc2;

    // for mono tone/voice prompt this is just a copy operator
    // for stereo voice prompts this operator extracts L and R voice
    // prompt samples from interleaved input stream and then mixes
    // them into one mono stream
   .BLOCK deinterleave_mix_op;
      .VAR deinterleave_mix_op.mtu_next  = $cbops.NO_MORE_OPERATORS;
      .VAR deinterleave_mix_op.main_next = copy_op;
      .VAR deinterleave_mix_op.func = $cbops.deinterleave_mix;
      .VAR deinterleave_mix_op.param[$cbops.deinterleave_mix.STRUC_SIZE] =
         0,                                  // Input index
         2,                                  // Output1 index
        -1,                                  // Output2 index
         0 ;                                 // de-interleave enabled flag
   .ENDBLOCK;

   .BLOCK copy_op;
      .VAR copy_op.mtu_next  = deinterleave_mix_op;
      .VAR copy_op.main_next = $cbops.NO_MORE_OPERATORS;
      .VAR copy_op.func = $cbops_iir_resamplev2;
      .VAR copy_op.param[$iir_resamplev2.OBJECT_SIZE_SNGL_STAGE] =
         2,                                  // Input index
         1,                                  // Output index
         &$M.iir_resamplev2.Up_2_Down_1.filter,    // FILTER_DEFINITION_PTR_FIELD
         0,                                  // INPUT_SCALE_FIELD
         8,                                  // OUTPUT_SCALE_FIELD,
         &$cbops.scratch.mem1,               // INTERMEDIATE_CBUF_PTR_FIELD
         LENGTH($cbops.scratch.mem1),        // INTERMEDIATE_CBUF_LEN_FIELD
         0 ...;
   .ENDBLOCK;

.ENDMODULE;

// Rate match enable masks
.CONST $RATE_MATCH_DISABLE_MASK              0x0000;
.CONST $HW_RATE_MATCH_ENABLE_MASK            0x0001;
.CONST $SW_RATE_MATCH_ENABLE_MASK            0x0002;
.CONST $SW_RATE_MATCH_MASK                   0x0003;

// Mask for interface mode
.CONST $AUDIO_IF_MASK                        (0x00ff);       // Mask to select the audio i/f info

.CONST $ADCDAC_PERIOD_USEC                  0.000625;

#ifndef BUILD_MULTI_KAPS
.MODULE $M.FrontEnd;
   .CODESEGMENT PM_FLASH;
   .DATASEGMENT DM;
#else
.MODULE $M.FrontEnd;
    .CODESEGMENT PM;
    .DATASEGMENT DM;
#endif

   .VAR $sw_ratematching_adc = 0;
   .VAR $sw_ratematching_dac = 0;

   .VAR frame_adc_sampling_rate=8000;
   .VAR frame_dac_sampling_rate=8000;
   .VAR current_tone_sampling_rate=8000;

// *****************************************************************************
// MODULE:
//    IdentifyFilter:
//
// DESCRIPTION:
//    Search resampler list for filter descriptor.
//    List is NULL terminated with the descriptors for targets of 8kHz and 16kHz
//
// INPUTS:
//    r8 = Search Table (cvc_rcvout_resampler_lookup or cvc_sndin_resampler_lookup)
//    r7 = Sample Rate
//    r6 = Target Sample Rate
//
// OUTPUTS:
//    r0 = Descriptor, or NULL
// *****************************************************************************
    .VAR cvc_rcvout_resampler_lookup[] =
        // Rate     ,     8kHz-->Rate,                          16kHz-->Rate
        32000,        $M.iir_resamplev2.Up_4_Down_1.filter,     $M.iir_resamplev2.Up_2_Down_1.filter,
        44100,        $M.iir_resamplev2.Up_441_Down_80.filter,  $M.iir_resamplev2.Up_441_Down_160.filter,
        48000,        $M.iir_resamplev2.Up_6_Down_1.filter,     $M.iir_resamplev2.Up_3_Down_1.filter,
        96000,        $M.iir_resamplev2.Up_12_Down_1.filter,    $M.iir_resamplev2.Up_6_Down_1.filter,
        192000,       $M.iir_resamplev2.Up_24_Down_1.filter,    $M.iir_resamplev2.Up_12_Down_1.filter,
        0;

    .VAR cvc_sndin_resampler_lookup[] =
        // Rate     ,     Rate-->8Kz,                            Rate-->16kHz
        32000,        $M.iir_resamplev2.Up_1_Down_4.filter,     $M.iir_resamplev2.Up_1_Down_2.filter,
        44100,        $M.iir_resamplev2.Up_80_Down_441.filter,  $M.iir_resamplev2.Up_160_Down_441.filter,
        48000,        $M.iir_resamplev2.Up_1_Down_6.filter,     $M.iir_resamplev2.Up_1_Down_3.filter,
        96000,        $M.iir_resamplev2.Up_1_Down_12.filter,    $M.iir_resamplev2.Up_1_Down_6.filter,
        8000,         0,                                        $M.iir_resamplev2.Up_2_Down_1.filter,
        16000,        $M.iir_resamplev2.Up_1_Down_2.filter,     0,
        0;

IdentifyFilter:
    r6 = r6 - 15999;
    if NEG r6 = NULL;
    // Target Sample Rate (r5)  : (0) 8kHz or (1) 16kHz
jp_identify:
    r0 = M[r8];
    if Z rts;
    r8 = r8 + 3;
    NULL = r0 - r7;
    if NZ jump jp_identify;
    r8 = r8 - 2;
    r0 = M[r8 + r6];
    rts;
// *****************************************************************************
// MODULE:
//    $ConfigureFrontEnd
//
// DESCRIPTION:
//    Set up the system Front End
//
// INPUTS:
//    none
// OUTPUTS:
//    none
// *****************************************************************************
$ConfigureFrontEnd:
    $push_rLink_macro;

    // Get CVC variant
    r9 = M[$M.CVC_SYS.cvc_bandwidth];

    // Setup sample rates based on variant
    r1 = 8000;      // 8  kHz
    r2 = r1 + r1;   // 16 kHz
    Null = r9 - $M.CVC.BANDWIDTH.NB;
    if NZ r1=r2;
    // SP.  for 1-mic ADC/DAC into frame process always the same sample rate
    //      This is not the case for 2-mic
    M[frame_adc_sampling_rate]=r1;
    M[frame_dac_sampling_rate]=r1;

    // Setup Aux Tone sample rate 8kHz or 16kHz
    r0 = &$M.iir_resamplev2.Up_2_Down_1.filter;
    NULL = r1 - r2;
    if NZ r0=NULL;
    r8 = &$tone_in.copy_op.param;
    call $iir_resamplev2.SetFilter;

    // FE/BEX functions
    r1 = &$frame.iir_resamplev2.Process;
    Null = r9 - $M.CVC.BANDWIDTH.FE;
    if NZ r1=NULL;
    M[$fe_frame_resample_process] = r1;

    // Set Front end Frame Size
    r0 = 60;
    NULL = r9 - $M.CVC.BANDWIDTH.NB;
    if NZ r0 = r0 + r0;
    M[&$M.CVC.data.stream_map_refin  + $framesync_ind.FRAME_SIZE_FIELD] = r0;
    M[&$M.CVC.data.stream_map_sndin  + $framesync_ind.FRAME_SIZE_FIELD] = r0;
    M[&$M.CVC.data.stream_map_rcvout + $framesync_ind.FRAME_SIZE_FIELD] = r0;

    // Setup Rate Matching
    r0 = M[$M.audio_config.audio_rate_matching];
    r0 = r0 AND $SW_RATE_MATCH_ENABLE_MASK;
    M[$sw_ratematching_dac] = r0;
    M[$sw_ratematching_adc] = r0;

    // ---- Setup ADC -----

    // Setup ADC sampling Rate
    r6 = M[frame_adc_sampling_rate];  // target rate (only 8kHz or 16kHz)
    r7 = M[$M.audio_config.adc_sampling_rate]; // I2S or DAC rate
    r8 = &cvc_sndin_resampler_lookup;
    call IdentifyFilter;

    // r0 = filter descriptor or NULL (passthrough)
    r8 = &$adc_in.copy_op.param;
    call $iir_resamplev2.SetFilter;

    r1 = M[frame_adc_sampling_rate];
    r3 = 10;
    r8 = &$adc_in.hw_rate_op.param;
    call $cbops.hw_warp_op.Initialize;
    
    r1 = M[frame_adc_sampling_rate];    
    r8 = &$adc_in.sw_rate_op.param;
    call $cbops.rate_monitor_op.Initialize;

    // ---- Setup ADC and DAC -----
    //  MAX_ADVANCE_FIELD for DAC cBuffer and sidetone (8kHz or 16kHz time 625usec)
    //      NB:  DAC cBuffer & Sidetone = 8kHz
    //      FE:  DAC cBuffer & Sidetone = 16kHz
    //      WB:  DAC cBuffer & Sidetone = 16kHz
    r6 = M[frame_dac_sampling_rate];
    r2 = r6 * $ADCDAC_PERIOD_USEC (frac);
    r2 = r2 + 1;
    M[$dac_out.sidetone_mix_op.param + $cbops.sidetone_mix_op.SIDETONE_MAX_SAMPLES_FIELD] = r2;
    M[$dac_out.insert_op.param + $cbops.insert_op.MAX_ADVANCE_FIELD]  = r2;
    // Smaller jitter value means lesser room for latency to diverge from the 
    // nominal accumulation level. It also means that there may be more opportunities
    // for frame process latency control to kick in and drop/insert samples as
    // required. 
    r3 = r2 + r2; 
    M[&$M.CVC.data.stream_map_rcvout + $framesync_ind.JITTER_FIELD]   = r3;
    // clear frame counter to reset frame threshold calculation
    M[$M.CVC.data.stream_map_rcvout + $framesync_ind.COUNTER_FIELD] = NULL;
    // Jitter for send in
    M[&$M.CVC.data.stream_map_sndin  + $framesync_ind.JITTER_FIELD]   = r3;
    // clear frame counter to reset frame threshold calculation
    M[$M.CVC.data.stream_map_sndin + $framesync_ind.COUNTER_FIELD] = NULL;
    // We maintain 2 period of jitter on all the frontend buffers - i.e rcv out,
    // ref in, adc. Having the same quantity of jitter is important since it allows 
    // the buffer latency deviation from the nominal level to be within the same 
    // range on all buffers. This is essential for latency stability needed for 
    // time sensitive processing modules such as AEC.
    M[&$M.CVC.data.stream_map_refin  + $framesync_ind.JITTER_FIELD]   = r3;
    // clear frame counter to reset frame threshold calculation
    M[$M.CVC.data.stream_map_refin + $framesync_ind.COUNTER_FIELD] = NULL;

    r7 = M[$M.audio_config.dac_sampling_rate];
    r8 = &cvc_rcvout_resampler_lookup;
    call IdentifyFilter;
    // r0 is resampler Descriptor or NULL (passthrough)
    // Set DAC port MAX_ADVANCE_FIELD (r2)
    r3 = r7 * $ADCDAC_PERIOD_USEC (frac);
    r3 = r3 + 1;
    NULL = r0;
    if NZ r2 = r3;
    M[$dac_out.dac_wrap_op.param + $cbops.port_wrap_op.MAX_ADVANCE_FIELD] = r2;

    r1 = M[$M.audio_config.dac_sampling_rate];
    r8 = &$dac_out.sw_rate_monitor_op.param;
    push r0;
    call $cbops.rate_monitor_op.Initialize;
    pop r0;

    r8 = &$dac_out.resample_op.param;
    push r0;
    call $iir_resamplev2.SetFilter;
    pop r0;

  jump $pop_rLink_and_rts;

$DAC_CheckConnectivity:
    $push_rLink_macro;

    r8 = 1;
    r0 = $DAC_PORT_R;
    call $cbuffer.is_it_enabled;
    if Z r8=NULL;
    M[$dac_out.RightPortEnabled] = r8;

    jump $pop_rLink_and_rts;

// *****************************************************************************
// MODULE:
//    $set_tone_rate_from_vm
//
// DESCRIPTION: message handler for receiving tone rate from VM
//
// INPUTS:
//  r1 = (tone rate)
//  r2 = bit 0: mono / stereo
//       bit 1: tone / prompt
//  r3 = Not used
//  r4 = Not used
// OUTPUTS:
//    none
// *****************************************************************************
   $set_tone_rate_from_vm:

   $push_rLink_macro;

   // extract  mono/stereo flag, this application
   // coverts streo voice prompts to mono
   r0 = r2 AND 1;
   M[$tone_in.deinterleave_mix_op.param + $cbops.deinterleave_mix.INPUT_INTERLEAVED_FIELD] = r0;

   // firmware tones are boosted by 3 bits, voice promts expected to be normalised
   r0 = 8 + $PROMPT_EXTRA_BOOST_BITS;
   r3 = 8 + $TONE_EXTRA_BOOST_BITS;
   Null = r2 AND 0x2;
   if Z r0 = r3;
   M[$tone_in.copy_op.param + $iir_resamplev2.OUTPUT_SCALE_FIELD] = r0;

   // extract tone rate
   r1 = r1 AND 0xFFFF;
   M[current_tone_sampling_rate] = r1;

   // look up table to search
   // for matching entry
   r2 = cvc_sndin_resampler_lookup;
   r3 = r2;
   r4 = 0x7FFFFF;
   search_tone_rate_loop:
      // r3 = best match so far
      // r4 = best distance so far
      // r1 = tone sample rate

      // read next entry sample rate
      r0 = M[r2 + 0];
      // if no more entry, exit the loop
      if Z jump seach_done;

      // if exact match found exit
      r0 = r0 - r1;
      if Z jump exact_tone_found;

      // else look for nearest one
      if NEG r0 = -r0;
      Null = r4 - r0;
      if NEG jump  continue_search;

      // update nearest one
      r4 = r0;
      r3 = r2;
      continue_search:
      // go to next entry
      r2 = r2 + 3;
   jump  search_tone_rate_loop;

   exact_tone_found:
   r3 = r2;
   seach_done:
   // r3 = exact or nearest entry
   r0 = M[r3 + 1];
   r1 = M[r3 + 2];
   // r0 = rate -> 8khz coeffs
   // r1 = rate -> 16khz coeffs
   r2 = M[frame_dac_sampling_rate];
   Null = r2 - 8000;
   if NZ r0 = r1;
   // set operator filter coeffs
   r8 = &$tone_in.copy_op.param;
   call $iir_resamplev2.SetFilter;

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
// *****************************************************************************

.MODULE $M.detect_end_of_aux_stream;
   .CODESEGMENT PM;
   .DATASEGMENT DM;
   
   .VAR last_write_address = &$tone_in.mem;   
   .VAR write_move_counter = 0;

   $detect_end_of_aux_stream:

   $push_rLink_macro;

   // detect end of tone/prompt auxiliary pcm input stream
   r3 = M[$tone_in.aux_input_stream_available];
   if Z jump $pop_rLink_and_rts;

   // see if the input is active
   r0 = &$tone_in.cbuffer_struc;
   call $cbuffer.calc_amount_data;

   // check if input activity has been seen before
   Null = r3 AND 0x2;
   if NZ jump input_has_received;

   // input hasn't started yet, so no end check
   Null = r0;
   if Z jump $pop_rLink_and_rts;

   // input just received
   r3 = r3 OR 0x2;
   M[&$tone_in.aux_input_stream_available] = r3;
   jump $pop_rLink_and_rts;

   input_has_received:

   // Get the write address for the output buffer
   r0 = &$tone_in.cbuffer_struc;
   call $cbuffer.get_write_address_and_size;
   r4 = M[last_write_address];
   r3 = M[write_move_counter];
   r5 = 0;
   
   Null = r0 - r4;
   if Z r5 = r3 + 1;
   M[write_move_counter] = r5;
   
   Null = r5 - $PCM_END_DETECTION_TIME_OUT;
   if NEG jump $pop_rLink_and_rts;
   
   // inactive more than a threshold
   // notify VM about end of play back of aux input
   r2 = PLAY_BACK_FINISHED_MSG;
   r3 = 0;
   r4 = 0;
   r5 = 0;
   r6 = 0;      
   call $message.send_short;
   M[$tone_in.aux_input_stream_available] = 0;
   M[write_move_counter] = 0;
   
   // Purge tone buffers
   r0 = M[&$tone_in.cbuffer_struc + $cbuffer.WRITE_ADDR_FIELD];
   M[&$tone_in.cbuffer_struc + $cbuffer.READ_ADDR_FIELD] = r0; 
     
   // pop rLink from stack
   jump $pop_rLink_and_rts;
   
.ENDMODULE;
