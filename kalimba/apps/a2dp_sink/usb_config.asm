// *****************************************************************************
// Copyright (c) 2009 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
//
// *****************************************************************************

// *****************************************************************************
// DESCRIPTION
//    Handle stereo USB input configuration, play/pause detection.
//    USB input data is copied from the DSP MMU buffers using the frame_sync
//    usbio drivers
//
// *****************************************************************************

#ifdef USB_ENABLE

#include "stack.h"
#include "usbio.h"
#include "codec_decoder.h"

.MODULE $M.usb;
   .DATASEGMENT DM;

   .VAR $usb_stats[$music_example.CODEC_STATS_SIZE] =
      &$music_example.SamplingRate,                            //
      &$M.system_config.data.ZeroValue,                        //
      &$M.system_config.data.ZeroValue,                        // CODEC_STAT1
      &$M.system_config.data.ZeroValue,                        // CODEC_STAT2
      &$M.system_config.data.ZeroValue,                        // CODEC_STAT3
      &$M.system_config.data.ZeroValue,                        // CODEC_STAT4
      &$M.system_config.data.ZeroValue,                        // CODEC_STAT5
      &$M.system_config.data.ZeroValue,                        // CODEC_STAT6
      &$M.system_config.data.ZeroValue,                        // CODEC_STAT7
      &$M.system_config.data.ZeroValue,                        // CODEC_STAT8
      &$M.system_config.data.ZeroValue,                        // CODEC_STAT9
      &$M.system_config.data.ZeroValue,                        // CODEC_STATA
      &$M.system_config.data.ZeroValue;                        // CODEC_STATB


   // USB rate control table: Sample rate, large packet size (bytes), packet modulo period
   .VAR $usb_rate_candidates[] =
      48000, 192, 0,
      8000, 32, 0,
      16000, 64, 0,
      22050, 92, 20,
      32000, 128, 0,
      44100, 180, 10,
      0;

   // ** allocate memory for USB input (non-cbops) copy routine **
   .VAR/DM1 $usb_audio_in_copy_struc[$frame_sync.USB_IN_STEREO_COPY_STRUC_SIZE] =
      $CON_IN_PORT,                       // USB source buffer
      &$audio_out_left_cbuffer_struc,     // Left sink buffer
      &$audio_out_right_cbuffer_struc,    // Right sink buffer
      $USB_PACKET_LEN,                    // "large" packet length (Number of audio data bytes in a "large" USB packet for all channels)
      8,                                  // Shift amount
      0 ...;                              // Sync byte counter

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.config_usb
//
// DESCRIPTION:
//    Routine configures the USB operator structure for the given rate
//
// INPUTS:
//    r8 = pointer to USB operator structure
//
// OUTPUTS:
//    none
//
// TRASHES: r0, r1, I0, M0
//
// *****************************************************************************
.MODULE $M.config_usb;
   .CODESEGMENT PM;

   $config_usb:

   r0 = $usb_rate_candidates;       // Pointer to the beginning of the candidate table
   I0 = r0;

   M0 = 2;

   r0 = M[$current_codec_sampling_rate];
   rate_loop:

      r1 = M[I0,1];                    // Get the candidate USB rate and point at packet length
      null = r1;
      if Z jump error;
      null = r1 - r0;
      if Z jump done;

      r1 = M[I0,M0];                     // Step to next candidate USB rate (i.e. next table record)

   jump rate_loop;

   error:
   I0 = $usb_rate_candidates + 1;

   done:

   // Set the candidate packet length (in bytes) for a "large" packet
   r0 = M[I0,1];
   M[r8 + $frame_sync.USB_IN_STEREO_COPY_PACKET_LENGTH_FIELD] = r0;

   // Set the candidate packet sequence period (in packets)
   r0 = M[I0,1];
   M[r8 + $frame_sync.USB_IN_STEREO_PACKET_COUNT_MODULO_FIELD] = r0;

   rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.usb_state_mc
//
// DESCRIPTION:
//    State machine to coordinate starting and stopping USB audio
//
//    This detects a pause and clears the audio buffers to prevent
//    old data playout on resume. It also detects a resume and inserts silence
//    to prevent the audio buffers emptying (upsetting rate matching and giving
//    audio artifacts).
//
// INPUTS:
//    r7 = USB input handler function pointer
//    r8 = USB audio in copy struc pointer
//
// OUTPUTS:
//    none
//
// TRASHES: Assume all
//
// *****************************************************************************
.MODULE $M.usb_state_mc;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   // Enumerations for states
   .CONST PAUSED                                0;
   .CONST WAITING_TO_START                      1;
   .CONST RUNNING                               2;

   .CONST SILENCE_INSERTION_TIME                0.006;         // Silence insertion time in seconds (default 6 msec)
   .CONST PAUSE_THRESH                          3;             // Threshold for pause detection (default 3 timer periods)

   // Function table corresponding to states
   .VAR $usb_fn_table[] = paused_fn, waiting_to_start_fn, running_fn;

   .VAR $usb_state = PAUSED;                                   // Current state

   .VAR $usb_pause_period_count;                                     // Period counter for USB pause detection
   .VAR $usb_pause_period_thresh = PAUSE_THRESH;                     // Threshold in periods for determining a pause
   .VAR $usb_pause_silence_insertion_time = SILENCE_INSERTION_TIME;  // Amount of silence to insert (in fractional time)
   .VAR $usb_reset_pending = 1;                                      // Flag to indicate a buffer reset is pending
   .VAR $debug_usb_pause_count;                                      // Count the number of times through the statemachine

   // Call this routine to process USB port data
   $run_usb_state_mc:

      // Push rLink onto stack
      $push_rLink_macro;

      // Get amount of data (in bytes) in input port
      r0 = $CON_IN_PORT;
      call $cbuffer.calc_amount_data;

      // Get USB state and execute function according to state
      r3 = M[$usb_state];
      r1 = M[$usb_fn_table + r3];

      // Inputs: r0=data in USB input port, r1=function pointer, r3=current state
      // Outputs r3=next state
      call r1;                                     // Call the state function

      // Next state
      M[$usb_state] = r3;

       // Pop rLink from stack
      jump $pop_rLink_and_rts;

   // State handling functions
   // ------------------------

   // Purge USB buffers (then wait to start in next state)
   paused_fn:

      // push rLink onto stack
      $push_rLink_macro;

      // Count the number of times through the statemachine (for debug)
      r0 = M[$debug_usb_pause_count];
      r0 = r0 + 1;
      M[$debug_usb_pause_count] = r0;

      // Empty the USB port
      r0 = $CON_IN_PORT;
      call $cbuffer.empty_buffer;

      // Request a purge of the audio buffers
      r0 = 1;
      M[$usb_reset_pending] = r0;                  // Set the purge request flag

      // Default next state
      r3 = WAITING_TO_START;

      // buffer USB input to overcome jitter and prevent occasional zero insertions, which sound like
      // pops and clicks.
      r0 = 1;
      M[$M.jitter_buffering.is_buffering] = r0;

      // pop rLink from stack
      jump $pop_rLink_and_rts;

   // Wait for some USB data
   waiting_to_start_fn:

      // Push rLink onto stack
      $push_rLink_macro;

      null = r0;                                   // USB data available?
      if Z jump still_waiting_to_start;            // No - skip and stay in state

         // Have the buffers been cleared/reset?
         null = M[$usb_reset_pending];             // Buffers cleared/reset?
         if Z jump buffer_reset_done;              // Yes - start decoding USB

            // Empty the USB port
            push r3;
            r0 = $CON_IN_PORT;
            call $cbuffer.empty_buffer;
            pop r3;
            jump still_waiting_to_start;           // No - skip and stay in state

         buffer_reset_done:

         // Reset the start period counter
         M[$usb_pause_period_count] = 0;

         // Number of samples of silence to insert
         r0 = M[$current_codec_sampling_rate];     // Get the sampling rate
         r1 = M[$usb_pause_silence_insertion_time];// Time to insert
         r5 = r0 * r1 (frac);                      // Number of samples @sampling rate to insert

         // Insert silence data into the empty audio buffers (r4=cbuffer pointer, r5=number of samples)
         r4 = $audio_out_left_cbuffer_struc;
         call $cbuffer_insert_silence;
         r4 = $audio_out_right_cbuffer_struc;
         call $cbuffer_insert_silence;

         // Decode USB data from the USB port and copy to the audio buffers
         // inputs: r7=USB input handler function ptr, r8=USB autio in copy struc
         call r7;

         r3 = RUNNING;                             // Next state

      still_waiting_to_start:

      // Pop rLink from stack
      jump $pop_rLink_and_rts;

   // State that allows USB to run until $usb_pause_period_thresh consecutive empty periods are detected
   running_fn:

      // Push rLink onto stack
      $push_rLink_macro;

      r5 = PAUSED;                                 // Potential next state

      r4 = M[$usb_pause_period_count];             // Get the number of consecutive zero input timer periods
      r4 = r4 + 1;                                 // Count up

      null = r0;                                   // If data is received...
      if NZ r4 = 0;                                // ...zero the count of consecutive of zero periods
      M[$usb_pause_period_count] = r4;             // Save the count

      null = r4 - M[$usb_pause_period_thresh];     // Check if the threshold for a pause is reached
      if POS r3 = r5;                              // Yes, change state to paused

      push r3;
      // Decode USB data from the USB port and copy to the audio buffers
      // inputs: r7=USB input handler function ptr, r8=USB audio in copy struc
      call r7;
      pop r3;

      // Pop rLink from stack
      jump $pop_rLink_and_rts;
.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.cbuffer_insert_silence
//
// DESCRIPTION:
//    Inserts a given number of silence samples into a cbuffer
//
// INPUTS:
//    r4 = cbuffer structure address
//    r5 = number of silence samples to insert
//
// OUTPUTS:
//    none
//
// TRASHES: (for a cbuffer) r0, r1, r2, r3, r10, I0, L0, DoLoop
//
// *****************************************************************************
.MODULE $M.cbuffer_insert_silence;
    .CODESEGMENT PM;

$cbuffer_insert_silence:

   // Push rLink onto stack
   $push_rLink_macro;

   // Get audio cbuffer write address
   r0 = r4;
   call $cbuffer.get_write_address_and_size;
   I0 = r0;
   L0 = r1;

   // Load number of samples to insert
   r10 = r5;

   // Zero words give silence
   r0 = 0;

   // Generate silence
   do audio_fill_loop;
     M[I0,1] = r0;
   audio_fill_loop:

   // Set new write address
   r0 = r4;
   r1 = I0;
   call $cbuffer.set_write_address;
   L0 = 0;

   // Pop rLink from stack
   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.usb_cond_buffer_reset
//
// DESCRIPTION:
//    Conditionally clear the cbuffers used for USB audio according to the
//    state of the $usb_reset_pending flag
//
// INPUTS:
//    none
//
// OUTPUTS:
//    none
//
// TRASHES: assume all but save r0
//
// *****************************************************************************
.MODULE $M.usb_cond_buffer_reset;
    .CODESEGMENT PM;

$usb_cond_buffer_reset:

   // Push rLink onto stack
   $push_rLink_macro;

   push r0;                               // Keep r0

   null = M[$usb_reset_pending];          // Is a reset required?
   if NZ call $master_app_reset;          // Purge the audio buffers (interrupts are blocked for this)
   M[$usb_reset_pending] = 0;             // Clear the reset pending flag?

   pop r0;

   // Pop rLink from stack
   jump $pop_rLink_and_rts;

.ENDMODULE;

#endif
