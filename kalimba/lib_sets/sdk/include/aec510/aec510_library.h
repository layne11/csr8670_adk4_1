// *****************************************************************************
// Copyright (c) 2007 - 2015 Qualcomm Technologies International, Ltd.
// %%version
//
// $Change: 2612975 $  $DateTime: 2016/08/04 16:34:47 $
// *****************************************************************************

#ifndef AEC510_LIB_H_INCLUDED
#define AEC510_LIB_H_INCLUDED

// *****************************************************************************
// AEC algorithm matlab version 510
//
// VERSION HISTORY:
//    1.0.0 - initial version aec500
//    2.0.0 - aec510: run-time HS/HF/CE configurable
// *****************************************************************************

// -----------------------------------------------------------------------------
// AEC510 external user constants
// -----------------------------------------------------------------------------
.CONST $aec510.Num_Primary_Taps              2;
.CONST $aec510.Num_Auxillary_Taps            0;

.CONST $aec510_HF.Num_Auxillary_Taps         3;
.CONST $aec510_HF.Num_Primary_Taps           8;

.CONST $aec510.RER_dim                       64;

.CONST $aec510.fbc.nb.FILTER_SIZE            80;
.CONST $aec510.fbc.wb.FILTER_SIZE            160;
.CONST $aec510.fbc.PERD                      1;
.CONST $aec510.fbc.NIBBLE                    0.001 * 2;
.CONST $aec510.fbc.HFP_B_SZIE                6;

// -----------------------------------------------------------------------------
// AEC510 user parameter structure
// -----------------------------------------------------------------------------

// @DATA_OBJECT AECPARAMETEROBJECT

// @DOC_FIELD_TEXT CND gain, default 0x200000 (Q3.21) (CVC parameter)
// @DOC_FIELD_FORMAT Q3.21
.CONST $aec510.Parameter.OFFSET_CNG_Q_ADJUST          0;
// @DOC_FIELD_TEXT, Comfort noise color selection -1=wht,0=brn,1=pnk,2=blu,3=pur (CVC parameter)
// @DOC_FIELD_FORMAT Flag
.CONST $aec510.Parameter.OFFSET_CNG_NOISE_COLOR       1;
// @DOC_FIELD_TEXT DTC aggressiveness, default 0.5 (Q1.23) (CVC parameter)
// @DOC_FIELD_FORMAT Q1.23
.CONST $aec510.Parameter.OFFSET_DTC_AGRESSIVENESS     2;
// @DOC_FIELD_TEXT Maximum Power Margin, default 0x028000 (Q8.16) (CVC parameter for handsfree)
// @DOC_FIELD_FORMAT Q8.16
.CONST $aec510.Parameter.OFFSET_MAX_LPWR_MARGIN       3;
// @DOC_FIELD_TEXT Flag for repeating AEC filtering
// @DOC_FIELD_TEXT Set to '1' for handsfree
// @DOC_FIELD_TEXT Set to '0' for headset (CVC parameter)
// @DOC_FIELD_FORMAT Flag
.CONST $aec510.Parameter.OFFSET_ENABLE_AEC_REUSE      4;
// @DOC_FIELD_TEXT Reference Power Threshold. Default set to '$aec510.AEC_L2Px_HB' (Q8.16)
// @DOC_FIELD_TEXT CVC parameter for handsfree
// @DOC_FIELD_FORMAT Q8.16
.CONST $aec510.Parameter.OFFSET_AEC_REF_LPWR_HB       5;
// @DOC_FIELD_TEXT RER variation threshold, for handsfree only, default '0x040000' (Q8.16) (CVC parameter)
// @DOC_FIELD_FORMAT Q8.16
.CONST $aec510.Parameter.OFFSET_RER_VAR_THRESH        6;
// @DOC_FIELD_TEXT Handsfree only. RER aggresiveness. Default 0x200000 (Q6.18) (Handsfree CVC parameter)
// @DOC_FIELD_FORMAT Q6.18
.CONST $aec510.Parameter.OFFSET_RER_AGGRESSIVENESS    7;
// @DOC_FIELD_TEXT Handsfree only. RER weight. Default 0x200000 (Q3.21) (Handsfree CVC parameter)
// @DOC_FIELD_FORMAT Q3.21
.CONST $aec510.Parameter.OFFSET_RER_WGT_SY            8;
// @DOC_FIELD_TEXT Handsfree only. RER offset. Default 0 (Q8.16) (Handsfree CVC parameter)
// @DOC_FIELD_FORMAT Q8.16
.CONST $aec510.Parameter.OFFSET_RER_OFFSET_SY         9;
// @DOC_FIELD_TEXT Handsfree only. RER power. Default 2 (Q24.0) (Handsfree CVC parameter)
// @DOC_FIELD_FORMAT Integer
.CONST $aec510.Parameter.OFFSET_RER_POWER             10;
// @DOC_FIELD_TEXT Threshold for DTC decision
// @DOC_FIELD_FORMAT Q8.16
.CONST $aec510.Parameter.OFFSET_L2TH_RERDT_OFF        11;
// @DOC_FIELD_TEXT RERDT aggressiveness
// @DOC_FIELD_FORMAT Q6.18
.CONST $aec510.Parameter.OFFSET_RERDT_ADJUST          12;
// @DOC_FIELD_TEXT Handsfree only. RERDT power.
// @DOC_FIELD_FORMAT Integer
.CONST $aec510.Parameter.OFFSET_RERDT_POWER           13;
// @DOC_FIELD_TEXT Speaker and Auto only. Threshold of FBC RER processing.
// @DOC_FIELD_FORMAT Q1.23
.CONST $aec510.Parameter.OFFSET_FBC_TH_RER            14;
// @DOC_FIELD_TEXT Speaker and Auto only. RER adaptation.
// @DOC_FIELD_FORMAT Q1.23
.CONST $aec510.Parameter.OFFSET_RER_ADAPT             15;
// @DOC_FIELD_TEXT Speaker and Auto only. FBC High Pass Pre-filter On/Off
// @DOC_FIELD_FORMAT Integer
.CONST $aec510.Parameter.OFFSET_FBC_HPF_ON            16;
// @DOC_FIELD_TEXT Speaker and Auto only. FBC taillength, in milli-seconds
// @DOC_FIELD_FORMAT Q22.1
.CONST $aec510.Parameter.OFFSET_FBC_FILTER_LENGTH     17;

// @END  DATA_OBJECT AECPARAMETEROBJECT


// -----------------------------------------------------------------------------
// AEC510 data object structure
// -----------------------------------------------------------------------------

// @DATA_OBJECT AECDATAOBJECT

// @DOC_FIELD_TEXT AEC mode object (wideband/narrowband)
// @DOC_FIELD_TEXT Available objects:
// @DOC_FIELD_TEXT    - $aec510.mode.narrow_band
// @DOC_FIELD_TEXT    - $aec510.mode.wide_band
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.MODE_FIELD                    0;

// @DOC_FIELD_TEXT Pointer to AEC Parameters
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.PARAM_FIELD                   1;

// @DOC_FIELD_TEXT RER_Aggr parameter
// @DOC_FIELD_TEXT Default: AUTO -> 0, CE -> 1.0
// @DOC_FIELD_FORMAT Q1.23
.CONST $aec510.RER_AGGR_FIELD                2;

// @DOC_FIELD_TEXT Pointer to G of input send OMS (i.e. pre-AEC OMS)
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.OMS_G_FIELD                   3;

// @DOC_FIELD_TEXT Pointer to AEC reference channel X (real/imag/BExp)
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.X_FIELD                       4;

// @DOC_FIELD_TEXT Pointer to real part of receive buffer X_buf, size of 'Num_FFT_Freq_Bins*Num_Primary_Taps'
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.XBUF_REAL_FIELD               5;

// @DOC_FIELD_TEXT Pointer to imaginary part of receive buffer X_buf, size of 'Num_FFT_Freq_Bins*Num_Primary_Taps'
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.XBUF_IMAG_FIELD               6;

// @DOC_FIELD_TEXT Pointer to BExp_X_buf (internal array, permanant), size of 'Num_Primary_Taps+1'
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.XBUF_BEXP_FIELD               7;

// @DOC_FIELD_TEXT Pointer to left channel FBC object
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.PTR_FBC_OBJ_FIELD             8;

// @DOC_FIELD_TEXT Pointer to AEC (left) channel D (real/imag/BExp)
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.D_FIELD                       9;

// @DOC_FIELD_TEXT Pointer to real part of Ga, size of 'Num_FFT_Freq_Bins*Num_Primary_Taps'
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.GA_REAL_FIELD                 10;

// @DOC_FIELD_TEXT Pointer to imaginary part of Ga, size of 'Num_FFT_Freq_Bins*Num_Primary_Taps'
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.GA_IMAG_FIELD                 11;

// @DOC_FIELD_TEXT Pointer to BExp_Ga (internal array, permanant), size of Num_FFT_Freq_Bins
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.GA_BEXP_FIELD                 12;

// @DOC_FIELD_TEXT Pointer to second channel AEC object
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.DM_OBJ_FIELD                  13;

// @DOC_FIELD_TEXT Pointer to LPwrX0 (internal array, permanant), size of Num_FFT_Freq_Bins
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.LPWRX0_FIELD                  14;

// @DOC_FIELD_TEXT Pointer to LPwrX1 (internal array, permanant), size of Num_FFT_Freq_Bins
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.LPWRX1_FIELD                  15;

// @DOC_FIELD_TEXT Pointer to RatFE (internal array, permanant), size of RER_dim
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.RATFE_FIELD                   16;

// @DOC_FIELD_TEXT Pointer to imaginary part of Gr (RER internal complex array)
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.RER_GR_IMAG_FIELD             17;

// @DOC_FIELD_TEXT Pointer to real part of Gr (RER internal complex array)
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.RER_GR_REAL_FIELD             18;

// @DOC_FIELD_TEXT Pointer to SqGr (RER internal real array)
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.RER_SQGR_FIELD                19;

// @DOC_FIELD_TEXT Pointer to L2absGr (RER internal real array)
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.RER_L2ABSGR_FIELD             20;

// @DOC_FIELD_TEXT Pointer to LPwrD (RER internal real array)
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.RER_LPWRD_FIELD               21;

// @DOC_FIELD_TEXT Pointer to G of output send OMS (i.e. post-AEC OMS)
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.CNG_OMS_G_FIELD               22;

// @DOC_FIELD_TEXT Pointer to LpD_nz of send OMS (i.e. post-AEC OMS)
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.CNG_OMS_LPDNZ_FIELD           23;

// @DOC_FIELD_TEXT Pointer to LpZ_nz (internal array, permanant), size of Num_FFT_Freq_Bins
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.CNG_LPZNZ_FIELD               24;

// @DOC_FIELD_TEXT Pointer to currently selected CNG noise shaping table (internal array, permanant), size of Num_FFT_Freq_Bins
// @DOC_FIELD_FORMAT Pointer Q2.22
.CONST $aec510.CNG_CUR_NZ_TABLE_FIELD        25;

// @DOC_FIELD_TEXT Pointer to L_adaptA (internal array, scratch in DM1), size of Num_FFT_Freq_Bins
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.SCRPTR_LADAPTA_FIELD          26;

// @DOC_FIELD_TEXT Pointer to a scratch memory in DM2 with size of '2*$M.CVC.Num_FFT_Freq_Bins + 1'
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.SCRPTR_EXP_MTS_ADAPT_FIELD    27;

// @DOC_FIELD_TEXT Pointer to Attenuation (internal array, scratch in DM1), size of Num_FFT_Freq_Bins
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.SCRPTR_ATTENUATION_FIELD      28;

// @DOC_FIELD_TEXT Pointer to W_ri (RER internal interleaved complex array)
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.SCRPTR_W_RI_FIELD             29;

// @DOC_FIELD_TEXT Pointer to L_adaptR (RER internal real array)
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.SCRPTR_LADAPTR_FIELD          30;

// @DOC_FIELD_TEXT Pointer to DTC_lin array, size of Num_FFT_Freq_Bins
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.SCRPTR_DTC_LIN_FIELD          31;

// @DOC_FIELD_TEXT Scratch pointer to channel structure T
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.SCRPTR_T_FIELD                32;

// @DOC_FIELD_TEXT DTC status array for each frequency bins, scratch
// @DOC_FIELD_TEXT Size of Number of FFT bins
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.SCRPTR_RERDT_DTC_FIELD        33;

// @DOC_FIELD_TEXT Pointer to real part of Gb, size of 'RER_dim*Num_Auxillary_Taps'
// @DOC_FIELD_TEXT Handsfree only. For headset set to '0'
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.GB_REAL_FIELD                 34;

// @DOC_FIELD_TEXT Pointer to imaginary part of Gb, size of 'RER_dim*Num_Auxillary_Taps'
// @DOC_FIELD_TEXT Handsfree only. For headset set to '0'
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.GB_IMAG_FIELD                 35;

// @DOC_FIELD_TEXT Pointer to BExp_Gb (internal array, permanant), size of RER_dim
// @DOC_FIELD_TEXT Handsfree only. For headset set to '0'
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.GB_BEXP_FIELD                 36;

// @DOC_FIELD_TEXT Pointer to L_RatSqG (internal array, permanant), size of RER_dim
// @DOC_FIELD_TEXT Handsfree only. For headset set to '0'
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.L_RATSQG_FIELD                37;

// @DOC_FIELD_TEXT Handsfree flag
// @DOC_FIELD_TEXT Headset use case if cleared
// @DOC_FIELD_TEXT Handsfree use case if set
// @DOC_FIELD_FORMAT Flag
.CONST $aec510.HF_FLAG_FIELD                 38;

// Internal AEC Data - Variables
// PREP
.CONST $aec510.G_OMS_IN2_FIELD               1 + $aec510.HF_FLAG_FIELD;
.CONST $aec510.L2PXT_FIELD                   1 + $aec510.G_OMS_IN2_FIELD;
.CONST $aec510.L2PDT_FIELD                   1 + $aec510.L2PXT_FIELD;

// DTC
.CONST $aec510.L_MUA_FIELD                   1 + $aec510.L2PDT_FIELD;
.CONST $aec510.L_MUB_FIELD                   1 + $aec510.L_MUA_FIELD;
.CONST $aec510.L_DTC_HFREQ_FEF_FIELD         1 + $aec510.L_MUB_FIELD;
.CONST $aec510.DTC_AVG_FIELD                 1 + $aec510.L_DTC_HFREQ_FEF_FIELD;
.CONST $aec510.DTC_PROB_FIELD                1 + $aec510.DTC_AVG_FIELD;
.CONST $aec510.DTC_AVGRFE_FIELD              1 + $aec510.DTC_PROB_FIELD;
.CONST $aec510.DTC_STDRFE_FIELD              1 + $aec510.DTC_AVGRFE_FIELD;
.CONST $aec510.mn_L_RatSqGt                  1 + $aec510.DTC_STDRFE_FIELD;

.CONST $aec510.OFFSET_L_RatSqG               1 + $aec510.mn_L_RatSqGt;
.CONST $aec510.OFFSET_dL2PxFB				      $aec510.OFFSET_L_RatSqG+1;
.CONST $aec510.OFFSET_L2Pxt0					  $aec510.OFFSET_dL2PxFB+1;
.CONST $aec510.OFFSET_DTC_dLpX				  $aec510.OFFSET_L2Pxt0+1;
.CONST $aec510.OFFSET_DTC_LpXt_prev			  $aec510.OFFSET_DTC_dLpX+1;


.CONST $aec510.OFFSET_tt_dtc					  $aec510.OFFSET_DTC_LpXt_prev+1;
.CONST $aec510.OFFSET_ct_init			        $aec510.OFFSET_tt_dtc+1;
.CONST $aec510.OFFSET_ct_Px				     $aec510.OFFSET_ct_init+1;
.CONST $aec510.OFFSET_tt_cng				     $aec510.OFFSET_ct_Px+1;
.CONST $aec510.OFFSET_L_DTC                  $aec510.OFFSET_tt_cng+1;

// RERDT
.CONST $aec510.OFFSET_LPXFB_RERDT           $aec510.OFFSET_L_DTC+1;
.CONST $aec510.RERDT_DTC_ACTIVE_FIELD       $aec510.OFFSET_LPXFB_RERDT+1;

// CNG
.CONST $aec510.OFFSET_CNG_offset				  $aec510.RERDT_DTC_ACTIVE_FIELD+1;

// HD
.CONST $aec510.OFFSET_AEC_COUPLING          $aec510.OFFSET_CNG_offset+1;
.CONST $aec510.OFFSET_HD_L_AECgain          $aec510.OFFSET_AEC_COUPLING+1;

// scratch variables
.CONST $aec510.LPWRX_MARGIN_FIELD            $aec510.OFFSET_HD_L_AECgain +1;
.CONST $aec510.MN_PWRX_DIFF_FIELD            $aec510.LPWRX_MARGIN_FIELD +1;

.CONST $aec510.OFFSET_OMS_AGGRESSIVENESS    $aec510.MN_PWRX_DIFF_FIELD +1;
.CONST $aec510.OFFSET_TEMP_FIELD         $aec510.OFFSET_OMS_AGGRESSIVENESS +1; 

//RER variables
.CONST $aec510.RER_BEXP_FIELD                $aec510.OFFSET_TEMP_FIELD +1;
.CONST $aec510.RER_E_FIELD                   $aec510.RER_BEXP_FIELD +1;
.CONST $aec510.RER_L2PET_FIELD               $aec510.RER_E_FIELD +1;
.CONST $aec510.OFFSET_PXRS_FIELD             $aec510.RER_L2PET_FIELD +1;
.CONST $aec510.OFFSET_PXRD_FIELD             $aec510.OFFSET_PXRS_FIELD +1; 
.CONST $aec510.OFFSET_PDRS_FIELD             $aec510.OFFSET_PXRD_FIELD +1;
.CONST $aec510.OFFSET_PDRD_FIELD             $aec510.OFFSET_PDRS_FIELD +1; 

// NB/WB
.CONST $aec510.OFFSET_NUM_FREQ_BINS         $aec510.OFFSET_PDRD_FIELD +1;
.CONST $aec510.OFFSET_LPWRX_MARGIN_OVFL     $aec510.OFFSET_NUM_FREQ_BINS+1;
.CONST $aec510.OFFSET_LPWRX_MARGIN_SCL      $aec510.OFFSET_LPWRX_MARGIN_OVFL+1;

// HS/HF
.CONST $aec510.OFFSET_NUM_PRIMARY_TAPS      $aec510.OFFSET_LPWRX_MARGIN_SCL+1;
.CONST $aec510.OFFSET_NUM_AUXILLARY_TAPS    $aec510.OFFSET_NUM_PRIMARY_TAPS+1;
.CONST $aec510.OFFSET_AEC_L_MUA_ON		     $aec510.OFFSET_NUM_AUXILLARY_TAPS+1;
.CONST $aec510.OFFSET_AEC_L_MUB_ON          $aec510.OFFSET_AEC_L_MUA_ON+1;
.CONST $aec510.OFFSET_AEC_ALFA_A            $aec510.OFFSET_AEC_L_MUB_ON+1;
.CONST $aec510.OFFSET_AEC_L_ALFA_A          $aec510.OFFSET_AEC_ALFA_A+1;
.CONST $aec510.OFFSET_DTC_scale_dLpX        $aec510.OFFSET_AEC_L_ALFA_A+1;

.CONST $aec510.FLAG_BYPASS_CNG_FIELD         1 + $aec510.OFFSET_DTC_scale_dLpX;
.CONST $aec510.FLAG_BYPASS_RER_FIELD         1 + $aec510.FLAG_BYPASS_CNG_FIELD;
.CONST $aec510.FLAG_BYPASS_RERDT_FIELD       1 + $aec510.FLAG_BYPASS_RER_FIELD;  
.CONST $aec510.FLAG_BYPASS_RERDEV_FIELD      1 + $aec510.FLAG_BYPASS_RERDT_FIELD; 
.CONST $aec510.FLAG_BYPASS_RERCBA_FIELD      1 + $aec510.FLAG_BYPASS_RERDEV_FIELD; 
.CONST $aec510.FLAG_BYPASS_FBC_FIELD         1 + $aec510.FLAG_BYPASS_RERCBA_FIELD; 

.CONST $aec510.STRUCT_SIZE                   1 + $aec510.FLAG_BYPASS_FBC_FIELD;

// @END  DATA_OBJECT AECDATAOBJECT


// -----------------------------------------------------------------------------
// AEC510 dual microphone (second channel) data object structure
// -----------------------------------------------------------------------------

// @DATA_OBJECT AECDM_DATAOBJECT

// @DOC_FIELD_TEXT Pointer to external microphone mode
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.dm.PTR_MIC_MODE_FIELD         0;

// @DOC_FIELD_TEXT Pointer to rightt channel FBC object
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.dm.PTR_FBC1_OBJ_FIELD         1;

// @DOC_FIELD_TEXT Pointer to AEC (right) channel (D1)(real/imag/BExp)
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.dm.D1_FIELD                   2;

// @DOC_FIELD_TEXT Pointer to real part of Ga1, size of 'Num_FFT_Freq_Bins*Num_Primary_Taps'
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.dm.GA1_REAL_FIELD             3;

// @DOC_FIELD_TEXT Pointer to imaginary part of Ga1, size of 'Num_FFT_Freq_Bins*Num_Primary_Taps'
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.dm.GA1_IMAG_FIELD             4;

// @DOC_FIELD_TEXT Pointer to BExp_Ga1 (internal array, permanant), size of Num_FFT_Freq_Bins
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.dm.GA1_BEXP_FIELD             5;

.CONST $aec510.dm.STRUCT_SIZE                6;
// @END  DATA_OBJECT AECDM_DATAOBJECT

// -----------------------------------------------------------------------------
// AEC510 FDNLP/VSM sub-module object structure
// -----------------------------------------------------------------------------
// FDNLP - (Handsfree)
.CONST $M.FDNLP_500.OFFSET_VSM_HB              0;
.CONST $M.FDNLP_500.OFFSET_VSM_LB              $M.FDNLP_500.OFFSET_VSM_HB+1;
.CONST $M.FDNLP_500.OFFSET_VSM_MAX_ATT         $M.FDNLP_500.OFFSET_VSM_LB+1;
.CONST $M.FDNLP_500.OFFSET_FDNLP_HB            $M.FDNLP_500.OFFSET_VSM_MAX_ATT+1;
.CONST $M.FDNLP_500.OFFSET_FDNLP_LB            $M.FDNLP_500.OFFSET_FDNLP_HB+1;
.CONST $M.FDNLP_500.OFFSET_FDNLP_MB            $M.FDNLP_500.OFFSET_FDNLP_LB+1;
.CONST $M.FDNLP_500.OFFSET_FDNLP_NBINS         $M.FDNLP_500.OFFSET_FDNLP_MB+1;
.CONST $M.FDNLP_500.OFFSET_FDNLP_ATT           $M.FDNLP_500.OFFSET_FDNLP_NBINS+1;
.CONST $M.FDNLP_500.OFFSET_FDNLP_ATT_THRESH    $M.FDNLP_500.OFFSET_FDNLP_ATT+1;
.CONST $M.FDNLP_500.OFFSET_FDNLP_ECHO_THRESH   $M.FDNLP_500.OFFSET_FDNLP_ATT_THRESH+1;
.CONST $M.FDNLP_500.STRUCT_SIZE                $M.FDNLP_500.OFFSET_FDNLP_ECHO_THRESH+1;


// -----------------------------------------------------------------------------
// AEC510 NLP user parameter structure
// -----------------------------------------------------------------------------

.CONST $aec510.nlp.Parameter.OFFSET_HD_THRESH_GAIN           0;
.CONST $aec510.nlp.Parameter.OFFSET_TIER2_THRESH             1;
.CONST $aec510.nlp.Parameter.OFFSET_TIER1_CONFIG             $aec510.nlp.Parameter.OFFSET_TIER2_THRESH + 1;
.CONST $aec510.nlp.Parameter.OFFSET_TIER2_CONFIG             $aec510.nlp.Parameter.OFFSET_TIER1_CONFIG + $M.FDNLP_500.STRUCT_SIZE;

.CONST $aec510.nlp.Parameter.HF_OBJECT_SIZE                  $aec510.nlp.Parameter.OFFSET_TIER2_CONFIG + $M.FDNLP_500.STRUCT_SIZE;
.CONST $aec510.nlp.Parameter.HS_OBJECT_SIZE                  2;


// -----------------------------------------------------------------------------
// AEC510 NLP object structure
// -----------------------------------------------------------------------------

// @DATA_OBJECT NLPDATAOBJECT

// @DOC_FIELD_TEXT Pointer to AEC master object
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.nlp.AEC_OBJ_PTR                  0;

// @DOC_FIELD_TEXT Pointer to Non-Linear Processing Parameters
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.nlp.OFFSET_PARAM_PTR             1;

// FDNLP - VSM
// @DOC_FIELD_TEXT Pointer to current system call state flag
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.nlp.OFFSET_CALLSTATE_PTR         2;

// @DOC_FIELD_TEXT Pointer to receive path signal VAD flag
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.nlp.OFFSET_PTR_RCV_DETECT        3;

// @DOC_FIELD_TEXT Pointer to Attenuation, same array as used in AEC main object
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.nlp.OFFSET_SCRPTR_Attenuation    4;

// @DOC_FIELD_TEXT Pointer to scratch memory with size of Num_FFT_Freq_Bins + RER_dim
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.nlp.OFFSET_SCRPTR                5;

// @DOC_FIELD_TEXT Function pointer for FDNLP
// @DOC_FIELD_TEXT To enable: set '$aec510.FdnlpProcess'
// @DOC_FIELD_TEXT To disable: set '0'
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.nlp.FDNLP_FUNCPTR                6;

// @DOC_FIELD_TEXT Function pointer for VSM
// @DOC_FIELD_TEXT To enable: set '$aec510.VsmProcess'
// @DOC_FIELD_TEXT To disable: set '0'
// @DOC_FIELD_FORMAT Pointer
.CONST $aec510.nlp.VSM_FUNCPTR                  7;

// SP.  Internal FNDLP Data
.CONST $aec510.nlp.OFFSET_PTR_RatFE             $aec510.nlp.VSM_FUNCPTR+1;
.CONST $aec510.nlp.OFFSET_PTR_SqGr              $aec510.nlp.OFFSET_PTR_RatFE+1;
.CONST $aec510.nlp.OFFSET_PTR_L2ABSGR           $aec510.nlp.OFFSET_PTR_SqGr+1;
.CONST $aec510.nlp.OFFSET_SCRPTR_absGr          $aec510.nlp.OFFSET_PTR_L2ABSGR+1;
.CONST $aec510.nlp.OFFSET_SCRPTR_temp           $aec510.nlp.OFFSET_SCRPTR_absGr+1;
.CONST $aec510.nlp.OFFSET_PTR_CUR_CONFIG        $aec510.nlp.OFFSET_SCRPTR_temp+1;
.CONST $aec510.nlp.OFFSET_hd_ct_hold            $aec510.nlp.OFFSET_PTR_CUR_CONFIG+$M.FDNLP_500.STRUCT_SIZE;
.CONST $aec510.nlp.OFFSET_hd_att                $aec510.nlp.OFFSET_hd_ct_hold+1;
.CONST $aec510.nlp.OFFSET_G_vsm                 $aec510.nlp.OFFSET_hd_att+1;
.CONST $aec510.nlp.OFFSET_fdnlp_cont_test       $aec510.nlp.OFFSET_G_vsm+1;
.CONST $aec510.nlp.OFFSET_mean_len              $aec510.nlp.OFFSET_fdnlp_cont_test+1;
.CONST $aec510.nlp.OFFSET_Vad_ct_burst          $aec510.nlp.OFFSET_mean_len+1;
.CONST $aec510.nlp.OFFSET_Vad_ct_hang           $aec510.nlp.OFFSET_Vad_ct_burst+1; // must follow ct_burst

.CONST $aec510.nlp.FLAG_BYPASS_HD_FIELD         $aec510.nlp.OFFSET_Vad_ct_hang+1;
.CONST $aec510.nlp.OFFSET_HC_TIER_STATE         $aec510.nlp.FLAG_BYPASS_HD_FIELD+1;
.CONST $aec510.nlp.OFFSET_NUM_FREQ_BINS         $aec510.nlp.OFFSET_HC_TIER_STATE+1;
.CONST $aec510.nlp.OFFSET_D_REAL_PTR            $aec510.nlp.OFFSET_NUM_FREQ_BINS+1;
.CONST $aec510.nlp.OFFSET_D_IMAG_PTR            $aec510.nlp.OFFSET_D_REAL_PTR+1;

.CONST $aec510.nlp.STRUCT_SIZE                  $aec510.nlp.OFFSET_D_IMAG_PTR+1;

// @END  DATA_OBJECT NLPDATAOBJECT


// -----------------------------------------------------------------------------
// FBC data object structure
// -----------------------------------------------------------------------------

// @DATA_OBJECT AECFBC_DATAOBJECT

.CONST $aec510.fbc.STREAM_D_FIELD               0;
.CONST $aec510.fbc.STREAM_X_FIELD               1;
.CONST $aec510.fbc.PTR_VADX_FIELD               2;
.CONST $aec510.fbc.G_A_FIELD                    3;
.CONST $aec510.fbc.G_B_FIELD                    4;
.CONST $aec510.fbc.PERD_FIELD                   5;
.CONST $aec510.fbc.NIBBLE_FIELD                 6;
.CONST $aec510.fbc.HPF_STREAM_FIELD             7;

// Internal fields
.CONST $aec510.fbc.FILTER_LENGTH_FIELD          8;
.CONST $aec510.fbc.FRAME_SIZE_FIELD             1 + $aec510.fbc.FILTER_LENGTH_FIELD;
.CONST $aec510.fbc.G_A_AMP_FIELD                1 + $aec510.fbc.FRAME_SIZE_FIELD;
.CONST $aec510.fbc.G_B_AMP_FIELD                1 + $aec510.fbc.G_A_AMP_FIELD;
.CONST $aec510.fbc.X_BUF_PWR_GW_FIELD           1 + $aec510.fbc.G_B_AMP_FIELD;
.CONST $aec510.fbc.X_BUF_PWR_MSW_FIELD          1 + $aec510.fbc.X_BUF_PWR_GW_FIELD;
.CONST $aec510.fbc.X_BUF_PWR_LSW_FIELD          1 + $aec510.fbc.X_BUF_PWR_MSW_FIELD;
.CONST $aec510.fbc.MU_MANTISA_FIELD             1 + $aec510.fbc.X_BUF_PWR_LSW_FIELD;
.CONST $aec510.fbc.MU_EXP_FIELD                 1 + $aec510.fbc.MU_MANTISA_FIELD;
.CONST $aec510.fbc.MU2_MANTISA_FIELD            1 + $aec510.fbc.MU_EXP_FIELD;
.CONST $aec510.fbc.MU2_EXP_FIELD                1 + $aec510.fbc.MU2_MANTISA_FIELD;
.CONST $aec510.fbc.DERLE_AMP_FIELD              1 + $aec510.fbc.MU2_EXP_FIELD;
.CONST $aec510.fbc.DERLE_FIL_FIELD              1 + $aec510.fbc.DERLE_AMP_FIELD;
.CONST $aec510.fbc.ERLE_FOLD_FIELD              1 + $aec510.fbc.DERLE_FIL_FIELD;
.CONST $aec510.fbc.HD_FOLD_FIELD                1 + $aec510.fbc.ERLE_FOLD_FIELD;
.CONST $aec510.fbc.HD_FLAG_FIELD                1 + $aec510.fbc.HD_FOLD_FIELD;
.CONST $aec510.fbc.HD_CNTR_FIELD                1 + $aec510.fbc.HD_FLAG_FIELD;
.CONST $aec510.fbc.TH_CNTR_FIELD                1 + $aec510.fbc.HD_CNTR_FIELD;
.CONST $aec510.fbc.SIL_CNTR_FIELD               1 + $aec510.fbc.TH_CNTR_FIELD;
.CONST $aec510.fbc.DIVERGE_FLAG_FIELD           1 + $aec510.fbc.SIL_CNTR_FIELD;
.CONST $aec510.fbc.LRAT_0_FIELD                 1 + $aec510.fbc.DIVERGE_FLAG_FIELD;
.CONST $aec510.fbc.LRAT_1_FIELD                 1 + $aec510.fbc.LRAT_0_FIELD;
.CONST $aec510.fbc.LRAT_2_FIELD                 1 + $aec510.fbc.LRAT_1_FIELD;
.CONST $aec510.fbc.L2P_IBUF_D_FIELD             1 + $aec510.fbc.LRAT_2_FIELD;
.CONST $aec510.fbc.L2P_OBUF_D_FIELD             1 + $aec510.fbc.L2P_IBUF_D_FIELD;
.CONST $aec510.fbc.L2P_OBUF_D_1_FIELD           1 + $aec510.fbc.L2P_OBUF_D_FIELD;
.CONST $aec510.fbc.L2P_OBUF_D_2_FIELD           1 + $aec510.fbc.L2P_OBUF_D_1_FIELD;
.CONST $aec510.fbc.L2_HD_GAIN_FIELD             1 + $aec510.fbc.L2P_OBUF_D_2_FIELD;
.CONST $aec510.fbc.HD_GAIN_FIELD                1 + $aec510.fbc.L2_HD_GAIN_FIELD;
.CONST $aec510.fbc.IBUF_D_PRE_PWR_FIELD         1 + $aec510.fbc.HD_GAIN_FIELD;
.CONST $aec510.fbc.OBUF_D_PRE_PWR_FIELD         1 + $aec510.fbc.IBUF_D_PRE_PWR_FIELD;
.CONST $aec510.fbc.L2P_PREP_FBC_FIELD           1 + $aec510.fbc.OBUF_D_PRE_PWR_FIELD;
.CONST $aec510.fbc.L2P_PWR_DIFFERENCE_FIELD     1 + $aec510.fbc.L2P_PREP_FBC_FIELD;
.CONST $aec510.fbc.STREAM_D_DLY_B_FIELD         1 + $aec510.fbc.L2P_PWR_DIFFERENCE_FIELD;
.CONST $aec510.fbc.STREAM_X_DLY_B_FIELD         1 + $aec510.fbc.STREAM_D_DLY_B_FIELD;
.CONST $aec510.fbc.STREAM_D_HI_FIELD            1 + $aec510.fbc.STREAM_X_DLY_B_FIELD;
.CONST $aec510.fbc.STREAM_X_HI_FIELD            1 + $aec510.fbc.STREAM_D_HI_FIELD;
.CONST $aec510.fbc.FLAG_BYPASS_HPF_FIELD        1 + $aec510.fbc.STREAM_X_HI_FIELD;
.CONST $aec510.fbc.TEMP0_FIELD                  1 + $aec510.fbc.FLAG_BYPASS_HPF_FIELD;
.CONST $aec510.fbc.TEMP1_FIELD                  1 + $aec510.fbc.TEMP0_FIELD;

.CONST $aec510.fbc.STRUCT_SIZE                  1 + $aec510.fbc.TEMP1_FIELD;

// @END  DATA_OBJECT AECFBC_DATAOBJECT

// -----------------------------------------------------------------------------
// AEC filter_bank data object structure
// -----------------------------------------------------------------------------
.CONST $M.filter_bank.Parameters.OFFSET_DELAY_STREAM_PTR    6;
.CONST $M.filter_bank.Parameters.OFFSET_DELAY_PARAM_PTR     7;
.CONST $aec510.filter_bank.Parameters.ONE_CHNL_BLOCK_SIZE   8;

#endif // AEC510_LIB_H_INCLUDED
