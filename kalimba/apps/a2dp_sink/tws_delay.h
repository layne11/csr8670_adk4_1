// *****************************************************************************
// Copyright (c) 2008 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
//
// *****************************************************************************

#ifndef TWS_DELAY_HEADER_INCLUDED
#define TWS_DELAY_HEADER_INCLUDED

   // Pointer to input audio stream
   .CONST $tws.delay.INPUT_ADDR_FIELD             0;
   // Pointer to output audio stream
   .CONST $tws.delay.OUTPUT_ADDR_FIELD            1;
   // Pointer to delay buffer
   // Allocate to different data bank with input/output stream for lower cycles
   .CONST $tws.delay.DBUFF_ADDR_FIELD             2;
   // Delay length in samples
   .CONST $tws.delay.DELAY_FIELD                  3;
   // Mode: 0 = stores 24-bit audio samples; 1 = compact mode, which lops off 8 least significant bits from source and packs 16-bit audio samples into 24-bit words
   .CONST $tws.delay.MODE                         4;
   // Bit position (only used in compact mode): 0 = 24-bit word boundary; 2 = least significant byte; 1 = middle byte
   //  -------------------------
   //  | 8bits | 8bits | 8bits |
   //  -------------------------
   //  |       |       |
   //  0       1       2
   .CONST $tws.delay.write_bytepos                5;
   // Structure size of delay data object
   .CONST $tws.delay.STRUC_SIZE                   6;

#endif // TWS_DELAY_HEADER_INCLUDED


/*************************************************************************
// TRUTH Table for determining reas byte position based on delay size (Quotient, Remainder) 
// and write Position. Used only in 16 bit mode
// In the representations below consider Delay size of 6 samples(=4 24bit words)
If Remainder (R) == 0 (Delay Field=3)
   If write pos = 0 then M2 = (Delay size - Q), read pos = 0
    |-----------|-----------|-----------|-----------|
    |   |   |   |   |   |   | 1 | 1 | 2 | 2 | 3 | 3 |
    |-----------|-----------|-----------|-----------|
    /\                      /\                      
    w                       r
   If write pos = 2 then M2 = (Delay size - Q), read pos = 2
    |-----------|-----------|-----------|-----------|
    | 3 | 3 |   |   |   |   |   |   | 1 | 1 | 2 | 2 |
    |-----------|-----------|-----------|-----------|
            /\                      /\                      
            w                       r
   If write pos = 1 then M2 = (Delay size - Q), read pos = 1
    |-----------|-----------|-----------|-----------|
    | 3 |   |   |   |   |   |   | 1 | 1 | 2 | 2 | 3 |
    |-----------|-----------|-----------|-----------|
       /\                       /\                      
       w                        r

If Remainder (R) == 1 (Delay Field = 2)-->2*2/3 gives Q=1,R=1
   If write pos = 0 then M2 = (Delay size - (Q + 1), read pos = 2
    |-----------|-----------|-----------|-----------|
    |   |   |   |   |   |   |   |   | 1 | 1 | 2 | 2 |
    |-----------|-----------|-----------|-----------|
    /\                              /\                      
    w                               r
   If write pos = 2 then M2 = (Delay size - Q), read pos = 1
    |-----------|-----------|-----------|-----------|
    | 2 | 2 |   |   |   |   |   |   |   |   | 1 | 1 |
    |-----------|-----------|-----------|-----------|
            /\                              /\
            w                               r
   If write pos = 1 then M2 = (Delay size - Q), read pos = 0
    |-----------|-----------|-----------|-----------|
    | 2 |   |   |   |   |   |   |   |   | 1 | 1 | 2 |
    |-----------|-----------|-----------|-----------|
       /\                               /\                      
       w                                r
If Remainder (R) == 2 (Delay Field =4)-->4*2/3 gives Q=2,R=2
   If write pos = 0 then M2 = (Delay size - (Q + 1), read pos = 1
    |-----------|-----------|-----------|-----------|
    |   |   |   |   | 1 | 1 | 2 | 2 | 3 | 3 | 4 | 4 |
    |-----------|-----------|-----------|-----------|
    /\             /\                      
    w              r
   If write pos = 2 then M2 = (Delay size - Q), read pos = 0
    |-----------|-----------|-----------|-----------|
    | 4 | 4 |   |   |   |   | 1 | 1 | 2 | 2 | 3 | 3 |
    |-----------|-----------|-----------|-----------|
           /\               /\                      
           w                r
   If write pos = 1 then M2 = (Delay size - (Q + 1), read pos = 2
    |-----------|-----------|-----------|-----------|
    | 4 |   |   |   |   | 1 | 1 | 2 | 2 | 3 | 3 | 4 |
    |-----------|-----------|-----------|-----------|
       /\               /\                      
       w                r
***************************************************************************/