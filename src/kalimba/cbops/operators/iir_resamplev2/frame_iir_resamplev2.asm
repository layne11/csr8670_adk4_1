// *****************************************************************************
// Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
//
// *****************************************************************************

// *****************************************************************************
// NAME:
//    IIR resampler framesync Processing Module
//
// DESCRIPTION:
//    This processing mdoule uses an IIR and a FIR filter combination to
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
//    The operator utilizes its own history buffers and in-place operation is supported
//    for downsampling.  Because frame processing is used a fixed frame size only
//    resampling ratios where the numerator or denominator is one are supported.
//      Such as 1:2, 2:1, 1:4, etc.
//    As the frame size is fixed available data/space in buffers is not validated
//
// When using the operator the following data structure is used:
//
//  $cbops.frame.resamplev2.INPUT_PTR_FIELD = input buffer pointer
//
//  $cbops.frame.resamplev2.INPUT_LENGTH_FIELD  = Input Buffer Length
//
//  $cbops.frame.resamplev2.OUTPUT_PTR_FIELD  = Output Buffer Pointer
//
//  $cbops.frame.resamplev2.OUTPUT_LENGTH_FIELD = Output Buffer Length
//
//  $cbops.frame.resamplev2.NUM_SAMPLES_FIELD = Number of samples to consume
//
//  $cbops.frame.resamplev2.FILTER_DEFINITION_PTR_FIELD  = Pointer to configuration
//       object defining the supported sample rate conversions.  These objects are
//       constants in defined iir_resamplev2_coefs.asm
//
//  $cbops.frame.resamplev2.INPUT_SCALE_FIELD = A power of 2 scale factor applied
//       to the input signal to scale it to a Q8.15 resolution (i.e 16-bits).
//
//  $cbops.frame.resamplev2.OUTPUT_SCALE_FIELD  = A power of 2 scale factor applied
//       to the output signal to scale it from the internally used Q8.15 resolution
//
//  $cbops.frame.resamplev2.INTERMEDIATE_CBUF_PTR_FIELD = Pointer to a temporary buffer
//      used between stages.  This buffer be allocaed from scratch memory.  If the
//      desired resampling is single stage this pointer may be NULL.
//
//  $cbops.frame.resamplev2.INTERMEDIATE_CBUF_LEN_FIELD = Length of temporary buffer
//
//  $cbops.frame.resamplev2.PARTIAL1_FIELD = Internal parameter used for 1st stage
//      downsampling
//
//  $cbops.frame.resamplev2.SAMPLE_COUNT1_FIELD = Internal parameter used for 1st stage
//      polphase tracking
//
//  $cbops.frame.resamplev2.FIR_HISTORY_BUF1_PTR_FIELD = Pointer for 1st stage FIR filter.
//      For elvis this is a seperate circular buffer.  Otherewise, it is allocated
//      from the end of this data structure in the reset operation.  If the
//      desired resampling is single stage this pointer may be NULL.
//
//  $cbops.frame.resamplev2.IIR_HISTORY_BUF1_PTR_FIELD  = Pointer for 1st stage IIR filter.
//      For elvis this is a seperate circular buffer.  Otherewise, it is allocated
//      from the end of this data structure in the reset operation.  If the
//      desired resampling is single stage this pointer may be NULL.
//
//  $cbops.frame.resamplev2.PARTIAL2_FIELD  = Internal parameter used for 2nd stage
//      downsampling
//
//  $cbops.frame.resamplev2.SAMPLE_COUNT2_FIELD = Internal parameter used for 2nd stage
//      polphase tracking
//
//  $cbops.frame.resamplev2.FIR_HISTORY_BUF2_PTR_FIELD  = Pointer for 2nd stage FIR filter.
//      For elvis this is a seperate circular buffer.  Otherewise, it is allocated
//      from the end of this data structure in the reset operation.
//
//  $cbops.frame.resamplev2.IIR_HISTORY_BUF2_PTR_FIELD  = Pointer for 1st stage IIR filter.
//      For elvis this is a seperate circular buffer.  Otherewise, it is allocated
//      from the end of this data structure in the reset operation.
//
//  $cbops.frame.resamplev2.RESET_FLAG_FIELD = Zero to indicate that the resampler needs
//      to be re-initialied
//
// *****************************************************************************
#include "cbuffer.h"
#include "stack.h"
#include "iir_resamplev2_header.h"

// *****************************************************************************
// MODULE:
//    $frame.iir_resamplev2.Initialize
//
// DESCRIPTION:
//    Initialize function for IIR resampler processing module
//
// INPUTS:
//    - r7 = pointer to module structure
//
// OUTPUTS:
//    none
//
// TRASHED REGISTERS:
//    all
//
// *****************************************************************************
.MODULE $M.frame.iir_resamplev2.initialize;
   .CODESEGMENT IIR_RESAMPLEV2_FRAMESYNC_PM;

   $frame.iir_resamplev2.Initialize:

   /* History Buffers appended to end of data structure. This does not use multi-channel framework,
    * therefore can directly get to the common, channel and working fields in the param struct.
    * It always works with a single in and out buffer.
    */
    
   // Get resampling configuration
   r3 = M[r7 + $cbops.frame.resamplev2.FILTER_DEFINITION_PTR_FIELD];
   if Z rts;


   M[r7 + $cbops.frame.resamplev2.RESET_FLAG_FIELD] = r3;
   
   r4 = r7 + $cbops.frame.resamplev2.WORKING_FIELD;
   r0 = r7 + $cbops.frame.resamplev2.COMMON_FIELD;
   r8 = r7 + $cbops.frame.resamplev2.CHANNEL_FIELD;
   jump $reset_iir_resampler;

.ENDMODULE;


// *****************************************************************************
// MODULE:
//    $frame.iir_resamplev2.process
//
// DESCRIPTION:
//    Process function for IIR resampler processing module
//
// INPUTS:
//    - r7  = pointer to module structure
//
// OUTPUTS:
//    none
//
// TRASHED REGISTERS:
//    everything
//
// *****************************************************************************

.MODULE $M.frame.iir_resamplev2.process;
   .CODESEGMENT IIR_RESAMPLEV2_FRAMESYNC_PM;

   $frame.iir_resamplev2.Process:

   $push_rLink_macro;
   M0 = 1;
   I0 = r7;
   r0=M[I0,M0];      //INPUT_PTR_FIELD
   // Get Input Buffer
   I1  = r0,            r1=M[I0,M0];      //INPUT_LENGTH_FIELD
   L1  = r1,            r0=M[I0,M0];      //OUTPUT_PTR_FIELD
   // Get output buffer
   I5 = r0,             r1=M[I0,M0];      //OUTPUT_LENGTH_FIELD
   L5 = r1,             r0=M[I0,M0];      //NUM_SAMPLES_FIELD
   r10 = r0;

//    r0 = Pointer to common struct
//    r8 - Pointer to channel structure
//    r4 - Pointer to history buffers

   /* History Buffers appended to end of data structure. This does not use multi-channel framework,
    * therefore can directly get to the common, channel and working fields in the param struct.
    * It always works with a single in and out buffer.
    */
   r4 = r7 + $frame_iir_resamplerv2.iir_resampler_op_struct.WORKING_FIELD;
   r0 = r7 + $cbops.frame.resamplev2.COMMON_FIELD;
   r8 = r7 + $cbops.frame.resamplev2.CHANNEL_FIELD;
   call $iir_perform_resample;

   // Clear Circular Registsers
   L1 = NULL;
   L5 = NULL;

   push NULL;
   B0 = M[SP-1];
   B1 = M[SP-1];
   B4 = M[SP-1];
   pop   B5;

   // pop rLink from stack
   jump $pop_rLink_and_rts;


.ENDMODULE;


