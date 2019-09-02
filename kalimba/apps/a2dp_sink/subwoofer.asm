// *****************************************************************************
// Copyright (c) 2009 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
//
// *****************************************************************************

// *****************************************************************************
// DESCRIPTION
//   Soundbar link to a Wireless subwoofer
//
// *****************************************************************************
#include "core_library.h"
#include "audio_proc_library.h"
#include "music_example.h"
#include "cbops_library.h"
#include "sr_adjustment.h"
#include "frame_sync_stream_macros.h"
#include "codec_decoder.h"
#include "frame_sync_buffer.h"


.Module $M.Subwoofer;
   .DATASEGMENT DM;
   .CODESEGMENT PM;

#ifdef WIRELESS_SUB_ENABLE

   DeclareCBuffer($sub_codec_out_cbuffer_struc,$sub_codec_out,$L2CAP_PACKET_SIZE_BYTES);
   DeclareCBuffer($sub_out_1k2_cbuffer_struc,$sub_out_1k2,1800/40);

     // buffers and variables for sub link
    .VAR $set_synchronized_warp_message_struc[$timer.STRUC_SIZE];

    .var $sub_link_port = 0;             // use as pointer to sub port used
    .var $new_esco_port_connection = 0;  // signals that the ESCO port has just connected

    .var $sub_esco_out_copy_struc[] =
        &$signal_detect_op,             // First operator block
        1,                               // Number of inputs
        &$sub_codec_out_cbuffer_struc,   // Input
        1,                               // Number of outputs
        $AUDIO_ESCO_SUB_OUT_PORT;        // Output

    .var $sub_l2cap_out_copy_struc[] =
        &$signal_detect_op,             // First operator block
        1,                               // Number of inputs
        &$sub_codec_out_cbuffer_struc,   // Input
        1,                               // Number of outputs
        $AUDIO_L2CAP_SUB_OUT_PORT;        // Output


   .BLOCK $signal_detect_op;
      .VAR $signal_detect_op.next = $codec_out_copy_op;
      .VAR $signal_detect_op.func = &$cbops.signal_detect_op;
      .VAR $signal_detect_op.param[$cbops.signal_detect_op.STRUC_SIZE_MONO] =
         &$M.multi_chan_output.signal_detect_coeffs,           // pointer to coefficients
         1,                               // number of channels to process
         0;                               // channel index
   .ENDBLOCK;


    .block $codec_out_copy_op;
        .var $codec_out_copy_op.next = $cbops.NO_MORE_OPERATORS;
        .var $codec_out_copy_op.func = &$cbops.copy_op;
        .var $codec_out_copy_op.param[$cbops.copy_op.STRUC_SIZE] =
            0,                            // Input index
            1;                            // Output index
    .endblock;

   // us per sample is in q5.19
   .VAR/DM1 $us_per_sample_sbc[4] = 0, 0, 0.708616780045352, 0.651041666666667;

   // buffers that are after Frame Processing
   .VAR $output_buffers[] =
      &$multi_chan_primary_left_out_cbuffer_struc,
      &$us_per_sample_sbc + 2,            // inverse sample_rate Q5.19
      0;

   .CONST $CALC_TIME_TO_PLAY_PRIMARY_LEFT_PORT_FIELD               0;
   .CONST $CALC_TIME_TO_PLAY_PORT_US_PER_SAMPLE_PTR_FIELD          1;
   .CONST $CALC_TIME_TO_PLAY_SUB_ALIGNMENT_DELAY_FIELD             2;
   .CONST $CALC_TIME_TO_PLAY_SUB_ALIGNMENT_US_PER_SAMPLE_FIELD     3;
   .CONST $CALC_TIME_TO_PLAY_TIME_TO_PLAY_FIELD                    4;

      .VAR $output_port_and_delay_size[] =
         &$MULTI_CHAN_PRIMARY_LEFT_OUT_PORT,
         &$us_per_sample_sbc + 2,     // port sample rate
         $L2CAP_SUB_ALIGNMENT_DELAY,
         &$us_per_sample_sbc + 2,     // delay sample rate
         0;                           // TIME_TO_PLAY

   .VAR playback_time = 0;
    //--------------------------------------------------------------------------
    // variables required for ESCO sub
    //--------------------------------------------------------------------------

    .var $bdaddr_message_struc[$message.STRUC_SIZE];
    .var $sco_params_message_struc[$message.STRUC_SIZE];
    .var $wall_clock_message_struc[$message.STRUC_SIZE];
    .var $sco_timer_struc[$timer.STRUC_SIZE];
    .var $wall_clock_timer_struc[$timer.STRUC_SIZE];
    .var $bdaddr_struc[4];
    .var $wall_clock_msw;
    .var $wall_clock_lsw;
    .var $wall_clock_time;
    .var $sco_param_tesco = 0;
    .var $sco_param_wesco = 0;
    .var $sco_param_clock_msw;
    .var $sco_param_clock_lsw;
    .var $sco_param_to_air_size;
    .var $sco_param_from_air_size;
    .var $sco_param_to_air_time;
    .var $sco_param_from_air_time;
    .var $sco_timer_id = 0;
    .var $bass_timer_id = 0;
    .var $zero_rate = 0;
    .VAR $subwoofer_temp = 0;
    .VAR $sub_app_esco_func;  // app-specefic function runs when eSCO timer expires
    .VAR $subwoofer_trim_buffers_flag = 0;
    .VAR recompute_time_to_play_flag = 0;
    .VAR sco_schedule_to_air_flag = 0;

    .CONST $WALL_CLOCK_UPDATE_PERIOD    100000;
    .CONST $SCHEDULING_PADDING             500;

   l2cap_handler:

      $push_rLink_macro;

       // create sub channel data
       call $downsample_sub_to_1k2;
       call $create_sub_packet;

       // Cbops can only be run in both the foreground and background if the foreground is protected
       call $block_interrupts;

       // copy to output port
       r8 = &$sub_l2cap_out_copy_struc;
       call $cbops.copy;

       call $unblock_interrupts;

       // pop rLink from stack
       jump $pop_rLink_and_rts;

    esco_post_process:

        $push_rLink_macro;

        call $downsample_sub_to_1k2;        // downsample to create 9 samples of sub data

        // schedule timer to copy bass to output MMU port
        r2 = 10;  // schedule pretty much immediately

        r1 = M[$sco_param_tesco];
        Null = r1 - 6;
        if NZ jump do_not_send_duplicates;
           r1 = 1;
           M[$queue_another_bass_packet] = r1;
        do_not_send_duplicates:

        r1 = &$bass_timer_struc;
        r3 = m[r1 + $timer.ID_FIELD];
        if NZ jump bass_packet_already_scheduled;
            r3 = &send_bass_packet;
            call $timer.schedule_event_in;
            jump not_scheduled;

        bass_packet_already_scheduled:

not_scheduled:
        M[$bass_timer_id] = r3;

        jump $pop_rLink_and_rts;

// **************************************************************
send_bass_packet:
   .VAR $bass_timer_struc[$timer.STRUC_SIZE];
   .VAR $queue_another_bass_packet;

   $push_rLink_macro;

   // create sco packet for transmission
   call $create_sub_packet;

   // Debug to monitor port space
   // copy to output port
   r0 = M[$sub_link_port];
   call $cbuffer.calc_amount_space;

   .VAR $min_port_space = 1000;
   r4 = M[$min_port_space];
   Null = r0 - r4;
   if NEG r4 = r0;
   M[$min_port_space] = r4;

   r8 = &$sub_esco_out_copy_struc;
   call $cbops.copy;

   r1 = M[$TIMER_TIME];
   r2 = M[$esco_write_deadline];
   r3 = r1 - r2;
   if NEG jump early;
      // late write
      .VAR $late_write_counter;
      r4 = M[$late_write_counter];
      r4 = r4 + 1;
      M[$late_write_counter] = r4;
      .VAR $max_late_write;
      r4 = M[$max_late_write];
      Null = r3 - r4;
      if POS r4 = r3;
      M[$max_late_write] = r4;  // this is how late the write operation was.
      jump done_logging_debug_data;

   early:
   .VAR $early_write_counter;
   r4 = M[$early_write_counter];
   r4 = r4 + 1;
   M[$early_write_counter] = r4;
   .VAR $min_early_write = -5000;
   r4 = M[$min_early_write];
   Null = r3 - r4;
   if POS r4 = r3;
   M[$min_early_write] = r4;  // this is early, close to the dealine (min_early).

   .VAR $max_early_write;
   r4 = M[$max_early_write];
   Null = r3 - r4;
   if NEG r4 = r3;
   M[$max_early_write] = r4;  // this is early, ahead of the dealine (max_early).

done_logging_debug_data:
   r4 = M[$sco_param_tesco];
   r4 = r4 * 625 (int);
   r2 = r2 + r4;
   M[$esco_write_deadline] = r2;

   // queue another write
   Null = M[$queue_another_bass_packet];
   if Z jump done_sending_bass_packets;
   M[$queue_another_bass_packet] = Null;
   r2 = $SUBWOOFER_ESCO_PORT_RATE;
   r1 = &$bass_timer_struc;
   r3 = &send_bass_packet;
   call $timer.schedule_event_in_period;
   M[$bass_timer_id] = r3;

done_sending_bass_packets:
   jump $pop_rLink_and_rts;


esco_port_connected:
   $push_rLink_macro;

   push r1;
   call $M.Subwoofer.subwoofer_init;

   pop r3;
   r2 = $MESSAGE_GET_BT_ADDRESS;
   call $message.send;

   r0 = $AUDIO_ESCO_SUB_OUT_PORT;
   M[$sub_link_port] = r0;

   r0 = 1;
   M[$M.create_sub_packet.packet_counter] = r0;

   // Force $SystemSleepEsco to wake to prevent lock up
   M[$M.Sleep.sync_flag_esco] = Null;

   r0 = $music_example.NUM_SAMPLES_PER_FRAME;
   M[$music_example.frame_processing_size] = r0;

   r0 = &$M.system_config.data.stream_map_sub_out;
   M[$M.system_config.data.spkr_ctrl_struct + $spkr_ctrl_system.SUB_OUTPUT_PTR] = r0;
   r0 = 2;
   M[$M.system_config.data.spkr_ctrl_coefs + $spkr_ctrl_system.COEF_CONFIG] = r0;

   r7 = &$M.system_config.data.spkr_ctrl_bass_buffer;
   r8 = length(&$M.system_config.data.spkr_ctrl_bass_buffer);
   call  $spkr_ctrl_system.zero_data;

   r7 = &$M.system_config.data.spkr_ctrl_init_struct;
   call $spkr_ctrl_system.initialize;

   r0 = $M.MUSIC_MANAGER.CONFIG.IFFUNC_SW;
   M[$music_example.OTA_IF_Connections] = r0;

   r0 = 1;
   M[$subwoofer_trim_buffers_flag] = r0;

   jump $pop_rLink_and_rts;

l2cap_port_connected:
   // start timer that copies codec input data
   $push_rLink_macro;

   push r1;
   call $M.Subwoofer.subwoofer_init;

   pop r3;
   r2 = $MESSAGE_GET_BT_ADDRESS;
   call $message.send;

   r0 = $L2CAP_SUB_ALIGNMENT_DELAY;
   M[$M.system_config.data.delay_pri_left + $audio_proc.delay.DELAY_FIELD] = r0;
   M[$M.system_config.data.delay_pri_right + $audio_proc.delay.DELAY_FIELD] = r0;
   M[$M.system_config.data.delay_sec_left + $audio_proc.delay.DELAY_FIELD] = r0;
   M[$M.system_config.data.delay_sec_right + $audio_proc.delay.DELAY_FIELD] = r0;
   M[$M.system_config.data.delay_aux_left + $audio_proc.delay.DELAY_FIELD] = r0;
   M[$M.system_config.data.delay_aux_right + $audio_proc.delay.DELAY_FIELD] = r0;
   M[$M.main.samples_latency_measure + 0] = r0;

   r8 = &$M.system_config.data.delay_pri_left;
   call $audio_proc.delay.initialize;
   r8 = &$M.system_config.data.delay_pri_right;
   call $audio_proc.delay.initialize;
   r8 = &$M.system_config.data.delay_sec_left;
   call $audio_proc.delay.initialize;
   r8 = &$M.system_config.data.delay_sec_right;
   call $audio_proc.delay.initialize;
   r8 = &$M.system_config.data.delay_aux_left;
   call $audio_proc.delay.initialize;
   r8 = &$M.system_config.data.delay_aux_right;
   call $audio_proc.delay.initialize;

   // For multi-channel: all Music Manager and later processing is at the DAC rate
   r0 = M[$current_dac_sampling_rate];
   call $calc_us_per_sample_addr;
   M[$output_buffers + 1] = r3;                                                                 // For Music Manager output buffer: $multi_chan_primary_left_out_cbuffer_struc
   M[$output_port_and_delay_size + $CALC_TIME_TO_PLAY_SUB_ALIGNMENT_US_PER_SAMPLE_FIELD] = r3;  // For sub alignment delay buffer
   M[$output_port_and_delay_size + $CALC_TIME_TO_PLAY_PORT_US_PER_SAMPLE_PTR_FIELD] = r3;       // For output port

   r0 = $AUDIO_L2CAP_SUB_OUT_PORT;
   M[$sub_link_port] = r0;

   r0 = 1;
   M[$M.create_sub_packet.packet_counter] = r0;

   r0 = $music_example.NUM_SAMPLES_PER_FRAME;
   r0 = r0 ASHIFT 1;
   M[$music_example.frame_processing_size] = r0;

   r0 = &$M.system_config.data.stream_map_sub_out;
   M[$M.system_config.data.spkr_ctrl_struct + $spkr_ctrl_system.SUB_OUTPUT_PTR] = r0;
   r0 = 2;
   M[$M.system_config.data.spkr_ctrl_coefs + $spkr_ctrl_system.COEF_CONFIG] = r0;

   r7 = &$M.system_config.data.spkr_ctrl_bass_buffer;
   r8 = length(&$M.system_config.data.spkr_ctrl_bass_buffer);
   call  $spkr_ctrl_system.zero_data;

   r7 = &$M.system_config.data.spkr_ctrl_init_struct;
   call $spkr_ctrl_system.initialize;

   r0 = $M.MUSIC_MANAGER.CONFIG.IFFUNC_SW;
   M[$music_example.OTA_IF_Connections] = r0;

   jump $pop_rLink_and_rts;

esco_port_disconnected:
l2cap_port_disconnected:
   // start timer that copies codec input data
   $push_rLink_macro;
   call $disable_subwoofer;
   jump $pop_rLink_and_rts;

// *****************************************************************************
trim_buffers:
   $push_rLink_macro;

   r0 = M[$sub_link_port];
   Null = r0 - $AUDIO_ESCO_SUB_OUT_PORT;
   if NZ jump done_trimming;

   Null = M[$subwoofer_trim_buffers_flag];
   if Z jump done_trimming;
   M[$subwoofer_trim_buffers_flag] = Null;

   call $block_interrupts;
   call $multi_chan_purge_buffers;
   call $purge_input_pcm_cbuffers;
   call $unblock_interrupts;

done_trimming:
   jump $pop_rLink_and_rts;

// *****************************************************************************
// Schedule routine always called (but only schedules for wired inputs)
schedule_and_sleep:
   $push_rLink_macro;
   // Only schedule if an esco subwoofer is connected
   r0 = M[$sub_link_port];
   Null = r0 - $AUDIO_ESCO_SUB_OUT_PORT;
   if NZ jump not_esco_sub;

      r0 = M[$M.Subwoofer.sco_schedule_to_air_flag];
      if NZ call $sco_schedule_to_air;

      // Sco timer must be set before sleeping
      r0 = M[$sco_timer_struc + $timer.ID_FIELD];
      if Z jump timer_not_set;

         call $M.Subwoofer.trim_buffers;
         call $SystemSleepEsco;

      timer_not_set:

   not_esco_sub:
   jump $pop_rLink_and_rts;

// *****************************************************************************
set_frame_size:
   M[$M.system_config.data.stream_map_left_in + $framesync_ind.FRAME_SIZE_FIELD] = r3;
   M[$M.system_config.data.stream_map_right_in + $framesync_ind.FRAME_SIZE_FIELD] = r3;
   M[$M.system_config.data.stream_map_primary_left_out + $framesync_ind.FRAME_SIZE_FIELD] = r3;
   M[$M.system_config.data.stream_map_primary_right_out + $framesync_ind.FRAME_SIZE_FIELD] = r3;
   M[$M.system_config.data.stream_map_secondary_left_out + $framesync_ind.FRAME_SIZE_FIELD] = r3;
   M[$M.system_config.data.stream_map_secondary_right_out + $framesync_ind.FRAME_SIZE_FIELD] = r3;
   M[$M.system_config.data.stream_map_aux_left_out + $framesync_ind.FRAME_SIZE_FIELD] = r3;
   M[$M.system_config.data.stream_map_aux_right_out + $framesync_ind.FRAME_SIZE_FIELD] = r3;
   M[$M.system_config.data.stream_map_sub_out + $framesync_ind.FRAME_SIZE_FIELD] = r3;
   rts;

// *****************************************************************************
transmit_wireless_subwoofer_audio:
   $push_rLink_macro;

   r0 = M[$sub_link_port];
   Null = r0 - $AUDIO_ESCO_SUB_OUT_PORT;
   if NZ jump check_l2cap_sub;
   Null = M[$sub_app_esco_func];
   if Z call $M.Subwoofer.esco_post_process;

   check_l2cap_sub:
   // if we have an L2CAP port open for sub channel, downsample output from frame processing sub channel
   r0 = M[$sub_link_port];
   Null = r0 - $AUDIO_L2CAP_SUB_OUT_PORT;
   if NZ jump done_with_transmission;

   // if don't have enough data in 'sub_out' buffer, then keep calm and carry on
   r0 = &$multi_chan_sub_out_cbuffer_struc;
   call $cbuffer.calc_amount_data;
   Null = r0 - 720;
   if neg jump done_with_transmission;

   call $M.Subwoofer.l2cap_handler;

   done_with_transmission:
   jump $pop_rLink_and_rts;

// *****************************************************************************
validate_time_to_play:
   $push_rLink_macro;

   // If the output copy hander runs in between $M.Subwoofer.synchronise and $frame_sync.update_streams_ind
   // the TimeToPlay value won't be correct because av_copy will insert samples into the port.  This condition
   // is detected as zero samples in the output cbuffer and a non-zero TimeToPlay value.
   r3 = M[playback_time];
   if Z jump set_flag;
      r3 = Null;
      // playback_time has a value.  If there are no samples in the output cbuffer, the TimeToPlay needs to be computed.
      r0 = &$multi_chan_primary_left_out_cbuffer_struc;
      call $cbuffer.calc_amount_data;
      Null = r0;
      if NZ jump set_flag;
      r3 = 1;

   set_flag:
   M[recompute_time_to_play_flag] = r3;
   jump $pop_rLink_and_rts;

// *****************************************************************************
compute_time_to_play:
   $push_rLink_macro;

   Null = M[recompute_time_to_play_flag];
   if Z jump done_with_compute_time_to_play;
      call $M.Subwoofer.synchronise;
   done_with_compute_time_to_play:
   jump $pop_rLink_and_rts;

// *****************************************************************************
   subwoofer_init:
      $push_rLink_macro;

      // register message handlers for ESCO Subwoofer
      r1 = $SCHEDULING_PADDING;
      call $enable_esco_subwoofer;

      jump $pop_rLink_and_rts;

// *****************************************************************************
    synchronise:
    // compute playback time and schedule warp
      $push_rLink_macro;
      r0 = M[$sub_link_port];
      Null = r0 - $AUDIO_L2CAP_SUB_OUT_PORT;
      if NZ jump done_synchronise;
         r7 = &$output_buffers;
         r8 = &$output_port_and_delay_size;
         call $calc_time_to_play;
         M[playback_time] = r6;

      done_synchronise:
      jump $pop_rLink_and_rts;

#endif // WIRELESS_SUB_ENABLE

.ENDMODULE;
//------------------------------------------------------------------------------
.module $M.downsample_sub_to_1k2;
//------------------------------------------------------------------------------
// downsample sub signal to 1.2 kHz and apply mute
// - downsampling from 48 kHz to 1.2 kHz is done by picking 1 sample in 40
// - downsampling from 44.1 kHz to 1.2 kHz is done by picking closest sample in
//   36.75 input samples
// - mute is an 8 sample sin^2 curve applied to 1.2 kHz sub data using the same
//   technique as the cbops mute operator, but with an 8 sample (6.67 ms) curve
//------------------------------------------------------------------------------

    .datasegment dm;

    .var mute_direction = 1;

#ifndef WIRELESS_SUB_ENABLE

    // if sub not enabled, close off module and don't include any more sub code.
    //   mute_direction is always included because it is used in vm_message.asm
    //   which is common to all codecs.

    .endmodule;

#else // WIRELESS_SUB_ENABLE

    .var residual = 0;

    .var mute_index = 0;
    .var sub_mute_coefs[] = 0x000000, 0xF9A97A, 0xE7E742, 0xCE3DC9, 0xB1C238, 0x9818BF, 0x865687, 0x800000;

    .codesegment pm;

$downsample_sub_to_1k2:

    $push_rLink_macro;

    // If we're out of data stop transmitting to the subwoofer.
    r0 = &$multi_chan_sub_out_cbuffer_struc;
    call $cbuffer.calc_amount_data;
    Null = r0 - 360;
    if NEG jump $pop_rLink_and_rts;

    // if running at 48 kHz, then do 40 to one downsampling from sub_out[] to sub_out_1k2[]
    // if running 44.1 kHz, then downsample by 36.75 by picking nearest sample

    // get sub output cbuffer pointer
    r0 = $multi_chan_sub_out_cbuffer_struc;
    call $cbuffer.get_read_address_and_size;
    i0 = r0;
    l0 = r1;

    // get 1.2 kHz sub audio cbuffer pointer
    r0 = $sub_out_1k2_cbuffer_struc;
    call $cbuffer.get_write_address_and_size;
    i4 = r0;
    l4 = r1;

    // determine amount of data to downsample depending on sub port type
    r0  = $L2CAP_SUB_AUDIO_PACKET_SIZE;
    r10 = $ESCO_SUB_AUDIO_PACKET_SIZE;
    r1 = M[$sub_link_port];
    Null = r1 - $AUDIO_L2CAP_SUB_OUT_PORT;
    if Z r10 = r0;

    r1 = 0;
    M0 = 40;
    r1 = 0;

    r0 = M[$sub_link_port];
    Null = r0 - $AUDIO_L2CAP_SUB_OUT_PORT;
    if z jump downsample;

    // If link to subwoofer is ESCO and the input to the Soundbar is 44.1 kHz USB
    // we need to resample such that we're creating 9 audio samples every 7.5 ms.

    r1 = 0;
    // In multi-channel mode the sub channel rate is the same as the dac rate
    // (since the resampling is done pre-Music Manager)
    r0 = m[$current_dac_sampling_rate];
    Null = r0 - 44100;
    if nz jump downsample;
    M0 = 36;
    r1 = 0.75;

downsample:
    r4 = m[mute_index];
    r5 = m[mute_direction];
    r2 = m[residual];
    do copy_sub_to_downsampled_buffer;
        r0 = m[i0,m0];       // full rate sub channel

        // work out gain value for mute from ramp table
        r4 = r4 + r5;
        if neg r4 = 0;
        r3 = length(sub_mute_coefs)-1;
        Null = r3 - r4;
        if neg r4 = r3;
        r3 = sub_mute_coefs;
        r3 = r4 + r3;
        r3 = m[r3];                             // get gain

        // apply gain to audio
        r0 = r0 * r3 (frac);                    // apply gain to audio
        r0 = -r0;                               // invert

        // tweak audio input index depending on size of residual
        r2 = r2 + r1;
        if POS jump no_extra_read;
           r3 = M[I0,1];      // dummy read to update pointer
           r2 = r2 AND 0x7FFFFF;
        no_extra_read:
        M[i4,1] = r0;          // downsampled sub channel for delivery over the air
    copy_sub_to_downsampled_buffer:

    m[residual] = r2;
    m[mute_index] = r4;

    // update cbuffer read address
    r0 = $multi_chan_sub_out_cbuffer_struc;
    r1 = i0;
    call $cbuffer.set_read_address;
    l0 = 0;

    // update cbuffer write address
    r0 = $sub_out_1k2_cbuffer_struc;
    r1 = i4;
    call $cbuffer.set_write_address;
    l4 = 0;

    // pop rLink from stack
    jump $pop_rLink_and_rts;

.endmodule;


// r0 = message ID
// r1 = message Data 0
// r2 = message Data 1
// r3 = message Data 2
// r4 = message Data 3
// word 0 - bits 0:6  - either type of port number
//          bit 7     - set on failure
//          bits 8:15 - type field of typed BDADDR
// word 1 - bits 47:32 of BDADDR
// word 2 - bits 31:16 of BDADDR
// word 3 - bits 15:0 of BDADDR

.MODULE $M.bdaddr_message_handler;
    .CODESEGMENT PM;
    .DATASEGMENT DM;

    $bdaddr_message_handler:

    $push_rLink_macro;

    // store device address
    r1 = r1 LSHIFT -8;
    M[$bdaddr_struc + 0] = r1;  // type
    M[$bdaddr_struc + 1] = r2; // bits 47:32 of BDADDR
    M[$bdaddr_struc + 2] = r3; // bits 31:16 of BDADDR
    M[$bdaddr_struc + 3] = r4; // bits 15:0 of BDADDR

    // request wall clock
    r2 = Null OR $MESSAGE_GET_WALL_CLOCK_TBTA;
    r3 = M[$bdaddr_struc + 0];
    r4 = M[$bdaddr_struc + 1];
    r5 = M[$bdaddr_struc + 2];
    r6 = M[$bdaddr_struc + 3];
    call $message.send;

    // start wall clock update timer
    r1 = &$wall_clock_timer_struc;
    r2 = $WALL_CLOCK_UPDATE_PERIOD;
    r3 = &$wall_clock_timer_handler;
    call $timer.schedule_event_in;

    r1 = 1;
    M[$subwoofer.running] = r1;

    // pop rLink from stack
    jump $pop_rLink_and_rts;

.ENDMODULE;

// r0 = $message.LONG_MESSAGE_MODE_ID (a special flag to imply long message mode)
// r1 = long message ID (the actual ID of the message to send)
// r2 = long message size (in words)
// r3 = long message address of payload
//
// word 0 - type of BDADDR
// word 1 - bits 47:32 of BDADDR
// word 2 - bits 31:16 of BDADDR
// word 3 - bits 15:0 of BDADDR
// word 4 - MS 16bits of clock
// word 5 - LS 16bits of clock
// word 6 - MS 16bits of TIMER_TIME when the clock value was valid
// word 7 - LS 16bits of TIMER_TIME when the clock value was valid

.MODULE $M.wall_clock_message_handler;
    .CODESEGMENT PM;
    .DATASEGMENT DM;

    $wall_clock_message_handler:

    $push_rLink_macro;

    // store wall clock

    // calc: 'firmware wall clock' x FIRMWARE_WALL_CLOCK_PERIOD
    r0 = M[r3 + 4];
    M[$wall_clock_msw] = r0;
    r0 = M[r3 + 5];
    M[$wall_clock_lsw] = r0;

    // store local time (convert to 24bit)
    r0 = M[r3 + 6];
    r1 = M[r3 + 7];
    r0 = r0 LSHIFT 16;
    r1 = r1 AND 0xFFFF;
    r1 = r1 + r0;
    M[$wall_clock_time] = r1;

#if 1  // TODO: This will get cleaned up when we move over to using the wall clock library functions.
    r1 = M[r3 + 4];
    M[$wall_clock_msw] = r1;
    r2 = M[r3 + 5];
    M[$wall_clock_lsw] = r2;

    rMAC = r1 LSHIFT -8;
    rMAC = rMAC AND 0xFF;
    rMAC = rMAC * ($wall_clock.FIRMWARE_WALL_CLOCK_PERIOD_VALUE * 2) (int);
    r1 = r1 LSHIFT 16;
    r2 = r2 AND 0xFFFF;
    r2 = r2 + r1;
    rMAC = rMAC + r2 * ($wall_clock.FIRMWARE_WALL_CLOCK_PERIOD_VALUE) (UU);
    r2 = rMAC LSHIFT (23 + $wall_clock.FIRMWARE_WALL_CLOCK_PERIOD_SHIFT);

    // calc:  - 'firmware timer time'
    r4 = M[r3 + 7];
    r3 = M[r3 + 6];
    r3 = r3 LSHIFT 16;
    r4 = r4 AND 0xFFFF;
    r4 = r4 + r3;
    r2 = r2 - r4;
    .VAR $wall_clock_adjustment;
    M[$wall_clock_adjustment] = r2;
#endif


   // request sco parameters if we haven't fetched them already
   Null = M[$sco_param_tesco];
   if NZ jump done;

   call $block_interrupts;

   // Purge the wired output multi-channel cbuffers
   call $multi_chan_purge_buffers;

   r0 = M[&$sub_out_1k2_cbuffer_struc + $cbuffer.WRITE_ADDR_FIELD];
   M[&$sub_out_1k2_cbuffer_struc + $cbuffer.READ_ADDR_FIELD] = r0;

   r0 = M[&$sub_codec_out_cbuffer_struc + $cbuffer.WRITE_ADDR_FIELD];
   M[&$sub_codec_out_cbuffer_struc + $cbuffer.READ_ADDR_FIELD] = r0;

   call $unblock_interrupts;

    r1 = M[$sub_link_port];
    Null = r1 - $AUDIO_L2CAP_SUB_OUT_PORT;
    if Z jump uses_l2cap;

    r2 = $MESSAGE_GET_SCO_PARAMS;
    r3 = M[$sub_link_port];             // sub port
    // Remove port config and send only port number
    // 91A00E AND 0x00001f = 0x00000E i.e port 2
    r3 = r3 AND $cbuffer.TOTAL_PORT_NUMBER_MASK;
    r4 = 0;
    r5 = 0;
    r6 = 0;
    call $message.send;

   // need to set delay buffer and clear buffers
   r0 = $ESCO_SUB_ALIGNMENT_DELAY;
   M[$M.system_config.data.delay_pri_left + $audio_proc.delay.DELAY_FIELD] = r0;
   M[$M.system_config.data.delay_pri_right + $audio_proc.delay.DELAY_FIELD] = r0;
   M[$M.system_config.data.delay_sec_left + $audio_proc.delay.DELAY_FIELD] = r0;
   M[$M.system_config.data.delay_sec_right + $audio_proc.delay.DELAY_FIELD] = r0;
   M[$M.system_config.data.delay_aux_left + $audio_proc.delay.DELAY_FIELD] = r0;
   M[$M.system_config.data.delay_aux_right + $audio_proc.delay.DELAY_FIELD] = r0;
   M[$M.main.samples_latency_measure + 0] = r0;

   r8 = &$M.system_config.data.delay_pri_left;
   call $audio_proc.delay.initialize;
   r8 = &$M.system_config.data.delay_pri_right;
   call $audio_proc.delay.initialize;
   r8 = &$M.system_config.data.delay_sec_left;
   call $audio_proc.delay.initialize;
   r8 = &$M.system_config.data.delay_sec_right;
   call $audio_proc.delay.initialize;
   r8 = &$M.system_config.data.delay_aux_left;
   call $audio_proc.delay.initialize;
   r8 = &$M.system_config.data.delay_aux_right;
   call $audio_proc.delay.initialize;

   jump done;

uses_l2cap:
    r0 = 1;
    M[$sco_param_tesco] = r0;
    r0 = $L2CAP_PACKET_SIZE_BYTES;
    M[$sco_param_to_air_size] = r0;

done:
    // pop rLink from stack
    jump $pop_rLink_and_rts;

.ENDMODULE;


// r0 = $message.LONG_MESSAGE_MODE_ID (a special flag to imply long message mode)
// r1 = long message ID (the actual ID of the message to send)
// r2 = long message size (in words)
// r3 = long message address of payload
//
// word 0 - either type of port number
// word 1 - Tesco, in slots
// word 2 - Wesco, in slots
// word 3 - to-air packet length, in bytes
// word 4 - from-air packet length, in bytes
// word 5/6 - wall clock value for the start of the first
//            reserved slot (MSW in 5, LSW in 6)
// word 7 - to-air processing time required by firmware, in us
// word 8 - from-air processing time required by firmware, in us

.MODULE $M.sco_params_message_handler;
    .CODESEGMENT PM;
    .DATASEGMENT DM;

    $sco_params_message_handler:

    $push_rLink_macro;

    // store Tesco
    r0 = M[r3 + 1];
    M[$sco_param_tesco] = r0;

    // store packet sizes
    r0 = M[r3 + 3];
    M[$sco_param_to_air_size] = r0;
    r0 = M[r3 + 4];
    M[$sco_param_from_air_size] = r0;

    // join MSW & LSW to get 32 bit SCO clock value
    r0 = M[r3 + 5];
    r1 = M[r3 + 6];
    r0 = r0 LSHIFT 16;
    r1 = r1 AND 0xFFFF;
    r1 = r1 + r0;
    r0 = M[r3 + 5];
    r0 = r0 LSHIFT - 8;
    M[$sco_param_clock_msw] = r0;
    M[$sco_param_clock_lsw] = r1;

    // store F/W deadlines (sign extend to 24 bits)
    r0 = M[r3 + 7];
    r0 = r0 LSHIFT 8;
    r0 = r0 ASHIFT -8;
    M[$sco_param_to_air_time] = r0;
    r0 = M[r3 + 8];
    r0 = r0 LSHIFT 8;
    r0 = r0 ASHIFT -8;
    M[$sco_param_from_air_time] = r0;

    // cancel any running timer
    r2 = M[$sco_timer_id];
    call $timer.cancel_event;

    // turn-on frame based semantics
    r2 = Null OR $MESSAGE_SET_FRAME_LENGTH;
    r3 = M[$sub_link_port];
    // Remove port config and send only port number
    // 91A00E AND 0x00001f = 0x00000E i.e port 2
    r3 = r3 AND $cbuffer.TOTAL_PORT_NUMBER_MASK;
    r4 = M[$sco_param_to_air_size];
    call $message.send;

    // schedule timer
    r0 = 1;
    M[$M.Subwoofer.sco_schedule_to_air_flag] = r0;

    // pop rLink from stack
    jump $pop_rLink_and_rts;

.ENDMODULE;


.MODULE $M.sco_schedule_to_air;
    .CODESEGMENT PM;
    .DATASEGMENT DM;

    .VAR $esco_write_deadline;

    $sco_schedule_to_air:

    $push_rLink_macro;

    // join MSW & LSW to get 32 bit wall-clock value
    r1 = M[$wall_clock_msw];
    r2 = M[$wall_clock_lsw];
    r1 = r1 LSHIFT 16;
    r2 = r2 AND 0xFFFF;
    r2 = r2 + r1;
    r1 = M[$wall_clock_msw];
    r1 = r1 LSHIFT - 8;

    // get sco clock
    r3 = M[$sco_param_clock_msw];
    r4 = M[$sco_param_clock_lsw];

sco_schedule_next_instant:

    // calculate difference between sco clock and wall-clock
    r6 = r4 - r2;           // lsw
    r5 = r3 - r1 - borrow;  // msw
    // todo handle clock wrap around

    // convert into uS (1 clk is 312.5uS)
    r5 = r6 * 625 (int);
    r5 = r5 ASHIFT -1;

    // add to wall-clock time to get absolute time
    r0 = M[$wall_clock_time];
    r2 = r0 + r5;

    // subtract F/W deadline
    r2 = r2 - M[$sco_param_to_air_time];

    r3 = r2;  // write needs to occur before this time

    // subtract DSP time
    r2 = r2 - M[$subwoofer.sco_offset];

   // see if we're past this time already
    Null = r2 - M[$TIMER_TIME];
    if POS jump continue;
    // time has expired, advance by tesco
    r1 = M[$sco_param_tesco];
    r1 = r1 * 625 (int);
advance:
    r2 = r2 + r1;
    r3 = r3 + r1;
    Null = r2 - M[$TIMER_TIME];
    if NEG jump advance;
continue:

    M[$esco_write_deadline] = r3;

    push r2;

    // cancel timer if one is set
    r2 = M[$sco_timer_id];
    call $timer.cancel_event;

    pop r2;

    r1 = &$sco_timer_struc;
    r3 = &$sco_timer_handler;
    call $timer.schedule_event_at;
    M[$sco_timer_id] = r3;

#if 0
    r0 = $multi_chan_sub_out_cbuffer_struc;
    call $cbuffer.get_write_address_and_size;
    I0 = r0;
    L0 = r1;
    M0 = -100;
    r0 = M[I0,M0];
    r0 = $multi_chan_sub_out_cbuffer_struc;
    r1 = I0;
    call $cbuffer.set_read_address;
    L0 = 0;
#endif

    M[$M.Subwoofer.sco_schedule_to_air_flag] = Null;

    // pop rLink from stack
    jump $pop_rLink_and_rts;

.ENDMODULE;

.module $M.sco_timer_handler;
    .codesegment pm;
    .datasegment dm;

    .var $time_before_fp;

    $sco_timer_handler:

    $push_rLink_macro;

    // schedule next timer handler
    r2 = $L2CAP_FRAME_PROCESSING_RATE;
    r1 = &$sco_timer_struc;
    r3 = &$sco_timer_handler;
    call $timer.schedule_event_in_period;
    M[$sco_timer_id] = r3;

    // Wake $SystemSleepEsco
    M[$M.Sleep.sync_flag_esco] = Null;

    // pop rLink from stack
    jump $pop_rLink_and_rts;

.endmodule;


.MODULE $M.wall_clock_timer_handler;
    .CODESEGMENT PM;
    .DATASEGMENT DM;

    .VAR $wall_clock_timer_id = 0;

    $wall_clock_timer_handler:

    $push_rLink_macro;

    // request wall clock
    r2 = Null OR $MESSAGE_GET_WALL_CLOCK_TBTA;
    r3 = M[$bdaddr_struc + 0];
    r4 = M[$bdaddr_struc + 1];
    r5 = M[$bdaddr_struc + 2];
    r6 = M[$bdaddr_struc + 3];
    call $message.send;

    // schedule next wall clock update
    r1 = &$wall_clock_timer_struc;
    r2 = $WALL_CLOCK_UPDATE_PERIOD;
    r3 = &$wall_clock_timer_handler;
    call $timer.schedule_event_in_period;
    M[$wall_clock_timer_id] = r3;

    // pop rLink from stack
    jump $pop_rLink_and_rts;

.ENDMODULE;


//------------------------------------------------------------------------------
.module $M.create_sub_packet;
//------------------------------------------------------------------------------
    .codesegment pm;
    .datasegment dm;

    .var packet_counter = 1;

$create_sub_packet:

    $push_rLink_macro;

    // Prevent routine from continuously transmitting stale data to the subwoofer when USB is paused or stoppped.
    r0 = $sub_out_1k2_cbuffer_struc;
    call $cbuffer.calc_amount_data;

    r2 = $ESCO_SUB_AUDIO_PACKET_SIZE;
    r10 = r0 - r2;
    if POS jump continue;
       r10 = Null - r10;
       r0 = $sub_out_1k2_cbuffer_struc;
       call $cbuffer.get_write_address_and_size;
       I0 = r0;
       L0 = r1;
       r0 = 0;
       do zero_insertion;
          M[I0,1] = r0;
       zero_insertion:
       r0 = $sub_out_1k2_cbuffer_struc;
       r1 = I0;
       call $cbuffer.set_write_address;
       L0 = 0;

    continue:
    // get output cbuffer pointer
    r0 = $sub_codec_out_cbuffer_struc;
    call $cbuffer.get_write_address_and_size;
    i0 = r0;
    l0 = r1;

    // get sub audio cbuffer pointer
    r0 = $sub_out_1k2_cbuffer_struc;
    call $cbuffer.get_read_address_and_size;
    i4 = r0;
    l4 = r1;

    // get size of frame
    r10 = m[$sco_param_to_air_size];
    r10 = r10 ashift -1;

    // write sync word
    r0 = 0xf5f5;
    m[i0,1] = r0;

    // write packet identifier (1 or 2)
    r4 = m[packet_counter];
    r0 = r4 AND 0xffff;
    m[i0,1] = r0; // LSW

    // write wall clock
    r0 = M[$wall_clock_adjustment];
    r0 = r0 + M[$TIMER_TIME];           // current time

    r2 = r0 LSHIFT -16;                 // MSB
    r5 = r4 AND 0xFF0000;
    r5 = r5 LSHIFT -8;                  // MSB of packet counter
    r2 = r5 + r2;                       // LSB packet counter, LSB wall clock
    M[I0,1] = r2;
    r2 = r0 AND 0xFFFF;                 // LSW
    M[I0,1] = r2;

    // check whether writing to L2CAP sub port or ESCO sub port
    r1 = M[$sub_link_port];
    Null = r1 - $AUDIO_L2CAP_SUB_OUT_PORT;
    if Z jump write_l2cap_sub_data;

    // write "$ESCO_SUB_AUDIO_PACKET_SIZE" audio samples, then write zero padding to fill sco packet
    r2 = $ESCO_SUB_AUDIO_PACKET_SIZE;
    r3 = r2 + 4;   // ($ESCO_SUB_AUDIO_PACKET_SIZE + 1 /*0xf5f5*/ + 1 /*packet count*/ + 2 /*wallClock*/);      // amount of padding
    r3 = r10 - r3; // r3 = amount of zero padding
    r10 = r2;
    do copy_esco_sub_data_to_air;
        r0 = m[i4,1];       // audio to send to sub
        r0 = r0 ashift -8;
        m[i0,1] = r0;
    copy_esco_sub_data_to_air:
    r10 = r3;
    do zero_padding;
        r0 = 0;
        m[i0,1] = r0;
    zero_padding:

    // don't update read pointer if first transmission of packet (we will be retransmitting it)
    r1 = M[$sco_param_tesco];
    Null = r1 - 6;
    if NZ jump update_read_addr;  // we only send one packet when tesco = 12

    // tesco must = 6
    Null = r4 - 1;
    if Z jump dont_update_sub_1k2_read_ptrs;
    update_read_addr:
        // update cbuffer read address
        r0 = $sub_out_1k2_cbuffer_struc;
        r1 = i4;
        call $cbuffer.set_read_address;
    dont_update_sub_1k2_read_ptrs:

    r1 = M[$sco_param_tesco];
    Null = r1 - 6;
    if NZ jump do_not_send_duplicates;
       // if transmitted 2nd packet, reset packet counter
       r1 = 1;
       r4 = r4 + 1;
       Null = r4 - 2;
       if ne r4 = r1;
       m[packet_counter] = r4;
    do_not_send_duplicates:

    jump done;

    write_l2cap_sub_data:

    // write time to play value and current warp value
    r0 = M[$M.Subwoofer.playback_time];
    r2 = r0 LSHIFT -16;                       // MSB
    r1 = Null;                                // Warp is applied before frame processing so we send zero to the subwoofer
    r3 = r1 LSHIFT -16;                       // MSB of current warp
    r3 = r3 LSHIFT 8;
    r2 = r3 + r2;                             // MSB of current warp, MSB of TIME_TO_PLAY
    M[I0,1] = r2;
    r2 = r0 AND 0xFFFF;                       // LSW of TIME_TO_PLAY
    M[I0,1] = r2;
    r2 = r1 AND 0xFFFF;                       // LSW of current warp
    M[I0,1] = r2;


    // write "$L2CAP_SUB_AUDIO_PACKET_SIZE" audio samples.  L2CAP doesn't have any zero padding
    r10 = $L2CAP_SUB_AUDIO_PACKET_SIZE;
    do copy_l2cap_sub_data_to_air;
        r0 = m[i4,1];       // audio to send to sub
        r0 = r0 ashift -8;
        m[i0,1] = r0;
    copy_l2cap_sub_data_to_air:

    // update cbuffer read address
    r0 = $sub_out_1k2_cbuffer_struc;
    r1 = i4;
    call $cbuffer.set_read_address;

    // increment packet counter
    r4 = r4 + 1;
    m[packet_counter] = r4;

    done:

    // update cbuffer write address
    r0 = $sub_codec_out_cbuffer_struc;
    r1 = i0;
    call $cbuffer.set_write_address;
    l0 = 0;
    l4 = 0;

    // pop rLink from stack
    jump $pop_rLink_and_rts;

.endmodule;

.MODULE $M.subwoofer;
    .CODESEGMENT PM;
    .DATASEGMENT DM;

    .VAR $subwoofer.running = 0;

    $disable_subwoofer:

    $push_rLink_macro;

    // Adjust codec rate adjustment
    r0 = M[$sra_rate_addr];
    M[$codec_rate_adj.stereo + $cbops.rate_adjustment_and_shift.Process.SRA_TARGET_RATE_ADDR_FIELD] = r0;

    // cancel timers used with subwoofer
    r2 = M[$sco_timer_id];
    call $timer.cancel_event;

    r2 = M[$wall_clock_timer_id];
    call $timer.cancel_event;

    r2 = M[$bass_timer_id];
    call $timer.cancel_event;

    M[$bdaddr_struc + 0] = Null;
    M[$bdaddr_struc + 1] = Null;
    M[$bdaddr_struc + 2] = Null;
    M[$bdaddr_struc + 3] = Null;
    M[$sco_param_tesco]  = Null;
    M[$sub_link_port]    = Null;
    M[$M.Sleep.sync_flag_esco] = Null;
    M[$M.Subwoofer.playback_time] = Null;
    M[$M.system_config.data.delay_pri_left + $audio_proc.delay.DELAY_FIELD] = Null;
    M[$M.system_config.data.delay_pri_right + $audio_proc.delay.DELAY_FIELD] = Null;
    M[$M.system_config.data.delay_sec_left + $audio_proc.delay.DELAY_FIELD] = Null;
    M[$M.system_config.data.delay_sec_right + $audio_proc.delay.DELAY_FIELD] = Null;
    M[$M.system_config.data.delay_aux_left + $audio_proc.delay.DELAY_FIELD] = Null;
    M[$M.system_config.data.delay_aux_right + $audio_proc.delay.DELAY_FIELD] = Null;
    M[$M.main.samples_latency_measure + 0] = Null;
    M[$music_example.OTA_IF_Connections] = Null;

    // Set crossover to use the filter set designed for no subwoofer
    r0 = 1;
    M[$M.system_config.data.spkr_ctrl_coefs + $spkr_ctrl_system.COEF_CONFIG] = r0;

    M[$M.system_config.data.spkr_ctrl_struct + $spkr_ctrl_system.SUB_OUTPUT_PTR] = Null;

    r7 = &$M.system_config.data.spkr_ctrl_bass_buffer;
    r8 = length(&$M.system_config.data.spkr_ctrl_bass_buffer);
    call  $spkr_ctrl_system.zero_data;

    r7 = &$M.system_config.data.spkr_ctrl_init_struct;
    call $spkr_ctrl_system.initialize;

    r0 = $music_example.NUM_SAMPLES_PER_FRAME;
    M[$music_example.frame_processing_size] = r0;

    r3 = $MESSAGE_PORT_BT_ADDRESS;
    call $message.unregister_handler;

    // register message handler for sco parameters
    r3 = $LONG_MESSAGE_SCO_PARAMS_RESULT;
    call $message.unregister_handler;

    // set up message hander for $LONG_MESSAGE_WALL_CLOCK_RESPONSE message
    r3 = $LONG_MESSAGE_WALL_CLOCK_RESPONSE_TBTA;
    call $message.unregister_handler;

done:
    // pop rLink from stack
    jump $pop_rLink_and_rts;

.ENDMODULE;

.MODULE $M.enable_esco_subwoofer;

   .CODESEGMENT PM;
   .DATASEGMENT DM;

   .VAR $subwoofer.sco_offset;

   $enable_esco_subwoofer:

   $push_rLink_macro;

   M[$subwoofer.sco_offset] = r1;

   // register message handler for bt address
   r1 = &$bdaddr_message_struc;
   r2 = $MESSAGE_PORT_BT_ADDRESS;
   r3 = &$bdaddr_message_handler;
   call $message.register_handler;

   // register message handler for sco parameters
   r1 = &$sco_params_message_struc;
   r2 = $LONG_MESSAGE_SCO_PARAMS_RESULT;
   r3 = &$sco_params_message_handler;
   call $message.register_handler;

   // set up message hander for $LONG_MESSAGE_WALL_CLOCK_RESPONSE message
   r1 = &$wall_clock_message_struc;
   r2 = $LONG_MESSAGE_WALL_CLOCK_RESPONSE_TBTA;
   r3 = &$wall_clock_message_handler;
   call $message.register_handler;

done:
    // pop rLink from stack
    jump $pop_rLink_and_rts;

.ENDMODULE;

#endif // WIRELESS_SUB_ENABLE


.MODULE $M.calc_time_to_play;
   .DATASEGMENT DM;
   .CODESEGMENT PM;

    $calc_time_to_play:

    // push rLink onto stack
    $push_rLink_macro;

    call $block_interrupts;

    // Count the number of cbuffer samples, which will be played before this frame.
    // Each buffer can have its own sample rate associated with it.  The data structure
    // is populated with inverse sample rates so we can use a multiplication.

    r6 = 0;
    proc_loop:
       r0 = M[r7];
       // Null value terminates List
       if Z jump done_counting_cbuffer_data;
       // calculate amount in buffer
       call $cbuffer.calc_amount_data;
       // get inverse of sample rate
       r1 = M[r7 + 1];
       r1 = M[r1];
       // compute q29.19 result
       rMAC = r0 * r1;
       // convert to 24.24
       rMAC = rMAC ASHIFT 5;
       // upper 24 bits is the amount of time it will take the data to play out of this cbuffer
       r1 = rMAC1;
       // accumulate time
       r6 = r6 + r1;
       // point to next cbuffer
       r7 = r7 + 2;
    jump proc_loop;
done_counting_cbuffer_data:

    // r6 contains the amount of time it will take to play data currently in
    // the cbuffers.  It's in microsecods.

calculate_num_samples_in_port:
    r0 = M[r8 + $CALC_TIME_TO_PLAY_PRIMARY_LEFT_PORT_FIELD];
    call $cbuffer.calc_amount_space;
    r0 = r0 + 1;        // compensate for routine reporting 1 less that true amount of space

    // Get the size in samples
    push r3;
    r3 = M[r8 + $CALC_TIME_TO_PLAY_PRIMARY_LEFT_PORT_FIELD];
    // Convert to samples (in: r2 = octets, r3 = port config; out r2 = samples)
    call $cbuffer.mmu_octets_to_samples;
    pop r3;
    r0 = r2 - r0;       // number of samples in port = size of buffer - amount of space

    // get inverse of port sample rate
    r1 = M[r8 + $CALC_TIME_TO_PLAY_PORT_US_PER_SAMPLE_PTR_FIELD];
    r1 = M[r1];
    // compute q29.19 result
    rMAC = r0 * r1;
    // convert to 24.24
    rMAC = rMAC ASHIFT 5;
    // upper 24 bits is the amount of time it will take the data to play out of this port
    r1 = rMAC1;

    // Push time in port
    push r1;

    // add latency of delay buffer
    r0 = M[r8 + $CALC_TIME_TO_PLAY_SUB_ALIGNMENT_DELAY_FIELD];
    // get inverse of delay buffer sample rate
    r1 = M[r8 + $CALC_TIME_TO_PLAY_SUB_ALIGNMENT_US_PER_SAMPLE_FIELD];
    r1 = M[r1];
    // compute q29.19 result
    rMAC = r0 * r1;
    // convert to 24.24
    rMAC = rMAC ASHIFT 5;
    // upper 24 bits is the amount of time it will take the data to play out of this cbuffer
    r1 = rMAC1;

    // Compute total amount of time ahead of this current frame in delay and cbuffers
    r6 = r6 + r1;

    // Add amount of time in port
    pop r1;
    r2 = r6 + r1;

    // Add absolute time in front of this frame to wall clock time.  This
    // value will be sent to the slave so it knows when to play the frame.
    r1 = M[$wall_clock_adjustment];
    r1 = r1 + M[$TIMER_TIME];
    r6 = r1 + r2;
    M[r8 + $CALC_TIME_TO_PLAY_TIME_TO_PLAY_FIELD] = r6;  // Store TIME_TO_PLAY

done:

    call $unblock_interrupts;

    jump $pop_rLink_and_rts;

.ENDMODULE;


.MODULE $M.audio_insert_silence;
    .CODESEGMENT PM;
    .DATASEGMENT DM;

$audio_insert_silence:

    $push_rLink_macro;

    // get audio port write address
    r0 = r3;
    call $cbuffer.get_write_address_and_size;
    I0 = r0;
    L0 = r1;

    r0 = 0;
    // generate silence
    do audio_fill_loop;
        M[I0,1] = r0;
    audio_fill_loop:

    // set new write address
    r0 = r3;
    r1 = I0;
    call $cbuffer.set_write_address;
    L0 = 0;

    // pop rLink from stack
    jump $pop_rLink_and_rts;

.ENDMODULE;


// *****************************************************************************
// MODULE:
//    $M.calc_us_per_sample_addr
//
// DESCRIPTION:
//    Calculate the number of usec per sample given the sampling rate and
//    return the address of the value (assumes 44100Hz or 48000Hz rates only)
//
//  INPUT:
//    r0 - Sampling rate (in Hz)
//
//  OUTPUT:
//    r3 - Address of value for the number of usec per sample
//
//  TRASHED REGISTERS:
//    r2
//
// *****************************************************************************
.MODULE $M.calc_us_per_sample_addr;

   .CODESEGMENT PM;

   $calc_us_per_sample_addr:

   r2 = $us_per_sample_sbc + 2;                    // 44100 Hz table entry address
   r3 = $us_per_sample_sbc + 3;                    // Default to 48000 Hz table entry address
   Null = r0 - 48000;
   if NZ r3 = r2;                                  // Select 44100 Hz table entry address

   rts;

.ENDMODULE;

