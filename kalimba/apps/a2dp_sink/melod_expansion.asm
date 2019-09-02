// *****************************************************************************
// Copyright (c) 2009 - 2016 Qualcomm Technologies International, Ltd.         http://www.csr.com
// Part of ADK 4.1
//
// $Change: 2433201 $  $DateTime: 2015/11/12 10:46:51 $
// *****************************************************************************

#include "music_example.h"
#include "music_manager_config.h"

#if uses_STEREO_ENHANCEMENT

// *****************************************************************************
// MODULE:
//    $melod_expansion_process_wrapper
//
// DESCRIPTION:
//    Wrapper function to process the MeloD Expansion feature
//    (this allows the MeloD Expansion to be selected and bypassed)
//
// INPUTS:
//    - r7 = pointer to the MeloD Expansion data object
//      r8 = 0
//
// OUTPUTS:
//    none
//
// TRASHED:
//    - Assume all
//
// *****************************************************************************
.MODULE $M.melod_expansion_process_wrapper;
   .CODESEGMENT PM;

   $melod_expansion_process_wrapper:

   $push_rLink_macro;

   // Get the spatial enhancement selection
   r1 = M[$M.system_config.data.CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SPATIAL_ENHANCEMENT_SELECTION];

   // MeloD Expansion selected?
   null = r1 AND $M.MUSIC_MANAGER.CONFIG.SPATIAL_ENHANCEMENT_SELECT_MELOD_EXPANSION;
   if Z jump exit;   // No - exit

      // Process the left and right MeloD Expansion channels
      // r7 = MeloD Expansion data object, r8 = 0
      call $MeloD_Expansion.process;

   exit:

   jump $pop_rLink_and_rts;

.ENDMODULE;

#endif
