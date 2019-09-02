// *****************************************************************************
// Copyright (c) 2015 - 2016 Qualcomm Technologies International, Ltd. 
// Part of ADK 4.1
//
// $Change: 2612975 $  $DateTime: 2016/08/04 16:34:47 $
// *****************************************************************************

#ifndef AUDIO_VSE_HEADER_INCLUDED
#define AUDIO_VSE_HEADER_INCLUDED

#ifdef KYMERA
#include "cbuffer_asm.h"
#else
#include "core_library.h"
#endif


   .CONST $VSE_FRAME_SIZE                             60;
   .CONST $VSE_SAMPLE_RATE_32000                      32000;
   .CONST $VSE_SAMPLE_RATE_44100                      44100;
   .CONST $VSE_SAMPLE_RATE_48000                      48000;
   .CONST $VSE_SAMPLE_RATE_96000                      96000;
   .CONST $VSE_SPEAKER_SPACING_5CM                    0.05;
   .CONST $VSE_SPEAKER_SPACING_10CM                   0.10;
   .CONST $VSE_SPEAKER_SPACING_15CM                   0.15;
   .CONST $VSE_SPEAKER_SPACING_20CM                   0.20;
   .CONST $BIN_SYNTH_FILTER_COEFF_SIZE                3;
   .CONST $ITF_COEFF_FILTER_SIZE                      2;
   .CONST $LSF_PEAK_COEFF_FILTER_SIZE                 5;
   .CONST $SAMPLE_RATES_SUPPORTED_COUNT               5;
   .CONST $LIMIT_ATTACK_TC_44K                        0.999437428993052;
   .CONST $LIMIT_ATTACK_TC_NON_44K                    0.999446915629852;
   .CONST $LIMIT_DECAY_TC_44K                         0.007455065308502;
   .CONST $LIMIT_DECAY_TC_NON_44K                     0.007471945180862;
   .CONST $LINEAR_ATTACK_TC_44K                       0.776109601539266;
   .CONST $LINEAR_ATTACK_TC_NON_44K                   0.776869839851570;
   .CONST $LINEAR_DECAY_TC_44K                        0.014854552618250;
   .CONST $LINEAR_DECAY_TC_NON_44K                    0.014888060396937;
   .CONST $LIMIT_THRESHOLD                            0xFF8071;                // -0.498289214233104 in Q.16
   .CONST $MAKEUPGAIN                                 1.0;

   // 3dv parameter structure
   .CONST $audio_proc.vse.parameter.VSE_CONFIG        0;
   .CONST $audio_proc.vse.parameter.BINAURAL_FLAG     1 + $audio_proc.vse.parameter.VSE_CONFIG;
   .CONST $audio_proc.vse.parameter.SPEAKER_SPACING   1 + $audio_proc.vse.parameter.BINAURAL_FLAG;
   .CONST $audio_proc.vse.parameter.VIRTUAL_ANGLE     1 + $audio_proc.vse.parameter.SPEAKER_SPACING;
   .CONST $audio_proc.vse.parameter.STRUCT_SIZE       1 + $audio_proc.vse.parameter.VIRTUAL_ANGLE;


   // VSE structure
   // -------------------------------- START PUBLIC SECTION ---------------------------------
   .CONST $audio_proc.vse.INPUT_ADDR_FIELD            0;
   .CONST $audio_proc.vse.OUTPUT_ADDR_FIELD           1 + $audio_proc.vse.INPUT_ADDR_FIELD;
   .CONST $audio_proc.vse.PARAM_PTR_FIELD             1 + $audio_proc.vse.OUTPUT_ADDR_FIELD;
   .CONST $audio_proc.vse.FS                          1 + $audio_proc.vse.PARAM_PTR_FIELD;
   // -------------------------------- START INTERNAL SECTION ---------------------------------
   .CONST $audio_proc.vse.FRAMEBUFFER_FLAG            1 + $audio_proc.vse.FS;
   .CONST $audio_proc.vse.INPUT_READ_ADDR             1 + $audio_proc.vse.FRAMEBUFFER_FLAG;
   .CONST $audio_proc.vse.OUTPUT_WRITE_ADDR           1 + $audio_proc.vse.INPUT_READ_ADDR;
   .CONST $audio_proc.vse.SAMPLES_TO_PROCESS          1 + $audio_proc.vse.OUTPUT_WRITE_ADDR;
   .CONST $audio_proc.vse.VSE_CUR_BLOCK_SIZE          1 + $audio_proc.vse.SAMPLES_TO_PROCESS;
   .CONST $audio_proc.vse.IPSI_COEFF_PTR_FIELD        1 + $audio_proc.vse.VSE_CUR_BLOCK_SIZE;
   .CONST $audio_proc.vse.CONTRA_COEFF_PTR_FIELD      1 + $audio_proc.vse.IPSI_COEFF_PTR_FIELD;
   .CONST $audio_proc.vse.XTC_COEFF_PTR_FIELD         1 + $audio_proc.vse.CONTRA_COEFF_PTR_FIELD;
   .CONST $audio_proc.vse.DELAY_FIELD                 1 + $audio_proc.vse.XTC_COEFF_PTR_FIELD;
   .CONST $audio_proc.vse.DCB_COEFF_PTR_FIELD         1 + $audio_proc.vse.DELAY_FIELD;
   .CONST $audio_proc.vse.ITF_COEFF_PTR_FIELD         1 + $audio_proc.vse.DCB_COEFF_PTR_FIELD;
   .CONST $audio_proc.vse.LSF_COEFF_PTR_FIELD         1 + $audio_proc.vse.ITF_COEFF_PTR_FIELD;
   .CONST $audio_proc.vse.PEAK_COEFF_PTR_FIELD        1 + $audio_proc.vse.LSF_COEFF_PTR_FIELD;
   .CONST $audio_proc.vse.PTR_HISTORY_BUF_Hi          1 + $audio_proc.vse.PEAK_COEFF_PTR_FIELD;
   .CONST $audio_proc.vse.PTR_HISTORY_BUF_Hc          1 + $audio_proc.vse.PTR_HISTORY_BUF_Hi;
   .CONST $audio_proc.vse.PTR_HISTORY_BUF_PEAK        1 + $audio_proc.vse.PTR_HISTORY_BUF_Hc;
   .CONST $audio_proc.vse.PTR_HISTORY_BUF_LSF         1 + $audio_proc.vse.PTR_HISTORY_BUF_PEAK;

   // *****************************************************************************
   //                                         LIMITER SECTION
   // *****************************************************************************
   .CONST $audio_proc.vse.LIMIT_ATTACK_TC        1 + $audio_proc.vse.PTR_HISTORY_BUF_LSF;
   .CONST $audio_proc.vse.LIMIT_DECAY_TC         1 + $audio_proc.vse.LIMIT_ATTACK_TC;
   .CONST $audio_proc.vse.LINEAR_ATTACK_TC       1 + $audio_proc.vse.LIMIT_DECAY_TC;
   .CONST $audio_proc.vse.LINEAR_DECAY_TC        1 + $audio_proc.vse.LINEAR_ATTACK_TC;
   .CONST $audio_proc.vse.LIMIT_THRESHOLD        1 + $audio_proc.vse.LINEAR_DECAY_TC;
   .CONST $audio_proc.vse.MAKEUPGAIN             1 + $audio_proc.vse.LIMIT_THRESHOLD;
   .CONST $audio_proc.vse.LIMITER_GAIN           1 + $audio_proc.vse.MAKEUPGAIN;
   .CONST $audio_proc.vse.LIMITER_GAIN_LOG       1 + $audio_proc.vse.LIMITER_GAIN;

   // *****************************************************************************
   //                                   Hi' FILTER
   // *****************************************************************************
   // history buffer for Hi'
   .CONST $audio_proc.vse.START_HISTORY_BUF_Hi       1 + $audio_proc.vse.LIMITER_GAIN_LOG;
   .CONST $audio_proc.vse.END_HISTORY_BUF_Hi         3 + $audio_proc.vse.START_HISTORY_BUF_Hi; /// y_l'  , y_h', x_in'

   // *****************************************************************************
   //                                      Hc' FILTER
   // *****************************************************************************

   // history buffer for Hc'
   .CONST $audio_proc.vse.START_HISTORY_BUF_Hc       0 + $audio_proc.vse.END_HISTORY_BUF_Hi;
   .CONST $audio_proc.vse.Hc_HIST_BUF_SIZE          66 + $audio_proc.vse.START_HISTORY_BUF_Hc;
   .CONST $audio_proc.vse.DELAYLINE_SIZE_FIELD       1 + $audio_proc.vse.Hc_HIST_BUF_SIZE;
   .CONST $audio_proc.vse.HcOUT_H_1                  0 + $audio_proc.vse.DELAYLINE_SIZE_FIELD;
   .CONST $audio_proc.vse.HcOUT_L_1                  1 + $audio_proc.vse.HcOUT_H_1;

   // *****************************************************************************
   //                                 DCB
   // *****************************************************************************

   // history buffer for DCB
   .CONST $audio_proc.vse.START_HISTORY_BUF_DCB       1 + $audio_proc.vse.HcOUT_L_1;
   .CONST $audio_proc.vse.END_HISTORY_BUF_DCB         3 + $audio_proc.vse.START_HISTORY_BUF_DCB; /// y_l'  , y_h', x_in'

   // *****************************************************************************
   //                                 EQ
   // *****************************************************************************

   // history buffer for EQ
   .CONST $audio_proc.vse.START_HISTORY_BUF_EQ        0 + $audio_proc.vse.END_HISTORY_BUF_DCB;
   .CONST $audio_proc.vse.END_HISTORY_BUF_EQ          3 + $audio_proc.vse.START_HISTORY_BUF_EQ; /// y_l'  , y_h', x_in'

   // *****************************************************************************
   //                                 ITF
   // *****************************************************************************

   // history buffer for DCB
   .CONST $audio_proc.vse.HiftOUT_L_1                 1 + $audio_proc.vse.END_HISTORY_BUF_EQ;
   .CONST $audio_proc.vse.HiftOUT_H_1                 1 + $audio_proc.vse.HiftOUT_L_1;

   // *****************************************************************************
   //                                     PEAK FILTER
   // *****************************************************************************

   // history buffer for PEAK FILTER
   .CONST $audio_proc.vse.START_HISTORY_BUF_PEAK       1 + $audio_proc.vse.HiftOUT_H_1;
   .CONST $audio_proc.vse.PEAK_HIST_BUF_SIZE           9 + $audio_proc.vse.START_HISTORY_BUF_PEAK;//// y_l,y_l',y_l'',x ,x',x'',y_h,y_h',y_h''

   // *****************************************************************************
   //                                     LSF FILTER
   // *****************************************************************************

   // history buffer for LSF FILTER
   .CONST $audio_proc.vse.START_HISTORY_BUF_LSF        1 + $audio_proc.vse.PEAK_HIST_BUF_SIZE;
   .CONST $audio_proc.vse.LSF_HIST_BUF_SIZE            9 + $audio_proc.vse.START_HISTORY_BUF_LSF; // y_l,y_l',y_l'',x ,x',x'',y_h,y_h',y_h''

   .CONST $audio_proc.vse.out_ipsi                     1 + $audio_proc.vse.LSF_HIST_BUF_SIZE;
   .CONST $audio_proc.vse.out_contra                   1 + $audio_proc.vse.out_ipsi;
   .CONST $audio_proc.vse.FILTER_COEFF_FIELD           1 + $audio_proc.vse.out_contra;
   .CONST $audio_proc.vse.STRUC_SIZE                   24 + $audio_proc.vse.FILTER_COEFF_FIELD;

#endif // AUDIO_3dv_HEADER_INCLUDED