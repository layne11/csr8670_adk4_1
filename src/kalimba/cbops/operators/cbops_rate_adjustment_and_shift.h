// *****************************************************************************
// Copyright (c) 2007 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
//
// *****************************************************************************


#ifndef CBOPS_RATE_ADJUSTMENT_AND_SHIFT_HEADER_INCLUDED
#define CBOPS_RATE_ADJUSTMENT_AND_SHIFT_HEADER_INCLUDED

   .CONST   $cbops.rate_adjustment_and_shift.INPUT1_START_INDEX_FIELD             0; // left input
   .CONST   $cbops.rate_adjustment_and_shift.OUTPUT1_START_INDEX_FIELD            1; // left output
   .CONST   $cbops.rate_adjustment_and_shift.INPUT2_START_INDEX_FIELD             2; // right input
   .CONST   $cbops.rate_adjustment_and_shift.OUTPUT2_START_INDEX_FIELD            3; // right output
   .CONST   $cbops.rate_adjustment_and_shift.SHIFT_AMOUNT_FIELD                   4; // shift amount
   .CONST   $cbops.rate_adjustment_and_shift.FILTER_COEFFS_FIELD                  5; // filter coeffs address
   .CONST   $cbops.rate_adjustment_and_shift.HIST1_BUF_FIELD                      6; // history cirular buffer for left channel
   .CONST   $cbops.rate_adjustment_and_shift.HIST2_BUF_FIELD                      7; // history cirular buffer for right channel
#ifdef BASE_REGISTER_MODE
   .CONST   $cbops.rate_adjustment_and_shift.HIST1_BUF_START_FIELD                $cbops.rate_adjustment_and_shift.HIST2_BUF_FIELD+1;               // history cirular buffer for left channel start address
   .CONST   $cbops.rate_adjustment_and_shift.HIST2_BUF_START_FIELD                $cbops.rate_adjustment_and_shift.HIST1_BUF_START_FIELD+1;         // history cirular buffer for right channel start address
   .CONST   $cbops.rate_adjustment_and_shift.SRA_TARGET_RATE_ADDR_FIELD           $cbops.rate_adjustment_and_shift.HIST2_BUF_START_FIELD+1;         // target rate (address)
#else
   .CONST   $cbops.rate_adjustment_and_shift.SRA_TARGET_RATE_ADDR_FIELD           $cbops.rate_adjustment_and_shift.HIST2_BUF_FIELD+1;               // target rate (address)
#endif
   .CONST   $cbops.rate_adjustment_and_shift.DITHER_TYPE_FIELD                    $cbops.rate_adjustment_and_shift.SRA_TARGET_RATE_ADDR_FIELD+1;    // type of dithering
   .CONST   $cbops.rate_adjustment_and_shift.ENABLE_COMPRESSOR_FIELD              $cbops.rate_adjustment_and_shift.DITHER_TYPE_FIELD+1;             // 0: no compressor 1: compressor in
   .CONST   $cbops.rate_adjustment_and_shift.FILTER_COEFFS_SIZE_FIELD             $cbops.rate_adjustment_and_shift.ENABLE_COMPRESSOR_FIELD+1;
   .CONST   $cbops.rate_adjustment_and_shift.SRA_CURRENT_RATE_FIELD               $cbops.rate_adjustment_and_shift.FILTER_COEFFS_SIZE_FIELD+1;      // current rate
   .CONST   $cbops.rate_adjustment_and_shift.RF                                   $cbops.rate_adjustment_and_shift.SRA_CURRENT_RATE_FIELD+1;        // internal state
   .CONST   $cbops.rate_adjustment_and_shift.PREV_SHORT_SAMPLES_FIELD             $cbops.rate_adjustment_and_shift.RF+1;                            // internal state
   .CONST   $cbops.rate_adjustment_and_shift.WORKING_STATE_FIELD                  $cbops.rate_adjustment_and_shift.PREV_SHORT_SAMPLES_FIELD+1;      // delay or SRA mode
   .CONST   $cbops.rate_adjustment_and_shift.DITHER_HIST_LEFT_INDEX_FIELD         $cbops.rate_adjustment_and_shift.WORKING_STATE_FIELD+1;           // internal state
   .CONST   $cbops.rate_adjustment_and_shift.DITHER_HIST_RIGHT_INDEX_FIELD        $cbops.rate_adjustment_and_shift.DITHER_HIST_LEFT_INDEX_FIELD+1;  // internal state

   .CONST   $cbops.rate_adjustment_and_shift.AMOUNT_USED_FIELD   $cbops.rate_adjustment_and_shift.DITHER_HIST_RIGHT_INDEX_FIELD+1;
   .CONST   $cbops.rate_adjustment_and_shift.STRUC_SIZE $cbops.rate_adjustment_and_shift.AMOUNT_USED_FIELD + 1;

   // Completion operator parameter structure size
   .CONST   $cbops.rate_adjustment_and_shift_complete.STRUC_SIZE                  1;

   .CONST $cbops.rate_adjustment_and_shift.SRA_UPRATE      21;

   .CONST $cbops.rate_adjustment_and_shift.SRA_COEFFS_SIZE 12;
   .CONST $cbops.rate_adjustment_and_shift.SRA_HD_QUALITY_COEFFS_SIZE 36;

  .CONST $sra.MOVING_STEP (0.0015*(1.0/1000.0)/10.0); // 0.0015: interrupt period, this means it would take 8 seconds for 1hz change for a 1khz tone

   #define $sra_ENABLE_DITHER_FUNCTIONS
   #ifdef $sra_ENABLE_DITHER_FUNCTIONS
   // TODO: cannot declare constant as below.  For now replace $sra.scratch_buffer with $M.cbops.av_copy.left_silence_buffer
   // COME BACK AND ADDRESS THIS LATER!
   #endif

// *************************************************************************************************************
// The following code is included to allow a general purpose interface to the rate_adjustment_and_shift operator
// (this can be used to implement rate adjustment in the foreground (non-interrupt context)

   .CONST   $cbops.rate_adjustment_and_shift.Process.INPUT1_CBUFFER_ADDR_FIELD          0;  // left input cbuffer address
   .CONST   $cbops.rate_adjustment_and_shift.Process.OUTPUT1_CBUFFER_ADDR_FIELD         1;  // left output cbuffer address
   .CONST   $cbops.rate_adjustment_and_shift.Process.INPUT2_CBUFFER_ADDR_FIELD          2;  // right input cbuffer address
   .CONST   $cbops.rate_adjustment_and_shift.Process.OUTPUT2_CBUFFER_ADDR_FIELD         3;  // right output cbuffer address
   .CONST   $cbops.rate_adjustment_and_shift.Process.SHIFT_AMOUNT_FIELD                 4;  // shift amount
   .CONST   $cbops.rate_adjustment_and_shift.Process.FILTER_COEFFS_FIELD                5;  // filter coeffs address
   .CONST   $cbops.rate_adjustment_and_shift.Process.HIST1_BUF_FIELD                    6;  // history circular buffer address for left channel
   .CONST   $cbops.rate_adjustment_and_shift.Process.HIST2_BUF_FIELD                    7;  // history circular buffer address for right channel
#ifdef BASE_REGISTER_MODE
   .CONST   $cbops.rate_adjustment_and_shift.Process.HIST1_BUF_START_FIELD              $cbops.rate_adjustment_and_shift.Process.HIST2_BUF_FIELD+1;             // history circular buffer start address for left channel start address
   .CONST   $cbops.rate_adjustment_and_shift.Process.HIST2_BUF_START_FIELD              $cbops.rate_adjustment_and_shift.Process.HIST1_BUF_START_FIELD+1;       // history circular buffer start address for right channel start address
   .CONST   $cbops.rate_adjustment_and_shift.Process.SRA_TARGET_RATE_ADDR_FIELD         $cbops.rate_adjustment_and_shift.Process.HIST2_BUF_START_FIELD+1;       // target rate (address)
#else
   .CONST   $cbops.rate_adjustment_and_shift.Process.SRA_TARGET_RATE_ADDR_FIELD         $cbops.rate_adjustment_and_shift.Process.HIST2_BUF_FIELD+1;             // target rate (address)
#endif
        // - address which stores the target rate value,
        //   output_rate = (1 + target_rate) * input_rate, target rate is expeted to be
        //   between 0.005 and 0.005
   .CONST   $cbops.rate_adjustment_and_shift.Process.DITHER_TYPE_FIELD                  $cbops.rate_adjustment_and_shift.Process.SRA_TARGET_RATE_ADDR_FIELD+1;  // type of dithering
        // - determines which type of ditherring is applied befor shifting
        //    one of the follwing values
        // - $cbops.dither_and_shift.DITHER_TYPE_NONE (0)       -> no dithering
        // - $cbops.dither_and_shift.DITHER_TYPE_TRIANGULAR (1) -> triangular dithering
        // - $cbops.dither_and_shift.DITHER_TYPE_SHAPED (2)     -> noise shaped dithering
        // NOTE: $sra.ENABLE_DITHER_FUNCTIONS must be defined to enable dithering funtions
   .CONST   $cbops.rate_adjustment_and_shift.Process.ENABLE_COMPRESSOR_FIELD            $cbops.rate_adjustment_and_shift.Process.DITHER_TYPE_FIELD+1;           // 0: no compressor 1: compressor in
   .CONST   $cbops.rate_adjustment_and_shift.Process.FILTER_COEFFS_SIZE_FIELD           $cbops.rate_adjustment_and_shift.Process.ENABLE_COMPRESSOR_FIELD+1;
   .CONST   $cbops.rate_adjustment_and_shift.Process.SRA_CURRENT_RATE_FIELD             $cbops.rate_adjustment_and_shift.Process.FILTER_COEFFS_SIZE_FIELD+1;    // current rate
   .CONST   $cbops.rate_adjustment_and_shift.Process.RF                                 $cbops.rate_adjustment_and_shift.Process.SRA_CURRENT_RATE_FIELD+1;      // internal state
   .CONST   $cbops.rate_adjustment_and_shift.Process.PREV_SHORT_SAMPLES_FIELD           $cbops.rate_adjustment_and_shift.Process.RF+1;                          // internal state
   .CONST   $cbops.rate_adjustment_and_shift.Process.WORKING_STATE_FIELD                $cbops.rate_adjustment_and_shift.Process.PREV_SHORT_SAMPLES_FIELD+1;    // delay or SRA mode
   .CONST   $cbops.rate_adjustment_and_shift.Process.DITHER_HIST_LEFT_INDEX_FIELD       $cbops.rate_adjustment_and_shift.Process.WORKING_STATE_FIELD+1;         // internal state
   .CONST   $cbops.rate_adjustment_and_shift.Process.DITHER_HIST_RIGHT_INDEX_FIELD      $cbops.rate_adjustment_and_shift.Process.DITHER_HIST_LEFT_INDEX_FIELD+1;// internal state

   .CONST   $cbops.rate_adjustment_and_shift.Process.LAST_RUN_TIME_FIELD                $cbops.rate_adjustment_and_shift.Process.DITHER_HIST_RIGHT_INDEX_FIELD+1;// internal state

   .CONST   $cbops.rate_adjustment_and_shift.Process.STRUC_SIZE                         $cbops.rate_adjustment_and_shift.Process.LAST_RUN_TIME_FIELD+1;



     // define scratch structure, will be allocated in stack
    .CONST $sra.scratch.RIGHT_CHANNEL_ENABLE_FIELD                 0;
    .CONST $sra.scratch.RIGHT_CHANNEL_INPUT_BUFFER_ADDR_FIELD      1;
    .CONST $sra.scratch.RIGHT_CHANNEL_INPUT_BUFFER_LENGTH_FIELD    2;
    .CONST $sra.scratch.RIGHT_CHANNEL_INPUT_BUFFER_START_FIELD     3;
    .CONST $sra.scratch.RIGHT_CHANNEL_OUTPUT_BUFFER_ADDR_FIELD     4;
    .CONST $sra.scratch.RIGHT_CHANNEL_OUTPUT_BUFFER_LENGTH_FIELD   5;
    .CONST $sra.scratch.RIGHT_CHANNEL_OUTPUT_BUFFER_START_FIELD    6;
    .CONST $sra.scratch.SHIFT_VALUE_FIELD                          7;
    .CONST $sra.scratch.TEMP1_FIELD                                8;
    .CONST $sra.scratch.TEMP2_FIELD                                9;
    .CONST $sra.scratch.TEMP3_FIELD                                10;
    .CONST $sra.scratch.TEMP4_FIELD                                11;
    .CONST $sra.scratch.N_SAMPLES_FIELD                            12;
    .CONST $sra.scratch.CHN_NO_FIELD                                 13;
    .CONST $sra.scratch.DITHER_FUNCTION_FIELD                      14;
    .CONST $sra.scratch.STRUC_SIZE                                 15;    


#endif // CBOPS_RATE_ADJUSTMENT_AND_SHIFT_HEADER_INCLUDED
