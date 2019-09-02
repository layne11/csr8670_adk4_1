// *****************************************************************************
// Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
//
// *****************************************************************************

#ifndef CBOPS_IIR_RESAMPLEV2_HEADER_INCLUDED
#define CBOPS_IIR_RESAMPLEV2_HEADER_INCLUDED

#include "iir_resamplerv2_common_static.h"

/* -------------------- cBops Operator --------------------------------------- */
   .CONST   $iir_resamplev2.INPUT_1_START_INDEX_FIELD              0;
   .CONST   $iir_resamplev2.OUTPUT_1_START_INDEX_FIELD             $iir_resamplev2.INPUT_1_START_INDEX_FIELD+1;
   .CONST   $iir_resamplev2.COMMON_FIELD                           $iir_resamplev2.OUTPUT_1_START_INDEX_FIELD+1;
      // Filter Definition
      .CONST   $iir_resamplev2.FILTER_DEFINITION_PTR_FIELD           $iir_resamplev2.COMMON_FIELD + $iir_resamplerv2_common.iir_resampler_common_struct.FILTER_FIELD;
      .CONST   $iir_resamplev2.INPUT_SCALE_FIELD                     $iir_resamplev2.COMMON_FIELD + $iir_resamplerv2_common.iir_resampler_common_struct.INPUT_SCALE_FIELD;
      .CONST   $iir_resamplev2.OUTPUT_SCALE_FIELD                    $iir_resamplev2.COMMON_FIELD + $iir_resamplerv2_common.iir_resampler_common_struct.OUTPUT_SCALE_FIELD;
      // Buffer between Stages
      .CONST   $iir_resamplev2.INTERMEDIATE_CBUF_PTR_FIELD           $iir_resamplev2.COMMON_FIELD  + $iir_resamplerv2_common.iir_resampler_common_struct.INTERMEDIATE_PTR_FIELD;
      .CONST   $iir_resamplev2.INTERMEDIATE_CBUF_LEN_FIELD           $iir_resamplev2.COMMON_FIELD  + $iir_resamplerv2_common.iir_resampler_common_struct.INTERMEDIATE_SIZE_FIELD;
      // Reset Flags (Set to NULL)
      .CONST   $iir_resamplev2.RESET_FLAG_FIELD                      $iir_resamplev2.COMMON_FIELD  + $iir_resamplerv2_common.iir_resampler_common_struct.RESET_FLAG_FIELD;
      .CONST   $iir_resamplev2.DBL_PRECISSION_FIELD                  $iir_resamplev2.COMMON_FIELD  + $iir_resamplerv2_common.iir_resampler_common_struct.DBL_PRECISSION_FIELD;
   .CONST   $iir_resamplev2.CHANNEL_FIELD                          $iir_resamplev2.COMMON_FIELD + $iir_resamplerv2_common.iir_resampler_common_struct.STRUC_SIZE;
      // 1st Stage
      .CONST   $iir_resamplev2.PARTIAL1_FIELD                        $iir_resamplev2.CHANNEL_FIELD + $iir_resamplerv2_common.iir_resampler_channel_struct.PARTIAL1_FIELD;
      .CONST   $iir_resamplev2.SAMPLE_COUNT1_FIELD                   $iir_resamplev2.CHANNEL_FIELD + $iir_resamplerv2_common.iir_resampler_channel_struct.SMPL_COUNT1_FIELD;
      .CONST   $iir_resamplev2.FIR_HISTORY_BUF1_PTR_FIELD            $iir_resamplev2.CHANNEL_FIELD + $iir_resamplerv2_common.iir_resampler_channel_struct.FIR1_PTR_FIELD;
      .CONST   $iir_resamplev2.IIR_HISTORY_BUF1_PTR_FIELD            $iir_resamplev2.CHANNEL_FIELD + $iir_resamplerv2_common.iir_resampler_channel_struct.IIR1_PTR_FIELD;
      // 2nd Stage
      .CONST   $iir_resamplev2.PARTIAL2_FIELD                        $iir_resamplev2.CHANNEL_FIELD + $iir_resamplerv2_common.iir_resampler_channel_struct.PARTIAL2_FIELD;
      .CONST   $iir_resamplev2.SAMPLE_COUNT2_FIELD                   $iir_resamplev2.CHANNEL_FIELD + $iir_resamplerv2_common.iir_resampler_channel_struct.SMPL_COUNT2_FIELD;
      .CONST   $iir_resamplev2.FIR_HISTORY_BUF2_PTR_FIELD            $iir_resamplev2.CHANNEL_FIELD + $iir_resamplerv2_common.iir_resampler_channel_struct.FIR2_PTR_FIELD;
      .CONST   $iir_resamplev2.IIR_HISTORY_BUF2_PTR_FIELD            $iir_resamplev2.CHANNEL_FIELD + $iir_resamplerv2_common.iir_resampler_channel_struct.IIR2_PTR_FIELD;
   .CONST   $iir_resamplev2.WORKING_FIELD                            $iir_resamplev2.CHANNEL_FIELD  + $iir_resamplerv2_common.iir_resampler_channel_struct.STRUC_SIZE;
   .CONST   $iir_resamplev2.STRUC_SIZE                             $iir_resamplev2.WORKING_FIELD;

/* -------------------- Completion Operator ---------------------------------- */
   .CONST   $cbops.complete.iir_resamplev2.STRUC_SIZE   1;


/* ------------------------ History Buffers ---------------------------------- */

   .CONST   $IIR_RESAMPLEV2_IIR_BUFFER_SIZE      19;
   .CONST   $IIR_RESAMPLEV2_FIR_BUFFER_SIZE      10;

#ifdef SUPPORTS_DBL_PRECISSION
   .CONST   $IIR_RESAMPLEV2_APPENDED_BUFFER_SIZE      (2*$IIR_RESAMPLEV2_IIR_BUFFER_SIZE+$IIR_RESAMPLEV2_FIR_BUFFER_SIZE);
#else
   .CONST   $IIR_RESAMPLEV2_APPENDED_BUFFER_SIZE      ($IIR_RESAMPLEV2_IIR_BUFFER_SIZE+$IIR_RESAMPLEV2_FIR_BUFFER_SIZE);
#endif
   
   .CONST   $iir_resamplev2.OBJECT_SIZE               $iir_resamplev2.STRUC_SIZE + 2*$IIR_RESAMPLEV2_APPENDED_BUFFER_SIZE;
   .CONST   $iir_resamplev2.OBJECT_SIZE_SNGL_STAGE    $iir_resamplev2.STRUC_SIZE + $IIR_RESAMPLEV2_APPENDED_BUFFER_SIZE;


   // Interface to old frame sync
   .CONST   $cbops.frame.resamplev2.INPUT_PTR_FIELD                       0;
   .CONST   $cbops.frame.resamplev2.INPUT_LENGTH_FIELD                    $cbops.frame.resamplev2.INPUT_PTR_FIELD + 1;
   .CONST   $cbops.frame.resamplev2.OUTPUT_PTR_FIELD                      $cbops.frame.resamplev2.INPUT_LENGTH_FIELD + 1;
   .CONST   $cbops.frame.resamplev2.OUTPUT_LENGTH_FIELD                   $cbops.frame.resamplev2.OUTPUT_PTR_FIELD  + 1;
   .CONST   $cbops.frame.resamplev2.NUM_SAMPLES_FIELD                     $cbops.frame.resamplev2.OUTPUT_LENGTH_FIELD + 1;
   .CONST   $cbops.frame.resamplev2.COMMON_FIELD                          $cbops.frame.resamplev2.NUM_SAMPLES_FIELD + 1;
         // Filter Definition
      .CONST   $cbops.frame.resamplev2.FILTER_DEFINITION_PTR_FIELD           $cbops.frame.resamplev2.COMMON_FIELD + $iir_resamplerv2_common.iir_resampler_common_struct.FILTER_FIELD;
      .CONST   $cbops.frame.resamplev2.INPUT_SCALE_FIELD                     $cbops.frame.resamplev2.COMMON_FIELD + $iir_resamplerv2_common.iir_resampler_common_struct.INPUT_SCALE_FIELD;
      .CONST   $cbops.frame.resamplev2.OUTPUT_SCALE_FIELD                    $cbops.frame.resamplev2.COMMON_FIELD + $iir_resamplerv2_common.iir_resampler_common_struct.OUTPUT_SCALE_FIELD;
      // Buffer between Stages
      .CONST   $cbops.frame.resamplev2.INTERMEDIATE_CBUF_PTR_FIELD           $cbops.frame.resamplev2.COMMON_FIELD + $iir_resamplerv2_common.iir_resampler_common_struct.INTERMEDIATE_PTR_FIELD;
      .CONST   $cbops.frame.resamplev2.INTERMEDIATE_CBUF_LEN_FIELD           $cbops.frame.resamplev2.COMMON_FIELD + $iir_resamplerv2_common.iir_resampler_common_struct.INTERMEDIATE_SIZE_FIELD;
      // Reset Flags (Set to NULL)
      .CONST   $cbops.frame.resamplev2.RESET_FLAG_FIELD                      $cbops.frame.resamplev2.COMMON_FIELD + $iir_resamplerv2_common.iir_resampler_common_struct.RESET_FLAG_FIELD;
      .CONST   $cbops.frame.resamplev2.DBL_PRECISSION_FIELD                  $cbops.frame.resamplev2.COMMON_FIELD + $iir_resamplerv2_common.iir_resampler_common_struct.DBL_PRECISSION_FIELD;
    .CONST   $cbops.frame.resamplev2.CHANNEL_FIELD                        $cbops.frame.resamplev2.COMMON_FIELD + $iir_resamplerv2_common.iir_resampler_common_struct.STRUC_SIZE;
      // 1st Stage
      .CONST   $cbops.frame.resamplev2.PARTIAL1_FIELD                        $cbops.frame.resamplev2.CHANNEL_FIELD + $iir_resamplerv2_common.iir_resampler_channel_struct.PARTIAL1_FIELD;
      .CONST   $cbops.frame.resamplev2.SAMPLE_COUNT1_FIELD                   $cbops.frame.resamplev2.CHANNEL_FIELD + $iir_resamplerv2_common.iir_resampler_channel_struct.SMPL_COUNT1_FIELD;
      .CONST   $cbops.frame.resamplev2.FIR_HISTORY_BUF1_PTR_FIELD            $cbops.frame.resamplev2.CHANNEL_FIELD + $iir_resamplerv2_common.iir_resampler_channel_struct.FIR1_PTR_FIELD;
      .CONST   $cbops.frame.resamplev2.IIR_HISTORY_BUF1_PTR_FIELD            $cbops.frame.resamplev2.CHANNEL_FIELD + $iir_resamplerv2_common.iir_resampler_channel_struct.IIR1_PTR_FIELD;
      // 2nd Stage
      .CONST   $cbops.frame.resamplev2.PARTIAL2_FIELD                        $cbops.frame.resamplev2.CHANNEL_FIELD + $iir_resamplerv2_common.iir_resampler_channel_struct.PARTIAL2_FIELD;
      .CONST   $cbops.frame.resamplev2.SAMPLE_COUNT2_FIELD                   $cbops.frame.resamplev2.CHANNEL_FIELD + $iir_resamplerv2_common.iir_resampler_channel_struct.SMPL_COUNT2_FIELD;
      .CONST   $cbops.frame.resamplev2.FIR_HISTORY_BUF2_PTR_FIELD            $cbops.frame.resamplev2.CHANNEL_FIELD + $iir_resamplerv2_common.iir_resampler_channel_struct.FIR2_PTR_FIELD;
      .CONST   $cbops.frame.resamplev2.IIR_HISTORY_BUF2_PTR_FIELD            $cbops.frame.resamplev2.CHANNEL_FIELD + $iir_resamplerv2_common.iir_resampler_channel_struct.IIR2_PTR_FIELD;
   .CONST   $cbops.frame.resamplev2.WORKING_FIELD                         $cbops.frame.resamplev2.CHANNEL_FIELD + $iir_resamplerv2_common.iir_resampler_channel_struct.STRUC_SIZE;
   .CONST   $cbops.frame.resamplev2.STRUC_SIZE                            $cbops.frame.resamplev2.WORKING_FIELD;               


   .CONST   $cbops.frame.resamplev2.OBJECT_SIZE               $cbops.frame.resamplev2.STRUC_SIZE + 2*$IIR_RESAMPLEV2_APPENDED_BUFFER_SIZE;
   .CONST   $cbops.frame.resamplev2.OBJECT_SIZE_SNGL_STAGE    $cbops.frame.resamplev2.STRUC_SIZE + $IIR_RESAMPLEV2_APPENDED_BUFFER_SIZE;

#endif // CBOPS_IIR_RESAMPLE_HEADER_INCLUDED

