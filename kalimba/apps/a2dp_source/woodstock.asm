// *****************************************************************************
// Copyright (c) 2015 - 2016 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
//
// *****************************************************************************

#ifdef WOODSTOCK

#include "core_library.h"

.MODULE $M.woodstock;
   .DATASEGMENT DM;

   .VAR $woodstock_peq_message_struc[$message.STRUC_SIZE];              // receive PEQ index from VM
   .VAR $WOODSTOCK_MUTE_SPEAKER_MESSAGE_struc[$message.STRUC_SIZE];     // receive mute speaker flag from VM
   .VAR $WOODSTOCK_MUTE_MICROPHONE_MESSAGE_struc[$message.STRUC_SIZE];  // receive mute microphone flag from VM
   .VAR $woodstock_mute_spkr_flag = 0;                                  // default mute spkr status is to pass audio
   .VAR $woodstock_mute_mic_flag = 0;                                   // default mute mic status is to pass audio
.ENDMODULE;

// *********************************************************************************
// MODULE:
//    $woodstock_peq_message_handler
//
// DESCRIPTION:
//   change eq curve
//
// INPUTS:
//    - r0 = message_ID
//    - r1 = data[0] = new EQ curve ID
// OUTPUTS:
//    - none
//
// TRASHED REGISTERS:
//
// **********************************************************************************
.MODULE $M.woodstock_peq_message_handler;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   .VAR $current_key;
   .VAR $ps_key_struc[$pskey.STRUC_SIZE];

   //-------------------------------------------------------------------
   $woodstock_peq_message_handler:
   //-------------------------------------------------------------------
   // peq change curve message.  Sends get PSKey message to VM
   //-------------------------------------------------------------------
   // PSKey = PSKEY_DSP0 + r1
   // Send Get PSKey message to VM

   // push rLink onto stack
   $push_rLink_macro;

// http://intranet01/group/software/psbc/key_pskey_dsp0.html
#define PSKEY_DSP0 0x2258

   r2 = PSKEY_DSP0;
   r2 = r2 + r1;
   r1 = $ps_key_struc;
   r3 = $woodstock_PsKeyReadHandler;
   call $pskey.read_key;
   nop;

   // pop rLink onto stack
   jump $pop_rLink_and_rts;

   //-------------------------------------------------------------------
   $woodstock_PsKeyReadHandler:
   //-------------------------------------------------------------------
        // Extract PSKey into Coefficient memory for PEQ
   //-------------------------------------------------------------------
   // r1 = Key ID
   // r2 = Buffer Length; $pskey.FAILED_READ_LENGTH on failure
   // r3 = Payload. Key ID Plus data

   NULL = r2 - $pskey.FAILED_READ_LENGTH;
   if Z rts;

   // bomb out if key length is wrong
   NULL = r2 - 46;
   if NZ rts;

   // push rLink onto stack
   $push_rLink_macro;

   I0 = r3;
   r0 = M[I0,1];            // read current key
   M[$current_key] = r0;

   // copy coefficients from PSKEY into coefficient buffer

   I4 = $M.peq.peq_coeffs1;
   r10 = 15;
   do woodstock_read_coefficients_pskey_loop;
      r0 = M[I0,1];
      r1 = M[I0,1];
      r2 = M[I0,1];

      r0 = r0 LSHIFT 8;
      r3 = r1 LSHIFT 8;       // gets rid of 8 bits of sign extension
      r3 = r3 LSHIFT -16;
      r0 = r0 OR r3;

      r1 = r1 LSHIFT 16;
      r2 = r2 LSHIFT 8;
      r2 = r2 LSHIFT -8;
      r1 = r1 OR r2;

      M[I4,1] = r0;
      M[I4,1] = r1;
   woodstock_read_coefficients_pskey_loop:

   // pop rLink onto stack
   jump $pop_rLink_and_rts;

.ENDMODULE;

// *********************************************************************************
// MODULE:
//    $WOODSTOCK_MUTE_SPEAKER_MESSAGE_handler
//
// DESCRIPTION:
//   change speaker mute status
//
// INPUTS:
//    - r0 = message_ID
//    - r1 = data[0] = mute state
// OUTPUTS:
//    - none
//
// TRASHED REGISTERS:
//
// **********************************************************************************
.MODULE $M.WOODSTOCK_MUTE_SPEAKER_MESSAGE_handler;
   .CODESEGMENT PM;

   $WOODSTOCK_MUTE_SPEAKER_MESSAGE_handler:

   // store the new state
   M[$woodstock_mute_spkr_flag] = r1;

   rts;

.ENDMODULE;

// *********************************************************************************
// MODULE:
//    $WOODSTOCK_MUTE_MICROPHONE_MESSAGE_handler
//
// DESCRIPTION:
//   change microphone mute status
//
// INPUTS:
//    - r0 = message_ID
//    - r1 = data[0] = mute state
// OUTPUTS:
//    - none
//
// TRASHED REGISTERS:
//
// **********************************************************************************
.MODULE $M.WOODSTOCK_MUTE_MICROPHONE_MESSAGE_handler;
   .CODESEGMENT PM;

   $WOODSTOCK_MUTE_MICROPHONE_MESSAGE_handler:

   // store the new state
   M[$woodstock_mute_mic_flag] = r1;

#if defined(ENABLE_FASTSTREAM_VOICE) || defined(ENABLE_BACK_CHANNEL)
   // if mute, then change usb output shift to 24 (effectively mutes audio)
   r2 = -8;
   r3 = -24;
   r1 = M[$woodstock_mute_mic_flag];
   if NZ r2 = r3;
   M[$faststream_voice_shift_op.param + 2]=r2;
#endif

   rts;

.ENDMODULE;

#endif // WOODSTOCK