// *****************************************************************************
// Copyright (c) 2009 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
//
// *****************************************************************************

// *****************************************************************************
// DESCRIPTION
// System Routines to run music manager
//
// *****************************************************************************

#include "stack.h"
#include "music_example.h"
#include "core_library.h"
#include "spi_comm_library.h"
#include "mips_profile.h"
#include "music_manager_config.h"
#include "cbops.h"
#include "codec_library.h"
#include "default_eq_coefs.h"

// *****************************************************************************
// MODULE:
//    $M.music_example_process
//
// DESCRIPTION:
// This routine is called from the main loop when a frame of data is ready
// to be processed.
//
// INPUTS:
//    - none
//
// OUTPUTS:
//    - none
//
// CPU USAGE:
//    CODE memory:    9  words
//    DATA memory:    2  words
// *****************************************************************************
.MODULE $M.music_example_process;
 .CODESEGMENT  MUSIC_EXAMPLE_PROCESS_PM;
 .DATASEGMENT  DM;
   .VAR  TimeStamp;
   .VAR  PeakMips;

$music_example_process:
   $push_rLink_macro;

   M[$ARITHMETIC_MODE] = NULL;

   // Check for Initialization
   NULL = M[$music_example.reinit];
   if NZ call $music_example_reinitialize;

   // Configure the dither type for all channels to the current dither setting
   r1 = M[$M.system_config.data.dithertype];
   call $multi_chan_config_dither_type;

   // Get Current System Mode
   r1 = M[$music_example.sys_mode];
   // Override Mode & CallState
   r0 = M[$music_example.SysControl];
   // r1 = mode (from above)
   r4 = M[$music_example.OvrMode];

   Null = r0 AND $M.MUSIC_MANAGER.CONTROL.MODE_OVERRIDE;
   if NZ r1 = r4;

   r2 = $M.MUSIC_MANAGER.SYSMODE.PASSTHRU;

   // Music Manager processes at the $current_dac_sampling_rate rate
   r0 = M[$current_dac_sampling_rate];

   // Force the Music Manager into pass-through mode if attempting to use an unsupported rate
   // (i.e. if the sample rate is not equal to 44.1kHz, 48kHz, [88.2kHz or 96kHz])
   NULL = r0 - 44100;
   if Z jump no_fs_force_passthru;
   NULL = r0 - 48000;
   if Z jump no_fs_force_passthru;

   // Define HI_RATE_MUSIC_MANAGER_SUPPORT to allow Music Manager support at hi-rates
#define HI_RATE_MUSIC_MANAGER_SUPPORT
#ifdef HI_RATE_MUSIC_MANAGER_SUPPORT
   NULL = r0 - 88200;
   if Z jump no_fs_force_passthru;
   NULL = r0 - 96000;
   if Z jump no_fs_force_passthru;
#endif

   // Set r1 = PASSTHRU if fs not equal to 44.1kHz, 48kHz, 88.2kHz or 96kHz
   r1 = r2;
no_fs_force_passthru:
   M[$music_example.CurMode] = r1;

   // Call processing table that corresponds to the current mode
   r4 = M[$M.system_config.data.mode_table + r1];
   call $frame_sync.run_function_table;

   // Store Function MIPS
   r0 = M[$FunctionMips_data_block + $mips_profile.MIPS.MAIN_CYCLES_OFFSET];
   M[&$music_example.PeakMipsFunc] = r0;

   // Store the VM supplied sampling rate for the statistics delivery
   r0 = M[$current_codec_sampling_rate];
   M[&$music_example.SamplingRate] = r0;

#ifdef APTX_ENABLE
   // Get the connection type and copy stats if processing APTX
   r0 = M[$app_config.io];
   null = r0 - $APTX_IO;
   if Z call $M.music_example.aptx_stats_update.func;
   r0 = M[$app_config.io];
   null = r0 - $APTXHD_IO;
   if Z call $M.music_example.aptx_stats_update.func;
#endif

#ifdef APTX_ACL_SPRINT_ENABLE
   // Get the connection type and copy stats if processing APTX ACL SPRINT
   r0 = M[$app_config.io];
   null = r0 - $APTX_ACL_SPRINT_IO;
   if Z call $M.music_example.aptx_stats_update.func;
#endif

   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.music_example_reinitialize
//
// DESCRIPTION:
// This routine is called by music_example_process when the algorithm needs to
// be reinitialized.
//
// INPUTS:
//    - none
//
// OUTPUTS:
//    - none
//
// CPU USAGE:
//    CODE memory:    6  words
//    DATA memory:    0  words
// *****************************************************************************
.MODULE $M.music_example_reinitialize;
 .CODESEGMENT MUSIC_EXAMPLE_REINIT_PM;

$music_example_reinitialize:
   $push_rLink_macro;

// Transfer Parameters to Modules.
// Assumes at least one value is copied
   M1 = 1;
   I0 = &$M.system_config.data.ParameterMap;
   // Get Source (Pre Load)
   r0 = M[I0,M1];
lp_param_copy:
      // Get Destination
      r1 = M[I0,M1];
      // Read Source,
      r0 = M[r0];
      // Write Destination,  Get Source (Next)
      M[r1] = r0, r0 = M[I0,M1];
      // Check for NULL termination
      Null = r0;
   if NZ jump lp_param_copy;

   // Copy current config word to codec specific config word so they're synchronized
   r0 = M[&$M.system_config.data.CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_CONFIG];

   // Set the codec specific config word
   r2 = M[$codec_config];
   M[&$M.system_config.data.CurParams + r2] = r0;

   // Tell VM the current EQ Bank
   r2 = $music_example.VMMSG.CUR_EQ_BANK;
   r3 = r0 AND $M.MUSIC_MANAGER.CONFIG.USER_EQ_SELECT;
   call $message.send_short;

    // new config word into MSW and LSW and send back to VM
    r0 = M[&$M.system_config.data.CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_CONFIG];
    r2 = $music_example.VMMSG.SETCONFIG_RESP ;
    r3 = r0 LSHIFT -16 ;
    r4 = r0 AND 0xFFFF ;
    call $message.send_short ;


// Call Module Initialize Functions
   r4 = &$M.system_config.data.reinitialize_table;
   call $frame_sync.run_function_table;

// Clear Reinitialization Flag
   M[$music_example.reinit]    = NULL;
   jump $pop_rLink_and_rts;
.ENDMODULE;

#if uses_ANC_EQ
// *****************************************************************************
// MODULE:
//    $M.music_example.anc_eq.process
//
// DESCRIPTION:
//    front end for ANC'speq's.
//
// INPUTS:
//    - r7 = pointer to peq object
//    - r8 = pointer to bypass flag
//
// OUTPUTS:
//    - none
//
//
// *****************************************************************************
.MODULE $music_example.anc_eq;
   .CODESEGMENT MUSIC_EXAMPLE_PEQ_PM;
.DATASEGMENT DM;
//------------------------------------------------------------------------------
process:
//------------------------------------------------------------------------------
// anc flag status
// - read vcalue of ANC active flag
//------------------------------------------------------------------------------
// on entry r7 = pointer to filter object (used by audio_proc.hq_peq.process)
//          r8 = bypass flag
//------------------------------------------------------------------------------
   // if ANC Mode not active then No EQ
    Null = M[$ancMode];
    if Z rts;
    jump $music_example.peq.process;
.ENDMODULE;
#endif //uses_ANC_EQ
// *****************************************************************************
// MODULE:
//    $M.music_example.peq.process
//
// DESCRIPTION:
//    front end for peq's. Exits if 0-stage.
//
// INPUTS:
//    - r7 = pointer to peq object
//    - r8 = pointer to bank select object
//
// OUTPUTS:
//    - none
//
//
// *****************************************************************************

.MODULE $music_example.peq;
   .CODESEGMENT MUSIC_EXAMPLE_PEQ_PM;

.DATASEGMENT DM;

//------------------------------------------------------------------------------
initialize:
//------------------------------------------------------------------------------
// initialise parametric filter
// - if user_eq, then need to select filter bank, and adjust it depending on
//   sample rate.  If filter bank is zero, then don't update coefficients as
//   filtering is put into bypass by peq processing wrapper (below).
//   Bank 0 means flat curve, but is only valid if EQFLAT is enabled.
//   Bank 1 means use the peq 1 for 44.1 kHz or peq 7 for 48 kHz etc...
// - if not user_eq, then force filter bank to zero.  sample rate bank switch is
//   still performed.
//------------------------------------------------------------------------------
// on entry r7 = pointer to filter object
//          r8 = pointer to bank selection object
//------------------------------------------------------------------------------

    r0 = M[&$M.system_config.data.CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_CONFIG];
    r5 = M[r8];     // Number of banks per sample rate

    // running user_eq so get selected bank number
    // speaker and base boost only have one bank
    r3 = r0 and $M.MUSIC_MANAGER.CONFIG.USER_EQ_SELECT;
    NULL = r5 - 1;
    if Z r3=Null;

    // Use sample rate to update bank
    r1 = M[$current_dac_sampling_rate];
    Null = r1 - 48000;
    if Z r3 = r3 + r5;

    // Access the requested Bank
    // PARAM_PTR_FIELD=Null for no Peq
    r8 = r8 + 1;
    r0 = M[r8 + r3];
    M[r7 + $audio_proc.peq.PARAM_PTR_FIELD] = r0;
    if Z rts;
    jump $audio_proc.hq_peq.initialize;


//------------------------------------------------------------------------------
process:
//------------------------------------------------------------------------------
// peq processing wrapper
// - return without processing if bypassed
// - if running user_eq (BYPASS_BIT_MASK_FIELD == USER_EQ_BYPASS)
//     then check whether user eq bank is 0
//------------------------------------------------------------------------------
// on entry r7 = pointer to filter object (used by audio_proc.hq_peq.process)
//          r8 = bypass mask
//------------------------------------------------------------------------------

    r0 = M[&$M.system_config.data.CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_CONFIG];


    // check if EQ is bypassed
    null = r0 and r8;
    if NZ rts;

    // if Parameters is Null then no Peq
    Null = M[r7 + $audio_proc.peq.PARAM_PTR_FIELD];
    if Z rts;

    jump $audio_proc.hq_peq.process;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $music_example.mix
//
// DESCRIPTION:
//   Function to copy data from one buffer into another.
//
// INPUTS:
//   r7 -  data object containing pointers to input and output data.  Also
//         contains the number of samples to process.
//
// OUTPUTS:
//   - none
//
// CPU USAGE:
//   CODE memory:   27  words
//   DATA memory:    0  words (externally defined in data object)
// *****************************************************************************

.MODULE $music_example.mix;
 .codesegment MUSIC_EXAMPLE_MIX_PM;
process:

// get data from data object
   I3 = r7;
   M1 = 1;
   r0 = M[I3, M1];
   I0 = r0,   r0 = M[I3,M1];  // INPUT_CH1_PTR_BUFFER_FIELD
   L0 = r0,   r0 = M[I3,M1];  // INPUT_CH1_CIRCBUFF_SIZE_FIELD
   I4 = r0,   r0 = M[I3,M1];  // INPUT_CH2_PTR_BUFFER_FIELD
   L4 = r0,   r0 = M[I3,M1];  // INPUT_CH2_CIRCBUFF_SIZE_FIELD
   I1 = r0,   r0 = M[I3,M1];  // OUTPUT_PTR_BUFFER_FIELD
   L1 = r0,   r0 = M[I3,M1];  // OUTPUT_CIRCBUFF_SIZE_FIELD
   r3 = r0,   r0 = M[I3,M1];  // INPUT_CH1_GAIN_FIELD
   r4 = r0,   r0 = M[I3,M1];  // INPUT_CH2_GAIN_FIELD
   r10 = r0;                  // NUM_SAMPLES

// ADC-> SCO OUT  : SCO IN -> DAC
   do loop_copy_data;
      r0 = M[I0,1], r2 = M[I4,1];
      rMAC = r0 * r3;
      rMAC = rMAC + r2 * r4;
      M[I1,1] = rMAC;
   loop_copy_data:

// Update data object
   I3 = r7;
   M2 = 2;
   r0 = I0;
   r0 = I4,  M[I3,M2] = r0;   // INPUT_CH1_PTR_BUFFER_FIELD
   r0 = I1,  M[I3,M2] = r0;   // INPUT_CH2_PTR_BUFFER_FIELD
             M[I3,M2] = r0;   // OUTPUT_CH1_PTR_BUFFER_FIELD


// Clear L registers
   L0 = 0;
   L1 = 0;
   L4 = 0;
   L5 = 0;
   rts;
.ENDMODULE;


// *****************************************************************************
// MODULE:
//    $M.music_example.send_ready_msg
//
// DESCRIPTION:
//    This function sends a ready message to the VM application signifying that
//    it is okay for the VM application to connect streams to the kalimba.  The
//    application needs to call this function just prior to scheduling the audio
//    interrupt handler.
//
// INPUTS:
//    none
//
// OUTPUTS:
//    none
//
// CPU USAGE:
//    cycles =
//    CODE memory:    5  words
//    DATA memory:    4  words
// *****************************************************************************
.MODULE $M.music_example.power_up_reset;
 .DATASEGMENT    DM;
 .CODESEGMENT    MUSIC_EXAMPLE_POWER_UP_RESET_PM;

// Entries can be added to this table to suit the system being developed.
   .VAR  message_handlers[] =
// Message Struc Ptr  Message ID  Message Handler  Registration Function
   &$M.music_example_message.set_plugin_message_struc,     $music_example.VMMSG.SETPLUGIN,               &$M.music_example_message.SetPlugin.func,    $message.register_handler,
   &$M.music_example_message.set_mode_message_struc,       $music_example.VMMSG.SETMODE,                 &$M.music_example_message.SetMode.func,      $message.register_handler,
   &$M.music_example_message.set_config_message_struc,     $music_example.VMMSG.SETCONFIG,               &$M.music_example_message.SetConfig.func,    $message.register_handler,

   &$M.music_example_message.load_params_message_struc,    $music_example.VMMSG.LOADPARAMS,              &$M.music_example.LoadParams.func,           $message.register_handler,
   &$M.music_example_message.signal_detect_message_struct, $music_example.VMMSG.SIGNAL_DETECT_SET_PARMS, &$M.music_example_message.SignalDetect.func, $message.register_handler,
   &$M.music_example_message.soft_mute_message_struct,     $music_example.VMMSG.SOFT_MUTE,               &$M.music_example_message.SoftMute.func,     $message.register_handler,

#if defined(LATENCY_REPORTING)
   &$M.configure_latency_reporting.message_struct,         $VMMSG.CONFIGURE_LATENCY_REPORTING,           &$M.configure_latency_reporting.func,        $message.register_handler,
#endif

   &$M.music_example_message.set_user_eq_param_message_struct,         $music_example.GAIAMSG.SET_USER_PARAM,          $M.music_example_message.SetUserEqParamMsg.func,        $message.register_handler,
   &$M.music_example_message.get_user_eq_param_message_struct,         $music_example.GAIAMSG.GET_USER_PARAM,          $M.music_example_message.GetUserEqParamMsg.func,        $message.register_handler,
   &$M.music_example_message.set_user_eq_group_param_message_struct,   $music_example.GAIAMSG.SET_USER_GROUP_PARAM,    $M.music_example_message.SetUserEqGroupParamMsg.func,   $message.register_handler,
   &$M.music_example_message.get_user_eq_group_param_message_struct,   $music_example.GAIAMSG.GET_USER_GROUP_PARAM,    $M.music_example_message.GetUserEqGroupParamMsg.func,   $message.register_handler,

#if defined(APTX_ENABLE) || defined(APTX_ACL_SPRINT_ENABLE)
   &$M.music_example_message.security_message_struc,       $music_example.VMMSG.APTX_SECURITY,           &$M.music_example.AptxSecurityStatus.func,   $message.register_handler,
#endif

   &$M.music_example_message.set_resolution_modes_message_struc,       $M.music_example.VMMSG.SET_RESOLUTION_MODES,    &$M.music_example_message.SetResolutionModes.func,      $message.register_handler,
   &$M.music_example_message.set_i2s_mode_message_struc,               $M.music_example.VMMSG.SET_I2S_MODE,            &$M.music_example_message.SetI2SMode.func,              $message.register_handler,
   &$M.music_example_message.set_anc_mode_message_struc,               $M.music_example.VMMSG.SET_ANC_MODE,            &$M.music_example_message.SetANCMode.func,              $message.register_handler,

   &$M.music_example_spi.status_message_struc,             $M.music_example.SPIMSG.STATUS,               &$M.music_example.GetStatus.func,            $spi_comm.register_handler,
   &$M.music_example_spi.version_message_struc,            $M.music_example.SPIMSG.VERSION,              &$M.music_example.GetVersion.func,           $spi_comm.register_handler,
   &$M.music_example_spi.control_message_struc,            $M.music_example.SPIMSG.CONTROL,              &$M.music_example.GetControl.func,           $spi_comm.register_handler,
   &$M.music_example_spi.reinit_message_struc,             $M.music_example.SPIMSG.REINIT,               &$M.music_example.ReInit.func,               $spi_comm.register_handler,
   &$M.music_example_spi.parameter_message_struc,          $M.music_example.SPIMSG.PARAMS,               &$M.music_example.GetParams.func,            $spi_comm.register_handler,
#ifdef SPDIF_ENABLE
   &$M.music_example_spi.spdif_config_message_struc,       $M.music_example.SPIMSG.SPDIF_CONFIG,         &$M.music_example.GetSpdifConfig.func,         $spi_comm.register_handler,
#endif

   // Replacement message configuration messages
   &$M.music_example_message.set_output_dev_type_s_message_struc,       $M.music_example.VMMSG.SET_OUTPUT_DEV_TYPE_S,      &$M.music_example_message.SetOutputDevType_s.func,       $message.register_handler,
   &$M.music_example_message.multi_channel_main_mute_s_message_struc,   $M.music_example.VMMSG.MULTI_CHANNEL_MAIN_MUTE_S,  &$M.music_example_message.MultiChannelMainMute_s.func,   $message.register_handler,
   &$M.music_example_message.multi_channel_aux_mute_s_message_struc,    $M.music_example.VMMSG.MULTI_CHANNEL_AUX_MUTE_S,   &$M.music_example_message.MultiChannelAuxMute_s.func,    $message.register_handler,
   &$M.music_example_message.multi_volume_s_message_struc,              $M.music_example.VMMSG.VOLUME_S,                   &$M.music_example_message.MainVolume_s.func,             $message.register_handler,
   &$M.music_example_message.aux_volume_s_message_struc,                $M.music_example.VMMSG.AUXVOLUME_S,                &$M.music_example_message.AuxVolume_s.func,              $message.register_handler,
   0;

$music_example.power_up_reset:
   $push_rLink_macro;

   // Copy default parameters into current parameters
   call $M.music_example.load_default_params.func;

   r4 = &message_handlers;
   call $frame_sync.register_handlers;
   jump $pop_rLink_and_rts;
.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.music_example.load_default_params
//
// DESCRIPTION:
//    This function copies the (packed) default parameter values into the current
//    parameters block...
//
//    Packing format should be three 16-bit words to two 24-bit words
//    eg: 1234 5678 9abc def0 to 123456,789abc
//
//    Throughput is 2 outputs/8 cycles
//
// INPUTS:
//    NONE
//
// OUTPUTS:
//    loads Default parameter values into CurParams block.
//
// TRASHED REGISTERS:
//    r0,r2,r3,r4,L0,I0,I1,I4,r10,Loop
//
// CPU USAGE:
//    cycles =
//    CODE memory:     18 words
//    DATA memory:     5 words
//
// Note:
//    LENGTH($M.system_config.data.CurParams) must be even
//
// *****************************************************************************
.MODULE $M.music_example.load_default_params;


   .CODESEGMENT MUSIC_EXAMPLE_LOAD_DEFAULT_PARAMS_PM;
   .DATASEGMENT DM;
   .VAR/DM1CIRC operatorvals[] = 8,0x00ff00,-8,0x00ffff,16;

func:
   L0 = LENGTH(operatorvals);
   I0 = &operatorvals;
   I4 = &$M.system_config.data.DefaultParameters;
   I1 = &$M.system_config.data.CurParams;
   r10 = LENGTH($M.system_config.data.CurParams);
#if 1 // compiler bug: should divide above by two @ compile time
   r10 = r10 ASHIFT -1;
#endif
   r4 = M[I0, 1], r0 = M[I4, 1];  // load 8, load 0x1234
   do three16_to_two24_loop;
      r0 = r0 LSHIFT r4, r4 = M[I0, 1], r2 = M[I4, 0]; // load 0x5678,load mask
      r2 = r2 AND r4, r4 = M[I0, 1];    // mask sign bits, load -8
      r2 = r2 LSHIFT r4, r4 = M[I0, 1], r3 = M[I4, 1]; // load mask, load 0x5678
      r0 = r0 OR r2,               r2 = M[I4, 1]; // load 0x9abc, word1 done
      r2 = r2 AND r4, r4 = M[I0, 1];      // clear upper bits of word2, load 16
      r3 = r3 LSHIFT r4, M[I1, 1] = r0;   // store word1
      r3 = r3 OR r2,     r4 = M[I0, 1], r0 = M[I4, 1]; // load 8, load 0xdef0
      M[I1, 1] = r3;  // word2 done, store word2
   three16_to_two24_loop:
   L0 = 0;
   rts;

.ENDMODULE;

#ifdef FASTSTREAM_ENABLE
// *****************************************************************************
// MODULE:
//    $M.music_example.extract_faststream_info
//
// DESCRIPTION:
//    utility function to extract some info from codec, this info is
//    required when sending Statistic struct to music manager
//
// INPUTS:
//    NONE
//
// OUTPUTS:
//    NONE
//
// TRASHED REGISTERS:
//    r0
// *****************************************************************************
.MODULE $M.music_example.extract_faststream_info;

   .CODESEGMENT MUSIC_EXAMPLE_EXTRACT_FASTSTREAM_INFO_PM;
func:
   // get info only when a frame successfully decoded
   r0 = M[&$decoder_codec_stream_struc + $codec.av_decode.MODE_FIELD];
   Null = r0 - $codec.SUCCESS;
   if NZ rts;

   // copy sampling rate and bitbool for just decoded frame
   r0 = M[$sbc.sampling_freq];
   M[&$music_example.dec_sampling_freq] = r0;
   r0 = M[$sbc.bitpool];
   M[&$music_example.dec_bitpool] = r0;
   rts;
.ENDMODULE;
#endif

// *****************************************************************************
// MODULE:
//    $M.music_example.aptx_stats_update
//
// DESCRIPTION:
//    utility function to collect some info from aptX codec, this info is
//    required when sending Statistic struct to music manager
//
// INPUTS:
//    NONE
//
// OUTPUTS:
//    NONE
//
// TRASHED REGISTERS:
//    r0
// *****************************************************************************
#if defined(APTX_ENABLE) || defined(APTX_ACL_SPRINT_ENABLE)
.MODULE $M.music_example.aptx_stats_update;

   .CODESEGMENT PM;
   .DATASEGMENT DM;

   .VAR  $aptx_security_status = 0;
   .VAR  $aptx_decoder_version = 0;

func:
   // Copy aptX statistics into music example block.

   // Always use stereo mode
   r0 = 2;
   M[$music_example.aptx_channel_mode] = r0;

   // Use the values set from the VM (security message)
   r0 = M[$aptx_security_status];
   M[$music_example.aptx_security_status] = r0;
   r0 = M[$aptx_decoder_version];
   M[$music_example.aptx_decoder_version] = r0;

   rts;
.ENDMODULE;

#endif
// *****************************************************************************
// MODULE:
//    $M.music_example
//
// DESCRIPTION:
//    music_example data object.
//
// *****************************************************************************
.MODULE $M.MUSIC_EXAMPLE_VERSION_STAMP;
   .DATASEGMENT DM;
   .BLOCK VersionStamp;
   .VAR  h1 = 0xbeef;
   .VAR  h2 = 0xbeef;
   .VAR  h3 = 0xbeef;
   .VAR  SysID = $MUSIC_MANAGER_SYSID;
   .VAR  BuildVersion = MUSIC_EXAMPLE_VERSION;
   .VAR  h4 = 0xbeef;
   .VAR  h5 = 0xbeef;
   .VAR  h6 = 0xbeef;
   .ENDBLOCK;
.ENDMODULE;

#define MUSIC_MANAGER_CONFIG_FLAG_RAW                                   \
    ( (flag_uses_SPEAKER_CROSSOVER * uses_SPEAKER_CROSSOVER)            \
    + (flag_uses_SPKR_EQ * uses_SPKR_EQ)                                \
    + (flag_uses_BASS_BOOST * uses_BASS_BOOST)                          \
    + (flag_uses_BASS_PLUS * uses_BASS_PLUS)                            \
    + (flag_uses_USER_EQ * uses_USER_EQ)                                \
    + (flag_uses_STEREO_ENHANCEMENT * uses_STEREO_ENHANCEMENT)          \
    + (flag_uses_3DV * uses_3DV)                                        \
    + (flag_uses_DITHER * uses_DITHER)                                  \
    + (flag_uses_COMPANDER * uses_COMPANDER)                            \
    + (flag_uses_VOLUME_CONTROL * uses_VOLUME_CONTROL)                  \
    + (flag_uses_SIGNAL_DETECTION * uses_SIGNAL_DETECTION)              \
    + (flag_uses_WIRED_SUB_EQ * uses_WIRED_SUB_EQ)                      \
    + (flag_uses_WIRED_SUB_COMPANDER * uses_WIRED_SUB_COMPANDER)        \
    + (flag_uses_SPKR_EQ_RAW_COEFS * USE_PRECALCULATED_SPKR_COEFS)      \
    + (flag_uses_BASS_BOOST_RAW_COEFS * USE_PRECALCULATED_BOOST_COEFS)  \
    + (flag_uses_USER_EQ_RAW_COEFS * USE_PRECALCULATED_USER_COEFS) )

// System Configuration is saved in kap file.
.MODULE $M.MUSIC_EXAMPLE_MODULES_STAMP;
   .DATASEGMENT DM;
   .BLOCK ModulesStamp;
      .VAR  s1 = 0xfeeb;
      .VAR  s2 = 0xfeeb;
      .VAR  s3 = 0xfeeb;
      .VAR  CompConfig = MUSIC_MANAGER_CONFIG_FLAG_RAW;
      .VAR  s4 = 0xfeeb;
      .VAR  s5 = 0xfeeb;
      .VAR  s6 = 0xfeeb;
   .ENDBLOCK;
.ENDMODULE;

.MODULE $music_example;
 .DATASEGMENT DM;
   .VAR  Version    = MUSIC_EXAMPLE_VERSION;
   .VAR  sys_mode   = $M.MUSIC_MANAGER.SYSMODE.FULLPROC;
   .VAR  reinit     = $music_example.REINITIALIZE;
   .VAR  frame_processing_size = $music_example.NUM_SAMPLES_PER_FRAME;
   .VAR  config_raw = MUSIC_MANAGER_CONFIG_FLAG_RAW;
   .VAR  config_anc = (MUSIC_MANAGER_CONFIG_FLAG_RAW + (flag_uses_ANC_EQ * uses_ANC_EQ));
// SPI System Control
.BLOCK SpiSysControl;
   // Bit-wise flag for tuning control
   .VAR  SysControl = 0;
   // override System Volumes
   .VAR  OvrSystemVolume = 0x0009;
   .VAR  OvrCallState = 0;
   .VAR  OvrMode    = 0;

   // override Aux Master volumes
   .VAR  AuxOvrMasterVolumes = 0x0000;
   // override Aux Left & Right Trim volumes
   .VAR  AuxOvrTrimVolumes = 0x0000;

   // override Main Master volumes
   .VAR  MainOvrMasterVolumes = 0x0000;
   // override Primary Left & Right Trim volumes
   .VAR  PriOvrTrimVolumes = 0x0000;
   // override Secondary Left & Right Trim volumes
   .VAR  SecOvrTrimVolumes = 0x0000;
   // override Subwoofer Trim volumes
   .VAR  SubOvrTrimVolumes = 0x0000;
.ENDBLOCK;


   .VAR  SystemVolume = 11;                  // system volume (index)

   .VAR  Aux.MasterVolume = 0;               // Master volume (dB/60)
   .VAR  Aux.ToneVolume = 0;                 // Tone volume (dB/60)
   .VAR  Aux.LeftTrimVolume = 0;             // Aux Left trim volume (dB/60)
   .VAR  Aux.RightTrimVolume = 0;            // Aux Right trim volume(dB/60)


   .VAR  Main.MasterVolume = 0;              // Master volume (dB/60)
   .VAR  Main.ToneVolume = 0;                // Tone volume (dB/60)
   .VAR  Main.PrimaryLeftTrimVolume = 0;     // Primary Left trim volume (dB/60)
   .VAR  Main.PrimaryRightTrimVolume = 0;    // Primary Right trim volume(dB/60)
   .VAR  Main.SecondaryLeftTrimVolume = 0;   // Secondary Left trim volume (dB/60)
   .VAR  Main.SecondaryRightTrimVolume = 0;  // Secondary Right trim volume(dB/60)
   .VAR  Main.SubTrimVolume = 0;                 // Sub trim volume (dB/60)

   .VAR  DAC_IF_Connections = 0;
   .VAR  SPDIF_IF_Connections = 0;
   .VAR  I2S_IF_Connections = 0;
   .VAR  OTA_IF_Connections = 0;


.BLOCK Statistics;
   .VAR  CurMode            = 0;
   .VAR  PeakMipsFunc       = 0;
   .VAR  PeakMipsDecoder    = 0;
   .VAR  SamplingRate       = 0;

#ifdef FASTSTREAM_ENABLE
   .VAR  dec_sampling_freq;
   .VAR  dec_bitpool;
   .VAR  PeakMipsEncoder;
#endif
#if defined(APTX_ENABLE) || defined(APTX_ACL_SPRINT_ENABLE)
   .VAR  aptx_channel_mode;
   .VAR  aptx_security_status = 0;
   .VAR  aptx_decoder_version;
#endif
.ENDBLOCK;

.ENDMODULE;
