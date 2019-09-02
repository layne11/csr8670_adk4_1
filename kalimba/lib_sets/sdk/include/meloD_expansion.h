/*******************************************************************************
Copyright (c) 2015 Qualcomm Technologies International, Ltd.
 Part of ADK 4.1
*******************************************************************************/

#ifndef MELOD_EXPANSION_INCLUDED
#define MELOD_EXPANSION_INCLUDED

// first four entries need to be same as for stereo copy as that is used when process is in bypass

.const $MeloD_Expansion.INPUT_LEFT_PTR_BUFFER_FIELD            0;
.const $MeloD_Expansion.INPUT_RIGHT_PTR_BUFFER_FIELD           1;
.const $MeloD_Expansion.OUTPUT_LEFT_PTR_BUFFER_FIELD           2;
.const $MeloD_Expansion.OUTPUT_RIGHT_PTR_BUFFER_FIELD          3;

.const $MeloD_Expansion.BYPASS_WORD_PTR                        4;
.const $MeloD_Expansion.BYPASS_WORD_MASK_FIELD                 5;
.const $MeloD_Expansion.CROSSFADE_GAIN_FIELD                   6;
.const $MeloD_Expansion.FILTER_DATA_PTR_FIELD                  7;
.const $MeloD_Expansion.FILTER_COEF_PTR_FIELD                  8;

.const $MeloD_Expansion.STRUC_SIZE                             9;


.const $MeloD_Expansion.COEF_S_EQ_B1                           0;
.const $MeloD_Expansion.COEF_S_EQ_B0                           1;
.const $MeloD_Expansion.COEF_S_EQ_A1                           2;
.const $MeloD_Expansion.COEF_L_AP1_B2                          3;
.const $MeloD_Expansion.COEF_L_AP1_B1                          4;
.const $MeloD_Expansion.COEF_L_AP1_B0                          5;
.const $MeloD_Expansion.COEF_L_AP2_B2                          6;
.const $MeloD_Expansion.COEF_L_AP2_B1                          7;
.const $MeloD_Expansion.COEF_L_AP2_B0                          8;
.const $MeloD_Expansion.COEF_R_AP1_B2                          9;
.const $MeloD_Expansion.COEF_R_AP1_B1                         10;
.const $MeloD_Expansion.COEF_R_AP1_B0                         11;
.const $MeloD_Expansion.COEF_R_AP2_B2                         12;
.const $MeloD_Expansion.COEF_R_AP2_B1                         13;
.const $MeloD_Expansion.COEF_R_AP2_B0                         14;

.const $MeloD_Expansion.COEFS_STRUC_SIZE                      15;


.const $MeloD_Expansion.FILTER_DATA_SIZE                      18;


#endif	// MELOD_EXPANSION_INCLUDED
