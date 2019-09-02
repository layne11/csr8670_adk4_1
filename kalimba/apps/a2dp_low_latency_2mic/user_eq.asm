// *****************************************************************************
// Copyright (c) 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
// *****************************************************************************

//------------------------------------------------------------------------------
// music_example user eq coefficient calculation handling
//------------------------------------------------------------------------------


#include "stack.h"
#include "a2dp_low_latency_2mic_library_gen.h"
#include "a2dp_low_latency_2mic_config.h"
#include "user_eq.h"


.module $user_eq;
.CODESEGMENT PM;
  
.DATASEGMENT DM;

//------------------------------------------------------------------------------
userEqInitialize:
//------------------------------------------------------------------------------
// call coefficient calculation
//------------------------------------------------------------------------------
// on entry r7 = pointer to user eq definition table
//          r8 = pointer to bank selection object
//------------------------------------------------------------------------------

    $push_rLink_macro;

    // get EQ bank number (and limit to maximum just in case)
    r2 = M[&$M.CVC.data.CurParams + $M.A2DP_LOW_LATENCY_2MIC.PARAMETERS.OFFSET_CONFIG];
    r2 = r2 and $M.A2DP_LOW_LATENCY_2MIC.CONFIG.USER_EQ_SELECT;
    r1 = m[r7 + $user_eq.DEFINITION_TABLE_MAX_NUM_BANKS];   // r1 = max number of user banks
    r2 = min r1;
    
    r0 = m[r7 + $user_eq.DEFINITION_TABLE_COEFS_A_PTR];
    r1 = m[r7 + $user_eq.DEFINITION_TABLE_COEFS_B_PTR];

    r8 = r8 + r2;
    r8 = m[r8];     // r8 now points to user eq parameters
    // if address is '-1' then we use precalculated coefficients
    r3 = r8 + 1;
	if nz jump calcBankCoefs.jump_entry;
    
	// parameter ptr is '-1', so select coefficients to use based on sample rate
    // - Coef banks 1-6 are 44.1 kHz coefficients and Coef banks 7-12 are for 48 kHz

    // r2 = bank number from User EQ Select
    r2 = r2 - 1;
    r3 = m[r7 + $user_eq.DEFINITION_TABLE_MAX_NUM_BANKS];
    r1 = M[$M.set_codec_rate.current_codec_sampling_rate];
    null = r1 - 48000;
    if z r2 = r2 + r3;
    r3 = m[r7 + $user_eq.DEFINITION_TABLE_MAX_NUM_BANDS];
    r3 = r3 * 6 (int);                  // 6 coefficients per band (including scale)
    r3 = r3 + 3;                        // 3 coefficients (1 for num bands in use and 2 for gain)
    r3 = r3 * r2 (int);
    r0 = r0 + r3;

	// switch coefficients
	
    pushm <r0,r7>;
    r7 = m[r7 + $user_eq.DEFINITION_TABLE_LEFT_DM_PTR];         // r7 points to left channel EQ memory structure    
    m[r7 + $audio_proc.peq.PARAM_PTR_FIELD] = r0;
    call $audio_proc.hq_peq.initialize;
    
    popm <r0,r7>;
    r7 = m[r7 + $user_eq.DEFINITION_TABLE_RIGHT_DM_PTR];     // r7 points to right channel EQ memory structure    
    m[r7 + $audio_proc.peq.PARAM_PTR_FIELD] = r0;
    call $audio_proc.hq_peq.initialize;
    
    jump $pop_rLink_and_rts;


//------------------------------------------------------------------------------
eqInitialize:
//------------------------------------------------------------------------------
// call coefficient calculation
//------------------------------------------------------------------------------
// on entry r7 = pointer to speaker eq definition table
//          r8 = pointer to speaker eq parameters pointer
//------------------------------------------------------------------------------

    $push_rLink_macro;
	
	// if eq parameter ptr is non-zero then call coefficient calculation

    r0 = m[r7 + $user_eq.DEFINITION_TABLE_COEFS_A_PTR];
    r1 = m[r7 + $user_eq.DEFINITION_TABLE_COEFS_B_PTR];

	r8 = m[r8];
	if nz jump calcBankCoefs.jump_entry;
	
	// parameter ptr is zero, so select coefficients to use based on sample rate
    // - CoefsA are 44.1 kHz coefficients and CoefsB are for 48 kHz

    r2 = M[$M.set_codec_rate.current_codec_sampling_rate];
    null = r2 - 48000;
    if z r0 = r1;
	
	// switch coefficients
	
    pushm <r0,r7>;
    r7 = m[r7 + $user_eq.DEFINITION_TABLE_LEFT_DM_PTR];         // r7 points to left channel EQ memory structure    
    m[r7 + $audio_proc.peq.PARAM_PTR_FIELD] = r0;
    call $audio_proc.hq_peq.initialize;
    
    popm <r0,r7>;
    r7 = m[r7 + $user_eq.DEFINITION_TABLE_RIGHT_DM_PTR];     // r7 points to right channel EQ memory structure    
    m[r7 + $audio_proc.peq.PARAM_PTR_FIELD] = r0;
    call $audio_proc.hq_peq.initialize;
    
    jump $pop_rLink_and_rts;


//------------------------------------------------------------------------------
calcBankCoefs.call_entry:
    push rLink;
calcBankCoefs.jump_entry:
//------------------------------------------------------------------------------
// calculate coefficients for supplied eq bank
//------------------------------------------------------------------------------
// on entry r0 = address of "CoefsA"
//          r1 = address of "CoefsB"
//          r7 = pointer to eq definition table
//          r8 = address of user eq parameters
//------------------------------------------------------------------------------

    // check which set of coefficients are currently in use, and use other set
    // for calculating the coefficients.
    r2 = m[r7 + $user_eq.DEFINITION_TABLE_LEFT_DM_PTR];
    r2 = m[r2 + $audio_proc.peq.PARAM_PTR_FIELD];
    null = r2 - r0;
    if eq r0 = r1;
    
    // r0 points to coefficient set to be calculated
    // r7 points to user eq initialisation array
    // r8 points to parameters to calculate
    
    pushm <r0,r7>;

    //calculate coefficients
    //--------------------------------------------------------------------------
    
    i0 = r8;            // i0 = address of biquad parameters
    i1 = r0;            // i1 = address of biquad coefficients

    null = r8;
    if nz jump calcBankCoefs.NotNullBank;
        // bank address is zero so create null bank coefficients
        r0 = 0;                 // [0] = zero bands
        m[i1,1] = r0;
        r0 = 0x000001;          // [1] = gain exponent
        m[i1,1] = r0;
        r0 = 0x400000;          // [2] = gain mantissa
        m[i1,1] = r0;
        jump calcBankCoefs.switchToNewCoefs;

    calcBankCoefs.NotNullBank:

    // first copy number of EQ bands to coefficient storage.
    r0 = m[i0,1];
    m[i1,1] = r0;
    push r0;                // keep number of bands for further use
    
    // next we calculate the MasterGain of the eq bank
    call calcPreGain.call_entry;

    
    r0 = i1;
    r1 = m[r7 + $user_eq.DEFINITION_TABLE_MAX_NUM_BANDS];
    r1 = r1 * 5 (int);      // 5 coefficients per band (excluding scale)
    r0 = r0 + r1;
    i2 = r0;                // i2 = scale coefficients
    
    calcBandLoop:
        pop r0;
        r0 = r0 - 1;
        if neg jump calcBankCoefs.switchToNewCoefs;
        push r0;
    
        pushm <i0,i1,i2>;
        call coefCalcBiquadBand.call_entry;
        popm <r0,r1,r2>;
        r0 = r0 + 4;        // parameter increment (4 params per band)
        i0 = r0;
        r1 = r1 + 5;        // coefficient increment (5 coefs per band (ex. scale))
        i1 = r1;
        r2 = r2 + 1;        // scale increment
        i2 = r2;
        
        jump calcBandLoop;
        
    calcBankCoefs.switchToNewCoefs:
    
    // switch to new set of coefficients
    popm <r0,r7>;
    
    pushm <r0,r7>;
    r7 = m[r7 + $user_eq.DEFINITION_TABLE_LEFT_DM_PTR];         // r7 points to left channel EQ memory structure    
    m[r7 + $audio_proc.peq.PARAM_PTR_FIELD] = r0;
    call $audio_proc.hq_peq.initialize;
    
    popm <r0,r7>;
    r7 = m[r7 + $user_eq.DEFINITION_TABLE_RIGHT_DM_PTR];     // r7 points to right channel EQ memory structure    
    m[r7 + $audio_proc.peq.PARAM_PTR_FIELD] = r0;
    call $audio_proc.hq_peq.initialize;

    jump $pop_rLink_and_rts;


//------------------------------------------------------------------------------
calcBandCoefs:
//------------------------------------------------------------------------------
// calculate coefficients for supplied eq band
//------------------------------------------------------------------------------
// on entry r0 = param ID word - used to calculate address of eq parameters
//          r1 = address of "CoefsA"
//          r2 = address of "CoefsB"
//          r3 = base address of EQ parameters
//          r7 = pointer to eq definition table
//------------------------------------------------------------------------------

    $push_rLink_macro;
    
    r4 = r0;
    r0 = r0 and 0x00fff0;
    call calcParamAddrOffset;
    r8 = r0 + r3;
    
    //--------------------------------------------------------------------------
    // work out which set of coefficients we are currently using and
    // which set we are going to calculate
    
    r6 = m[r7 + $user_eq.DEFINITION_TABLE_LEFT_DM_PTR];     // r6 points to left channel EQ memory structure
    r6 = m[r6 + $audio_proc.peq.PARAM_PTR_FIELD];
    null = r6 - r1;
    if eq jump calcBandCoefsB;      // currently using "A" so calculate "B"
    
    // calcBandCoefsA:
    r3 = r2;        // "CoefsB"
    r2 = r1;        // "CoefsA"
    jump calcBandCoefsAorB;
    
    calcBandCoefsB:
    r3 = r1;        // "CoefsA"
    //              // r2 = "CoefsB"

    calcBandCoefsAorB:
    //--------------------------------------------------------------------------
    // r2 points to set of coefficients we are going to calculate
    // r3 points to set of coefficients we are currently using
    
    // only calculating one band so first copy current coefficients to new set
    i0 = r3;        // current coefficients
    i1 = r2;        // new coefficients
    
    // calculate number of coefficients to copy
    r10 = m[r7 + $user_eq.DEFINITION_TABLE_MAX_NUM_BANDS];
    r10 = r10 * 6 (int);     // 6 coefficients per band (including scale)
    r10 = r10 + 3;           // 3 coefficients (1 for num bands in use and 2 for gain)
    
    // copy current coefficients to new set
    do copyCurrentCoefsLoop;
        r0 = m[i0,1];
        m[i1,1] = r0;
    copyCurrentCoefsLoop:
    
    i0 = r8;            // i0 = address of biquad parameters

    // calculate coefficient address of filter
    r0 = r4;
    call calcCoefAddrOffset;
    r8 = r0 + r2;
    i1 = r8;            // i1 = address of biquad coefficients
    r8 = r1 + r2;
    i2 = r8;            // i2 = address of biquad scale
    
    //--------------------------------------------------------------------------
    // i0 = address of biquad parameters
    // i1 = address of biquad coefficients
    // i2 = address of biquad scale

    pushm <r2,r7>;

    // now call filter calculation routine...
    r0 = r4;
    call coefCalc;

    // switch to new set of coefficients
    popm <r2,r7>;
    pushm <r2,r7>;
    r7 = m[r7 + $user_eq.DEFINITION_TABLE_LEFT_DM_PTR];         // r7 points to left channel EQ memory structure    
    m[r7 + $audio_proc.peq.PARAM_PTR_FIELD] = r2;
    call $audio_proc.hq_peq.initialize;
    
    popm <r2,r7>;
    r7 = m[r7 + $user_eq.DEFINITION_TABLE_RIGHT_DM_PTR];     // r7 points to right channel EQ memory structure    
    m[r7 + $audio_proc.peq.PARAM_PTR_FIELD] = r2;
    call $audio_proc.hq_peq.initialize;

    jump $pop_rLink_and_rts;


//------------------------------------------------------------------------------
calcParamAddrOffset:
//------------------------------------------------------------------------------
// calculate position of parameter within parameter table
//------------------------------------------------------------------------------
// on entry r0 = parameter ID
//          r7 = pointer to eq definition table
// on exit  r0 = address offset of parameter in parameter table
// all other regsiters preserved
//------------------------------------------------------------------------------

    pushm <r1,r2,r3>;

    r1 = r0 and 0x000f00;       // bank #
    r1 = r1 ashift -8;
    r2 = r0 and 0x0000f0;       // biquad #
    r2 = r2 ashift -4;
    r3 = r0 and 0x00000f;       // param #
    
    // if bank is zero then paramAddrOffset = param
    r0 = r3;
    null = r1 - 0;
    if eq jump completedParamAddrCalculation;
    
    // if biquad is zero then paramAddrOffset = (bank-1)*xx + param + 1
    r1 = r1 - 1;
    r0 = m[r7 + $user_eq.DEFINITION_TABLE_MAX_NUM_BANDS];
    r0 = r0 * 4 (int);          // 4 parameters per band
    r0 = r0 + 2;                // 2 parameters (1 for num bands in use and 1 for gain)
    r1 = r1 * r0 (int);
    r0 = r1 + r3;
    r0 = r0 + 1;
    null = r2 - 0;
    if eq jump completedParamAddrCalculation;
    
    // paramAddrOffset = (bank-1)*xx + (biquad-1)*4 + param + 3
    r2 = r2 - 1;
    r2 = r2 * 4 (int);          // 4 parameters per band
    r0 = r0 + r2;
    r0 = r0 + 2;
        
    completedParamAddrCalculation:
        
    popm <r1,r2,r3>;
    
    rts;


//------------------------------------------------------------------------------
calcCoefAddrOffset:
//------------------------------------------------------------------------------
// calculate position of coefficients within coefficient table
//------------------------------------------------------------------------------
// on entry r0 = parameter ID
//          r7 = pointer to eq definition table
// on exit  r0 = address offset of coefficients in coefficient table
//          r1 = address offset of scale factors
// all other regsiters preserved
//------------------------------------------------------------------------------

    pushm <r2,r3>;

    // unlike Param address calculation, we're not interested in bank number
    r2 = r0 and 0x0000f0;       // biquad #
    r2 = r2 ashift -4;
    r3 = r0 and 0x00000f;       // param #
    
    // if biquad is zero then coefAddrOffset = param
    r0 = r3;
    null = r2 - 0;
    if eq jump completedCoefAddrCalculation;

    // calculate coefficient address of filter
    r2 = r2 - 1;
    r0 = m[r7 + $user_eq.DEFINITION_TABLE_MAX_NUM_BANDS];
    r0 = r0 * 5 (int);      // 5 coefficients per band (excluding scale)
    r0 = r0 + 3;            // 3 coefficients (1 for num bands in use and 2 for gain)
    r1 = r2 + r0;           // r1 = address of biquad scale
    r0 = r2 * 5 (int);      // 5 coefficients per band (excluding scale)
    r0 = r0 + 3;            // r0 = address of biquad coefficients
        
    completedCoefAddrCalculation:
        
    popm <r2,r3>;
    
    rts;


//------------------------------------------------------------------------------
coefCalc:
//------------------------------------------------------------------------------
// run appropriate coefficient calculation routine
// - The routines used contain the rts, so jump to the required routines
//------------------------------------------------------------------------------
// on entry r0 = parameter ID
//          i0 = address of biquad parameters
//          i1 = address of biquad coefficients
//          i2 = address of biquad scale
//------------------------------------------------------------------------------

    push rLink;
    
    // first check whether we're processing the number of biquads in use
    r2 = r0 and 0x0000f0;       // biquad #
    r2 = r2 ashift -4;
    if nz jump coefCalcBiquadBand.jump_entry;

    // biquad band is zero, so determine coefficient calculation on parameter #

    r0 = r0 and 0x00000f;
    null = r0 - $user_eq.num_bands;
    if eq jump calcNumBands.jump_entry;
    
    r2 = m[i0,1];               // update parameter pointer
    null = r0 - $user_eq.pre_gain;
    if eq jump calcPreGain.jump_entry;
    
    pop rLink;    
    rts;
    
    
//------------------------------------------------------------------------------
coefCalcBiquadBand.call_entry:
    push rLink;
coefCalcBiquadBand.jump_entry:
//------------------------------------------------------------------------------
// check filter type and run appropriate coefficient calculation routine
// - The routines used contain the rts, so jump to the required routines
//------------------------------------------------------------------------------
// on entry i0 = address of biquad parameters
//          i1 = address of biquad coefficients
//          i2 = address of biquad scale
//------------------------------------------------------------------------------

    r0 = m[i0,1];
    
    null = r0 - $user_eq.filter_type.bypass;
    if eq jump calcBypass.jump_entry;
    
    // 1st order Xpass filters use calcXP1 parameter checking and calling routine

    r7 = $kal_filter_coef_lib.calc_low_pass_1st;
    null = r0 - $user_eq.filter_type.lp_1;
    if eq jump calcXP1.jump_entry;

    r7 = $kal_filter_coef_lib.calc_high_pass_1st;
    null = r0 - $user_eq.filter_type.hp_1;
    if eq jump calcXP1.jump_entry;

    r7 = $kal_filter_coef_lib.calc_all_pass_1st;
    null = r0 - $user_eq.filter_type.ap_1;
    if eq jump calcXP1.jump_entry;
    
    // 2nd order Xpass filters use calcXP2 parameter checking and calling routine

    r7 = $kal_filter_coef_lib.calc_low_pass_2nd;
    null = r0 - $user_eq.filter_type.lp_2;
    if eq jump calcXP2.jump_entry;

    r7 = $kal_filter_coef_lib.calc_high_pass_2nd;
    null = r0 - $user_eq.filter_type.hp_2;
    if eq jump calcXP2.jump_entry;

    r7 = $kal_filter_coef_lib.calc_all_pass_2nd;
    null = r0 - $user_eq.filter_type.ap_2;
    if eq jump calcXP2.jump_entry;
    
    // 1st order Shelf filters use calcShelf1 parameter checking and calling routine
    
    r7 = $kal_filter_coef_lib.calc_low_shelf_1st;
    null = r0 - $user_eq.filter_type.ls_1;
    if eq jump calcShelf1.jump_entry;

    r7 = $kal_filter_coef_lib.calc_high_shelf_1st;
    null = r0 - $user_eq.filter_type.hs_1;
    if eq jump calcShelf1.jump_entry;

    r7 = $kal_filter_coef_lib.calc_tilt_1st;
    null = r0 - $user_eq.filter_type.tlt_1;
    if eq jump calcShelf1.jump_entry;
    
    // 2nd order Shelf filters use calcShelf2 parameter checking and calling routine
    
    r7 = $kal_filter_coef_lib.calc_low_shelf_2nd;
    null = r0 - $user_eq.filter_type.ls_2;
    if eq jump calcShelf2.jump_entry;

    r7 = $kal_filter_coef_lib.calc_high_shelf_2nd;
    null = r0 - $user_eq.filter_type.hs_2;
    if eq jump calcShelf2.jump_entry;

    r7 = $kal_filter_coef_lib.calc_tilt_2nd;
    null = r0 - $user_eq.filter_type.tlt_2;
    if eq jump calcShelf2.jump_entry;
    
    // parametric EQ is all on its own - poor little PEQ

    null = r0 - $user_eq.filter_type.peq;
    if eq jump calcPEQ.jump_entry;
    
    coefCalcInvalid:
   
    // not a valid filter type, so exit
    // - coefficients will get switched,
    //   but will be a copy of existing set with no changes

    pop rLink;    
    rts;
    


calcNumBands.call_entry:
//------------------------------------------------------------------------------
    push rLink;    

calcNumBands.jump_entry:

    r0 = m[i0,1];
    m[i1,1] = r0;

    pop rLink;    
    rts;



calcPreGain.call_entry:
//------------------------------------------------------------------------------
    push rLink;    

calcPreGain.jump_entry:

    // g
    r0 = $user_eq.gain_param_scale;
    call $kal_float_lib.int_to_float;
    r2 = r0;
    r3 = r1;
    r0 = m[i0,1];                       // gain (need to sign extend)
    r0 = r0 lshift 8;
    r0 = r0 ashift -8;
    // constrain parameter to specified range
    r1 = $user_eq.gain_lo_gain_limit;
    r0 = max r1;
    r1 = $user_eq.gain_hi_gain_limit;
    r0 = min r1;
    call $kal_float_lib.int_to_float;
    call $kal_float_lib.div;

    call $kal_filter_coef_lib.__db2lin.call_entry;
    // r0 = exponenet   [S---------------EEEEEEEE]
    // r1 = mantissa    [MMMMMMMMMMMMMMMMMMMMMMMM]

    r0 = r0 and 0x0000ff;       // expect sign to be zero, but mask off just in case
    r0 = r0 - 127;              // remove offset from exponent
    r1 = r1 lshift -1;          // shift mantissa so fits twos complement arithmetic
    
    m[i1,1] = r0;
    m[i1,1] = r1;

    pop rLink;    
    rts;



calcBypass.jump_entry:
//------------------------------------------------------------------------------

    // reserve space on stack for coefficients to be returned
    sp = sp + 6;
    
    call $kal_filter_coef_lib.calc_bypass;
    
    jump pop_biquad_coefs_and_rts;
    


calcXP1.jump_entry:
//------------------------------------------------------------------------------

    // reserve space on stack for coefficients to be returned
    sp = sp + 6;
    
    r0 = $user_eq.freq_param_scale;
    call $kal_float_lib.int_to_float;
    r2 = r0;
    r3 = r1;
    r0 = m[i0,1];                       // fc
    // constrain parameter to specified range
    r5 = 48000;
    r4 = m[$M.set_codec_rate.current_codec_sampling_rate];
    if z r4 = r5;						// Fs
	r5 = $user_eq.hi_nyquist_freq_limit;
	r4 = r4 * r5;						// 0.453515 * Fs (20 kHz @ 44.1k, 7.256 kHz @ 16k)
    r4 = r4 * 3 (int);                  // convert to same format as parameter (3 * Hz)
	null = r0 - r4;
	if gt jump substituteBypassFilter.jump_entry;	// Fs is too low for requested filter so use bypass
    r1 = $user_eq.xp_1_lo_freq_limit;
    r0 = max r1;
    r1 = $user_eq.xp_1_hi_freq_limit;
    r0 = min r1;
    call $kal_float_lib.int_to_float;
    call $kal_float_lib.div;
    pushm <r0,r1>;
    
    r1 = 48000;
    r0 = m[$M.set_codec_rate.current_codec_sampling_rate];
    if z r0 = r1;
    call $kal_float_lib.int_to_float;
    pushm <r0,r1>;                      // fs (default to 48kHz if not currently set
    
    call r7;
    
    jump pop_biquad_coefs_and_rts;



calcXP2.jump_entry:
//------------------------------------------------------------------------------

    // reserve space on stack for coefficients to be returned
    sp = sp + 6;
    
    // fc
    r0 = $user_eq.freq_param_scale;
    call $kal_float_lib.int_to_float;
    r2 = r0;
    r3 = r1;
    r0 = m[i0,1];                       // fc
    // constrain parameter to specified range
    r5 = 48000;
    r4 = m[$M.set_codec_rate.current_codec_sampling_rate];
    if z r4 = r5;						// Fs
	r5 = $user_eq.hi_nyquist_freq_limit;
	r4 = r4 * r5;						// 0.453515 * Fs (20 kHz @ 44.1k, 7.256 kHz @ 16k)
    r4 = r4 * 3 (int);                  // convert to same format as parameter (3 * Hz)
	null = r0 - r4;
	if gt jump substituteBypassFilter.jump_entry;	// Fs is too low for requested filter so use bypass
    r1 = $user_eq.xp_2_lo_freq_limit;
    r0 = max r1;
    r1 = $user_eq.xp_2_hi_freq_limit;
    r0 = min r1;
    call $kal_float_lib.int_to_float;
    call $kal_float_lib.div;
    pushm <r0,r1>;                      // fc
    
    r0 = m[i0,1];                     // gain is unused

    // q
    r0 = m[i0,1];
    // constrain parameter to specified range
    r1 = $user_eq.xp_2_lo_q_limit;
    r0 = max r1;
    r1 = $user_eq.xp_2_hi_q_limit;
    r0 = min r1;
    r1 = $user_eq.q_param_scale;
    call $kal_float_lib.q_to_float;
    pushm <r0,r1>;                      // q
    
    r1 = 48000;
    r0 = m[$M.set_codec_rate.current_codec_sampling_rate];
    if z r0 = r1;
    call $kal_float_lib.int_to_float;
    pushm <r0,r1>;                      // fs (default to 48kHz if not currently set
    
    call r7;
    
    jump pop_biquad_coefs_and_rts;



calcShelf1.jump_entry:
//------------------------------------------------------------------------------

    // reserve space on stack for coefficients to be returned
    sp = sp + 6;
    
    // fc
    r0 = $user_eq.freq_param_scale;
    call $kal_float_lib.int_to_float;
    r2 = r0;
    r3 = r1;
    r0 = m[i0,1];                       // fc
    // constrain parameter to specified range
    r5 = 48000;
    r4 = m[$M.set_codec_rate.current_codec_sampling_rate];
    if z r4 = r5;						// Fs
	r5 = $user_eq.hi_nyquist_freq_limit;
	r4 = r4 * r5;						// 0.453515 * Fs (20 kHz @ 44.1k, 7.256 kHz @ 16k)
    r4 = r4 * 3 (int);                  // convert to same format as parameter (3 * Hz)
	null = r0 - r4;
	if gt jump substituteBypassFilter.jump_entry;	// Fs is too low for requested filter so use bypass
    r1 = $user_eq.shelf_1_lo_freq_limit;
    r0 = max r1;
    r1 = $user_eq.shelf_1_hi_freq_limit;
    r0 = min r1;
    call $kal_float_lib.int_to_float;
    call $kal_float_lib.div;
    pushm <r0,r1>;                      // fc
    
    // g
    r0 = $user_eq.gain_param_scale;
    call $kal_float_lib.int_to_float;
    r2 = r0;
    r3 = r1;
    r0 = m[i0,1];                       // gain (need to sign extend)
    r0 = r0 lshift 8;
    r0 = r0 ashift -8;
    // constrain parameter to specified range
    r1 = $user_eq.shelf_1_lo_gain_limit;
    r0 = max r1;
    r1 = $user_eq.shelf_1_hi_gain_limit;
    r0 = min r1;
    call $kal_float_lib.int_to_float;
    call $kal_float_lib.div;
    pushm <r0,r1>;                      // g
    
    r1 = 48000;
    r0 = m[$M.set_codec_rate.current_codec_sampling_rate];
    if z r0 = r1;
    call $kal_float_lib.int_to_float;
    pushm <r0,r1>;                      // fs (default to 48kHz if not currently set
    
    call r7;
    
    jump pop_biquad_coefs_and_rts;
    


calcShelf2.jump_entry:
//------------------------------------------------------------------------------

    // reserve space on stack for coefficients to be returned
    sp = sp + 6;
    
    // fc
    r0 = $user_eq.freq_param_scale;
    call $kal_float_lib.int_to_float;
    r2 = r0;
    r3 = r1;
    r0 = m[i0,1];                       // fc
    // constrain parameter to specified range
    r5 = 48000;
    r4 = m[$M.set_codec_rate.current_codec_sampling_rate];
    if z r4 = r5;						// Fs
	r5 = $user_eq.hi_nyquist_freq_limit;
	r4 = r4 * r5;						// 0.453515 * Fs (20 kHz @ 44.1k, 7.256 kHz @ 16k)
    r4 = r4 * 3 (int);                  // convert to same format as parameter (3 * Hz)
	null = r0 - r4;
	if gt jump substituteBypassFilter.jump_entry;	// Fs is too low for requested filter so use bypass
    r1 = $user_eq.shelf_2_lo_freq_limit;
    r0 = max r1;
    r1 = $user_eq.shelf_2_hi_freq_limit;
    r0 = min r1;
    call $kal_float_lib.int_to_float;
    call $kal_float_lib.div;
    pushm <r0,r1>;                      // fc

    // g
    r0 = $user_eq.gain_param_scale;
    call $kal_float_lib.int_to_float;
    r2 = r0;
    r3 = r1;
    r0 = m[i0,1];                       // gain (need to sign extend)
    r0 = r0 lshift 8;
    r0 = r0 ashift -8;
    // constrain parameter to specified range
    r1 = $user_eq.shelf_2_lo_gain_limit;
    r0 = max r1;
    r1 = $user_eq.shelf_2_hi_gain_limit;
    r0 = min r1;
    call $kal_float_lib.int_to_float;
    call $kal_float_lib.div;
    pushm <r0,r1>;                      // g
    
    // q
    r0 = m[i0,1];
    // constrain parameter to specified range
    r1 = $user_eq.shelf_2_lo_q_limit;
    r0 = max r1;
    r1 = $user_eq.shelf_2_hi_q_limit;
    r0 = min r1;
    r1 = $user_eq.q_param_scale;
    call $kal_float_lib.q_to_float;
    pushm <r0,r1>;                      // q
    
    r1 = 48000;
    r0 = m[$M.set_codec_rate.current_codec_sampling_rate];
    if z r0 = r1;
    call $kal_float_lib.int_to_float;
    pushm <r0,r1>;                      // fs (default to 48kHz if not currently set
    
    call r7;
    
    jump pop_biquad_coefs_and_rts;



calcPEQ.jump_entry:
//------------------------------------------------------------------------------

    // reserve space on stack for coefficients to be returned
    sp = sp + 6;
    
    // fc
    r0 = $user_eq.freq_param_scale;
    call $kal_float_lib.int_to_float;
    r2 = r0;
    r3 = r1;
    r0 = m[i0,1];                       // fc
    // constrain parameter to specified range
    r5 = 48000;
    r4 = m[$M.set_codec_rate.current_codec_sampling_rate];
    if z r4 = r5;						// Fs
	r5 = $user_eq.hi_nyquist_freq_limit;
	r4 = r4 * r5;						// 0.453515 * Fs (20 kHz @ 44.1k, 7.256 kHz @ 16k)
    r4 = r4 * 3 (int);                  // convert to same format as parameter (3 * Hz)
	null = r0 - r4;
	if gt jump substituteBypassFilter.jump_entry;	// Fs is too low for requested filter so use bypass
    r1 = $user_eq.peq_lo_freq_limit;
    r0 = max r1;
    r1 = $user_eq.peq_hi_freq_limit;
    r0 = min r1;
    call $kal_float_lib.int_to_float;
    call $kal_float_lib.div;
    pushm <r0,r1>;                      // fc

    // g
    r0 = $user_eq.gain_param_scale;
    call $kal_float_lib.int_to_float;
    r2 = r0;
    r3 = r1;
    r0 = m[i0,1];                       // gain (need to sign extend)
    r0 = r0 lshift 8;
    r0 = r0 ashift -8;
    // constrain parameter to specified range
    r1 = $user_eq.peq_lo_gain_limit;
    r0 = max r1;
    r1 = $user_eq.peq_hi_gain_limit;
    r0 = min r1;
    call $kal_float_lib.int_to_float;
    call $kal_float_lib.div;
    pushm <r0,r1>;                      // g
    
    // q
    r0 = m[i0,1];
    // constrain parameter to specified range
    r1 = $user_eq.peq_lo_q_limit;
    r0 = max r1;
    r1 = $user_eq.peq_hi_q_limit;
    r0 = min r1;
    r1 = $user_eq.q_param_scale;
    call $kal_float_lib.q_to_float;
    pushm <r0,r1>;                      // q
    
    r1 = 48000;
    r0 = m[$M.set_codec_rate.current_codec_sampling_rate];
    if z r0 = r1;
    call $kal_float_lib.int_to_float;
    pushm <r0,r1>;                      // fs (default to 48kHz if not currently set
    
    call $kal_filter_coef_lib.calc_peq;
    
    jump pop_biquad_coefs_and_rts;



substituteBypassFilter.jump_entry:
//------------------------------------------------------------------------------

    call $kal_filter_coef_lib.calc_bypass;
    
    jump pop_biquad_coefs_and_rts;
    


pop_biquad_coefs_and_rts:
//------------------------------------------------------------------------------
    
    pop r0;             // b2
    m[i1,1] = r0;
    pop r0;             // b1
    m[i1,1] = r0;
    pop r0;             // b0
    m[i1,1] = r0;
    pop r0;             // a2
    m[i1,1] = r0;
    pop r0;             // a1
    m[i1,1] = r0;
    pop r0;             // scale
    m[i2,1] = r0;

    pop rLink;    
    rts;




.endmodule;



//------------------------------------------------------------------------------
.module $M.a2dp_low_latency_msg.SetUserEqParamMsg;
//------------------------------------------------------------------------------
// receives user eq parameter update message.
//   If "update" field is non-zero, the coefficient calculation routine is run
//------------------------------------------------------------------------------
// Parameter is sent as a short message
//   <msgID> <Param ID> <value> <update> <>
//------------------------------------------------------------------------------
// On entry
//   r0 = message ID (ignored)
//   r1 = parameter ID
//   r2 = value
//   r3 = update
//   r4 = unused
//------------------------------------------------------------------------------

    .datasegment dm ;
    .codesegment pm ;

func:

    $push_rLink_macro ;

    r0 = r1;                // r0 = paramID
    r7 = &$M.system_config.data.UserEqDefnTable;
    call $user_eq.calcParamAddrOffset;          // on exit, r0 = ParamAddrOffset
    r0 = r0 + ($M.CVC.data.CurParams + $M.A2DP_LOW_LATENCY_2MIC.PARAMETERS.OFFSET_USER_EQ_NUM_BANKS);
        
    r2 = r2 and 0x00ffff;
    m[r0] = r2;
    
    // if update flag is zero then exit now and don't recalculate coefficients
    null = r3 - 0;
    if eq jump $pop_rLink_and_rts ;
    
    r0 = r1;
    r1 = &$M.system_config.data.UserEqCoefsA;
    r2 = &$M.system_config.data.UserEqCoefsB;
    r3 = ($M.CVC.data.CurParams + $M.A2DP_LOW_LATENCY_2MIC.PARAMETERS.OFFSET_USER_EQ_NUM_BANKS);
    call $user_eq.calcBandCoefs;

    jump $pop_rLink_and_rts ;

.endmodule ;


//------------------------------------------------------------------------------
.module $M.a2dp_low_latency_msg.GetUserEqParamMsg;
//------------------------------------------------------------------------------
// request user eq parameter message.
//   return message contains requested parameter
//------------------------------------------------------------------------------
// Parameter is sent as a short message
//   <msgID> <Param ID> <> <> <>
// Reply is sent as a short message
//   <msgID> <Param ID> <value> <> <>
//------------------------------------------------------------------------------
// On entry
//   r0 = message ID (ignored)
//   r1 = parameter ID
//   r2 = unused
//   r3 = unused
//   r4 = unused
//------------------------------------------------------------------------------

    .datasegment dm ;
    .codesegment pm ;

func:

    $push_rLink_macro ;

    r3 = r1;          // word 1 of return message (parameter ID)

    r0 = r1;                // r0 = paramID
    r7 = &$M.system_config.data.UserEqDefnTable;
    call $user_eq.calcParamAddrOffset;          // on exit, r0 = ParamAddrOffset
    r0 = r0 + ($M.CVC.data.CurParams + $M.A2DP_LOW_LATENCY_2MIC.PARAMETERS.OFFSET_USER_EQ_NUM_BANKS);
        
    r4 = m[r0];       // word 2 (value)
    r5 = 0;           // word 3
    r6 = 0;           // word 4
    r2 = $user_eq.GAIAMSG.GET_USER_PARAM_RESP;
    call $message.send_short;

    jump $pop_rLink_and_rts ;

.endmodule ;


//------------------------------------------------------------------------------
.module $M.a2dp_low_latency_msg.SetUserEqGroupParamMsg;
//------------------------------------------------------------------------------
// receives user eq group parameter update message.
//   If "update" field is non-zero, the coefficient calculation routine is run
//------------------------------------------------------------------------------
// Parameter is sent as a long message
//   <numParams> <Param-1> <value-1>...<Param-n> <value-n> <update>
//------------------------------------------------------------------------------
// On entry
//   r1 = message ID (ignored)
//   r2 = message length in words
//   r3 = message
//------------------------------------------------------------------------------

    .datasegment dm ;
    .codesegment pm ;

func:

    $push_rLink_macro ;

    i0 = r3;                // i0 ptr to message
    r1 = m[i0,1];           // number of parameters to retrieve
    r7 = &$M.system_config.data.UserEqDefnTable;
    r10 = r1;
    do SetParamsLoop;
        r0 = m[i0,1];       // r0 = paramID
        call $user_eq.calcParamAddrOffset;          // on exit, r0 = ParamAddrOffset
        r0 = r0 + ($M.CVC.data.CurParams + $M.A2DP_LOW_LATENCY_2MIC.PARAMETERS.OFFSET_USER_EQ_NUM_BANKS);
        r1 = m[i0,1];       // get value to store
        r1 = r1 and 0x00ffff;
        m[r0] = r1;         // store value
    SetParamsLoop:
    
    //// currently ignore update flag for group parameter message
    
    jump $pop_rLink_and_rts ;

.endmodule ;


//------------------------------------------------------------------------------
.module $M.a2dp_low_latency_msg.GetUserEqGroupParamMsg;
//------------------------------------------------------------------------------
// request user eq group parameter message.
//   return LONG message contains requested parameters
//------------------------------------------------------------------------------
// Parameter is sent as a long message
//   <numParams> <Param-1> < 00000 >...<Param-n> < 00000 >
// Reply is sent as a long message
//   <numParams> <Param-1> <value-1>...<Param-n> <value-n>
//------------------------------------------------------------------------------
// On entry
//   r1 = message ID (ignored)
//   r2 = message length in words
//   r3 = message
//------------------------------------------------------------------------------

    .datasegment dm ;
    .codesegment pm ;

func:

    $push_rLink_macro ;
    
    i0 = r3;                // i0 ptr to message
    r1 = m[i0,1];           // number of parameters to retrieve
    r7 = &$M.system_config.data.UserEqDefnTable;
    r10 = r1;
    do GetParamsLoop;
        r0 = m[i0,1];               // r0 = paramID
        call $user_eq.calcParamAddrOffset;          // on exit, r0 = ParamAddrOffset
        r0 = r0 + ($M.CVC.data.CurParams + $M.A2DP_LOW_LATENCY_2MIC.PARAMETERS.OFFSET_USER_EQ_NUM_BANKS);
        r0 = m[r0];         // r0 = paramter value
        m[i0,1] = r0;       // store parameter back into message to return to VM
    GetParamsLoop:
    
    r5 = r3;
    r4 = r2;
    r3 = $user_eq.GAIAMSG.GET_USER_GROUP_PARAM_RESP;
    call $message.send_long;    

    jump $pop_rLink_and_rts ;

.endmodule ;

