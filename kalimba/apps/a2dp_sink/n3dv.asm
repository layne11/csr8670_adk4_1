// *****************************************************************************
// Copyright (c) 2009 - 2016 Qualcomm Technologies International, Ltd.         http://www.csr.com
// Part of ADK 4.1
//
// $Change: 2433201 $  $DateTime: 2015/11/12 10:46:51 $
// *****************************************************************************

#include "music_example.h"
#include "music_manager_config.h"
#include "vse.h"

#if uses_3DV

// Internal VSE buffers
.MODULE $M.n3dv;
   .DATASEGMENT DM;

   .CONST $N3DV_INTERNAL_BUFFER_SIZE            60;

   .BLOCK left_int;
      .VAR left_int.out_ipsi_buf[$N3DV_INTERNAL_BUFFER_SIZE];
      .VAR left_int.out_contra_buf[$N3DV_INTERNAL_BUFFER_SIZE];
   .ENDBLOCK;

   .BLOCK right_int;
      .VAR right_int.out_ipsi_buf[$N3DV_INTERNAL_BUFFER_SIZE];
      .VAR right_int.out_contra_buf[$N3DV_INTERNAL_BUFFER_SIZE];
   .ENDBLOCK;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $n3dv_init_wrapper
//
// DESCRIPTION:
//    Wrapper function to initialise the 3DV feature
//    (this calls the VSE initialise function but also initialises
//    some additional internal buffers)
//
// INPUTS:
//    - r7 = pointer to the vse left channel data object
//    - r8 = pointer to the vse right channel data object
//
// OUTPUTS:
//    none
//
// TRASHED:
//    - rMAC,r0,r1,r2,r3,r4,r5,r6,r7,I7
//
// *****************************************************************************
.MODULE $M.n3dv_init_wrapper;
   .CODESEGMENT PM;

   $n3dv_init_wrapper:

   $push_rLink_macro;

   push r8;
   r8 = r7;

   // Initialise the left channel VSE internal buffer pointers
   r0 = $M.n3dv.left_int.out_ipsi_buf;
   M[r8 + $audio_proc.vse.out_ipsi] = r0;
   r0 = $M.n3dv.left_int.out_contra_buf;
   M[r8 + $audio_proc.vse.out_contra] = r0;

   // Initialise the left VSE
   call $audio_proc.vse.initialize;
   pop r8;

   // Initialise the right channel VSE internal buffer pointers
   r0 = $M.n3dv.right_int.out_ipsi_buf;
   M[r8 + $audio_proc.vse.out_ipsi] = r0;
   r0 = $M.n3dv.right_int.out_contra_buf;
   M[r8 + $audio_proc.vse.out_contra] = r0;

   // Initialise the right VSE
   call $audio_proc.vse.initialize;

   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $n3dv_process_wrapper
//
// DESCRIPTION:
//    Wrapper function to process the 3DV feature
//    (this allows the 3DV to be selected and bypassed)
//
// INPUTS:
//    - r7 = pointer to the vse left channel data object
//    - r8 = pointer to the vse right channel data object
//
// OUTPUTS:
//    none
//
// TRASHED:
//    - Assume all
//
// *****************************************************************************
.MODULE $M.n3dv_process_wrapper;
   .CODESEGMENT PM;

   $n3dv_process_wrapper:

   $push_rLink_macro;

   // Get the spatial enhancement selection
   r1 = M[$M.system_config.data.CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SPATIAL_ENHANCEMENT_SELECTION];

   // 3DV selected?
   null = r1 AND $M.MUSIC_MANAGER.CONFIG.SPATIAL_ENHANCEMENT_SELECT_3DV;
   if Z jump exit;   // No - exit

      // Get the current configuration word (this controls which MM features are enabled)
      r0 = M[$M.system_config.data.CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_CONFIG];

      // Spatial enhancement bypassed?
      null = r0 AND $M.MUSIC_MANAGER.CONFIG.SPATIAL_ENHANCEMENT_BYPASS;

      // No - process the left and right 3DV channel
      if Z call $audio_proc.vse.frame_process;

   exit:

   jump $pop_rLink_and_rts;

.ENDMODULE;

#endif
