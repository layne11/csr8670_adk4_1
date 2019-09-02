// *****************************************************************************
// Copyright (c) 2008 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
//
// *****************************************************************************


#include "music_example.h"
#include "stack.h"
#include "pskey.h"
#include "message.h"
#include "cbops_library.h"
#include "multichannel_output.h"

// VM Message Handlers
.MODULE $M.music_example_message;
   .DATASEGMENT DM;
   .VAR set_plugin_message_struc[$message.STRUC_SIZE];
   .VAR set_mode_message_struc[$message.STRUC_SIZE];
   .VAR set_param_message_struc[$message.STRUC_SIZE];
   .VAR set_config_message_struc[$message.STRUC_SIZE];
   .VAR ping_message_struc[$message.STRUC_SIZE];
   .VAR get_param_message_struc[$message.STRUC_SIZE];
   .VAR load_params_message_struc[$message.STRUC_SIZE];
#if defined(APTX_ENABLE) || defined(APTX_ACL_SPRINT_ENABLE)
   .VAR security_message_struc[$message.STRUC_SIZE];
#endif

   .VAR set_resolution_modes_message_struc[$message.STRUC_SIZE];
   .VAR set_i2s_mode_message_struc[$message.STRUC_SIZE];
   .VAR set_anc_mode_message_struc[$message.STRUC_SIZE];
   .VAR ps_key_struc[$pskey.STRUC_SIZE];
   .VAR signal_detect_message_struct[$message.STRUC_SIZE];
   .VAR soft_mute_message_struct[$message.STRUC_SIZE];
   .VAR set_user_eq_param_message_struct[$message.STRUC_SIZE];
   .VAR get_user_eq_param_message_struct[$message.STRUC_SIZE];
   .VAR set_user_eq_group_param_message_struct[$message.STRUC_SIZE];
   .VAR get_user_eq_group_param_message_struct[$message.STRUC_SIZE];

   // Replacement message configuration message structures
   .VAR set_output_dev_type_s_message_struc[$message.STRUC_SIZE];
   .VAR multi_volume_s_message_struc[$message.STRUC_SIZE];
   .VAR aux_volume_s_message_struc[$message.STRUC_SIZE];
   .VAR multi_channel_main_mute_s_message_struc[$message.STRUC_SIZE];
   .VAR multi_channel_aux_mute_s_message_struc[$message.STRUC_SIZE];

.ENDMODULE;

.MODULE $M.music_example_message_payload_cache;
   .DATASEGMENT DM;
   .CONST $message.PAYLOAD_CACHE_SIZE       10;                   // Enough space for the largest expected long message payload

   // Replacement message configuration message structures
   .VAR set_output_dev_type_s[$message.PAYLOAD_CACHE_SIZE];
   .VAR multi_channel_main_mute_s[$message.PAYLOAD_CACHE_SIZE];
   .VAR multi_channel_aux_mute_s[$message.PAYLOAD_CACHE_SIZE];
   .VAR multi_volume_s[$message.PAYLOAD_CACHE_SIZE];
   .VAR aux_volume_s[$message.PAYLOAD_CACHE_SIZE];

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.music_example_message.SetPlugin
//
// FUNCTION
//    $M.music_example_message.SetPlugin.func
//
// DESCRIPTION:
//    Handle the set plugin VM message
//    (this sets the connection type)
//
// INPUTS:
//    r1 = connection type:
//       SBC_DECODER          = 1
//       MP3_DECODER          = 2
//       AAC_DECODER          = 3
//       FASTSTREAM_SINK      = 4
//       USB_DECODER          = 5
//       APTX_DECODER         = 6
//       ANALOGUE_DECODER     = 7
//       APTX_SPRINT_DECODER  = 8
//       SPDIF_DECODER        = 9
//       I2S_DECODER          = 10
// OUTPUTS:
//    none
//
// TRASHES: r0, r1
//
// *****************************************************************************
.MODULE $M.music_example_message.SetPlugin;

   .CODESEGMENT   PM;

func:

   // Allow only the first message ($app_config.io is initialised to -1)
   null = M[$app_config.io];
   if POS rts;

   // Set the plugin type
   M[$app_config.io] = r1;

   // Set up the codec_type and codec_config for the music manager
#ifdef USB_ENABLE
   null = r1 - $USB_IO;
   if NZ jump skip_usb;

      r0 = $music_example.USB_CODEC_TYPE;
      r1 = $music_example.USB_CODEC_CONFIG;

      jump set_mm_codec_info;

   skip_usb:
#endif

#ifdef ANALOGUE_ENABLE
   null = r1 - $ANALOGUE_IO;
   if NZ jump skip_analogue;

      r0 = $music_example.ANALOGUE_CODEC_TYPE;
      r1 = $music_example.ANALOGUE_CODEC_CONFIG;

      jump set_mm_codec_info;

   skip_analogue:
#endif

#ifdef I2S_ENABLE
   null = r1 - $I2S_IO;
   if NZ jump skip_i2s;

      r0 = $music_example.I2S_CODEC_TYPE;
      r1 = $music_example.I2S_CODEC_CONFIG;

      jump set_mm_codec_info;

   skip_i2s:
#endif

#ifdef SBC_ENABLE
   null = r1 - $SBC_IO;
   if NZ jump skip_sbc;

      r0 = $music_example.SBC_CODEC_TYPE;
      r1 = $music_example.SBC_CODEC_CONFIG;

      jump set_mm_codec_info;

   skip_sbc:
#endif

#ifdef MP3_ENABLE
   null = r1 - $MP3_IO;
   if NZ jump skip_mp3;

      r0 = $music_example.MP3_CODEC_TYPE;
      r1 = $music_example.MP3_CODEC_CONFIG;

      jump set_mm_codec_info;

   skip_mp3:
#endif

#ifdef FASTSTREAM_ENABLE
   null = r1 - $FASTSTREAM_IO;
   if NZ jump skip_faststream;

      r0 = $music_example.FASTSTREAM_CODEC_TYPE;
      r1 = $music_example.FASTSTREAM_CODEC_CONFIG;

      jump set_mm_codec_info;

   skip_faststream:
#endif

#ifdef APTX_ENABLE
   null = r1 - $APTX_IO;
   if NZ jump skip_aptx;

      r0 = $music_example.APTX_CODEC_TYPE;
      r1 = $music_example.APTX_CODEC_CONFIG;

      jump set_mm_codec_info;

   skip_aptx:

   null = r1 - $APTXHD_IO;
   if NZ jump skip_aptxhd;

      r0 = $music_example.APTXHD_CODEC_TYPE;
      r1 = $music_example.APTXHD_CODEC_CONFIG;

      jump set_mm_codec_info;

   skip_aptxhd:
#endif

#ifdef APTX_ACL_SPRINT_ENABLE
   null = r1 - $APTX_ACL_SPRINT_IO;
   if NZ jump skip_aptx_acl_sprint;

      r0 = $music_example.APTX_ACL_SPRINT_CODEC_TYPE;
      r1 = $music_example.APTX_ACL_SPRINT_CODEC_CONFIG;

      jump set_mm_codec_info;

   skip_aptx_acl_sprint:
#endif

#ifdef AAC_ENABLE
   null = r1 - $AAC_IO;
   if NZ jump skip_aac;

      r0 = $music_example.AAC_CODEC_TYPE;
      r1 = $music_example.AAC_CODEC_CONFIG;

      jump set_mm_codec_info;

   skip_aac:

   null = r1 - $AAC_ELD_IO;
   if NZ jump skip_eld_aac;

      r0 = $music_example.AAC_ELD_CODEC_TYPE;
      r1 = $music_example.AAC_ELD_CODEC_CONFIG;

      jump set_mm_codec_info;

   skip_eld_aac:

#endif

#ifdef SPDIF_ENABLE
   null = r1 - $SPDIF_IO;
   if NZ jump skip_spdif;

      r0 = $music_example.SPDIF_CODEC_TYPE;
      r1 = $music_example.SPDIF_CODEC_CONFIG;

      jump set_mm_codec_info;

   skip_spdif:
#endif

   // Unknown codec
   jump $error;

   set_mm_codec_info:

      // Set the Music Manager codec type and codec config
      M[$codec_type] = r0;
      M[$codec_config] = r1;

   rts;
.ENDMODULE;

// *****************************************************************************
// DESCRIPTION: $MsgMusicSetMode
//       handle mode change
//  r1 = processing mode
//  r2 = eq bank state (TODO: to which modes does this apply?)
//       0 = do not advance to next EQ bank
//       1 = advance to next EQ bank
//       2 = use eq Bank that is specified in r3
//  r3 = eq bank (only used if r2 = 2)
// *****************************************************************************
.MODULE $M.music_example_message.SetMode;

   .CODESEGMENT   MUSIC_EXAMPLE_VM_SETMODE_PM;
func:
   Null = r2; /* TODO see if the plugin is doing this initially */
   if Z jump do_not_advance_to_next_eq_bank;
      r4 = $M.MUSIC_MANAGER.CONFIG.USER_EQ_SELECT;

      // get number of EQ banks in use
      r5 = M[&$M.system_config.data.CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_USER_EQ_NUM_BANKS];
      r5 = r5 and r4;

      // get the current EQ bank and advance to next
      r0 = M[&$M.system_config.data.CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_CONFIG];
      r6 = r0 AND r4;
      r6 = r6 + 1;

      // use specified index if r2==2
      Null = r2 - 2;
      if Z r6 = r3;

      // If EQFLAT bit is one it means a flat curve has been added to the system.
      // The flat curve is used when bank==0;
      // If EQFLAT bit is zero AND bank==0, bank must be forced to one.
      r8 = $M.MUSIC_MANAGER.CONFIG.EQFLAT;
      r3 = 0;
      r7 = 1;
      Null = r0 AND r8;     // is zero if flat curve not allowed (i.e. Bank0 not allowed)
      if Z r3 = r7;
      NULL = r5 - r6;
      if LT r6 = r3;        // reset index to 0 or 1 depending on EQFLAT

      // If the VM sent r2=2 and r3=0, use Bank1 if a flat curve isn't included
      Null = r6 - 0;
      if Z r6 = r3;

      // update EQ bank bits of Music Manager Config Parameter
      r7 = 0xffffff XOR r4;
      r7 = r0 AND r7;
      r6 = r7 OR r6;
      M[&$M.system_config.data.CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_CONFIG] = r6;

      // User has requested a new EQ bank, but shouldn't need to call
      // coefficient calculation routine here as "reinit" is requested.

   do_not_advance_to_next_eq_bank:

   // ensure mode is valid
   r3 = $M.MUSIC_MANAGER.SYSMODE.MAX_MODES;
   Null = r3 - r1;
   if NEG r1 = r3;
   r3 = $M.MUSIC_MANAGER.SYSMODE.STANDBY;
   Null = r3 - r1;
   if POS r1 = r3;
   // save mode
   M[$music_example.sys_mode] = r1;
   // Re-init because mode or EQ setting has changed
   r1 = 1;
   M[$music_example.reinit] = r1;
   rts;
.ENDMODULE;

// *****************************************************************************
// DESCRIPTION: $MsgMusicSetParameter
//       handle set parameter
//  r1 = Parameter ID
//  r2 = Parameter MSW
//  r3 = Parameter LSW
//  r4 = Status (NZ=done)
// *****************************************************************************
.MODULE $M.music_example_message.SetParam;

   .CODESEGMENT MUSIC_EXAMPLE_VM_SETPARAM_PM;

func:
   Null = r1;
   if NEG rts;
   Null = r1 - $M.MUSIC_MANAGER.PARAMETERS.STRUCT_SIZE;
   if POS rts;
      // Save Parameter
      r3 = r3 AND 0xFFFF;
      r2 = r2 LSHIFT 16;
      r2 = r2 OR r3;
      M[$M.system_config.data.CurParams + r1] = r2;
      // Check status
      Null = Null + r4;
      if NZ jump $music_example_reinitialize;
      // Set Mode to standby
      r8 = $M.MUSIC_MANAGER.SYSMODE.STANDBY;
      M[$music_example.sys_mode] = r8;
      rts;
.ENDMODULE;

// *****************************************************************************
// DESCRIPTION: Read Parameter
//       handle get parameter
// INPUTS:
//    r1 = Parameter ID
// Response
//    P0 = requested ID
//    P1 = returned ID, 0 if request was invalid
//    P2 = MSW
//    P3 = LSW
// *****************************************************************************
.MODULE $M.music_example_message.GetParam;

   .CODESEGMENT MUSIC_EXAMPLE_VM_GETPARAM_PM;

func:
   // Validate Request
   // P0
   r3 = r1;
   // P1
   r4 = r1;
   if NEG r4 = Null;
   Null = r1 - $M.MUSIC_MANAGER.PARAMETERS.STRUCT_SIZE;
   if POS r4 = Null;
   r6 = M[$M.system_config.data.CurParams + r4];
   // MSW   : P2
   r5 = r6 LSHIFT -16;
   // LSW   : P3
   r6 = r6 AND 0xFFFF;
   // push rLink onto stack
   $push_rLink_macro;
   // Send GET RESPONSE Message
   r2 = $music_example.VMMSG.GETPARAM_RESP;
   call $message.send_short;
   // pop rLink from stack
   jump $pop_rLink_and_rts;
   rts;
.ENDMODULE;

.MODULE $M.music_example_message;
   .DATASEGMENT DM;
   .CODESEGMENT PM;

// ---------------------------------------
// vmdB2vol:
//    helper function to convert
//    vm volumes(dB/60.) to suitable
//    linear format for DSP (Q5.19)
//
//    input r0 = vm vol dB
//
//    output r0 = kal vol linear
// ---------------------------------------
vmdB2vol:
   // convert vmdB to log2 format
   r1 = 0.4215663554485;
   rMAC = r0 * 181 (int);
   rMAC = rMAC + r0 * r1;
   // less 24dB for Q5.19 format
   r0 = rMAC - (1<<18);
   if POS r0 = 0;
   // r0 = log2(volume/16.0)
   jump $math.pow2_taylor;

.ENDMODULE;

// *****************************************************************************
// DESCRIPTION: $MsgMusicExampleSetAuxVolume
//       handle call state mode
//  r3 = message pay load
//     word0 = sytem volume index
//     word1 = master_volume_dB * 60
//     word2 = tone_volume_dB * 60
//     word3 = left_trim_volume_dB * 60
//     word4 = right_trim_volume_dB * 60
//
// *****************************************************************************
.MODULE $M.music_example_message.AuxVolume;
   .DATASEGMENT DM;
   .CODESEGMENT  PM;

   .VAR $aux_vol_msg_echo = 0;
   .VAR temp_msg_ptr;
// ------------------------------------------
// update_volumes:
//    update the system when receiving
//    new volumes, it also sends the
//    system volume to vm
// ------------------------------------------
update_volumes:

   // push rLink onto stack
   $push_rLink_macro;

   // update internal volumes
   I0 = r3;
   M[temp_msg_ptr] = r3;

   r4 = M[I0,1];
   null = M[$DAC_conn_aux];
   if Z jump no_system_vol;
   // update system volume
   r4 = r4 AND 0xF;
   M[$music_example.SystemVolume] = r4;

 no_system_vol:

   // update master volume
   r0 = M[I0,1];
   null = r0;
   if POS r0 = 0;
   M[$music_example.Aux.MasterVolume] = r0;
      // convert master volume to linear
   call $M.music_example_message.vmdB2vol;
   r0 = r0 ASHIFT 2; // compensation for headroom
   M[$M.system_config.data.aux_stereo_volume_and_limit_obj + $volume_and_limit.MASTER_VOLUME_FIELD] = r0;

   // update tone volume
   r0 = M[I0,1];
   Null = r0;
   if POS r0 = 0;
   M[$music_example.Aux.ToneVolume] = r0;
   // Tone volume on aux will have no effect

   // convert tone volume to linear
   call $M.music_example_message.vmdB2vol;
   r3 = r0 ASHIFT 3;  // 4-bit up for converting Q5.19 to Q1.23, 1 down for mixing
   // Input r3 = tone mixing ratio
   call $multi_chan_set_aux_tone_mix_ratio;


   // update left trim volume
   r0 = M[I0,1];
   r1 = r0 - $music_example.MAX_VM_TRIM_VOLUME_dB;
   if POS r0 = r0 - r1;
   r1 = r0 - $music_example.MIN_VM_TRIM_VOLUME_dB;
   if NEG r0 = r0 - r1;
   M[$music_example.Aux.LeftTrimVolume] = r0;
   // convert left trim to linear
     call $M.music_example_message.vmdB2vol;
   M[$M.system_config.data.aux_left_channel_vol_struc + $volume_and_limit.channel.TRIM_VOLUME_FIELD] = r0;


   // right trim volume
   r0 = M[I0,1];
   r1 = r0 - $music_example.MAX_VM_TRIM_VOLUME_dB;
   if POS r0 = r0 - r1;
   r1 = r0 - $music_example.MIN_VM_TRIM_VOLUME_dB;
   if NEG r0 = r0 - r1;
   M[$music_example.Aux.RightTrimVolume] = r0;
   // convert right trim to linear
      call $M.music_example_message.vmdB2vol;
   M[$M.system_config.data.aux_right_channel_vol_struc + $volume_and_limit.channel.TRIM_VOLUME_FIELD] = r0;

   null = M[$aux_vol_msg_echo];
   if Z jump done;

   // VM expects entire volume message to be sent back
  r5 = M[temp_msg_ptr];
  r4 = 5; // size of aux volume message
  r3 = $music_example.VMMSG.MESSAGE_AUX_VOLUME_RESP;
  call $message.send_long;
done:
   // pop rLink from stack
   jump $pop_rLink_and_rts;

func:
   // push rLink onto stack
   $push_rLink_macro;

   call update_volumes;

volume_msg_done:
   // pop rLink from stack
   jump $pop_rLink_and_rts;
.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.music_example_message.AuxVolume_s
//
// DESCRIPTION:
//    Handler for the replacement short multi-channel configuration message.
//    (this uses the original corresponding long configuration message handler)
//    Message from VM->DSP to specify the mute status of all aux wired outputs
//    (Note 1: all aux wired multi-channel outputs are specified)
//    (Note 2: r1 selects the parameters being set by the message
//     Parameters are cached until a message with r1 = 0 is received
//     this sets all the parameters atomically from the cache.)
//
// INPUTS:
//
//    r1 = volume select = 0                       <=  This must be sent to synchronise
//    r2 = system volume index (i.e. DAC index)        other aux volume changes
//    r3 = master volume (dB)*60
//    r4 = tone volume (dB)*60

//    r1 = volume select = 1
//    r2 = Left aux trim volume (dB)*60
//    r3 = Right aux trim volume (dB)*60
//    r4 = <not used>
// OUTPUTS:
//    none
//
// TRASHED REGISTERS:
//    r0, r1, r2, r3, r4, r5, r8, I0
//
// *****************************************************************************
.MODULE $M.music_example_message.AuxVolume_s;

   .CODESEGMENT PM;

func:

   // Point the temporary message payload cache
   r5 = $M.music_example_message_payload_cache.aux_volume_s;
   I0 = r5;

   null = r1;                          // Test the select
   if NZ jump skip_select0;

      // Load parameters into cache then process all the volume parameters from the cache
      M[I0,1] = r2;                    // System volume index
      M[I0,1] = r3;                    // Master volume (dB)*60
      M[I0,1] = r4;                    // Tone volume (dB)*60

      // Point at the payload cache
      r3 = r5;

      // Execute the original long message handler (input: r3 points at the payload cache)
      jump $M.music_example_message.AuxVolume.func;

   skip_select0:

   null = r1 - 1;                      // Test the select
   if NZ jump skip_select1;

      // Load parameters into cache then exit without processing
      M0 = 3;
      r0 = M[I0,M0];                   // Dummy read to skip other parameters in cache
      M[I0,1] = r2;                    // Left aux trim volume (dB)*60
      M[I0,1] = r3;                    // Right aux trim volume (dB)*60

   skip_select1:

   // Exit without processing volume parameters
   rts;
.ENDMODULE;

// *****************************************************************************
// DESCRIPTION: $MsgMusicExampleSetMainVolume
//       handle call state mode
//  r3 = message pay load
//     word0 = sytem volume index
//     word1 = master_volume_dB * 60
//     word2 = tone_volume_dB * 60
//     word3 = primary_left_trim_volume_dB * 60
//     word4 = primary_right_trim_volume_dB * 60
//     word5 = secondary_left_trim_volume_dB * 60
//     word6 = secondary_right_trim_volume_dB * 60
//     word7 = sub_trim_volume_dB * 60
//
// *****************************************************************************
.MODULE $M.music_example_message.MainVolume;

.DATASEGMENT DM;

.CODESEGMENT   PM;

   .VAR $multichannel_vol_msg_echo = 0;
   .VAR temp_msg_ptr;
// ------------------------------------------
// update_volumes:
//    update the system when receiving
//    new volumes, it also sends the
//    system volume to vm
//
// ------------------------------------------
update_volumes:

   // push rLink onto stack
   $push_rLink_macro;

   // update internal volumes
   I0 = r3;
   M[temp_msg_ptr] = r3;


   r4 = M[I0,1];

   null = M[$DAC_conn_main];
   if Z jump no_system_vol;
   // update system volume
   r4 = r4 AND 0xF;
   M[$music_example.SystemVolume] = r4;

 no_system_vol:

   // update master volume
   r0 = M[I0,1];
   null =r0;
   if POS r0 = 0;
   M[$music_example.Main.MasterVolume] = r0;

   // convert master volume to linear
   call $M.music_example_message.vmdB2vol;
   r0 = r0 ASHIFT 2; // compensation for headroom
   M[$M.system_config.data.multichannel_volume_and_limit_obj + $volume_and_limit.MASTER_VOLUME_FIELD] = r0;

   // update tone volume
   r0 = M[I0,1];
   Null = r0;
   if POS r0 = 0;
   M[$music_example.Main.ToneVolume] = r0;

   // convert tone volume to linear
   call $M.music_example_message.vmdB2vol;
   r3 = r0 ASHIFT 3;  // 4-bit up for converting Q5.19 to Q1.23, 1 down for mixing
   // Input r3 = tone mixing ratio
   call $multi_chan_set_prim_tone_mix_ratio;

   // update primary left trim volume
   r0 = M[I0,1];
   r1 = r0 - $music_example.MAX_VM_TRIM_VOLUME_dB;
   if POS r0 = r0 - r1;
   r1 = r0 - $music_example.MIN_VM_TRIM_VOLUME_dB;
   if NEG r0 = r0 - r1;
   M[$music_example.Main.PrimaryLeftTrimVolume] = r0;

   // convert trim to linear
   call $M.music_example_message.vmdB2vol;
   M[$M.system_config.data.left_primary_channel_vol_struc + $volume_and_limit.channel.TRIM_VOLUME_FIELD] = r0;

   // right primary trim volume
   r0 = M[I0,1];
   r1 = r0 - $music_example.MAX_VM_TRIM_VOLUME_dB;
   if POS r0 = r0 - r1;
   r1 = r0 - $music_example.MIN_VM_TRIM_VOLUME_dB;
   if NEG r0 = r0 - r1;
   M[$music_example.Main.PrimaryRightTrimVolume] = r0;

   // convert  trim to linear
   call $M.music_example_message.vmdB2vol;
   M[$M.system_config.data.right_primary_channel_vol_struc + $volume_and_limit.channel.TRIM_VOLUME_FIELD] = r0;

    // update secondary left trim volume
   r0 = M[I0,1];
   r1 = r0 - $music_example.MAX_VM_TRIM_VOLUME_dB;
   if POS r0 = r0 - r1;
   r1 = r0 - $music_example.MIN_VM_TRIM_VOLUME_dB;
   if NEG r0 = r0 - r1;
   M[$music_example.Main.SecondaryLeftTrimVolume] = r0;

   // convert trim to linear
   call $M.music_example_message.vmdB2vol;
   M[$M.system_config.data.left_secondary_channel_vol_struc + $volume_and_limit.channel.TRIM_VOLUME_FIELD] = r0;

   // secondary right trim volume
   r0 = M[I0,1];
   r1 = r0 - $music_example.MAX_VM_TRIM_VOLUME_dB;
   if POS r0 = r0 - r1;
   r1 = r0 - $music_example.MIN_VM_TRIM_VOLUME_dB;
   if NEG r0 = r0 - r1;
   M[$music_example.Main.SecondaryRightTrimVolume] = r0;

   // convert trim to linear
   call $M.music_example_message.vmdB2vol;
   M[$M.system_config.data.right_secondary_channel_vol_struc + $volume_and_limit.channel.TRIM_VOLUME_FIELD] = r0;

   // wired sub trim volume
   r0 = M[I0,1];
   r1 = r0 - $music_example.MAX_VM_TRIM_VOLUME_dB;
   if POS r0 = r0 - r1;
   r1 = r0 - $music_example.MIN_VM_TRIM_VOLUME_dB;
   if NEG r0 = r0 - r1;
   M[$music_example.Main.SubTrimVolume] = r0;

   // convert right trim to linear
   call $M.music_example_message.vmdB2vol;
   M[$M.system_config.data.wired_sub_channel_vol_struc + $volume_and_limit.channel.TRIM_VOLUME_FIELD] = r0;

   // VM expects entire volume message to be sent back
   null = M[$multichannel_vol_msg_echo];
   if Z jump done;
   r5 = M[temp_msg_ptr];
   r4 = 8; // size of main volume message
   r3 = $music_example.VMMSG.MESSAGE_MAIN_VOLUME_RESP;
   call $message.send_long;

done:
   // pop rLink from stack
   jump $pop_rLink_and_rts;

func:
   // push rLink onto stack
   $push_rLink_macro;


#ifdef TWS_ENABLE
   r0 = M[$tws.local_mute];
   if Z jump skip_volume_save;

   r2 = length($tws.local_saved_volume_struc);
   r1 = &$tws.local_saved_volume_struc;
   call $tws.copy_array;
   jump volume_msg_done;
   skip_volume_save:
#endif
   call update_volumes;

volume_msg_done:
   // pop rLink from stack
   jump $pop_rLink_and_rts;

tws_func:
    // push rLink onto stack
   $push_rLink_macro;

   call update_volumes;

   // pop rLink from stack
   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.music_example_message.MainVolume_s
//
// DESCRIPTION:
//    Handler for the replacement short multi-channel configuration message.
//    (this uses the original corresponding long configuration message handler)
//    Message from VM->DSP to specify the mute status of all main wired outputs
//    (Note 1: all main wired multi-channel outputs are specified)
//    (Note 2: r1 selects the parameters being set by the message
//     Parameters are cached until a message with r1 = 0 is received
//     this sets all the parameters atomically from the cache.)
//
// INPUTS:
//
//    r1 = volume select = 0                       <=  This must be sent to synchronise
//    r2 = system volume index (i.e. DAC index)        other main volume changes
//    r3 = master volume (dB)*60
//    r4 = tone volume (dB)*60

//    r1 = volume select = 1
//    r2 = Left primary trim volume (dB)*60
//    r3 = Right primary trim volume (dB)*60
//    r4 = <not used>

//    r1 = volume select = 2
//    r2 = Left secondary trim volume (dB)*60
//    r3 = Right secondary trim volume (dB)*60
//    r4 = Wired subwoofer trim (dB)*60

// OUTPUTS:
//    none
//
// TRASHED REGISTERS:
//    r0, r1, r2, r3, r4, r5, r8, I0
//
// *****************************************************************************
.MODULE $M.music_example_message.MainVolume_s;

   .CODESEGMENT PM;

func:

   // Point the temporary message payload cache
   r5 = $M.music_example_message_payload_cache.multi_volume_s;
   I0 = r5;

   null = r1;                          // Test the select
   if NZ jump skip_select0;

      // Load parameters into cache then process all the volume parameters from the cache
      M[I0,1] = r2;                    // System volume index
      M[I0,1] = r3;                    // Master volume (dB)*60
      M[I0,1] = r4;                    // Tone volume (dB)*60

      // Point at the payload cache
      r3 = r5;

      // Execute the original long message handler (input: r3 points at the payload cache)
      jump $M.music_example_message.MainVolume.func;

   skip_select0:

   null = r1 - 1;                      // Test the select
   if NZ jump skip_select1;

      // Load parameters into cache then exit without processing
      M0 = 3;
      r0 = M[I0,M0];                   // Dummy read to skip other parameters in cache
      M[I0,1] = r2;                    // Left primary trim volume (dB)*60
      M[I0,1] = r3;                    // Right primary trim volume (dB)*60

      jump exit;                       // Exit without processing
   skip_select1:

   null = r1 - 2;                      // Test the select
   if NZ jump skip_select2;

      // Load parameters into cache then exit without processing
      M0 = 5;
      r0 = M[I0,M0];                   // Dummy read to skip other parameters in cache
      M[I0,1] = r2;                    // Left secondary trim volume (dB)*60
      M[I0,1] = r3;                    // Right secondary trim volume (dB)*60
      M[I0,1] = r4;                    // Wired subwoofer trim (dB)*60

   skip_select2:
   exit:

   // Exit without processing volume parameters
   rts;
.ENDMODULE;

// *****************************************************************************
// DESCRIPTION: $LoadParams
// r1 = PsKey Address containing Music Example parameters
// *****************************************************************************
.MODULE $M.music_example.LoadParams;

   .CODESEGMENT MUSIC_EXAMPLE_VM_LOADPARAMS_PM;
   .DATASEGMENT DM;
   .VAR paramoffset = 0;
   .VAR Pskey_fetch_flg = 1;
   .VAR Last_PsKey;

func:
   $push_rLink_macro;
   // Set Mode to standby
   r8 = $M.MUSIC_MANAGER.SYSMODE.STANDBY;
   M[$music_example.sys_mode] = r8;
   push r1; // save key
   // Copy default parameters into current parameters
   call $M.music_example.load_default_params.func;
   // Save for SPI Status
   M[paramoffset] = 0; // needed if loadparams is called more than once
   pop r2;
   M[Last_PsKey] = r2;
TestPsKey:
   if Z jump done;
      // r2 = key;
      //  &$friendly_name_pskey_struc;
      r1 = &$M.music_example_message.ps_key_struc;
      // &$DEVICE_NAME_pskey_handler;
      r3 = &$M.music_example.PsKeyReadHandler.func;
      call $pskey.read_key;
      jump $pop_rLink_and_rts;
done:

   // copy codec config word to current config word
   r0 = M[$codec_config];
   r0 = M[&$M.system_config.data.CurParams + r0];

   // Set the current codec config word
   M[&$M.system_config.data.CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_CONFIG] = r0;

   call $M.music_example.ReInit.func;

   // Tell VM is can send other messages
   r2 = $music_example.VMMSG.PARAMS_LOADED;
   call $message.send_short;

   jump $pop_rLink_and_rts;
.ENDMODULE;



// *****************************************************************************
// DESCRIPTION: $PsKeyReadHandler
//  INPUTS:
//    Standard (short) message mode:
//    r1 = Key ID
//    r2 = Buffer Length; $pskey.FAILED_READ_LENGTH on failure
//    r3 = Payload.  Key ID Plus data
// *****************************************************************************
.MODULE $M.music_example.PsKeyReadHandler;

   .CODESEGMENT MUSIC_EXAMPLE_PSKEYREADHANDLER_PM;

func:
   $push_rLink_macro;

   // error checking - check if read failed
   // if so, DSP default values will be used instead of PsKey values.
   Null = r2 - $pskey.FAILED_READ_LENGTH;
   if NZ jump No_Retry;
   //Retry requesting for the PSKEY once.
   r0 = M[$M.music_example.LoadParams.Pskey_fetch_flg];  //If Z we have retried once already
   if Z jump No_2nd_Retry;
   M[$M.music_example.LoadParams.Pskey_fetch_flg] = 0;
   r2 = M[$M.music_example.LoadParams.Last_PsKey];
   jump $M.music_example.LoadParams.TestPsKey;
No_2nd_Retry:
   //Reset flag for next time and keep the default parameters
   r0 = 1;
   M[$M.music_example.LoadParams.Pskey_fetch_flg] = r0;
   jump $M.music_example.LoadParams.done;
No_Retry:
   // Adjust for Key Value in payload?
   I0 = r3 + 1;
   r10 = r2 - 1;
   // Clear sign bits
   // I2=copy of address
   I2 = I0;
   // r3=mask to clear sign extension
   r3 = 0x00ffff;
   do loop1;
      r0 = M[I2,0];
      r0 = r0 AND r3;
      M[I2,1] = r0;
loop1:
   r10 = 256;

   // End of buffer pointer (last valid location)
   I2 = I2 - 1;

   // error checking - make sure first value is the Pskey address.
   // if not, DSP default values will be used instead of PsKey values.
   r0 = M[I0,1];
   Null = r1 - r0;
   if NZ jump $M.music_example.LoadParams.done;

   // get next Pskey address
   // r5 = address of next PsKey
   r5 = M[I0,1];
   r0 = M[I0,1];
   // r4 = NumParams (last parameter + 1)
   r4 = r0 AND 0xff;
   if Z r4 = r10;
   // r0 = firstParam (zero-based index into
   //      paramBlock)
   r0 = r0 LSHIFT -8;

   // initial mask value
   r8 = Null;

start_loop:
      r8 = r8 LSHIFT -1;
      if NZ jump withinGroup;

      // Check for past end of buffer
      null = I2 - I0;
      if NEG jump endOfBuffer;

      // group
      r3 = M[I0,1];

      // mask value
      r8 = 0x8000;
      // used for odd variable
      r7 = Null;
withinGroup:
      Null = r3 AND r8;
      if Z jump dontOverwriteCurrentValue;
         // overwrite current parameter
         r7 = r7 XOR 0xffffff;
         if Z jump SomeWhere;
         // MSB for next two parameters
         r2 = M[I0,1];
         // MSB for param1
         r6 = r2 LSHIFT -8;
         jump SomeWhereElse;
SomeWhere:
         // MSB for param2
         r6 = r2 AND 0xff;
SomeWhereElse:
         // LSW
         r1 = M[I0,1];
         r6 = r6 LSHIFT 16;
         // Combine MSW and LSW
         r1 = r1 OR r6;
         r6 = r0 + M[$M.music_example.LoadParams.paramoffset];
         M[$M.system_config.data.CurParams + r6] = r1;
dontOverwriteCurrentValue:
      r0 = r0 + 1;
      Null = r0 - r4;
   if NEG jump start_loop;

endOfBuffer:
   // inc offset if lastkey=0
   r2 = M[$M.music_example.LoadParams.paramoffset];
   Null = r4 - r10;
   if Z r2 = r2 + r10;
   M[$M.music_example.LoadParams.paramoffset] = r2;
   // PS Key Being requested
   r2 = r5;
   jump $M.music_example.LoadParams.TestPsKey;

.ENDMODULE;

#if defined(APTX_ENABLE)
// *****************************************************************************
// DESCRIPTION: $AptxSecurityStatus
// r1 = PsKey Address containing apt-X Security Status
// *****************************************************************************
.MODULE $M.music_example.AptxSecurityStatus;

   .CODESEGMENT PM;

func:
   $push_rLink_macro;

   // Security status is in r1;
   M[$aptx_security_status] = r1;

   call $aptx.decoder_version;

   M[$aptx_decoder_version] = r1;

   jump $pop_rLink_and_rts;
.ENDMODULE;


#elif defined APTX_ACL_SPRINT_ENABLE
// *****************************************************************************
// DESCRIPTION: $AptxSecurityStatus
// r1 = PsKey Address containing apt-X Security Status
// *****************************************************************************
.MODULE $M.music_example.AptxSecurityStatus;

   .CODESEGMENT PM;

func:
   $push_rLink_macro;

   // Security status is in r1;
   M[$aptx_security_status] = r1;

  call $aptx_sprint.decoder_version;


   M[$aptx_decoder_version] = r1;

   jump $pop_rLink_and_rts;
.ENDMODULE;
#endif

// *****************************************************************************
// MODULE:
//    $M.music_example_message.SetOutputDevType
//
// DESCRIPTION:
//    Handler for multi-channel configuration message.
//    Message from VM->DSP to specify the device types of multi-channel outputs
//    (Note: all wired multi-channel outputs are specified)
//
// INPUTS:
//  r3 = address of message pay load
//     word0 = Primary left output type
//     word1 = Primary right output type
//     word2 = Secondary left output type
//     word3 = Secondary right output type
//     word4 = Wired Sub output type
//     word5 = Aux left output type
//     word6 = Aux right output type
//
// OUTPUTS:
//    none
//
// TRASHED REGISTERS:
//    r0, I0
//
// *****************************************************************************
.MODULE $M.music_example_message.SetOutputDevType;

   .CODESEGMENT PM;

func:

   // Point at the payload
   I0 = r3;

   r0 = M[I0,1];
   M[$M.multi_chan_output.wired_out_type_table + $MULTI_CHAN_PRIMARY_LEFT_OUT_CHAN] = r0;
   r0 = M[I0,1];
   M[$M.multi_chan_output.wired_out_type_table + $MULTI_CHAN_PRIMARY_RIGHT_OUT_CHAN] = r0;
   r0 = M[I0,1];
   M[$M.multi_chan_output.wired_out_type_table + $MULTI_CHAN_SECONDARY_LEFT_OUT_CHAN] = r0;
   r0 = M[I0,1];
   M[$M.multi_chan_output.wired_out_type_table + $MULTI_CHAN_SECONDARY_RIGHT_OUT_CHAN] = r0;
   r0 = M[I0,1];
   M[$M.multi_chan_output.wired_out_type_table + $MULTI_CHAN_SUB_WIRED_OUT_CHAN] = r0;
   r0 = M[I0,1];
   M[$M.multi_chan_output.wired_out_type_table + $MULTI_CHAN_AUX_LEFT_OUT_CHAN] = r0;
   r0 = M[I0,1];
   M[$M.multi_chan_output.wired_out_type_table + $MULTI_CHAN_AUX_RIGHT_OUT_CHAN] = r0;

   rts;
.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.music_example_message.SetOutputDevType_s
//
// DESCRIPTION:
//    Handler for the replacement short multi-channel configuration message.
//    (this uses the original corresponding long configuration message handler)
//    Message from VM->DSP to specify the device types of multi-channel outputs
//    (Note: all wired multi-channel outputs are specified)
//
// INPUTS:
//
//  r1 = ls octet    Primary left output type
//  r1 = ms octet    Primary right output type
//  r2 = ls octet    Secondary left output type
//  r2 = ms octet    Secondary right output type
//  r3 = ls octet    Wired sub output type
//  r3 = ms octet    <not used>
//  r4 = ls octet    Aux left output type
//  r4 = ms octet    Aux right output type
//
// OUTPUTS:
//    none
//
// TRASHED REGISTERS:
//    r0, r3, r5, I0
//
// *****************************************************************************
.MODULE $M.music_example_message.SetOutputDevType_s;

   .CODESEGMENT PM;

func:

   // Point the temporary message payload cache
   r5 = $M.music_example_message_payload_cache.set_output_dev_type_s;
   I0 = r5;

   // Load payload cache with the message paramters
   r0 = r1 AND 0xff;
   M[I0,1] = r0;                       // Primary left output type
   r0 = r1 LSHIFT -8;
   M[I0,1] = r0;                       // Primary right output type

   r0 = r2 AND 0xff;
   M[I0,1] = r0;                       // Secondary left output type
   r0 = r2 LSHIFT -8;
   M[I0,1] = r0;                       // Secondary right output type

   r0 = r3 AND 0xff;
   M[I0,1] = r0;                       // Wired subwoofer output type

   r0 = r4 AND 0xff;
   M[I0,1] = r0;                       // Aux left output type
   r0 = r4 LSHIFT -8;
   M[I0,1] = r0;                       // Aux right output type

   // Point at the payload cache
   r3 = r5;

   // Execute the original long message handler
   jump $M.music_example_message.SetOutputDevType.func;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.music_example_message.SetI2SMode
//
// DESCRIPTION:
//    Handler for multi-channel configuration message.
//    Message from VM->DSP to specify the operation mode of I2S interfaces
//
// INPUTS:
//    r1 = 0: master; 1: slave
//
// OUTPUTS:
//    none
//
// TRASHED REGISTERS:
//    none
//
// *****************************************************************************
.MODULE $M.music_example_message.SetI2SMode;

   .CODESEGMENT PM;

func:

   // Set the master/slave flag
   M[$M.multi_chan_output.i2s_slave0] = r1;

   rts;
.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.music_example_message.SetResolutionModes
//
// DESCRIPTION:
//    Handler for the set resolution mode configuration message.
//    Message from VM->DSP to specify the application resolution modes.
//
//    These can each be set independently to 16 or 24bit mode and are used by
//    the DSP application to determine how to configure:
//       1) the audio input port resolution
//       2) the audio processing resolution
//       3) the audio output port resolution
//
// INPUTS:
//    r1 = the audio input port resolution (16: 16bit mode; 24: 24bit mode)
//    r2 = the audio processing resolution (16: 16bit mode; 24: 24bit mode)
//    r3 = the audio output port resolution (16: 16bit mode; 24: 24bit mode)
//
// OUTPUTS:
//    none
//
// TRASHED REGISTERS:
//    none
//
// *****************************************************************************
.MODULE $M.music_example_message.SetResolutionModes;

   .CODESEGMENT PM;

func:

   // Set the resolution modes (16 or 24bit mode)
   M[$inputResolutionMode] = r1;
   M[$procResolutionMode] = r2;
   M[$outputResolutionMode] = r3;

   rts;
.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.music_example_message.SetANCMode
//
// DESCRIPTION:
//    Handler for multi-channel ANC configuration message.
//    Message from VM->DSP to specify the operation mode of ANC
//
// INPUTS:
//    r1 = bits(1:0): Select ANC output rate (0: No ANC, 1: 96kHz, 2: 192kHz)
//
// OUTPUTS:
//    none
//
// TRASHED REGISTERS:
//    none
//
// *****************************************************************************
.MODULE $M.music_example_message.SetANCMode;
   .CODESEGMENT PM;
func:
   r2 = M[$music_example.config_raw];
   r0 = M[$music_example.config_anc];
   r1 = r1 AND $ANC_MASK;                             // Mask for ANC output rate control
   M[$ancMode] = r1;                                  // Set the ANC statistic (0: none, 1: 96kHz, 2: 192kHz)
   if NZ r2 = r0; 
   M[$M.MUSIC_EXAMPLE_MODULES_STAMP.CompConfig] = r2; //Set UFE config word to enable/disable ANC block
   rts;
.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.music_example_message.MultiChannelMainMute
//
// DESCRIPTION:
//    Handler for the multi-channel main mute message.
//    Message from VM->DSP to specify the mute status of all main wired outputs
//    (Note: all main wired multi-channel outputs are specified)
//
// INPUTS:
//  r3 = address of message pay load
//     word0 = Primary left mute control
//     word1 = Primary right mute control
//     word2 = Secondary left mute control
//     word3 = Secondary right mute control
//     word4 = Wired Sub mute control
//
// OUTPUTS:
//    none
//
// TRASHED REGISTERS:
//    r0, r1, r2, r3, r4, r8, I0
//
// *****************************************************************************
.MODULE $M.music_example_message.MultiChannelMainMute;

   .CODESEGMENT PM;

func:
   $push_rLink_macro;

   // Point at the payload
   I0 = r3;

   // Get the current mute state (all wired channels)
   r3 = M[$M.multi_chan_output.channels_mute_en];        //
   r3 = r3 AND $MULTI_CHAN_AUX_CHANNELS_MASK;            // Keep the current aux channel mute state

   // Set the new main channel mute status
   r1 = (1 << $MULTI_CHAN_PRIMARY_LEFT_OUT_CHAN);
   r0 = M[I0,1];
   null = r0;
   if NZ r3 = r3 OR r1;
   r1 = (1 << $MULTI_CHAN_PRIMARY_RIGHT_OUT_CHAN);
   r0 = M[I0,1];
   null = r0;
   if NZ r3 = r3 OR r1;
   r1 = (1 << $MULTI_CHAN_SECONDARY_LEFT_OUT_CHAN);
   r0 = M[I0,1];
   null = r0;
   if NZ r3 = r3 OR r1;
   r1 = (1 << $MULTI_CHAN_SECONDARY_RIGHT_OUT_CHAN);
   r0 = M[I0,1];
   null = r0;
   if NZ r3 = r3 OR r1;
   r1 = (1 << $MULTI_CHAN_SUB_WIRED_OUT_CHAN);
   r0 = M[I0,1];
   null = r0;
   if NZ r3 = r3 OR r1;

   // Configure the multi-channel muting control (r3 = new mute bitmask)
   push r3;                                                 // Save the channel mute mask
   call $multi_chan_soft_mute;                              //
   pop r3;                                                  // Restore the channel mute mask

   // Wireless subwoofer is muted when wired subwoofer is muted
   r0 = 1;
   r1 = -1;
   null = r3 AND (1 << $MULTI_CHAN_SUB_WIRED_OUT_CHAN);     // Test the wired subwoofer bit field
   if Z r1 = r0;                                            //
   m[$M.downsample_sub_to_1k2.mute_direction] = r1;         // (+1=unmute, -1=mute)

   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.music_example_message.MultiChannelMainMute_s
//
// DESCRIPTION:
//    Handler for the replacement short multi-channel configuration message.
//    (this uses the original corresponding long configuration message handler)
//    Message from VM->DSP to specify the mute status of all main wired outputs
//    (Note: all main wired multi-channel outputs are specified)
//
// INPUTS:
//    r1 = multi-channel mute enable flags
//          bit0 = Primary left mute
//          bit1 = Primary right mute
//          bit2 = Secondary left mute
//          bit3 = Secondary right mute
//          bit4 = Wired Sub mute
//          bit5-bit15  <not used>
//
// OUTPUTS:
//    none
//
// TRASHED REGISTERS:
//    r0, r1, r2, r3, r4, r8, I0
//
// *****************************************************************************
.MODULE $M.music_example_message.MultiChannelMainMute_s;

   .CODESEGMENT PM;

func:

   // Point the temporary message payload cache
   r3 = $M.music_example_message_payload_cache.multi_channel_main_mute_s;
   I0 = r3;

   // Set the new main channel mute status
   r0 = r1 AND 0x01;
   M[I0,1] = r0;                       // Primary left mute control

   r1 = r1 LSHIFT -1;
   r0 = r1 AND 0x01;
   M[I0,1] = r0;                       // Primary right mute control

   r1 = r1 LSHIFT -1;
   r0 = r1 AND 0x01;
   M[I0,1] = r0;                       // Secondary left mute control

   r1 = r1 LSHIFT -1;
   r0 = r1 AND 0x01;
   M[I0,1] = r0;                       // Secondary right mute control

   r1 = r1 LSHIFT -1;
   r0 = r1 AND 0x01;
   M[I0,1] = r0;                       // Wired subwoofer mute control

   // Execute the original long message handler (input: r3 points at the payload cache)
   jump $M.music_example_message.MultiChannelMainMute.func;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.music_example_message.MultiChannelAuxMute
//
// DESCRIPTION:
//    Handler for the multi-channel aux mute message.
//    Message from VM->DSP to specify the mute status of the aux wired outputs
//
// INPUTS:
//  r3 = address of message pay load
//     word0 = Aux left mute control
//     word1 = Aux right mute control
//
// OUTPUTS:
//    none
//
// TRASHED REGISTERS:
//    r0, r1, r2, r3, r4, r8, I0
//
// *****************************************************************************
.MODULE $M.music_example_message.MultiChannelAuxMute;

   .CODESEGMENT PM;

func:
   $push_rLink_macro;

   // Point at the payload
   I0 = r3;

   // Get the current mute state (all wired channels)
   r3 = M[$M.multi_chan_output.channels_mute_en];        //
   r3 = r3 AND $MULTI_CHAN_MAIN_CHANNELS_MASK;           // Keep the current main channel mute state

   // Set the new aux channel mute status
   r1 = (1 << $MULTI_CHAN_AUX_LEFT_OUT_CHAN);
   r0 = M[I0,1];
   null = r0;
   if NZ r3 = r3 OR r1;
   r1 = (1 << $MULTI_CHAN_AUX_RIGHT_OUT_CHAN);
   r0 = M[I0,1];
   null = r0;
   if NZ r3 = r3 OR r1;

   // Configure the multi-channel muting control (r3 = new mute bitmask)
   call $multi_chan_soft_mute;

   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.music_example_message.MultiChannelAuxMute_s
//
// DESCRIPTION:
//    Handler for the replacement short multi-channel configuration message.
//    (this uses the original corresponding long configuration message handler)
//    Message from VM->DSP to specify the mute status of all aux wired outputs
//    (Note: all aux wired multi-channel outputs are specified)
//
// INPUTS:
//    r1 = multi-channel mute enable flags
//          bit0 = Aux left mute control
//          bit1 = Aux right mute control
//          bit2-bit15  <not used>
//
// OUTPUTS:
//    none
//
// TRASHED REGISTERS:
//    r0, r1, r2, r3, r4, r8, I0
//
// *****************************************************************************
.MODULE $M.music_example_message.MultiChannelAuxMute_s;

   .CODESEGMENT PM;

func:

   // Point the temporary message payload cache
   r3 = $M.music_example_message_payload_cache.multi_channel_aux_mute_s;
   I0 = r3;

   // Set the new aux channel mute status
   r0 = r1 AND 0x01;
   M[I0,1] = r0;                       // Aux left mute control

   r1 = r1 LSHIFT -1;
   r0 = r1 AND 0x01;
   M[I0,1] = r0;                       // Aux right mute control

   // Execute the original long message handler (input: r3 points at the payload cache)
   jump $M.music_example_message.MultiChannelAuxMute.func;

.ENDMODULE;

// *****************************************************************************
// DESCRIPTION: $SetConfig
// *****************************************************************************
// Set and return configuration word of music manager
// - Config word enables or bypasses the various audio processing modules
// - 24 bit config word is split into two parts.
// - Most significant 8 bits and Least significant 16 bits
// *****************************************************************************
// on entry r1 = Mask MSW (8 bits)
//          r2 = Mask LSW (16 bits)
//          r3 = Data MSW (8 bits)
//          r4 = Data LSW (8 bits)
// *****************************************************************************
// Return Param 0 = Data MSW (8 bits)
//              1 = Data LSW (16 bits)
// *****************************************************************************

.MODULE $M.music_example_message.SetConfig ;

   .CODESEGMENT PM ;

func:

    $push_rLink_macro ;

    // reconstruct config mask
    r2 = r2 and 0xffff ;
    r1 = r1 lshift 16 ;
    r1 = r1 or r2 ;
    // reconstruct data word
    r4 = r4 and 0xffff ;
    r3 = r3 lshift 16;
    r3 = r3 or r4 ;

    r0 = M[&$M.system_config.data.CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_CONFIG] ;

    // modify code word ( (data & mask) ^ (configWord & !mask) )
    r3 = r3 and r1 ;
    r1 = r1 xor 0xfffff ;
    r0 = r0 and r1 ;
    r0 = r0 or r3 ;

    M[&$M.system_config.data.CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_CONFIG] = r0 ;

    // Re-init because has changed
    r1 = 1;
    M[$music_example.reinit] = r1;

    jump $pop_rLink_and_rts ;

.ENDMODULE ;

//------------------------------------------------------------------------------
.module $M.music_example_message.SignalDetect;
//------------------------------------------------------------------------------
// Receives paramters for standby detection
//------------------------------------------------------------------------------
// on entry r1 = threshold (16 bit fractional value - aligned to MSB in DSP)
//          r2 = trigger time in seconds (16 bit int)
// *** NO CHECKING IS PERFORMED ON MESSAGE PARAMETERS ***
//------------------------------------------------------------------------------

    .datasegment dm ;
    .codesegment pm ;

func:

    $push_rLink_macro ;

    r1 = r1 lshift 8 ;
    r8 = $M.multi_chan_output.signal_detect_coeffs;
    call $M.cbops.signal_detect_op.message_handler.func;

    jump $pop_rLink_and_rts ;

.endmodule ;


//------------------------------------------------------------------------------
.module $M.music_example_message.SoftMute;
//------------------------------------------------------------------------------
// Receives paramters for soft mute
//------------------------------------------------------------------------------
// on entry r1 = mute control (0=unmute, 1=mute)
//               bit 0 = left/right mute control
//               bit 1 = subwoofer mute control
// *** NO CHECKING IS PERFORMED ON MESSAGE PARAMETERS ***
//------------------------------------------------------------------------------

    .datasegment dm ;
    .codesegment pm ;

func:

    $push_rLink_macro ;

    // take copy of r1 as it's destroyed
    r2 = r1;

    // left/right mute control
    r1 = r1 and 0x01;

    // Control muting of all wired output channels
    r3 = $MULTI_CHAN_MAIN_CHANNELS_MASK | $MULTI_CHAN_AUX_CHANNELS_MASK;   // Default to mute all channels
    null = r1;                                                             // Mute required?
    if Z r3 = 0;                                                           // No - zero mask for unmute all channels
    push r2;                                                               // Save r2 since used in next call
    call $multi_chan_soft_mute;                                            // Apply muting as required
    pop r2;

    // subwoofer mute control
    r1 = r2 lshift -1;
    r1 = r1 and 0x01;
    r0 = 1;
    r1 = -r1;
    null = r1;
    if Z r1 = r0;
    m[$M.downsample_sub_to_1k2.mute_direction] = r1;

    jump $pop_rLink_and_rts ;

.endmodule ;


//------------------------------------------------------------------------------
.module $M.music_example_message.SetUserEqParamMsg;
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
    r0 = r0 + ($M.system_config.data.CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_USER_EQ_NUM_BANKS);

    r2 = r2 and 0x00ffff;
    m[r0] = r2;

    // if update flag is zero then exit now and don't recalculate coefficients
    null = r3 - 0;
    if eq jump $pop_rLink_and_rts ;

    r0 = r1;
    r1 = &$M.system_config.data.UserEqCoefsA;
    r2 = &$M.system_config.data.UserEqCoefsB;
    r3 = ($M.system_config.data.CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_USER_EQ_NUM_BANKS);
    call $user_eq.calcBandCoefs;

    jump $pop_rLink_and_rts ;

.endmodule ;


//------------------------------------------------------------------------------
.module $M.music_example_message.GetUserEqParamMsg;
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
    r0 = r0 + ($M.system_config.data.CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_USER_EQ_NUM_BANKS);

    r4 = m[r0];       // word 2 (value)
    r5 = 0;           // word 3
    r6 = 0;           // word 4
    r2 = $music_example.GAIAMSG.GET_USER_PARAM_RESP;
    call $message.send_short;

    jump $pop_rLink_and_rts ;

.endmodule ;


//------------------------------------------------------------------------------
.module $M.music_example_message.SetUserEqGroupParamMsg;
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
        r0 = r0 + ($M.system_config.data.CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_USER_EQ_NUM_BANKS);
        r1 = m[i0,1];       // get value to store
        r1 = r1 and 0x00ffff;
        m[r0] = r1;         // store value
    SetParamsLoop:

    //// currently ignore update flag for group parameter message

    jump $pop_rLink_and_rts ;

.endmodule ;


//------------------------------------------------------------------------------
.module $M.music_example_message.GetUserEqGroupParamMsg;
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
        r0 = r0 + ($M.system_config.data.CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_USER_EQ_NUM_BANKS);
        r0 = m[r0];         // r0 = paramter value
        m[i0,1] = r0;       // store parameter back into message to return to VM
    GetParamsLoop:

    r5 = r3;
    r4 = r2;
    r3 = $music_example.GAIAMSG.GET_USER_GROUP_PARAM_RESP;
    call $message.send_long;

    jump $pop_rLink_and_rts ;

.endmodule ;


