//------------------------------------------------------------------------------
// Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd.
//------------------------------------------------------------------------------
// NAME:
//   spkr_ctrl_system
//------------------------------------------------------------------------------
// DESCRIPTION:
//   Apply bass management and crossovers to audio
//   Potential input scenarios
//   - Left and right audio
//   - Left and right audio with LFE
//   Potential output scenarios
//   - Left and right full range speakers
//   - Left and right full range speakers with subwoofer (wired or wireless)
//   - Left and right tweeters with mono mid/woofer
//   - Left and right tweeters with mono mid/woofer and subwoofer (wired or wireless)
//   - Left and right tweeters with left and right mid/woofers
//   - Left and right tweeters with left and right mid/woofers and subwoofer (wired or wireless)
//------------------------------------------------------------------------------
// Initialisation structure
//   - $spkr_ctrl_system.INIT_RUNTIME_STRUCT_PTR
//   - $spkr_ctrl_system.INIT_PRI_EQ_DEFN_PTR           (0 if not present)
//   - $spkr_ctrl_system.INIT_PRI_EQ_BANK_SELECT_PTR    (0 if not present)
//   - $spkr_ctrl_system.INIT_SEC_EQ_DEFN_PTR           (0 if not present)
//   - $spkr_ctrl_system.INIT_SEC_EQ_BANK_SELECT_PTR    (0 if not present)
//   - $spkr_ctrl_system.INIT_BASS_EQ_DEFN_PTR          (0 if not present)
//   - $spkr_ctrl_system.INIT_BASS_EQ_BANK_SELECT_PTR   (0 if not present)
//------------------------------------------------------------------------------
// Runtime structure - points to streams used by processing and coefficients structure
//   - $spkr_ctrl_system.LEFT_INPUT_PTR
//   - $spkr_ctrl_system.RIGHT_INPUT_PTR
//   - $spkr_ctrl_system.LEFT_PRI_OUTPUT_PTR            (0 if not present)
//   - $spkr_ctrl_system.RIGHT_PRI_OUTPUT_PTR           (0 if not present)
//   - $spkr_ctrl_system.LEFT_SEC_OUTPUT_PTR            (0 if not present)
//   - $spkr_ctrl_system.RIGHT_SEC_OUTPUT_PTR           (0 if not present)
//   - $spkr_ctrl_system.LFE_INPUT_PTR                  (0 if not present)
//   - $spkr_ctrl_system.SUB_OUTPUT_PTR                 (0 if not present)
//   - $spkr_ctrl_system.BASS_BUFFER_PTR
//   - $spkr_ctrl_system.BYPASS_WORD_PTR
//   - $spkr_ctrl_system.BYPASS_BIT_MASK_FIELD
//   - $spkr_ctrl_system.COEFS_PTR
//------------------------------------------------------------------------------
//   - $spkr_ctrl_system.COEF_CONFIG
//   - $spkr_ctrl_system.COEF_EQ_L_PRI_PTR              (0 if not present)
//   - $spkr_ctrl_system.COEF_EQ_R_PRI_PTR              (0 if not present)
//   - $spkr_ctrl_system.COEF_EQ_L_SEC_PTR              (0 if not present)
//   - $spkr_ctrl_system.COEF_EQ_R_SEC_PTR              (0 if not present)
//   - $spkr_ctrl_system.COEF_EQ_BASS_PTR               (0 if not present)
//   - $spkr_ctrl_system.COEF_GAIN_ACTIVE_PTR
//   - $spkr_ctrl_system.COEF_GAIN_A_PTR
//   - $spkr_ctrl_system.COEF_GAIN_B_PTR
//------------------------------------------------------------------------------
//   - $spkr_ctrl_system.COEF_GAIN.GAIN_L_L_PRI
//   - $spkr_ctrl_system.COEF_GAIN.GAIN_R_L_PRI
//   - $spkr_ctrl_system.COEF_GAIN.GAIN_B_L_PRI
//   - $spkr_ctrl_system.COEF_GAIN.GAIN_L_R_PRI
//   - $spkr_ctrl_system.COEF_GAIN.GAIN_R_R_PRI
//   - $spkr_ctrl_system.COEF_GAIN.GAIN_B_R_PRI
//   - $spkr_ctrl_system.COEF_GAIN.GAIN_L_L_SEC
//   - $spkr_ctrl_system.COEF_GAIN.GAIN_R_L_SEC
//   - $spkr_ctrl_system.COEF_GAIN.GAIN_B_L_SEC
//   - $spkr_ctrl_system.COEF_GAIN.GAIN_L_R_SEC
//   - $spkr_ctrl_system.COEF_GAIN.GAIN_R_R_SEC
//   - $spkr_ctrl_system.COEF_GAIN.GAIN_B_R_SEC
//   - $spkr_ctrl_system.COEF_GAIN.GAIN_L_BASS
//   - $spkr_ctrl_system.COEF_GAIN.GAIN_R_BASS
//   - $spkr_ctrl_system.COEF_GAIN.GAIN_LFE_BASS
//   - $spkr_ctrl_system.COEF_GAIN.GAIN_SUB
//------------------------------------------------------------------------------



#include "cbuffer.h"
#include "stack.h"

#include "spkr_ctrl_system.h"


//------------------------------------------------------------------------------
.MODULE $M.spkr_ctrl_system.initialisation;
//------------------------------------------------------------------------------
// initialisation routines for speaker control system
//------------------------------------------------------------------------------

    .CODESEGMENT AUDIO_PROC_SPEAKER_CONTROL_SYSTEM_INITIALIZE_PM;
    .DATASEGMENT DM;

//------------------------------------------------------------------------------
$spkr_ctrl_system.initialize:
//------------------------------------------------------------------------------
// initialise speaker control system memory structures
// - clear bass channel of any stale data that might be lurking
// - run coefficient calculation for filters
//------------------------------------------------------------------------------
// INPUTS:
// - r7 = pointer to initialisation structure
//------------------------------------------------------------------------------
// OUTPUTS:
//   none
//------------------------------------------------------------------------------
// TRASHED REGISTERS:
//   Lots
//------------------------------------------------------------------------------

    push rLink;

    // initialise filter coefficients

    push r7;

    r2 = M[r7 + $spkr_ctrl_system.INIT_RUNTIME_STRUCT_PTR];
    r2 = M[r2 + $spkr_ctrl_system.COEFS_PTR];
    r2 = M[r2 + $spkr_ctrl_system.COEF_CONFIG];                     // bank number
    r8 = M[r7 + $spkr_ctrl_system.INIT_PRI_EQ_BANK_SELECT_PTR];     // primary eq bank selection object
    if Z jump dontCalcPriEqCoefs;
        r7 = M[r7 + $spkr_ctrl_system.INIT_PRI_EQ_DEFN_PTR];        // primary eq filter definition object

        r0 = M[r7 + $user_eq.DEFINITION_TABLE_COEFS_A_PTR];
        r1 = M[r7 + $user_eq.DEFINITION_TABLE_COEFS_B_PTR];
        r8 = r8 + r2;
        r8 = M[r8];     // r8 now points to eq parameters

        call $user_eq.calcBankCoefs.call_entry;
    dontCalcPriEqCoefs:

    r7 = M[SP-1];

    r2 = M[r7 + $spkr_ctrl_system.INIT_RUNTIME_STRUCT_PTR];
    r2 = M[r2 + $spkr_ctrl_system.COEFS_PTR];
    r2 = M[r2 + $spkr_ctrl_system.COEF_CONFIG];                     // bank number
    r8 = M[r7 + $spkr_ctrl_system.INIT_SEC_EQ_BANK_SELECT_PTR];     // secondary eq bank selection object
    if Z jump dontCalcSecEqCoefs;
        r7 = M[r7 + $spkr_ctrl_system.INIT_SEC_EQ_DEFN_PTR];        // secondary eq filter definition object

        r0 = M[r7 + $user_eq.DEFINITION_TABLE_COEFS_A_PTR];
        r1 = M[r7 + $user_eq.DEFINITION_TABLE_COEFS_B_PTR];
        r8 = r8 + r2;
        r8 = M[r8];     // r8 now points to eq parameters

        call $user_eq.calcBankCoefs.call_entry;
    dontCalcSecEqCoefs:

    r7 = M[SP-1];

    r2 = M[r7 + $spkr_ctrl_system.INIT_RUNTIME_STRUCT_PTR];
    r2 = M[r2 + $spkr_ctrl_system.COEFS_PTR];
    r2 = M[r2 + $spkr_ctrl_system.COEF_CONFIG];                     // bank number
    r8 = M[r7 + $spkr_ctrl_system.INIT_BASS_EQ_BANK_SELECT_PTR];    // bass eq bank selection object
    if Z jump dontCalcBassEqCoefs;
        r7 = M[r7 + $spkr_ctrl_system.INIT_BASS_EQ_DEFN_PTR];       // bass eq filter definition object

        r0 = M[r7 + $user_eq.DEFINITION_TABLE_COEFS_A_PTR];
        r1 = M[r7 + $user_eq.DEFINITION_TABLE_COEFS_B_PTR];
        r8 = r8 + r2;
        r8 = M[r8];     // r8 now points to eq parameters

        call $user_eq.calcBankCoefs.call_entry;
    dontCalcBassEqCoefs:

    // select appropriate set of gains

    r7 = M[SP-1];

    r2 = M[r7 + $spkr_ctrl_system.INIT_RUNTIME_STRUCT_PTR];
    r2 = M[r2 + $spkr_ctrl_system.COEFS_PTR];
    r1 = M[r2 + $spkr_ctrl_system.COEF_CONFIG];                     // bank number (1 or 2)

    r3 = r2 + r1;
    r0 = M[r3 + $spkr_ctrl_system.COEF_GAIN_ACTIVE_PTR];
    M[r2 + $spkr_ctrl_system.COEF_GAIN_ACTIVE_PTR] = r0;

    pop r7;

    pop rLink;
    rts;



//------------------------------------------------------------------------------
$spkr_ctrl_system.zero_data:
//------------------------------------------------------------------------------
// clear data memory in filters to remove stale data
//------------------------------------------------------------------------------
// INPUTS:
// - r7 = pointer to bass channel buffer
// - r8 = length of bass channel buffer
//------------------------------------------------------------------------------

    push rLink;

    // r7 points to buffer structure of bass channel
    null = r7;
    if z jump no_output_buffer_to_zero;
        r10 = r8;
        i0 = r7;
        r0 = 0;
        do clear_buffer;
            M[i0,1] = r0;
        clear_buffer:
    no_output_buffer_to_zero:

    pop rLink;
    rts;


.ENDMODULE;



//------------------------------------------------------------------------------
.MODULE $M.spkr_ctrl_system.processing;
//------------------------------------------------------------------------------
// speaker control system audio processing routines
//------------------------------------------------------------------------------

    .CODESEGMENT AUDIO_PROC_SPEAKER_CONTROL_SYSTEM_PROCESS_PM;
    .DATASEGMENT DM;

//------------------------------------------------------------------------------
$spkr_ctrl_system.process:
//------------------------------------------------------------------------------
// apply crossovers and bass management to audio
//------------------------------------------------------------------------------
// INPUTS:
// - r7 = pointer to procesing structure
//------------------------------------------------------------------------------
// OUTPUTS:
//   none
//------------------------------------------------------------------------------
// TRASHED REGISTERS:
//   Lots
//------------------------------------------------------------------------------
// If (have LFE):
//     Copy LFE to internal bass buffer
// Else:
//     clear internal bass buffer
//
// bass = Matrix of L, R, bass
//
// If (have L mid ouput):
//     L mid = Matrix of L, R, bass
//
// If (have R mid output):
//     R mid = Matrix of L, R, bass
//
// If (sub output connected):
//     Copy internal bass buffer to Sub output with sub out gain
//
// Copy L/R input to L/R treble output with gain
//------------------------------------------------------------------------------

    push rLink;

    // check if processing is bypassed
    r0 = M[r7 + $spkr_ctrl_system.BYPASS_WORD_PTR];
    r0 = M[r0];
    r1 = M[r7 + $spkr_ctrl_system.BYPASS_BIT_MASK_FIELD];
    null = r0 and r1;
    if nz jump bypass;

    r8  = M[r7 + $spkr_ctrl_system.COEFS_PTR];


    // If (have LFE):
    //     Copy LFE to internal bass buffer
    // Else:
    //     clear internal bass buffer
    //--------------------------------------------------------------------------

    // get frame size from left channel input
    r0 = M[r7 + $spkr_ctrl_system.LEFT_INPUT_PTR];
    call get_ptrs;
    r10 = r3;
    // get pointer to bass buffer
    r0 = M[r7 + $spkr_ctrl_system.BASS_BUFFER_PTR];
    i2 = r0;
    // check for LFE buffer
    r0 = M[r7 + $spkr_ctrl_system.LFE_INPUT_PTR];
    if Z jump clearBassBuffer;
        call get_ptrs;
        i0 = r0;
        l0 = r1;
        do inputLFEcopyLoop;
            r0 = M[i0,1];
            M[i2,1] = r0;
        inputLFEcopyLoop:
        jump bassChannelReadyForMixingLR;

    clearBassBuffer:
        r0 = 0;
        do clearBassBufferLoop;
            M[i2,1] = r0;
        clearBassBufferLoop:

    bassChannelReadyForMixingLR:


    // bass = Matrix of L, R, bass
    // Apply sub filtering to bass
    //--------------------------------------------------------------------------

    r0 = M[r7 + $spkr_ctrl_system.LEFT_INPUT_PTR];
    call get_ptrs;
    i0 = r0;
    l0 = r1;
    r10 = r3;

    push r10;

    r0 = M[r7 + $spkr_ctrl_system.RIGHT_INPUT_PTR];
    call get_ptrs;
    i4 = r0;
    l4 = r1;

    r0 = M[r7 + $spkr_ctrl_system.BASS_BUFFER_PTR];
    i2 = r0;

    push r8;
    r8 = M[r8 + $spkr_ctrl_system.COEF_GAIN_ACTIVE_PTR];
    r3 = M[r8 + $spkr_ctrl_system.COEF_GAIN.GAIN_L_BASS];
    r4 = M[r8 + $spkr_ctrl_system.COEF_GAIN.GAIN_R_BASS];
    r5 = M[r8 + $spkr_ctrl_system.COEF_GAIN.GAIN_LFE_BASS];
    pop r8;

    do createBassLoop;
                                r0 = M[i0,1];
        rmac = r0 * r3,         r0 = M[i4,1];
        rmac = rmac + r0 * r4,  r0 = M[i2,0];
        rmac = rmac + r0 * r5;
        r0 = rmac ashift 2;
        M[i2,1] = r0;
    createBassLoop:

    pop r4;     // number of samples to process

    r0 = m[r8 + $spkr_ctrl_system.COEF_EQ_BASS_PTR];
    if Z jump dontFilterBass;
        pushm <r7,r8>;
        r7 = r0;
        r0 = M[r7 + $audio_proc.peq.INPUT_ADDR_FIELD];
        i0 = r0;
        i4 = r0;
        l0 = 0;
        l4 = 0;
        call $audio_proc.hq_peq.process_op;
        popm <r7,r8>;
    dontFilterBass:



    // If (have primary L output):
    //     L = Matrix of L, R, bass
    //     Apply filtering to L
    //--------------------------------------------------------------------------

    r0 = M[r7 + $spkr_ctrl_system.LEFT_PRI_OUTPUT_PTR];
    if z jump dontCreateLeftPriOutput;

        r0 = M[r7 + $spkr_ctrl_system.LEFT_INPUT_PTR];
        call get_ptrs;
        i0 = r0;
        l0 = r1;
        r10 = r3;

        r0 = M[r7 + $spkr_ctrl_system.RIGHT_INPUT_PTR];
        call get_ptrs;
        i4 = r0;
        l4 = r1;

        r0 = M[r7 + $spkr_ctrl_system.BASS_BUFFER_PTR];
        i2 = r0;

        r0 = M[r7 + $spkr_ctrl_system.LEFT_PRI_OUTPUT_PTR];
        r3 = r10;
        call $frmbuffer.set_frame_size;
        call get_ptrs;
        i1 = r0;
        l1 = r1;

        push r8;
        r8 = M[r8 + $spkr_ctrl_system.COEF_GAIN_ACTIVE_PTR];
        r3 = M[r8 + $spkr_ctrl_system.COEF_GAIN.GAIN_L_L_PRI];
        r4 = M[r8 + $spkr_ctrl_system.COEF_GAIN.GAIN_R_L_PRI];
        r5 = M[r8 + $spkr_ctrl_system.COEF_GAIN.GAIN_B_L_PRI];
        pop r8;

        do createLeftPriOutLoop;
                                    r0 = M[i0,1];
            rmac = r0 * r3,         r0 = M[i4,1];
            rmac = rmac + r0 * r4,  r0 = M[i2,1];
            rmac = rmac + r0 * r5;
            r0 = rmac ashift 2;
            M[i1,1] = r0;
        createLeftPriOutLoop:

        r0 = m[r8 + $spkr_ctrl_system.COEF_EQ_L_PRI_PTR];
        if Z jump dontFilterLeftPriOutput;
            pushm <r7,r8>;
            r7 = r0;
            call $audio_proc.hq_peq.process;
            popm <r7,r8>;
        dontFilterLeftPriOutput:

    dontCreateLeftPriOutput:


    // If (have primary R output):
    //     R = Matrix of L, R, bass
    //     Apply filtering to R
    //--------------------------------------------------------------------------

    r0 = M[r7 + $spkr_ctrl_system.RIGHT_PRI_OUTPUT_PTR];
    if z jump dontCreateRightPriOutput;

        r0 = M[r7 + $spkr_ctrl_system.LEFT_INPUT_PTR];
        call get_ptrs;
        i0 = r0;
        l0 = r1;
        r10 = r3;

        r0 = M[r7 + $spkr_ctrl_system.RIGHT_INPUT_PTR];
        call get_ptrs;
        i4 = r0;
        l4 = r1;

        r0 = M[r7 + $spkr_ctrl_system.BASS_BUFFER_PTR];
        i2 = r0;

        r0 = M[r7 + $spkr_ctrl_system.RIGHT_PRI_OUTPUT_PTR];
        r3 = r10;
        call $frmbuffer.set_frame_size;
        call get_ptrs;
        i1 = r0;
        l1 = r1;

        push r8;
        r8 = M[r8 + $spkr_ctrl_system.COEF_GAIN_ACTIVE_PTR];
        r3 = M[r8 + $spkr_ctrl_system.COEF_GAIN.GAIN_L_R_PRI];
        r4 = M[r8 + $spkr_ctrl_system.COEF_GAIN.GAIN_R_R_PRI];
        r5 = M[r8 + $spkr_ctrl_system.COEF_GAIN.GAIN_B_R_PRI];
        pop r8;

        do createRightPriOutLoop;
                                    r0 = M[i0,1];
            rmac = r0 * r3,         r0 = M[i4,1];
            rmac = rmac + r0 * r4,  r0 = M[i2,1];
            rmac = rmac + r0 * r5;
            r0 = rmac ashift 2;
            M[i1,1] = r0;
        createRightPriOutLoop:

        r0 = m[r8 + $spkr_ctrl_system.COEF_EQ_R_PRI_PTR];
        if Z jump dontFilterRightPriOutput;
            pushm <r7,r8>;
            r7 = r0;
            call $audio_proc.hq_peq.process;
            popm <r7,r8>;
        dontFilterRightPriOutput:

    dontCreateRightPriOutput:


    // If (have secondary L output):
    //     L = Matrix of L, R, bass
    //     Apply filtering to L
    //--------------------------------------------------------------------------

    r0 = M[r7 + $spkr_ctrl_system.LEFT_SEC_OUTPUT_PTR];
    if z jump dontCreateLeftSecOutput;

        r0 = M[r7 + $spkr_ctrl_system.LEFT_INPUT_PTR];
        call get_ptrs;
        i0 = r0;
        l0 = r1;
        r10 = r3;

        r0 = M[r7 + $spkr_ctrl_system.RIGHT_INPUT_PTR];
        call get_ptrs;
        i4 = r0;
        l4 = r1;

        r0 = M[r7 + $spkr_ctrl_system.BASS_BUFFER_PTR];
        i2 = r0;

        r0 = M[r7 + $spkr_ctrl_system.LEFT_SEC_OUTPUT_PTR];
        r3 = r10;
        call $frmbuffer.set_frame_size;
        call get_ptrs;
        i1 = r0;
        l1 = r1;

        push r8;
        r8 = M[r8 + $spkr_ctrl_system.COEF_GAIN_ACTIVE_PTR];
        r3 = M[r8 + $spkr_ctrl_system.COEF_GAIN.GAIN_L_L_SEC];
        r4 = M[r8 + $spkr_ctrl_system.COEF_GAIN.GAIN_R_L_SEC];
        r5 = M[r8 + $spkr_ctrl_system.COEF_GAIN.GAIN_B_L_SEC];
        pop r8;

        do createLeftSecOutLoop;
                                    r0 = M[i0,1];
            rmac = r0 * r3,         r0 = M[i4,1];
            rmac = rmac + r0 * r4,  r0 = M[i2,1];
            rmac = rmac + r0 * r5;
            r0 = rmac ashift 2;
            M[i1,1] = r0;
        createLeftSecOutLoop:

        r0 = m[r8 + $spkr_ctrl_system.COEF_EQ_L_SEC_PTR];
        if Z jump dontFilterLeftSecOutput;
            pushm <r7,r8>;
            r7 = r0;
            call $audio_proc.hq_peq.process;
            popm <r7,r8>;
        dontFilterLeftSecOutput:

    dontCreateLeftSecOutput:


    // If (have secondary R output):
    //     R = Matrix of L, R, bass
    //     Apply filtering to R
    //--------------------------------------------------------------------------

    r0 = M[r7 + $spkr_ctrl_system.RIGHT_SEC_OUTPUT_PTR];
    if z jump dontCreateRightSecOutput;

        r0 = M[r7 + $spkr_ctrl_system.LEFT_INPUT_PTR];
        call get_ptrs;
        i0 = r0;
        l0 = r1;
        r10 = r3;

        r0 = M[r7 + $spkr_ctrl_system.RIGHT_INPUT_PTR];
        call get_ptrs;
        i4 = r0;
        l4 = r1;

        r0 = M[r7 + $spkr_ctrl_system.BASS_BUFFER_PTR];
        i2 = r0;

        r0 = M[r7 + $spkr_ctrl_system.RIGHT_SEC_OUTPUT_PTR];
        r3 = r10;
        call $frmbuffer.set_frame_size;
        call get_ptrs;
        i1 = r0;
        l1 = r1;

        push r8;
        r8 = M[r8 + $spkr_ctrl_system.COEF_GAIN_ACTIVE_PTR];
        r3 = M[r8 + $spkr_ctrl_system.COEF_GAIN.GAIN_L_R_SEC];
        r4 = M[r8 + $spkr_ctrl_system.COEF_GAIN.GAIN_R_R_SEC];
        r5 = M[r8 + $spkr_ctrl_system.COEF_GAIN.GAIN_B_R_SEC];
        pop r8;

        do createRightSecOutLoop;
                                    r0 = M[i0,1];
            rmac = r0 * r3,         r0 = M[i4,1];
            rmac = rmac + r0 * r4,  r0 = M[i2,1];
            rmac = rmac + r0 * r5;
            r0 = rmac ashift 2;
            M[i1,1] = r0;
        createRightSecOutLoop:

        r0 = m[r8 + $spkr_ctrl_system.COEF_EQ_R_SEC_PTR];
        if Z jump dontFilterRightSecOutput;
            pushm <r7,r8>;
            r7 = r0;
            call $audio_proc.hq_peq.process;
            popm <r7,r8>;
        dontFilterRightSecOutput:

    dontCreateRightSecOutput:


    // If (have sub output):
    //     output bass channel to sub with gain
    //--------------------------------------------------------------------------

    r0 = M[r7 + $spkr_ctrl_system.SUB_OUTPUT_PTR];
    if z jump dontCreateSubOutput;

        r0 = M[r7 + $spkr_ctrl_system.LEFT_INPUT_PTR];
        call get_ptrs;
        r10 = r3;

        // get pointer to bass buffer
        r0 = M[r7 + $spkr_ctrl_system.BASS_BUFFER_PTR];
        i2 = r0;

        r0 = M[r7 + $spkr_ctrl_system.SUB_OUTPUT_PTR];
        r3 = r10;
        call $frmbuffer.set_frame_size;
        call get_ptrs;
        i1 = r0;
        l1 = r1;

        push r8;
        r8 = M[r8 + $spkr_ctrl_system.COEF_GAIN_ACTIVE_PTR];
        r3 = M[r8 + $spkr_ctrl_system.COEF_GAIN.GAIN_SUB];
        pop r8;

        do createSubOutLoop;
                                    r0 = M[i2,1];
            rmac = r0 * r3;
            r0 = rmac ashift 2;
            M[i1,1] = r0;
        createSubOutLoop:

    dontCreateSubOutput:


    exit:

    // zero length registers
    l0 = 0;
    l1 = 0;
    l4 = 0;
    l5 = 0;

    pop rLink;
    rts;


//------------------------------------------------------------------------------
bypass:
//------------------------------------------------------------------------------
// copy left and right inputs to primary outputs
// if we have a sub output, fill it with zeros or copy LFE to it
// if we have a left secondary output, fill it with zeros
// if we have a right secondary output, fill it with zeros
//------------------------------------------------------------------------------

    // copy left and right inputs to outputs at full output
    //--------------------------------------------------------------------------

    r0 = M[r7 + $spkr_ctrl_system.LEFT_INPUT_PTR];
    call get_ptrs;
    i0 = r0;
    l0 = r1;
    r10 = r3;
    
    r0 = M[r7 + $spkr_ctrl_system.RIGHT_INPUT_PTR];
    call get_ptrs;
    i4 = r0;
    l4 = r1;
    
    r0 = M[r7 + $spkr_ctrl_system.LEFT_PRI_OUTPUT_PTR];
    r3 = r10;
    call $frmbuffer.set_frame_size;
    call get_ptrs;
    i1 = r0;
    l1 = r1;
    
    r0 = M[r7 + $spkr_ctrl_system.RIGHT_PRI_OUTPUT_PTR];
    r3 = r10;
    call $frmbuffer.set_frame_size;
    call get_ptrs;
    i5 = r0;
    l5 = r1;

    push r10;

    do copyLRinToLRoutLoop;
        r0 = M[i0,1],       r1 = M[i4,1];
        M[i1,1] = r0,       M[i5,1] = r1;
    copyLRinToLRoutLoop:

    // if we have a sub output, fill it with zeros or copy LFE to it
    //--------------------------------------------------------------------------

    // check whether we have a sub output to fill
    pop r10;
    r0 = M[r7 + $spkr_ctrl_system.SUB_OUTPUT_PTR];
    if z jump bypassNoSubOutput;
    // set sub output frame size
    r3 = r10;
    call $frmbuffer.set_frame_size;
    call get_ptrs;
    r10 = r3;
    i1 = r0;
    l1 = r1;

    // if we have an LFE input, copy it to the sub, otherwise fill sub with zero
    r0 = M[r7 + $spkr_ctrl_system.LFE_INPUT_PTR];
    if Z jump bypassClearSubOutput;
        call get_ptrs;
        i0 = r0;
        l0 = r1;
        do bypassLFEcopyLoop;
            r0 = M[i0,1];
            M[i1,1] = r0;
        bypassLFEcopyLoop:
        jump bypassNoSubOutput;

    bypassClearSubOutput:
        r0 = 0;
        do bypassClearSubOutputLoop;
            M[i1,1] = r0;
        bypassClearSubOutputLoop:

    bypassNoSubOutput:

    // if we have a secondary left output, fill it with zeros
    //--------------------------------------------------------------------------

    r0 = M[r7 + $spkr_ctrl_system.LEFT_SEC_OUTPUT_PTR];
    if z jump bypassNoLeftSecOutput;
        call get_ptrs;
        r10 = r3;
        i1 = r0;
        l1 = r1;
        r0 = 0;
        do bypassClearLeftSecOutputLoop;
            M[i1,1] = r0;
        bypassClearLeftSecOutputLoop:
    bypassNoLeftSecOutput:

    // if we have a secondary right output, fill it with zeros
    //--------------------------------------------------------------------------

    r0 = M[r7 + $spkr_ctrl_system.RIGHT_SEC_OUTPUT_PTR];
    if z jump bypassNoRightSecOutput;
        call get_ptrs;
        r10 = r3;
        i1 = r0;
        l1 = r1;
        r0 = 0;
        do bypassClearRightSecOutputLoop;
            M[i1,1] = r0;
        bypassClearRightSecOutputLoop:
    bypassNoRightSecOutput:

    // zero length registers
    l0 = 0;
    l1 = 0;
    l4 = 0;
    l5 = 0;

    pop rLink;
    rts;



//------------------------------------------------------------------------------
get_ptrs:
//------------------------------------------------------------------------------
// INPUTS:
//  - r0 = pointer to frame buffer structure
//
// OUTPUTS:
//  - r0 = buffer address
//  - r1 = buffer size
//  - r2 = buffer start address   <base address variant>
//  - r3 = frame size
//------------------------------------------------------------------------------

    push rLink;

    #ifdef BASE_REGISTER_MODE
        call $frmbuffer.get_buffer_with_start_address;
        push r2;
        pop  B0;
    #else
        call $frmbuffer.get_buffer;
    #endif

    pop rLink;
    rts;

.ENDMODULE;
