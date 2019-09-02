/*******************************************************************************
Copyright (c) 2015 Qualcomm Technologies International, Ltd.
 Part of ADK 4.1
*******************************************************************************/

//------------------------------------------------------------------------------
// Example default coefficients file for speaker and bass boost filtering
//------------------------------------------------------------------------------


// The #defines below control whether the DSP code uses the UFE defined parmeters
// or uses the precalculated coefficients
// Setting the define to '0' uses the UFE defined paramters and the coefficient calculation
// setting the define to '1' uses the precalculated coefficients

#define USE_PRECALCULATED_USER_COEFS        0  // change to '1' to force app to use these coefficients for the user EQ
#define USE_PRECALCULATED_SPKR_COEFS        0  // change to '1' to force app to use these coefficients for the speaker EQ
#define USE_PRECALCULATED_BOOST_COEFS       0  // change to '1' to force app to use these coefficients for the bass boost
#define USE_PRECALCULATED_ANC_COEFS         0  // change to '1' to force app to use these coefficients for the ANC PEQ
#define USE_PRECALCULATED_WIRED_SUB_COEFS   0  // change to '1' to force app to use these coefficients for the wired subwoofer EQ

// 44.1 kHz coefficients

.const $userEq.Fs44.NumBanks                6;

.const $userEq.Fs44.Bank1.NumBands          0;
// Gain = 0.0 dB
.const $userEq.Fs44.Bank1.GainExp           1;
.const $userEq.Fs44.Bank1.GainMant          0.5;
// Type = Bypass
.const $userEq.Fs44.Bank1.Stage1.b0         0.5;
.const $userEq.Fs44.Bank1.Stage1.b1         0;
.const $userEq.Fs44.Bank1.Stage1.b2         0;
.const $userEq.Fs44.Bank1.Stage1.a1         0;
.const $userEq.Fs44.Bank1.Stage1.a2         0;
.const $userEq.Fs44.Bank1.Stage1.scale      1;
// Type = Bypass
.const $userEq.Fs44.Bank1.Stage2.b0         0.5;
.const $userEq.Fs44.Bank1.Stage2.b1         0;
.const $userEq.Fs44.Bank1.Stage2.b2         0;
.const $userEq.Fs44.Bank1.Stage2.a1         0;
.const $userEq.Fs44.Bank1.Stage2.a2         0;
.const $userEq.Fs44.Bank1.Stage2.scale      1;
// Type = Bypass
.const $userEq.Fs44.Bank1.Stage3.b0         0.5;
.const $userEq.Fs44.Bank1.Stage3.b1         0;
.const $userEq.Fs44.Bank1.Stage3.b2         0;
.const $userEq.Fs44.Bank1.Stage3.a1         0;
.const $userEq.Fs44.Bank1.Stage3.a2         0;
.const $userEq.Fs44.Bank1.Stage3.scale      1;
// Type = Bypass
.const $userEq.Fs44.Bank1.Stage4.b0         0.5;
.const $userEq.Fs44.Bank1.Stage4.b1         0;
.const $userEq.Fs44.Bank1.Stage4.b2         0;
.const $userEq.Fs44.Bank1.Stage4.a1         0;
.const $userEq.Fs44.Bank1.Stage4.a2         0;
.const $userEq.Fs44.Bank1.Stage4.scale      1;
// Type = Bypass
.const $userEq.Fs44.Bank1.Stage5.b0         0.5;
.const $userEq.Fs44.Bank1.Stage5.b1         0;
.const $userEq.Fs44.Bank1.Stage5.b2         0;
.const $userEq.Fs44.Bank1.Stage5.a1         0;
.const $userEq.Fs44.Bank1.Stage5.a2         0;
.const $userEq.Fs44.Bank1.Stage5.scale      1;

.const $userEq.Fs44.Bank2.NumBands          0;
// Gain = 0.0 dB
.const $userEq.Fs44.Bank2.GainExp           1;
.const $userEq.Fs44.Bank2.GainMant          0.5;
// Type = Bypass
.const $userEq.Fs44.Bank2.Stage1.b0         0.5;
.const $userEq.Fs44.Bank2.Stage1.b1         0;
.const $userEq.Fs44.Bank2.Stage1.b2         0;
.const $userEq.Fs44.Bank2.Stage1.a1         0;
.const $userEq.Fs44.Bank2.Stage1.a2         0;
.const $userEq.Fs44.Bank2.Stage1.scale      1;
// Type = Bypass
.const $userEq.Fs44.Bank2.Stage2.b0         0.5;
.const $userEq.Fs44.Bank2.Stage2.b1         0;
.const $userEq.Fs44.Bank2.Stage2.b2         0;
.const $userEq.Fs44.Bank2.Stage2.a1         0;
.const $userEq.Fs44.Bank2.Stage2.a2         0;
.const $userEq.Fs44.Bank2.Stage2.scale      1;
// Type = Bypass
.const $userEq.Fs44.Bank2.Stage3.b0         0.5;
.const $userEq.Fs44.Bank2.Stage3.b1         0;
.const $userEq.Fs44.Bank2.Stage3.b2         0;
.const $userEq.Fs44.Bank2.Stage3.a1         0;
.const $userEq.Fs44.Bank2.Stage3.a2         0;
.const $userEq.Fs44.Bank2.Stage3.scale      1;
// Type = Bypass
.const $userEq.Fs44.Bank2.Stage4.b0         0.5;
.const $userEq.Fs44.Bank2.Stage4.b1         0;
.const $userEq.Fs44.Bank2.Stage4.b2         0;
.const $userEq.Fs44.Bank2.Stage4.a1         0;
.const $userEq.Fs44.Bank2.Stage4.a2         0;
.const $userEq.Fs44.Bank2.Stage4.scale      1;
// Type = Bypass
.const $userEq.Fs44.Bank2.Stage5.b0         0.5;
.const $userEq.Fs44.Bank2.Stage5.b1         0;
.const $userEq.Fs44.Bank2.Stage5.b2         0;
.const $userEq.Fs44.Bank2.Stage5.a1         0;
.const $userEq.Fs44.Bank2.Stage5.a2         0;
.const $userEq.Fs44.Bank2.Stage5.scale      1;

.const $userEq.Fs44.Bank3.NumBands          0;
// Gain = 0.0 dB
.const $userEq.Fs44.Bank3.GainExp           1;
.const $userEq.Fs44.Bank3.GainMant          0.5;
// Type = Bypass
.const $userEq.Fs44.Bank3.Stage1.b0         0.5;
.const $userEq.Fs44.Bank3.Stage1.b1         0;
.const $userEq.Fs44.Bank3.Stage1.b2         0;
.const $userEq.Fs44.Bank3.Stage1.a1         0;
.const $userEq.Fs44.Bank3.Stage1.a2         0;
.const $userEq.Fs44.Bank3.Stage1.scale      1;
// Type = Bypass
.const $userEq.Fs44.Bank3.Stage2.b0         0.5;
.const $userEq.Fs44.Bank3.Stage2.b1         0;
.const $userEq.Fs44.Bank3.Stage2.b2         0;
.const $userEq.Fs44.Bank3.Stage2.a1         0;
.const $userEq.Fs44.Bank3.Stage2.a2         0;
.const $userEq.Fs44.Bank3.Stage2.scale      1;
// Type = Bypass
.const $userEq.Fs44.Bank3.Stage3.b0         0.5;
.const $userEq.Fs44.Bank3.Stage3.b1         0;
.const $userEq.Fs44.Bank3.Stage3.b2         0;
.const $userEq.Fs44.Bank3.Stage3.a1         0;
.const $userEq.Fs44.Bank3.Stage3.a2         0;
.const $userEq.Fs44.Bank3.Stage3.scale      1;
// Type = Bypass
.const $userEq.Fs44.Bank3.Stage4.b0         0.5;
.const $userEq.Fs44.Bank3.Stage4.b1         0;
.const $userEq.Fs44.Bank3.Stage4.b2         0;
.const $userEq.Fs44.Bank3.Stage4.a1         0;
.const $userEq.Fs44.Bank3.Stage4.a2         0;
.const $userEq.Fs44.Bank3.Stage4.scale      1;
// Type = Bypass
.const $userEq.Fs44.Bank3.Stage5.b0         0.5;
.const $userEq.Fs44.Bank3.Stage5.b1         0;
.const $userEq.Fs44.Bank3.Stage5.b2         0;
.const $userEq.Fs44.Bank3.Stage5.a1         0;
.const $userEq.Fs44.Bank3.Stage5.a2         0;
.const $userEq.Fs44.Bank3.Stage5.scale      1;

.const $userEq.Fs44.Bank4.NumBands          0;
// Gain = 0.0 dB
.const $userEq.Fs44.Bank4.GainExp           1;
.const $userEq.Fs44.Bank4.GainMant          0.5;
// Type = Bypass
.const $userEq.Fs44.Bank4.Stage1.b0         0.5;
.const $userEq.Fs44.Bank4.Stage1.b1         0;
.const $userEq.Fs44.Bank4.Stage1.b2         0;
.const $userEq.Fs44.Bank4.Stage1.a1         0;
.const $userEq.Fs44.Bank4.Stage1.a2         0;
.const $userEq.Fs44.Bank4.Stage1.scale      1;
// Type = Bypass
.const $userEq.Fs44.Bank4.Stage2.b0         0.5;
.const $userEq.Fs44.Bank4.Stage2.b1         0;
.const $userEq.Fs44.Bank4.Stage2.b2         0;
.const $userEq.Fs44.Bank4.Stage2.a1         0;
.const $userEq.Fs44.Bank4.Stage2.a2         0;
.const $userEq.Fs44.Bank4.Stage2.scale      1;
// Type = Bypass
.const $userEq.Fs44.Bank4.Stage3.b0         0.5;
.const $userEq.Fs44.Bank4.Stage3.b1         0;
.const $userEq.Fs44.Bank4.Stage3.b2         0;
.const $userEq.Fs44.Bank4.Stage3.a1         0;
.const $userEq.Fs44.Bank4.Stage3.a2         0;
.const $userEq.Fs44.Bank4.Stage3.scale      1;
// Type = Bypass
.const $userEq.Fs44.Bank4.Stage4.b0         0.5;
.const $userEq.Fs44.Bank4.Stage4.b1         0;
.const $userEq.Fs44.Bank4.Stage4.b2         0;
.const $userEq.Fs44.Bank4.Stage4.a1         0;
.const $userEq.Fs44.Bank4.Stage4.a2         0;
.const $userEq.Fs44.Bank4.Stage4.scale      1;
// Type = Bypass
.const $userEq.Fs44.Bank4.Stage5.b0         0.5;
.const $userEq.Fs44.Bank4.Stage5.b1         0;
.const $userEq.Fs44.Bank4.Stage5.b2         0;
.const $userEq.Fs44.Bank4.Stage5.a1         0;
.const $userEq.Fs44.Bank4.Stage5.a2         0;
.const $userEq.Fs44.Bank4.Stage5.scale      1;

.const $userEq.Fs44.Bank5.NumBands          0;
// Gain = 0.0 dB
.const $userEq.Fs44.Bank5.GainExp           1;
.const $userEq.Fs44.Bank5.GainMant          0.5;
// Type = Bypass
.const $userEq.Fs44.Bank5.Stage1.b0         0.5;
.const $userEq.Fs44.Bank5.Stage1.b1         0;
.const $userEq.Fs44.Bank5.Stage1.b2         0;
.const $userEq.Fs44.Bank5.Stage1.a1         0;
.const $userEq.Fs44.Bank5.Stage1.a2         0;
.const $userEq.Fs44.Bank5.Stage1.scale      1;
// Type = Bypass
.const $userEq.Fs44.Bank5.Stage2.b0         0.5;
.const $userEq.Fs44.Bank5.Stage2.b1         0;
.const $userEq.Fs44.Bank5.Stage2.b2         0;
.const $userEq.Fs44.Bank5.Stage2.a1         0;
.const $userEq.Fs44.Bank5.Stage2.a2         0;
.const $userEq.Fs44.Bank5.Stage2.scale      1;
// Type = Bypass
.const $userEq.Fs44.Bank5.Stage3.b0         0.5;
.const $userEq.Fs44.Bank5.Stage3.b1         0;
.const $userEq.Fs44.Bank5.Stage3.b2         0;
.const $userEq.Fs44.Bank5.Stage3.a1         0;
.const $userEq.Fs44.Bank5.Stage3.a2         0;
.const $userEq.Fs44.Bank5.Stage3.scale      1;
// Type = Bypass
.const $userEq.Fs44.Bank5.Stage4.b0         0.5;
.const $userEq.Fs44.Bank5.Stage4.b1         0;
.const $userEq.Fs44.Bank5.Stage4.b2         0;
.const $userEq.Fs44.Bank5.Stage4.a1         0;
.const $userEq.Fs44.Bank5.Stage4.a2         0;
.const $userEq.Fs44.Bank5.Stage4.scale      1;
// Type = Bypass
.const $userEq.Fs44.Bank5.Stage5.b0         0.5;
.const $userEq.Fs44.Bank5.Stage5.b1         0;
.const $userEq.Fs44.Bank5.Stage5.b2         0;
.const $userEq.Fs44.Bank5.Stage5.a1         0;
.const $userEq.Fs44.Bank5.Stage5.a2         0;
.const $userEq.Fs44.Bank5.Stage5.scale      1;

.const $userEq.Fs44.Bank6.NumBands          0;
// Gain = 0.0 dB
.const $userEq.Fs44.Bank6.GainExp           1;
.const $userEq.Fs44.Bank6.GainMant          0.5;
// Type = Bypass
.const $userEq.Fs44.Bank6.Stage1.b0         0.5;
.const $userEq.Fs44.Bank6.Stage1.b1         0;
.const $userEq.Fs44.Bank6.Stage1.b2         0;
.const $userEq.Fs44.Bank6.Stage1.a1         0;
.const $userEq.Fs44.Bank6.Stage1.a2         0;
.const $userEq.Fs44.Bank6.Stage1.scale      1;
// Type = Bypass
.const $userEq.Fs44.Bank6.Stage2.b0         0.5;
.const $userEq.Fs44.Bank6.Stage2.b1         0;
.const $userEq.Fs44.Bank6.Stage2.b2         0;
.const $userEq.Fs44.Bank6.Stage2.a1         0;
.const $userEq.Fs44.Bank6.Stage2.a2         0;
.const $userEq.Fs44.Bank6.Stage2.scale      1;
// Type = Bypass
.const $userEq.Fs44.Bank6.Stage3.b0         0.5;
.const $userEq.Fs44.Bank6.Stage3.b1         0;
.const $userEq.Fs44.Bank6.Stage3.b2         0;
.const $userEq.Fs44.Bank6.Stage3.a1         0;
.const $userEq.Fs44.Bank6.Stage3.a2         0;
.const $userEq.Fs44.Bank6.Stage3.scale      1;
// Type = Bypass
.const $userEq.Fs44.Bank6.Stage4.b0         0.5;
.const $userEq.Fs44.Bank6.Stage4.b1         0;
.const $userEq.Fs44.Bank6.Stage4.b2         0;
.const $userEq.Fs44.Bank6.Stage4.a1         0;
.const $userEq.Fs44.Bank6.Stage4.a2         0;
.const $userEq.Fs44.Bank6.Stage4.scale      1;
// Type = Bypass
.const $userEq.Fs44.Bank6.Stage5.b0         0.5;
.const $userEq.Fs44.Bank6.Stage5.b1         0;
.const $userEq.Fs44.Bank6.Stage5.b2         0;
.const $userEq.Fs44.Bank6.Stage5.a1         0;
.const $userEq.Fs44.Bank6.Stage5.a2         0;
.const $userEq.Fs44.Bank6.Stage5.scale      1;


.const $BoostEq.Fs44.NumBands               1;
// Gain = 0.0 dB
.const $BoostEq.Fs44.GainExp                1;
.const $BoostEq.Fs44.GainMant               0.5;
// Type = Bypass
.const $BoostEq.Fs44.b0                     0.5;
.const $BoostEq.Fs44.b1                     0;
.const $BoostEq.Fs44.b2                     0;
.const $BoostEq.Fs44.a1                     0;
.const $BoostEq.Fs44.a2                     0;
.const $BoostEq.Fs44.scale                  1;

.const $AncEq.Fs44.NumBands                5;
.const $AncEq.Fs44.GainExp                 1;
.const $AncEq.Fs44.GainMant                0.5;
.const $AncEq.Fs44.Stage1.b0               0.5;
.const $AncEq.Fs44.Stage1.b1               0;
.const $AncEq.Fs44.Stage1.b2               0;
.const $AncEq.Fs44.Stage1.a1               0;
.const $AncEq.Fs44.Stage1.a2               0;
.const $AncEq.Fs44.Stage1.scale            1;
.const $AncEq.Fs44.Stage2.b0               0.5;
.const $AncEq.Fs44.Stage2.b1               0;
.const $AncEq.Fs44.Stage2.b2               0;
.const $AncEq.Fs44.Stage2.a1               0;
.const $AncEq.Fs44.Stage2.a2               0;
.const $AncEq.Fs44.Stage2.scale            1;
.const $AncEq.Fs44.Stage3.b0               0.5;
.const $AncEq.Fs44.Stage3.b1               0;
.const $AncEq.Fs44.Stage3.b2               0;
.const $AncEq.Fs44.Stage3.a1               0;
.const $AncEq.Fs44.Stage3.a2               0;
.const $AncEq.Fs44.Stage3.scale            1;
.const $AncEq.Fs44.Stage4.b0               0.5;
.const $AncEq.Fs44.Stage4.b1               0;
.const $AncEq.Fs44.Stage4.b2               0;
.const $AncEq.Fs44.Stage4.a1               0;
.const $AncEq.Fs44.Stage4.a2               0;
.const $AncEq.Fs44.Stage4.scale            1;
.const $AncEq.Fs44.Stage5.b0               0.5;
.const $AncEq.Fs44.Stage5.b1               0;
.const $AncEq.Fs44.Stage5.b2               0;
.const $AncEq.Fs44.Stage5.a1               0;
.const $AncEq.Fs44.Stage5.a2               0;
.const $AncEq.Fs44.Stage5.scale            1;
.const $spkrEq.Fs44.NumBands                10;
// Gain = 0.0 dB
.const $spkrEq.Fs44.GainExp                 1;
.const $spkrEq.Fs44.GainMant                0.5;
// Type = Bypass
.const $spkrEq.Fs44.Stage1.b0               0.5;
.const $spkrEq.Fs44.Stage1.b1               0;
.const $spkrEq.Fs44.Stage1.b2               0;
.const $spkrEq.Fs44.Stage1.a1               0;
.const $spkrEq.Fs44.Stage1.a2               0;
.const $spkrEq.Fs44.Stage1.scale            1;
// Type = Bypass
.const $spkrEq.Fs44.Stage2.b0               0.5;
.const $spkrEq.Fs44.Stage2.b1               0;
.const $spkrEq.Fs44.Stage2.b2               0;
.const $spkrEq.Fs44.Stage2.a1               0;
.const $spkrEq.Fs44.Stage2.a2               0;
.const $spkrEq.Fs44.Stage2.scale            1;
// Type = Bypass
.const $spkrEq.Fs44.Stage3.b0               0.5;
.const $spkrEq.Fs44.Stage3.b1               0;
.const $spkrEq.Fs44.Stage3.b2               0;
.const $spkrEq.Fs44.Stage3.a1               0;
.const $spkrEq.Fs44.Stage3.a2               0;
.const $spkrEq.Fs44.Stage3.scale            1;
// Type = Bypass
.const $spkrEq.Fs44.Stage4.b0               0.5;
.const $spkrEq.Fs44.Stage4.b1               0;
.const $spkrEq.Fs44.Stage4.b2               0;
.const $spkrEq.Fs44.Stage4.a1               0;
.const $spkrEq.Fs44.Stage4.a2               0;
.const $spkrEq.Fs44.Stage4.scale            1;
// Type = Bypass
.const $spkrEq.Fs44.Stage5.b0               0.5;
.const $spkrEq.Fs44.Stage5.b1               0;
.const $spkrEq.Fs44.Stage5.b2               0;
.const $spkrEq.Fs44.Stage5.a1               0;
.const $spkrEq.Fs44.Stage5.a2               0;
.const $spkrEq.Fs44.Stage5.scale            1;
// Type = Bypass
.const $spkrEq.Fs44.Stage6.b0               0.5;
.const $spkrEq.Fs44.Stage6.b1               0;
.const $spkrEq.Fs44.Stage6.b2               0;
.const $spkrEq.Fs44.Stage6.a1               0;
.const $spkrEq.Fs44.Stage6.a2               0;
.const $spkrEq.Fs44.Stage6.scale            1;
// Type = Bypass
.const $spkrEq.Fs44.Stage7.b0               0.5;
.const $spkrEq.Fs44.Stage7.b1               0;
.const $spkrEq.Fs44.Stage7.b2               0;
.const $spkrEq.Fs44.Stage7.a1               0;
.const $spkrEq.Fs44.Stage7.a2               0;
.const $spkrEq.Fs44.Stage7.scale            1;
// Type = Bypass
.const $spkrEq.Fs44.Stage8.b0               0.5;
.const $spkrEq.Fs44.Stage8.b1               0;
.const $spkrEq.Fs44.Stage8.b2               0;
.const $spkrEq.Fs44.Stage8.a1               0;
.const $spkrEq.Fs44.Stage8.a2               0;
.const $spkrEq.Fs44.Stage8.scale            1;
// Type = Bypass
.const $spkrEq.Fs44.Stage9.b0               0.5;
.const $spkrEq.Fs44.Stage9.b1               0;
.const $spkrEq.Fs44.Stage9.b2               0;
.const $spkrEq.Fs44.Stage9.a1               0;
.const $spkrEq.Fs44.Stage9.a2               0;
.const $spkrEq.Fs44.Stage9.scale            1;
// Type = Bypass
.const $spkrEq.Fs44.Stage10.b0              0.5;
.const $spkrEq.Fs44.Stage10.b1              0;
.const $spkrEq.Fs44.Stage10.b2              0;
.const $spkrEq.Fs44.Stage10.a1              0;
.const $spkrEq.Fs44.Stage10.a2              0;
.const $spkrEq.Fs44.Stage10.scale           1;

// 48 kHz coefficients

.const $userEq.Fs48.NumBanks                6;

.const $userEq.Fs48.Bank1.NumBands          0;
// Gain = 0.0 dB
.const $userEq.Fs48.Bank1.GainExp           1;
.const $userEq.Fs48.Bank1.GainMant          0.5;
// Type = Bypass
.const $userEq.Fs48.Bank1.Stage1.b0         0.5;
.const $userEq.Fs48.Bank1.Stage1.b1         0;
.const $userEq.Fs48.Bank1.Stage1.b2         0;
.const $userEq.Fs48.Bank1.Stage1.a1         0;
.const $userEq.Fs48.Bank1.Stage1.a2         0;
.const $userEq.Fs48.Bank1.Stage1.scale      1;
// Type = Bypass
.const $userEq.Fs48.Bank1.Stage2.b0         0.5;
.const $userEq.Fs48.Bank1.Stage2.b1         0;
.const $userEq.Fs48.Bank1.Stage2.b2         0;
.const $userEq.Fs48.Bank1.Stage2.a1         0;
.const $userEq.Fs48.Bank1.Stage2.a2         0;
.const $userEq.Fs48.Bank1.Stage2.scale      1;
// Type = Bypass
.const $userEq.Fs48.Bank1.Stage3.b0         0.5;
.const $userEq.Fs48.Bank1.Stage3.b1         0;
.const $userEq.Fs48.Bank1.Stage3.b2         0;
.const $userEq.Fs48.Bank1.Stage3.a1         0;
.const $userEq.Fs48.Bank1.Stage3.a2         0;
.const $userEq.Fs48.Bank1.Stage3.scale      1;
// Type = Bypass
.const $userEq.Fs48.Bank1.Stage4.b0         0.5;
.const $userEq.Fs48.Bank1.Stage4.b1         0;
.const $userEq.Fs48.Bank1.Stage4.b2         0;
.const $userEq.Fs48.Bank1.Stage4.a1         0;
.const $userEq.Fs48.Bank1.Stage4.a2         0;
.const $userEq.Fs48.Bank1.Stage4.scale      1;
// Type = Bypass
.const $userEq.Fs48.Bank1.Stage5.b0         0.5;
.const $userEq.Fs48.Bank1.Stage5.b1         0;
.const $userEq.Fs48.Bank1.Stage5.b2         0;
.const $userEq.Fs48.Bank1.Stage5.a1         0;
.const $userEq.Fs48.Bank1.Stage5.a2         0;
.const $userEq.Fs48.Bank1.Stage5.scale      1;

.const $userEq.Fs48.Bank2.NumBands          0;
// Gain = 0.0 dB
.const $userEq.Fs48.Bank2.GainExp           1;
.const $userEq.Fs48.Bank2.GainMant          0.5;
// Type = Bypass
.const $userEq.Fs48.Bank2.Stage1.b0         0.5;
.const $userEq.Fs48.Bank2.Stage1.b1         0;
.const $userEq.Fs48.Bank2.Stage1.b2         0;
.const $userEq.Fs48.Bank2.Stage1.a1         0;
.const $userEq.Fs48.Bank2.Stage1.a2         0;
.const $userEq.Fs48.Bank2.Stage1.scale      1;
// Type = Bypass
.const $userEq.Fs48.Bank2.Stage2.b0         0.5;
.const $userEq.Fs48.Bank2.Stage2.b1         0;
.const $userEq.Fs48.Bank2.Stage2.b2         0;
.const $userEq.Fs48.Bank2.Stage2.a1         0;
.const $userEq.Fs48.Bank2.Stage2.a2         0;
.const $userEq.Fs48.Bank2.Stage2.scale      1;
// Type = Bypass
.const $userEq.Fs48.Bank2.Stage3.b0         0.5;
.const $userEq.Fs48.Bank2.Stage3.b1         0;
.const $userEq.Fs48.Bank2.Stage3.b2         0;
.const $userEq.Fs48.Bank2.Stage3.a1         0;
.const $userEq.Fs48.Bank2.Stage3.a2         0;
.const $userEq.Fs48.Bank2.Stage3.scale      1;
// Type = Bypass
.const $userEq.Fs48.Bank2.Stage4.b0         0.5;
.const $userEq.Fs48.Bank2.Stage4.b1         0;
.const $userEq.Fs48.Bank2.Stage4.b2         0;
.const $userEq.Fs48.Bank2.Stage4.a1         0;
.const $userEq.Fs48.Bank2.Stage4.a2         0;
.const $userEq.Fs48.Bank2.Stage4.scale      1;
// Type = Bypass
.const $userEq.Fs48.Bank2.Stage5.b0         0.5;
.const $userEq.Fs48.Bank2.Stage5.b1         0;
.const $userEq.Fs48.Bank2.Stage5.b2         0;
.const $userEq.Fs48.Bank2.Stage5.a1         0;
.const $userEq.Fs48.Bank2.Stage5.a2         0;
.const $userEq.Fs48.Bank2.Stage5.scale      1;

.const $userEq.Fs48.Bank3.NumBands          0;
// Gain = 0.0 dB
.const $userEq.Fs48.Bank3.GainExp           1;
.const $userEq.Fs48.Bank3.GainMant          0.5;
// Type = Bypass
.const $userEq.Fs48.Bank3.Stage1.b0         0.5;
.const $userEq.Fs48.Bank3.Stage1.b1         0;
.const $userEq.Fs48.Bank3.Stage1.b2         0;
.const $userEq.Fs48.Bank3.Stage1.a1         0;
.const $userEq.Fs48.Bank3.Stage1.a2         0;
.const $userEq.Fs48.Bank3.Stage1.scale      1;
// Type = Bypass
.const $userEq.Fs48.Bank3.Stage2.b0         0.5;
.const $userEq.Fs48.Bank3.Stage2.b1         0;
.const $userEq.Fs48.Bank3.Stage2.b2         0;
.const $userEq.Fs48.Bank3.Stage2.a1         0;
.const $userEq.Fs48.Bank3.Stage2.a2         0;
.const $userEq.Fs48.Bank3.Stage2.scale      1;
// Type = Bypass
.const $userEq.Fs48.Bank3.Stage3.b0         0.5;
.const $userEq.Fs48.Bank3.Stage3.b1         0;
.const $userEq.Fs48.Bank3.Stage3.b2         0;
.const $userEq.Fs48.Bank3.Stage3.a1         0;
.const $userEq.Fs48.Bank3.Stage3.a2         0;
.const $userEq.Fs48.Bank3.Stage3.scale      1;
// Type = Bypass
.const $userEq.Fs48.Bank3.Stage4.b0         0.5;
.const $userEq.Fs48.Bank3.Stage4.b1         0;
.const $userEq.Fs48.Bank3.Stage4.b2         0;
.const $userEq.Fs48.Bank3.Stage4.a1         0;
.const $userEq.Fs48.Bank3.Stage4.a2         0;
.const $userEq.Fs48.Bank3.Stage4.scale      1;
// Type = Bypass
.const $userEq.Fs48.Bank3.Stage5.b0         0.5;
.const $userEq.Fs48.Bank3.Stage5.b1         0;
.const $userEq.Fs48.Bank3.Stage5.b2         0;
.const $userEq.Fs48.Bank3.Stage5.a1         0;
.const $userEq.Fs48.Bank3.Stage5.a2         0;
.const $userEq.Fs48.Bank3.Stage5.scale      1;

.const $userEq.Fs48.Bank4.NumBands          0;
// Gain = 0.0 dB
.const $userEq.Fs48.Bank4.GainExp           1;
.const $userEq.Fs48.Bank4.GainMant          0.5;
// Type = Bypass
.const $userEq.Fs48.Bank4.Stage1.b0         0.5;
.const $userEq.Fs48.Bank4.Stage1.b1         0;
.const $userEq.Fs48.Bank4.Stage1.b2         0;
.const $userEq.Fs48.Bank4.Stage1.a1         0;
.const $userEq.Fs48.Bank4.Stage1.a2         0;
.const $userEq.Fs48.Bank4.Stage1.scale      1;
// Type = Bypass
.const $userEq.Fs48.Bank4.Stage2.b0         0.5;
.const $userEq.Fs48.Bank4.Stage2.b1         0;
.const $userEq.Fs48.Bank4.Stage2.b2         0;
.const $userEq.Fs48.Bank4.Stage2.a1         0;
.const $userEq.Fs48.Bank4.Stage2.a2         0;
.const $userEq.Fs48.Bank4.Stage2.scale      1;
// Type = Bypass
.const $userEq.Fs48.Bank4.Stage3.b0         0.5;
.const $userEq.Fs48.Bank4.Stage3.b1         0;
.const $userEq.Fs48.Bank4.Stage3.b2         0;
.const $userEq.Fs48.Bank4.Stage3.a1         0;
.const $userEq.Fs48.Bank4.Stage3.a2         0;
.const $userEq.Fs48.Bank4.Stage3.scale      1;
// Type = Bypass
.const $userEq.Fs48.Bank4.Stage4.b0         0.5;
.const $userEq.Fs48.Bank4.Stage4.b1         0;
.const $userEq.Fs48.Bank4.Stage4.b2         0;
.const $userEq.Fs48.Bank4.Stage4.a1         0;
.const $userEq.Fs48.Bank4.Stage4.a2         0;
.const $userEq.Fs48.Bank4.Stage4.scale      1;
// Type = Bypass
.const $userEq.Fs48.Bank4.Stage5.b0         0.5;
.const $userEq.Fs48.Bank4.Stage5.b1         0;
.const $userEq.Fs48.Bank4.Stage5.b2         0;
.const $userEq.Fs48.Bank4.Stage5.a1         0;
.const $userEq.Fs48.Bank4.Stage5.a2         0;
.const $userEq.Fs48.Bank4.Stage5.scale      1;

.const $userEq.Fs48.Bank5.NumBands          0;
// Gain = 0.0 dB
.const $userEq.Fs48.Bank5.GainExp           1;
.const $userEq.Fs48.Bank5.GainMant          0.5;
// Type = Bypass
.const $userEq.Fs48.Bank5.Stage1.b0         0.5;
.const $userEq.Fs48.Bank5.Stage1.b1         0;
.const $userEq.Fs48.Bank5.Stage1.b2         0;
.const $userEq.Fs48.Bank5.Stage1.a1         0;
.const $userEq.Fs48.Bank5.Stage1.a2         0;
.const $userEq.Fs48.Bank5.Stage1.scale      1;
// Type = Bypass
.const $userEq.Fs48.Bank5.Stage2.b0         0.5;
.const $userEq.Fs48.Bank5.Stage2.b1         0;
.const $userEq.Fs48.Bank5.Stage2.b2         0;
.const $userEq.Fs48.Bank5.Stage2.a1         0;
.const $userEq.Fs48.Bank5.Stage2.a2         0;
.const $userEq.Fs48.Bank5.Stage2.scale      1;
// Type = Bypass
.const $userEq.Fs48.Bank5.Stage3.b0         0.5;
.const $userEq.Fs48.Bank5.Stage3.b1         0;
.const $userEq.Fs48.Bank5.Stage3.b2         0;
.const $userEq.Fs48.Bank5.Stage3.a1         0;
.const $userEq.Fs48.Bank5.Stage3.a2         0;
.const $userEq.Fs48.Bank5.Stage3.scale      1;
// Type = Bypass
.const $userEq.Fs48.Bank5.Stage4.b0         0.5;
.const $userEq.Fs48.Bank5.Stage4.b1         0;
.const $userEq.Fs48.Bank5.Stage4.b2         0;
.const $userEq.Fs48.Bank5.Stage4.a1         0;
.const $userEq.Fs48.Bank5.Stage4.a2         0;
.const $userEq.Fs48.Bank5.Stage4.scale      1;
// Type = Bypass
.const $userEq.Fs48.Bank5.Stage5.b0         0.5;
.const $userEq.Fs48.Bank5.Stage5.b1         0;
.const $userEq.Fs48.Bank5.Stage5.b2         0;
.const $userEq.Fs48.Bank5.Stage5.a1         0;
.const $userEq.Fs48.Bank5.Stage5.a2         0;
.const $userEq.Fs48.Bank5.Stage5.scale      1;

.const $userEq.Fs48.Bank6.NumBands          0;
// Gain = 0.0 dB
.const $userEq.Fs48.Bank6.GainExp           1;
.const $userEq.Fs48.Bank6.GainMant          0.5;
// Type = Bypass
.const $userEq.Fs48.Bank6.Stage1.b0         0.5;
.const $userEq.Fs48.Bank6.Stage1.b1         0;
.const $userEq.Fs48.Bank6.Stage1.b2         0;
.const $userEq.Fs48.Bank6.Stage1.a1         0;
.const $userEq.Fs48.Bank6.Stage1.a2         0;
.const $userEq.Fs48.Bank6.Stage1.scale      1;
// Type = Bypass
.const $userEq.Fs48.Bank6.Stage2.b0         0.5;
.const $userEq.Fs48.Bank6.Stage2.b1         0;
.const $userEq.Fs48.Bank6.Stage2.b2         0;
.const $userEq.Fs48.Bank6.Stage2.a1         0;
.const $userEq.Fs48.Bank6.Stage2.a2         0;
.const $userEq.Fs48.Bank6.Stage2.scale      1;
// Type = Bypass
.const $userEq.Fs48.Bank6.Stage3.b0         0.5;
.const $userEq.Fs48.Bank6.Stage3.b1         0;
.const $userEq.Fs48.Bank6.Stage3.b2         0;
.const $userEq.Fs48.Bank6.Stage3.a1         0;
.const $userEq.Fs48.Bank6.Stage3.a2         0;
.const $userEq.Fs48.Bank6.Stage3.scale      1;
// Type = Bypass
.const $userEq.Fs48.Bank6.Stage4.b0         0.5;
.const $userEq.Fs48.Bank6.Stage4.b1         0;
.const $userEq.Fs48.Bank6.Stage4.b2         0;
.const $userEq.Fs48.Bank6.Stage4.a1         0;
.const $userEq.Fs48.Bank6.Stage4.a2         0;
.const $userEq.Fs48.Bank6.Stage4.scale      1;
// Type = Bypass
.const $userEq.Fs48.Bank6.Stage5.b0         0.5;
.const $userEq.Fs48.Bank6.Stage5.b1         0;
.const $userEq.Fs48.Bank6.Stage5.b2         0;
.const $userEq.Fs48.Bank6.Stage5.a1         0;
.const $userEq.Fs48.Bank6.Stage5.a2         0;
.const $userEq.Fs48.Bank6.Stage5.scale      1;


.const $BoostEq.Fs48.NumBands               1;
// Gain = 0.0 dB
.const $BoostEq.Fs48.GainExp                1;
.const $BoostEq.Fs48.GainMant               0.5;
// Type = Bypass
.const $BoostEq.Fs48.b0                     0.5;
.const $BoostEq.Fs48.b1                     0;
.const $BoostEq.Fs48.b2                     0;
.const $BoostEq.Fs48.a1                     0;
.const $BoostEq.Fs48.a2                     0;
.const $BoostEq.Fs48.scale                  1;

.const $AncEq.Fs48.NumBands                5;
.const $AncEq.Fs48.GainExp                 1;
.const $AncEq.Fs48.GainMant                0.5;
.const $AncEq.Fs48.Stage1.b0               0.5;
.const $AncEq.Fs48.Stage1.b1               0;
.const $AncEq.Fs48.Stage1.b2               0;
.const $AncEq.Fs48.Stage1.a1               0;
.const $AncEq.Fs48.Stage1.a2               0;
.const $AncEq.Fs48.Stage1.scale            1;
.const $AncEq.Fs48.Stage2.b0               0.5;
.const $AncEq.Fs48.Stage2.b1               0;
.const $AncEq.Fs48.Stage2.b2               0;
.const $AncEq.Fs48.Stage2.a1               0;
.const $AncEq.Fs48.Stage2.a2               0;
.const $AncEq.Fs48.Stage2.scale            1;
.const $AncEq.Fs48.Stage3.b0               0.5;
.const $AncEq.Fs48.Stage3.b1               0;
.const $AncEq.Fs48.Stage3.b2               0;
.const $AncEq.Fs48.Stage3.a1               0;
.const $AncEq.Fs48.Stage3.a2               0;
.const $AncEq.Fs48.Stage3.scale            1;
.const $AncEq.Fs48.Stage4.b0               0.5;
.const $AncEq.Fs48.Stage4.b1               0;
.const $AncEq.Fs48.Stage4.b2               0;
.const $AncEq.Fs48.Stage4.a1               0;
.const $AncEq.Fs48.Stage4.a2               0;
.const $AncEq.Fs48.Stage4.scale            1;
.const $AncEq.Fs48.Stage5.b0               0.5;
.const $AncEq.Fs48.Stage5.b1               0;
.const $AncEq.Fs48.Stage5.b2               0;
.const $AncEq.Fs48.Stage5.a1               0;
.const $AncEq.Fs48.Stage5.a2               0;
.const $AncEq.Fs48.Stage5.scale            1;
.const $spkrEq.Fs48.NumBands                10;
// Gain = 0.0 dB
.const $spkrEq.Fs48.GainExp                 1;
.const $spkrEq.Fs48.GainMant                0.5;
// Type = Bypass
.const $spkrEq.Fs48.Stage1.b0               0.5;
.const $spkrEq.Fs48.Stage1.b1               0;
.const $spkrEq.Fs48.Stage1.b2               0;
.const $spkrEq.Fs48.Stage1.a1               0;
.const $spkrEq.Fs48.Stage1.a2               0;
.const $spkrEq.Fs48.Stage1.scale            1;
// Type = Bypass
.const $spkrEq.Fs48.Stage2.b0               0.5;
.const $spkrEq.Fs48.Stage2.b1               0;
.const $spkrEq.Fs48.Stage2.b2               0;
.const $spkrEq.Fs48.Stage2.a1               0;
.const $spkrEq.Fs48.Stage2.a2               0;
.const $spkrEq.Fs48.Stage2.scale            1;
// Type = Bypass
.const $spkrEq.Fs48.Stage3.b0               0.5;
.const $spkrEq.Fs48.Stage3.b1               0;
.const $spkrEq.Fs48.Stage3.b2               0;
.const $spkrEq.Fs48.Stage3.a1               0;
.const $spkrEq.Fs48.Stage3.a2               0;
.const $spkrEq.Fs48.Stage3.scale            1;
// Type = Bypass
.const $spkrEq.Fs48.Stage4.b0               0.5;
.const $spkrEq.Fs48.Stage4.b1               0;
.const $spkrEq.Fs48.Stage4.b2               0;
.const $spkrEq.Fs48.Stage4.a1               0;
.const $spkrEq.Fs48.Stage4.a2               0;
.const $spkrEq.Fs48.Stage4.scale            1;
// Type = Bypass
.const $spkrEq.Fs48.Stage5.b0               0.5;
.const $spkrEq.Fs48.Stage5.b1               0;
.const $spkrEq.Fs48.Stage5.b2               0;
.const $spkrEq.Fs48.Stage5.a1               0;
.const $spkrEq.Fs48.Stage5.a2               0;
.const $spkrEq.Fs48.Stage5.scale            1;
// Type = Bypass
.const $spkrEq.Fs48.Stage6.b0               0.5;
.const $spkrEq.Fs48.Stage6.b1               0;
.const $spkrEq.Fs48.Stage6.b2               0;
.const $spkrEq.Fs48.Stage6.a1               0;
.const $spkrEq.Fs48.Stage6.a2               0;
.const $spkrEq.Fs48.Stage6.scale            1;
// Type = Bypass
.const $spkrEq.Fs48.Stage7.b0               0.5;
.const $spkrEq.Fs48.Stage7.b1               0;
.const $spkrEq.Fs48.Stage7.b2               0;
.const $spkrEq.Fs48.Stage7.a1               0;
.const $spkrEq.Fs48.Stage7.a2               0;
.const $spkrEq.Fs48.Stage7.scale            1;
// Type = Bypass
.const $spkrEq.Fs48.Stage8.b0               0.5;
.const $spkrEq.Fs48.Stage8.b1               0;
.const $spkrEq.Fs48.Stage8.b2               0;
.const $spkrEq.Fs48.Stage8.a1               0;
.const $spkrEq.Fs48.Stage8.a2               0;
.const $spkrEq.Fs48.Stage8.scale            1;
// Type = Bypass
.const $spkrEq.Fs48.Stage9.b0               0.5;
.const $spkrEq.Fs48.Stage9.b1               0;
.const $spkrEq.Fs48.Stage9.b2               0;
.const $spkrEq.Fs48.Stage9.a1               0;
.const $spkrEq.Fs48.Stage9.a2               0;
.const $spkrEq.Fs48.Stage9.scale            1;
// Type = Bypass
.const $spkrEq.Fs48.Stage10.b0              0.5;
.const $spkrEq.Fs48.Stage10.b1              0;
.const $spkrEq.Fs48.Stage10.b2              0;
.const $spkrEq.Fs48.Stage10.a1              0;
.const $spkrEq.Fs48.Stage10.a2              0;
.const $spkrEq.Fs48.Stage10.scale           1;

// Wired Subwoofer 44.1 kHz coefficients
.const $subEq.Fs44.NumBands                3;
// Gain = 0.0 dB
.const $subEq.Fs44.GainExp                 1;
.const $subEq.Fs44.GainMant                0.5;
// Type = Bypass
.const $subEq.Fs44.Stage1.b0               0.5;
.const $subEq.Fs44.Stage1.b1               0;
.const $subEq.Fs44.Stage1.b2               0;
.const $subEq.Fs44.Stage1.a1               0;
.const $subEq.Fs44.Stage1.a2               0;
.const $subEq.Fs44.Stage1.scale            1;
// Type = Bypass
.const $subEq.Fs44.Stage2.b0               0.5;
.const $subEq.Fs44.Stage2.b1               0;
.const $subEq.Fs44.Stage2.b2               0;
.const $subEq.Fs44.Stage2.a1               0;
.const $subEq.Fs44.Stage2.a2               0;
.const $subEq.Fs44.Stage2.scale            1;
// Type = Bypass
.const $subEq.Fs44.Stage3.b0               0.5;
.const $subEq.Fs44.Stage3.b1               0;
.const $subEq.Fs44.Stage3.b2               0;
.const $subEq.Fs44.Stage3.a1               0;
.const $subEq.Fs44.Stage3.a2               0;
.const $subEq.Fs44.Stage3.scale            1;

// Wired Subwoofer 48 kHz coefficients
.const $subEq.Fs48.NumBands                3;
// Gain = 0.0 dB
.const $subEq.Fs48.GainExp                 1;
.const $subEq.Fs48.GainMant                0.5;
// Type = Bypass
.const $subEq.Fs48.Stage1.b0               0.5;
.const $subEq.Fs48.Stage1.b1               0;
.const $subEq.Fs48.Stage1.b2               0;
.const $subEq.Fs48.Stage1.a1               0;
.const $subEq.Fs48.Stage1.a2               0;
.const $subEq.Fs48.Stage1.scale            1;
// Type = Bypass
.const $subEq.Fs48.Stage2.b0               0.5;
.const $subEq.Fs48.Stage2.b1               0;
.const $subEq.Fs48.Stage2.b2               0;
.const $subEq.Fs48.Stage2.a1               0;
.const $subEq.Fs48.Stage2.a2               0;
.const $subEq.Fs48.Stage2.scale            1;
// Type = Bypass
.const $subEq.Fs48.Stage3.b0               0.5;
.const $subEq.Fs48.Stage3.b1               0;
.const $subEq.Fs48.Stage3.b2               0;
.const $subEq.Fs48.Stage3.a1               0;
.const $subEq.Fs48.Stage3.a2               0;
.const $subEq.Fs48.Stage3.scale            1;
