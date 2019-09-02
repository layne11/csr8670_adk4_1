// *****************************************************************************
// Copyright (c) 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
// *****************************************************************************

//------------------------------------------------------------------------------
// MODULE:
//    $M.MeloD_Expansion
//------------------------------------------------------------------------------
// DESCRIPTION:
//	  audio image widening processor
//------------------------------------------------------------------------------
// TYPICAL USAGE
//
//    in music_example_config.asm, create the following structures...
//
//      // LoFreqWidth =     7.00
//      // HiFreqWidth =     2.50
//      // XvrFreq     =   600.00
//      // MonoWidth   =     1.00
//
//      .var MeloD_Expansion_coefficients[$MeloD_Expansion.COEFS_STRUC_SIZE] = 
//          0xB1AA81, 0x571F3E, 0xC3ECED,   // -1.22396840/2,  1.36128197/2, -0.93866428/2,
//          0x400000, 0x84DA2F, 0x3B3194,   // 0.5, -1.92418325/2,  0.92490121/2,
//          0x400000, 0xD53FA4, 0xFEA000,   // 0.5, -0.66799074/2, -0.02148449/2,
//          0x400000, 0x81543A, 0x3EB7E0,   // 0.5, -1.97923440/2,  0.97997290/2,
//          0x400000, 0xBB3B4C, 0x24BC7F;   // 0.5, -1.07450601/2,  0.57400504/2;
//  
//      .var MeloD_Expansion_filter_data[$MeloD_Expansion.FILTER_DATA_SIZE];
//      
//      .var /dm2 MeloD_Expansion_struct[$MeloD_Expansion.STRUC_SIZE] = 
//          &stream_map_left_in,
//          &stream_map_right_in,
//          &stream_map_left_out,
//          &stream_map_right_out,
//          $M.MUSIC_MANAGER.CONFIG.SPATIAL_BYPASS,
//          0,
//          &MeloD_Expansion_filter_data,
//          &MeloD_Expansion_coefficients;
//  
//    and add the following entry into the music manager function table
//
//      $MeloD_Expansion.process, &MeloD_Expansion_struct, &Bypass_word,
//
//------------------------------------------------------------------------------


#include "core_library.h"
#include "meloD_expansion.h"


//------------------------------------------------------------------------------
.module $M.MeloD_Expansion_initialize;
//------------------------------------------------------------------------------

    .codesegment AUDIO_PROC_MELOD_EXPANSION_INITIALIZE_PM;
    .datasegment DM;
	
$MeloD_Expansion.initialize:
    
    $push_rLink_macro;
    
    // don't have anything to do yet.  Left as a placeholder
    
    jump $pop_rLink_and_rts;


.endmodule;     // $M.MeloD_Expansion_initialize


    
//------------------------------------------------------------------------------
.module $M.MeloD_Expansion_process;
//------------------------------------------------------------------------------

    .codesegment AUDIO_PROC_MELOD_EXPANSION_PROCESS_PM;
    .datasegment DM;
	
$MeloD_Expansion.process:

    //--------------------------------------------------------------------------
    // To ensure clickless insertion and removal of processing we have a two
    // stage bypass system.  If the music manager configuration is set to bypass
    // the algorithm, then the processing is crossfaded to the dry audio.  When
    // when the crossfade gain is zero, the processing is bypassed totally.

    r0 = m[r7 + $MeloD_Expansion.BYPASS_WORD_MASK_FIELD];
    r6 = m[r7 + $MeloD_Expansion.CROSSFADE_GAIN_FIELD];
    r8 = m[r7 + $MeloD_Expansion.BYPASS_WORD_PTR];
    r8 = m[r8];
    // r5 is index modifier (+15709 = turning on / -15709 = turning off)
    // 15709 results in a gain value of 8,388,606 (0x7ffffe) after 534 samples
    r5 = 15709;
    r0 = r0 and r8;
    if z jump not_bypassed_in_music_manager;
    // bypassed in music manager, so check whether crossfade is set to zero
    null = r6;
    if z jump $M.audio_proc.stereo_copy.Process.func;
    r5 = -r5;
    not_bypassed_in_music_manager:

    $push_rLink_macro;
    
    // save arithmetic mode and set to saturating arithmetic
    r0 = m[$ARITHMETIC_MODE];
    push r0;
    r0 = 1;
    m[$ARITHMETIC_MODE] = r0;
    
    // Get Input Buffers
    r0 = m[r7 + $MeloD_Expansion.INPUT_LEFT_PTR_BUFFER_FIELD];
    call $frmbuffer.get_buffer;
    i0  = r0;
    l0  = r1;
    r0 = m[r7 + $MeloD_Expansion.INPUT_RIGHT_PTR_BUFFER_FIELD];
    call $frmbuffer.get_buffer;
    i4  = r0;
    l4  = r1;
    
    // Use input frame size
    r10 = r3;
    
    // Get output buffers
    r0 = m[r7 + $MeloD_Expansion.OUTPUT_LEFT_PTR_BUFFER_FIELD];
    r3 = r10;
    call $frmbuffer.set_frame_size;
    call $frmbuffer.get_buffer;
    i1 = r0;
    l1 = r1;
    r0 = m[r7 + $MeloD_Expansion.OUTPUT_RIGHT_PTR_BUFFER_FIELD];
    r3 = r10;
    call $frmbuffer.set_frame_size;
    call $frmbuffer.get_buffer;
    i5 = r0;
    l5 = r1;
    
    // buffer pointers
    //---------------------------------------
    // left in = i0/l0,     right in = i4/l4
    // left out = i1/l1,    right out = i5/l5
    
    m1 = 1;
    m2 = -2;
    
    do widening_process_loop;
    
        r0 = m[r7 + $MeloD_Expansion.FILTER_DATA_PTR_FIELD];
        i2 = r0;
        r0 = m[r7 + $MeloD_Expansion.FILTER_COEF_PTR_FIELD];
        i6 = r0;
        
        // get input audio and convert to m-s for shuffler
        r0 = m[i0,0],           r1 = m[i4,0];                       // r0 = left, r1 = right
        r0 = r0 ashift -1;
        r1 = r1 ashift -1;
        r2 = r0 - r1;                                               // r2 = side (S)
        
        // shuffler - filter difference (S) signal
        
        r1 = r0 + r1,           r0 = m[i2,0],   r3 = m[i6,1];       // r1 = mono (M), r0 = DM1, r3 = b1/2
        rmac = r0 * r3,         m[i2,1] = r2,   r3 = m[i6,1];       // DM1 = r2, r3 = b0/2
        rmac = rmac + r2 * r3,  r0 = m[i2,0],   r3 = m[i6,1];       // r0 = DM2, r3 = a1/2
        rmac = rmac - r0 * r3;
        rmac = rmac ashift 1 (56bit);
        r2 = rmac;
        
        // convert back to l/r
        r0 = r1 + r2,           m[i2,m1] = r2;                      // r0 = left
        
        // mono 2 stereo section
        
        r1 = r1 - r2,           r2 = m[i2,1],   r3 = m[i6,1];       // r1 = right, r2 = DM2, r3 = b2/2
        rmac = r2 * r3,         r2 = m[i2,-1],  r3 = m[i6,1];       // r2 = DM1, r3 = b1/2 & a1/2
        rmac = rmac + r2 * r3,  m[i2,2] = r2,   r4 = m[i6,1];       // DM2 = r2, r4 = b0/2 & a2/2
        rmac = rmac + r0 * r4,  r2 = m[i2,1];                       // DM2 = r0
        rmac = rmac - r2 * r4,  r2 = m[i2,m2];
        rmac = rmac - r2 * r3,  m[i2,1] = r0;
        rmac = rmac ashift 1 (56bit);
        r0 = rmac;
        m[i2,1] = r2;
        m[i2,1] = r0;
        
                                r2 = m[i2,1],   r3 = m[i6,1];       // r2 = DM2, r3 = b2/2
        rmac = r2 * r3,         r2 = m[i2,-1],  r3 = m[i6,1];       // r2 = DM1, r3 = b1/2 & a1/2
        rmac = rmac + r2 * r3,  m[i2,2] = r2,   r4 = m[i6,1];       // DM2 = r2, r4 = b0/2 & a2/2
        rmac = rmac + r0 * r4,  r2 = m[i2,1];                       // DM2 = r0
        rmac = rmac - r2 * r4,  r2 = m[i2,m2];
        rmac = rmac - r2 * r3,  m[i2,1] = r0;
        rmac = rmac ashift 1 (56bit);
        r0 = rmac;
        m[i2,1] = r2;
        m[i2,1] = r0;

                                r2 = m[i2,1],   r3 = m[i6,1];       // r2 = DM2, r3 = b2/2
        rmac = r2 * r3,         r2 = m[i2,-1],  r3 = m[i6,1];       // r2 = DM1, r3 = b1/2 & a1/2
        rmac = rmac + r2 * r3,  m[i2,2] = r2,   r4 = m[i6,1];       // DM2 = r2, r4 = b0/2 & a2/2
        rmac = rmac + r1 * r4,  r2 = m[i2,1];                       // DM2 = r0
        rmac = rmac - r2 * r4,  r2 = m[i2,m2];
        rmac = rmac - r2 * r3,  m[i2,1] = r1;
        rmac = rmac ashift 1 (56bit);
        r1 = rmac;
        m[i2,1] = r2;
        m[i2,1] = r1;
        
                                r2 = m[i2,1],   r3 = m[i6,1];       // r2 = DM2, r3 = b2/2
        rmac = r2 * r3,         r2 = m[i2,-1],  r3 = m[i6,1];       // r2 = DM1, r3 = b1/2 & a1/2
        rmac = rmac + r2 * r3,  m[i2,2] = r2,   r4 = m[i6,1];       // DM2 = r2, r4 = b0/2 & a2/2
        rmac = rmac + r1 * r4,  r2 = m[i2,1];                       // DM2 = r0
        rmac = rmac - r2 * r4,  r2 = m[i2,m2];
        rmac = rmac - r2 * r3,  m[i2,1] = r1;
        rmac = rmac ashift 1 (56bit);
        r1 = rmac;
        
        // apply in/out matrix for crossfading between dry and processed audio
        
        r6 = r6 + r5,           m[i2,m1] = r2;
        if neg r6 = r6 - r5;    // if gain has gone negative, it's wrapped so undo addition
        r4 = 0x7fffff;
        r4 = r4 - r6,           m[i2,m1] = r1;
        rmac = r0 * r6,         r2 = m[i0,m1];                      // r2 = left in
        rmac = rmac + r2 * r4,  r3 = m[i4,m1];                      // r3 = right in
        rmac = r1 * r6,         m[i1,m1] = rmac;                    // store left audio
        rmac = rmac + r3 * r4;
        m[i5,m1] = rmac;                                            // store right audio
    
    widening_process_loop:
    
    m[r7 + $MeloD_Expansion.CROSSFADE_GAIN_FIELD] = r6;
    
    // clear L-regs
    l0 = null;
    l4 = null;
    l1 = null;
    l5 = null;
    
    // restore arithmetic mode
    pop r0;
    m[$ARITHMETIC_MODE] = r0;

    // pop rLink from stack
    jump $pop_rLink_and_rts;
    
.endmodule;     // $M.MeloD_Expansion_process
