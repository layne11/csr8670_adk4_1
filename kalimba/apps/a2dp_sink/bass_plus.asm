// *****************************************************************************
// Copyright (c) 2009 - 2016 Qualcomm Technologies International, Ltd.         http://www.csr.com
// Part of ADK 4.1
//
// $Change: 2433201 $  $DateTime: 2015/11/12 10:46:51 $
// *****************************************************************************

#include "music_example.h"
#include "music_manager_config.h"
#include "dbe.h"

#if uses_BASS_PLUS

// Internal DBE buffers
.MODULE $M.bass_plus;
   .DATASEGMENT DM;

   .BLOCK left_int;
      .VAR left_int.hp1_out_buf[$DBE_FRAME_SIZE/2];
      .VAR left_int.hp3_out_buf[$DBE_FRAME_SIZE/2];
      .VAR left_int.hp2_out_buf[$DBE_FRAME_SIZE];
      .VAR left_int.ntp_tp_filters_buf[$DBE_FRAME_SIZE/2];
      .VAR left_int.high_freq_output_buf[$DBE_FRAME_SIZE];
   .ENDBLOCK;

   .BLOCK right_int;
      .VAR right_int.hp1_out_buf[$DBE_FRAME_SIZE/2];
      .VAR right_int.hp3_out_buf[$DBE_FRAME_SIZE/2];
      .VAR right_int.hp2_out_buf[$DBE_FRAME_SIZE];
      .VAR right_int.ntp_tp_filters_buf[$DBE_FRAME_SIZE/2];
      .VAR right_int.high_freq_output_buf[$DBE_FRAME_SIZE];
   .ENDBLOCK;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $bass_plus_init_wrapper
//
// DESCRIPTION:
//    Wrapper function to initialise the Bass+ feature
//    (this calls the DBE initialise function but also initialises
//    some additional internal buffers)
//
// INPUTS:
//    - r7 = pointer to the dbe left channel data object
//    - r8 = pointer to the dbe right channel data object
//
// OUTPUTS:
//    none
//
// TRASHED:
//    - rMAC,r0,r1,r2,r3,r4,r5,r6,r7,I7
//
// *****************************************************************************
.MODULE $M.bass_plus_init_wrapper;
   .CODESEGMENT PM;

   $bass_plus_init_wrapper:

   $push_rLink_macro;

   push r8;
   r8 = r7;

   // Initialise the left channel DBE internal buffer pointers
   r0 = $M.bass_plus.left_int.hp1_out_buf;
   M[r8 + $audio_proc.dbe.hp1_out] = r0;
   r0 = $M.bass_plus.left_int.hp3_out_buf;
   M[r8 + $audio_proc.dbe.hp3_out] = r0;
   r0 = $M.bass_plus.left_int.hp2_out_buf;
   M[r8 + $audio_proc.dbe.hp2_out] = r0;
   r0 = $M.bass_plus.left_int.ntp_tp_filters_buf;
   M[r8 + $audio_proc.dbe.ntp_tp_filters_buf] = r0;
   r0 = $M.bass_plus.left_int.high_freq_output_buf;
   M[r8 + $audio_proc.xover.high_freq_output_buf] = r0;

   // Initialise the left DBE
   call $audio_proc.dbe.initialize;
   pop r8;

   // Initialise the right channel DBE internal buffer pointers
   r0 = $M.bass_plus.right_int.hp1_out_buf;
   M[r8 + $audio_proc.dbe.hp1_out] = r0;
   r0 = $M.bass_plus.right_int.hp3_out_buf;
   M[r8 + $audio_proc.dbe.hp3_out] = r0;
   r0 = $M.bass_plus.right_int.hp2_out_buf;
   M[r8 + $audio_proc.dbe.hp2_out] = r0;
   r0 = $M.bass_plus.right_int.ntp_tp_filters_buf;
   M[r8 + $audio_proc.dbe.ntp_tp_filters_buf] = r0;
   r0 = $M.bass_plus.right_int.high_freq_output_buf;
   M[r8 + $audio_proc.xover.high_freq_output_buf] = r0;

   // Initialise the right DBE
   call $audio_proc.dbe.initialize;

   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $bass_plus_process_wrapper
//
// DESCRIPTION:
//    Wrapper function to process the Bass+ feature
//    (this allows the bass_plus to be selected and bypassed)
//
// INPUTS:
//    - r7 = pointer to the dbe left channel data object
//    - r8 = pointer to the dbe right channel data object
//
// OUTPUTS:
//    none
//
// TRASHED:
//    - Assume all
//
// *****************************************************************************
.MODULE $M.bass_plus_process_wrapper;
   .CODESEGMENT PM;

   $bass_plus_process_wrapper:

   $push_rLink_macro;

   // Get the bass enhancement selection
   r1 = M[$M.system_config.data.CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_BASS_ENHANCEMENT_SELECTION];

   // Bass Plus selected?
   null = r1 AND $M.MUSIC_MANAGER.CONFIG.BASS_ENHANCEMENT_SELECT_BASS_PLUS;
   if Z jump exit;   // No - exit

      // Get the current configuration word (this controls which MM features are enabled)
      r0 = M[$M.system_config.data.CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_CONFIG];

      // Bass enhancement bypassed?
      null = r0 AND $M.MUSIC_MANAGER.CONFIG.BASS_ENHANCEMENT_BYPASS;

      // No - process the left and right DBE channel
      if Z call $audio_proc.dbe.frame_process;

   exit:

   jump $pop_rLink_and_rts;

.ENDMODULE;

#endif
