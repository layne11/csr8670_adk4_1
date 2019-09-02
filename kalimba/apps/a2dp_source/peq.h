// *****************************************************************************
// Copyright (c) 2015 - 2016 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
//
// *****************************************************************************

#ifndef PEQ_H
#define PEQ_H

#ifdef USES_PEQ

   // ** Audio PEQ buffers **
   #define MAX_NUM_PEQ_STAGES                           5

   .CONST $mv_dongle.peq.INPUT_ADDR_FIELD               0;

   // Size of input audio stream circular buffer size, '0' if linear buffer
   .CONST $mv_dongle.peq.INPUT_SIZE_FIELD               1;

   // Pointer to output audio stream
   .CONST $mv_dongle.peq.OUTPUT_ADDR_FIELD              2;

   // Size of output audio stream circular buffer size, '0' if linear buffer
   .CONST $mv_dongle.peq.OUTPUT_SIZE_FIELD              3;

   // Pointer to PEQ delay buffer, MUST be circular
   // Minimum size of the buffer: 2 * (number of stages + 1)
   .CONST $mv_dongle.peq.DELAYLINE_ADDR_FIELD           4;

   // Pointer to PEQ filter coefficients buffer, MUST be circular
   // Minimum size of the buffer: 5 * (number of stages)
   // The filter coefficients should be arranged in the following order:
   // - stage 1: b2(1), b1(1), b0(1), a2(1), a1(1)
   // - stage 2: b2(2), b1(2), b0(2), a2(2), a1(2)
   // . .......: ....., ....., ....., ....., .....
   // - stage n: b2(n), b1(n), b0(n), a2(n), a1(n)
   .CONST $mv_dongle.peq.COEFS_ADDR_FIELD               5;

   // Number of stage
   .CONST $mv_dongle.peq.NUM_STAGES_FIELD               6;

   // Size of delay line circular buffer
   // This field is set by initialisation routine based on NUM_STAGES_FIELD
   .CONST $mv_dongle.peq.DELAYLINE_SIZE_FIELD           7;

   // Size of filter coefficients circular buffer
   // This field is set by initialisation routine based on NUM_STAGES_FIELD
   .CONST $mv_dongle.peq.COEFS_SIZE_FIELD               8;

   // Size of data block to be processed
   .CONST $mv_dongle.peq.BLOCK_SIZE_FIELD               9;

   // Pointer to scaling buffer
   // Minimum size of the buffer: (number of stages), scaling for each stage
   .CONST $mv_dongle.peq.SCALING_ADDR_FIELD             10;

   // Pointer to 'gain exponent' variable
   .CONST $mv_dongle.peq.GAIN_EXPONENT_ADDR_FIELD       11;

   // Pointer to 'gain mantissa' variable
   .CONST $mv_dongle.peq.GAIN_MANTISA_ADDR_FIELD        12;

   // Structure size of PEQ data object
   .CONST $mv_dongle.peq.STRUC_SIZE                     13;

#endif

#endif
