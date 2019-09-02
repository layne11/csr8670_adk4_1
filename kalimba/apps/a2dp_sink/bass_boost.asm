// *****************************************************************************
// Copyright (c) 2009 - 2016 Qualcomm Technologies International, Ltd.         http://www.csr.com
// Part of ADK 4.1
//
// $Change: 2433201 $  $DateTime: 2015/11/12 10:46:51 $
// *****************************************************************************

#include "music_example.h"
#include "music_manager_config.h"

#if uses_BASS_BOOST

// *****************************************************************************
// MODULE:
//    $bass_boost_process_wrapper
//
// DESCRIPTION:
//    Wrapper function to process the Bass Boost feature
//    (this allows the bass_boost to be selected and bypassed)
//
// INPUTS:
//    - r7 = pointer to the bass boost left channel data object
//    - r8 = pointer to the bass boost right channel data object
//
// OUTPUTS:
//    none
//
// TRASHED:
//    - Assume all
//
// *****************************************************************************
.MODULE $M.bass_boost_process_wrapper;
   .CODESEGMENT PM;

   $bass_boost_process_wrapper:

   $push_rLink_macro;

   // Get the bass enhancement selection
   r1 = M[$M.system_config.data.CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_BASS_ENHANCEMENT_SELECTION];

   // Bass Boost selected?
   null = r1 AND $M.MUSIC_MANAGER.CONFIG.BASS_ENHANCEMENT_SELECT_BASS_BOOST;
   if Z jump exit;   // No - exit

      push r8;

      r8 = $M.MUSIC_MANAGER.CONFIG.BASS_ENHANCEMENT_BYPASS;

      // Process the left channel
      // r7 = Bass enhancement data object, r8 = bypass bitfield?
      call $music_example.peq.process;

      pop r7;

      r8 = $M.MUSIC_MANAGER.CONFIG.BASS_ENHANCEMENT_BYPASS;

      // Process the right channel
      // r7 = Bass enhancement data object, r8 = bypass bitfield?
      call $music_example.peq.process;

   exit:

   jump $pop_rLink_and_rts;

.ENDMODULE;

#endif
