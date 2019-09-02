// *****************************************************************************
// Copyright (c) 2005 - 2016 Qualcomm Technologies International, Ltd.         http://www.csr.com
// Part of ADK 4.1
//
// $Change: 1172207 $  $DateTime: 2011/12/09 20:46:31 $
// *****************************************************************************

#include "stack.h"
#include "cbops.h"

#ifdef KYMERA
#include "cbuffer_asm.h"
#else
#include "cbuffer.h"
#endif

#include "cbops_iir_resamplerv2_op_asm_defs.h"


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
// *****************************************************************************

// Private Library Exports
.PUBLIC $cbops_iir_resamplev2;

.MODULE $M.cbops.iir_resamplev2;
   .DATASEGMENT DM;

   // ** function vector **
   .VAR $cbops_iir_resamplev2[$cbops.function_vector.STRUC_SIZE] =
      $cbops.function_vector.NO_FUNCTION,         // reset function
      &$cbops.iir_resamplev2.amount_to_use,  // amount to use function
      &$cbops.iir_resamplev2.main;           // main function

.ENDMODULE;

// Simplify access to common_struct
#define RSMPL_FILTER_PTR_OFFSET        ($cbops_iir_resamplerv2_op.iir_resampler_op_struct.COMMON_FIELD+$iir_resamplerv2_common.iir_resampler_common_struct.FILTER_FIELD)
#define RSMPL_RESET_OFFSET             ($cbops_iir_resamplerv2_op.iir_resampler_op_struct.COMMON_FIELD+$iir_resamplerv2_common.iir_resampler_common_struct.RESET_FLAG_FIELD)

// *****************************************************************************
// MODULE:
//    $cbops_iir_resamplev2.amount_to_use
//
// DESCRIPTION:
//    operator amount_to_use function for IIR resampler operator
//
// INPUTS:
//    - r5 = the minimum of the number of input samples available and the
//      amount of output space available
//    - r6 = the number of input samples available
//    - r7 = the amount of output space available
//    - r8 = pointer to operator structure
//
// OUTPUTS:
//    none (r5-r8) preserved
//
// TRASHED REGISTERS:
//    r0, r1, r2, r10, I0, I2
//
// *****************************************************************************
.MODULE $M.cbops_iir_resamplev2.amount_to_use;
   .CODESEGMENT PM;


$cbops.iir_resamplev2.amount_to_use:

   // get number of input channels - transform it into addresses, so that we don't do the latter
   // for every channel. We can rely on the fact that in and out channels are of same number.
   r9 = M[r8 + $cbops.param_hdr.NR_INPUT_CHANNELS_FIELD];
   Words2Addr(r9);
   r2 = r9 ASHIFT 1;

   // I0 = address of input channel index 0, I2 = address of output channel index 0
   I0 = r8 + $cbops.param_hdr.CHANNEL_INDEX_START_FIELD;

   // get to the start of COMMON parameters (after the nr in/out and index table "header" part)
   r10 = r2 + $cbops.param_hdr.CHANNEL_INDEX_START_FIELD;
   r10 = r10 + r8;

   // Get resampling configuration.  If NULL perform passthrough operation
   // The RSMPL_FILTER_PTR_OFFSET will get us to filter ptr field inside common params. NOTE that now in multichannel
   // generalisation the cbops_iir_resamplerv2_op.iir_resampler_op_struct.COMMON_FIELD offset is zero and we
   // above did the "skipping" of header part to get to the params.
   r2 = M[r10 + $cbops_iir_resamplerv2_op.iir_resampler_common_struct.FILTER_FIELD];
   if Z rts;

   push rLink;

   r1 = r7;    // r7 is available space
   r0 = r10;

   call $estimate_iir_resampler_consumed;

   // I2 is the output index(es) location
   I2 = I0 + r9;

   // Limit Input
   r5 = r6;
   NULL = r5 - r1;
   if POS r5 = r1;

   // Check Reset flag and if NULL reset.
   Null = M[r10 + $cbops_iir_resamplerv2_op.iir_resampler_common_struct.RESET_FLAG_FIELD];
   if NZ jump $pop_rLink_and_rts;

   // Push op struct ptr
   push r8;

   // History Buffers appended to end of data structure. Must work out location for each channel, the data is
   // structured as [common][ptr to history[[channel1]...[channelN]
   // Start of working1 is at offset N*sizeof(channel params) relative to start of channel-specific part (that comes after the common part
   // and the index header in multichannel scheme). COMMON_FIELD == 0 for multichannel case as it stands.
   r0 = r10;

   // Offset of first channel's param block
   r8 = r10 + $cbops_iir_resamplerv2_op.iir_resampler_op_struct.CHANNEL_FIELD;

   // Pointer to first channel's working data block (it is actually allocated after the channel parameters)
   r4 = M[r10 + $cbops_iir_resamplerv2_op.iir_resampler_op_struct.WORKING_FIELD];

   /* there might be some channels that fall out of use and will not actually be processed later. Don't reset those, save tedium. */
  reset_channel:

   r2 = M[I0,MK1];                // get the input index - sanity checking

   // index may show unused channel for whatever reason
   Null = r2 - $cbops.UNUSED_CHANNEL_IDX;
   if Z jump next_channel;

   r2 = M[I2,MK1];                // get the output index - sanity checking

   // index may show unused channel for whatever reason. This is bombproofing, creator entity should not have
   // left an output index in the air if input index showed channel not in use, but... it makes for a lovely
   // debug saga generator if it twists an ankle here.
   Null = r2 - $cbops.UNUSED_CHANNEL_IDX;
   if Z jump next_channel;

   pushm <I0, I2>;
   push r0;
   call $reset_iir_resampler;
   pop r0;
   popm <I0, I2>;

 next_channel:

   // r4 is working buffer for next channel, so put it to use - and also advance channel params ptr to
   // the next channel's param block.
   r8 = r8 + ($iir_resamplerv2_common.iir_resampler_channel_struct.STRUC_SIZE * ADDR_PER_WORD);

   // countdown - r9 is in addr per word units already
   r9 = r9 - 1*ADDR_PER_WORD;
   if GT jump reset_channel;

   // restore op struct ptr and close shop
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
//    - r4 = pointer to internal framework object
//    - r5 = pointer to the list of input and output buffer start addresses (base_addr)
//    - r6 = pointer to the list of input and output buffer pointers
//    - r7 = pointer to the list of buffer lengths
//    - r8 = pointer to operator structure:
//    - r10 = data to process
//
// OUTPUTS:
//    - none
//
// TRASHED REGISTERS:
//    everything
//
// *****************************************************************************
.MODULE $M.cbops_iir_resamplev2.main;      // Upsample
   .CODESEGMENT PM;
   .DATASEGMENT DM;

$cbops.iir_resamplev2.main:

   // Make sure there is something to process
   NULL = r10;
   if LE rts;

   pushm <FP(=SP), rLink>;
   pushm <r4, r5, r6, r7, r8, r10>;   // save cbops fw ptrs, buffer start, rd/wr and size table ptrs, op struct ptr

   // just to really make anyone realise one has to change the offsets if changed stack pile, so avoid magic numbers
   .CONST $cbops_iir_resamplev2.BASE_PTRS                      3*ADDR_PER_WORD;                                          // r5
   .CONST $cbops_iir_resamplev2.RDWR_PTRS                      $cbops_iir_resamplev2.BASE_PTRS + ADDR_PER_WORD;          // r6
   .CONST $cbops_iir_resamplev2.LENGTH_PTRS                    $cbops_iir_resamplev2.RDWR_PTRS + ADDR_PER_WORD;          // r7
   .CONST $cbops_iir_resamplev2.PARAM_STRUCT_ADDR              $cbops_iir_resamplev2.LENGTH_PTRS + ADDR_PER_WORD;        // r8
   .CONST $cbops_iir_resamplev2.AMOUNT_TO_PROCESS              $cbops_iir_resamplev2.PARAM_STRUCT_ADDR + ADDR_PER_WORD;  // r10

   M0 = MK1;

   // get number of input channels - transform it into addresses, so that we don't do the latter
   // for every channel. We can rely on the fact that in and out channels are of same number.
   r9 = M[r8 + $cbops.param_hdr.NR_INPUT_CHANNELS_FIELD];
   Words2Addr(r9);
   r3 = r9 ASHIFT 1;

   // I0 = address of input channel index 0, I2 = address of output channel index 0
   I0 = r8 + $cbops.param_hdr.CHANNEL_INDEX_START_FIELD;
   I2 = I0 + r9;

   // get to the start of COMMON parameters (after the nr in/out and index table "header" part)
   r0 = r3 + $cbops.param_hdr.CHANNEL_INDEX_START_FIELD;
   r0 = r0 + r8;

   // Offset of first channel's param block
   r8 = r0 + $cbops_iir_resamplerv2_op.iir_resampler_op_struct.CHANNEL_FIELD;

   // Pointer to first channel's working data block (it is actually allocated after the channel parameters)
   r4 = M[r0 + $cbops_iir_resamplerv2_op.iir_resampler_op_struct.WORKING_FIELD];

 process_channel:

   // Setup input & output
   r2 = M[I0,M0];                // get the input index

   // index may show unused channel
   Null = r2 - $cbops.UNUSED_CHANNEL_IDX;
   if Z jump next_channel;

   Words2Addr(r2);               // r2 = input_index (arch4: in addrs)

   // get our buffer info table pointers
   r5 = M[FP + $cbops_iir_resamplev2.BASE_PTRS];
   r6 = M[FP + $cbops_iir_resamplev2.RDWR_PTRS];
   r7 = M[FP + $cbops_iir_resamplev2.LENGTH_PTRS];

   r1 = M[r6 + r2];
   I1 = r1, r3 = M[I2,M0];       // r3 = output index
   r1 = M[r7 + r2];
   L1 = r1;
   r1 = M[r5 + r2];
   push r1;
   pop B1;

   // output index, in theory, may show unused channel if creator entity screwed up something.
   // If we absolutely trust the creator,
   // then we save some tedium by not checking it, hoping that no output goes missing if
   // corresponding input is there - but the risk is a mushroom cloud being produced and then debugging sagas.
   Null = r3 - $cbops.UNUSED_CHANNEL_IDX;
   if Z jump next_channel;

   Words2Addr(r3);               // output_index (arch4: in addrs)

   r1 = M[r6 + r3];
   I5 = r1;
   r1 = M[r7 + r3];
   L5 = r1;
   r1 = M[r5 + r3];
   push r1;
   pop B5;

   // History Buffers appended to end of data structure
   // r4 = working data for this channel, r0 = start of common params, r8 = start of this channel's param block;
   // salvage latter two, while r4 will advance to next block of working data.
   // Processing same amount on all channels - hence reload precious r10.
   r10 = M[FP + $cbops_iir_resamplev2.AMOUNT_TO_PROCESS];

   pushm <r0, r8, r9>;
   pushm <I0, I2>;
   call $iir_perform_resample;
   popm <I0, I2>;
   popm <r0, r8, r9>;

 next_channel:
   // r4 is WORKING buffer for next channel, use it to progress in next iteration,
   // and also get to next channel's param block in r8
   r8 = r8 + ($iir_resamplerv2_common.iir_resampler_channel_struct.STRUC_SIZE * ADDR_PER_WORD);

   // We move to next channel. In the case of this cbop, it is enough to
   // count based on input channels here.
   r9 = r9 - 1*ADDR_PER_WORD;

   // gobbled up all channels?
   if GT jump process_channel;

 jp_done:
   // Record amount produced (r7) - this is now same across all channels, so taking just last one
   r0 = r7;
   popm <r4, r5, r6, r7, r8, r10>;

   M[r4 + $cbops.fw.AMOUNT_WRITTEN_FIELD] = r0;

   popm <FP, rLink>;
   rts;

.ENDMODULE;
// Expose the location of this table to C
.set $_cbops_iir_resampler_table,  $cbops_iir_resamplev2








