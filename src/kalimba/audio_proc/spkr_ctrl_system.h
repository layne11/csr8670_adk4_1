// Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd.

#ifndef SPKR_CTRL_SYSTEM_HEADER_INCLUDED
#define SPKR_CTRL_SYSTEM_HEADER_INCLUDED

    // structure definition - initialisation data

    .const   $spkr_ctrl_system.INIT_RUNTIME_STRUCT_PTR       0; 
    .const   $spkr_ctrl_system.INIT_PRI_EQ_DEFN_PTR          1; 
    .const   $spkr_ctrl_system.INIT_PRI_EQ_BANK_SELECT_PTR   2; 
    .const   $spkr_ctrl_system.INIT_SEC_EQ_DEFN_PTR          3; 
    .const   $spkr_ctrl_system.INIT_SEC_EQ_BANK_SELECT_PTR   4; 
    .const   $spkr_ctrl_system.INIT_BASS_EQ_DEFN_PTR         5; 
    .const   $spkr_ctrl_system.INIT_BASS_EQ_BANK_SELECT_PTR  6; 

    .const   $spkr_ctrl_system.INIT_STRUCT_SIZE              7; 


    // structure definition - streams and buffers

    .const   $spkr_ctrl_system.LEFT_INPUT_PTR                0;
    .const   $spkr_ctrl_system.RIGHT_INPUT_PTR               1;
    .const   $spkr_ctrl_system.LEFT_PRI_OUTPUT_PTR           2;
    .const   $spkr_ctrl_system.RIGHT_PRI_OUTPUT_PTR          3;
    .const   $spkr_ctrl_system.LEFT_SEC_OUTPUT_PTR           4;
    .const   $spkr_ctrl_system.RIGHT_SEC_OUTPUT_PTR          5;
    .const   $spkr_ctrl_system.LFE_INPUT_PTR                 6;
    .const   $spkr_ctrl_system.SUB_OUTPUT_PTR                7;
    .const   $spkr_ctrl_system.BASS_BUFFER_PTR               8;
    .const   $spkr_ctrl_system.BYPASS_WORD_PTR               9;
    .const   $spkr_ctrl_system.BYPASS_BIT_MASK_FIELD        10;
    .const   $spkr_ctrl_system.COEFS_PTR                    11;

    .const   $spkr_ctrl_system.STRUCT_SIZE                  12; 


    // coefficient pointers
    
    .const   $spkr_ctrl_system.COEF_CONFIG                   0;
    .const   $spkr_ctrl_system.COEF_EQ_L_PRI_PTR             1;
    .const   $spkr_ctrl_system.COEF_EQ_R_PRI_PTR             2;
    .const   $spkr_ctrl_system.COEF_EQ_L_SEC_PTR             3;
    .const   $spkr_ctrl_system.COEF_EQ_R_SEC_PTR             4;
    .const   $spkr_ctrl_system.COEF_EQ_BASS_PTR              5;
    .const   $spkr_ctrl_system.COEF_GAIN_ACTIVE_PTR          6;
    .const   $spkr_ctrl_system.COEF_GAIN_A_PTR               7;
    .const   $spkr_ctrl_system.COEF_GAIN_B_PTR               8;

	.const   $spkr_ctrl_system.COEF_STRUCT_SIZE              9;

    
    // gain coefficients
    
    .const   $spkr_ctrl_system.COEF_GAIN.GAIN_L_L_PRI        0;
    .const   $spkr_ctrl_system.COEF_GAIN.GAIN_R_L_PRI        1;
    .const   $spkr_ctrl_system.COEF_GAIN.GAIN_B_L_PRI        2;
    .const   $spkr_ctrl_system.COEF_GAIN.GAIN_L_R_PRI        3;
    .const   $spkr_ctrl_system.COEF_GAIN.GAIN_R_R_PRI        4;
    .const   $spkr_ctrl_system.COEF_GAIN.GAIN_B_R_PRI        5;
    .const   $spkr_ctrl_system.COEF_GAIN.GAIN_L_L_SEC        6;
    .const   $spkr_ctrl_system.COEF_GAIN.GAIN_R_L_SEC        7;
    .const   $spkr_ctrl_system.COEF_GAIN.GAIN_B_L_SEC        8;
    .const   $spkr_ctrl_system.COEF_GAIN.GAIN_L_R_SEC        9;
    .const   $spkr_ctrl_system.COEF_GAIN.GAIN_R_R_SEC       10;
    .const   $spkr_ctrl_system.COEF_GAIN.GAIN_B_R_SEC       11;
    .const   $spkr_ctrl_system.COEF_GAIN.GAIN_L_BASS        12;
    .const   $spkr_ctrl_system.COEF_GAIN.GAIN_R_BASS        13;
    .const   $spkr_ctrl_system.COEF_GAIN.GAIN_LFE_BASS      14;
    .const   $spkr_ctrl_system.COEF_GAIN.GAIN_SUB           15;
    
	.const   $spkr_ctrl_system.COEF_GAIN.STRUCT_SIZE        16;
    
    
#endif // SPKR_CTRL_SYSTEM_HEADER_INCLUDED