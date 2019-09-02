// *****************************************************************************
// Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
//
// *****************************************************************************

#include "cbops.h"
#include "core_library.h"
#include "iir_resamplev2_header.h"

// *****************************************************************************
// NAME:
//    IIR resampler cbops operator
//
// DESCRIPTION:
//    This operator uses an IIR and a FIR filter combination to
//    perform sample rate conversion.  The utilization of the IIR
//    filter allows a lower order FIR filter to be used to obtain
//    an equivalent frequency response.  The result is that the
//    IIR resampler uses less MIPs than the current polyphase FIR method.
//    It also provides a better frequency response.
//
//    To further reduce coefficients for a given resampling up to two
//    complete filter stages are supported.  The configurations include.
//          IIR --> FIR(10)
//          FIR(10) --> IIR
//          FIR(6) --> FIR(10) --> IIR
//          FIR(6) --> IIR --> FIR(10)
//          IIR --> FIR(10) --> IIR --> FIR(10)
//          IIR --> FIR(10) --> FIR(10) --> IIR
//          FIR(10) --> IIR --> IIR --> FIR(10)
//          FIR(10) --> IIR --> FIR(10) --> IIR
//
//    The IIR filter may be from 9th order to 19 order.
//
//    The FIR filters are implemented in a polyphase configuration. The FIR(6)
//    filter uses a 6th order polyphase kernal and the FIR(10) filter uses a
//    10th order polyphase kernal.  The filters are symetrical so only half the
//    coefficients need to be stored.
//
//    The operator utilizes its own history buffers.  As a result the input and/or
//    output may be a port.  Also, for downsampling, in-place operation is supported.
//
//    If the resampler operator is not the last operator in the cbops chain and the
//    input is not a port, the "$cbops.iir_resamplev2_complete" must be included at
//    the end of the operator chain.  The completion operator resets M[$cbops.amount_to_use]
//    to the amount of input processed so the input cBuffer can be correctly advanced.
//
// When using the operator the following data structure is used:
//
//  $iir_resamplev2.INPUT_1_START_INDEX_FIELD    = The index of the input
//       buffer
//
//  $iir_resamplev2.OUTPUT_1_START_INDEX_FIELD   = The index of the output
//       buffer
//
//  $iir_resamplev2.FILTER_DEFINITION_PTR_FIELD  = Pointer to configuration
//       object defining the supported sample rate conversions.  These objects are
//       constants in defined iir_resamplev2_coefs.asm
//
//  $iir_resamplev2.INPUT_SCALE_FIELD = A power of 2 scale factor applied
//       to the input signal to scale it to a Q8.15 resolution (i.e 16-bits).
//
//  $iir_resamplev2.OUTPUT_SCALE_FIELD  = A power of 2 scale factor applied
//       to the output signal to scale it from the internally used Q8.15 resolution
//
//  $iir_resamplev2.INTERMEDIATE_CBUF_PTR_FIELD = Pointer to a temporary buffer
//      used between stages.  This buffer may be allocaed from scratch memory.  If the
//      desired resampling is single stage this pointer may be NULL.
//
//  $iir_resamplev2.INTERMEDIATE_CBUF_LEN_FIELD = Length of temporary buffer
//
//  $iir_resamplev2.PARTIAL1_FIELD = Internal parameter used for 1st stage
//      downsampling
//
//  $iir_resamplev2.SAMPLE_COUNT1_FIELD = Internal parameter used for 1st stage
//      polyphase tracking
//
//  $iir_resamplev2.FIR_HISTORY_BUF1_PTR_FIELD = Pointer for 1st stage FIR filter.
//      For elvis this is a seperate circular buffer.  Otherewise, it is allocated
//      from the end of this data structure in the reset operation.  If the
//      desired resampling is single stage this pointer may be NULL.
//
//  $iir_resamplev2.IIR_HISTORY_BUF1_PTR_FIELD  = Pointer for 1st stage IIR filter.
//      For elvis this is a seperate circular buffer.  Otherewise, it is allocated
//      from the end of this data structure in the reset operation.  If the
//      desired resampling is single stage this pointer may be NULL.
//
//  $iir_resamplev2.PARTIAL2_FIELD  = Internal parameter used for 2nd stage
//      downsampling
//
//  $iir_resamplev2.SAMPLE_COUNT2_FIELD = Internal parameter used for 2nd stage
//      polphase tracking
//
//  $iir_resamplev2.FIR_HISTORY_BUF2_PTR_FIELD  = Pointer for 2nd stage FIR filter.
//      For elvis this is a seperate circular buffer.  Otherewise, it is allocated
//      from the end of this data structure in the reset operation.
//
//  $iir_resamplev2.IIR_HISTORY_BUF2_PTR_FIELD  = Pointer for 1st stage IIR filter.
//      For elvis this is a seperate circular buffer.  Otherewise, it is allocated
//      from the end of this data structure in the reset operation.
//
//  $iir_resamplev2.RESET_FLAG_FIELD = Zero to indicate that the resampler needs
//      to be re-initialied
//
// *****************************************************************************


.MODULE $M.cbops.iir_resamplev2;
   .DATASEGMENT DM;

    .VAR amount_used;

   // ** function vector **
   .VAR $cbops_iir_resamplev2[$cbops.function_vector.STRUC_SIZE] =
      $cbops.function_vector.NO_FUNCTION,         // reset function
      &$cbops.iir_resamplev2.amount_to_use,  // amount to use function
      &$cbops.iir_resamplev2.main;           // main function

.ENDMODULE;

.MODULE $M.cbops.iir_resamplev2_complete;
   .DATASEGMENT DM;

   // ** function vector **
   .VAR $cbops_iir_resamplev2_complete[$cbops.function_vector.STRUC_SIZE] =
      $cbops.function_vector.NO_FUNCTION,         // reset function
      $cbops.function_vector.NO_FUNCTION,         // amount to use function
      $cbops.iir_resamplev2.complete;             // main function

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $cbops.iir_resamplev2.complete
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
.MODULE $M.cbops.iir_resamplev2.complete;
   .CODESEGMENT IIR_RESAMPLEV2_CBOPS_PM;
$cbops.iir_resamplev2.complete:
    // If Last operator in chain then set $cbops.amount_to_use to advance input buffer
    r4 = M[$M.cbops.iir_resamplev2.amount_used];
    M[$cbops.amount_to_use] = r4;
    rts;

.ENDMODULE;


// *****************************************************************************
// MODULE:
//    $cbops_iir_resamplev2.amount_to_use
//
// DESCRIPTION:
//    operator amount_to_use function for IIR resampler operator
//
// INPUTS:
//    - r5 = amount of input data to use
//    - r6 = minimum available input amount
//    - r7 = minimum available output amount
//    - r8 = pointer to operator structure
//
// OUTPUTS:
//    - r5 = amount of input data to use
//    - r8  = pointer to operator structure
//
// TRASHED REGISTERS:
//    r0, r1, r2, r10, I0
//
// *****************************************************************************
.MODULE $M.cbops_iir_resamplev2.amount_to_use;
   .CODESEGMENT IIR_RESAMPLEV2_CBOPS_PM;

$cbops.iir_resamplev2.amount_to_use:
   // Clear Amount to use (stereo support)
   M[$M.cbops.iir_resamplev2.amount_used]=NULL;

$iir_resamplev2.amount_to_use:
   // Get resampling configuration
   r2 = M[r8 + $iir_resamplev2.FILTER_DEFINITION_PTR_FIELD];
   if Z rts;
   
   // Push return Address onto stack
   push rLink;

   r0 = r8 + $iir_resamplev2.COMMON_FIELD;
   r1 = r7;    // r7 is available space
   call $estimate_iir_resampler_consumed;
   // Limit Input
   r5 = r6;
   NULL = r5 - r1;
   if POS r5 = r1;

   // Check for REset
   Null = M[r8 + $iir_resamplev2.RESET_FLAG_FIELD];
   if NZ jump $pop_rLink_and_rts;
   push r8;
   r4 = r8 + $iir_resamplev2.WORKING_FIELD;
   r0 = r8 + $iir_resamplev2.COMMON_FIELD;
   r8 = r8 + $iir_resamplev2.CHANNEL_FIELD;
   call $reset_iir_resampler;
   pop r8;

   jump $pop_rLink_and_rts;
.ENDMODULE;



// *****************************************************************************
// MODULE:
//    $cbops_iir_resamplev2.main
//
// DESCRIPTION:
//    operator main function for IIR resampler operator
//
// INPUTS:
//    - r6  = pointer to the list of input and output buffer pointers
//    - r7  = pointer to the list of buffer lengths
//    - r8  = pointer to operator structure
//    - r10 = the number of samples to process
//
// OUTPUTS:
//    - r8  = pointer to operator structure
//
// TRASHED REGISTERS:
//    everything
//
// *****************************************************************************
.MODULE $M.cbops_iir_resamplev2.main;      // Upsample
   .CODESEGMENT IIR_RESAMPLEV2_CBOPS_PM;
   .DATASEGMENT DM;

$cbops.iir_resamplev2.main:
   $push_rLink_macro;

   M0 = 1;
   I0 = r8;
   push r8,             r0=M[I0,M0];      //INPUT_1_START_INDEX_FIELD
   r1 = M[r6 + r0];
   I1 = r1,             r4=M[I0,M0];      //OUTPUT_1_START_INDEX_FIELD
   r1 = M[r7 + r0];
   L1 = r1;
   r1 = M[r6 + r4];
   I5 = r1;
   r1 = M[r7 + r4];
   L5 = r1;
#ifdef BASE_REGISTER_MODE 
   // Set base registers for IO
   r1 = M[r5 + r0];
   push r1;
   pop  B1;
   r1 = M[r5 + r4];
   push r1;
   pop  B5;
#endif  
   
   // r8 - operarator params
   // r10 - Amount of Input
   // I1,L1,B1 - Input buffers
   // I5,L5,B5 - Output Buffers

   // Check and update amount of input (stereo support)
   r1 = M[$M.cbops.iir_resamplev2.amount_used];
   if NZ r10=r1;
   M[$M.cbops.iir_resamplev2.amount_used] = r10;
   
   // Perform Resampling
   r0 = r8 + $iir_resamplev2.COMMON_FIELD;
   r4 = r8 + $iir_resamplev2.WORKING_FIELD;
   r8 = r8 + $iir_resamplev2.CHANNEL_FIELD;   
   call $iir_perform_resample;   
   // Restore Data Object
   pop r8;

   // number of output samples generated
   M[$cbops.amount_written] = r7;

   // If not Last operator in chain then set M[$cbops.amount_to_use] to amount of output generated
   // Othwerise, set it to the amount of input consumed.
   // Will need completion operator to reset M[$cbops.amount_to_use] if input is not a port
   // and a resampler operator is not the last operator in the chain.
   r1 = M[$M.cbops.iir_resamplev2.amount_used];

   r0 = M[r8 + ($cbops.NEXT_OPERATOR_ADDR_FIELD - $cbops.PARAMETER_AREA_START_FIELD)];
   Null = r0 - $cbops.NO_MORE_OPERATORS;
   if Z r7=r1;
   M[$cbops.amount_to_use] = r7;

   // pop rLink from stack
   jump $pop_rLink_and_rts;


.ENDMODULE;





