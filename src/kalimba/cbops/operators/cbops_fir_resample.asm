// *****************************************************************************
// Copyright (c) 2013 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
//
// *****************************************************************************
// *****************************************************************************
// NAME:
//    fir fractional resample operator
//
// DESCRIPTION:
//    cbops Operator that resamples the input buffer into output buffer
//    using FIR resampling method.
//    Operator structure:
//        -INPUT_INDEX_FIELD
//            index of the input buffer/port
//
//        -OUTPUT_INDEX_FIELD
//            index of the output buffer/port
//
//        -COEF_BUF_INDEX_FIELD
//            FIR filter coefficients,
//            Filter length = 12, Over sampling = 21
//            Symmetrical and only second half is needed
//            shall be defined in DM2 for efficiency
//            use $sra_coeffs if you use this for upsampling only
//
//        -INPUT_RATE_ADDR_FIELD
//           address holding input rate in Hz
//
//        -OUTPUT_RATE_ADDR_FIELD
//           address holding output rate in Hz
//
//        -HIST_BUF_FIELD
//           history buffer, circular and shall be defined DM1 for efficiency
//           length must be Filter length + 1
//
//        - RESAMPLE_UNITY_RATIO_FIELD
//           if input rate is the same as output rate by default this operator
//           just copies input to output, if you want the input always passes
//           through resampling this field shall be non-zero. For example if
//           you use this operator for resampling and rate matching at the
//           same time (NOTE:input and output rate can change while running)
//           then this field shall be non zero.
// *****************************************************************************
#include "stack.h"
#include "cbops_library.h"
.MODULE $M.cbops.fir_resample;
    .DATASEGMENT DM;

    .VAR amount_used;

    // ** function vector **
    .VAR $cbops.fir_resample[$cbops.function_vector.STRUC_SIZE] =
        $cbops.function_vector.NO_FUNCTION,        // reset function
        &$cbops.fir_resample.amount_to_use,        // amount to use function
        &$cbops.fir_resample.main;                 // main function

.ENDMODULE;

.MODULE $M.cbops.fir_resample_complete;
   .DATASEGMENT DM;

   // ** function vector **
   .VAR $cbops_fir_resample_complete[$cbops.function_vector.STRUC_SIZE] =
      $cbops.function_vector.NO_FUNCTION,          // reset function
      $cbops.function_vector.NO_FUNCTION,          // amount to use function
      $cbops.fir_resample.complete;                // main function

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $cbops.fir_resample.complete
//
// DESCRIPTION:
//    operator main function to complete resampling
//    This operator is the last in the chain.  Reseting $cbops.amount_to_use
//    to correctly advance the input buffer
//
// INPUTS:
//    - r6  = pointer to the list of input and output buffer pointers
//    - r7  = pointer to the list of buffer lengths
//    - r8  = pointer to operator structure
//    - r10 = the number of samples to generate
//
// OUTPUTS:
//    - r8  = pointer to operator structure
//
// TRASHED REGISTERS:
//    everything
//
// *****************************************************************************
.MODULE $M.cbops.fir_resample.complete;
   .CODESEGMENT CBOPS_FIR_RESAMPLE_PM;

$cbops.fir_resample.complete:
    // If Last operator in chain then set $cbops.amount_to_use to advance input buffer
    r4 = M[$M.cbops.fir_resample.amount_used];
    M[$cbops.amount_to_use] = r4;
    rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $$cbops.fir_resample.main
//
// DESCRIPTION:
//    Operator that resamples the input buffer into output buffer
//    using FIR resampling method
//
// INPUTS:
//    - r6 = pointer to the list of input and output buffer pointers
//    - r7 = pointer to the list of buffer lengths
//    - r8 = pointer to operator structure
//    - r10 = the number of samples to process
//
// OUTPUTS:
//    - none
//
// TRASHED REGISTERS:
//    assume everything except r8-r9
//
// *****************************************************************************
.MODULE $M.cbops.fir_resample.main;
   .CODESEGMENT CBOPS_FIR_RESAMPLE_PM;

   $cbops.fir_resample.main:

   // push rLink onto stack
   $push_rLink_macro;

   #ifdef ENABLE_PROFILER_MACROS
      // start profiling if enabled
      r0 = &$cbops.profile_fir_resample;
      call $profiler.start;
   #endif // #ifdef ENABLE_PROFILER_MACROS

   // get the offset to the read buffer to use
   r0 = M[r8 + $cbops.fir_resample.INPUT_INDEX_FIELD];

   // get the input buffer read address
   r1 = M[r6 + r0];
   // store the value in I0
   I0 = r1;
   // get the input buffer length
   r1 = M[r7 + r0];
   // store the value in L0
   L0 = r1;
   #ifdef BASE_REGISTER_MODE
      // get the start address
      r1 = M[r5 + r0];
      push r1;
      pop B0;
   #endif

   // get the offset to the write buffer to use
   r0 = M[r8 + $cbops.fir_resample.OUTPUT_INDEX_FIELD];

   // get the output buffer write address
   r1 = M[r6 + r0];
   // store the value in I4
   I4 = r1;
   // get the output buffer length
   r1 = M[r7 + r0];
   // store the value in L4
   L4 = r1;
   #ifdef BASE_REGISTER_MODE
      // get the start address
      r1 = M[r5 + r0];
      push r1;
      pop B4;
   #endif

   // Check and update amount of input (stereo support)
   r1 = M[$M.cbops.fir_resample.amount_used];
   if NZ r10 = r1;
   M[$M.cbops.fir_resample.amount_used] = r10;

   // if see if we need to always resample
   Null = M[r8 + $cbops.fir_resample.RESAMPLE_UNITY_RATIO_FIELD];
   if NZ jump do_resample;

   // no resampling if fin == fout
   r0  = M[r8 + $cbops.fir_resample.CURRENT_OUTPUT_RATE_FIELD];
   r1  = M[r8 + $cbops.fir_resample.CURRENT_INPUT_RATE_FIELD];
   Null = r0 - r1;
   if NZ jump do_resample;

   // no resampling is needed
   // just copy input to output
   r7 = r10;
   r10 = r10 - 1;
   r0 = M[I0,1];
   do copy_loop;
      r0 = M[I0,1], M[I4,1] = r0;
   copy_loop:
   M[I4,1] = r0;
   jump process_done;

   do_resample:
   // get the hist buf info
   L1 = $cbops.fir_resample.HIST_LENGTH;
   r0 = M[r8 + $cbops.fir_resample.HIST_BUF_FIELD];
   I1 = r0;

   // get the index to coefficient buffer
   r5 = M[r8 + $cbops.fir_resample.COEF_BUF_INDEX_FIELD];
   I3 = r5 + (($cbops.fir_resample.FILTER_LENGTH*$cbops.fir_resample.FILTER_UPRATE)/2);

   // get the conversion ratio (fin/fout)
   // r5 = frac, I2 = int
   r5 = M[r8 + $cbops.fir_resample.CONVERT_RATIO_FRAC_FIELD];
   r3 = M[r8 + $cbops.fir_resample.CONVERT_RATIO_INT_FIELD];
   I5 = r3;

   // load the current interpolation info
   r4 = M[r8 + $cbops.fir_resample.IR_RATIO_FIELD];
   r0 = M[r8 + $cbops.fir_resample.INT_SAMPLES_LEFT_FIELD];
   I2 = r0;

   // set some registers
   M3 = 1;
   r6 = $cbops.fir_resample.FILTER_UPRATE;
   M2 = -r6;
   M0 = r6;

   // r7 = number of output sample generated
   r7 = 0;

   push r8;
   do fractinal_gen_loop;
      r3 = M[I0, 1];
      I2 = I2 - M3, M[I1,1] = r3;
      if POS jump continue_reading;
      generate_output_loop:

         // input is upsampled by rate=r6, find the two adjacent points to interpolate
         rMAC = r4*r6;       // r6=uprate r4=fraction
         r8 = rMAC0;
         r8 = r8 LSHIFT -1;  // r8 = interpolation ratio
         r2 = rMAC1;         // r2 = index of first point

         // FIR filtering starts from right side of coeffs
         I6 = I3 + r2 ;
         r0 = (-$cbops.fir_resample.FILTER_UPRATE-1) - r2;
         M1 = r0 - r2;
         // save I1
         I7 = I1;

         rMAC = 0,              r0 = M[I1,M3], r1 = M[I6,M2]; //1st coeff & sample
         rMAC = rMAC - r0 * r1, r0 = M[I1,M3], r1 = M[I6,M2]; //2nd coeff & sample
         rMAC = rMAC - r0 * r1, r0 = M[I1,M3], r1 = M[I6,M2]; //3rd coeff & sample
         rMAC = rMAC - r0 * r1, r0 = M[I1,M3], r1 = M[I6,M2]; //4th coeff & sample
         rMAC = rMAC - r0 * r1, r0 = M[I1,M3], r1 = M[I6,M2]; //5th coeff & sample
         rMAC = rMAC - r0 * r1, r0 = M[I1,M3], r1 = M[I6,M1]; //6th coeff & sample
         rMAC = rMAC - r0 * r1, r0 = M[I1,M3], r1 = M[I6,M0]; //7th coeff & sample
         rMAC = rMAC - r0 * r1, r0 = M[I1,M3], r1 = M[I6,M0]; //8th coeff & sample
         rMAC = rMAC - r0 * r1, r0 = M[I1,M3], r1 = M[I6,M0]; //9th coeff & sample
         rMAC = rMAC - r0 * r1, r0 = M[I1,M3], r1 = M[I6,M0]; //10th coeff & sample
         rMAC = rMAC - r0 * r1, r0 = M[I1,M3], r1 = M[I6,M0]; //11th coeff & sample
         rMAC = rMAC - r0 * r1, r0 = M[I1,M3], r1 = M[I6,M0]; //12th coeff & sample
         rMAC = rMAC - r0 * r1, r0 = M[I1,M3];

         //calculate second point
         M1 = M1 - 2;
         r2 = r2 + M3;
         //if first point index is (r6-1) then to calculate next point an extra load is required
         if NEG jump no_extra_load;
            // load an extra point
            M1 = r6 - 1;
            r2 = -r6, r1 = M[I1, M3];
         no_extra_load:

         // calculate second point
         I6 = I3 + r2;
         r2 = rMAC;

         r7 = r7 + M3,          r0 = M[I1,M3], r1 = M[I6,M2]; //1st coeff & sample
         rMAC = rMAC + r0 * r1, r0 = M[I1,M3], r1 = M[I6,M2]; //2nd coeff & sample
         rMAC = rMAC + r0 * r1, r0 = M[I1,M3], r1 = M[I6,M2]; //3rd coeff & sample
         rMAC = rMAC + r0 * r1, r0 = M[I1,M3], r1 = M[I6,M2]; //4th coeff & sample
         rMAC = rMAC + r0 * r1, r0 = M[I1,M3], r1 = M[I6,M2]; //5th coeff & sample
         rMAC = rMAC + r0 * r1, r0 = M[I1,M3], r1 = M[I6,M1]; //6th coeff & sample
         rMAC = rMAC + r0 * r1, r0 = M[I1,M3], r1 = M[I6,M0]; //7th coeff & sample
         rMAC = rMAC + r0 * r1, r0 = M[I1,M3], r1 = M[I6,M0]; //8th coeff & sample
         rMAC = rMAC + r0 * r1, r0 = M[I1,M3], r1 = M[I6,M0]; //9th coeff & sample
         rMAC = rMAC + r0 * r1, r0 = M[I1,M3], r1 = M[I6,M0]; //10th coeff & sample
         rMAC = rMAC + r0 * r1, r0 = M[I1,M3], r1 = M[I6,M0]; //11th coeff & sample
         rMAC = rMAC + r0 * r1, r0 = M[I1,M3], r1 = M[I6,M0]; //12th coeff & sample
         rMAC = rMAC + r0 * r1;

         // restore I1
         I1 = I7;
         //linear interpolation between two adjacent points
         rMAC = rMAC*r8;
         rMAC = rMAC - r2*1.0;
         r4 = r4 + r5, M[I4, 1] = rMAC;
         if NEG jump gen_done;
            I2 = I2 + 1;
            r4 = r4 - 1.0;
         gen_done:
         I2 = I2 + I5;
      if NEG jump generate_output_loop;

      continue_reading:
      nop;
   fractinal_gen_loop:
   pop r8;
   // save hist pointer
   r0 = I1;
   M[r8 + $cbops.fir_resample.HIST_BUF_FIELD] = r0;

   r0 = I2;
   M[r8 + $cbops.fir_resample.INT_SAMPLES_LEFT_FIELD] = r0;

   // store the sample counter
   M[r8 + $cbops.fir_resample.IR_RATIO_FIELD] = r4;

   process_done:

   L4 = 0;
   L1 = 0;
   L0 = 0;

   #ifdef BASE_REGISTER_MODE
      // Zero the base registers
      push Null;
      B4 = M[SP-1];
      pop B0;
   #endif

   // Update the number of output samples produced
   M[$cbops.amount_written] = r7;

   // If not Last operator in chain then set M[$cbops.amount_to_use] to amount of output generated
   // Otherwise, set it to the amount of input consumed.
   // Will need completion operator to reset M[$cbops.amount_to_use] if input is not a port
   // and a resampler operator is not the last operator in the chain.
   r1 = M[$M.cbops.fir_resample.amount_used];

   r0 = M[r8 + ($cbops.NEXT_OPERATOR_ADDR_FIELD - $cbops.PARAMETER_AREA_START_FIELD)];
   Null = r0 - $cbops.NO_MORE_OPERATORS;
   if Z r7 = r1;
   M[$cbops.amount_to_use] = r7;

   #ifdef ENABLE_PROFILER_MACROS
      // stop profiling if enabled
      r0 = &$cbops.profile_fir_resample;
      call $profiler.stop;
   #endif
   jump $pop_rLink_and_rts;
.ENDMODULE;

// *****************************************************************************
// MODULE:
//     $cbops.fir_resample.amount_to_use
//
// DESCRIPTION:
//    Sets the transfer based on the minimum of the output space
//    and the available input data
//
// INPUTS:
//    - r5 = the minimum of the number of input samples available and the
//      amount of output space available
//    - r6 = the number of input samples available
//    - r7 = the amount of output space available
//    - r8 = pointer to operator structure:
// OUTPUTS:
//    - r5
//
// TRASHED REGISTERS:
//    r0-r4, rMAC, DivResult
//
// *****************************************************************************
.MODULE $M.cbops.fir_resample.amount_to_use;
   .CODESEGMENT CBOPS_FIR_RESAMPLE_PM;

   $cbops.fir_resample.amount_to_use:

   // Clear Amount to use (stereo support)
   M[$M.cbops.fir_resample.amount_used] = null;

   // default output rate
   r3 = 48000;

   // default input rate
   r4 = 8000;

   // get input sample rate
   r1 = M[r8 + $cbops.fir_resample.INPUT_RATE_ADDR_FIELD];
   if NZ r1 = M[r1];
   if Z r1 = r4;

   // get output sample rate
   r2 = M[r8 + $cbops.fir_resample.OUTPUT_RATE_ADDR_FIELD];
   if NZ r2 = M[r2];
   if Z r2 = r3;

   // new setup if input rate has changed
   r0 = M[r8 + $cbops.fir_resample.CURRENT_INPUT_RATE_FIELD];
   Null = r0 - r1;
   if NZ jump do_set_up;

   // new setup if output rate has changed
   r0 = M[r8 + $cbops.fir_resample.CURRENT_OUTPUT_RATE_FIELD];
   Null = r0 - r2;
   if Z jump set_up_done;
   // setting up the ratios
   do_set_up:
      // store last setup input/output rates
      M[r8 + $cbops.fir_resample.CURRENT_INPUT_RATE_FIELD] = r1;
      M[r8 + $cbops.fir_resample.CURRENT_OUTPUT_RATE_FIELD] = r2;
      rMAC = 0;
      rMAC0 = r1;
      Div = rMAC / r2;
      r0 = DivResult;
      M[r8 + $cbops.fir_resample.CONVERT_RATIO_INT_FIELD] = r0;
      r0 = r0 * r2 (int);
      rMAC0 = r2;
      r1 = r1 - r0;
      rMAC12 = r1 (ZP);
      r2 = r2 ASHIFT 1;
      Div = rMAC / r2;
      r0 = DivResult;
      M[r8 + $cbops.fir_resample.CONVERT_RATIO_FRAC_FIELD] = r0;
   set_up_done:

   // r5 = minimum amount of input data and output space available
   // r6 = amount of input data available
   // r7 = amount of output space available
   // now work out maximum input samples that
   // can be consumed by the operator
   r4 = M[r8 + $cbops.fir_resample.CONVERT_RATIO_FRAC_FIELD];
   r3 = M[r8 + $cbops.fir_resample.CONVERT_RATIO_INT_FIELD];
   r3 = r3 * r7 (int);
   r1 = r7 * r4 (frac);
   r1 = r1 + r3;
   r1 = r1 - 2;
   if NEG r1 = 0;

   // Limit the number of input samples to process to the amount calculated from the output space available
   // (r5 is already limited by the amount of input data available)
   r5 = MIN r1;

   rts;
.ENDMODULE;
