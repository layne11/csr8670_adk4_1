// *****************************************************************************
// Copyright (c) 2003 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
//
// *****************************************************************************

// *****************************************************************************
// DESCRIPTION
//    Rate adjustment used to provide fine control to match the delivered rate
//    to the consumption rate of the output device. This is used for
//    multi-channel operation and is performed before channel splitting
//    (before the Music manager) in order to reduce MIPS.
//
// *****************************************************************************

#include "core_library.h"
#include "cbops_library.h"
#include "music_example.h"

#ifdef TWS_ENABLE
#include "relay_conn.h"
#endif

#if defined(APTX_ACL_SPRINT_ENABLE)
#include "codec_decoder_aptx_acl_sprint.h"
#include "sr_adjustment_gaming.h"
#elseif defined (FASTSTREAM_ENABLE)
#include "codec_decoder_faststream.h"
#include "sr_adjustment_gaming.h"
#else
#include "codec_decoder.h"
#include "sr_adjustment.h"
#endif

#include "frame_sync_stream_macros.h"
#include "frame_sync_buffer.h"
// CODEC rate adjustment module
.MODULE $codec_rate_adj;
   .DATASEGMENT DM;

#if defined(APTX_ACL_SPRINT_ENABLE) || defined(FASTSTREAM_ENABLE)
   // Size of CODEC resampler output buffers when ONLY APTX or FASTSTREAM decoders are used
   .CONST CODEC_RATE_ADJUSTMENT_CBUFFER_SIZE          ($music_example.NUM_SAMPLES_PER_FRAME + 100);
#else
   // Size of CODEC resampler output buffers (needs to be double the frame size when using the subwoofer)
   .CONST CODEC_RATE_ADJUSTMENT_CBUFFER_SIZE          (2 * $music_example.NUM_SAMPLES_PER_FRAME + 100);
#endif

   DeclareCBuffer($codec_rate_adj_out_left_cbuffer_struc,$codec_rate_adj_out_left,CODEC_RATE_ADJUSTMENT_CBUFFER_SIZE);
   DeclareCBuffer($codec_rate_adj_out_right_cbuffer_struc,$codec_rate_adj_out_right,CODEC_RATE_ADJUSTMENT_CBUFFER_SIZE);
#if defined(SPDIF_ENABLE) && defined(AC3_ENABLE)
   DeclareCBuffer($codec_rate_adj_out_lfe_cbuffer_struc,$codec_rate_adj_out_lfe,CODEC_RATE_ADJUSTMENT_CBUFFER_SIZE);
#endif // defined(SPDIF_ENABLE) && defined(AC3_ENABLE)

   // Always use history buffers large enough for the HD adjustment filter
   // (this will work with the standard quality or HD quality filter)
   .VAR/DM1CIRC hist_left[$cbops.rate_adjustment_and_shift.SRA_HD_QUALITY_COEFFS_SIZE];
   .VAR/DM1CIRC hist_right[$cbops.rate_adjustment_and_shift.SRA_HD_QUALITY_COEFFS_SIZE];
#if defined(SPDIF_ENABLE) && defined(AC3_ENABLE)
   .VAR/DM1CIRC hist_lfe;                                                     // LFE doesn't need history since it uses only interpolation
#endif // defined(SPDIF_ENABLE) && defined(AC3_ENABLE)

   // Use the process interface to the rate_adjustment_and_shift library
   .VAR stereo[$cbops.rate_adjustment_and_shift.Process.STRUC_SIZE] =
      $codec_resamp_out_left_cbuffer_struc,                                   // $cbops.rate_adjustment_and_shift.Process.INPUT1_CBUFFER_ADDR_FIELD
      $codec_rate_adj_out_left_cbuffer_struc,                                 // $cbops.rate_adjustment_and_shift.Process.OUTPUT1_CBUFFER_ADDR_FIELD
      $codec_resamp_out_right_cbuffer_struc,                                  // $cbops.rate_adjustment_and_shift.Process.INPUT2_CBUFFER_ADDR_FIELD
      $codec_rate_adj_out_right_cbuffer_struc,                                // $cbops.rate_adjustment_and_shift.Process.OUTPUT2_CBUFFER_ADDR_FIELD
      0,                                                                      // $cbops.rate_adjustment_and_shift.Process.SHIFT_AMOUNT_FIELD
#if defined(SPDIF_ENABLE)
      $sra_coeffs_hd_quality,                                                 // $cbops.rate_adjustment_and_shift.Process.FILTER_COEFFS_FIELD
#else
      $sra_coeffs,                                                            // $cbops.rate_adjustment_and_shift.Process.FILTER_COEFFS_FIELD
#endif
      hist_left,                                                              // $cbops.rate_adjustment_and_shift.Process.HIST1_BUF_FIELD
      hist_right,                                                             // $cbops.rate_adjustment_and_shift.Process.HIST2_BUF_FIELD
#ifdef BASE_REGISTER_MODE
      hist_left,                                                              // $cbops.rate_adjustment_and_shift.Process.HIST1_BUF_START_FIELD
      hist_right,                                                             // $cbops.rate_adjustment_and_shift.Process.HIST2_BUF_START_FIELD
#endif //BASE_REGISTER_MODE
#if defined(SPDIF_ENABLE)
      $spdif_sra_rate,                                                        // $cbops.rate_adjustment_and_shift.Process.SRA_TARGET_RATE_ADDR_FIELD
#else // SPDIF_ENABLE
      $sra_struct + $sra.SRA_RATE_FIELD,                                      // $cbops.rate_adjustment_and_shift.Process.SRA_TARGET_RATE_ADDR_FIELD
#endif // SPDIF_ENABLE
      $cbops.dither_and_shift.DITHER_TYPE_NONE,                               // $cbops.rate_adjustment_and_shift.Process.DITHER_TYPE_FIELD
      0,
      length(hist_left),                                                      // $cbops.rate_adjustment_and_shift.FILTER_COEFFS_SIZE_FIELD
      0 ...;                                                                  // Zero all other remaining fields

#if defined(SPDIF_ENABLE) && defined(AC3_ENABLE)
   // Use the process interface to the rate_adjustment_and_shift library
   .VAR lfe[$cbops.rate_adjustment_and_shift.Process.STRUC_SIZE] =
      $codec_resamp_out_lfe_cbuffer_struc,                                    // $cbops.rate_adjustment_and_shift.Process.INPUT1_CBUFFER_ADDR_FIELD
      $codec_rate_adj_out_lfe_cbuffer_struc,                                  // $cbops.rate_adjustment_and_shift.Process.OUTPUT1_CBUFFER_ADDR_FIELD
      -1,                                                                     // $cbops.rate_adjustment_and_shift.Process.INPUT2_CBUFFER_ADDR_FIELD
      -1,                                                                     // $cbops.rate_adjustment_and_shift.Process.OUTPUT2_CBUFFER_ADDR_FIELD
      0,                                                                      // $cbops.rate_adjustment_and_shift.Process.SHIFT_AMOUNT_FIELD
      0,                                                                      // $cbops.rate_adjustment_and_shift.Process.FILTER_COEFFS_FIELD
      hist_lfe,                                                               // $cbops.rate_adjustment_and_shift.Process.HIST1_BUF_FIELD
      0,                                                                      // $cbops.rate_adjustment_and_shift.Process.HIST2_BUF_FIELD
#ifdef BASE_REGISTER_MODE
      hist_lfe,                                                               // $cbops.rate_adjustment_and_shift.Process.HIST1_BUF_START_FIELD
      0,                                                                      // $cbops.rate_adjustment_and_shift.Process.HIST2_BUF_START_FIELD
#endif //BASE_REGISTER_MODE
      $spdif_sra_rate,                                                        // $cbops.rate_adjustment_and_shift.Process.SRA_TARGET_RATE_ADDR_FIELD
      $cbops.dither_and_shift.DITHER_TYPE_NONE,                               // $cbops.rate_adjustment_and_shift.Process.DITHER_TYPE_FIELD,
      0,
      1,                                                                      // $cbops.rate_adjustment_and_shift.Process.FILTER_COEFFS_SIZE_FILED,
                                                                              // "1" is special case for doing interpolation only
      0 ...;                                                                  // Zero all other remaining fields
#endif // defined(SPDIF_ENABLE) && defined(AC3_ENABLE)

   // Flag showing whether the rate adjustment is active
   //.VAR active = 0;

// *****************************************************************************
// MODULE:
//    $codec_rate_adj.config_buffers
//
// DESCRIPTION:
//    The main processing loop will look like the following chain,
//    input -> resampler -> rate adjustment -> music manager
//
//    The resampler and/or rate adjustment might not be available
//    in some system configurations, this function will
//    set the proper buffers for the modules input & outputs.
//
// INPUTS:
//    - none
//
// OUTPUTS:
//    - none
//
// TRASHED REGISTERS:
//    Assume everything
//
// NOTE:
//    Before this function is called:
//
//    1- $codec_resampler.resampler_active must be already set,
//       which shows whether a resampler is present in main loop.
//
//    2- Set two important variables $chain0_hw_warp_enable and
//       $chain1_hw_warp_enable, these two show whether a chain can use
//       hardware warping.
//
//       Restrictions which aren't checked by this function:
//
//       a. At most one chain can use hardware warping
//       b. The chain that uses HW warp, must be connected to stereo DACs
//       c. If rate match is disabled via $rate_match_disable, no chain shall use
//          HW warp
//       d. If sub or TWS is operating, chain 0 can not use HW warping
//       e. This function hasn't been tested for change of the HW warping switch
//         in real time, i.e. set the switches only in init function.
//
// *****************************************************************************

   .CODESEGMENT CODEC_RATE_ADJ_PM;
   config_buffers:

   // Push rLink onto stack
   $push_rLink_macro;

   call $block_interrupts;

   // r3 = left
   // r4 = right
   // r5 = lfe (for spdif only)
   r4 =  $codec_resamp_out_left_cbuffer_struc;
   r5 =  $codec_resamp_out_right_cbuffer_struc;
   #if defined(SPDIF_ENABLE) && defined(AC3_ENABLE)
      r6 =  $codec_resamp_out_lfe_cbuffer_struc;
   #endif

   // see if resampler is active
   Null = M[$codec_resampler.resampler_active];
   if NZ jump resampler_done;
       // resample is off, just clear the buffers
       // in case some latency calc uses them
       r0 =  $codec_resamp_out_left_cbuffer_struc;
       call $cbuffer.empty_buffer;
       r0 =  $codec_resamp_out_right_cbuffer_struc;
       call $cbuffer.empty_buffer;
       #if defined(SPDIF_ENABLE) && defined(AC3_ENABLE)
          r0 =  $codec_resamp_out_lfe_cbuffer_struc;
          call $cbuffer.empty_buffer;
       #endif

       // input will directly be routed to
       // rate adjust
       r4 = $audio_out_left_cbuffer_struc;
       r5 = $audio_out_right_cbuffer_struc;
       #if defined(SPDIF_ENABLE) && defined(AC3_ENABLE)
          r6 = $audio_out_lfe_cbuffer_struc;
       #endif
   resampler_done:

   // no software rate match for chain0
   // if hw rate match is to be used
   // or it is disabled altogether
   r0 = M[$chain0_hw_warp_enable];
   r0 = r0 + M[$rate_match_disable];
   if Z jump chain0_sw_rate_matching;
       // No sw rate adjustment is needed, clear related
       // buffers in case some latency calc uses them
       r0 = $codec_rate_adj_out_left_cbuffer_struc;
       call $cbuffer.empty_buffer;
       r0 = $codec_rate_adj_out_right_cbuffer_struc;
       call $cbuffer.empty_buffer;
       #if defined(SPDIF_ENABLE) && defined(AC3_ENABLE)
          r0 = $codec_rate_adj_out_lfe_cbuffer_struc;
          call $cbuffer.empty_buffer;
       #endif
       jump rate_adj_done;

   // chain0 uses sw rate adjustment
   chain0_sw_rate_matching:
   // config rate adjustment buffers
   M[stereo + $cbops.rate_adjustment_and_shift.Process.INPUT1_CBUFFER_ADDR_FIELD] = r4;
   M[stereo + $cbops.rate_adjustment_and_shift.Process.INPUT2_CBUFFER_ADDR_FIELD] = r5;
   r4 = $codec_rate_adj_out_left_cbuffer_struc;
   r5 = $codec_rate_adj_out_right_cbuffer_struc;
   #if defined(SPDIF_ENABLE) && defined(AC3_ENABLE)
      M[lfe + $cbops.rate_adjustment_and_shift.Process.INPUT1_CBUFFER_ADDR_FIELD] = r6;
      r6 = $codec_rate_adj_out_lfe_cbuffer_struc;
   #endif
   rate_adj_done:

   // set the music manager input buffers
   M[$M.system_config.data.stream_map_left_in + $framesync_ind.CBUFFER_PTR_FIELD] = r4;
   M[$M.system_config.data.stream_map_right_in + $framesync_ind.CBUFFER_PTR_FIELD] = r5;
   #if defined(SPDIF_ENABLE) && defined(AC3_ENABLE)
      M[$M.system_config.data.stream_map_lfe_in + $framesync_ind.CBUFFER_PTR_FIELD] = r6;
   #endif

   // the proper chain to use hw rate matching
   #if defined(SPDIF_ENABLE)
      r0 = $spdif_sra_struct + $spdif_sra.SRA_RATE_FIELD;
   #else
      r0 = $sra_struct + $sra.SRA_RATE_FIELD;
   #endif
   r1 = $chain1_to_chain0_pcm_sync_struct + $pcm_sync.SRA_RATE_FIELD;
   Null = M[$chain1_hw_warp_enable];
   if NZ r0 = r1;
   M[$hw_warp_struct +  $hw_warp.TARGET_RATE_PTR_FIELD] = r0;

   call $unblock_interrupts;

   // see if no chain uses hw warping
   r0 = M[$chain0_hw_warp_enable];
   r0 = r0 + M[$chain1_hw_warp_enable];
   if NZ jump $pop_rLink_and_rts;

   // Just in case that this function is used in real time
   // if both HW warp switches are off, then make sure the
   // warp rate is cleared
   r4 = 0;
   r2 = &$MESSAGE_WARP_DAC;
   r3 = 3;
   call $message.send_short;

   // Pop rLink from stack
   jump $pop_rLink_and_rts;

// *****************************************************************************
// MODULE:
//    $codec_rate_adj.run_rate_adjustment
//
// DESCRIPTION:
//    utility function that runs the rate adjustment for main chain
//
// INPUTS:
//    - none
//
// OUTPUTS:
//    - none
//
// TRASHED REGISTERS:
//    Assume everything
//
// *****************************************************************************

run_rate_adjustment:

   // see if rate match is disabled
   Null = M[$rate_match_disable];
   if NZ rts;

   // Push rLink onto stack
   $push_rLink_macro;
   #ifdef TWS_ENABLE
      r1 = M[$relay.mode];
      null = r1 - 2;
      if NEG jump no_limit ;
      Null = M[$codec_resampler.resampler_active]; // if resample is active, limit took place there...
      if NZ jump no_limit;
      //  rate adjustment for Left & Right channels
      r8 = $codec_rate_adj.stereo;
      r5 = $TWS_FORCE_FRAME_SIZE;
      call $cbops.rate_adjustment_and_shift.Limited_Process; //Limited_Process
      jump done;
     no_limit:
   #endif

   //  rate adjustment for Left & Right channels
   r8 = $codec_rate_adj.stereo;
   call $cbops.rate_adjustment_and_shift.Process;

   #if defined(SPDIF_ENABLE) && defined(AC3_ENABLE)
      // rate adjustment for LFE channel
      r8 = $codec_rate_adj.lfe;
      call $cbops.rate_adjustment_and_shift.Process;
   #endif

   done:

   // Pop rLink from stack
   jump $pop_rLink_and_rts;

// *****************************************************************************
// MODULE:
//    $codec_rate_adj.config_quality
//
// DESCRIPTION:
//    Set the codec rate adjustment filter quality
//
// INPUTS:
//    none
//
// OUTPUTS:
//    none
//
// TRASHES: r2, r3
//
// *****************************************************************************

   config_quality:

   // Configure SW warp to use HD or standard coefficients
   r2 = $sra_coeffs;
   r3 = $cbops.rate_adjustment_and_shift.SRA_COEFFS_SIZE;

   // Check music manager selection parameter
   Null = M[$M.system_config.data.CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SW_WARP_TYPE];
   if Z jump set_warp_operator;

      r2 = $sra_coeffs_hd_quality;
      r3 = $cbops.rate_adjustment_and_shift.SRA_HD_QUALITY_COEFFS_SIZE;

   // r2=coeffs, r3=coeff size
   set_warp_operator:

   M[stereo + $cbops.rate_adjustment_and_shift.Process.FILTER_COEFFS_FIELD] = r2;
   M[stereo + $cbops.rate_adjustment_and_shift.Process.FILTER_COEFFS_SIZE_FIELD] = r3;

   rts;

// *****************************************************************************
// MODULE:
//    $codec_rate_adj.clear_history_buffers
//
// DESCRIPTION:
//    Clear the codec rate adjustment filter history buffers
//    (note: only left and right channels, LFE channel uses interpolation
//    only and so doesn't use a history buffer)
//
// INPUTS:
//    none
//
// OUTPUTS:
//    none
//
// TRASHES: r0, r10, I0, I1, DoLoop
//
// *****************************************************************************

   clear_history_buffers:

   r10 = $cbops.rate_adjustment_and_shift.SRA_HD_QUALITY_COEFFS_SIZE;
   r0 = $codec_rate_adj.hist_left;
   I0 = r0;
   r0 = $codec_rate_adj.hist_right;
   I1 = r0;

   r0 = 0;
   do audio_fill_loop1;
      M[I0,1] = r0;
      M[I1,1] = r0;
   audio_fill_loop1:

   rts;

.ENDMODULE;
