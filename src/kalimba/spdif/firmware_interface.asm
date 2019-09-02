// *****************************************************************************
// Copyright (C) Cambridge Silicon Radio Limited 2012-2015        http://www.csr.comes/ Part of ADK 3.5.1
//
// $Change$  $DateTime$
// $Revision$
// *****************************************************************************

// *****************************************************************************
// DESCRIPTION
//
//   DSP receives messages from firmware about the status of the spdif Rx
//   stream. Vm needs to enable this feature in order to firmware sending DSP
//   the releavant messages.
//
//   Messages that are processed by DSP are:
//   $MESSAGE_SPDIF_CHNL_STS_EVENT: channel status words for spdif Rx stream
//   $MESSAGE_AUDIO_STREAM_RATE_EVENT: Indicates the rate of audio stream
//   $MESSAGE_SPDIF_BLOCK_START_EVENT: Indicates that the block start has been
//   received
//
// *****************************************************************************
#ifndef SPDIF_FIRMARE_INTERFACE_ASM_INCLUDED
#define SPDIF_FIRMARE_INTERFACE_ASM_INCLUDED
#include "core_library.h"
#include "spdif.h"
#include "spdif_stream_decode.h"

// *****************************************************************************
// MODULE:
//    $spdif.frame_copy.register_spdif_stream_messages_handlers
//
// DESCRIPTION:
//    registering handlers for spdif Rx messages from firmware
//
// INPUTS:
//    None
//
// OUTPUTS:
//    None
//
// TRASHED REGISTERS:
//    assume everything
// *****************************************************************************
.MODULE $M.spdif.frame_copy.register_spdif_stream_messages_handlers;
   .CODESEGMENT SPDIF_FRAME_COPY_RGISTER_SPDIF_STREAM_MESSAGE_HANDLERS_PM;
   .DATASEGMENT DM;

   $spdif.frame_copy.register_spdif_stream_messages_handlers:

   // push rLink onto stack
   $push_rLink_macro;

   // set up message handler for  $MESSAGE_SPDIF_CHNL_STS_EVENT fw message
   r1 = &$spdif.frame_copy.chnl_sts_event_message_struc;
   r2 = $MESSAGE_SPDIF_CHNL_STS_EVENT;
   r3 = &$spdif.frame_copy.chnl_sts_event_message_handler;
   call $message.register_handler;

   // set up message handler for $MESSAGE_AUDIO_STREAM_RATE_EVENT firmware message
   r1 = &$spdif.frame_copy.valid_stream_event_message_struc;
   r2 = $MESSAGE_AUDIO_STREAM_RATE_EVENT;
   r3 = &$spdif.frame_copy.valid_stream_event_message_handler;
   call $message.register_handler;

   // set up message handler for $MESSAGE_SPDIF_BLOCK_START_EVENT firmware message
   r1 = &$spdif.frame_copy.block_start_message_struc;
   r2 = $MESSAGE_SPDIF_BLOCK_START_EVENT;
   r3 = &$spdif.frame_copy.block_start_message_handler;
   call $message.register_handler;

   // set up message handler for $MESSAGE_ACTIVATE_AUDIO_RESPONSE firmware response message
   r1 = &$spdif.frame_copy.activate_audio_response_message_struc;
   r2 = $MESSAGE_ACTIVATE_AUDIO_RESPONSE;
   r3 = &$spdif.frame_copy.activate_audio_response_message_handler;
   call $message.register_handler;

   // pop rLink from stack
   jump $pop_rLink_and_rts;

.ENDMODULE;
// *****************************************************************************
// MODULE:
//    $spdif.frame_copy.chnl_sts_stream_event_message_handler
//
// DESCRIPTION:
//    handler for $MESSAGE_SPDIF_CHNL_STS_EVENT message from firmware
//
// INPUTS:
//   None
//
// OUTPUTS:
//   None
//
// TRASHED REGISTERS:
//    assume everything
// *****************************************************************************
.MODULE $M.spdif.frame_copy.chnl_sts_event_message_handler;
   .CODESEGMENT SPDIF_FRAME_COPY_CHNL_STS_EVENT_MESSAGE_HANDLER_PM;
   .DATASEGMENT DM;

 $spdif.frame_copy.chnl_sts_event_message_handler:
   // push rLink onto stack
   $push_rLink_macro;

   .VAR chts_fs[] = 44100,-1,48000,32000;

   r8 = M[$spdif.frame_copy.frame_copy_struct];
   // get number of ports
   I2 = r3;
   r0 = M[I2, 1];
   #ifdef DEBUG_ON
      Null = r0;
      if LE call $error;
      Null = r0 - 2;
      if GT call $error;
   #endif

   // read the channel status bits
   // I6 = status words
   // r1 = port
   I6 = I2 + r0;
   r1 = M[I2, 1];
   push r0;
   call update_chsts;
   pop r0;

   // see if it's for two channels
   Null = r0 - 2;
   if NZ jump update_chsts_done;

   // read the channel status bits for second channel
   // I6 = status words
   // r1 = port
   I6 = I2 + r0;
   r1 = M[I2, 1];
   call update_chsts;
   update_chsts_done:

   // extract sampling frequency and audio/data mode bit
   r0 = M[I2, 1];
   M[r8 + $spdif.frame_copy.CHSTS_FIRST_WORD_FIELD] = r0;
   r0 = r0 AND 0x2;
   r4 = M[I2, 1];
   r4 = r4 LSHIFT -8;
   r4 = r4 AND 0x3;
   r4 = M[chts_fs + r4];
   r3 = r0 LSHIFT -1;
   M[r8 + $spdif.frame_copy.CHSTS_DATA_MODE_FIELD] = r3;
   M[r8 + $spdif.frame_copy.CHSTS_SAMPLE_RATE_FIELD] = r4;

   #ifdef SPDIF_REPORT_EVENTS
      // channel status received,
      // report changes
      call $spdif.report_event;
   #endif

   // pop rLink from stack
   jump $pop_rLink_and_rts;

   // ----------------------------
   // Get the channel status
   // r1:  port
   // I6:  channel status data
   //
   // ---------------------------
   update_chsts:
   $push_rLink_macro;

   // see if its for channel A
   r2 = M[r8 + $spdif.frame_copy.LEFT_INPUT_PORT_FIELD];
   r2 = r2 AND $cbuffer.TOTAL_PORT_NUMBER_MASK;
   Null = r2 - r1;
   if Z jump is_channel_A;

   // see if it's for channel B
   r5 = 1;
   r2 = M[r8 + $spdif.frame_copy.RIGHT_INPUT_PORT_FIELD];
   r2 = r2 AND $cbuffer.TOTAL_PORT_NUMBER_MASK;
   Null = r2 - r1;
   if Z jump is_channel_B;
   #ifdef DEBUG_ON
     call $error;
   #endif
   is_channel_A:
   r5 = 0;
   is_channel_B:

   // r5 = 0 if channle A else 1
   I3 = r8 + $spdif.frame_copy.CHSTS_WORDS_FIELD;
   r0 = r5 * $spdif.CHANNEL_FULL_FULL_REPORT_SIZE  (int);
   I3 = I3 + r0;
   M[I3, 1] = r5;
   r10 = 12;
   r2 = 0;
   do copy_chts_loop;
      r0 = M[I6, 1], r1 = M[I3, 0];
      r0 = r0 AND 0xFFFF;
      r1 = r1 - r0;
      r2 = r2 OR r1, M[I3, 1] = r0;
   copy_chts_loop:
   // no further action if all the bits are the same
   Null = r2;
   if Z jump $pop_rLink_and_rts;

   // channel status bits have changed,
   // if full bits report has been
   // requested now it's time to report them
   r0 = M[r8 + $spdif.frame_copy.CHSTS_FULL_BITS_REPORT_FIELD];
   r5 = 1 LSHIFT r5;
   r0 = r0 AND r5;
   if Z jump $pop_rLink_and_rts;


   // send full channel status
   r3 = $SPDIF_FULL_CHSTS_WORDS_MESSAGE_ID;
   r4 = $spdif.CHANNEL_FULL_FULL_REPORT_SIZE;
   r5 = I3 - $spdif.CHANNEL_FULL_FULL_REPORT_SIZE;
   push I2;
   call $message.send_long;
   pop I2;

   // pop rLink from stack
   jump $pop_rLink_and_rts;
// *****************************************************************************
// MODULE:
//    $spdif.select_chsts_channel
//
// DESCRIPTION:
//   alternate between status bits channel A & B
// INPUTS:
//   r8 = spdif frame copy structure
//
// OUTPUTS:
//   None
//
// TRASHED REGISTERS:
//    assume everything
// *****************************************************************************
   $spdif.select_chsts_channel:

   // push rLink onto stack
   $push_rLink_macro;

   // get the structures
   r8 = M[$spdif.stream_decode.stream_decode_struct];
   r7 = M[r8 + $spdif.stream_decode.SPDIF_FRAME_COPY_STRUCT_FIELD];

   // no action if confirms is still pending
   Null =  M[r7 + $spdif.frame_copy.FW_CONFIRMS_PENDING_FIELD];
   if NZ jump $pop_rLink_and_rts;

   // no point to alternate if input isn't valid (for any reason)
   Null = M[r7 + $spdif.frame_copy.STREAM_INVALID_FIELD];
   if NZ jump $pop_rLink_and_rts;

   #ifdef SPDIF_OUTPUT_INTERFACE_CONTROL
   // extra check, wait until output is in stable state
   r1 = M[r8 + $spdif.stream_decode.OUTPUT_INTERFACE_PROCESSING_STATE_FIELD];
   r2 = r1 - $spdif.STATE_FULL_ACTIVE;
   r1 = r1 - $spdif.STATE_IDLE;
   r1 = r1 * r2 (int);
   if NZ jump $pop_rLink_and_rts;
   #endif

   // set a lower limit on the period
   r3 = M[r7 + $spdif.frame_copy.CHSTS_ALTERNATING_PERIOD_FIELD];
   r0 = r3 - $spdif.CHSTS_MIN_ALTERNATING_PERIOD;
   if NEG r3 = r3 - r0;

   // see if it's time to alternate
   r4 = M[$TIMER_TIME];
   r1 = M[r7 + $spdif.frame_copy.CHSTS_ALTERNATING_TIMER_FIELD];
   r2 = r4 - r1;
   if NEG r2 = -r2;
   Null = r2 - r3;
   if NEG jump $pop_rLink_and_rts;

   // update timer
   r1 = r1 + r3;
   r2 = r1 - r4;
   if NEG r2 = -r2;
   Null = r2 - r3;
   if POS r1 = r4;
   M[r7 + $spdif.frame_copy.CHSTS_ALTERNATING_TIMER_FIELD] = r1;

   // switch to the other channel
   r3 = M[r7 + $spdif.frame_copy.LEFT_INPUT_PORT_FIELD];
   r0 = M[r7 + $spdif.frame_copy.CHSTS_CURRENT_CHANNEL_FIELD];
   r0 = r0 XOR 1;
   r0 = r0 AND 1;
   M[r7 + $spdif.frame_copy.CHSTS_CURRENT_CHANNEL_FIELD] = r0;
   r3 = r3 AND $cbuffer.TOTAL_PORT_NUMBER_MASK;

   // send a message to fw
   r2 = $MESSAGE_AUDIO_CONFIGURE;
   r4 = 0x501;
   r5 = r0 + 1;
   r6 = 0;
   call $message.send_short;
   call $spdif.frame_copy.increment_pending_confirms;

   // pop rLink from stack
   jump $pop_rLink_and_rts;

.ENDMODULE;
// *****************************************************************************
// MODULE:
//    $spdif.frame_copy.valid_stream_event_message_handler
//
// DESCRIPTION:
//    handler for $MESSAGE_AUDIO_STREAM_RATE_EVENT message from firmware
//
// INPUTS:
//   None
//
// OUTPUTS:
//   None
//
// TRASHED REGISTERS:
//    assume everything
// *****************************************************************************
.MODULE $M.spdif.frame_copy.valid_stream_event_message_handler;
   .CODESEGMENT SPDIF_FRAME_COPY_VALID_STREAM_EVENT_MESSAGE_HANDLER_PM;
   .DATASEGMENT DM;

 $spdif.frame_copy.valid_stream_event_message_handler:
   // push rLink onto stack
   $push_rLink_macro;

   r8 = M[$spdif.frame_copy.frame_copy_struct];

   #ifdef DEBUG_ON
      // number of ports must be 2
      r0 = M[r3 + 0];
      Null = r0 - 2;
      if NZ call $error;

      // check LEFT port
      r1 = M[r3 + 1];
      r2 = M[r8 + $spdif.frame_copy.LEFT_INPUT_PORT_FIELD];
      r2 = r2 AND $cbuffer.TOTAL_PORT_NUMBER_MASK;
      Null = r1 - r2;
      if NZ call $error;

      // check right port
      r1 = M[r3 + 2];
      r2 = M[r8 + $spdif.frame_copy.RIGHT_INPUT_PORT_FIELD];
      r2 = r2 AND $cbuffer.TOTAL_PORT_NUMBER_MASK;
      Null = r1 - r2;
      if NZ call $error;
      #endif

   // extract sample rate and valid flag
   r0 = M[r3 + 3];
   r0 = r0 AND 0xFFFF;
   r1 = M[r3 + 4];
   r1 = r1 LSHIFT 16;
   r3 = r1 + r0;
   M[r8 + $spdif.frame_copy.FW_SAMPLING_FREQ_FIELD] = r3;
#ifdef $spdif.DSP_SPDIF_RATE_MEASUREMENT
   M[r8 + $spdif.frame_copy.RATE_DETECT_HIST_FW_RATE_CHNAGED_FIELD] = r3;
#endif

#ifdef $spdif.FLEXIBLE_HIGH_RATES_SUPPORT
   // DSP can restart the interface if high rate is reported by FW
   r0 = M[$TIMER_TIME];
   M[r8 + $spdif.frame_copy.HIGH_RATE_RATE_DETECT_TIMER_TIME_FIELD] = r0;
   r0 = 1;
   M[r8 + $spdif.frame_copy.CAN_RESTART_INTERFACE_FIELD] = r0;
#endif

   call $spdif.frame_copy.process_valid_rate_message;

   // pop rLink from stack
   jump $pop_rLink_and_rts;

   $spdif.frame_copy.process_valid_rate_message:

   // push rLink onto stack
   $push_rLink_macro;

   r3 = M[r8 + $spdif.frame_copy.FW_SAMPLING_FREQ_FIELD];
#ifdef $spdif.DSP_SPDIF_RATE_CHECK
   r0 = M[r8 + $spdif.frame_copy.DSP_SAMPLING_FREQ_FIELD];
   #ifdef $spdif.FLEXIBLE_HIGH_RATES_SUPPORT
      // if:
      //       FW  has reported a high rate and,
      //       DSP has detected a high rate
      // then:
      //       DSP rate will be the final decision
      //       even if they are different
      //       for other rates FW & DSP should agree on rate
      Null = r3 - 48000;
      if LE jump no_high_rate;
      Null = r0 - 48000;
      if GT r3 = r0;
      no_high_rate:
   #endif
   Null = r0 - r3;
   if NZ r3 = 0;
#endif
   Null = r3;
   if Z jump is_invalid;

      // can restart immediately
      r0 = 2;
      M[r8 + $spdif.frame_copy.CAN_RESTART_INTERFACE_FIELD] = r0;

      // update the sampling rate
      M[r8 + $spdif.frame_copy.SAMPLING_FREQ_FIELD] = r3;

      #ifdef SPDIF_LATENCY_MEASUREMENT
         // calculate inverse sample rate
         // in suitable format for latency measurements
         r0 = r3;
         call $latency.calc_inv_fs;
         M[r8 + $spdif.frame_copy.INV_SAMPLING_FREQ_FIELD] = r0;
      #endif

      // stream is valid from fw side, clear related flag
      r4 = M[r8 + $spdif.frame_copy.STREAM_INVALID_FIELD];
      r4 = r4 AND (~$spdif.STREAM_INVALID_FROM_FW);

      // stream is valid
      M[r8 + $spdif.frame_copy.STREAM_INVALID_FIELD] = r4;
      jump message_process_done;

   is_invalid:
      // stream is invalid from fw side, clear related flag
      r4 = 1;
      M[r8 + $spdif.frame_copy.RESET_NEEDED_FIELD] = r4;

   // now invalidate the stream
   r4 = M[r8 + $spdif.frame_copy.STREAM_INVALID_FIELD];
   r4 = r4 OR $spdif.STREAM_INVALID_FROM_FW;
   M[r8 + $spdif.frame_copy.STREAM_INVALID_FIELD] = r4;

   message_process_done:
   #ifdef SPDIF_REPORT_EVENTS
      // report the event
      call $spdif.report_event;
   #endif

   // inform output interface handler of the event
   #ifdef SPDIF_OUTPUT_INTERFACE_CONTROL
       call $spdif.output_interface_control.event_process;
   #endif

   // pop rLink from stack
   jump $pop_rLink_and_rts;

.ENDMODULE;
// *****************************************************************************
// MODULE:
//    $spdif.frame_copy.block_start_message_handler
//
// DESCRIPTION:
//    handler for $KAL_MSG_BLOCK_START_MESSAGE_ID message from firmware
//
// INPUTS:
//   None
//
// OUTPUTS:
//   None
//
// TRASHED REGISTERS:
//    assume everything
// *****************************************************************************
.MODULE $M.spdif.frame_copy.block_start_message_handler;
   .CODESEGMENT SPDIF_FRAME_COPY_BLOCK_START_MESSAGE_HANDLER_PM;
   .DATASEGMENT DM;

 $spdif.frame_copy.block_start_message_handler:
 rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $spdif.output_interface_control.response_message_handler
//
// DESCRIPTION:
//    handler for $MESSAGE_AUDIO_CONFIGURE_RESPONSE and
//    $MESSAGE_ACTIVATE_AUDIO_RESPONSE messages from firmware
//
// INPUTS:
//   r1 = port number
//   r2 = response (0 means failure)
//
// OUTPUTS:
//    None
//
// TRASHED REGISTERS:
//    assume everything
// *****************************************************************************
.MODULE $M.spdif.frame_copy.response_message_handler;
   .CODESEGMENT SPDIF_OUTPUT_INTERFACE_CONTROL_RESPONSE_MESSAGE_HANDLER_PM;
   .DATASEGMENT DM;

   $spdif.frame_copy.audio_configure_response_message_handler:
   $spdif.frame_copy.activate_audio_response_message_handler:
   $spdif.frame_copy.response_message_handler:


   // push rLink onto stack
   $push_rLink_macro;

   // A failure is fatal
   Null = r2;
   if Z call $error;

   // get input structure
   r8 = M[$spdif.stream_decode.stream_decode_struct];
   r7 = M[r8 + $spdif.stream_decode.SPDIF_FRAME_COPY_STRUCT_FIELD];

   // decrement pending responses counter
   call $spdif.frame_copy.decrement_pending_confirms;

   // let state machine knows if all pending responses received
   Null =  M[r7 + $spdif.frame_copy.FW_CONFIRMS_PENDING_FIELD];
   if Z call $spdif.frame_copy.all_confirms_received;

   // pop rLink from stack
   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $spdif.output_interface_control.all_confirms_received
//
// DESCRIPTION:
//   Does the proper operation when confirmations to DSP messages are received
//   from fw.
//
// INPUTS:
//   None
//
// OUTPUTS:
//    None
//
// TRASHED REGISTERS:
//    assume everything
// *****************************************************************************
.MODULE $M.spdif.frame_copy.all_confirms_received;
   .CODESEGMENT SPDIF_OUTPUT_INTERFACE_CONTROL_ALL_CONFIRMS_RECEIVED_PM;
   .DATASEGMENT DM;

   $spdif.frame_copy.all_confirms_received:

   // push rLink onto stackb
   $push_rLink_macro;

   // get the structures
   r8 = M[$spdif.stream_decode.stream_decode_struct];
   r7 = M[r8 + $spdif.stream_decode.SPDIF_FRAME_COPY_STRUCT_FIELD];

   // see if input interface is being de-activated
   r1 = M[r7 + $spdif.frame_copy.STREAM_INVALID_FIELD];
   r0 = r1 AND $spdif.STREAM_INVALID_DEACTIVATING_INTERFACE;
   if NZ jump if_deactivation_done;

   // see if input interface is being activated
   r0 = r1 AND $spdif.STREAM_INVALID_ACTIVATING_INTERFACE;
   if Z jump if_check_done;

   // -----------------------------------
   // activation process has completed
   // No further action needed
   // ----------------------------------
   if_activation_done:

   r1 = r1 AND ~$spdif.STREAM_INVALID_ACTIVATING_INTERFACE;
   M[r7 + $spdif.frame_copy.STREAM_INVALID_FIELD] = r1;
   // pop rLink from stack
   jump $pop_rLink_and_rts;

   // -----------------------------------
   // De-activation process has completed
   // Now we can re-activate the process
   // ----------------------------------
   if_deactivation_done:

   r4 = 1;
   call $spdif.frame_copy.activate_interface;

   // pop rLink from stack
   jump $pop_rLink_and_rts;


   if_check_done:
   // -----------------------------------
   // No action needed for input interface
   // we are here because some attention
   // needed for output interface
   // ----------------------------------
#ifdef SPDIF_OUTPUT_INTERFACE_CONTROL
   .VAR confirm_table[] = no_action,
                          output_interface_rate_done,
                          output_interface_activation_done,
                          no_action,
                          output_interface_fading_done,
                          output_interface_deactivation_done,
                          output_interface_mute_done,
                          output_interface_unmute_done;

   r1 = M[r8 + $spdif.stream_decode.OUTPUT_INTERFACE_PROCESSING_STATE_FIELD];
   r4 = M[r8 + $spdif.stream_decode.OUTPUT_INTERFACE_STATE_FIELD];
   r1 = M[confirm_table + r1];
   jump r1;

   // IDLE and FULL_ACTIVE states
   no_action:
   jump $pop_rLink_and_rts;

   // WAIT_OUTPUT_INTERFACE_RATE
   output_interface_rate_done:
      r0 = M[r8 + $spdif.stream_decode.OUTPUT_INTERFACE_CONFIG_RATE_FIELD];
      M[r8 + $spdif.stream_decode.OUTPUT_INTERFACE_CURRENT_RATE_FIELD] = r0;
   jump end_confirm;

   // WAIT_OUTPUT_INTERFACE_ACTIVATE
   output_interface_activation_done:
      r4 = r4 OR $spdif.OUTPUT_INTERFACE_STATE_ACTIVE;
      r4 = r4 AND ~($spdif.OUTPUT_INTERFACE_STATE_FADING_DONE);
   jump end_confirm;

   // OUTPUT_INTERFACE_FADING_OUT
   output_interface_fading_done:
      r4 = r4 OR $spdif.OUTPUT_INTERFACE_STATE_FADING_DONE;
      r4 = r4 AND (~$spdif.OUTPUT_INTERFACE_STATE_FADING);
   jump end_confirm;

   // WAIT_OUTPUT_INTERFACE_DEACTIVATE
   output_interface_deactivation_done:
      r4 = r4 AND (~$spdif.OUTPUT_INTERFACE_STATE_ACTIVE);
   jump end_confirm;

   // WAIT_OUTPUT_INTERFACE_MUTE
   output_interface_mute_done:
      r4 = r4 AND (~$spdif.OUTPUT_INTERFACE_STATE_UNMUTE);
   jump end_confirm;

   // WAIT_OUTPUT_INTERFACE_UNMUTE
   output_interface_unmute_done:
      r4 = r4 OR ($spdif.OUTPUT_INTERFACE_STATE_UNMUTE);
   jump end_confirm;

   end_confirm:
   // set the latest state of output interface and call the event handler
   M[r8 + $spdif.stream_decode.OUTPUT_INTERFACE_STATE_FIELD] = r4;

   // all confirms received and updates done, now run event handler
   call $spdif.output_interface_control.event_process;

#endif // #ifdef SPDIF_OUTPUT_INTERFACE_CONTROL

   // pop rLink from stack
   jump $pop_rLink_and_rts;

   // -----------------------------------
   // Utility function to increment
   // pending responses
   // ----------------------------------
   $spdif.frame_copy.increment_pending_confirms:
   r7 = M[r8 + $spdif.stream_decode.SPDIF_FRAME_COPY_STRUCT_FIELD];
   r0 = M[r7 + $spdif.frame_copy.FW_CONFIRMS_PENDING_FIELD];
   r0 = r0 + 1;
   M[r7 + $spdif.frame_copy.FW_CONFIRMS_PENDING_FIELD]= r0;
   rts;
   // -----------------------------------
   // Utility function to decrement
   // pending responses
   // ----------------------------------
   $spdif.frame_copy.decrement_pending_confirms:
   r7 = M[r8 + $spdif.stream_decode.SPDIF_FRAME_COPY_STRUCT_FIELD];
   r0 = M[r7 + $spdif.frame_copy.FW_CONFIRMS_PENDING_FIELD];
   r0 = r0 - 1;
   M[r7 + $spdif.frame_copy.FW_CONFIRMS_PENDING_FIELD]= r0;
   rts;

// *****************************************************************************
// MODULE:
//    $spdif.frame_copy.activate_interface
//
// DESCRIPTION:
//  activate/de-activate input interface
//
// INPUTS:
//   r4 = 0/1 activate/de-activate
//
// OUTPUTS:
//    None
//
// TRASHED REGISTERS:
//    assume everything except r8/r7
// *****************************************************************************
   $spdif.frame_copy.activate_interface:
   // r4 = 0/1 activate/de-activate
   // push rLink onto stackb
   $push_rLink_macro;

   pushm <r7,r8>;

   // get the structures
   r8 = M[$spdif.stream_decode.stream_decode_struct];
   r7 = M[r8 + $spdif.stream_decode.SPDIF_FRAME_COPY_STRUCT_FIELD];

   // only do this if no confirm is pending from fw
   Null =  M[r7 + $spdif.frame_copy.FW_CONFIRMS_PENDING_FIELD];
   if NZ jump activation_end;

   // restarting interface can be done once only
   M[r7 + $spdif.frame_copy.CAN_RESTART_INTERFACE_FIELD] = Null;

   // update stream status flags
   r1 = M[r7 + $spdif.frame_copy.STREAM_INVALID_FIELD];
   r1 = r1 AND ~($spdif.STREAM_INVALID_ACTIVATING_INTERFACE|$spdif.STREAM_INVALID_DEACTIVATING_INTERFACE);
   r0 = $spdif.STREAM_INVALID_ACTIVATING_INTERFACE;
   r2 = $spdif.STREAM_INVALID_DEACTIVATING_INTERFACE;
   Null = r4;
   if Z r0 = r2;
   r1 = r1 OR r0;
   M[r7 + $spdif.frame_copy.STREAM_INVALID_FIELD] = r1;

   // activate LEFT port
   r2 = $MESSAGE_ACTIVATE_AUDIO;
   r3 = M[r7 + $spdif.frame_copy.LEFT_INPUT_PORT_FIELD];
   r3 = r3 AND $cbuffer.TOTAL_PORT_NUMBER_MASK;
   r5 = 0;
   r6 = 0;
   push r4;
   call $message.send_short;
   pop r4;

   // increment pending confirms
   call $spdif.frame_copy.increment_pending_confirms;

   // activate RIGHT port
   r2 = $MESSAGE_ACTIVATE_AUDIO;
   r3 = M[r7 + $spdif.frame_copy.RIGHT_INPUT_PORT_FIELD];
   r3 = r3 AND $cbuffer.TOTAL_PORT_NUMBER_MASK;
   r5 = 0;
   r6 = 0;
   call $message.send_short;

   // increment pending confirms
   call $spdif.frame_copy.increment_pending_confirms;

   activation_end:
   popm <r7, r8>;
   // pop rLink from stack
   jump $pop_rLink_and_rts;

.ENDMODULE;
#endif // #ifndef SPDIF_FIRMARE_INTERFACE_ASM_INCLUDED
