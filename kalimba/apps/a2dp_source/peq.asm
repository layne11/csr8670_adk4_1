// *****************************************************************************
// Copyright (c) 2015 - 2016 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
//
// *****************************************************************************

#ifdef WOODSTOCK
    #define USES_PEQ
#endif // WOODSTOCK

#ifdef USES_PEQ

#include "stack.h"
#include "peq.h"
#include "mv_codec_encoder_common.h"

#ifdef WOODSTOCK
    #if (MAX_NUM_PEQ_STAGES != 5)
        .error WOODSTOCK MODS REQUIRE NUMBER OF PEQ STAGES TO BE 5
    #endif
#endif // WOODSTOCK

.MODULE $M.peq;
   .DATASEGMENT DM;

.VAR  ZeroValue = 0;
.VAR  OneValue = 1.0;

declare_cbuf_and_struc($peq_in_left, $peq_in_left_cbuffer_struc, AUDIO_CBUFFER_SIZE)
declare_cbuf_and_struc($peq_in_right, $peq_in_right_cbuffer_struc, AUDIO_CBUFFER_SIZE)

.VAR/DM2CIRC left_peq_delaybuf_dm2[2 * (MAX_NUM_PEQ_STAGES + 1)];
.VAR/DM2CIRC right_peq_delaybuf_dm2[2 * (MAX_NUM_PEQ_STAGES + 1)];

.VAR/DM1CIRC peq_coeffs1[6 * MAX_NUM_PEQ_STAGES] =
#ifdef WOODSTOCK
   0x000000, 0x000000, 0x7fffff, 0x000000, 0x000000,
   0x000000, 0x000000, 0x7fffff, 0x000000, 0x000000,
   0x000000, 0x000000, 0x7fffff, 0x000000, 0x000000,
   0x000000, 0x000000, 0x7fffff, 0x000000, 0x000000,
   0x000000, 0x000000, 0x7fffff, 0x000000, 0x000000,
   0,0,0,0,0;
#else // WOODSTOCK
   0x33E7EF,
   0x8CAB31,
   0x42116A,
   0x35F95A,
   0x8CAB31,
   0x2BE882,
   0x9AF23F,
   0x4127C1,
   0x2D1044,
   0x9AF23F,
   0x1FC756,
   0xB6E2D3,
   0x3FFFFF,
   0x1FC756,
   0xB6E2D3,
   0x1D7F9A,
   0xD2A8DE,
   0x7FFFFF,
   0x1D7F9A,
   0xD2A8DE,
   0xE658C4,
   0x465179,
   0x7FFFFF,
   0xE658C4,
   0x465179,
   1,1,1,0,0;
#endif // WOODSTOCK

.VAR $left_peq_struc[$mv_dongle.peq.STRUC_SIZE] =
   0,                                    // PTR_INPUT_DATA_BUFF_FIELD
   0,                                    // INPUT_CIRCBUFF_SIZE_FIELD
   0,                                    // PTR_OUTPUT_DATA_BUFF_FIELD
   0,                                    // OUTPUT_CIRCBUFF_SIZE_FIELD
   left_peq_delaybuf_dm2,               // PTR_DELAY_LINE_FIELD
   peq_coeffs1,                         // PTR_COEFS_BUFF_FIELD
   MAX_NUM_PEQ_STAGES,                   // NUM_STAGES_FIELD
   0,                                    // DELAY_BUF_SIZE
   0,                                    // COEFF_BUF_SIZE
   0,                                    // BLOCK_SIZE_FIELD
   peq_coeffs1+5*MAX_NUM_PEQ_STAGES,    // PTR_SCALE_BUFF_FIELD
   ZeroValue,                           // INPUT_GAIN_EXPONENT_PTR
   OneValue;                            // INPUT_GAIN_MANTISSA_PTR

.VAR $right_peq_struc[$mv_dongle.peq.STRUC_SIZE] =
   0,                                    // PTR_INPUT_DATA_BUFF_FIELD
   0,                                    // INPUT_CIRCBUFF_SIZE_FIELD
   0,                                    // PTR_OUTPUT_DATA_BUFF_FIELD
   0,                                    // OUTPUT_CIRCBUFF_SIZE_FIELD
   right_peq_delaybuf_dm2,              // PTR_DELAY_LINE_FIELD
   peq_coeffs1,                         // PTR_COEFS_BUFF_FIELD
   MAX_NUM_PEQ_STAGES,                   // NUM_STAGES_FIELD
   0,                                    // DELAY_BUF_SIZE
   0,                                    // COEFF_BUF_SIZE
   0,                                    // BLOCK_SIZE_FIELD
   peq_coeffs1+5*MAX_NUM_PEQ_STAGES,    // PTR_SCALE_BUFF_FIELD
   ZeroValue,                           // INPUT_GAIN_EXPONENT_PTR
   OneValue;                            // INPUT_GAIN_MANTISSA_PTR

.ENDMODULE;

// $****************************************************************************
// NAME:
//    Audio Processing Library PEQ Module (version 2.0.0)
//
// DESCRIPTION:
//    Parametric Equaliser based on multi-stage biquad filter
//
// MODULES:
//    $mv_dongle.peq.initialize
//    $mv_dongle.peq.process
// *****************************************************************************
.MODULE $M.mv_dongle.peq.initialize;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

$mv_dongle.peq.initialize:

   //number of stages
   r0 = M[r7 + $mv_dongle.peq.NUM_STAGES_FIELD];

   // size of delay buffer = (num_stage+1)*2
   r1 = r0 + 1;
   r1 = r1 ASHIFT 1;
   M[r7 + $mv_dongle.peq.DELAYLINE_SIZE_FIELD] = r1;

   // size of coef buffer = (num_stage) * 5
   r1 = r0 ASHIFT 2;
   r1 = r1 + r0;
   M[r7 + $mv_dongle.peq.COEFS_SIZE_FIELD] = r1;

   // Initialise delay buffer to zero
   r1 = M[r7 + $mv_dongle.peq.DELAYLINE_ADDR_FIELD];
   I0 = r1;
   r10 = M[r7 + $mv_dongle.peq.DELAYLINE_SIZE_FIELD];
   // to zero the delay buffer
   r0 = 0;
   do init_dly_ln_loop;
      M[I0, 1] = r0;
   init_dly_ln_loop:

   rts;
.ENDMODULE;

.MODULE $M.mv_dongle.peq.caller;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   $mv_dongle.peq.caller:

   $push_rLink_macro;

   // PEQ the left channel

   r7 = $left_peq_struc;

   r0 = $peq_in_left_cbuffer_struc;
   call $cbuffer.get_read_address_and_size;
   M[r7 + $mv_dongle.peq.INPUT_ADDR_FIELD] = r0;
   M[r7 + $mv_dongle.peq.INPUT_SIZE_FIELD] = r1;

   r0 = $audio_in_left_cbuffer_struc;
   call $cbuffer.get_write_address_and_size;
   M[r7 + $mv_dongle.peq.OUTPUT_ADDR_FIELD] = r0;
   M[r7 + $mv_dongle.peq.OUTPUT_SIZE_FIELD] = r1;

   r0 = $peq_in_left_cbuffer_struc;
   call $cbuffer.calc_amount_data;
   r4 = r0;
   r0 = $audio_in_left_cbuffer_struc;
   call $cbuffer.calc_amount_space;
   NULL = r0 - r4;
   if GT r0 = r4;

   // Keep each copy amount within a reasonable range
   NULL = r0 - $PEQ_COPY_MINIMUM;
   if LT jump right_channel;
   NULL = r0 - $PEQ_COPY_MAXIMUM;
   if LT jump skip_overwrite_1;
      r0 = $PEQ_COPY_MAXIMUM;
   skip_overwrite_1:

   M[r7 + $mv_dongle.peq.BLOCK_SIZE_FIELD] = r0;

   call $mv_dongle.peq.process;

   //r7 = $left_peq_struc;

   r1 = M[r7 + $mv_dongle.peq.INPUT_ADDR_FIELD];
   r0 = $peq_in_left_cbuffer_struc;
   call $cbuffer.set_read_address;

   r1 = M[r7 + $mv_dongle.peq.OUTPUT_ADDR_FIELD];
   r0 = $audio_in_left_cbuffer_struc;
   call $cbuffer.set_write_address;

   // PEQ the right channel
right_channel:
   r7 = $right_peq_struc;

   r0 = $peq_in_right_cbuffer_struc;
   call $cbuffer.get_read_address_and_size;
   M[r7 + $mv_dongle.peq.INPUT_ADDR_FIELD] = r0;
   M[r7 + $mv_dongle.peq.INPUT_SIZE_FIELD] = r1;

   r0 = $audio_in_right_cbuffer_struc;
   call $cbuffer.get_write_address_and_size;
   M[r7 + $mv_dongle.peq.OUTPUT_ADDR_FIELD] = r0;
   M[r7 + $mv_dongle.peq.OUTPUT_SIZE_FIELD] = r1;

   r0 = $peq_in_right_cbuffer_struc;
   call $cbuffer.calc_amount_data;
   r4 = r0;
   r0 = $audio_in_right_cbuffer_struc;
   call $cbuffer.calc_amount_space;
   NULL = r0 - r4;
   if GT r0 = r4;

   // Keep each copy amount within a reasonable range
   NULL = r0 - $PEQ_COPY_MINIMUM;
   if LT jump all_done;
   NULL = r0 - $PEQ_COPY_MAXIMUM;
   if LT jump skip_overwrite_2;
      r0 = $PEQ_COPY_MAXIMUM;
   skip_overwrite_2:

   M[r7 + $mv_dongle.peq.BLOCK_SIZE_FIELD] = r0;

   call $mv_dongle.peq.process;

   r1 = M[r7 + $mv_dongle.peq.INPUT_ADDR_FIELD];
   r0 = $peq_in_right_cbuffer_struc;
   call $cbuffer.set_read_address;

   r1 = M[r7 + $mv_dongle.peq.OUTPUT_ADDR_FIELD];
   r0 = $audio_in_right_cbuffer_struc;
   call $cbuffer.set_write_address;

all_done:
   jump $pop_rLink_and_rts;

.ENDMODULE;

.MODULE $M.mv_dongle.peq.process;
   .CODESEGMENT   PM;
   .DATASEGMENT DM;

$mv_dongle.peq.process:

   M1 = 1;
   I3 = r7;
   M0 = -1;
   // Read INPUT_ADDR_FIELD
   r0 = M[I3,M1];
   // I4 = ptr to ip buffer,   Read INPUT_SIZE_FIELD
   I4 = r0, r0 = M[I3,M1];
   // L4 = length of buffer,   Read OUTPUT_ADDR_FIELD
   L4 = r0, r0 = M[I3,M1];
   // I0 = ptr to op buffer,   Read OUTPUT_SIZE_FIELD
   I0 = r0, r0 = M[I3,M1];
   // L0 = length of buffer,   Read DELAYLINE_ADDR_FIELD
   L0 = r0, r0 = M[I3,M1];
   // I5 = ptr to delay line,  Read COEFS_ADDR_FIELD
   I5 = r0, r0 = M[I3,M1];
   // I1 = ptr to coefs buffer,Read NUM_STAGES_FIELD
   I1 = r0, r1 = M[I3,M1];
   // M2 = num stages,        Read DELAYLINE_SIZE_FIELD
   M2 = r1, r1 = M[I3,M1];
   // L5 = delay buffer size, Read COEFS_SIZE_FIELD
   L5 = r1, r1 = M[I3,M1];
   // L1 = coeff buffer size, Read BLOCK_SIZE_FIELD
   L1 = r1, r4 = M[I3,M1];
   // Read SCALING_ADDR_FIELD
   r10 = Null, r1 = M[I3,M1];
   // I2 = scale buffer,      Read GAIN_EXPONENT_ADDR_FIELD
   I2 = r1 + M2, r1 = M[I3,M1];
   // needed for bug in index feed forward, M2 = -num stages
   M2 = Null - M2;
   // INPUT_GAIN_EXPONENT
   r6 = M[r1];
   // Add 2-bit head room     Read GAIN_MANTISA_ADDR_FIELD
   r6 = r6 + M0, r5 = M[I3,M1];
   r6 = r6 + M0;
   // INPUT_GAIN_MANTISA
   r5 = M[r5];
   // this loop excutes for each sample in the block
peq_block_loop:
      // get new input sample
      // number of Biquad stages used, get new input sample
      r10 = r10 - M2, r0 = M[I4,M1];
      // Apply mantissa,Exp to front end gain
      rMAC = r0 * r5, r0 = M[I2,M2];
      r0 = rMAC ASHIFT r6;
      do biquad_loop;
         // get x(n-2), get coef b2
         r1 = M[I5,M1], r2 = M[I1,M1];
         // b2*x(n-2), get x(n-1), get coef b1
         rMAC = r1 * r2, r1 = M[I5,M0], r2 = M[I1,M1];
         // +b1*x(n-1), store new x(n-2), get coef b0
         rMAC = rMAC + r1 * r2, M[I5,M1] = r1, r2 = M[I1,M1];
         // +b0*x(n),store new x(n-1)
         rMAC = rMAC + r0 * r2, M[I5,M1] = r0;
         // get y(n-2), get coef a2
         r1 = M[I5,M1], r2 = M[I1,M1];
         // -a2*y(n-2), get y(n-1), get coef a1
         rMAC = rMAC - r1 * r2, r1 = M[I5,M0], r2 = M[I1,M1];
         // -a1*y(n-1),get the scalefactor
         rMAC = rMAC - r1 * r2, r3 = M[I2,M1];
         // get y(n)
         r0 = rMAC ASHIFT r3;
biquad_loop:
      // store new y(n-2)
      M[I5,M1] = r1;
      // store new y(n-1)
      M[I5,M1] = r0;
      // Restore Head room
      r0 = r0 ASHIFT 2;
      // Decrement the block counter,write back o/p sample
      r4 = r4 - M1,  M[I0,M1] = r0;
   if NZ jump peq_block_loop;

   // Update the I/O buffer pointers back into PEQ data object before leaving.
   L0 = Null;
   L4 = Null;
   L1 = Null;
   L5 = Null;
   r0 = I4;
   // Update the input buffer pointer field
   M[r7 + $mv_dongle.peq.INPUT_ADDR_FIELD] = r0;
   r0 = I0;
   // Update the output buffer pointer field
   M[r7 + $mv_dongle.peq.OUTPUT_ADDR_FIELD] = r0;

   rts;

.ENDMODULE;

#endif
