// *****************************************************************************
// Copyright (c) 2009 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
//
// *****************************************************************************

// *****************************************************************************
// DESCRIPTION
// Handling incoming tones and voice prompts from firmware
//
//
// *****************************************************************************
#if defined(APTX_ACL_SPRINT_ENABLE)
   #include "codec_decoder_aptx_acl_sprint.h"
#elseif defined (FASTSTREAM_ENABLE)
   #include "codec_decoder_faststream.h"
#else
   #include "codec_decoder.h"
#endif
   #include "frame_sync_stream_macros.h"
   #include "cbops_library.h"

.MODULE $codec_tones;
   .DATASEGMENT DM;
   .CODESEGMENT PM;

   DeclareCBuffer($tone_in_left_cbuffer_struc,$tone_in_left,TONE_BUFFER_SIZE);
   DeclareCBuffer($tone_in_right_cbuffer_struc,$tone_in_right,TONE_BUFFER_SIZE);
   DeclareCBuffer($tone_in_left_resample_cbuffer_struc,$tone_in_left_resample,(TONE_BUFFER_SIZE * 6));
   DeclareCBuffer($tone_in_right_resample_cbuffer_struc,$tone_in_right_resample,(TONE_BUFFER_SIZE * 6));

   // Duplicate tone cbuffers (use the original tone data buffers)
   DeclareCBufferNoMem($tone0_in_left_resample_cbuffer_struc,$tone_in_left_resample);
   DeclareCBufferNoMem($tone0_in_right_resample_cbuffer_struc,$tone_in_right_resample);
   DeclareCBufferNoMem($tone1_in_left_resample_cbuffer_struc,$tone_in_left_resample);
   DeclareCBufferNoMem($tone1_in_right_resample_cbuffer_struc,$tone_in_right_resample);
   DeclareCBufferNoMem($tone2_in_left_resample_cbuffer_struc,$tone_in_left_resample);
   DeclareCBufferNoMem($tone2_in_right_resample_cbuffer_struc,$tone_in_right_resample);

   .VAR $tone_copy_timer_struc[$timer.STRUC_SIZE];

   .VAR $set_tone_rate_from_vm_message_struc[$message.STRUC_SIZE];   // Message structure for VM_SET_TONE_RATE_MESSAGE_ID message
   .VAR $current_tone_sampling_rate = 8000;                          // input tone/prompt sample rate, set by VM before tone starts
   .VAR $stereo_tone;                                                // flag showing input tone/prompt is stereo

   // Timer period variable for reading tone/prompt data
   // (this is modified according to the connection type and also to support local playback)
   .VAR $tmr_period_tone_copy = TMR_PERIOD_TONE_COPY;

   // ** allocate memory for tone input cbops copy routine **
   .VAR $mono_tone_in_copy_struc[] =
      &$tone_in_copy_op,               // first operator block
      1,                               // number of inputs
      $TONE_IN_PORT,                   // input
      1,                               // number of outputs
      &$tone_in_left_cbuffer_struc;    // output

   .BLOCK $tone_in_copy_op;
      .VAR $tone_in_copy_op.next = $cbops.NO_MORE_OPERATORS;
      .VAR $tone_in_copy_op.func = &$cbops.shift;
      .VAR $tone_in_copy_op.param[$cbops.shift.STRUC_SIZE] =
         0,                            // Input index
         1,                            // Output index
         3+8;                          // 3 bits amplification for tone(was in original file)
                                       // and 8 bits to convert from 16-bit to 24-bit
   .ENDBLOCK;

   // Allocate memory for tone input cbops resampler routine **
   .VAR $mono_tone_in_resample_copy_struc[] =
      $mono_tone_in_left_resample_op,        // first operator block
      1,                                     // number of inputs
      $tone_in_left_cbuffer_struc,           // input
      2,                                     // number of outputs
      $tone_in_left_resample_cbuffer_struc,  // output 0
      $tone_in_right_resample_cbuffer_struc; // output 1

   // History buffers for the tone resampler filters
   .VAR/DM1CIRC tone_left_resample_hist[$cbops.fir_resample.HIST_LENGTH];                                                               \
   .VAR/DM1CIRC tone_right_resample_hist[$cbops.fir_resample.HIST_LENGTH];

   .BLOCK $mono_tone_in_left_resample_op;
      .VAR $mono_tone_in_left_resample_op.next = $mono_tone_in_right_resample_op;
      .VAR $mono_tone_in_left_resample_op.func = $cbops.fir_resample;
      .VAR $mono_tone_in_left_resample_op.param[$cbops.fir_resample.STRUC_SIZE] =
         0,                                                                // INPUT_INDEX_FIELD, index of the input buffer
         1,                                                                // OUTPUT_INDEX_FIELD, index of the output buffer
         $sra_coeffs,                                                      // COEF_BUF_INDEX_FIELD, coefficients
         $current_tone_sampling_rate,                                      // INPUT_RATE_ADDR_FIELD, address holding input rate
         $current_dac_sampling_rate,                                       // OUTPUT_RATE_ADDR_FIELD, address holding output rate
         tone_left_resample_hist,                                          // HIST_BUF_FIELD, history buffer
         0 ...;
   .ENDBLOCK;

   .BLOCK $mono_tone_in_right_resample_op;
      .VAR $mono_tone_in_right_resample_op.next = $tone_in_resample_complete_op;
      .VAR $mono_tone_in_right_resample_op.func = $cbops.fir_resample;
      .VAR $mono_tone_in_right_resample_op.param[$cbops.fir_resample.STRUC_SIZE] =
         0,                                                                // INPUT_INDEX_FIELD, index of the input buffer
         2,                                                                // OUTPUT_INDEX_FIELD, index of the output buffer
         $sra_coeffs,                                                      // COEF_BUF_INDEX_FIELD, coefficients
         $current_tone_sampling_rate,                                      // INPUT_RATE_ADDR_FIELD, address holding input rate
         $current_dac_sampling_rate,                                       // OUTPUT_RATE_ADDR_FIELD, address holding output rate
         tone_right_resample_hist,                                         // HIST_BUF_FIELD, history buffer
         0 ...;
   .ENDBLOCK;

   // ** allocate memory for stereo tone input cbops copy routine **
   // This replaces  $mono_tone_in_copy_struc when the input tone is stereo
   .VAR $stereo_tone_in_copy_struc[] =
      $stereo_tone_in_copy_op,         // first operator block
      1,                               // number of inputs
      $TONE_IN_PORT,                   // input
      2,                               // number of outputs
      $tone_in_left_cbuffer_struc,     // output 0
      $tone_in_right_cbuffer_struc;    // output 1

   .BLOCK $stereo_tone_in_copy_op;
      .VAR $stereo_tone_in_copy_op.next = $cbops.NO_MORE_OPERATORS;
      .VAR $stereo_tone_in_copy_op.func = &$cbops.deinterleave;
      .VAR $stereo_tone_in_copy_op.param[$cbops.deinterleave.STRUC_SIZE] =
         0,                            // Input index
         1,                            // Output 0 index
         2,                            // output 1 index
         8;                            // and 8 bits to convert from 16-bit to 24-bit
   .ENDBLOCK;

   // Allocate memory for stereo tone input cbops resampler routine **
   .VAR $stereo_tone_in_resample_copy_struc[] =
      $tone_in_left_resample_op,             // first operator block
      2,                                     // number of inputs
      $tone_in_left_cbuffer_struc,           // input 0
      $tone_in_right_cbuffer_struc,          // input 1
      2,                                     // number of outputs
      $tone_in_left_resample_cbuffer_struc,  // output 0
      $tone_in_right_resample_cbuffer_struc; // output 1

   .BLOCK $tone_in_left_resample_op;
      .VAR $tone_in_left_resample_op.next = $tone_in_right_resample_op;
      .VAR $tone_in_left_resample_op.func = $cbops.fir_resample;
      .VAR $tone_in_left_resample_op.param[$cbops.fir_resample.STRUC_SIZE] =
         0,                                                                // INPUT_INDEX_FIELD, index of the input buffer
         2,                                                                // OUTPUT_INDEX_FIELD, index of the output buffer
         $sra_coeffs,                                                      // COEF_BUF_INDEX_FIELD, coefficients
         $current_tone_sampling_rate,                                      // INPUT_RATE_ADDR_FIELD, address holding input rate
         $current_dac_sampling_rate,                                       // OUTPUT_RATE_ADDR_FIELD, address holding output rate
         tone_left_resample_hist,                                          // HIST_BUF_FIELD, history buffer
         0 ...;
   .ENDBLOCK;

   .BLOCK $tone_in_right_resample_op;
      .VAR $tone_in_right_resample_op.next = $tone_in_resample_complete_op;
      .VAR $tone_in_right_resample_op.func = $cbops.fir_resample;
      .VAR $tone_in_right_resample_op.param[$cbops.fir_resample.STRUC_SIZE] =
         1,                                                                // INPUT_INDEX_FIELD, index of the input buffer
         3,                                                                // OUTPUT_INDEX_FIELD, index of the output buffer
         $sra_coeffs,                                                      // COEF_BUF_INDEX_FIELD, coefficients
         $current_tone_sampling_rate,                                      // INPUT_RATE_ADDR_FIELD, address holding input rate
         $current_dac_sampling_rate,                                       // OUTPUT_RATE_ADDR_FIELD, address holding output rate
         tone_right_resample_hist,                                         // HIST_BUF_FIELD, history buffer
         0 ...;
   .ENDBLOCK;

   .BLOCK $tone_in_resample_complete_op;
      .VAR $tone_in_resample_complete_op.next = $cbops.NO_MORE_OPERATORS;
      .VAR $tone_in_resample_complete_op.func = $cbops_fir_resample_complete;
   .ENDBLOCK;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $detect_end_of_aux_stream
//
// DESCRIPTION:
//    detects end of pcm tone/prompts and notifies vm
// *****************************************************************************

.MODULE $M.detect_end_of_aux_stream;
   .CODESEGMENT DETECT_END_OF_AUX_STREAM_PM;
   .DATASEGMENT DM;
   .VAR $no_tone_timer;

   $detect_end_of_aux_stream:

   $push_rLink_macro;

   // detect end of tone/prompt auxiliary pcm input stream
   r3 = M[$aux_input_stream_available];
   if Z jump $pop_rLink_and_rts;

   // see if the input is active
   r0 = $tone_in_left_resample_cbuffer_struc;
   call $cbuffer.calc_amount_data;

   // check if input activity has been seen before
   Null = r3 AND 0x2;
   if NZ jump input_has_received;

   // input hasn't started yet, so no end check
   Null = r0;
   if Z jump $pop_rLink_and_rts;

   // input just received
   r3 = r3 OR 0x2;
   M[$aux_input_stream_available] = r3;
   M[$no_tone_timer] = 0;
   jump $pop_rLink_and_rts;

   input_has_received:
   // see if input has become inactive
   r1 = M[$no_tone_timer];
   r1 = r1 + M[$tmr_period_tone_copy];
   Null = r0;
   if NZ r1 = 0;
   M[$no_tone_timer] = r1;
   // has it been inactive in past predefined time?
   Null = r1 - $PCM_END_DETECTION_TIME_OUT;
   if NEG jump $pop_rLink_and_rts;

   // inactive more than a threshold
   // notify VM about end of play back of aux input
   r2 = PLAY_BACK_FINISHED_MSG;
   r3 = 0;
   r4 = 0;
   r5 = 0;
   r6 = 0;
   call $message.send_short;
   M[$aux_input_stream_available] = 0;
   M[$no_tone_timer] = 0;
   // pop rLink from stack
   jump $pop_rLink_and_rts;
.ENDMODULE;
// *****************************************************************************
// MODULE:
//    $tone_copy_handler
//
// DESCRIPTION:
//    Function called on a timer interrupt to perform the copying of tone
//    samples.
//
// *****************************************************************************

.MODULE $M.tone_copy_handler;
   .CODESEGMENT TONE_COPY_HANDLER_PM;
   .DATASEGMENT DM;

   $tone_copy_handler:

   // push rLink onto stack
   $push_rLink_macro;

   // copy tone data from the port and resample if required
   Null = M[$stereo_tone];
   if Z jump mono_tone;

      // Copy the multiplexed stereo input tone from the port and demultiplex
      r8 = $stereo_tone_in_copy_struc;
      call $cbops.copy;

      // Resample the input tone/prompt if required
      r8 = $stereo_tone_in_resample_copy_struc;
      call $cbops.copy;

      jump resample_done;

   mono_tone:

      // Copy the mono input tone from the port and duplicate left channel for right channel
      r8 = $mono_tone_in_copy_struc;
      call $cbops.copy;

      // Resample the input tone/prompt if required
      r8 = $mono_tone_in_resample_copy_struc;
      call $cbops.copy;

   resample_done:

   // detect end of tone/prompt stream
   call $detect_end_of_aux_stream;

   // post another timer event
   r1 = &$tone_copy_timer_struc;
   r2 = M[$tmr_period_tone_copy];
   r3 = &$tone_copy_handler;
   call $timer.schedule_event_in_period;

   // pop rLink from stack
   jump $pop_rLink_and_rts;

// *****************************************************************************
// MODULE:
//    $tone_copy_extra
//
// DESCRIPTION:
//  extra copy from tone port
//
// *****************************************************************************
   $tone_copy_extra:
   // push rLink onto stack
   $push_rLink_macro;

   // Determine if tones are active for the multi-channel mode (output r0=0: inactive, 1: active)
   call $multi_chan_tones_active;

   Null = r0;
   if Z jump $pop_rLink_and_rts;;

   // see if level is already good
   r0 = $tone_in_left_cbuffer_struc;
   call $cbuffer.calc_amount_data;
   r2 = M[$current_tone_sampling_rate];
   r1 = M[$tmr_period_audio_copy];
   rMAC = r2 * r1;
   r2 = rMAC ASHIFT 13;
   r2 = r2 * (0.001536) (frac);
   Null = r0 - r2;
   if POS jump $pop_rLink_and_rts;

   // copy tone data from the port
   r8 = $mono_tone_in_copy_struc;
   r7 = $stereo_tone_in_copy_struc;
   Null = M[$stereo_tone];
   if NZ r8 = r7;
   call $cbops.copy;

   // pop rLink from stack
   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $set_tone_rate_from_vm
//
// DESCRIPTION: message handler for receiving tone rate from VM
//
// INPUTS:
//  r1 = (tone rate)
//  r2 = bit 0: mono / stereo
//       bit 1: tone / prompt
//  r3 = Not used
//  r4 = Not used
//
// OUTPUTS:
//    None
// TRASHED RGISTERS:
//    rMAC, Div, r1
// *****************************************************************************
.MODULE $M.set_tone_rate_from_vm;
   .CODESEGMENT SET_TONE_RATE_FROM_VM_PM;

   $set_tone_rate_from_vm:

   // Push rLink onto stack
   $push_rLink_macro;

   // extract tone rate
   r1 = r1 AND 0xFFFF;

   // timer period = TMR_PERIOD_TONE_COPY/(freq/8000.0)
   rMAC = (TMR_PERIOD_TONE_COPY/4);
   rMAC = rMAC * 8000;
   Div = rMAC / r1;

   // Store the tone sampling rate
   M[$current_tone_sampling_rate] = r1;

   // prompts are normalised
   // tones need 3 bits amplification
   r0 = 3;
   Null = r2 AND 2;
   if NZ r0 = 0;
   r0 = r0 + 8;
   M[$tone_in_copy_op.param + $cbops.shift.SHIFT_AMOUNT_FIELD] = r0;
   M[$stereo_tone_in_copy_op.param + $cbops.deinterleave.SHIFT_AMOUNT_FIELD] = r0;

   // Store the tone timer period
   r3 = DivResult;
   // extract stereo flag
   r0 = r2 AND 1;
   // timer period is halved for stereo prompts
   if Z r3 = r3 + r3;
   M[$tmr_period_tone_copy] = r3;
   M[$stereo_tone] = r0;

   // Configure the CHAIN0 and chain1 tone mixing
   call $multi_chan_config_tone_mixing;

   // purge tone buffers, safeguard to make sure left and
   // right channels are synchronised
   r0 = M[&$tone_in_left_cbuffer_struc + $cbuffer.WRITE_ADDR_FIELD];
   M[&$tone_in_left_cbuffer_struc + $cbuffer.READ_ADDR_FIELD] = r0;
   r0 = M[&$tone_in_right_cbuffer_struc + $cbuffer.WRITE_ADDR_FIELD];
   M[&$tone_in_right_cbuffer_struc + $cbuffer.READ_ADDR_FIELD] = r0;

   // auxiliary input expected now
   r0 = 1;
   M[$aux_input_stream_available] = r0;

   // Pop rLink from stack
   jump $pop_rLink_and_rts;
.ENDMODULE;
