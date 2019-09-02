// *****************************************************************************
// Copyright (c) 2009 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
//
// *****************************************************************************

// *****************************************************************************
// DESCRIPTION
//    Static configuration file that includes tables of function pointers and
//    corresponding data objects
//
//    Build configuration for processing modules shall be handled from within
//    the music_manager_config.h header file
//
// *****************************************************************************
#include "music_example.h"
#include "music_manager_config.h"
#include "audio_proc_library.h"
#include "cbops_library.h"
#include "codec_library.h"
#include "core_library.h"
#include "frame_sync_stream_macros.h"
#include "frame_sync_buffer.h"
#include "user_eq.h"
#include "default_eq_coefs.h"

#ifdef TWS_ENABLE
   #include "relay_conn.h"
#endif

#if defined(APTX_ACL_SPRINT_ENABLE)
   #include "codec_decoder_aptx_acl_sprint.h"
   #include "sr_adjustment_gaming.h"
#elseif defined (FASTSTREAM_ENABLE)
   #include "codec_decoder_faststream.h"
   #include "sr_adjustment_gaming.h"
#else
   #include "codec_decoder.h"
   #include "sr_adjustment.h"
#endif

#define MAX_NUM_SPKR_EQ_STAGES     (10)
#define MAX_NUM_BOOST_EQ_STAGES     (1)
#define MAX_NUM_USER_EQ_STAGES      (5)
#define MAX_NUM_ANC_EQ_STAGES      (5)
#define MAX_NUM_WIRED_SUB_EQ_STAGES (3)

#define MAX_NUM_SPKR_EQ_BANKS       (1)
#define MAX_NUM_BOOST_EQ_BANKS      (1)
#define MAX_NUM_USER_EQ_BANKS       (6)
#define MAX_NUM_ANC_EQ_BANKS       (1)
#define MAX_NUM_WIRED_SUB_EQ_BANKS  (1)

#define MAX_NUM_SPKR_CTRL_PRI_EQ_BANKS      (2)
#define MAX_NUM_SPKR_CTRL_PRI_EQ_STAGES     (7)

#define MAX_NUM_SPKR_CTRL_SEC_EQ_BANKS      (2)
#define MAX_NUM_SPKR_CTRL_SEC_EQ_STAGES     (7)

#define MAX_NUM_SPKR_CTRL_BASS_EQ_BANKS     (2)
#define MAX_NUM_SPKR_CTRL_BASS_EQ_STAGES    (6)

#if (MAX_NUM_SPKR_EQ_BANKS != 1)
    #error Number of Speaker Eq banks is not 1 - Mulitple bank switching not supported for Speaker Eq
#endif

#if (MAX_NUM_BOOST_EQ_BANKS != 1)
    #error Number of Bass Boost Eq banks is not 1 - Mulitple bank switching not supported for Bass boost Eq
#endif
#if (MAX_NUM_ANC_EQ_BANKS != 1)
    #error Number of ANC Eq banks is not 1 - Mulitple bank switching not supported for ANC Eq
#endif

.MODULE $M.system_config.data;
   .DATASEGMENT DM;

   // Temp Variable to handle disabled modules.
   .VAR  ZeroValue = 0;
   .VAR  OneValue = 1.0;
   .VAR  HalfValue = 0.5;
   .VAR  config;
   .VAR  MinusOne = -1;

   .VAR/DMCONST16  DefaultParameters[] =
   #include "music_manager_defaults.dat"
   ;

   // Parameter to Module Map
   .VAR/DM2 ParameterMap[] =

#if uses_STEREO_ENHANCEMENT
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_MELOD_EXPANSION_S_EQ_B1,  &MeloD_Expansion_coefficients + $MeloD_Expansion.COEF_S_EQ_B1,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_MELOD_EXPANSION_S_EQ_B0,  &MeloD_Expansion_coefficients + $MeloD_Expansion.COEF_S_EQ_B0,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_MELOD_EXPANSION_S_EQ_A1,  &MeloD_Expansion_coefficients + $MeloD_Expansion.COEF_S_EQ_A1,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_MELOD_EXPANSION_L_AP1_B2, &MeloD_Expansion_coefficients + $MeloD_Expansion.COEF_L_AP1_B2,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_MELOD_EXPANSION_L_AP1_B1, &MeloD_Expansion_coefficients + $MeloD_Expansion.COEF_L_AP1_B1,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_MELOD_EXPANSION_L_AP1_B0, &MeloD_Expansion_coefficients + $MeloD_Expansion.COEF_L_AP1_B0,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_MELOD_EXPANSION_L_AP2_B2, &MeloD_Expansion_coefficients + $MeloD_Expansion.COEF_L_AP2_B2,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_MELOD_EXPANSION_L_AP2_B1, &MeloD_Expansion_coefficients + $MeloD_Expansion.COEF_L_AP2_B1,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_MELOD_EXPANSION_L_AP2_B0, &MeloD_Expansion_coefficients + $MeloD_Expansion.COEF_L_AP2_B0,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_MELOD_EXPANSION_R_AP1_B2, &MeloD_Expansion_coefficients + $MeloD_Expansion.COEF_R_AP1_B2,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_MELOD_EXPANSION_R_AP1_B1, &MeloD_Expansion_coefficients + $MeloD_Expansion.COEF_R_AP1_B1,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_MELOD_EXPANSION_R_AP1_B0, &MeloD_Expansion_coefficients + $MeloD_Expansion.COEF_R_AP1_B0,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_MELOD_EXPANSION_R_AP2_B2, &MeloD_Expansion_coefficients + $MeloD_Expansion.COEF_R_AP2_B2,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_MELOD_EXPANSION_R_AP2_B1, &MeloD_Expansion_coefficients + $MeloD_Expansion.COEF_R_AP2_B1,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_MELOD_EXPANSION_R_AP2_B0, &MeloD_Expansion_coefficients + $MeloD_Expansion.COEF_R_AP2_B0,
#endif

#if uses_SPEAKER_CROSSOVER
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SPKR_CTRL_GAIN1_L_L_PRI,  &spkr_ctrl_gain_coefs_A + $spkr_ctrl_system.COEF_GAIN.GAIN_L_L_PRI,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SPKR_CTRL_GAIN1_R_L_PRI,  &spkr_ctrl_gain_coefs_A + $spkr_ctrl_system.COEF_GAIN.GAIN_R_L_PRI,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SPKR_CTRL_GAIN1_B_L_PRI,  &spkr_ctrl_gain_coefs_A + $spkr_ctrl_system.COEF_GAIN.GAIN_B_L_PRI,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SPKR_CTRL_GAIN1_L_R_PRI,  &spkr_ctrl_gain_coefs_A + $spkr_ctrl_system.COEF_GAIN.GAIN_L_R_PRI,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SPKR_CTRL_GAIN1_R_R_PRI,  &spkr_ctrl_gain_coefs_A + $spkr_ctrl_system.COEF_GAIN.GAIN_R_R_PRI,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SPKR_CTRL_GAIN1_B_R_PRI,  &spkr_ctrl_gain_coefs_A + $spkr_ctrl_system.COEF_GAIN.GAIN_B_R_PRI,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SPKR_CTRL_GAIN1_L_L_SEC,  &spkr_ctrl_gain_coefs_A + $spkr_ctrl_system.COEF_GAIN.GAIN_L_L_SEC,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SPKR_CTRL_GAIN1_R_L_SEC,  &spkr_ctrl_gain_coefs_A + $spkr_ctrl_system.COEF_GAIN.GAIN_R_L_SEC,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SPKR_CTRL_GAIN1_B_L_SEC,  &spkr_ctrl_gain_coefs_A + $spkr_ctrl_system.COEF_GAIN.GAIN_B_L_SEC,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SPKR_CTRL_GAIN1_L_R_SEC,  &spkr_ctrl_gain_coefs_A + $spkr_ctrl_system.COEF_GAIN.GAIN_L_R_SEC,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SPKR_CTRL_GAIN1_R_R_SEC,  &spkr_ctrl_gain_coefs_A + $spkr_ctrl_system.COEF_GAIN.GAIN_R_R_SEC,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SPKR_CTRL_GAIN1_B_R_SEC,  &spkr_ctrl_gain_coefs_A + $spkr_ctrl_system.COEF_GAIN.GAIN_B_R_SEC,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SPKR_CTRL_GAIN1_L_BASS,   &spkr_ctrl_gain_coefs_A + $spkr_ctrl_system.COEF_GAIN.GAIN_L_BASS,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SPKR_CTRL_GAIN1_R_BASS,   &spkr_ctrl_gain_coefs_A + $spkr_ctrl_system.COEF_GAIN.GAIN_R_BASS,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SPKR_CTRL_GAIN1_LFE_BASS, &spkr_ctrl_gain_coefs_A + $spkr_ctrl_system.COEF_GAIN.GAIN_LFE_BASS,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SPKR_CTRL_GAIN1_SUB,      &spkr_ctrl_gain_coefs_A + $spkr_ctrl_system.COEF_GAIN.GAIN_SUB,

      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SPKR_CTRL_GAIN2_L_L_PRI,  &spkr_ctrl_gain_coefs_B + $spkr_ctrl_system.COEF_GAIN.GAIN_L_L_PRI,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SPKR_CTRL_GAIN2_R_L_PRI,  &spkr_ctrl_gain_coefs_B + $spkr_ctrl_system.COEF_GAIN.GAIN_R_L_PRI,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SPKR_CTRL_GAIN2_B_L_PRI,  &spkr_ctrl_gain_coefs_B + $spkr_ctrl_system.COEF_GAIN.GAIN_B_L_PRI,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SPKR_CTRL_GAIN2_L_R_PRI,  &spkr_ctrl_gain_coefs_B + $spkr_ctrl_system.COEF_GAIN.GAIN_L_R_PRI,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SPKR_CTRL_GAIN2_R_R_PRI,  &spkr_ctrl_gain_coefs_B + $spkr_ctrl_system.COEF_GAIN.GAIN_R_R_PRI,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SPKR_CTRL_GAIN2_B_R_PRI,  &spkr_ctrl_gain_coefs_B + $spkr_ctrl_system.COEF_GAIN.GAIN_B_R_PRI,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SPKR_CTRL_GAIN2_L_L_SEC,  &spkr_ctrl_gain_coefs_B + $spkr_ctrl_system.COEF_GAIN.GAIN_L_L_SEC,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SPKR_CTRL_GAIN2_R_L_SEC,  &spkr_ctrl_gain_coefs_B + $spkr_ctrl_system.COEF_GAIN.GAIN_R_L_SEC,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SPKR_CTRL_GAIN2_B_L_SEC,  &spkr_ctrl_gain_coefs_B + $spkr_ctrl_system.COEF_GAIN.GAIN_B_L_SEC,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SPKR_CTRL_GAIN2_L_R_SEC,  &spkr_ctrl_gain_coefs_B + $spkr_ctrl_system.COEF_GAIN.GAIN_L_R_SEC,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SPKR_CTRL_GAIN2_R_R_SEC,  &spkr_ctrl_gain_coefs_B + $spkr_ctrl_system.COEF_GAIN.GAIN_R_R_SEC,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SPKR_CTRL_GAIN2_B_R_SEC,  &spkr_ctrl_gain_coefs_B + $spkr_ctrl_system.COEF_GAIN.GAIN_B_R_SEC,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SPKR_CTRL_GAIN2_L_BASS,   &spkr_ctrl_gain_coefs_B + $spkr_ctrl_system.COEF_GAIN.GAIN_L_BASS,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SPKR_CTRL_GAIN2_R_BASS,   &spkr_ctrl_gain_coefs_B + $spkr_ctrl_system.COEF_GAIN.GAIN_R_BASS,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SPKR_CTRL_GAIN2_LFE_BASS, &spkr_ctrl_gain_coefs_B + $spkr_ctrl_system.COEF_GAIN.GAIN_LFE_BASS,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SPKR_CTRL_GAIN2_SUB,      &spkr_ctrl_gain_coefs_B + $spkr_ctrl_system.COEF_GAIN.GAIN_SUB,
#endif // uses_SPEAKER_CROSSOVER

#if uses_COMPANDER
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_CONFIG,                 &cmpd100_obj + $cmpd100.OFFSET_CONTROL_WORD,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_EXPAND_THRESHOLD,       &cmpd100_obj + $cmpd100.OFFSET_EXPAND_THRESHOLD,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_LINEAR_THRESHOLD,       &cmpd100_obj + $cmpd100.OFFSET_LINEAR_THRESHOLD,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_COMPRESS_THRESHOLD,     &cmpd100_obj + $cmpd100.OFFSET_COMPRESS_THRESHOLD,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_LIMIT_THRESHOLD,        &cmpd100_obj + $cmpd100.OFFSET_LIMIT_THRESHOLD,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_INV_EXPAND_RATIO,       &cmpd100_obj + $cmpd100.OFFSET_INV_EXPAND_RATIO,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_INV_LINEAR_RATIO,       &cmpd100_obj + $cmpd100.OFFSET_INV_LINEAR_RATIO,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_INV_COMPRESS_RATIO,     &cmpd100_obj + $cmpd100.OFFSET_INV_COMPRESS_RATIO,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_INV_LIMIT_RATIO,        &cmpd100_obj + $cmpd100.OFFSET_INV_LIMIT_RATIO,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_EXPAND_ATTACK_TC,       &cmpd100_obj + $cmpd100.OFFSET_EXPAND_ATTACK_TC,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_EXPAND_DECAY_TC,        &cmpd100_obj + $cmpd100.OFFSET_EXPAND_DECAY_TC,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_LINEAR_ATTACK_TC,       &cmpd100_obj + $cmpd100.OFFSET_LINEAR_ATTACK_TC,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_LINEAR_DECAY_TC,        &cmpd100_obj + $cmpd100.OFFSET_LINEAR_DECAY_TC,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_COMPRESS_ATTACK_TC,     &cmpd100_obj + $cmpd100.OFFSET_COMPRESS_ATTACK_TC,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_COMPRESS_DECAY_TC,      &cmpd100_obj + $cmpd100.OFFSET_COMPRESS_DECAY_TC,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_LIMIT_ATTACK_TC,        &cmpd100_obj + $cmpd100.OFFSET_LIMIT_ATTACK_TC,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_LIMIT_DECAY_TC,         &cmpd100_obj + $cmpd100.OFFSET_LIMIT_DECAY_TC,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_MAKEUP_GAIN,            &cmpd100_obj + $cmpd100.OFFSET_MAKEUP_GAIN,
#endif

#if uses_WIRED_SUB_COMPANDER
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_CONFIG,                           &cmpd100_obj_wired_sub + $cmpd100.OFFSET_CONTROL_WORD,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_WIRED_SUB_EXPAND_THRESHOLD,       &cmpd100_obj_wired_sub + $cmpd100.OFFSET_EXPAND_THRESHOLD,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_WIRED_SUB_LINEAR_THRESHOLD,       &cmpd100_obj_wired_sub + $cmpd100.OFFSET_LINEAR_THRESHOLD,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_WIRED_SUB_COMPRESS_THRESHOLD,     &cmpd100_obj_wired_sub + $cmpd100.OFFSET_COMPRESS_THRESHOLD,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_WIRED_SUB_LIMIT_THRESHOLD,        &cmpd100_obj_wired_sub + $cmpd100.OFFSET_LIMIT_THRESHOLD,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_WIRED_SUB_INV_EXPAND_RATIO,       &cmpd100_obj_wired_sub + $cmpd100.OFFSET_INV_EXPAND_RATIO,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_WIRED_SUB_INV_LINEAR_RATIO,       &cmpd100_obj_wired_sub + $cmpd100.OFFSET_INV_LINEAR_RATIO,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_WIRED_SUB_INV_COMPRESS_RATIO,     &cmpd100_obj_wired_sub + $cmpd100.OFFSET_INV_COMPRESS_RATIO,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_WIRED_SUB_INV_LIMIT_RATIO,        &cmpd100_obj_wired_sub + $cmpd100.OFFSET_INV_LIMIT_RATIO,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_WIRED_SUB_EXPAND_ATTACK_TC,       &cmpd100_obj_wired_sub + $cmpd100.OFFSET_EXPAND_ATTACK_TC,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_WIRED_SUB_EXPAND_DECAY_TC,        &cmpd100_obj_wired_sub + $cmpd100.OFFSET_EXPAND_DECAY_TC,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_WIRED_SUB_LINEAR_ATTACK_TC,       &cmpd100_obj_wired_sub + $cmpd100.OFFSET_LINEAR_ATTACK_TC,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_WIRED_SUB_LINEAR_DECAY_TC,        &cmpd100_obj_wired_sub + $cmpd100.OFFSET_LINEAR_DECAY_TC,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_WIRED_SUB_COMPRESS_ATTACK_TC,     &cmpd100_obj_wired_sub + $cmpd100.OFFSET_COMPRESS_ATTACK_TC,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_WIRED_SUB_COMPRESS_DECAY_TC,      &cmpd100_obj_wired_sub + $cmpd100.OFFSET_COMPRESS_DECAY_TC,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_WIRED_SUB_LIMIT_ATTACK_TC,        &cmpd100_obj_wired_sub + $cmpd100.OFFSET_LIMIT_ATTACK_TC,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_WIRED_SUB_LIMIT_DECAY_TC,         &cmpd100_obj_wired_sub + $cmpd100.OFFSET_LIMIT_DECAY_TC,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_WIRED_SUB_MAKEUP_GAIN,            &cmpd100_obj_wired_sub + $cmpd100.OFFSET_MAKEUP_GAIN,
#endif

#if uses_DITHER
      // Update both the stereo and mono operators with the noise shape field
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_DITHER_NOISE_SHAPE,      &dithertype,
#endif

      // enable or disable the hard-limiter when applying volumes
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_CONFIG,                  &multichannel_volume_and_limit_obj + $volume_and_limit.OFFSET_CONTROL_WORD_FIELD,

#ifdef WIRELESS_SUB_ENABLE
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_CONFIG,                  &sub_volume_and_limit_obj + $volume_and_limit.OFFSET_CONTROL_WORD_FIELD,
#endif
      // Allow DC remove to be bypassed for test purposes
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_DC_REMOVE_DISABLE,       &$M.multi_chan_output.dc_remove_disable,

#if uses_BASS_PLUS
      // Bass Plus left channel parameters
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_DBE_CONFIG,             &bass_plus_params + $audio_proc.dbe.parameter.DBE_CONFIG,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_EFFECT_STRENGTH,        &bass_plus_params + $audio_proc.dbe.parameter.EFFECT_STRENGTH,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_AMP_LIMIT,              &bass_plus_params + $audio_proc.dbe.parameter.AMP_LIMIT,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_FC_LP,                  &bass_plus_params + $audio_proc.dbe.parameter.FC_LP,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_FC_HP,                  &bass_plus_params + $audio_proc.dbe.parameter.FC_HP,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_HARM_CONTENT,           &bass_plus_params + $audio_proc.dbe.parameter.HARM_CONTENT,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_XOVER_FC,               &bass_plus_params + $audio_proc.dbe.parameter.XOVER_FC,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_MIX_BALANCE,            &bass_plus_params + $audio_proc.dbe.parameter.MIX_BALANCE,

      // Sampling rate for left channel processing
      &$current_dac_sampling_rate,                                            &bass_plus_left + $audio_proc.dbe.SAMPLE_RATE_FIELD,

      // Sampling rate for right channel processing
      &$current_dac_sampling_rate,                                            &bass_plus_right + $audio_proc.dbe.SAMPLE_RATE_FIELD,
#endif

#if uses_3DV
      // 3DV left channel parameters
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_VSE_CONFIG,             &n3dv_params + $audio_proc.vse.parameter.VSE_CONFIG,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_BINAURAL_FLAG,          &n3dv_params + $audio_proc.vse.parameter.BINAURAL_FLAG,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SPEAKER_SPACING,        &n3dv_params + $audio_proc.vse.parameter.SPEAKER_SPACING,
      &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_VIRTUAL_ANGLE,          &n3dv_params + $audio_proc.vse.parameter.VIRTUAL_ANGLE,

      // Sampling rate for left channel processing
      &$current_dac_sampling_rate,                                            &n3dv_left + $audio_proc.vse.FS,

      // Sampling rate for right channel processing
      &$current_dac_sampling_rate,                                            &n3dv_right + $audio_proc.vse.FS,
#endif
      0;

// End of ParameterMap
// -----------------------------------------------------------------------------
   // guarantee even length
   .VAR  CurParams[2*ROUND(0.5*$M.MUSIC_MANAGER.PARAMETERS.STRUCT_SIZE)];
// -----------------------------------------------------------------------------
// DATA OBJECTS USED WITH PROCESSING MODULES
//
// This section would be updated if more processing modules with data objects
// were to be added to the system.

// Creating 12dB headroom for processing modules, the headroom will be
// compensated in volume module at the end of processing chain
   .VAR headroom_mant = 0.25; // 2 bits attenuation

   .VAR ModeControl[$music_example.STRUC_SIZE];

   .VAR passthru_primary_left[$M.audio_proc.stream_gain.STRUC_SIZE] =
     &stream_map_left_in,                             // OFFSET_INPUT_PTR
     0,                                               // OFFSET_OUTPUT_PTR
     &ModeControl + $music_example.PRIM_LEFT_MANT,    // OFFSET_PTR_MANTISSA
     &ModeControl + $music_example.PRIM_LEFT_EXP;     // OFFSET_PTR_EXPONENT

   .VAR passthru_primary_right[$M.audio_proc.stream_gain.STRUC_SIZE] =
     &stream_map_right_in,                            // OFFSET_INPUT_PTR
     0,                                               // OFFSET_OUTPUT_PTR
     &ModeControl + $music_example.PRIM_RGHT_MANT,    // OFFSET_PTR_MANTISSA
     &ModeControl + $music_example.PRIM_RGHT_EXP;     // OFFSET_PTR_EXPONENT

   .VAR passthru_sub[$M.audio_proc.stream_gain.STRUC_SIZE] =
     &stream_map_left_in,                             // OFFSET_INPUT_PTR
     0,                                               // OFFSET_OUTPUT_PTR
     &ModeControl + $music_example.SUB_MANT,          // OFFSET_PTR_MANTISSA
     &ModeControl + $music_example.SUB_EXP;           // OFFSET_PTR_EXPONENT

   .VAR passthru_secondary_left[$M.audio_proc.stream_gain.STRUC_SIZE] =
     &stream_map_left_in,                             // OFFSET_INPUT_PTR
     0,                                               // OFFSET_OUTPUT_PTR
     &ModeControl + $music_example.SCND_LEFT_MANT,    // OFFSET_PTR_MANTISSA
     &ModeControl + $music_example.SCND_LEFT_EXP;     // OFFSET_PTR_EXPONENT

   .VAR passthru_secondary_right[$M.audio_proc.stream_gain.STRUC_SIZE] =
     &stream_map_right_in,                            // OFFSET_INPUT_PTR
     0,                                               // OFFSET_OUTPUT_PTR
     &ModeControl + $music_example.SCND_RGHT_MANT,    // OFFSET_PTR_MANTISSA
     &ModeControl + $music_example.SCND_RGHT_EXP;     // OFFSET_PTR_EXPONENT

 // Used in full processing
   .VAR left_headroom_obj[$M.audio_proc.stream_gain.STRUC_SIZE] =
     &stream_map_left_in,              // OFFSET_INPUT_PTR
     &stream_map_left_in,              // OFFSET_OUTPUT_PTR
     &headroom_mant,                   // OFFSET_PTR_MANTISSA
     &ZeroValue;                       // OFFSET_PTR_EXPONENT

   .VAR right_headroom_obj[$M.audio_proc.stream_gain.STRUC_SIZE] =
     &stream_map_right_in,             // OFFSET_INPUT_PTR
     &stream_map_right_in,             // OFFSET_OUTPUT_PTR
     &headroom_mant,                   // OFFSET_PTR_MANTISSA
     &ZeroValue;                       // OFFSET_PTR_EXPONENT

#if defined(SPDIF_ENABLE) && defined(AC3_ENABLE)
   .VAR lfe_headroom_obj[$M.audio_proc.stream_gain.STRUC_SIZE] =
     &stream_map_lfe_in,     // OFFSET_INPUT_PTR
     &stream_map_lfe_in,     // OFFSET_OUTPUT_PTR
     &headroom_mant,          // OFFSET_PTR_MANTISSA
     &ZeroValue;              // OFFSET_PTR_EXPONENT
#endif // defined(SPDIF_ENABLE) && defined(AC3_ENABLE)

   .VAR pcmin_l_pk_dtct[$M.audio_proc.peak_monitor.STRUCT_SIZE] =
      &stream_map_left_in,                 // PTR_INPUT_BUFFER_FIELD
      0;                                   // PEAK_LEVEL

   .VAR pcmin_r_pk_dtct[$M.audio_proc.peak_monitor.STRUCT_SIZE] =
      &stream_map_right_in,                // PTR_INPUT_BUFFER_FIELD
      0;                                   // PEAK_LEVEL

#if defined(SPDIF_ENABLE) && defined(AC3_ENABLE)
   .VAR pcmin_lfe_pk_dtct[$M.audio_proc.peak_monitor.STRUCT_SIZE] =
      &stream_map_lfe_in,                  // PTR_INPUT_BUFFER_FIELD
      0;                                   // PEAK_LEVEL
#endif // defined(SPDIF_ENABLE) && defined(AC3_ENABLE)

   .VAR primout_l_pk_dtct[$M.audio_proc.peak_monitor.STRUCT_SIZE] =
      &stream_map_primary_left_out,        // PTR_INPUT_BUFFER_FIELD
      0;                                   // PEAK_LEVEL

   .VAR primout_r_pk_dtct[$M.audio_proc.peak_monitor.STRUCT_SIZE] =
      &stream_map_primary_right_out,       // PTR_INPUT_BUFFER_FIELD
      0;                                   // PEAK_LEVEL

   .VAR sub_pk_dtct[$M.audio_proc.peak_monitor.STRUCT_SIZE] =
      &stream_map_sub_out,                 // PTR_INPUT_BUFFER_FIELD
      0;                                   // PEAK_LEVEL_PTR

   .VAR scndout_l_pk_dtct[$M.audio_proc.peak_monitor.STRUCT_SIZE] =
      &stream_map_secondary_left_out,      // PTR_INPUT_BUFFER_FIELD
      0;                                   // PEAK_LEVEL

   .VAR scndout_r_pk_dtct[$M.audio_proc.peak_monitor.STRUCT_SIZE] =
      &stream_map_secondary_right_out,     // PTR_INPUT_BUFFER_FIELD
      0;                                   // PEAK_LEVEL

   .VAR auxout_l_pk_dtct[$M.audio_proc.peak_monitor.STRUCT_SIZE] =
      &stream_map_aux_left_out,            // PTR_INPUT_BUFFER_FIELD
      0;                                   // PEAK_LEVEL

   .VAR auxout_r_pk_dtct[$M.audio_proc.peak_monitor.STRUCT_SIZE] =
      &stream_map_aux_right_out,           // PTR_INPUT_BUFFER_FIELD
      0;                                   // PEAK_LEVEL

#if uses_SPKR_EQ

   .VAR/DM2 spkr_eq_left_dm2[HQ_PEQ_OBJECT_SIZE(MAX_NUM_SPKR_EQ_STAGES)] =
        &stream_map_left_in,                    // PTR_INPUT_DATA_BUFF_FIELD
        &stream_map_left_in,                    // PTR_OUTPUT_DATA_BUFF_FIELD
        MAX_NUM_SPKR_EQ_STAGES,                 // MAX_STAGES_FIELD
        &SpkrEqCoefsA,                          // PARAM_PTR_FIELD
        0 ...;

   .VAR/DM2 spkr_eq_right_dm2[HQ_PEQ_OBJECT_SIZE(MAX_NUM_SPKR_EQ_STAGES)] =
        &stream_map_right_in,                   // PTR_INPUT_DATA_BUFF_FIELD
        &stream_map_right_in,                   // PTR_OUTPUT_DATA_BUFF_FIELD
        MAX_NUM_SPKR_EQ_STAGES,                 // MAX_STAGES_FIELD
        &SpkrEqCoefsA,                          // PARAM_PTR_FIELD
        0 ...;

   // 44.1 kHz coefficients if not using coefficient calculation routines
    .VAR SpkrEqCoefsA[3+6*MAX_NUM_SPKR_EQ_STAGES] =
        $spkrEq.Fs44.NumBands,
        $spkrEq.Fs44.GainExp,
        $spkrEq.Fs44.GainMant,
        $spkrEq.Fs44.Stage1.b2,  $spkrEq.Fs44.Stage1.b1,  $spkrEq.Fs44.Stage1.b0,  $spkrEq.Fs44.Stage1.a2,  $spkrEq.Fs44.Stage1.a1,
        $spkrEq.Fs44.Stage2.b2,  $spkrEq.Fs44.Stage2.b1,  $spkrEq.Fs44.Stage2.b0,  $spkrEq.Fs44.Stage2.a2,  $spkrEq.Fs44.Stage2.a1,
        $spkrEq.Fs44.Stage3.b2,  $spkrEq.Fs44.Stage3.b1,  $spkrEq.Fs44.Stage3.b0,  $spkrEq.Fs44.Stage3.a2,  $spkrEq.Fs44.Stage3.a1,
        $spkrEq.Fs44.Stage4.b2,  $spkrEq.Fs44.Stage4.b1,  $spkrEq.Fs44.Stage4.b0,  $spkrEq.Fs44.Stage4.a2,  $spkrEq.Fs44.Stage4.a1,
        $spkrEq.Fs44.Stage5.b2,  $spkrEq.Fs44.Stage5.b1,  $spkrEq.Fs44.Stage5.b0,  $spkrEq.Fs44.Stage5.a2,  $spkrEq.Fs44.Stage5.a1,
        $spkrEq.Fs44.Stage6.b2,  $spkrEq.Fs44.Stage6.b1,  $spkrEq.Fs44.Stage6.b0,  $spkrEq.Fs44.Stage6.a2,  $spkrEq.Fs44.Stage6.a1,
        $spkrEq.Fs44.Stage7.b2,  $spkrEq.Fs44.Stage7.b1,  $spkrEq.Fs44.Stage7.b0,  $spkrEq.Fs44.Stage7.a2,  $spkrEq.Fs44.Stage7.a1,
        $spkrEq.Fs44.Stage8.b2,  $spkrEq.Fs44.Stage8.b1,  $spkrEq.Fs44.Stage8.b0,  $spkrEq.Fs44.Stage8.a2,  $spkrEq.Fs44.Stage8.a1,
        $spkrEq.Fs44.Stage9.b2,  $spkrEq.Fs44.Stage9.b1,  $spkrEq.Fs44.Stage9.b0,  $spkrEq.Fs44.Stage9.a2,  $spkrEq.Fs44.Stage9.a1,
        $spkrEq.Fs44.Stage10.b2, $spkrEq.Fs44.Stage10.b1, $spkrEq.Fs44.Stage10.b0, $spkrEq.Fs44.Stage10.a2, $spkrEq.Fs44.Stage10.a1,
      $spkrEq.Fs44.Stage1.scale, $spkrEq.Fs44.Stage2.scale, $spkrEq.Fs44.Stage3.scale, $spkrEq.Fs44.Stage4.scale, $spkrEq.Fs44.Stage5.scale,
      $spkrEq.Fs44.Stage6.scale, $spkrEq.Fs44.Stage7.scale, $spkrEq.Fs44.Stage8.scale, $spkrEq.Fs44.Stage9.scale, $spkrEq.Fs44.Stage10.scale;

   // 48 kHz coefficients if not using coefficient calculation routines
    .VAR SpkrEqCoefsB[3+6*MAX_NUM_SPKR_EQ_STAGES] =
        $spkrEq.Fs48.NumBands,
        $spkrEq.Fs48.GainExp,
        $spkrEq.Fs48.GainMant,
        $spkrEq.Fs48.Stage1.b2,  $spkrEq.Fs48.Stage1.b1,  $spkrEq.Fs48.Stage1.b0,  $spkrEq.Fs48.Stage1.a2,  $spkrEq.Fs48.Stage1.a1,
        $spkrEq.Fs48.Stage2.b2,  $spkrEq.Fs48.Stage2.b1,  $spkrEq.Fs48.Stage2.b0,  $spkrEq.Fs48.Stage2.a2,  $spkrEq.Fs48.Stage2.a1,
        $spkrEq.Fs48.Stage3.b2,  $spkrEq.Fs48.Stage3.b1,  $spkrEq.Fs48.Stage3.b0,  $spkrEq.Fs48.Stage3.a2,  $spkrEq.Fs48.Stage3.a1,
        $spkrEq.Fs48.Stage4.b2,  $spkrEq.Fs48.Stage4.b1,  $spkrEq.Fs48.Stage4.b0,  $spkrEq.Fs48.Stage4.a2,  $spkrEq.Fs48.Stage4.a1,
        $spkrEq.Fs48.Stage5.b2,  $spkrEq.Fs48.Stage5.b1,  $spkrEq.Fs48.Stage5.b0,  $spkrEq.Fs48.Stage5.a2,  $spkrEq.Fs48.Stage5.a1,
        $spkrEq.Fs48.Stage6.b2,  $spkrEq.Fs48.Stage6.b1,  $spkrEq.Fs48.Stage6.b0,  $spkrEq.Fs48.Stage6.a2,  $spkrEq.Fs48.Stage6.a1,
        $spkrEq.Fs48.Stage7.b2,  $spkrEq.Fs48.Stage7.b1,  $spkrEq.Fs48.Stage7.b0,  $spkrEq.Fs48.Stage7.a2,  $spkrEq.Fs48.Stage7.a1,
        $spkrEq.Fs48.Stage8.b2,  $spkrEq.Fs48.Stage8.b1,  $spkrEq.Fs48.Stage8.b0,  $spkrEq.Fs48.Stage8.a2,  $spkrEq.Fs48.Stage8.a1,
        $spkrEq.Fs48.Stage9.b2,  $spkrEq.Fs48.Stage9.b1,  $spkrEq.Fs48.Stage9.b0,  $spkrEq.Fs48.Stage9.a2,  $spkrEq.Fs48.Stage9.a1,
        $spkrEq.Fs48.Stage10.b2, $spkrEq.Fs48.Stage10.b1, $spkrEq.Fs48.Stage10.b0, $spkrEq.Fs48.Stage10.a2, $spkrEq.Fs48.Stage10.a1,
      $spkrEq.Fs48.Stage1.scale, $spkrEq.Fs48.Stage2.scale, $spkrEq.Fs48.Stage3.scale, $spkrEq.Fs48.Stage4.scale, $spkrEq.Fs48.Stage5.scale,
      $spkrEq.Fs48.Stage6.scale, $spkrEq.Fs48.Stage7.scale, $spkrEq.Fs48.Stage8.scale, $spkrEq.Fs48.Stage9.scale, $spkrEq.Fs48.Stage10.scale;

    .VAR SpkrEqDefnTable[$user_eq.DEFINITION_TABLE_SIZE] =
        MAX_NUM_SPKR_EQ_BANKS,
        MAX_NUM_SPKR_EQ_STAGES,
        &spkr_eq_left_dm2,
        &spkr_eq_right_dm2,
        &SpkrEqCoefsA,
        &SpkrEqCoefsB;

   // pointer to speaker eq parameters
   // if zero, use the coefficients that are in the code above
   #if USE_PRECALCULATED_SPKR_COEFS
      #ifdef ROM
         #error cannot use precalculated coefficients with ROM part
      #endif
      .var SpkrEqParams = 0;
   #else
        .var SpkrEqParams = &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SPKR_EQ_NUM_BANDS;
   #endif  // USE_PRECALCULATED_SPKR_COEFS

#endif  // uses_SPKR_EQ

#if uses_BASS_BOOST

    // object for currently running Boost EQs
   .VAR/DM2 boost_eq_left_dm2[HQ_PEQ_OBJECT_SIZE(MAX_NUM_BOOST_EQ_STAGES)] =
      &stream_map_left_in,              // PTR_INPUT_DATA_BUFF_FIELD
      &stream_map_left_in,              // PTR_OUTPUT_DATA_BUFF_FIELD
      MAX_NUM_BOOST_EQ_STAGES,          // MAX_STAGES_FIELD
      &BoostEqCoefsA,                   // PARAM_PTR_FIELD
      0 ...;

   .VAR/DM2 boost_eq_right_dm2[HQ_PEQ_OBJECT_SIZE(MAX_NUM_BOOST_EQ_STAGES)] =
      &stream_map_right_in,             // PTR_INPUT_DATA_BUFF_FIELD
      &stream_map_right_in,             // PTR_OUTPUT_DATA_BUFF_FIELD
      MAX_NUM_BOOST_EQ_STAGES,          // MAX_STAGES_FIELD
      &BoostEqCoefsA,                   // PARAM_PTR_FIELD
      0 ...;

   // 44.1 kHz coefficients if not using coefficient calculation routines
    .VAR BoostEqCoefsA[3+6*MAX_NUM_BOOST_EQ_STAGES] =
        $BoostEq.Fs44.NumBands,
        $BoostEq.Fs44.GainExp,
        $BoostEq.Fs44.GainMant,
        $BoostEq.Fs44.b2,  $BoostEq.Fs44.b1,  $BoostEq.Fs44.b0,  $BoostEq.Fs44.a2,  $BoostEq.Fs44.a1,
      $BoostEq.Fs44.scale;

   // 48 kHz coefficients if not using coefficient calculation routines
    .VAR BoostEqCoefsB[3+6*MAX_NUM_BOOST_EQ_STAGES] =
        $BoostEq.Fs48.NumBands,
        $BoostEq.Fs48.GainExp,
        $BoostEq.Fs48.GainMant,
        $BoostEq.Fs48.b2,  $BoostEq.Fs48.b1,  $BoostEq.Fs48.b0,  $BoostEq.Fs48.a2,  $BoostEq.Fs48.a1,
      $BoostEq.Fs48.scale;

    .VAR BoostEqDefnTable[$user_eq.DEFINITION_TABLE_SIZE] =
        MAX_NUM_BOOST_EQ_BANKS,
        MAX_NUM_BOOST_EQ_STAGES,
        &boost_eq_left_dm2,
        &boost_eq_right_dm2,
        &BoostEqCoefsA,
        &BoostEqCoefsB;

   // pointer to bass boost eq parameters
   // if zero, use the coefficients that are in the code above
   #if USE_PRECALCULATED_BOOST_COEFS
      #ifdef ROM
         #error cannot use precalculated coefficients with ROM part
      #endif
      .var BoostEqParams = 0;
   #else
      .var BoostEqParams = &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_BOOST_EQ_NUM_BANDS;
   #endif  // USE_PRECALCULATED_BOOST_COEFS

#endif  // uses_BASS_BOOST

#if uses_ANC_EQ
    // object for currently running Boost EQs
   .VAR/DM2 anc_eq_left_dm2[HQ_PEQ_OBJECT_SIZE(MAX_NUM_ANC_EQ_STAGES)] =
      &stream_map_left_in,              // PTR_INPUT_DATA_BUFF_FIELD
      &stream_map_left_in,              // PTR_OUTPUT_DATA_BUFF_FIELD
      MAX_NUM_ANC_EQ_STAGES,          // MAX_STAGES_FIELD
      &AncEqCoefsA,                   // PARAM_PTR_FIELD
      0 ...;
   .VAR/DM2 anc_eq_right_dm2[HQ_PEQ_OBJECT_SIZE(MAX_NUM_ANC_EQ_STAGES)] =
      &stream_map_right_in,             // PTR_INPUT_DATA_BUFF_FIELD
      &stream_map_right_in,             // PTR_OUTPUT_DATA_BUFF_FIELD
      MAX_NUM_ANC_EQ_STAGES,          // MAX_STAGES_FIELD
      &AncEqCoefsA,                   // PARAM_PTR_FIELD
      0 ...;
   // 44.1 kHz coefficients if not using coefficient calculation routines
    .VAR AncEqCoefsA[3+6*MAX_NUM_ANC_EQ_STAGES] =
        $AncEq.Fs44.NumBands,
        $AncEq.Fs44.GainExp,
        $AncEq.Fs44.GainMant,
        $AncEq.Fs44.Stage1.b2,  $AncEq.Fs44.Stage1.b1,  $AncEq.Fs44.Stage1.b0,  $AncEq.Fs44.Stage1.a2,  $AncEq.Fs44.Stage1.a1,
        $AncEq.Fs44.Stage2.b2,  $AncEq.Fs44.Stage2.b1,  $AncEq.Fs44.Stage2.b0,  $AncEq.Fs44.Stage2.a2,  $AncEq.Fs44.Stage2.a1,
        $AncEq.Fs44.Stage3.b2,  $AncEq.Fs44.Stage3.b1,  $AncEq.Fs44.Stage3.b0,  $AncEq.Fs44.Stage3.a2,  $AncEq.Fs44.Stage3.a1,
        $AncEq.Fs44.Stage4.b2,  $AncEq.Fs44.Stage4.b1,  $AncEq.Fs44.Stage4.b0,  $AncEq.Fs44.Stage4.a2,  $AncEq.Fs44.Stage4.a1,
        $AncEq.Fs44.Stage5.b2,  $AncEq.Fs44.Stage5.b1,  $AncEq.Fs44.Stage5.b0,  $AncEq.Fs44.Stage5.a2,  $AncEq.Fs44.Stage5.a1,
        $AncEq.Fs44.Stage1.scale,$AncEq.Fs44.Stage2.scale,$AncEq.Fs44.Stage3.scale,$AncEq.Fs44.Stage4.scale,$AncEq.Fs44.Stage5.scale;
   // 48 kHz coefficients if not using coefficient calculation routines
    .VAR AncEqCoefsB[3+6*MAX_NUM_ANC_EQ_STAGES] =
        $AncEq.Fs48.NumBands,
        $AncEq.Fs48.GainExp,
        $AncEq.Fs48.GainMant,
        $AncEq.Fs48.Stage1.b2,  $AncEq.Fs48.Stage1.b1,  $AncEq.Fs48.Stage1.b0,  $AncEq.Fs48.Stage1.a2,  $AncEq.Fs48.Stage1.a1,
        $AncEq.Fs48.Stage2.b2,  $AncEq.Fs48.Stage2.b1,  $AncEq.Fs48.Stage2.b0,  $AncEq.Fs48.Stage2.a2,  $AncEq.Fs48.Stage2.a1,
        $AncEq.Fs48.Stage3.b2,  $AncEq.Fs48.Stage3.b1,  $AncEq.Fs48.Stage3.b0,  $AncEq.Fs48.Stage3.a2,  $AncEq.Fs48.Stage3.a1,
        $AncEq.Fs48.Stage4.b2,  $AncEq.Fs48.Stage4.b1,  $AncEq.Fs48.Stage4.b0,  $AncEq.Fs48.Stage4.a2,  $AncEq.Fs48.Stage4.a1,
        $AncEq.Fs48.Stage5.b2,  $AncEq.Fs48.Stage5.b1,  $AncEq.Fs48.Stage5.b0,  $AncEq.Fs48.Stage5.a2,  $AncEq.Fs48.Stage5.a1,
        $AncEq.Fs48.Stage1.scale,$AncEq.Fs48.Stage2.scale,$AncEq.Fs48.Stage3.scale,$AncEq.Fs48.Stage4.scale,$AncEq.Fs48.Stage5.scale;
    .VAR AncEqDefnTable[$user_eq.DEFINITION_TABLE_SIZE] =
        MAX_NUM_ANC_EQ_BANKS,
        MAX_NUM_ANC_EQ_STAGES,
        &anc_eq_left_dm2,
        &anc_eq_right_dm2,
        &AncEqCoefsA,
        &AncEqCoefsB;
   // pointer to bass boost eq parameters
   // if zero, use the coefficients that are in the code above
   #if USE_PRECALCULATED_ANC_COEFS
      #ifdef ROM
         #error cannot use precalculated coefficients with ROM part
      #endif
      .var AncEqParams = 0;
   #else
      .var AncEqParams = &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_ANC_EQ_NUM_BANDS;
   #endif  // USE_PRECALCULATED_BOOST_COEFS
#endif  // uses_ANC_EQ
#if uses_BASS_PLUS
   // Bass Plus parameter storage
   .VAR bass_plus_params[$audio_proc.dbe.parameter.STRUCT_SIZE];

   // Bass Plus data object structures
   .VAR bass_plus_left[$audio_proc.dbe.STRUC_SIZE] =
      stream_map_left_in,                 // INPUT_ADDR_FIELD (in-place processing)
      stream_map_left_in,                 // OUTPUT_ADDR_FIELD (in-place processing)
      1,                                  // MONO_STEREO_FLAG_FIELD (always stereo here)
      0,                                  // SAMPLE_RATE_FIELD (initialised by Parameter to Module Map copy)
      bass_plus_params,                   // PARAM_PTR_FIELD
      0 ...;

   .VAR bass_plus_right[$audio_proc.dbe.STRUC_SIZE] =
      stream_map_right_in,                // INPUT_ADDR_FIELD (in-place processing)
      stream_map_right_in,                // OUTPUT_ADDR_FIELD (in-place processing)
      1,                                  // MONO_STEREO_FLAG_FIELD (always stereo here)
      0,                                  // SAMPLE_RATE_FIELD (initialised by Parameter to Module Map copy)
      bass_plus_params,                   // PARAM_PTR_FIELD
      0 ...;
#endif  // uses_BASS_PLUS

#if uses_3DV
   // 3DV parameter storage
   .VAR n3dv_params[$audio_proc.vse.parameter.STRUCT_SIZE];

   // 3DV data object structures
   .VAR n3dv_left[$audio_proc.vse.STRUC_SIZE] =
      stream_map_left_in,                 // INPUT_ADDR_FIELD (in-place processing)
      stream_map_left_in,                 // OUTPUT_ADDR_FIELD (in-place processing)
      n3dv_params,                        // PARAM_PTR_FIELD
      0,                                  // FS (initialised by Parameter to Module Map copy)
      0 ...;

   .VAR n3dv_right[$audio_proc.vse.STRUC_SIZE] =
      stream_map_right_in,                // INPUT_ADDR_FIELD (in-place processing)
      stream_map_right_in,                // OUTPUT_ADDR_FIELD (in-place processing)
      n3dv_params,                        // PARAM_PTR_FIELD
      0,                                  // FS (initialised by Parameter to Module Map copy)
      0 ...;
#endif  // uses_3DV

#if uses_USER_EQ

    // object for currently running User EQs
   .VAR/DM2 user_eq_left_dm2[HQ_PEQ_OBJECT_SIZE(MAX_NUM_USER_EQ_STAGES)] =
      &stream_map_left_in,              // PTR_INPUT_DATA_BUFF_FIELD
      &stream_map_left_in,              // PTR_OUTPUT_DATA_BUFF_FIELD
      MAX_NUM_USER_EQ_STAGES,           // MAX_STAGES_FIELD
      &UserEqCoefsA,                    // PARAM_PTR_FIELD
      0 ...;

    .VAR/DM2 user_eq_right_dm2[HQ_PEQ_OBJECT_SIZE(MAX_NUM_USER_EQ_STAGES)] =
      &stream_map_right_in,             // PTR_INPUT_DATA_BUFF_FIELD
      &stream_map_right_in,             // PTR_OUTPUT_DATA_BUFF_FIELD
      MAX_NUM_USER_EQ_STAGES,           // MAX_STAGES_FIELD
      &UserEqCoefsA,                    // PARAM_PTR_FIELD
      0 ...;

   #if USE_PRECALCULATED_USER_COEFS
        .BLOCK PrecalculatedUserEqCoefficients;
         // coefficients if not using coefficient calculation routines
            .VAR UserEqCoefsA[3+6*MAX_NUM_USER_EQ_STAGES] =             // 44 kHz bank 1
                $userEq.Fs44.Bank1.NumBands,
                $userEq.Fs44.Bank1.GainExp,
                $userEq.Fs44.Bank1.GainMant,
                $userEq.Fs44.Bank1.Stage1.b2,  $userEq.Fs44.Bank1.Stage1.b1,  $userEq.Fs44.Bank1.Stage1.b0,  $userEq.Fs44.Bank1.Stage1.a2,  $userEq.Fs44.Bank1.Stage1.a1,
                $userEq.Fs44.Bank1.Stage2.b2,  $userEq.Fs44.Bank1.Stage2.b1,  $userEq.Fs44.Bank1.Stage2.b0,  $userEq.Fs44.Bank1.Stage2.a2,  $userEq.Fs44.Bank1.Stage2.a1,
                $userEq.Fs44.Bank1.Stage3.b2,  $userEq.Fs44.Bank1.Stage3.b1,  $userEq.Fs44.Bank1.Stage3.b0,  $userEq.Fs44.Bank1.Stage3.a2,  $userEq.Fs44.Bank1.Stage3.a1,
                $userEq.Fs44.Bank1.Stage4.b2,  $userEq.Fs44.Bank1.Stage4.b1,  $userEq.Fs44.Bank1.Stage4.b0,  $userEq.Fs44.Bank1.Stage4.a2,  $userEq.Fs44.Bank1.Stage4.a1,
                $userEq.Fs44.Bank1.Stage5.b2,  $userEq.Fs44.Bank1.Stage5.b1,  $userEq.Fs44.Bank1.Stage5.b0,  $userEq.Fs44.Bank1.Stage5.a2,  $userEq.Fs44.Bank1.Stage5.a1,
                $userEq.Fs44.Bank1.Stage1.scale, $userEq.Fs44.Bank1.Stage2.scale, $userEq.Fs44.Bank1.Stage3.scale, $userEq.Fs44.Bank1.Stage4.scale, $userEq.Fs44.Bank1.Stage5.scale;
            .VAR UserEqCoefsB[3+6*MAX_NUM_USER_EQ_STAGES] =             // 44 kHz bank 2
                $userEq.Fs44.Bank2.NumBands,
                $userEq.Fs44.Bank2.GainExp,
                $userEq.Fs44.Bank2.GainMant,
                $userEq.Fs44.Bank2.Stage1.b2,  $userEq.Fs44.Bank2.Stage1.b1,  $userEq.Fs44.Bank2.Stage1.b0,  $userEq.Fs44.Bank2.Stage1.a2,  $userEq.Fs44.Bank2.Stage1.a1,
                $userEq.Fs44.Bank2.Stage2.b2,  $userEq.Fs44.Bank2.Stage2.b1,  $userEq.Fs44.Bank2.Stage2.b0,  $userEq.Fs44.Bank2.Stage2.a2,  $userEq.Fs44.Bank2.Stage2.a1,
                $userEq.Fs44.Bank2.Stage3.b2,  $userEq.Fs44.Bank2.Stage3.b1,  $userEq.Fs44.Bank2.Stage3.b0,  $userEq.Fs44.Bank2.Stage3.a2,  $userEq.Fs44.Bank2.Stage3.a1,
                $userEq.Fs44.Bank2.Stage4.b2,  $userEq.Fs44.Bank2.Stage4.b1,  $userEq.Fs44.Bank2.Stage4.b0,  $userEq.Fs44.Bank2.Stage4.a2,  $userEq.Fs44.Bank2.Stage4.a1,
                $userEq.Fs44.Bank2.Stage5.b2,  $userEq.Fs44.Bank2.Stage5.b1,  $userEq.Fs44.Bank2.Stage5.b0,  $userEq.Fs44.Bank2.Stage5.a2,  $userEq.Fs44.Bank2.Stage5.a1,
                $userEq.Fs44.Bank2.Stage1.scale, $userEq.Fs44.Bank2.Stage2.scale, $userEq.Fs44.Bank2.Stage3.scale, $userEq.Fs44.Bank2.Stage4.scale, $userEq.Fs44.Bank2.Stage5.scale;
            .VAR UserEqCoefs3[3+6*MAX_NUM_USER_EQ_STAGES] =             // 44 kHz bank 3
                $userEq.Fs44.Bank3.NumBands,
                $userEq.Fs44.Bank3.GainExp,
                $userEq.Fs44.Bank3.GainMant,
                $userEq.Fs44.Bank3.Stage1.b2,  $userEq.Fs44.Bank3.Stage1.b1,  $userEq.Fs44.Bank3.Stage1.b0,  $userEq.Fs44.Bank3.Stage1.a2,  $userEq.Fs44.Bank3.Stage1.a1,
                $userEq.Fs44.Bank3.Stage2.b2,  $userEq.Fs44.Bank3.Stage2.b1,  $userEq.Fs44.Bank3.Stage2.b0,  $userEq.Fs44.Bank3.Stage2.a2,  $userEq.Fs44.Bank3.Stage2.a1,
                $userEq.Fs44.Bank3.Stage3.b2,  $userEq.Fs44.Bank3.Stage3.b1,  $userEq.Fs44.Bank3.Stage3.b0,  $userEq.Fs44.Bank3.Stage3.a2,  $userEq.Fs44.Bank3.Stage3.a1,
                $userEq.Fs44.Bank3.Stage4.b2,  $userEq.Fs44.Bank3.Stage4.b1,  $userEq.Fs44.Bank3.Stage4.b0,  $userEq.Fs44.Bank3.Stage4.a2,  $userEq.Fs44.Bank3.Stage4.a1,
                $userEq.Fs44.Bank3.Stage5.b2,  $userEq.Fs44.Bank3.Stage5.b1,  $userEq.Fs44.Bank3.Stage5.b0,  $userEq.Fs44.Bank3.Stage5.a2,  $userEq.Fs44.Bank3.Stage5.a1,
                $userEq.Fs44.Bank3.Stage1.scale, $userEq.Fs44.Bank3.Stage2.scale, $userEq.Fs44.Bank3.Stage3.scale, $userEq.Fs44.Bank3.Stage4.scale, $userEq.Fs44.Bank3.Stage5.scale;
            .VAR UserEqCoefs4[3+6*MAX_NUM_USER_EQ_STAGES] =             // 44 kHz bank 4
                $userEq.Fs44.Bank4.NumBands,
                $userEq.Fs44.Bank4.GainExp,
                $userEq.Fs44.Bank4.GainMant,
                $userEq.Fs44.Bank4.Stage1.b2,  $userEq.Fs44.Bank4.Stage1.b1,  $userEq.Fs44.Bank4.Stage1.b0,  $userEq.Fs44.Bank4.Stage1.a2,  $userEq.Fs44.Bank4.Stage1.a1,
                $userEq.Fs44.Bank4.Stage2.b2,  $userEq.Fs44.Bank4.Stage2.b1,  $userEq.Fs44.Bank4.Stage2.b0,  $userEq.Fs44.Bank4.Stage2.a2,  $userEq.Fs44.Bank4.Stage2.a1,
                $userEq.Fs44.Bank4.Stage3.b2,  $userEq.Fs44.Bank4.Stage3.b1,  $userEq.Fs44.Bank4.Stage3.b0,  $userEq.Fs44.Bank4.Stage3.a2,  $userEq.Fs44.Bank4.Stage3.a1,
                $userEq.Fs44.Bank4.Stage4.b2,  $userEq.Fs44.Bank4.Stage4.b1,  $userEq.Fs44.Bank4.Stage4.b0,  $userEq.Fs44.Bank4.Stage4.a2,  $userEq.Fs44.Bank4.Stage4.a1,
                $userEq.Fs44.Bank4.Stage5.b2,  $userEq.Fs44.Bank4.Stage5.b1,  $userEq.Fs44.Bank4.Stage5.b0,  $userEq.Fs44.Bank4.Stage5.a2,  $userEq.Fs44.Bank4.Stage5.a1,
                $userEq.Fs44.Bank4.Stage1.scale, $userEq.Fs44.Bank4.Stage2.scale, $userEq.Fs44.Bank4.Stage3.scale, $userEq.Fs44.Bank4.Stage4.scale, $userEq.Fs44.Bank4.Stage5.scale;
            .VAR UserEqCoefs5[3+6*MAX_NUM_USER_EQ_STAGES] =             // 44 kHz bank 5
                $userEq.Fs44.Bank5.NumBands,
                $userEq.Fs44.Bank5.GainExp,
                $userEq.Fs44.Bank5.GainMant,
                $userEq.Fs44.Bank5.Stage1.b2,  $userEq.Fs44.Bank5.Stage1.b1,  $userEq.Fs44.Bank5.Stage1.b0,  $userEq.Fs44.Bank5.Stage1.a2,  $userEq.Fs44.Bank5.Stage1.a1,
                $userEq.Fs44.Bank5.Stage2.b2,  $userEq.Fs44.Bank5.Stage2.b1,  $userEq.Fs44.Bank5.Stage2.b0,  $userEq.Fs44.Bank5.Stage2.a2,  $userEq.Fs44.Bank5.Stage2.a1,
                $userEq.Fs44.Bank5.Stage3.b2,  $userEq.Fs44.Bank5.Stage3.b1,  $userEq.Fs44.Bank5.Stage3.b0,  $userEq.Fs44.Bank5.Stage3.a2,  $userEq.Fs44.Bank5.Stage3.a1,
                $userEq.Fs44.Bank5.Stage4.b2,  $userEq.Fs44.Bank5.Stage4.b1,  $userEq.Fs44.Bank5.Stage4.b0,  $userEq.Fs44.Bank5.Stage4.a2,  $userEq.Fs44.Bank5.Stage4.a1,
                $userEq.Fs44.Bank5.Stage5.b2,  $userEq.Fs44.Bank5.Stage5.b1,  $userEq.Fs44.Bank5.Stage5.b0,  $userEq.Fs44.Bank5.Stage5.a2,  $userEq.Fs44.Bank5.Stage5.a1,
                $userEq.Fs44.Bank5.Stage1.scale, $userEq.Fs44.Bank5.Stage2.scale, $userEq.Fs44.Bank5.Stage3.scale, $userEq.Fs44.Bank5.Stage4.scale, $userEq.Fs44.Bank5.Stage5.scale;
            .VAR UserEqCoefs6[3+6*MAX_NUM_USER_EQ_STAGES] =             // 44 kHz bank 6
                $userEq.Fs44.Bank6.NumBands,
                $userEq.Fs44.Bank6.GainExp,
                $userEq.Fs44.Bank6.GainMant,
                $userEq.Fs44.Bank6.Stage1.b2,  $userEq.Fs44.Bank6.Stage1.b1,  $userEq.Fs44.Bank6.Stage1.b0,  $userEq.Fs44.Bank6.Stage1.a2,  $userEq.Fs44.Bank6.Stage1.a1,
                $userEq.Fs44.Bank6.Stage2.b2,  $userEq.Fs44.Bank6.Stage2.b1,  $userEq.Fs44.Bank6.Stage2.b0,  $userEq.Fs44.Bank6.Stage2.a2,  $userEq.Fs44.Bank6.Stage2.a1,
                $userEq.Fs44.Bank6.Stage3.b2,  $userEq.Fs44.Bank6.Stage3.b1,  $userEq.Fs44.Bank6.Stage3.b0,  $userEq.Fs44.Bank6.Stage3.a2,  $userEq.Fs44.Bank6.Stage3.a1,
                $userEq.Fs44.Bank6.Stage4.b2,  $userEq.Fs44.Bank6.Stage4.b1,  $userEq.Fs44.Bank6.Stage4.b0,  $userEq.Fs44.Bank6.Stage4.a2,  $userEq.Fs44.Bank6.Stage4.a1,
                $userEq.Fs44.Bank6.Stage5.b2,  $userEq.Fs44.Bank6.Stage5.b1,  $userEq.Fs44.Bank6.Stage5.b0,  $userEq.Fs44.Bank6.Stage5.a2,  $userEq.Fs44.Bank6.Stage5.a1,
                $userEq.Fs44.Bank6.Stage1.scale, $userEq.Fs44.Bank6.Stage2.scale, $userEq.Fs44.Bank6.Stage3.scale, $userEq.Fs44.Bank6.Stage4.scale, $userEq.Fs44.Bank6.Stage5.scale;

            .VAR UserEqCoefs7[3+6*MAX_NUM_USER_EQ_STAGES] =             // 48 kHz bank 1 (7)
                $userEq.Fs48.Bank1.NumBands,
                $userEq.Fs48.Bank1.GainExp,
                $userEq.Fs48.Bank1.GainMant,
                $userEq.Fs48.Bank1.Stage1.b2,  $userEq.Fs48.Bank1.Stage1.b1,  $userEq.Fs48.Bank1.Stage1.b0,  $userEq.Fs48.Bank1.Stage1.a2,  $userEq.Fs48.Bank1.Stage1.a1,
                $userEq.Fs48.Bank1.Stage2.b2,  $userEq.Fs48.Bank1.Stage2.b1,  $userEq.Fs48.Bank1.Stage2.b0,  $userEq.Fs48.Bank1.Stage2.a2,  $userEq.Fs48.Bank1.Stage2.a1,
                $userEq.Fs48.Bank1.Stage3.b2,  $userEq.Fs48.Bank1.Stage3.b1,  $userEq.Fs48.Bank1.Stage3.b0,  $userEq.Fs48.Bank1.Stage3.a2,  $userEq.Fs48.Bank1.Stage3.a1,
                $userEq.Fs48.Bank1.Stage4.b2,  $userEq.Fs48.Bank1.Stage4.b1,  $userEq.Fs48.Bank1.Stage4.b0,  $userEq.Fs48.Bank1.Stage4.a2,  $userEq.Fs48.Bank1.Stage4.a1,
                $userEq.Fs48.Bank1.Stage5.b2,  $userEq.Fs48.Bank1.Stage5.b1,  $userEq.Fs48.Bank1.Stage5.b0,  $userEq.Fs48.Bank1.Stage5.a2,  $userEq.Fs48.Bank1.Stage5.a1,
                $userEq.Fs48.Bank1.Stage1.scale, $userEq.Fs48.Bank1.Stage2.scale, $userEq.Fs48.Bank1.Stage3.scale, $userEq.Fs48.Bank1.Stage4.scale, $userEq.Fs48.Bank1.Stage5.scale;
            .VAR SpkrEqCoefs8[3+6*MAX_NUM_USER_EQ_STAGES] =             // 48 kHz bank 2 (8)
                $userEq.Fs48.Bank2.NumBands,
                $userEq.Fs48.Bank2.GainExp,
                $userEq.Fs48.Bank2.GainMant,
                $userEq.Fs48.Bank2.Stage1.b2,  $userEq.Fs48.Bank2.Stage1.b1,  $userEq.Fs48.Bank2.Stage1.b0,  $userEq.Fs48.Bank2.Stage1.a2,  $userEq.Fs48.Bank2.Stage1.a1,
                $userEq.Fs48.Bank2.Stage2.b2,  $userEq.Fs48.Bank2.Stage2.b1,  $userEq.Fs48.Bank2.Stage2.b0,  $userEq.Fs48.Bank2.Stage2.a2,  $userEq.Fs48.Bank2.Stage2.a1,
                $userEq.Fs48.Bank2.Stage3.b2,  $userEq.Fs48.Bank2.Stage3.b1,  $userEq.Fs48.Bank2.Stage3.b0,  $userEq.Fs48.Bank2.Stage3.a2,  $userEq.Fs48.Bank2.Stage3.a1,
                $userEq.Fs48.Bank2.Stage4.b2,  $userEq.Fs48.Bank2.Stage4.b1,  $userEq.Fs48.Bank2.Stage4.b0,  $userEq.Fs48.Bank2.Stage4.a2,  $userEq.Fs48.Bank2.Stage4.a1,
                $userEq.Fs48.Bank2.Stage5.b2,  $userEq.Fs48.Bank2.Stage5.b1,  $userEq.Fs48.Bank2.Stage5.b0,  $userEq.Fs48.Bank2.Stage5.a2,  $userEq.Fs48.Bank2.Stage5.a1,
                $userEq.Fs48.Bank2.Stage1.scale, $userEq.Fs48.Bank2.Stage2.scale, $userEq.Fs48.Bank2.Stage3.scale, $userEq.Fs48.Bank2.Stage4.scale, $userEq.Fs48.Bank2.Stage5.scale;
            .VAR UserEqCoefs9[3+6*MAX_NUM_USER_EQ_STAGES] =             // 48 kHz bank 3 (9)
                $userEq.Fs48.Bank3.NumBands,
                $userEq.Fs48.Bank3.GainExp,
                $userEq.Fs48.Bank3.GainMant,
                $userEq.Fs48.Bank3.Stage1.b2,  $userEq.Fs48.Bank3.Stage1.b1,  $userEq.Fs48.Bank3.Stage1.b0,  $userEq.Fs48.Bank3.Stage1.a2,  $userEq.Fs48.Bank3.Stage1.a1,
                $userEq.Fs48.Bank3.Stage2.b2,  $userEq.Fs48.Bank3.Stage2.b1,  $userEq.Fs48.Bank3.Stage2.b0,  $userEq.Fs48.Bank3.Stage2.a2,  $userEq.Fs48.Bank3.Stage2.a1,
                $userEq.Fs48.Bank3.Stage3.b2,  $userEq.Fs48.Bank3.Stage3.b1,  $userEq.Fs48.Bank3.Stage3.b0,  $userEq.Fs48.Bank3.Stage3.a2,  $userEq.Fs48.Bank3.Stage3.a1,
                $userEq.Fs48.Bank3.Stage4.b2,  $userEq.Fs48.Bank3.Stage4.b1,  $userEq.Fs48.Bank3.Stage4.b0,  $userEq.Fs48.Bank3.Stage4.a2,  $userEq.Fs48.Bank3.Stage4.a1,
                $userEq.Fs48.Bank3.Stage5.b2,  $userEq.Fs48.Bank3.Stage5.b1,  $userEq.Fs48.Bank3.Stage5.b0,  $userEq.Fs48.Bank3.Stage5.a2,  $userEq.Fs48.Bank3.Stage5.a1,
                $userEq.Fs48.Bank3.Stage1.scale, $userEq.Fs48.Bank3.Stage2.scale, $userEq.Fs48.Bank3.Stage3.scale, $userEq.Fs48.Bank3.Stage4.scale, $userEq.Fs48.Bank3.Stage5.scale;
            .VAR UserEqCoefs10[3+6*MAX_NUM_USER_EQ_STAGES] =             // 48 kHz bank 4 (10)
                $userEq.Fs48.Bank4.NumBands,
                $userEq.Fs48.Bank4.GainExp,
                $userEq.Fs48.Bank4.GainMant,
                $userEq.Fs48.Bank4.Stage1.b2,  $userEq.Fs48.Bank4.Stage1.b1,  $userEq.Fs48.Bank4.Stage1.b0,  $userEq.Fs48.Bank4.Stage1.a2,  $userEq.Fs48.Bank4.Stage1.a1,
                $userEq.Fs48.Bank4.Stage2.b2,  $userEq.Fs48.Bank4.Stage2.b1,  $userEq.Fs48.Bank4.Stage2.b0,  $userEq.Fs48.Bank4.Stage2.a2,  $userEq.Fs48.Bank4.Stage2.a1,
                $userEq.Fs48.Bank4.Stage3.b2,  $userEq.Fs48.Bank4.Stage3.b1,  $userEq.Fs48.Bank4.Stage3.b0,  $userEq.Fs48.Bank4.Stage3.a2,  $userEq.Fs48.Bank4.Stage3.a1,
                $userEq.Fs48.Bank4.Stage4.b2,  $userEq.Fs48.Bank4.Stage4.b1,  $userEq.Fs48.Bank4.Stage4.b0,  $userEq.Fs48.Bank4.Stage4.a2,  $userEq.Fs48.Bank4.Stage4.a1,
                $userEq.Fs48.Bank4.Stage5.b2,  $userEq.Fs48.Bank4.Stage5.b1,  $userEq.Fs48.Bank4.Stage5.b0,  $userEq.Fs48.Bank4.Stage5.a2,  $userEq.Fs48.Bank4.Stage5.a1,
                $userEq.Fs48.Bank4.Stage1.scale, $userEq.Fs48.Bank4.Stage2.scale, $userEq.Fs48.Bank4.Stage3.scale, $userEq.Fs48.Bank4.Stage4.scale, $userEq.Fs48.Bank4.Stage5.scale;
            .VAR UserEqCoefs11[3+6*MAX_NUM_USER_EQ_STAGES] =             // 48 kHz bank 5 (11)
                $userEq.Fs48.Bank5.NumBands,
                $userEq.Fs48.Bank5.GainExp,
                $userEq.Fs48.Bank5.GainMant,
                $userEq.Fs48.Bank5.Stage1.b2,  $userEq.Fs48.Bank5.Stage1.b1,  $userEq.Fs48.Bank5.Stage1.b0,  $userEq.Fs48.Bank5.Stage1.a2,  $userEq.Fs48.Bank5.Stage1.a1,
                $userEq.Fs48.Bank5.Stage2.b2,  $userEq.Fs48.Bank5.Stage2.b1,  $userEq.Fs48.Bank5.Stage2.b0,  $userEq.Fs48.Bank5.Stage2.a2,  $userEq.Fs48.Bank5.Stage2.a1,
                $userEq.Fs48.Bank5.Stage3.b2,  $userEq.Fs48.Bank5.Stage3.b1,  $userEq.Fs48.Bank5.Stage3.b0,  $userEq.Fs48.Bank5.Stage3.a2,  $userEq.Fs48.Bank5.Stage3.a1,
                $userEq.Fs48.Bank5.Stage4.b2,  $userEq.Fs48.Bank5.Stage4.b1,  $userEq.Fs48.Bank5.Stage4.b0,  $userEq.Fs48.Bank5.Stage4.a2,  $userEq.Fs48.Bank5.Stage4.a1,
                $userEq.Fs48.Bank5.Stage5.b2,  $userEq.Fs48.Bank5.Stage5.b1,  $userEq.Fs48.Bank5.Stage5.b0,  $userEq.Fs48.Bank5.Stage5.a2,  $userEq.Fs48.Bank5.Stage5.a1,
                $userEq.Fs48.Bank5.Stage1.scale, $userEq.Fs48.Bank5.Stage2.scale, $userEq.Fs48.Bank5.Stage3.scale, $userEq.Fs48.Bank5.Stage4.scale, $userEq.Fs48.Bank5.Stage5.scale;
            .VAR UserEqCoefs12[3+6*MAX_NUM_USER_EQ_STAGES] =             // 48 kHz bank 6 (12)
                $userEq.Fs48.Bank6.NumBands,
                $userEq.Fs48.Bank6.GainExp,
                $userEq.Fs48.Bank6.GainMant,
                $userEq.Fs48.Bank6.Stage1.b2,  $userEq.Fs48.Bank6.Stage1.b1,  $userEq.Fs48.Bank6.Stage1.b0,  $userEq.Fs48.Bank6.Stage1.a2,  $userEq.Fs48.Bank6.Stage1.a1,
                $userEq.Fs48.Bank6.Stage2.b2,  $userEq.Fs48.Bank6.Stage2.b1,  $userEq.Fs48.Bank6.Stage2.b0,  $userEq.Fs48.Bank6.Stage2.a2,  $userEq.Fs48.Bank6.Stage2.a1,
                $userEq.Fs48.Bank6.Stage3.b2,  $userEq.Fs48.Bank6.Stage3.b1,  $userEq.Fs48.Bank6.Stage3.b0,  $userEq.Fs48.Bank6.Stage3.a2,  $userEq.Fs48.Bank6.Stage3.a1,
                $userEq.Fs48.Bank6.Stage4.b2,  $userEq.Fs48.Bank6.Stage4.b1,  $userEq.Fs48.Bank6.Stage4.b0,  $userEq.Fs48.Bank6.Stage4.a2,  $userEq.Fs48.Bank6.Stage4.a1,
                $userEq.Fs48.Bank6.Stage5.b2,  $userEq.Fs48.Bank6.Stage5.b1,  $userEq.Fs48.Bank6.Stage5.b0,  $userEq.Fs48.Bank6.Stage5.a2,  $userEq.Fs48.Bank6.Stage5.a1,
                $userEq.Fs48.Bank6.Stage1.scale, $userEq.Fs48.Bank6.Stage2.scale, $userEq.Fs48.Bank6.Stage3.scale, $userEq.Fs48.Bank6.Stage4.scale, $userEq.Fs48.Bank6.Stage5.scale;
        .ENDBLOCK;
    #else
        .VAR UserEqCoefsA[33] =
            0x000000,                                               // [0] = config (no eq bands)
            0x000001,                                               // [1] = gain exponent
            0x400000,                                               // [2] = gain mantissa
            0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [ 3... 7] = stage 1 (b2,b1,b0,a2,a1)
            0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [ 8...12] = stage 2
            0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [13...17] = stage 3
            0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [18...22] = stage 4
            0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [23...27] = stage 5
            0x000001, 0x000001, 0x000001, 0x000001, 0x000001;       // [28...32] = scales

        .VAR UserEqCoefsB[33] =
            0x000000,                                               // [0] = config (no eq bands)
            0x000001,                                               // [1] = gain exponent
            0x400000,                                               // [2] = gain mantissa
            0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [ 3... 7] = stage 1 (b2,b1,b0,a2,a1)
            0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [ 8...12] = stage 2
            0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [13...17] = stage 3
            0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [18...22] = stage 4
            0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [23...27] = stage 5
            0x000001, 0x000001, 0x000001, 0x000001, 0x000001;       // [28...32] = scales
   #endif // USE_PRECALCULATED_USER_COEFS

    .VAR UserEqDefnTable[$user_eq.DEFINITION_TABLE_SIZE] =
        MAX_NUM_USER_EQ_BANKS,
        MAX_NUM_USER_EQ_STAGES,
        &user_eq_left_dm2,
        &user_eq_right_dm2,
        &UserEqCoefsA,
        &UserEqCoefsB;

     // 6 configs
    .VAR/DM2 user_eq_bank_select[1 + MAX_NUM_USER_EQ_BANKS] =
        0,  // index 0 = flat response (no eq)
        &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_USER_EQ1_NUM_BANDS,
        &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_USER_EQ2_NUM_BANDS,
        &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_USER_EQ3_NUM_BANDS,
        &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_USER_EQ4_NUM_BANDS,
        &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_USER_EQ5_NUM_BANDS,
        &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_USER_EQ6_NUM_BANDS;

#endif


#if uses_STEREO_ENHANCEMENT
    // LoFreqWidth =     7.00
    // HiFreqWidth =     2.50
    // XvrFreq     =   600.00
    // MonoWidth   =     1.00
    .VAR MeloD_Expansion_coefficients[$MeloD_Expansion.COEFS_STRUC_SIZE] =
        0xB1AA81, 0x571F3E, 0xC3ECED,   // -1.22396840/2,  1.36128197/2, -0.93866428/2,
        0x400000, 0x84DA2F, 0x3B3194,   // 0.5, -1.92418325/2,  0.92490121/2,
        0x400000, 0xD53FA4, 0xFEA000,   // 0.5, -0.66799074/2, -0.02148449/2,
        0x400000, 0x81543A, 0x3EB7E0,   // 0.5, -1.97923440/2,  0.97997290/2,
        0x400000, 0xBB3B4C, 0x24BC7F;   // 0.5, -1.07450601/2,  0.57400504/2;

    .VAR MeloD_Expansion_filter_data[$MeloD_Expansion.FILTER_DATA_SIZE];

    .VAR /dm2 MeloD_Expansion_struct[$MeloD_Expansion.STRUC_SIZE] =
        &stream_map_left_in,
        &stream_map_right_in,
        &stream_map_left_in,
        &stream_map_right_in,
        &$M.system_config.data.CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_CONFIG,   // PTR TO CONFIG WORD WHICH CONTAINS BYPASS BIT
        $M.MUSIC_MANAGER.CONFIG.SPATIAL_ENHANCEMENT_BYPASS,
        0,      // initial processing state is off
        &MeloD_Expansion_filter_data,
        &MeloD_Expansion_coefficients;
#endif


#if uses_SPEAKER_CROSSOVER

    // Internal bass buffer. Doesn't need to be circular
    .VAR spkr_ctrl_bass_buffer[$music_example.NUM_SAMPLES_PER_FRAME*2];

    .VAR spkr_ctrl_init_struct[$spkr_ctrl_system.INIT_STRUCT_SIZE] =
        &spkr_ctrl_struct,                      // INIT_RUNTIME_STRUCT_PTR
        &spkr_ctrl_pri_eq_defn_table,           // INIT_PRI_EQ_DEFN_PTR
        &spkr_ctrl_pri_eq_bank_select,          // INIT_PRI_EQ_BANK_SELECT_PTR
        0,                                      // INIT_SEC_EQ_DEFN_PTR [config]
        0,                                      // INIT_SEC_EQ_BANK_SELECT_PTR [config]
        &spkr_ctrl_bass_eq_defn_table,          // INIT_BASS_EQ_DEFN_PTR
        &spkr_ctrl_bass_eq_bank_select;         // INIT_BASS_EQ_BANK_SELECT_PTR

    .VAR spkr_ctrl_struct[$spkr_ctrl_system.STRUCT_SIZE] =
        &stream_map_left_in,                    // LEFT_INPUT_PTR
        &stream_map_right_in,                   // RIGHT_INPUT_PTR
        0,                                      // LEFT_PRI_OUTPUT_PTR [config]
        0,                                      // RIGHT_PRI_OUTPUT_PTR [config]
        0,                                      // LEFT_SEC_OUTPUT_PTR [config]
        0,                                      // RIGHT_SEC_OUTPUT_PTR [config]

#if defined(SPDIF_ENABLE) && defined(AC3_ENABLE)
        &stream_map_lfe_in,                     // LFE_INPUT_PTR
#else
        0,                                      // LFE_INPUT_PTR
#endif // defined(SPDIF_ENABLE) && defined(AC3_ENABLE)
        0,                                      // SUB_OUTPUT_PTR
        spkr_ctrl_bass_buffer,                  // BASS_BUFFER_PTR
        &$M.system_config.data.CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_CONFIG,   // BYPASS_WORD_PTR
        $M.MUSIC_MANAGER.CONFIG.BASS_MANAGER_BYPASS,    // BYPASS_BIT_MASK_FIELD
        &spkr_ctrl_coefs;                       // COEFS_PTR

    .VAR spkr_ctrl_coefs[$spkr_ctrl_system.COEF_STRUCT_SIZE] =
        1,                                      // COEF_CONFIG (bank setting)[config]
        &spkr_ctrl_left_pri_eq_dm,              // COEF_EQ_L_PRI_PTR
        &spkr_ctrl_right_pri_eq_dm,             // COEF_EQ_R_PRI_PTR
        0,                                      // COEF_EQ_L_SEC_PTR [config]
        0,                                      // COEF_EQ_R_SEC_PTR [config]
        &spkr_ctrl_bass_eq_dm,                  // COEF_EQ_BASS_PTR
        &spkr_ctrl_gain_coefs_A,                // COEF_GAIN_ACTIVE_PTR
        &spkr_ctrl_gain_coefs_A,                // COEF_GAIN_A_PTR
        &spkr_ctrl_gain_coefs_B;                // COEF_GAIN_B_PTR

    .VAR spkr_ctrl_gain_coefs_A[$spkr_ctrl_system.COEF_GAIN.STRUCT_SIZE] =
        0x200000, 0x000000, 0x000000,           // L_PRI matrix (L,R,B)
        0x000000, 0x200000, 0x000000,           // R_PRI matrix (L,R,B)

        0x200000, 0x000000, 0x000000,           // L_SEC matrix (L,R,B)
        0x000000, 0x200000, 0x000000,           // R_SEC matrix (L,R,B)

        0x000000, 0x000000, 0x000000,           // BASS matrix (L,R,LFE)
        0x000000;                               // COEF_GAIN.GAIN_SUB

    .VAR spkr_ctrl_gain_coefs_B[$spkr_ctrl_system.COEF_GAIN.STRUCT_SIZE] =
        0x200000, 0x000000, 0x000000,           // L_PRI matrix (L,R,B)
        0x000000, 0x200000, 0x000000,           // R_PRI matrix (L,R,B)

        0x200000, 0x000000, 0x000000,           // L_SEC matrix (L,R,B)
        0x000000, 0x200000, 0x000000,           // R_SEC matrix (L,R,B)

        0x100000, 0x100000, 0x000000,           // BASS matrix (L,R,LFE)
        0x200000;                               // COEF_GAIN.GAIN_SUB

    // define EQ for left and right primary outputs

    .VAR/DM2 spkr_ctrl_left_pri_eq_dm[HQ_PEQ_OBJECT_SIZE(MAX_NUM_SPKR_CTRL_PRI_EQ_STAGES)] =
        &stream_map_primary_left_out,           // LEFT_PRI_OUTPUT_PTR
        &stream_map_primary_left_out,           // RIGHT_PRI_OUTPUT_PTR
        MAX_NUM_SPKR_CTRL_PRI_EQ_STAGES,        // MAX_STAGES_FIELD
        &spkr_ctrl_pri_eq_coefs_A,              // PARAM_PTR_FIELD
        0 ...;

    .VAR/DM2 spkr_ctrl_right_pri_eq_dm[HQ_PEQ_OBJECT_SIZE(MAX_NUM_SPKR_CTRL_PRI_EQ_STAGES)] =
        &stream_map_primary_right_out,          // LEFT_PRI_OUTPUT_PTR
        &stream_map_primary_right_out,          // RIGHT_PRI_OUTPUT_PTR
        MAX_NUM_SPKR_CTRL_PRI_EQ_STAGES,        // MAX_STAGES_FIELD
        &spkr_ctrl_pri_eq_coefs_A,              // PARAM_PTR_FIELD
        0 ...;

    .VAR spkr_ctrl_pri_eq_coefs_A[3+6*MAX_NUM_SPKR_CTRL_PRI_EQ_STAGES] =
        0x000000,                                               // [0] = config (no eq bands)
        0x000001,                                               // [1] = gain exponent
        0x400000,                                               // [2] = gain mantissa
        0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [ 3... 7] = stage 1 (b2,b1,b0,a2,a1)
        0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [ 8...12] = stage 2
        0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [13...17] = stage 3
        0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [18...22] = stage 4
        0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [23...27] = stage 5
        0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [28...32] = stage 6
        0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [33...37] = stage 7
        0x000001, 0x000001, 0x000001, 0x000001,
        0x000001, 0x000001, 0x000001;                           // [38...44] = scales

    .VAR spkr_ctrl_pri_eq_coefs_B[3+6*MAX_NUM_SPKR_CTRL_PRI_EQ_STAGES] =
        0x000000,                                               // [0] = config (no eq bands)
        0x000001,                                               // [1] = gain exponent
        0x400000,                                               // [2] = gain mantissa
        0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [ 3... 7] = stage 1 (b2,b1,b0,a2,a1)
        0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [ 8...12] = stage 2
        0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [13...17] = stage 3
        0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [18...22] = stage 4
        0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [23...27] = stage 5
        0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [28...32] = stage 6
        0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [33...37] = stage 7
        0x000001, 0x000001, 0x000001, 0x000001,
        0x000001, 0x000001, 0x000001;                           // [38...44] = scales

    .VAR spkr_ctrl_pri_eq_defn_table[$user_eq.DEFINITION_TABLE_SIZE] =
        MAX_NUM_SPKR_CTRL_PRI_EQ_BANKS,
        MAX_NUM_SPKR_CTRL_PRI_EQ_STAGES,
        &spkr_ctrl_left_pri_eq_dm,
        &spkr_ctrl_right_pri_eq_dm,
        &spkr_ctrl_pri_eq_coefs_A,
        &spkr_ctrl_pri_eq_coefs_B;

    .VAR/DM2 spkr_ctrl_pri_eq_bank_select[1 + MAX_NUM_SPKR_CTRL_PRI_EQ_BANKS] =
        0,  // index 0 = flat response (no eq)
        &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SPKR_CTRL_PRI_EQ1_NUM_BANDS,
        &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SPKR_CTRL_PRI_EQ2_NUM_BANDS;

    // define EQ for left and right secondary outputs
    .VAR/DM2 spkr_ctrl_left_sec_eq_dm[HQ_PEQ_OBJECT_SIZE(MAX_NUM_SPKR_CTRL_SEC_EQ_STAGES)] =
        &stream_map_secondary_left_out,        // LEFT_PRI_OUTPUT_PTR
        &stream_map_secondary_left_out,        // RIGHT_PRI_OUTPUT_PTR
        MAX_NUM_SPKR_CTRL_SEC_EQ_STAGES,       // MAX_STAGES_FIELD
        &spkr_ctrl_sec_eq_coefs_A,             // PARAM_PTR_FIELD
        0 ...;

    .VAR/DM2 spkr_ctrl_right_sec_eq_dm[HQ_PEQ_OBJECT_SIZE(MAX_NUM_SPKR_CTRL_SEC_EQ_STAGES)] =
        &stream_map_secondary_right_out,        // LEFT_PRI_OUTPUT_PTR
        &stream_map_secondary_right_out,        // RIGHT_PRI_OUTPUT_PTR
        MAX_NUM_SPKR_CTRL_SEC_EQ_STAGES,        // MAX_STAGES_FIELD
        &spkr_ctrl_sec_eq_coefs_A,              // PARAM_PTR_FIELD
        0 ...;

    .VAR spkr_ctrl_sec_eq_coefs_A[3+6*MAX_NUM_SPKR_CTRL_SEC_EQ_STAGES] =
        0x000000,                                               // [0] = config (no eq bands)
        0x000001,                                               // [1] = gain exponent
        0x400000,                                               // [2] = gain mantissa
        0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [ 3... 7] = stage 1 (b2,b1,b0,a2,a1)
        0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [ 8...12] = stage 2
        0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [13...17] = stage 3
        0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [18...22] = stage 4
        0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [23...27] = stage 5
        0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [28...32] = stage 6
        0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [33...37] = stage 7
        0x000001, 0x000001, 0x000001, 0x000001,
        0x000001, 0x000001, 0x000001;                           // [38...44] = scales

    .VAR spkr_ctrl_sec_eq_coefs_B[3+6*MAX_NUM_SPKR_CTRL_SEC_EQ_STAGES] =
        0x000000,                                               // [0] = config (no eq bands)
        0x000001,                                               // [1] = gain exponent
        0x400000,                                               // [2] = gain mantissa
        0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [ 3... 7] = stage 1 (b2,b1,b0,a2,a1)
        0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [ 8...12] = stage 2
        0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [13...17] = stage 3
        0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [18...22] = stage 4
        0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [23...27] = stage 5
        0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [28...32] = stage 6
        0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [33...37] = stage 7
        0x000001, 0x000001, 0x000001, 0x000001,
        0x000001, 0x000001, 0x000001;                           // [38...44] = scales

    .VAR spkr_ctrl_sec_eq_defn_table[$user_eq.DEFINITION_TABLE_SIZE] =
        MAX_NUM_SPKR_CTRL_SEC_EQ_BANKS,
        MAX_NUM_SPKR_CTRL_SEC_EQ_STAGES,
        &spkr_ctrl_left_sec_eq_dm,
        &spkr_ctrl_right_sec_eq_dm,
        &spkr_ctrl_sec_eq_coefs_A,
        &spkr_ctrl_sec_eq_coefs_B;

    .VAR/DM2 spkr_ctrl_sec_eq_bank_select[1 + MAX_NUM_SPKR_CTRL_SEC_EQ_BANKS] =
        0,  // index 0 = flat response (no eq)
        &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SPKR_CTRL_SEC_EQ1_NUM_BANDS,
        &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SPKR_CTRL_SEC_EQ2_NUM_BANDS;

    // define bass/sub EQ

    .VAR/DM2 spkr_ctrl_bass_eq_dm[HQ_PEQ_OBJECT_SIZE(MAX_NUM_SPKR_CTRL_BASS_EQ_STAGES)] =
        &spkr_ctrl_bass_buffer,                 // PTR_INPUT_DATA_BUFF_FIELD
        &spkr_ctrl_bass_buffer,                 // PTR_OUTPUT_DATA_BUFF_FIELD
        MAX_NUM_SPKR_CTRL_BASS_EQ_STAGES,       // MAX_STAGES_FIELD
        &spkr_ctrl_bass_eq_coefs_A,             // PARAM_PTR_FIELD
        0 ...;

    .VAR spkr_ctrl_bass_eq_coefs_A[3+6*MAX_NUM_SPKR_CTRL_BASS_EQ_STAGES] =
        0x000000,                                               // [0] = config (no eq bands)
        0x000001,                                               // [1] = gain exponent
        0x400000,                                               // [2] = gain mantissa
        0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [ 3... 7] = stage 1 (b2,b1,b0,a2,a1)
        0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [ 8...12] = stage 2
        0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [13...17] = stage 3
        0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [18...22] = stage 4
        0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [23...27] = stage 5
        0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [28...32] = stage 6
        0x000001, 0x000001, 0x000001,
        0x000001, 0x000001, 0x000001;                           // [33...38] = scales

    .VAR spkr_ctrl_bass_eq_coefs_B[3+6*MAX_NUM_SPKR_CTRL_BASS_EQ_STAGES] =
        0x000000,                                               // [0] = config (no eq bands)
        0x000001,                                               // [1] = gain exponent
        0x400000,                                               // [2] = gain mantissa
        0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [ 3... 7] = stage 1 (b2,b1,b0,a2,a1)
        0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [ 8...12] = stage 2
        0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [13...17] = stage 3
        0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [18...22] = stage 4
        0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [23...27] = stage 5
        0x000000, 0x000000, 0x400000, 0x000000, 0x000000,       // [28...32] = stage 6
        0x000001, 0x000001, 0x000001,
        0x000001, 0x000001, 0x000001;                           // [33...38] = scales

    .VAR spkr_ctrl_bass_eq_defn_table[$user_eq.DEFINITION_TABLE_SIZE] =
        MAX_NUM_SPKR_CTRL_BASS_EQ_BANKS,
        MAX_NUM_SPKR_CTRL_BASS_EQ_STAGES,
        &spkr_ctrl_bass_eq_dm,
        0,
        &spkr_ctrl_bass_eq_coefs_A,
        &spkr_ctrl_bass_eq_coefs_B;

    .VAR/DM2 spkr_ctrl_bass_eq_bank_select[1 + MAX_NUM_SPKR_CTRL_BASS_EQ_BANKS] =
        0,  // index 0 = flat response (no eq)
        &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SPKR_CTRL_BASS_EQ1_NUM_BANDS,
        &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SPKR_CTRL_BASS_EQ2_NUM_BANDS;
#endif // uses_SPEAKER_CROSSOVER

// aux downmix and stereo copy objects
    .VAR  uses_aux_out = 0;
    .VAR  aux_downmix = 0;

    .VAR aux_mix_obj[$M.audio_proc.stream_mixer.STRUC_SIZE] =
        &stream_map_left_in,                    // INPUT_CH1_PTR_BUFFER_FIELD
        &stream_map_right_in,                   // INPUT_CH2_PTR_BUFFER_FIELD
        &stream_map_aux_left_out,               // OUTPUT_CH1_PTR_BUFFER_FIELD
        &HalfValue,                             // OFFSET_PTR_CH1_MANTISSA
        &HalfValue,                             // OFFSET_PTR_CH2_MANTISSA
        &ZeroValue;                             // OFFSET_PTR_EXPONENT

    .VAR aux_stereo_copy_obj[$M.audio_proc.stereo_copy.STRUC_SIZE] =
        &stream_map_left_in,                    // INPUT_CH1_PTR_BUFFER_FIELD
        &stream_map_right_in,                   // INPUT_CH2_PTR_BUFFER_FIELD
        &stream_map_aux_left_out,               // OUTPUT_CH1_PTR_BUFFER_FIELD
        &stream_map_aux_right_out;              // OUTPUT_CH2_PTR_BUFFER_FIELD

// aux volume and limit object
.BLOCK aux_stereo_volume_and_limit_block;
   .VAR aux_stereo_volume_and_limit_obj[$volume_and_limit.STRUC_SIZE] =
        0x000000,                                          //OFFSET_CONTROL_WORD_FIELD
        $M.MUSIC_MANAGER.CONFIG.VOLUME_LIMITER_BYPASS, //OFFSET_BYPASS_BIT_FIELD
        2,                                                 //NROF_CHANNELS_FIELD
        &$current_dac_sampling_rate,                       //SAMPLE_RATE_PTR_FIELD
        $music_example.MUTE_MASTER_VOLUME,                 //MASTER_VOLUME_FIELD
        $music_example.LIMIT_THRESHOLD,                    //LIMIT_THRESHOLD_FIELD
        $music_example.LIMIT_THRESHOLD_LINEAR,             //LIMIT_THRESHOLD_LINEAR_FIELD
        $music_example.LIMIT_RATIO,                        //LIMIT_RATIO_FIELD_FIELD
        $music_example.RAMP_FACTOR,                        //RAMP FACTOR FIELD
        0 ...;

   .VAR aux_left_channel_vol_struc[$volume_and_limit.channel.STRUC_SIZE] =
        &stream_map_aux_left_out,                 // INPUT_PTR_FIELD
        &stream_map_aux_left_out,                 // OUTPUT_PTR_FIELD
        $music_example.DEFAULT_TRIM_VOLUME,       // TRIM_VOLUME_FIELD
        0 ...;

   .VAR aux_right_channel_vol_struc[$volume_and_limit.channel.STRUC_SIZE] =
        &stream_map_aux_right_out,                 // INPUT_PTR_FIELD
        &stream_map_aux_right_out,                 // OUTPUT_PTR_FIELD
        $music_example.DEFAULT_TRIM_VOLUME,        // TRIM_VOLUME_FIELD
        0 ...;
.ENDBLOCK;

    .VAR uses_wired_sub = 0;

#if uses_WIRED_SUB_EQ
    .VAR/DM2 sub_eq_dm2[PEQ_OBJECT_SIZE(MAX_NUM_WIRED_SUB_EQ_STAGES)] =
        &stream_map_sub_out,                    // PTR_INPUT_DATA_BUFF_FIELD
        &stream_map_sub_out,                    // PTR_OUTPUT_DATA_BUFF_FIELD
        MAX_NUM_WIRED_SUB_EQ_STAGES,            // MAX_STAGES_FIELD
        &SubEqCoefsA,                           // PARAM_PTR_FIELD
        0 ...;

       // 44.1 kHz coefficients if not using coefficient calculation routines
    .VAR SubEqCoefsA[3+6*MAX_NUM_WIRED_SUB_EQ_STAGES] =
        $subEq.Fs44.NumBands,
        $subEq.Fs44.GainExp,
        $subEq.Fs44.GainMant,
        $subEq.Fs44.Stage1.b2,  $subEq.Fs44.Stage1.b1,  $subEq.Fs44.Stage1.b0,  $subEq.Fs44.Stage1.a2,  $subEq.Fs44.Stage1.a1,
        $subEq.Fs44.Stage2.b2,  $subEq.Fs44.Stage2.b1,  $subEq.Fs44.Stage2.b0,  $subEq.Fs44.Stage2.a2,  $subEq.Fs44.Stage2.a1,
        $subEq.Fs44.Stage3.b2,  $subEq.Fs44.Stage3.b1,  $subEq.Fs44.Stage3.b0,  $subEq.Fs44.Stage3.a2,  $subEq.Fs44.Stage3.a1,
              $subEq.Fs44.Stage1.scale, $subEq.Fs44.Stage2.scale, $subEq.Fs44.Stage3.scale;

       // 48 kHz coefficients if not using coefficient calculation routines
    .VAR SubEqCoefsB[3+6*MAX_NUM_WIRED_SUB_EQ_STAGES] =
        $subEq.Fs48.NumBands,
        $subEq.Fs48.GainExp,
        $subEq.Fs48.GainMant,
        $subEq.Fs48.Stage1.b2,  $subEq.Fs48.Stage1.b1,  $subEq.Fs48.Stage1.b0,  $subEq.Fs48.Stage1.a2,  $subEq.Fs48.Stage1.a1,
        $subEq.Fs48.Stage2.b2,  $subEq.Fs48.Stage2.b1,  $subEq.Fs48.Stage2.b0,  $subEq.Fs48.Stage2.a2,  $subEq.Fs48.Stage2.a1,
        $subEq.Fs48.Stage3.b2,  $subEq.Fs48.Stage3.b1,  $subEq.Fs48.Stage3.b0,  $subEq.Fs48.Stage3.a2,  $subEq.Fs48.Stage3.a1,
              $subEq.Fs48.Stage1.scale, $subEq.Fs48.Stage2.scale, $subEq.Fs48.Stage3.scale;

    .VAR WiredSubEqDefnTable[$user_eq.DEFINITION_TABLE_SIZE] =
        MAX_NUM_WIRED_SUB_EQ_BANKS,
        MAX_NUM_WIRED_SUB_EQ_STAGES,
        &sub_eq_dm2,
        0,
        &SubEqCoefsA,
        &SubEqCoefsB;

       // pointer to sub eq parameters
       // if zero, use the coefficients that are in the code above
       #if USE_PRECALCULATED_WIRED_SUB_COEFS
              #ifdef ROM
                     #error cannot use precalculated coefficients with ROM part
              #endif
              .var WiredSubEqParams = 0;
       #else
        .var WiredSubEqParams = &CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_WIRED_SUB_EQ_NUM_BANDS;
       #endif  // USE_PRECALCULATED_WIRED_SUB_COEFS
#endif

#if uses_WIRED_SUB_COMPANDER
    .VAR/DM $cmpd_subwoofer_gain;

    .VAR cmpd100_obj_wired_sub[$cmpd100.STRUC_SIZE] =
     64,                                                        // OFFSET_CONTROL_WORD                0
     $M.MUSIC_MANAGER.CONFIG.WIRED_SUBWOOFER_COMPANDER_BYPASS,  // OFFSET_BYPASS_BIT_MASK             1
     &stream_map_sub_out,                                       // OFFSET_INPUT_CH1_PTR               2
     &stream_map_sub_out,                                       // OFFSET_INPUT_CH2_PTR               3
     &stream_map_sub_out,                                       // OFFSET_OUTPUT_CH1_PTR              4
     &stream_map_sub_out,                                       // OFFSET_OUTPUT_CH2_PTR              5
     0x080000,                                                  // OFFSET_MAKEUP_GAIN q4.19           6
     &$cmpd_subwoofer_gain,                                     // OFFSET_GAIN_PTR q4.19              7
     0x800000,                                                  // OFFSET_NEG_ONE q.23                8
     0.0625,                                                    // OFFSET_POW2_NEG4 q.23              9
     0xF9B037,                                                  // OFFSET_EXPAND_THRESHOLD q.23       10
     0xFA0541,                                                  // OFFSET_LINEAR_THRESHOLD q.23       11
     0xFE56CB,                                                  // OFFSET_COMPRESS_THRESHOLD q.23     12
     0xFF8070,                                                  // OFFSET_LIMIT_THRESHOLD q.23        13
     0x100000,                                                  // OFFSET_INV_EXPAND_RATIO q4.19      14
     0x080000,                                                  // OFFSET_INV_LINEAR_RATIO q4.19      15
     0x015555,                                                  // OFFSET_INV_COMPRESS_RATIO q4.19    16
     0x00CCCD,                                                  // OFFSET_INV_LIMIT_RATIO q4.19       17
     0,                                                         // OFFSET_EXPAND_CONSTANT q4.19       18
     0,                                                         // OFFSET_LINEAR_CONSTANT q4.19       19
     0,                                                         // OFFSET_COMPRESS_CONSTANT q4.19     20
     4328332,                                                   // OFFSET_EXPAND_ATTACK_TIME          21
     200465,                                                    // OFFSET_EXPAND_DECAY_TIME           22
     4328332,                                                   // OFFSET_LINEAR_ATTACK_TIME          23
     60649,                                                     // OFFSET_LINEAR_DECAY_TIME           24
     6423342,                                                   // OFFSET_COMPRESS_ATTACK_TIME        25
     40482,                                                     // OFFSET_COMPRESS_DECAY_TIME         26
     8165755,                                                   // OFFSET_LIMIT_ATTACK_TIME           27
     30380,                                                     // OFFSET_LIMIT_DECAY_TIME            28
     $cmpd100.HEADROOM_OFFSET(2);                               // OFFSET_HEADROOM_COMPENSATION       29;
#endif


#if uses_COMPANDER
    .VAR/DM $cmpd_gain;
    .VAR cmpd100_obj[$cmpd100.STRUC_SIZE] =
     64,                                                // OFFSET_CONTROL_WORD                0
     $M.MUSIC_MANAGER.CONFIG.COMPANDER_BYPASS,          // OFFSET_BYPASS_BIT_MASK             1
     &stream_map_left_in,                               // OFFSET_INPUT_CH1_PTR               2
     &stream_map_right_in,                              // OFFSET_INPUT_CH2_PTR               3
     &stream_map_left_in,                               // OFFSET_OUTPUT_CH1_PTR              4
     &stream_map_right_in,                              // OFFSET_OUTPUT_CH2_PTR              5
     0x080000,                                          // OFFSET_MAKEUP_GAIN q4.19           6
     &$cmpd_gain,                                       // OFFSET_GAIN_PTR q4.19              7
     0x800000,                                          // OFFSET_NEG_ONE q.23                8
     0.0625,                                            // OFFSET_POW2_NEG4 q.23              9
     0xF9B037,                                          // OFFSET_EXPAND_THRESHOLD q.23       10
     0xFA0541,                                          // OFFSET_LINEAR_THRESHOLD q.23       11
     0xFE56CB,                                          // OFFSET_COMPRESS_THRESHOLD q.23     12
     0xFF8070,                                          // OFFSET_LIMIT_THRESHOLD q.23        13
     0x100000,                                          // OFFSET_INV_EXPAND_RATIO q4.19      14
     0x080000,                                          // OFFSET_INV_LINEAR_RATIO q4.19      15
     0x015555,                                          // OFFSET_INV_COMPRESS_RATIO q4.19    16
     0x00CCCD,                                          // OFFSET_INV_LIMIT_RATIO q4.19       17
     0,                                                 // OFFSET_EXPAND_CONSTANT q4.19       18
     0,                                                 // OFFSET_LINEAR_CONSTANT q4.19       19
     0,                                                 // OFFSET_COMPRESS_CONSTANT q4.19     20
     4328332,                                           // OFFSET_EXPAND_ATTACK_TIME          21
     200465,                                            // OFFSET_EXPAND_DECAY_TIME           22
     4328332,                                           // OFFSET_LINEAR_ATTACK_TIME          23
     60649,                                             // OFFSET_LINEAR_DECAY_TIME           24
     6423342,                                           // OFFSET_COMPRESS_ATTACK_TIME        25
     40482,                                             // OFFSET_COMPRESS_DECAY_TIME         26
     8165755,                                           // OFFSET_LIMIT_ATTACK_TIME           27
     30380,                                             // OFFSET_LIMIT_DECAY_TIME            28
     $cmpd100.HEADROOM_OFFSET(2);                       // OFFSET_HEADROOM_COMPENSATION       29;
#endif

   // multichannel volume and limit object
.BLOCK multichannel_volume_and_limit_block;

   .VAR multichannel_volume_and_limit_obj[$volume_and_limit.STRUC_SIZE] =
        0x000000,                                         //OFFSET_CONTROL_WORD_FIELD
        $M.MUSIC_MANAGER.CONFIG.VOLUME_LIMITER_BYPASS,    //OFFSET_BYPASS_BIT_FIELD
        5,                                                //NROF_CHANNELS_FIELD
        &$current_dac_sampling_rate,                      //SAMPLE_RATE_PTR_FIELD
        $music_example.MUTE_MASTER_VOLUME,                //MASTER_VOLUME_FIELD
        $music_example.LIMIT_THRESHOLD,                   //LIMIT_THRESHOLD_FIELD
        $music_example.LIMIT_THRESHOLD_LINEAR,            //LIMIT_THRESHOLD_LINEAR_FIELD
        $music_example.LIMIT_RATIO,                       //LIMIT_RATIO_FIELD_FIELD
        $music_example.RAMP_FACTOR,                       //RAMP FACTOR FIELD
        0 ...;

   .VAR left_primary_channel_vol_struc[$volume_and_limit.channel.STRUC_SIZE] =
        &stream_map_primary_left_out,                 // INPUT_PTR_FIELD
        &stream_map_primary_left_out,                 // OUTPUT_PTR_FIELD
        $music_example.DEFAULT_TRIM_VOLUME,           // TRIM_VOLUME_FIELD
        0 ...;

   .VAR right_primary_channel_vol_struc[$volume_and_limit.channel.STRUC_SIZE] =
        &stream_map_primary_right_out,                // INPUT_PTR_FIELD
        &stream_map_primary_right_out,                // OUTPUT_PTR_FIELD
        $music_example.DEFAULT_TRIM_VOLUME,           // TRIM_VOLUME_FIELD
        0 ...;

   .VAR left_secondary_channel_vol_struc[$volume_and_limit.channel.STRUC_SIZE] =
        &stream_map_secondary_left_out,               // INPUT_PTR_FIELD
        &stream_map_secondary_left_out,               // OUTPUT_PTR_FIELD
        $music_example.DEFAULT_TRIM_VOLUME,           // TRIM_VOLUME_FIELD
        0 ...;

   .VAR right_secondary_channel_vol_struc[$volume_and_limit.channel.STRUC_SIZE] =
        &stream_map_secondary_right_out,              // INPUT_PTR_FIELD
        &stream_map_secondary_right_out,              // OUTPUT_PTR_FIELD
        $music_example.DEFAULT_TRIM_VOLUME,           // TRIM_VOLUME_FIELD
        0 ...;

   .VAR wired_sub_channel_vol_struc[$volume_and_limit.channel.STRUC_SIZE] =
        &stream_map_sub_out,                          // INPUT_PTR_FIELD
        &stream_map_sub_out,                          // OUTPUT_PTR_FIELD
        $music_example.DEFAULT_TRIM_VOLUME,           // TRIM_VOLUME_FIELD
        0 ...;

.ENDBLOCK;

#ifdef WIRELESS_SUB_ENABLE
// define volume and limit object for sub out, there is no volume for
// sub, it's just headroom compensation and limiter
.BLOCK sub_volume_and_limit_block;
   .VAR sub_volume_and_limit_obj[$volume_and_limit.STRUC_SIZE] =
        0x000000,                                         //OFFSET_CONTROL_WORD_FIELD
        $M.MUSIC_MANAGER.CONFIG.VOLUME_LIMITER_BYPASS,    //OFFSET_BYPASS_BIT_FIELD
        1,                                                //NROF_CHANNELS_FIELD
        &$current_dac_sampling_rate,                      //SAMPLE_RATE_PTR_FIELD
        $music_example.DEFAULT_MASTER_VOLUME,             //MASTER_VOLUME_FIELD
        $music_example.LIMIT_THRESHOLD,                   //LIMIT_THRESHOLD_FIELD
        $music_example.LIMIT_THRESHOLD_LINEAR,            //LIMIT_THRESHOLD_LINEAR_FIELD
        $music_example.LIMIT_RATIO,                       //LIMIT_RATIO_FIELD_FIELD
        $music_example.RAMP_FACTOR,                       //RAMP_FACTOR_FIELD
        0 ...;

   .VAR sub_channel_vol_struc[ $volume_and_limit.channel.STRUC_SIZE] =
        &stream_map_sub_out,                 // INPUT_PTR_FIELD
        &stream_map_sub_out,                 // OUTPUT_PTR_FIELD
        $music_example.DEFAULT_TRIM_VOLUME,   // TRIM_VOLUME_FIELD
        0 ...;
   .ENDBLOCK;
#endif


#ifdef WIRELESS_SUB_ENABLE
   #define ALIGNMENT_DELAY $L2CAP_SUB_ALIGNMENT_DELAY

   DeclareCBuffer(delay_buf_pri_left_cbuffer_struc,delay_buf_pri_left,(max($ESCO_SUB_ALIGNMENT_DELAY,$L2CAP_SUB_ALIGNMENT_DELAY) + 1));
   DeclareCBuffer(delay_buf_pri_right_cbuffer_struc,delay_buf_pri_right,(max($ESCO_SUB_ALIGNMENT_DELAY,$L2CAP_SUB_ALIGNMENT_DELAY) + 1));

#if defined(MULTI_CHANNEL_DISABLE)
   // Use minimal buffer sizes for secondary and aux channels when multi-channel is disabled
   DeclareCBuffer(delay_buf_sec_left_cbuffer_struc,delay_buf_sec_left,1);
   DeclareCBuffer(delay_buf_sec_right_cbuffer_struc,delay_buf_sec_right,1);
   DeclareCBuffer(delay_buf_aux_left_cbuffer_struc,delay_buf_aux_left,1);
   DeclareCBuffer(delay_buf_aux_right_cbuffer_struc,delay_buf_aux_right,1);
#else
   // Default is multi-channel
   DeclareCBuffer(delay_buf_sec_left_cbuffer_struc,delay_buf_sec_left,(max($ESCO_SUB_ALIGNMENT_DELAY,$L2CAP_SUB_ALIGNMENT_DELAY) + 1));
   DeclareCBuffer(delay_buf_sec_right_cbuffer_struc,delay_buf_sec_right,(max($ESCO_SUB_ALIGNMENT_DELAY,$L2CAP_SUB_ALIGNMENT_DELAY) + 1));
   DeclareCBuffer(delay_buf_aux_left_cbuffer_struc,delay_buf_aux_left,(max($ESCO_SUB_ALIGNMENT_DELAY,$L2CAP_SUB_ALIGNMENT_DELAY) + 1));
   DeclareCBuffer(delay_buf_aux_right_cbuffer_struc,delay_buf_aux_right,(max($ESCO_SUB_ALIGNMENT_DELAY,$L2CAP_SUB_ALIGNMENT_DELAY) + 1));
#endif

   .VAR delay_pri_left[$audio_proc.delay.STRUC_SIZE] =
      &stream_map_primary_left_out,        // OFFSET_INPUT_PTR
      &stream_map_primary_left_out,        // OFFSET_OUTPUT_PTR
      &delay_buf_pri_left_cbuffer_struc,
      0,                                   // ALIGNMENT_DELAY,
      0,                                   // delay bit depth (1 = 16, 0 = 24 bit)
      0;                                   // Write Byte Position Indicator

   .VAR delay_pri_right[$audio_proc.delay.STRUC_SIZE] =
      &stream_map_primary_right_out,       // OFFSET_INPUT_PTR
      &stream_map_primary_right_out,       // OFFSET_OUTPUT_PTR
      &delay_buf_pri_right_cbuffer_struc,
      0,                                   // ALIGNMENT_DELAY,
      0,                                   // delay bit depth (1 = 16, 0 = 24 bit)
      0;                                   // Write Byte Position Indicator

   .VAR delay_sec_left[$audio_proc.delay.STRUC_SIZE] =
      &stream_map_secondary_left_out,      // OFFSET_INPUT_PTR
      &stream_map_secondary_left_out,      // OFFSET_OUTPUT_PTR
      &delay_buf_sec_left_cbuffer_struc,
      0,                                   // ALIGNMENT_DELAY,
      0,                                   // delay bit depth (1 = 16, 0 = 24 bit)
      0;                                   // Write Byte Position Indicator

   .VAR delay_sec_right[$audio_proc.delay.STRUC_SIZE] =
      &stream_map_secondary_right_out,     // OFFSET_INPUT_PTR
      &stream_map_secondary_right_out,     // OFFSET_OUTPUT_PTR
      &delay_buf_sec_right_cbuffer_struc,
      0,                                   // ALIGNMENT_DELAY,
      0,                                   // delay bit depth (1 = 16, 0 = 24 bit)
      0;                                   // Write Byte Position Indicator

   .VAR delay_aux_left[$audio_proc.delay.STRUC_SIZE] =
      &stream_map_aux_left_out,            // OFFSET_INPUT_PTR
      &stream_map_aux_left_out,            // OFFSET_OUTPUT_PTR
      &delay_buf_aux_left_cbuffer_struc,
      0,                                   // ALIGNMENT_DELAY,
      0,                                   // delay bit depth (1 = 16, 0 = 24 bit)
      0;                                   // Write Byte Position Indicator

   .VAR delay_aux_right[$audio_proc.delay.STRUC_SIZE] =
      &stream_map_aux_right_out,           // OFFSET_INPUT_PTR
      &stream_map_aux_right_out,           // OFFSET_OUTPUT_PTR
      &delay_buf_aux_right_cbuffer_struc,
      0,                                   // ALIGNMENT_DELAY,
      0,                                   // delay bit depth (1 = 16, 0 = 24 bit)
      0;                                   // Write Byte Position Indicator

#endif // WIRELES_SUB_ENABLE


   .VAR dithertype = 0;

// End of processing module DATA OBJECTS
// -----------------------------------------------------------------------------

// Statistics from Modules sent via SPI

   .VAR StatisticsPtrs[$M.MUSIC_MANAGER.STATUS.BLOCK_SIZE] =
      &$music_example.CurMode,
      &$music_example.SysControl,
      &$music_example.PeakMipsFunc,
      &$music_example.PeakMipsDecoder,
      &$M.Sleep.Mips,
      &pcmin_l_pk_dtct + $M.audio_proc.peak_monitor.PEAK_LEVEL,
      &pcmin_r_pk_dtct + $M.audio_proc.peak_monitor.PEAK_LEVEL,
#if defined(SPDIF_ENABLE) && defined(AC3_ENABLE)
      &pcmin_lfe_pk_dtct + $M.audio_proc.peak_monitor.PEAK_LEVEL,
#else
      &ZeroValue,
#endif // defined(SPDIF_ENABLE) && defined(AC3_ENABLE)
      &auxout_l_pk_dtct + $M.audio_proc.peak_monitor.PEAK_LEVEL,
      &auxout_r_pk_dtct + $M.audio_proc.peak_monitor.PEAK_LEVEL,
      &primout_l_pk_dtct + $M.audio_proc.peak_monitor.PEAK_LEVEL,
      &primout_r_pk_dtct + $M.audio_proc.peak_monitor.PEAK_LEVEL,
      &scndout_l_pk_dtct + $M.audio_proc.peak_monitor.PEAK_LEVEL,
      &scndout_r_pk_dtct + $M.audio_proc.peak_monitor.PEAK_LEVEL,
      &sub_pk_dtct + $M.audio_proc.peak_monitor.PEAK_LEVEL, //STAT for Wired Sub
      &sub_pk_dtct + $M.audio_proc.peak_monitor.PEAK_LEVEL, //STAT for Wireless Sub

      &$music_example.SystemVolume,

      &$music_example.Aux.MasterVolume,
      &$music_example.Aux.LeftTrimVolume,
      &$music_example.Aux.RightTrimVolume,

      &$music_example.Main.MasterVolume,
      &$music_example.Main.PrimaryLeftTrimVolume,
      &$music_example.Main.PrimaryRightTrimVolume,
      &$music_example.Main.SecondaryLeftTrimVolume,
      &$music_example.Main.SecondaryRightTrimVolume,
      &$music_example.Main.SubTrimVolume,

#if uses_USER_EQ
      &$M.system_config.data.CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_CONFIG,
#else
      &ZeroValue,
#endif
      &$M.MUSIC_EXAMPLE_MODULES_STAMP.CompConfig,
#ifdef LATENCY_REPORTING
      &$M.configure_latency_reporting.average_latency,
#else
      &MinusOne,
#endif
      &$music_example.DAC_IF_Connections,                      // INTERFACE CONN
      &$music_example.SPDIF_IF_Connections,                    // INTERFACE CONN
      &$music_example.I2S_IF_Connections,                      // INTERFACE CONN
      &$music_example.OTA_IF_Connections,                      // INTERFACE CONN
      &$music_example.SamplingRate,                            // INPUT_RATE
      &$current_dac_sampling_rate,                             // OUTPUT_RATE
      &$current_codec_sampling_rate,                           // CODEC_RATE
      &$codec_type,                                            // Codec type
      &$ancMode,                                               // 0: ANC disabled, 1: 96kHz ANC enabled, 2: 192kHz ANC enabled
      &$inputResolutionMode,                                   // Wired input: 16 or 24bit depth
      &$procResolutionMode,                                    // Processing: 16 or 24bit depth
      &$outputResolutionMode,                                  // Wired output: 16 or 24bit depth
      0 ...;
// Rest of table contains codec specific statistics pointers

// End of Stats Blocks
// -----------------------------------------------------------------------------
// STREAM MAPS - Stream definitions
//
// A static stream object is created for each stream. Current system has:
// 3 input stream maps in L,R and optional LFE
// 7 output stream maps out primary L,R, out secondary L,R, out aux L,R, out sub
//
// Stream maps populate processing module data objects with input and output
// pointers so processing modules know where to get and write their data.
// -----------------------------------------------------------------------------

// left input stream map
   .VAR   stream_map_left_in[$framesync_ind.ENTRY_SIZE_FIELD] =
          0,                                        // $framesync_ind.CBUFFER_PTR_FIELD
          0,                                        // $framesync_ind.FRAME_PTR_FIELD
          0,                                        // $framesync_ind.CUR_FRAME_SIZE_FIELD
          $music_example.NUM_SAMPLES_PER_FRAME,     // $framesync_ind.FRAME_SIZE_FIELD
          $music_example.JITTER,                    // $framesync_ind.JITTER_FIELD
          $frame_sync.distribute_input_stream_ind,  // Distribute Function
          $frame_sync.update_input_streams_ind,     // Update Function
          0 ...;

// right input stream map
    .VAR  stream_map_right_in[$framesync_ind.ENTRY_SIZE_FIELD] =
          0,                                        // $framesync_ind.CBUFFER_PTR_FIELD
          0,                                        // $framesync_ind.FRAME_PTR_FIELD
          0,                                        // $framesync_ind.CUR_FRAME_SIZE_FIELD
          $music_example.NUM_SAMPLES_PER_FRAME,     // $framesync_ind.FRAME_SIZE_FIELD
          $music_example.JITTER,                    // $framesync_ind.JITTER_FIELD
          $frame_sync.distribute_input_stream_ind,  // Distribute Function
          $frame_sync.update_input_streams_ind,     // Update Function
          0 ...;

#if defined(SPDIF_ENABLE) && defined(AC3_ENABLE)
// LFE input stream map
    .VAR  stream_map_lfe_in[$framesync_ind.ENTRY_SIZE_FIELD] =
          0,                                        // $framesync_ind.CBUFFER_PTR_FIELD
          0,                                        // $framesync_ind.FRAME_PTR_FIELD
          0,                                        // $framesync_ind.CUR_FRAME_SIZE_FIELD
          $music_example.NUM_SAMPLES_PER_FRAME,     // $framesync_ind.FRAME_SIZE_FIELD
          $music_example.JITTER,                    // $framesync_ind.JITTER_FIELD
          $frame_sync.distribute_input_stream_ind,  // Distribute Function
          $frame_sync.update_input_streams_ind,     // Update Function
          0 ...;
#endif // defined(SPDIF_ENABLE) && defined(AC3_ENABLE)
// -----------------------------------------------------------------------------
// left primary output stream map
    .VAR  stream_map_primary_left_out[$framesync_ind.ENTRY_SIZE_FIELD] =
          &$multi_chan_primary_left_out_cbuffer_struc, // $framesync_ind.CBUFFER_PTR_FIELD
          0,                                        // $framesync_ind.FRAME_PTR_FIELD
          0,                                        // $framesync_ind.CUR_FRAME_SIZE_FIELD
          $music_example.NUM_SAMPLES_PER_FRAME,     // $framesync_ind.FRAME_SIZE_FIELD
          $music_example.JITTER,                    // $framesync_ind.JITTER_FIELD
          $frame_sync.distribute_output_stream_ind, // Distribute Function
          $frame_sync.update_output_streams_ind,    // Update Function
          0 ...;

// right primary output stream map
    .VAR  stream_map_primary_right_out[$framesync_ind.ENTRY_SIZE_FIELD] =
          &$multi_chan_primary_right_out_cbuffer_struc, // $framesync_ind.CBUFFER_PTR_FIELD
          0,                                        // $framesync_ind.FRAME_PTR_FIELD
          0,                                        // $framesync_ind.CUR_FRAME_SIZE_FIELD
          $music_example.NUM_SAMPLES_PER_FRAME,     // $framesync_ind.FRAME_SIZE_FIELD
          $music_example.JITTER,                    // $framesync_ind.JITTER_FIELD
          $frame_sync.distribute_output_stream_ind, // Distribute Function
          $frame_sync.update_output_streams_ind,    // Update Function
          0 ...;

// left secondary output stream map
    .VAR  stream_map_secondary_left_out[$framesync_ind.ENTRY_SIZE_FIELD] =
          &$multi_chan_secondary_left_out_cbuffer_struc, // $framesync_ind.CBUFFER_PTR_FIELD
          0,                                        // $framesync_ind.FRAME_PTR_FIELD
          0,                                        // $framesync_ind.CUR_FRAME_SIZE_FIELD
          $music_example.NUM_SAMPLES_PER_FRAME,     // $framesync_ind.FRAME_SIZE_FIELD
          $music_example.JITTER,                    // $framesync_ind.JITTER_FIELD
          $frame_sync.distribute_output_stream_ind, // Distribute Function
          $frame_sync.update_output_streams_ind,    // Update Function
          0 ...;

// right secondary output stream map
    .VAR  stream_map_secondary_right_out[$framesync_ind.ENTRY_SIZE_FIELD] =
          &$multi_chan_secondary_right_out_cbuffer_struc, // $framesync_ind.CBUFFER_PTR_FIELD
          0,                                        // $framesync_ind.FRAME_PTR_FIELD
          0,                                        // $framesync_ind.CUR_FRAME_SIZE_FIELD
          $music_example.NUM_SAMPLES_PER_FRAME,     // $framesync_ind.FRAME_SIZE_FIELD
          $music_example.JITTER,                    // $framesync_ind.JITTER_FIELD
          $frame_sync.distribute_output_stream_ind, // Distribute Function
          $frame_sync.update_output_streams_ind,    // Update Function
          0 ...;

// left aux output stream map
    .VAR  stream_map_aux_left_out[$framesync_ind.ENTRY_SIZE_FIELD] =
          &$multi_chan_aux_left_out_cbuffer_struc,  // $framesync_ind.CBUFFER_PTR_FIELD
          0,                                        // $framesync_ind.FRAME_PTR_FIELD
          0,                                        // $framesync_ind.CUR_FRAME_SIZE_FIELD
          $music_example.NUM_SAMPLES_PER_FRAME,     // $framesync_ind.FRAME_SIZE_FIELD
          $music_example.JITTER,                    // $framesync_ind.JITTER_FIELD
          $frame_sync.distribute_output_stream_ind, // Distribute Function
          $frame_sync.update_output_streams_ind,    // Update Function
          0 ...;

// right aux output stream map
    .VAR  stream_map_aux_right_out[$framesync_ind.ENTRY_SIZE_FIELD] =
          &$multi_chan_aux_right_out_cbuffer_struc, // $framesync_ind.CBUFFER_PTR_FIELD
          0,                                        // $framesync_ind.FRAME_PTR_FIELD
          0,                                        // $framesync_ind.CUR_FRAME_SIZE_FIELD
          $music_example.NUM_SAMPLES_PER_FRAME,     // $framesync_ind.FRAME_SIZE_FIELD
          $music_example.JITTER,                    // $framesync_ind.JITTER_FIELD
          $frame_sync.distribute_output_stream_ind, // Distribute Function
          $frame_sync.update_output_streams_ind,    // Update Function
          0 ...;


    // sub output stream map
    .VAR  stream_map_sub_out[$framesync_ind.ENTRY_SIZE_FIELD] =
          &$multi_chan_sub_out_cbuffer_struc,       // $framesync_ind.CBUFFER_PTR_FIELD
          0,                                        // $framesync_ind.FRAME_PTR_FIELD
          0,                                        // $framesync_ind.CUR_FRAME_SIZE_FIELD
          $music_example.NUM_SAMPLES_PER_FRAME,     // $framesync_ind.FRAME_SIZE_FIELD
          $music_example.JITTER,                    // $framesync_ind.JITTER_FIELD
          $frame_sync.distribute_output_stream_ind, // Distribute Function
          $frame_sync.update_output_streams_ind,    // Update Function
          0 ...;

// ----------------------------------------------------------------------------
// STREAM SETUP TABLE
//
// This table is used with $frame.distribute_streams and $frame.update_streams.
// Null terminated list of streams to process

     .VAR rcv_process_streams[] =
        &stream_map_left_in,
        &stream_map_right_in,
#if defined(SPDIF_ENABLE) && defined(AC3_ENABLE)
        &stream_map_lfe_in,
#endif // defined(SPDIF_ENABLE) && defined(AC3_ENABLE)
        &stream_map_primary_left_out,
        &stream_map_primary_right_out,
        &stream_map_secondary_left_out,
        &stream_map_secondary_right_out,
        &stream_map_aux_left_out,
        &stream_map_aux_right_out,
        &stream_map_sub_out,
        0;

// -----------------------------------------------------------------------------
// REINITIALIZATION FUNCTION TABLE
// Reinitialization functions and corresponding data objects can be placed
// in this table.  Functions in this table all called every time a frame of data
// is ready to be processed and the reinitialization flag is set.
// This table must be null terminated.

   .VAR reinitialize_table[] =
    // Function                          r7                   r8
#if uses_SPKR_EQ
    $user_eq.eqInitialize,              &SpkrEqDefnTable,     &SpkrEqParams,
#endif
#if uses_BASS_BOOST
    $user_eq.eqInitialize,              &BoostEqDefnTable,    &BoostEqParams,
#endif
#if uses_ANC_EQ
    $user_eq.eqInitialize,              &AncEqDefnTable,    &AncEqParams,
#endif
#if uses_BASS_PLUS
    $bass_plus_init_wrapper,            &bass_plus_left,      &bass_plus_right,
#endif  // uses_BASS_PLUS
#if uses_USER_EQ
    $user_eq.userEqInitialize,          &UserEqDefnTable,     &user_eq_bank_select,
#endif
#if uses_STEREO_ENHANCEMENT
    $MeloD_Expansion.initialize,        &MeloD_Expansion_struct,    0,
#endif
#if uses_3DV
    $n3dv_init_wrapper,                 &n3dv_left,           &n3dv_right,
#endif  // uses_3DV
#if uses_COMPANDER
    $cmpd100.initialize,                0,                      &cmpd100_obj,
#endif

    $volume_and_limit.initialize,  &multichannel_volume_and_limit_obj,   0,
    $volume_and_limit.initialize,  &aux_stereo_volume_and_limit_obj,     0,

#ifdef WIRELESS_SUB_ENABLE
    $volume_and_limit.initialize,  &sub_volume_and_limit_obj,   0,
#endif   // WIRELESS_SUB_ENABLE

#if uses_SPEAKER_CROSSOVER
    $spkr_ctrl_system.initialize,   &spkr_ctrl_init_struct,   0,
      #if uses_WIRED_SUB_EQ
          $user_eq.eqInitialize,      &WiredSubEqDefnTable,           &WiredSubEqParams,
      #endif
      #if uses_WIRED_SUB_COMPANDER
          $cmpd100.initialize,        0,                              &cmpd100_obj_wired_sub,
      #endif

#endif  // uses_SPEAKER_CROSSOVER
      0;

.VAR delay_reinitialize_table[] =
    // Function                          r7                   r8

#ifdef  WIRELESS_SUB_ENABLE
        $audio_proc.delay.initialize,   0,                    &delay_pri_left,
        $audio_proc.delay.initialize,   0,                    &delay_pri_right,
        $audio_proc.delay.initialize,   0,                    &delay_sec_left,
        $audio_proc.delay.initialize,   0,                    &delay_sec_right,
        $audio_proc.delay.initialize,   0,                    &delay_aux_left,
        $audio_proc.delay.initialize,   0,                    &delay_aux_right,
#endif // WIRELESS_SUB_ENABLE
    0;

// -----------------------------------------------------------------------------

   .VAR filter_reset_table[] =
    // Function                         r7                      r8
#if uses_SPKR_EQ
    $audio_proc.hq_peq.zero_delay_data,  &spkr_eq_left_dm2,    0,
    $audio_proc.hq_peq.zero_delay_data,  &spkr_eq_right_dm2,   0,
#endif
#if uses_BASS_BOOST
    $audio_proc.hq_peq.zero_delay_data,  &boost_eq_left_dm2,   0,
    $audio_proc.hq_peq.zero_delay_data,  &boost_eq_right_dm2,  0,
#endif
#if uses_ANC_EQ
    $audio_proc.hq_peq.zero_delay_data,  &anc_eq_left_dm2,   0,
    $audio_proc.hq_peq.zero_delay_data,  &anc_eq_right_dm2,  0,
#endif
#if uses_USER_EQ
    $audio_proc.hq_peq.zero_delay_data,  &user_eq_left_dm2,    0,
    $audio_proc.hq_peq.zero_delay_data,  &user_eq_right_dm2,   0,
#endif

    #if uses_SPEAKER_CROSSOVER
        $spkr_ctrl_system.zero_data,     &spkr_ctrl_bass_buffer, length(&spkr_ctrl_bass_buffer),
            #if uses_WIRED_SUB_EQ
                $audio_proc.peq.zero_delay_data,      &sub_eq_dm2,           0,
            #endif
    #endif
    0;

// -----------------------------------------------------------------------------

// MODE FUNCTION TABLE
// This table contains all of the modes in this system.  The VM plugin sends a
// message that contains a mode value, which is used as a pointer into this
// table.Developers can expand this table if they add additional processing modes
// to the system. This table must be null terminated.
//
// Every time a frame of data is ready to process, the functions from the
// corresponding mode table are called.

    .VAR mode_table[] =
        &copy_proc_funcs,
        &copy_proc_funcs,
        &full_proc_funcs,
        // more entries can be added here
        0;

// ----------------------------------------------------------------------------
// MODE TABLES (aka FUNCTION TABLES)
// Modes are defined as tables that contain a list of functions with
// corresponding
// data objects.  The functions are called in the order that they appear.
//
// $frame.distribute_stream should always be first as it tells the processing
// modules where to get and write data.
//
// $frame.update_streams should always be last as it advances cbuffer pointers
// to the correct positions after the processing modules have read and written
// data.
//
// The processing modules are called by the function $frame.run_function_table,
// which is defined in the frame_sync library.
//
// Processing modules must be written to take input from r7 and r8.  A zero
// should be used if the module does not require input.
//
// Mode tables must be null terminated.
//
// Additional modes can be created by adding new tables.
// -----------------------------------------------------------------------------

    .VAR copy_proc_funcs[] =
        // Function                                 r7                          r8
        $frame_sync.distribute_streams_ind,         &rcv_process_streams,       0,

        // Start Music Manager MIPS calculation
        $M.mips_profile.mainstart,                  0,                          $FunctionMips_data_block,

        $M.audio_proc.peak_monitor.Process.func,    &pcmin_l_pk_dtct,           0,
        $M.audio_proc.peak_monitor.Process.func,    &pcmin_r_pk_dtct,           0,

#if defined(SPDIF_ENABLE) && defined(AC3_ENABLE)
        $M.audio_proc.peak_monitor.Process.func,    &pcmin_lfe_pk_dtct,         0,
        $M.audio_proc.stream_gain.Process.func,     &lfe_headroom_obj,          0,
#endif // defined(SPDIF_ENABLE) && defined(AC3_ENABLE)
        $M.audio_proc.stream_gain.Process.func,     &left_headroom_obj,         0,
        $M.audio_proc.stream_gain.Process.func,     &right_headroom_obj,        0,

        $Set_Mode,                                  &ModeControl,               0,

        $M.check_aux_out.process,                   &uses_aux_out,              &aux_downmix,

        $volume_and_limit.apply_volume,             &multichannel_volume_and_limit_obj, 0,


        $M.audio_proc.peak_monitor.Process.func,     &primout_l_pk_dtct,        0,
        $M.audio_proc.peak_monitor.Process.func,     &primout_r_pk_dtct,        0,
        $M.audio_proc.peak_monitor.Process.func,     &sub_pk_dtct,              0,
        $M.audio_proc.peak_monitor.Process.func,     &scndout_l_pk_dtct,        0,
        $M.audio_proc.peak_monitor.Process.func,     &scndout_r_pk_dtct,        0,
        $M.audio_proc.peak_monitor.Process.func,     &auxout_l_pk_dtct,         0,
        $M.audio_proc.peak_monitor.Process.func,     &auxout_r_pk_dtct,         0,

        // Terminate Music Manager MIPS calculation
        $M.mips_profile.mainend,                     0,                          $FunctionMips_data_block,

        $frame_sync.update_streams_ind,             &rcv_process_streams,       0,
        0;

// -----------------------------------------------------------------------------

    .VAR full_proc_funcs[] =
        // Function                                 r7                          r8
        $frame_sync.distribute_streams_ind,         &rcv_process_streams,       0,

        // Start Music Manager MIPS calculation
        $M.mips_profile.mainstart,                  0,                          $FunctionMips_data_block,

        $M.audio_proc.peak_monitor.Process.func,    &pcmin_l_pk_dtct,           0,
        $M.audio_proc.peak_monitor.Process.func,    &pcmin_r_pk_dtct,           0,

#if defined(SPDIF_ENABLE) && defined(AC3_ENABLE)
        $M.audio_proc.peak_monitor.Process.func,    &pcmin_lfe_pk_dtct,         0,
        $M.audio_proc.stream_gain.Process.func,     &lfe_headroom_obj,          0,
#endif // defined(SPDIF_ENABLE) && defined(AC3_ENABLE)
        $M.audio_proc.stream_gain.Process.func,     &left_headroom_obj,         0,
        $M.audio_proc.stream_gain.Process.func,     &right_headroom_obj,        0,

        $M.check_aux_out.process,                   &uses_aux_out,              &aux_downmix,

#if uses_COMPANDER
        $cmpd100.analysis,                          0,                          &cmpd100_obj,
        $cmpd100.applygain,                         0,                          &cmpd100_obj,
#endif

#if uses_USER_EQ
        $music_example.peq.process,                 &user_eq_left_dm2,          $M.MUSIC_MANAGER.CONFIG.USER_EQ_BYPASS,
        $music_example.peq.process,                 &user_eq_right_dm2,         $M.MUSIC_MANAGER.CONFIG.USER_EQ_BYPASS,
#endif  // uses_USER_EQ

#if uses_ANC_EQ
        $music_example.anc_eq.process,              &anc_eq_left_dm2,           $M.MUSIC_MANAGER.CONFIG.ANC_EQ_BYPASS,
        $music_example.anc_eq.process,              &anc_eq_right_dm2,          $M.MUSIC_MANAGER.CONFIG.ANC_EQ_BYPASS,
#endif
#if uses_BASS_BOOST
        $bass_boost_process_wrapper,                &boost_eq_left_dm2,         &boost_eq_right_dm2,
#endif  // uses_BASS_BOOST

#if uses_BASS_PLUS
        $bass_plus_process_wrapper,                 &bass_plus_left,            &bass_plus_right,
#endif  // uses_BASS_PLUS

#if uses_SPKR_EQ
        $music_example.peq.process,                 &spkr_eq_left_dm2,          $M.MUSIC_MANAGER.CONFIG.SPKR_EQ_BYPASS,
        $music_example.peq.process,                 &spkr_eq_right_dm2,         $M.MUSIC_MANAGER.CONFIG.SPKR_EQ_BYPASS,
#endif  // uses_SPKR_EQ

#if uses_STEREO_ENHANCEMENT
        $melod_expansion_process_wrapper,           &MeloD_Expansion_struct,    0,
#endif  // uses_STEREO_ENHANCEMENT

#if uses_3DV
        $n3dv_process_wrapper,                      &n3dv_left,                 &n3dv_right,
#endif  // uses_3DV

#if uses_SPEAKER_CROSSOVER
        $spkr_ctrl_system.process,                  &spkr_ctrl_struct,          0,
        $M.check_wired_subwoofer.process,           &uses_wired_sub,            0,
#endif  // uses_SPEAKER_CROSSOVER

#ifdef WIRELESS_SUB_ENABLE
        $audio_proc.delay.process,                  0,                          &delay_pri_left,
        $audio_proc.delay.process,                  0,                          &delay_pri_right,
        $audio_proc.delay.process,                  0,                          &delay_sec_left,
        $audio_proc.delay.process,                  0,                          &delay_sec_right,
        $audio_proc.delay.process,                  0,                          &delay_aux_left,
        $audio_proc.delay.process,                  0,                          &delay_aux_right,
#endif   // WIRELESS_SUB_ENABLE

        $volume_and_limit.apply_volume,              &multichannel_volume_and_limit_obj, 0,

#ifdef WIRELESS_SUB_ENABLE
        $volume_and_limit.apply_volume,              &sub_volume_and_limit_obj,  0,
#endif   // WIRELESS_SUB_ENABLE

        $M.audio_proc.peak_monitor.Process.func,     &primout_l_pk_dtct,         0,
        $M.audio_proc.peak_monitor.Process.func,     &primout_r_pk_dtct,         0,
        $M.audio_proc.peak_monitor.Process.func,     &sub_pk_dtct,               0,
        $M.audio_proc.peak_monitor.Process.func,     &scndout_l_pk_dtct,         0,
        $M.audio_proc.peak_monitor.Process.func,     &scndout_r_pk_dtct,         0,
        $M.audio_proc.peak_monitor.Process.func,     &auxout_l_pk_dtct,          0,
        $M.audio_proc.peak_monitor.Process.func,     &auxout_r_pk_dtct,          0,

        // Terminate Music Manager MIPS calculation
        $M.mips_profile.mainend,                     0,                          $FunctionMips_data_block,

#ifdef WIRELESS_SUB_ENABLE
        $M.Subwoofer.synchronise,                    0,                          0,
        $M.Subwoofer.schedule_and_sleep,             0,                          0,
#endif
        $frame_sync.update_streams_ind,              &rcv_process_streams,       0,
        0;

.ENDMODULE;

// -----------------------------------------------------------------------------

.MODULE $M.check_wired_subwoofer;
   .CODESEGMENT PM;

   process:

   // push rLink onto stack
   $push_rLink_macro;

   Null = M[r7];
   if Z jump done;

#if uses_WIRED_SUB_EQ
   // wired subwoofer is connected
   r7 = $M.system_config.data.sub_eq_dm2;
   r8 = $M.MUSIC_MANAGER.CONFIG.WIRED_SUBWOOFER_EQ_BYPASS;
   call $music_example.peq.process;
#endif

#if uses_WIRED_SUB_COMPANDER
   r8 = $M.system_config.data.cmpd100_obj_wired_sub;
   call $cmpd100.analysis;

   r8 = $M.system_config.data.cmpd100_obj_wired_sub;
   call $cmpd100.applygain;
#endif

done:
   // pop rLink from stack
   jump $pop_rLink_and_rts;

.ENDMODULE;

// ----------------------------------------------------------------------------

.MODULE $M.check_aux_out;
   .CODESEGMENT PM;

   process:

   // push rLink onto stack
   $push_rLink_macro;

   Null = M[r7];
   if Z jump done;

   // check if downmix is required
   Null = M[r8];
   if Z jump stereo_copy;

   r7 = &$M.system_config.data.aux_mix_obj;
   r8 = 0;
   call $M.audio_proc.stream_mixer.Process.func;
   jump apply_vol;

stereo_copy:

   r7 = &$M.system_config.data.aux_stereo_copy_obj;
   r8 = 0;
   call $M.audio_proc.stereo_copy.Process.func;

apply_vol:
   // aux out is connected. set volume
   r7 = &$M.system_config.data.aux_stereo_volume_and_limit_obj;
   r8 = 0;
   call $volume_and_limit.apply_volume;

done:


   // pop rLink from stack
   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
//
// MODULE:
//    $M.set_mode_gains
//
// DESCRIPTION:
//    Sets gain based on the current mode. This function should only be called
//    from within standby, and pass-through modes.
//    (Note : Standby only mutes music manager processing.
//
// INPUTS:
//    r7 - Pointer to the data structure
//
// *****************************************************************************
.MODULE $M.set_mode_gains;
    .CODESEGMENT PM;
    .DATASEGMENT DM;

$Set_Mode:

   // push rLink onto stack
   $push_rLink_macro;

   r3 = M[$music_example.CurMode];
   Null = r3 - $M.MUSIC_MANAGER.SYSMODE.STANDBY;
   if Z jump standby_gains;

   r1 = 1.0;
   r2 = Null;
   jump continue;

standby_gains:

   r1 = Null;
   r2 = 1;

continue:
   r3 = M[$M.multi_chan_port_scan_and_routing_config.fp_config_input];
   r3 = M[r3 + $INTERFACE_MAP_ENABLED_CHANNELS_FIELD];

   //Check Primary Left-connect
   r4 = $MULTI_CHAN_PRIMARY_CHANNELS_LEFT_MASK;
   Null = r3 and r4;
   if Z jump no_pri_left;

   r0 = &$M.system_config.data.stream_map_primary_left_out;
   M[&$M.system_config.data.passthru_primary_left + $M.audio_proc.stream_gain.OFFSET_OUTPUT_PTR] = r0;
   M[r7 + $music_example.PRIM_LEFT_MANT]  = r1;
   M[r7 + $music_example.PRIM_LEFT_EXP]   = r2;

   pushm <r1,r2,r3,r7>;
   r7 = &$M.system_config.data.passthru_primary_left;
   r8 = 0;
   call $M.audio_proc.stream_gain.Process.func;
   popm <r1,r2,r3,r7>;

no_pri_left:
   r4 = $MULTI_CHAN_PRIMARY_CHANNELS_RIGHT_MASK;
   Null = r3 and r4;
   if Z jump no_pri_right;

   r0 = &$M.system_config.data.stream_map_primary_right_out;
   M[&$M.system_config.data.passthru_primary_right + $M.audio_proc.stream_gain.OFFSET_OUTPUT_PTR] = r0;
   M[r7 + $music_example.PRIM_RGHT_MANT]  = r1;
   M[r7 + $music_example.PRIM_RGHT_EXP]   = r2;

   pushm <r1,r2,r3,r7>;
   r7 = &$M.system_config.data.passthru_primary_right;
   r8 = 0;
   call $M.audio_proc.stream_gain.Process.func;
   popm <r1,r2,r3,r7>;

no_pri_right:
    // configure subwoofer signal path
   r4 = $MULTI_CHAN_SUB_WIRED_CHANNELS_MASK;
   null = r4 and r3;
   if Z jump no_wired_sub;

   r0 = &$M.system_config.data.stream_map_sub_out;
   M[&$M.system_config.data.passthru_sub + $M.audio_proc.stream_gain.OFFSET_OUTPUT_PTR] = r0;
   M[r7 + $music_example.SUB_MANT]        = r1;
   M[r7 + $music_example.SUB_EXP]         = r2;

   pushm <r1,r2,r3,r7>;
   r7 = &$M.system_config.data.passthru_sub;
   r8 = 0;
   call $M.audio_proc.stream_gain.Process.func;
   popm <r1,r2,r3,r7>;

no_wired_sub:
   r4 = $MULTI_CHAN_SECONDARY_CHANNELS_LEFT_MASK;
   Null = r3 and r4;
   if Z jump no_sec_left;

   r0 = &$M.system_config.data.stream_map_secondary_left_out;
   M[&$M.system_config.data.passthru_secondary_left + $M.audio_proc.stream_gain.OFFSET_OUTPUT_PTR] = r0;
   M[r7 + $music_example.SCND_LEFT_MANT]  = r1;
   M[r7 + $music_example.SCND_LEFT_EXP]   = r2;

   pushm <r1,r2,r3,r7>;
   r7 = &$M.system_config.data.passthru_secondary_left;
   r8 = 0;
   call $M.audio_proc.stream_gain.Process.func;
   popm <r1,r2,r3,r7>;

no_sec_left:
   r4 = $MULTI_CHAN_SECONDARY_CHANNELS_RIGHT_MASK;
   Null = r3 and r4;
   if Z jump no_sec_right;

   r0 = &$M.system_config.data.stream_map_secondary_right_out;
   M[&$M.system_config.data.passthru_secondary_right + $M.audio_proc.stream_gain.OFFSET_OUTPUT_PTR] = r0;
   M[r7 + $music_example.SCND_RGHT_MANT]  = r1;
   M[r7 + $music_example.SCND_RGHT_EXP]   = r2;

   pushm <r1,r2,r3,r7>;
   r7 = &$M.system_config.data.passthru_secondary_right;
   r8 = 0;
   call $M.audio_proc.stream_gain.Process.func;
   popm <r1,r2,r3,r7>;

no_sec_right:

   // pop rLink from stack
   jump $pop_rLink_and_rts;

.ENDMODULE;
// *****************************************************************************
// MODULE:
//    $M.frame_proc_stream_configure
//
// DESCRIPTION:
// This routine is called to configure the Frame Processing based on port
// connections
//
// INPUTS:
//    r3 = $interface_map_struc
//       ([0]: Wired channel enable mask
//        [1]: DAC channel mask (sets which channels are on DACs)
//        [2]: Bitmask for Chain 1 channels)
//        [3]: S/PDIF channel mask (sets which channels are on S/PDIF)
//
// *****************************************************************************
.MODULE $M.frame_proc_stream_configure;
 .CODESEGMENT PM;
 .DATASEGMENT DM;

func:
   .VAR $DAC_conn_main = 0;
   .VAR $DAC_conn_aux = 0;
   .VAR if_struc_temp=0;

   $push_rLink_macro;
   M[if_struc_temp] = r3;
   r0 = M[r3 + $INTERFACE_MAP_ENABLED_CHANNELS_FIELD];

   // Check to see which volume control is connected to the DAC

   r5 = NULL;
   r3 = r3 + $INTERFACE_MAP_DAC_CHANNELS_FIELD;
   r1 = M[r3];                      // I2S/DAC Mask
   r1 = r1 AND r0;                  // Mask of connected DAC ports

   r2 = $MULTI_CHAN_AUX_CHANNELS_LEFT_MASK;
   r4 = $M.MUSIC_MANAGER.CONFIG.IFFUNC_AUX_L;
   Null = r2 and r1;
   if Z r4 = null;
   r5 = r5 OR r4;

   r2 = $MULTI_CHAN_PRIMARY_CHANNELS_LEFT_MASK;
   r4 = $M.MUSIC_MANAGER.CONFIG.IFFUNC_PRI_L;
   Null = r2 and r1;
   if Z r4 = null;
   r5 = r5 OR r4;

   r2 = $MULTI_CHAN_SUB_WIRED_CHANNELS_MASK;
   r4 = $M.MUSIC_MANAGER.CONFIG.IFFUNC_SUB;
   Null = r2 and r1;
   if Z r4 = null;
   r5 = r5 OR r4;

   r2 = $MULTI_CHAN_AUX_CHANNELS_RIGHT_MASK;
   r4 = $M.MUSIC_MANAGER.CONFIG.IFFUNC_AUX_R;
   Null = r2 and r1;
   if Z r4 = null;
   r5 = r5 OR r4;

   r2 = $MULTI_CHAN_PRIMARY_CHANNELS_RIGHT_MASK;
   r4 = $M.MUSIC_MANAGER.CONFIG.IFFUNC_PRI_R;
   Null = r2 and r1;
   if Z r4 = null;
   r5 = r5 OR r4;

   M[$music_example.DAC_IF_Connections] = r5;

   r5 = NULL;                       //Reset for next stat

   r1 = M[r3];                      // I2S/DAC Mask
   r1 = r1 XOR 0xffffff;            // TBD: Everything that is not DAC gets counted as I2S? What about SPDIF? Probably needs to exclude SPDIF


   r1 = r1 AND r0;                  // Mask of connected I2S ports


   r2 = $MULTI_CHAN_AUX_CHANNELS_LEFT_MASK;
   r4 = $M.MUSIC_MANAGER.CONFIG.IFFUNC_AUX_L;
   Null = r2 and r1;
   if Z r4 = null;
   r5 = r5 OR r4;

   r2 = $MULTI_CHAN_PRIMARY_CHANNELS_LEFT_MASK;
   r4 = $M.MUSIC_MANAGER.CONFIG.IFFUNC_PRI_L;
   Null = r2 and r1;
   if Z r4 = null;
   r5 = r5 OR r4;

   r2 = $MULTI_CHAN_SECONDARY_CHANNELS_LEFT_MASK;
   r4 = $M.MUSIC_MANAGER.CONFIG.IFFUNC_SEC_L;
   Null = r2 and r1;
   if Z r4 = null;
   r5 = r5 OR r4;

   r2 = $MULTI_CHAN_SUB_WIRED_CHANNELS_MASK;
   r4 = $M.MUSIC_MANAGER.CONFIG.IFFUNC_SUB;
   Null = r2 and r1;
   if Z r4 = null;
   r5 = r5 OR r4;

   r2 = $MULTI_CHAN_AUX_CHANNELS_RIGHT_MASK;
   r4 = $M.MUSIC_MANAGER.CONFIG.IFFUNC_AUX_R;
   Null = r2 and r1;
   if Z r4 = null;
   r5 = r5 OR r4;

   r2 = $MULTI_CHAN_PRIMARY_CHANNELS_RIGHT_MASK;
   r4 = $M.MUSIC_MANAGER.CONFIG.IFFUNC_PRI_R;
   Null = r2 and r1;
   if Z r4 = null;
   r5 = r5 OR r4;

   r2 = $MULTI_CHAN_SECONDARY_CHANNELS_RIGHT_MASK;
   r4 = $M.MUSIC_MANAGER.CONFIG.IFFUNC_SEC_R;
   Null = r2 and r1;
   if Z r4 = null;
   r5 = r5 OR r4;

   M[$music_example.I2S_IF_Connections] = r5;

   r5 = NULL;                       //Reset for next stat

   r3 = M[if_struc_temp];
   r3 = r3 + $INTERFACE_MAP_SPDIF_CHANNELS_FIELD;
   r1 = M[r3];                      // SPDIF Mask
   r1 = r1 AND r0;                  // Mask of connected SPDIF ports

   r2 = $MULTI_CHAN_AUX_CHANNELS_LEFT_MASK;
   r4 = $M.MUSIC_MANAGER.CONFIG.IFFUNC_AUX_L;
   Null = r2 and r1;
   if Z r4 = null;
   r5 = r5 OR r4;

   r2 = $MULTI_CHAN_PRIMARY_CHANNELS_LEFT_MASK;
   r4 = $M.MUSIC_MANAGER.CONFIG.IFFUNC_PRI_L;
   Null = r2 and r1;
   if Z r4 = null;
   r5 = r5 OR r4;

   r2 = $MULTI_CHAN_SUB_WIRED_CHANNELS_MASK;
   r4 = $M.MUSIC_MANAGER.CONFIG.IFFUNC_SUB;
   Null = r2 and r1;
   if Z r4 = null;
   r5 = r5 OR r4;

   r2 = $MULTI_CHAN_AUX_CHANNELS_RIGHT_MASK;
   r4 = $M.MUSIC_MANAGER.CONFIG.IFFUNC_AUX_R;
   Null = r2 and r1;
   if Z r4 = null;
   r5 = r5 OR r4;

   r2 = $MULTI_CHAN_PRIMARY_CHANNELS_RIGHT_MASK;
   r4 = $M.MUSIC_MANAGER.CONFIG.IFFUNC_PRI_R;
   Null = r2 and r1;
   if Z r4 = null;
   r5 = r5 OR r4;

   M[$music_example.SPDIF_IF_Connections] = r5;

   r3 = M[if_struc_temp];
   r3 = r3 + $INTERFACE_MAP_DAC_CHANNELS_FIELD;
   r1 = M[r3];                      // I2S/DAC Mask

   r2 = $MULTI_CHAN_AUX_CHANNELS_LEFT_MASK;
   r2 = r2 OR $MULTI_CHAN_AUX_CHANNELS_RIGHT_MASK;
   r4 = 1;
   Null = r2 and r1;                // Test to see if DAC is connected to AUX
   if Z r4=NULL;                    // set $aux_vol_msg_echo to 1 if DAC<-Aux
   M[$DAC_conn_aux] = r4;


   r2 = $MULTI_CHAN_PRIMARY_CHANNELS_LEFT_MASK ;
   r2 = r2 OR $MULTI_CHAN_PRIMARY_CHANNELS_RIGHT_MASK ;
   r2 = r2 OR $MULTI_CHAN_SUB_WIRED_CHANNELS_MASK;

   r4 = 1;
   Null = r2 and r1;                // Test to see if DAC is connected to PRI OR SUB
   if Z r4=NULL;                    // set $multichannel_vol_msg_echo to 1 if DAC<-PRI/SUB
   M[$DAC_conn_main] = r4;


#if defined SHAREME_ENABLE || defined TWS_ENABLE

   r1 = M[$relay.mode];
   null = r1 - $TWS_MASTER_MODE;
   if NEG jump no_return_msg;

    // enable volume return messages
   r2 = M[$tws.external_volume_enable];
   r1 = M[$DAC_conn_main];
   r1 = r1 OR r2;
   M[$multichannel_vol_msg_echo] = r1;

   no_return_msg:

#endif



   //Check Primary Left-connect
   r2 = $MULTI_CHAN_PRIMARY_CHANNELS_LEFT_MASK;
   Null = r0 and r2;
   if Z jump no_pri_left;
   r1 = &$M.system_config.data.stream_map_primary_left_out;
   M[$M.system_config.data.spkr_ctrl_struct + $spkr_ctrl_system.LEFT_PRI_OUTPUT_PTR] = r1;
   jump check_pri_right;

   no_pri_left:
   M[$M.system_config.data.spkr_ctrl_struct + $spkr_ctrl_system.LEFT_PRI_OUTPUT_PTR] = NULL;
   M[$M.system_config.data.primout_l_pk_dtct + $M.audio_proc.peak_monitor.PEAK_LEVEL] = Null;

   check_pri_right:

   r2 = $MULTI_CHAN_PRIMARY_CHANNELS_RIGHT_MASK;
   Null = r0 and r2;
   if Z jump no_pri_right;
   r1 = &$M.system_config.data.stream_map_primary_right_out;
   M[$M.system_config.data.spkr_ctrl_struct + $spkr_ctrl_system.RIGHT_PRI_OUTPUT_PTR] = r1;
   jump xover_config;

   no_pri_right:
   M[$M.system_config.data.spkr_ctrl_struct + $spkr_ctrl_system.RIGHT_PRI_OUTPUT_PTR] = NULL;
   M[$M.system_config.data.primout_r_pk_dtct + $M.audio_proc.peak_monitor.PEAK_LEVEL] = Null;

   xover_config:

   // Default to no secondary channels
   M[$M.system_config.data.spkr_ctrl_init_struct + $spkr_ctrl_system.INIT_SEC_EQ_DEFN_PTR] = NULL;
   M[$M.system_config.data.spkr_ctrl_init_struct + $spkr_ctrl_system.INIT_SEC_EQ_BANK_SELECT_PTR] = NULL;

   r2 = $MULTI_CHAN_SECONDARY_CHANNELS_LEFT_MASK;
   Null = r0 and r2;
   if Z jump no_sec_left;

   r1 = &$M.system_config.data.stream_map_secondary_left_out;
   M[$M.system_config.data.spkr_ctrl_struct + $spkr_ctrl_system.LEFT_SEC_OUTPUT_PTR] = r1;
   r1 = &$M.system_config.data.spkr_ctrl_sec_eq_defn_table;
   M[$M.system_config.data.spkr_ctrl_init_struct + $spkr_ctrl_system.INIT_SEC_EQ_DEFN_PTR] = r1;
   r1 = &$M.system_config.data.spkr_ctrl_sec_eq_bank_select;
   M[$M.system_config.data.spkr_ctrl_init_struct + $spkr_ctrl_system.INIT_SEC_EQ_BANK_SELECT_PTR] = r1;
   r1 = &$M.system_config.data.spkr_ctrl_left_sec_eq_dm;
   M[$M.system_config.data.spkr_ctrl_coefs + $spkr_ctrl_system.COEF_EQ_L_SEC_PTR] = r1;

   jump check_sec_right;

   no_sec_left:
   M[$M.system_config.data.spkr_ctrl_struct + $spkr_ctrl_system.LEFT_SEC_OUTPUT_PTR] = NULL;
   M[$M.system_config.data.spkr_ctrl_coefs + $spkr_ctrl_system.COEF_EQ_L_SEC_PTR] = NULL;
   M[$M.system_config.data.scndout_l_pk_dtct + $M.audio_proc.peak_monitor.PEAK_LEVEL] = Null;

   check_sec_right:

   r2 = $MULTI_CHAN_SECONDARY_CHANNELS_RIGHT_MASK;
   Null = r0 and r2;
   if Z jump no_sec_right;

   r1 = &$M.system_config.data.stream_map_secondary_right_out;
   M[$M.system_config.data.spkr_ctrl_struct + $spkr_ctrl_system.RIGHT_SEC_OUTPUT_PTR] = r1;
   r1 = &$M.system_config.data.spkr_ctrl_sec_eq_defn_table;
   M[$M.system_config.data.spkr_ctrl_init_struct + $spkr_ctrl_system.INIT_SEC_EQ_DEFN_PTR] = r1;
   r1 = &$M.system_config.data.spkr_ctrl_sec_eq_bank_select;
   M[$M.system_config.data.spkr_ctrl_init_struct + $spkr_ctrl_system.INIT_SEC_EQ_BANK_SELECT_PTR] = r1;
   r1 = &$M.system_config.data.spkr_ctrl_right_sec_eq_dm;
   M[$M.system_config.data.spkr_ctrl_coefs + $spkr_ctrl_system.COEF_EQ_R_SEC_PTR] = r1;

   jump aux_config;

   no_sec_right:
   M[$M.system_config.data.spkr_ctrl_struct + $spkr_ctrl_system.RIGHT_SEC_OUTPUT_PTR] = NULL;
   M[$M.system_config.data.spkr_ctrl_coefs + $spkr_ctrl_system.COEF_EQ_R_SEC_PTR] = NULL;
   M[$M.system_config.data.scndout_r_pk_dtct + $M.audio_proc.peak_monitor.PEAK_LEVEL] = Null;

   aux_config:
   // configure aux signal path
   r2 = $MULTI_CHAN_AUX_CHANNELS_LEFT_MASK;
   r2 = r2 OR $MULTI_CHAN_AUX_CHANNELS_RIGHT_MASK;
   r1 = r2 and r0;
   if Z jump no_aux;
   r3  = 1;
   M[$M.system_config.data.uses_aux_out] = r3;
   // Check how many aux ports
   r4 = $MULTI_CHAN_AUX_CHANNELS_LEFT_MASK;
   r4 = -r4;
   r2 = r2 LSHIFT r4;
   r1 = r1 LSHIFT r4;
   r1 = r2 -r1;
   if NZ jump check_left_aux;                   // stereo aux
   M[$M.system_config.data.aux_downmix] = NULL;
   jump wired_sub_config;

   check_left_aux:
   null = r1 - 1;
   if NZ jump check_right_aux;                  // Left only
   M[$M.system_config.data.aux_downmix] = r3;
   r1 = &$M.system_config.data.stream_map_aux_left_out;
   M[$M.system_config.data.aux_mix_obj + $M.audio_proc.stream_mixer.OFFSET_OUTPUT_PTR] = r1;

   jump wired_sub_config;

   check_right_aux:
    M[$M.system_config.data.aux_downmix] = r3;  // Right only
   r1 = &$M.system_config.data.stream_map_aux_right_out;
   M[$M.system_config.data.aux_mix_obj + $M.audio_proc.stream_mixer.OFFSET_OUTPUT_PTR] = r1;

   jump wired_sub_config;

   no_aux:
   M[$M.system_config.data.uses_aux_out] = 0;
   M[$M.system_config.data.auxout_l_pk_dtct  + $M.audio_proc.peak_monitor.PEAK_LEVEL] = Null;
   M[$M.system_config.data.auxout_r_pk_dtct  + $M.audio_proc.peak_monitor.PEAK_LEVEL] = Null;

   wired_sub_config:
   // configure subwoofer signal path
   r2 = $MULTI_CHAN_SUB_WIRED_CHANNELS_MASK;
   null = r2 and r0;
   if Z jump no_wired_sub;

   r1 = &$M.system_config.data.stream_map_sub_out;
   M[$M.system_config.data.spkr_ctrl_struct + $spkr_ctrl_system.SUB_OUTPUT_PTR] = r1;
   M[$M.system_config.data.wired_sub_channel_vol_struc + $volume_and_limit.channel.INPUT_PTR_FIELD] = r1;
   M[$M.system_config.data.wired_sub_channel_vol_struc + $volume_and_limit.channel.OUTPUT_PTR_FIELD] = r1;

   r3  = 1;
   M[$M.system_config.data.uses_wired_sub] = r3;
   r3  = 2;
   M[$M.system_config.data.spkr_ctrl_coefs + $spkr_ctrl_system.COEF_CONFIG] = r3;

#ifdef WIRELESS_SUB_ENABLE
   M[$M.system_config.data.delay_pri_left + $audio_proc.delay.DELAY_FIELD] = Null;
   M[$M.system_config.data.delay_pri_right + $audio_proc.delay.DELAY_FIELD] = Null;
   M[$M.system_config.data.delay_sec_left + $audio_proc.delay.DELAY_FIELD] = Null;
   M[$M.system_config.data.delay_sec_right + $audio_proc.delay.DELAY_FIELD] = Null;
   M[$M.system_config.data.delay_aux_left + $audio_proc.delay.DELAY_FIELD] = Null;
   M[$M.system_config.data.delay_aux_right + $audio_proc.delay.DELAY_FIELD] = Null;
   M[$M.main.samples_latency_measure + 0] = Null;
#endif

  jump done;

   no_wired_sub:

   M[$M.system_config.data.wired_sub_channel_vol_struc + $volume_and_limit.channel.INPUT_PTR_FIELD] = NULL;
   M[$M.system_config.data.wired_sub_channel_vol_struc + $volume_and_limit.channel.OUTPUT_PTR_FIELD] = NULL;
   M[$M.system_config.data.uses_wired_sub] = NULL;
   M[$M.system_config.data.sub_pk_dtct + $M.audio_proc.peak_monitor.PEAK_LEVEL] = Null;
   done:

   jump $pop_rLink_and_rts;

.ENDMODULE;
