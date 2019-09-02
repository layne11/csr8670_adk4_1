// *****************************************************************************
// Copyright (c) 2008 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
//
// *****************************************************************************


// $****************************************************************************
// NAME:
//    TWS Delay Utility
//
// DESCRIPTION:
//    Utilities to delay an audio stream by a specified length in samples.
//
// MODULES:
//    - $tws.delay.initialize
//    - $tws.delay.process
// *****************************************************************************

#ifndef AUDIO_DELAY_INCLUDED
#define AUDIO_DELAY_INCLUDED

#include "core_library.h"
#include "delay.h"


// *****************************************************************************
// MODULE:
//    $tws.delay.initialize
//
// DESCRIPTION:
//    Clear the delay buffer
//
// INPUTS:
//    - r8 = pointer to delay data object, with the following fields being set
//       - $tws.delay.DBUFF_ADDR_FIELD
//       - $tws.delay.DBUFF_SIZE_FIELD
//
// OUTPUTS:
//    - none
//
// TRASHED REGISTERS:
//    r0, r10, I0, L0, LOOP
//
// NOTES:
// *****************************************************************************

.MODULE $M.tws.delay.initialize;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

$tws.delay.initialize:
   
   r0  = M[r8 + $tws.delay.DBUFF_ADDR_FIELD];
   push rLink;
#ifdef BASE_REGISTER_MODE
   call $cbuffer.get_read_address_and_size_and_start_address;
   push r2;
   B0 = M[SP-1];
   pop  B0;
#else
   call $cbuffer.get_read_address_and_size;
#endif
   I0  = r0;
   L0  = r1;
   r10 = r1;
   pop rLink;
   // Clear delay buffer   
   r0 = 0;
   do loop_delay_init;
      M[I0,1] = r0;
loop_delay_init:
   L0 = 0;
#ifdef BASE_REGISTER_MODE  
   push Null;
   pop  B0;
#endif
   rts;

.ENDMODULE;


// *****************************************************************************
// MODULE:
//    $tws.delay.process
//
// DESCRIPTION:
//    Delaying input audio stream into output audio stream via delay buffer
//
// INPUTS:
//    - r8 = pointer to delay data object, with every field being set
//    - r3 = size of data to process
//
// OUTPUTS:
//    - none
//
// TRASHED REGISTERS:
//    r0, r1, r10, I0-I2, I4-I5, L0-L1, L4-L5, M1-M2, LOOP
//
// NOTES:
//    - Delay buffer must be circular. Due to a known Kalimba chip bug, the
//      size of the buffer (DBUFF_SIZE_FIELD) must be at least 2.
//
//    - A '0' set in the 'DELAY_FIELD' means 'no delay'.
//
//    - Maximum legal delay is the size of the delay buffer (DBUFF_SIZE_FIELD).
// *****************************************************************************

.MODULE $M.tws.delay.process;
   .CODESEGMENT PM;
   .DATASEGMENT DM;
   .VAR read_byte_pos_func[3] = &read_byte_pos_0, &read_byte_pos_1, &read_byte_pos_2;
   .VAR write_byte_pos_func[3] = &write_byte_pos_0, &write_byte_pos_1, &write_byte_pos_2;

$tws.delay.process:

   $push_rLink_macro;
   // kick off division in case we need it (needed for 16bit mode)
   r6 = M[r8 + $tws.delay.DELAY_FIELD];
   rMAC = r6 LSHIFT 1;  // Read delay size in terms of #of 16bits * 2  
   rMAC = rMAC ASHIFT 0 (LO); 
   r0 = 3;
   Div = rMac/r0;

   //**** Get Input Buffers *****
   r0 = M[r8 + $tws.delay.INPUT_ADDR_FIELD];
#ifdef BASE_REGISTER_MODE  
   call $cbuffer.get_write_address_and_size_and_start_address;
   push r2;
   pop  B4;
#else
   call $cbuffer.get_write_address_and_size;
#endif
   
   M3 = -r3;
   I4  = r0;
   L4  = r1;
   r2 = M[I4,M3];

   I5 = r0;
   L5 = r1;
   r2 = M[I5,M3];

   // Use input frame size
   r10 = r3;

#if 0
// ***** Get output buffers ******
// Update output frame size from input
   r0 = M[r8 + $tws.delay.OUTPUT_ADDR_FIELD];

#ifdef BASE_REGISTER_MODE  
   call $cbuffer.get_write_address_and_size_and_start_address;
   push r2;
   pop  B5;
#else
   call $cbuffer.get_write_address_and_size;
#endif
   I5 = r0;
   L5 = r1;
#endif

   // Get read pointer of delayed stream, point to input stream if no delay
   // Depending on DELAY_FIELD
   r0 = M[r8 + $tws.delay.DELAY_FIELD];
   M2 = NULL - r0;
   if Z jump jp_no_delay;

   // Get write pointer to delay buffer (DBUFF_ADDR_FIELD / DBUFF_SIZE_FIELD)
   r0 = M[r8 + $tws.delay.DBUFF_ADDR_FIELD];
#ifdef BASE_REGISTER_MODE
   call $cbuffer.get_write_address_and_size_and_start_address;
   push r2;
   B0 = M[SP-1];
   pop  B1;
#else
   call $cbuffer.get_write_address_and_size;
#endif
   I1 = r0;
   L1 = r1;
   I0 = r0;
   L0 = r1;
   
   M2 = Null - r6;   //Set M2 to modify Read Start Point
   if Z jump jp_no_delay;

   // Determine if 24-bit or 16-bit samples are being stored
   Null = M[r8 + $tws.delay.MODE];
   if NZ jump mode_16bit; 
   
   // 24bit delay mode
   // Dummy read to set delay	
   r1 = M[I0,M2];
   
   // Delaying input stream into output stream via delay buffer
   do loop_delay;
      r0 = M[I4,1], r1 = M[I0,1];
      M[I1,1] = r0, M[I5,1] = r1;
loop_delay:

   jump update_delay_buffer;

// 16bit Delay Mode
mode_16bit:  
   // Read write byte position from structure.
   r4 = M[r8 + $tws.delay.write_bytepos];
   
   r0 = DivResult;      // Quotient = #of 24bit words
   r1 = DivRemainder;   // Remainder = Selector for BytePosition

   // Switch Case
   Null = r1;
   if Z jump remainder_0;
   Null = r1 - 1;
   if Z jump remainder_1;
   Null = r1 - 2;
   if Z jump remainder_2;

// See table at end of file for Truth Table
remainder_0:
// Remainder 0 indicates the set delay size represents an integer # of 24bit word  
// Read byte position will follow current write position
   M2 = Null - r0;   //Set M2 to -Q modify Read Start Point
   r7 = r4;          
   jump done_setting_delay_read;        

remainder_1:
// Remainder 1 indicates the set delay size represents an integer # of 24bit word  plus 1byte
// Read byte position will depend on current write position
   Null = r4;        //Check Write Byte Position
   if NZ jump rem1_writepos_not0;
   r0 = r0 + 1;      // Q+1
   M2 = Null - r0;   // Set M2 to -(Q+1) modify Read Start Point
   r7 = 2;           // Set read pos = 2 
   jump done_setting_delay_read;

rem1_writepos_not0:
   M2 = Null - r0;   // M2=-Q to modify Read Start Point
   r1 = 1;           // Set read pos = 1 if write pos 2
   Null = r4 LSHIFT -1; //Check if write pos = 2 or 1
   if Z r1 = 0;      // Set read pos = 0 if write pos 1
   r7 = r1;          // Set read pos 
   jump done_setting_delay_read; 
         
remainder_2:
// Remainder 2 indicates the set delay size represents an integer # of 24bit word  plus 2byte
// Read byte position will depend on current write position
   Null = r4 LSHIFT -1;    //Check if write pos = 2
   if Z jump rem2_writepos_not2;
   M2 = Null - r0;   // M2=-Q to modify Read Start Point
   r7 = 0;           // Set read pos = 0 
   jump done_setting_delay_read;

rem2_writepos_not2:
   r0 = r0 + 1;      // Q+1 
   M2 = Null - r0;   //M2 = -(Q+1) to modify Read Start Point
   r1 = 2;           // Set read pos = 2 ifwrite pos 1
   Null = r4; 
   if Z r1 = 1;      // Set read pos=1 if write pos 0
   r7 = r1;
   jump done_setting_delay_read; 
      
done_setting_delay_read:

   // Dummy read to set delay	
   r1 = M[I0,M2];
      
   // Delaying input stream into output stream via delay buffer
   do loop_delay_16bit;
      r0 = M[I4,1];            // read from input
      r0 = r0 AND 0xFFFF00;    // Covert to 16bit audio word by truncating LSB's
      
      r1 = M[&read_byte_pos_func + r7];	
      jump r1;
     
      // Read data from delay buffer and move it to output buffer
      // byte_pos cycles through values: 0,2,1,0,2,1,0,2,1, etc...   
      // I0 is used to track byte position for read from delay buffer, I5 is for output buffer
      read_byte_pos_0:
         r1 = M[I0,0];         // read from delay line, do not incremenmt read pointer
         r5 = r1 AND 0xFFFF00; //Audio is in upper 16 bits of 24bit word
         M[I5,1] = r5;         // write value to output buffer
         r7 = 2;               // update byte_pos
         jump do_write;
         
      read_byte_pos_1:
         r1 = M[I0,1];         // read from delay line and increment read pointer
         r5 = r1 AND 0x00FFFF; //Audio is in lower 16 bits of 24bit word  
         r5 = r5 LSHIFT 8;     // Align MSB  
         M[I5,1] = r5;         // write value to output buffer
         r7 = 0;               // update byte_pos
         jump do_write;
         
      read_byte_pos_2:
         r1 = M[I0,1];         // read from delay line and increment read pointer
         r5 = r1 AND 0x0000FF; // MSB of Audio is in LSB of current 24bit word
         r5 = r5 LSHIFT 16;    // Align MSB
         r1 = M[I0,0];         // read from delay line, do not incremenmt read pointer
         r1 = r1 AND 0xFF0000; // LSB of Audio is in MSB of current 24bit word
         r1 = r1 LSHIFT -8;    // Align LSB
         r5 = r5 OR r1;        // Merge LSB and MSB
         M[I5,1] = r5;         // write value to output buffer
         r7 = 1;               // update byte_pos
      
      // Read data from input buffer and move it into delay buffer
      // byte_pos cycles through values: 0,2,1,0,2,1,0,2,1, etc...
      // I1 is used to track byte position for write to delay buffer
      do_write:
         r1 = M[&write_byte_pos_func + r4];
         jump r1;
      
      write_byte_pos_0:
         r1 = M[I1,0];         // read from delay line, do not increment write pointer
         r1 = r1 AND 0x0000FF; // Preserve LSB 
         r1 = r1 OR r0;        // Merge input audio with LSB 
         M[I1,0] = r1;         // write value to delay line, do not increment
         r4 = 2;               // update byte_pos
         jump get_next_input_sample;
         
      write_byte_pos_1:
         r1 = M[I1,0];         // read from delay line, do not increment write pointer
         r1 = r1 AND 0xFF0000; // Preserve MSB
         r2 = r0 LSHIFT -8;    // Align MSB of input audio
         r1 = r1 OR r2;        // Merge Previous MSB with aligned audio
         M[I1,1] = r1;         // write value to delay line, and Increment write pointer
         r4 = 0;               // update byte_pos
         jump get_next_input_sample;
         
      write_byte_pos_2:
         r1 = M[I1,0];         // read from delay line, do not increment write pointer
         r1 = r1 AND 0xFFFF00; // Preserve first 2 MSB's
         r2 = r0 LSHIFT -16;   // Align MSB of audio Input
         r1 = r1 OR r2;        // Merge Audio MSB with previous data
         M[I1,1] = r1;         // write value to delay line, and Increment write pointer
         r1 = M[I1,0];         // read from delay line, do not increment write pointer
         r1 = r1 AND 0x00FFFF; // Preserve first 2 LSB's
         r2 = r0 LSHIFT 8;     // Align LSB of audio Input
         r2 = r2 AND 0xFF0000;
         r1 = r1 OR r2;        // Merge Audio LSB with previous data
         M[I1,0] = r1;         // write value to delay line, do not increment write pointer
		   r4 = 1;               // update byte_pos

      get_next_input_sample:      
      nop;
   loop_delay_16bit:
   
   // Update write position for next iteration of the processing.
   //Saving it in the structure makes it able to track pointers for channels independently
   M[r8 + $tws.delay.write_bytepos] = r4;  
   
   update_delay_buffer:
   r0 = M[r8 + $tws.delay.DBUFF_ADDR_FIELD];
   r1 = I1;
   call $cbuffer.set_write_address;

jp_done:
   // clear circular buffer registers 
   L0 = 0;
   L1 = 0;
   L4 = 0;
   L5 = 0;
#ifdef BASE_REGISTER_MODE  
   push Null;
   B4 = M[SP-1];
   B5 = M[SP-1];
   B1 = M[SP-1];
   pop  B0;
#endif
  // pop rLink from stack
  jump $pop_rLink_and_rts;

jp_no_delay:
  // Copy input to output
   do loop_copy;
      r1 = M[I4,1];
      M[I5,1] = r1;
loop_copy:

  jump jp_done;


.ENDMODULE;

#endif // AUDIO_DELAY_INCLUDED
