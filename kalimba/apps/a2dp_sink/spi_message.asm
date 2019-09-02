// *****************************************************************************
// Copyright (c) 2007 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
//
// *****************************************************************************


#include "music_example.h"
#include "spi_comm_library.h"
#include "stack.h"
#include "cbops.h"
#include "audio_proc_library.h"


// SPI Message Handlers
.MODULE $M.music_example_spi;
   .DATASEGMENT DM;
   .VAR status_message_struc[$spi_comm.STRUC_SIZE];
   .VAR version_message_struc[$spi_comm.STRUC_SIZE];
   .VAR reinit_message_struc[$spi_comm.STRUC_SIZE];
   .VAR parameter_message_struc[$spi_comm.STRUC_SIZE];
   .VAR control_message_struc[$spi_comm.STRUC_SIZE];
#ifdef SPDIF_ENABLE
   .VAR spdif_config_message_struc[$spi_comm.STRUC_SIZE];
#endif
.ENDMODULE;


// *****************************************************************************
// MODULE:
//    message handler
//
// DESCRIPTION:
//    User registered message handler
//
// INPUTS:
//    r0 - command ID
//   r1 - Command/Response data pointer
//   r2 - command data length
// OUTPUTS:
//    r8 - response data length
//
// *****************************************************************************
.MODULE $M.music_example.GetParams;

   .CODESEGMENT MUSIC_EXAMPLE_SPI_GETPARAMS_PM;

func:
   r3 = &$M.system_config.data.CurParams;
   r4 = $M.MUSIC_MANAGER.PARAMETERS.STRUCT_SIZE;
   r5 = &$M.system_config.data.DefaultParameters;
   M[r1] = r3;
   M[r1 + 1] = r4;
   M[r1 + 2] = r5;
   r8 = 3;
   rts;
.ENDMODULE;

// *****************************************************************************
// MODULE:
//    message handler
//
// DESCRIPTION:
//    User registered message handler
//
// INPUTS:
//    r0 - command ID
//   r1 - Command/Response data pointer
//   r2 - command data length
// OUTPUTS:
//    r8 - response data length
//
// *****************************************************************************
.MODULE $M.music_example.GetVersion;

   .CODESEGMENT MUSIC_EXAMPLE_SPI_GETVERSION_PM;

func:
   r3 = $MUSIC_MANAGER_SYSID;
   r5 = M[$music_example.Version];
   r6 = M[$current_codec_sampling_rate];
   // Make sure the order of arguments match the
   // SPI messaging document.
   M[r1] = r3;
   M[r1 + 1] = r5;
   M[r1 + 2] = r6;

   r8 = 3;
   rts;
.ENDMODULE;

// *****************************************************************************
// MODULE:
//    message handler
//
// DESCRIPTION:
//    User registered message handler
//
// INPUTS:
//    r0 - command ID
//   r1 - Command/Response data pointer
//   r2 - command data length
// OUTPUTS:
//    r8 - response data length
//
// *****************************************************************************

.MODULE $M.music_example.ReInit;

   .CODESEGMENT MUSIC_EXAMPLE_SPI_REINIT_PM;

func:
   r8 = 1;
   M[$music_example.reinit] = r8;
   r8 = 0;
   rts;
.ENDMODULE;
#ifdef SPDIF_ENABLE
// *****************************************************************************
// MODULE:
//    $M.music_example.GetSpdifConfig
//
// DESCRIPTION:
//    User registered message handler for SPDIF app configuration
//
// INPUTS:
//   r0 - command ID
//   r1 - Command/Response data pointer
//   r2 - command data length
// OUTPUTS:
//    r8 - response data length
//
// *****************************************************************************

.MODULE $M.music_example.GetSpdifConfig;

   .CODESEGMENT MUSIC_EXAMPLE_SPI_GETCONTROL_PM;

func:
   // push rLink onto stack
   $push_rLink_macro;
   r2 = M[r1 + 1]; // spdif config word
   r3 = M[r1 + 2]; // AC-3 config word 1
   r1 = M[r1 + 0]; // AC-3 config word 2
   call $spdif_apply_realtime_config;

   r8 = 0;
   // pop rLink from stack
   jump $pop_rLink_and_rts;
.ENDMODULE;
#endif
// *****************************************************************************
// MODULE:
//    message handler
//
// DESCRIPTION:
//    User registered message handler
//
// INPUTS:
//    r0 - command ID
//   r1 - Command/Response data pointer
//   r2 - command data length
// OUTPUTS:
//    r8 - response data length
//
// *****************************************************************************

.MODULE $M.music_example.GetControl;

   .CODESEGMENT MUSIC_EXAMPLE_SPI_GETCONTROL_PM;
   .DATASEGMENT DM;
   .VAR $ufe_main.message_volume_struc[$MAIN_VOL_MESSAGE_SIZE];
   .VAR $ufe_aux.message_volume_struc[5];
   
   .VAR local_sys_vol;
func:
   // push rLink onto stack
   $push_rLink_macro;

   // SysControl
   // OvrDACgain
   // OvrMode
   I4 = &$music_example.SpiSysControl;
   I0 = r1;
   r10 = LENGTH($music_example.SpiSysControl);
   do lp_copy_control;
      r0 = M[I0,1];
      M[I4,1] = r0;
lp_copy_control:
   
   r1 = M[$music_example.SystemVolume];
   M[local_sys_vol] = r1;
   
   r0 = M[$music_example.SysControl];
   NULL = r0 AND $M.MUSIC_MANAGER.CONTROL.DAC_OVERRIDE;
   if Z jump dontupdateDAC;
   r1 = M[$music_example.OvrSystemVolume];
   // extract system volume
   r1 = r1 AND 0xF;
   M[local_sys_vol] = r1;
   
   // VM expects L & R system volumes to be sent back for DAC override

   M[$music_example.SystemVolume] = r1; 
   r4 = M[local_sys_vol];
   r3 = r4;
   r2 = $music_example.VMMSG.CODEC;
   call $message.send_short;
   
   
      
dontupdateDAC:

   r0 = M[$music_example.SysControl];
   NULL = r0 AND $M.MUSIC_MANAGER.CONTROL.AUX_OVERRIDE;
   if Z jump dontupdateAUX;
   
   //Use System Volume from above
   r1 = M[local_sys_vol];   
   
   //Get Master volume from UFE   
   r2 = M[$music_example.AuxOvrMasterVolumes];

   //UFE does not Send Tone Volume
   r3 = M[$music_example.Main.ToneVolume];
 
   r4 = M[$music_example.AuxOvrTrimVolumes];
   // extract right trim volume
   r5 = r4 ASHIFT -12;
   // extract left trim volume
   r4 = r4 LSHIFT 12;
   r4 = r4 ASHIFT -12;
   
   I0 = &$ufe_aux.message_volume_struc;
   M[I0,1] = r1;  //Sys_volume  
   M[I0,1] = r2;  //Master
   M[I0,1] = r3;  //Tone
   M[I0,1] = r4;  //Left_trim
   M[I0,1] = r5;  //Right_trim
   I0 = null;
   
   r3 = &$ufe_aux.message_volume_struc;
   call $M.music_example_message.AuxVolume.update_volumes;
dontupdateAUX:   

   r0 = M[$music_example.SysControl];
   NULL = r0 AND $M.MUSIC_MANAGER.CONTROL.MAIN_OVERRIDE;
   if Z jump dontupdateMain;

   //Use System Volume from above
   r1 = M[local_sys_vol];   
   
   //Get Master volume from UFE
   r2 = M[$music_example.MainOvrMasterVolumes];

   //UFE does not Send Tone Volume
   r3 = M[$music_example.Main.ToneVolume];
 
   r4 = M[$music_example.PriOvrTrimVolumes];
   // extract right trim volume
   r5 = r4 ASHIFT -12;
   // extract left trim volume
   r4 = r4 LSHIFT 12;
   r4 = r4 ASHIFT -12;
   
   r6 = M[$music_example.SecOvrTrimVolumes];
   // extract right trim volume
   r7 = r6 ASHIFT -12;
   // extract left trim volume
   r6 = r6 LSHIFT 12;
   r6 = r6 ASHIFT -12;

   r8 = M[$music_example.SubOvrTrimVolumes];
    // extract subwoofer trim volume
   r8 = r8 LSHIFT 12;
   r8 = r8 ASHIFT -12;
   
   I0 = &$ufe_main.message_volume_struc;

   M[I0,1] = r1;  //Sys_volume  
   M[I0,1] = r2;  //Master
   M[I0,1] = r3;  //Tone
   M[I0,1] = r4;  //Pri_left_trim
   M[I0,1] = r5;  //Pri_right_trim
   //Index registers work only with r0-r5
   r0 = r6;
   M[I0,1] = r0;  //Sec_left_trim
   r0 = r7;
   M[I0,1] = r0;  //Sec_right_trim
   r0 = r8;
   M[I0,1] = r0;  //Sub_trim
   I0 = null;
   
#ifdef TWS_ENABLE
   r0 = M[$relay.mode];
   null = r0 - $TWS_MASTER_MODE;
   if NZ jump check_slave;
      r1 = M[$tws.override_flag];
      if NZ jump skip_save_m;

      I0 = &$tws.message_volume_struc;
      I4 = &$tws.message_volume_struc_temp;
   
      r10 = 8;
      do save_vol_struc_m;
      r0 = M[I0,1];  //Sys_volume 
      M[I4,1] = r0;
      save_vol_struc_m:
      I4 = null;
      
      skip_save_m:
      // set override flag for TWS saved settings
      r1 = 1;
      M[$tws.override_flag] = r1;
      I0 = &$tws.message_volume_struc;
      
      M[I0,1] = r1;  //Sys_volume  
      M[I0,1] = r2;  //Master
      M[I0,1] = r3;  //Tone
      M[I0,1] = r4;  //Pri_left_trim
      M[I0,1] = r5;  //Pri_right_trim
      //Index registers work only with r0-r5
      r0 = r6;
      M[I0,1] = r0;  //Sec_left_trim
      r0 = r7;
      M[I0,1] = r0;  //Sec_right_trim
      r0 = r8;
      M[I0,1] = r0;  //Sub_trim
      I0 = null;

      
      jump dontupdateDAC1;
   check_slave:
      null = r0 - $TWS_SLAVE_MODE;
      if NZ jump no_sync;
      r1 = M[$tws.override_flag];
      if NZ jump skip_save_s;
      
      I0 = &$tws.message_volume_struc;
      I4 = &$tws.message_volume_struc_temp;
   
      r10 = 8;
      do save_vol_struc_s;
      r0 = M[I0,1];  //Sys_volume 
      M[I4,1] = r0;
      save_vol_struc_s:
      I4 = null;
      skip_save_s:
      // set override flag for TWS saved settings
      r1 = 1;
      M[$tws.override_flag] = r1;
      I0 = &$tws.message_volume_struc;
      
      r0 = M[I0,1];  // skip Sys_volume  
      r0 = M[I0,1];  // skip Master
      M[I0,1] = r3;  // Tone
      M[I0,1] = r4;  //Pri_left_trim
      M[I0,1] = r5;  //Pri_right_trim
      //Index registers work only with r0-r5
      r0 = r6;
      M[I0,1] = r0;  //Sec_left_trim
      r0 = r7;
      M[I0,1] = r0;  //Sec_right_trim
      r0 = r8;
      M[I0,1] = r0;  //Sub_trim
      I0 = null;

      r3 = &$tws.message_volume_struc;
      call $M.music_example_message.MainVolume.update_volumes;
      jump dontupdateDAC1;
   no_sync:
#endif

   r3 = &$ufe_main.message_volume_struc;
   call $M.music_example_message.MainVolume.update_volumes;
dontupdateMain:  
 
#ifdef TWS_ENABLE
   null = M[$tws.override_flag];
   if Z jump dontupdateDAC1; 
      I0 = &$tws.message_volume_struc;
      I4 = &$tws.message_volume_struc_temp;
   
      r10 = 8;
      do restore_vol;
      r0 = M[I4,1];  //Sys_volume 
      M[I0,1] = r0;
      restore_vol:
      I0 = null;
      I4 = null;
      M[$tws.override_flag]= null;
      r3 = &$tws.message_volume_struc;
      call $M.music_example_message.MainVolume.update_volumes;

#endif 
dontupdateDAC1:
   r8 = 0;

   // pop rLink from stack
   jump $pop_rLink_and_rts;
.ENDMODULE;

// *****************************************************************************
// MODULE:
//    message handler
//
// DESCRIPTION:
//    User registered message handler
//
// INPUTS:
//    r0 - command ID
//   r1 - Command/Response data pointer
//   r2 - command data length
// OUTPUTS:
//    r8 - response data length
//
// *****************************************************************************
.MODULE $M.music_example.GetStatus;

   .CODESEGMENT MUSIC_EXAMPLE_SPI_GETSTATUS_PM;

func:
   // Pointer to Payload
   I1 = r1;
   // Copy Status
   r10 = $M.MUSIC_MANAGER.STATUS.BLOCK_SIZE;
   // Payload Size
   r8 = r10;
   I4 = &$M.system_config.data.StatisticsPtrs;
   r1 = M[I4,1];
   do lp_copy_status;
      // Dereference Pointer
      r1 = M[r1];
      // Copy, Next POinter
      M[I1,1] = r1, r1 = M[I4,1];
      // Bug in BC3MM in index register feed forward
      nop;
lp_copy_status:
   // Clear Peak Statistics
   r10 = LENGTH($music_example.Statistics);
   I4 = &$music_example.Statistics;
   r2 = r2 XOR r2;
   do loop_clr_statistics;
      M[I4,1] = r2;
loop_clr_statistics:

    // Clear Peak Detection
    M[$M.system_config.data.pcmin_l_pk_dtct + $M.audio_proc.peak_monitor.PEAK_LEVEL] = Null;
    M[$M.system_config.data.pcmin_r_pk_dtct + $M.audio_proc.peak_monitor.PEAK_LEVEL] = Null;
#if defined(SPDIF_ENABLE) && defined(AC3_ENABLE)
    M[$M.system_config.data.pcmin_lfe_pk_dtct + $M.audio_proc.peak_monitor.PEAK_LEVEL] = Null;
#endif // defined(SPDIF_ENABLE) && defined(AC3_ENABLE)

    M[$M.system_config.data.primout_l_pk_dtct + $M.audio_proc.peak_monitor.PEAK_LEVEL] = Null;
    M[$M.system_config.data.primout_r_pk_dtct + $M.audio_proc.peak_monitor.PEAK_LEVEL] = Null;
    M[$M.system_config.data.sub_pk_dtct + $M.audio_proc.peak_monitor.PEAK_LEVEL] = Null;
    M[$M.system_config.data.scndout_l_pk_dtct + $M.audio_proc.peak_monitor.PEAK_LEVEL] = Null;
    M[$M.system_config.data.scndout_r_pk_dtct + $M.audio_proc.peak_monitor.PEAK_LEVEL] = Null;
    M[$M.system_config.data.auxout_l_pk_dtct  + $M.audio_proc.peak_monitor.PEAK_LEVEL] = Null;
    M[$M.system_config.data.auxout_r_pk_dtct  + $M.audio_proc.peak_monitor.PEAK_LEVEL] = Null;
    
   rts;
   
.ENDMODULE;

