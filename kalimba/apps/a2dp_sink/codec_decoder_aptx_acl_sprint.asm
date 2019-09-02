// *****************************************************************************
// Copyright (c) 2003 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
//
// *****************************************************************************

// *****************************************************************************
// DESCRIPTION
// APTX ACL SPRINT decoder for an audio playing device
//
// *****************************************************************************
#define DEBUG_STATSx

#include "core_library.h"
#include "cbops_library.h"
#include "codec_library.h"
#include "frame_sync_library.h"
#include "music_example.h"
#include "spi_comm_library.h"
#include "mips_profile.h"
#include "sr_adjustment_gaming.h"
#include "codec_library.h"
#include "codec_decoder_aptx_acl_sprint.h"
#include "audio_proc_library.h"
#include "multichannel_output_macros.h"
#include "frame_sync_stream_macros.h"

// unused, but 'mute_direction' is referenced in vm_message.asm
.module $M.downsample_sub_to_1k2;
    .datasegment dm;
    .var mute_direction = 0;
.endmodule;

// Plugin type set from VM at run-time
.MODULE $app_config;
   .DATASEGMENT DM;

   .VAR io = $INVALID_IO;
.ENDMODULE;

.MODULE $M.main;
   .CODESEGMENT MAIN_PM;
   .DATASEGMENT DM;

   $main:

   // Allocate memory for cbuffer structures
   DeclareCBuffer($codec_in_cbuffer_struc,$codec_in,CODEC_CBUFFER_SIZE);
   DeclareCBuffer($audio_out_left_cbuffer_struc,$audio_out_left,AUDIO_CBUFFER_SIZE);
   DeclareCBuffer($audio_out_right_cbuffer_struc,$audio_out_right,AUDIO_CBUFFER_SIZE);

  // Structure to measure output port consuming rate
   .VAR $calc_chain0_actual_port_rate_struc[$calc_actual_port_rate.STRUC_SIZE] =
      $MULTI_CHAN_PRIMARY_LEFT_OUT_PORT, // PORT_FIELD
      0 ...;

   // ** allocate memory for timer structures **
   .VAR $codec_timer_struc[$timer.STRUC_SIZE];
   .VAR $audio_out_timer_struc[$timer.STRUC_SIZE];
   .VAR $signal_detect_timer_struc[$timer.STRUC_SIZE];

   // Variables to receive dac and codec sampling rates from the vm
   .VAR $current_dac_sampling_rate = 0;                            // Dac sample rate, set by message from VM
   .VAR $set_dac_rate_from_vm_message_struc[$message.STRUC_SIZE];  // Message structure for VM_SET_DAC_RATE_MESSAGE_ID message
   .VAR $current_codec_sampling_rate = 0;                          // codec data sample rate, set by vm
   .VAR $set_codec_rate_from_vm_message_struc[$message.STRUC_SIZE];// Message structure for VM_SET_CODEC_RATE_MESSAGE_ID message
   .VAR $get_aptx_ll_params_from_vm_message_struc1[$message.STRUC_SIZE];
   .VAR $get_aptx_ll_params_from_vm_message_struc2[$message.STRUC_SIZE];

   // Rate matching control variables
   .VAR $local_play_back;
   .VAR $audio_if_mode; // output interface type
   .VAR $max_clock_mismatch;
   .VAR $long_term_mismatch;
   .VAR $aux_input_stream_available;       // local pcm file is being mixed

   // ** allocate memory for codec input cbops copy routine **
   .VAR $codec_in_copy_struc[] =
          $codec_in_copy_op,             // first operator block
          1,                             // number of inputs
          $CODEC_IN_PORT,                // input
          1,                             // number of outputs
          $codec_in_cbuffer_struc;       // output

   .BLOCK $codec_in_copy_op;
      .VAR $codec_in_copy_op.next = $cbops.NO_MORE_OPERATORS;
      .VAR $codec_in_copy_op.func = &$cbops.copy_op;
      .VAR $codec_in_copy_op.param[$cbops.copy_op.STRUC_SIZE] =
               0,                         // Input index
               1;                         // Output index
   .ENDBLOCK;

   // Allocating memory for decoder codec stream struc
   .VAR/DM1 $decoder_codec_stream_struc[$codec.av_decode.STRUC_SIZE] =
      0,                                           // frame_decode function (set by conn_init function)
      0,                                           // reset_decoder function (set by conn_init function)
      0,                                           // silence_decoder function (set by conn_init function)
      &$codec_in_cbuffer_struc,                    // in cbuffer
      &$audio_out_left_cbuffer_struc,              // out left cbuffer
      &$audio_out_right_cbuffer_struc,             // out right cbuffer
      0,                                           // MODE_FIELD
      0,                                           // number of output samples
      0,                                           // data object pointer placeholder
      30000,                                           // DECODE_TO_STALL_THRESHOLD
      GOOD_WORKING_BUFFER_LEVEL,                   // STALL_BUFFER_LEVEL_FIELD
      0,                                           // NORMAL_BUFFER_LEVEL , POORLINK_DETECT_LEVEL - no longer used
      1,                                           // Enable codec in buffer purge when in pause
      &$master_app_reset,
      0 ...;                                       // Pad out remaining items with zeros

   // allocate meory for inverse of dac and codec sample rates
   .VAR $inv_codec_fs = $latency.INV_FS(48000);
   .VAR $inv_dac_fs = $latency.INV_FS(48000);

#ifdef LATENCY_REPORTING

   .VAR zero = 0;

   // WARP rate variable used for latency calculation on buffers before the rate adjustment
   .VAR $latency_calc_current_warp = $codec_rate_adj.stereo + $cbops.rate_adjustment_and_shift.Process.SRA_CURRENT_RATE_FIELD;

   // WARP rate variable used for latency calculation on buffers after the rate adjustment
   .VAR $latency_calc_port_warp = &zero;

   // Resampling is performed pre-Music Manager for Multi-channel operation
   // (when not resampling the resampler output buffer should be empty
   // so it will not contribute to the latency figure)
   .VAR cbuffers_latency_measure[] =
    $audio_out_left_cbuffer_struc, $inv_codec_fs, $latency_calc_current_warp,
    $codec_resamp_out_left_cbuffer_struc, $inv_dac_fs, $latency_calc_current_warp,
    $codec_rate_adj_out_left_cbuffer_struc, $inv_dac_fs, $latency_calc_port_warp,
    $multi_chan_primary_left_out_cbuffer_struc, $inv_dac_fs, $latency_calc_port_warp,
    $MULTI_CHAN_PRIMARY_LEFT_OUT_PORT, $inv_dac_fs, $latency_calc_port_warp,
    0;


    // define samples structure involved in pcm latency
    .VAR samples_latency_measure[] =
       0, &$inv_codec_fs, &$latency_calc_current_warp,
       0;
    // define pcm latency structure
    .VAR $pcm_latency_input_struct[$pcm_latency.STRUC_SIZE] =
      &cbuffers_latency_measure,
      &samples_latency_measure;

    // define encoded latency structure
    .VAR $encoded_latency_struct[$encoded_latency.STRUC_SIZE] =
      $pcm_latency_input_struct,
      &$codec_packets_info_cbuffer_struc,
      $codec_in_cbuffer_struc,
      &$audio_out_left_cbuffer_struc, &$inv_codec_fs, &$latency_calc_current_warp,
      3, // minimum over last three
      0 ...;
     // define cbuffers structure involved in pcm latency

#endif // LATENCY_REPORTING

   // This is the codec type being used
   .VAR $codec_type = -1;

   // This is the codec config being used
   .VAR $codec_config = -1;

   // Timer period variable for reading codec data
   // (this is modified according to the connection type and also to support local playback)
   .VAR $tmr_period_con_copy;

   // Output handler timer period (for writing output data)
   .VAR $tmr_period_audio_copy = TMR_PERIOD_AUDIO_COPY;

   // Rate matching data structure
   .VAR $sra_struct[$sra.STRUC_SIZE];
   .VAR $rate_match_disable = 0;

   // APTX-LL tuning parameters (set from the VM)
   .BLOCK $headset_tuning_params;
      .VAR $target_codec_level  = TARGET_CODEC_BUFFER_LEVEL;
      .VAR $good_working_level  = GOOD_WORKING_BUFFER_LEVEL;
      .VAR $sra_max_rate        = SRA_MAXIMUM_RATE;
      .VAR $sra_averaging_time  = SRA_AVERAGING_TIME;
   .ENDBLOCK;

   // Statistics pointer tables (these pointers are copied to the statistics pointer array by the conn_init function)
   // (this fixed size array contains pointers to codec specific information)
#ifdef APTX_ACL_SPRINT_ENABLE
   .VAR $codec_stats[$music_example.CODEC_STATS_SIZE] =
      &$music_example.SamplingRate,
      &$music_example.aptx_channel_mode,
      &$music_example.aptx_security_status,
      &$music_example.aptx_decoder_version,
      &$M.system_config.data.ZeroValue,
      &$M.system_config.data.ZeroValue,
      &$M.system_config.data.ZeroValue,
      &$M.system_config.data.ZeroValue,                        // CODEC_STAT6
      &$M.system_config.data.ZeroValue,                        // CODEC_STAT7
      &$M.system_config.data.ZeroValue,                        // CODEC_STAT8
      &$M.system_config.data.ZeroValue,                        // CODEC_STAT9
      &$M.system_config.data.ZeroValue,                        // CODEC_STATA
      &$M.system_config.data.ZeroValue;                        // CODEC_STATB

#endif

   // main output channel mode, assume master
   // set this to 1 for i2s slave mode
   .VAR $output_interface_operating_mode = 0;

   // Application resolution modes (default to 16bit mode)
   .VAR $inputResolutionMode = $RESOLUTION_MODE_16BIT;
   .VAR $procResolutionMode = $RESOLUTION_MODE_16BIT;
   .VAR $outputResolutionMode = $RESOLUTION_MODE_16BIT;

   // ANC mode
   .VAR $ancMode = $ANC_NONE;

//------------------------------------------------------------------------------
// Program code
//------------------------------------------------------------------------------

   // initialise the stack library
   call $stack.initialise;
   // initialise the interrupt library
   call $interrupt.initialise;
   // initialise the message library
   call $message.initialise;
   // initialise the cbuffer library
   call $cbuffer.initialise;
   // initialise the pskey library
   call $pskey.initialise;
   // initialise licensing subsystem
   call $Security.Initialize;
   // initialise the wallclock library
   call $wall_clock.initialise;
#if defined(DEBUG_ON)
   // initialise the profiler library
   call $profiler.initialise;
#endif
   // init DM_flash
   call $flash.init_dmconst;

   // set up message handler for VM_SET_DAC_RATE_MESSAGE_ID message
   r1 = &$set_dac_rate_from_vm_message_struc;
   r2 = VM_SET_DAC_RATE_MESSAGE_ID;
   r3 = &$set_dac_rate_from_vm;
   call $message.register_handler;

   // set up message handler for VM_SET_CODEC_RATE_MESSAGE_ID message
   r1 = &$set_codec_rate_from_vm_message_struc;
   r2 = VM_SET_CODEC_RATE_MESSAGE_ID;
   r3 = &$set_codec_rate_from_vm;
   call $message.register_handler;

   // set up message handler for VM_SET_CODEC_RATE_MESSAGE_ID message
   r1 = &$set_tone_rate_from_vm_message_struc;
   r2 = VM_SET_TONE_RATE_MESSAGE_ID;
   r3 = &$set_tone_rate_from_vm;
   call $message.register_handler;

   // set up message handlers for VM_APTX_LL_PARAMS_MESSAGE_ID messages
   r1 = &$get_aptx_ll_params_from_vm_message_struc1;
   r2 = VM_APTX_LL_PARAMS_MESSAGE1_ID;
   r3 = &$get_aptx_ll_params_from_vm1;
   call $message.register_handler;

   r1 = &$get_aptx_ll_params_from_vm_message_struc2;
   r2 = VM_APTX_LL_PARAMS_MESSAGE2_ID;
   r3 = &$get_aptx_ll_params_from_vm2;
   call $message.register_handler;

   // intialize SPI communications library
   call $spi_comm.initialize;

   // Power Up Reset needs to be called once during program execution
   call $music_example.power_up_reset;

   // Send Ready Message to VM so it can connect streams to the Kalimba
   r2 = $music_example.VMMSG.READY;
   r3 = $MUSIC_MANAGER_SYSID;
   // status
   r4 = M[$music_example.Version];
   r4 = r4 LSHIFT -8;
   call $message.send_short;

#ifdef LATENCY_REPORTING
   // reset encoded latency module
   r7 = &$encoded_latency_struct;
   call $latency.reset_encoded_latency;
#endif // LATENCY_REPORTING

   // tell vm we're ready and wait for the go message
   call $message.send_ready_wait_for_go;

   // Defer processing of VM messages until channel configuration is completed
   call $block_interrupts;

   // Initialize the connection type (only A2DP supported here)
   // (allows run-time selection of connection type)
   call $conn_init;

   // Configure the interface map struc (with enabled wired output channels and I2S/DAC usage)
   // interface map struct is set inside function
   //    r3 = interface map structure address
   //       [0]: Wired channel enable mask
   //       [1]: DAC channel mask (sets which channels are on DACs)
   //       [2]: Chain 1 mask
   //       [3]: S/PDIF channel mask (sets which channels are on S/PDIF)
   //    r7 = output resolution mode flag ($RESOLUTION_MODE_16BIT or $RESOLUTION_MODE_24BIT)
   r3 = $interface_map_struc;
   r7 = M[$outputResolutionMode];
   call $multi_chan_port_scan_and_routing_config;

   // Configure the initial tone mixing ratios
   call $multi_chan_config_tone_mixing;

   // Configure the codec resampler according to the codec and DAC sampling rates and processing resolution mode
   r7 = M[$procResolutionMode];
   call $codec_resampler.config;

   // Configure the rate matching algorithm
   call $config_rate_matching;

   // Configure which buffers are to be used for rate adjustment and resampling
   call $codec_rate_adj.config_buffers;

   // Configure the rate adjustment quality
   call $codec_rate_adj.config_quality;

   // Channel complete so allow processing of VM messages
   call $unblock_interrupts;

   // Set the output handler timer according to whether ANC is configured
   call $set_output_handler_timer;

   // for 200ms, clear all incomming codec data
   // this causes to synchronise source and sink
   // and having consistent delay
   r7 = 200;
   loop_discard_all_codec_data:
      call $timer.1ms_delay;
      r0 = $CODEC_IN_PORT;
      call $cbuffer.calc_amount_data;
      r10 = r0;
      r0 = $CODEC_IN_PORT;
      call $cbuffer.get_read_address_and_size;
      I0 = r0;
      L0 = r1;
      do lp_read_port_data;
         r0 = M[I0, 1];
         nop;
      lp_read_port_data:
      L0=0;
      r0 = $CODEC_IN_PORT;
      r1 = I0;
      call $cbuffer.set_read_address;
      r7 = r7 -1;
   if GT jump loop_discard_all_codec_data;


   // start timer that copies codec data
   r1 = &$codec_timer_struc;
   r2 = M[$tmr_period_con_copy];
   r3 = &$codec_copy_handler;
   call $timer.schedule_event_in;

   // Start in stalled mode until data arrives
   r5 = &$decoder_codec_stream_struc;
   r0 = $codec.NOT_ENOUGH_INPUT_DATA;
   M[r5 + $codec.av_decode.MODE_FIELD] = r0;

   // start timer that copies tone samples
   r1 = &$tone_copy_timer_struc;
   r2 = M[$tmr_period_tone_copy];
   r3 = &$tone_copy_handler;
   call $timer.schedule_event_in;

   // post timer event for standby level detector
   r1 = &$signal_detect_timer_struc;
   r2 = SIGNAL_DETECT_TIMER_PERIOD;
   r3 = &$signal_detect_timer_handler;
   call $timer.schedule_event_in;

   // wait for DAC buffers to have just wrapped around
   wait_for_dac_buffer_wraparound:
      r0 = $MULTI_CHAN_PRIMARY_LEFT_OUT_PORT;
      call $cbuffer.calc_amount_space;
      // if the amount of space in the buffer is less than 16 bytes then a
      // buffer wrap around must have just ocurred.
      Null = r0 - 16;
   if POS jump wait_for_dac_buffer_wraparound;

   // start timer that copies audio samples
   r1 = &$audio_out_timer_struc;
   r2 = M[$tmr_period_audio_copy];
   r3 = &$audio_out_copy_handler;
   call $timer.schedule_event_in;

   // continually decode and encode codec frames
   frame_loop:

      // Check Communication
      call $spi_comm.polled_service_routine;

      // Check the connection type (A2DP or USB/ANALOGUE) and process accordingly
      r0 = M[$app_config.io];

      // Fatal error if not A2DP APTX_ACL_SPRINT_IO connection type
      null = r0 - $APTX_ACL_SPRINT_IO;
      if NZ jump $error;

      // Start profiler
      r8 = &$DecoderMips_data_block;
      call $M.mips_profile.mainstart;   // start profiler

      // decode a frame
      r5 = &$decoder_codec_stream_struc;
      call $codec.av_decode;

      // run resampler if needed
      Null = M[$codec_resampler.resampler_active];
      if NZ call $codec_resampler.run_resampler;

      // run sw rate adjustment if needed
      Null = M[$chain0_hw_warp_enable];
      if Z call $codec_rate_adj.run_rate_adjustment;

      call $sra_calcrate;
      #ifdef LATENCY_REPORTING
          Null = M[$M.configure_latency_reporting.enabled];
          if Z jump skip_latency_measurement;
              r7 = &$encoded_latency_struct;
              call $latency.calc_encoded_latency;
          skip_latency_measurement:
      #endif // LATENCY_REPORTING

skip_decode:

      // Stop profiler
      r8 = &$DecoderMips_data_block;
      call $M.mips_profile.mainend;

      // Store Decoder MIPS
      r0 = M[r8 + $mips_profile.MIPS.MAIN_CYCLES_OFFSET];
      M[&$music_example.PeakMipsDecoder] = r0;

      // Synchronize frame process to audio interrupt
      r5 = &$decoder_codec_stream_struc;
      r0 = M[r5 + $codec.av_decode.MODE_FIELD];
      Null = r0 - $codec.SUCCESS;
      if NZ call $SystemSleepAudio;

      // data/space check only on left in/out primary buffers. These buffers are
      // assumed always active. The system around music manager guarantees
      // synchronized outputs
      r3 = M[$M.codec_copy_handler.frame_proc_num_samples];

      r0 = M[$M.system_config.data.stream_map_left_in + 0];
      call $cbuffer.calc_amount_data;
      null = r0 - r3;
      if NEG jump frame_loop;

      r0 = M[$M.system_config.data.stream_map_primary_left_out + 0];
      call $cbuffer.calc_amount_space;
      null = r0 - r3;
      if NEG jump frame_loop;

      // call processing function if block-size worth of data/space available
      if POS call $music_example_process;

      // compute adjust rate to synchronise chain1 if it's active
      r8 = $chain1_to_chain0_pcm_sync_struct;
      Null = M[$M.multi_chan_output.num_chain1_channels];
      if NZ call $pcm_sync_calc_rate;

   jump frame_loop;

.ENDMODULE;

//------------------------------------------------------------------------------
.module $M.signal_detect_timer_handler;
//------------------------------------------------------------------------------
// timer handler to call signal detector processing every second
//------------------------------------------------------------------------------

    .codesegment pm;
    .datasegment dm;

    $signal_detect_timer_handler:

    // push rLink onto stack
    $push_rLink_macro;

    r8 = $M.multi_chan_output.signal_detect_coeffs;
    call $cbops.signal_detect_op.timer_handler;

    // post another timer event
    r1 = &$signal_detect_timer_struc;
    r2 = SIGNAL_DETECT_TIMER_PERIOD;
    r3 = &$signal_detect_timer_handler;
    call $timer.schedule_event_in_period;

    // pop rLink from stack
    jump $pop_rLink_and_rts;

.endmodule;

// *****************************************************************************
// MODULE:
//    $audio_out_copy_handler
//
// DESCRIPTION:
//    Function called on a timer interrupt to perform the mono or stereo copying
//    of decoded samples to the output.
//
// INPUTS:
//  - none
//
// OUTPUTS:
//  - none
//
// NOTES:
// *****************************************************************************
.MODULE $M.audio_out_copy_handler;
   .CODESEGMENT AUDIO_OUT_COPY_HANDLER_PM;
   .DATASEGMENT DM;

   .VAR $multi_chain0_copy_struc_ptr = $M.multi_chan_output.chain0_copy_struc;
   .VAR $multi_chain1_copy_struc_ptr = $M.multi_chan_output.chain1_copy_struc;

   $audio_out_copy_handler:

   // push rLink onto stack
   $push_rLink_macro;

   // If not using cbops.stream_copy, need to manually
   // reset the sync flag.
   M[$frame_sync.sync_flag] = Null;

   // Calculate the actual consuming rate of the output interface, for master outputs
   // this shall be the same as nominal output rate, but for slave mode this might
   // be slightly different. This routine allows the rate matching code to cope
   // with 'slave' output devices where the sample rate is driven by an external clock
   r8 = $calc_chain0_actual_port_rate_struc;
   call $calc_actual_port_rate;

   // adjust sra expected rate before resampler
   r0 = M[$calc_chain0_actual_port_rate_struc + $calc_actual_port_rate.SAMPLE_RATE_FIELD];
   if NZ call $apply_sra_resampling_adjustment;

   // calculate chain1 actual consuming rate
   r8 = $calc_chain1_actual_port_rate_struc;
   Null = M[$M.multi_chan_output.num_chain1_channels];
   if NZ call $calc_actual_port_rate;

   #ifdef LATENCY_REPORTING

      // set the warp value to be used for latency caluclation
      r8 = $codec_rate_adj.stereo + $cbops.rate_adjustment_and_shift.Process.SRA_CURRENT_RATE_FIELD;
      r7 = $hw_warp_struct + $hw_warp.CURRENT_RATE_FIELD;
      Null = M[$chain0_hw_warp_enable];
      if NZ r8 = r7;
      M[$latency_calc_current_warp] = r8;

   #endif //LATENCY_REPORTING

   // Clone the tone cbuffers to allow multiple sinks of tone data from the same tone data buffers
   call $multi_chan_clone_tone_cbuffers;

   // If there are enabled CHAIN0 channels, copy to output ports using multi-channel cbops chains
   r8 = $M.multi_chan_output.chain0_copy_struc;
   null = M[$M.multi_chan_output.num_chain0_channels];
   if NZ call $cbops.dac_av_copy_m;

   // If there are enabled chain1 channels, copy to output ports using multi-channel cbops chains
   r8 = $M.multi_chan_output.chain1_copy_struc;
   null = M[$M.multi_chan_output.num_chain1_channels];
   if NZ call $cbops.dac_av_copy_m;

   // Align all the tone cbuffers read pointers
   call $multi_chan_adjust_tone_cbuffers;

   // Post another timer event
   r1 = &$audio_out_timer_struc;
   r2 = M[$tmr_period_audio_copy];
   r3 = &$audio_out_copy_handler;
   call $timer.schedule_event_in_period;

   // pop rLink from stack
   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $codec_copy_handler
//
// DESCRIPTION:
//    Function called on a timer interrupt to perform the copying of encoded
//    samples from the input.
//    Includes code to handle 1 byte being stuck in the MMU which would otherwise
//    not be read and may cause flow control to be asserted indefinitely.
//
// INPUTS:
//  - none
//
// OUTPUTS:
//  - none
//
// NOTES:
// *****************************************************************************
.MODULE $M.codec_copy_handler;
   .CODESEGMENT CODEC_COPY_HANDLER_PM;
   .DATASEGMENT DM;
   .VAR packet_size_obtained;
   .VAR frame_proc_num_samples = 160;
   .VAR initial_write_pos = 0;

   $codec_copy_handler:

   // push rLink onto stack
   $push_rLink_macro;

   #ifdef LATENCY_REPORTING
        r0 = M[&$codec_in_cbuffer_struc + $cbuffer.WRITE_ADDR_FIELD];
        M[$codec_packets_info_last_write_address] = r0;
    #endif // LATENCY_REPORTING

   //calculate mismatch rate
   call $sra_tagtimes;

   .VAR byte0;
   .VAR byte1;
   .VAR byte0_flag;

   // Force an MMU buffer set
   null = M[$PORT_BUFFER_SET];

   r0 = M[&$codec_in_cbuffer_struc + $cbuffer.WRITE_ADDR_FIELD];
   M[initial_write_pos] = r0;

   // Get amount of data (in bytes) in input port
   r0 = $CODEC_IN_PORT;
   call $cbuffer.calc_amount_data;
   null = r1;
   if Z jump copy_done; // Don't do anything if no data available

   // Was there an odd byte (byte0) remaining on the last copy from the port
   null = M[byte0_flag];
   if Z jump no_odd_byte;

      // Get amount of space (in words) in output buffer
      r0 = $codec_in_cbuffer_struc;
      call $cbuffer.calc_amount_space;
      null = r0;
      if Z jump copy_done; // Don't do anything if no space available

      // Switch to 8bit read mode
      M[($CODEC_IN_PORT & $cbuffer.PORT_NUMBER_MASK) + $READ_PORT0_CONFIG] = $BITMODE_8BIT_ENUM;

      // Read the second byte to complete the 16bit word
      r0 = M[($CODEC_IN_PORT & $cbuffer.PORT_NUMBER_MASK) + $READ_PORT0_DATA];
      r0 = r0 AND 0xff;
      M[byte1] = r0;

      // Update input port
      r0 = $CODEC_IN_PORT;
      r1 = ($CODEC_IN_PORT & $cbuffer.PORT_NUMBER_MASK) + $READ_PORT0_DATA;
      call $cbuffer.set_read_address;

      r0 = $CODEC_IN_PORT;
      call $cbuffer.calc_amount_data;

      // Switch to 16bit read mode
      r0 = $BITMODE_16BIT_ENUM + $BYTESWAP_MASK + $NOSIGNEXT_MASK;
      M[($CODEC_IN_PORT & $cbuffer.PORT_NUMBER_MASK) + $READ_PORT0_CONFIG] = r0;

      // Get the write address for the output buffer
      r0 = $codec_in_cbuffer_struc;
      call $cbuffer.get_write_address_and_size;
      I0 = r0;
      L0 = r1;

      // Combine bytes to form 16bit word to be written
      r0 = M[byte1];
      r1 = M[byte0];
      r1 = r1 LSHIFT 8;
      r0 = r1 OR r0;

      // Write the word of data (byte1 OR byte0)
      M[I0,1] = r0;

      // Reset for linear addressing
      L0 = 0;

      // Update output buffer
      r0 = $codec_in_cbuffer_struc;
      r1 = I0;
      call $cbuffer.set_write_address;

      // Clear the flag
      M[byte0_flag] = 0;

   no_odd_byte:


   // copy data from the port to the cbuffer
   r8 = &$codec_in_copy_struc;
   call $cbops.copy;

   // Force an MMU buffer set
   null = M[$PORT_BUFFER_SET];

   // Get amount of data (in bytes) in input port
   r0 = $CODEC_IN_PORT;
   call $cbuffer.calc_amount_data;

   // Only 1 byte left?
   null = r1 - 1;
   if NZ jump skip_odd_byte;

      // Switch to 8bit read mode
      M[($CODEC_IN_PORT & $cbuffer.PORT_NUMBER_MASK) + $READ_PORT0_CONFIG] = $BITMODE_8BIT_ENUM;

      // Read the odd byte (byte0)
      r0 = M[($CODEC_IN_PORT & $cbuffer.PORT_NUMBER_MASK) + $READ_PORT0_DATA];
      r0 = r0 AND 0xff;
      M[byte0] = r0;

      // Update input port (this lets the firmware send more data)
      r0 = $CODEC_IN_PORT;
      r1 = ($CODEC_IN_PORT & $cbuffer.PORT_NUMBER_MASK) + $READ_PORT0_DATA;
      call $cbuffer.set_read_address;

      // Switch to 16bit read mode
      r0 = $BITMODE_16BIT_ENUM + $BYTESWAP_MASK + $NOSIGNEXT_MASK;
      M[($CODEC_IN_PORT & $cbuffer.PORT_NUMBER_MASK) + $READ_PORT0_CONFIG] = r0;

      // Flag an odd byte (byte0) has been read
      r0 = 1;
      M[byte0_flag] = r0;

   skip_odd_byte:
   copy_done:

   // apply hw rate matching
   r0 = M[$chain0_hw_warp_enable];
   r0 = r0 + M[$chain1_hw_warp_enable];
   if NZ call $apply_hardware_warp_rate;

// DEBUG stats
#ifdef DEBUG_STATS
#define SAMPLE_SIZE 1000
.VAR $buffer_post_copy_data = 0;
.VAR $buff_post_copy_ptr = &$buff_post_copy_buffer;
.VAR/DMCIRC $buff_post_copy_buffer[SAMPLE_SIZE];
.VAR $codec_in_buffer_empty_count = 0;


r0 = $codec_in_cbuffer_struc;
call $cbuffer.calc_amount_data;

M[$buffer_post_copy_data] = r0;

Null = r0;
if NZ jump not_empty;
r0 = M[$codec_in_buffer_empty_count];
r0 = r0 + 1;
M[$codec_in_buffer_empty_count] = r0;
not_empty:

// Store last 100 values of fill level
r5 = M[$buffer_post_copy_data];
r0 = M[$buff_post_copy_ptr];
I0 = r0;
L0 = SAMPLE_SIZE;
M[I0,1] = r5;
r0 = I0;
M[$buff_post_copy_ptr] = r0;
L0 = 0;
#endif

    #ifdef LATENCY_REPORTING
        Null = M[$M.configure_latency_reporting.enabled];
        if Z jump skip_packet_detection;
            call $media_packet_boundary_detection;
        skip_packet_detection:
    #endif // LATENCY_REPORTING

   r0 = M[&$codec_in_cbuffer_struc + $cbuffer.WRITE_ADDR_FIELD];
   r0 = r0 - M[initial_write_pos];
   if Z jump packet_length_obtained;
   Null = M[packet_size_obtained];
   if NZ jump packet_length_obtained;
      r1 = 0;
      r2 = 150;
      Null = r0 - 300;
      if Z r1 = r2;
      Null = r0 - 150;
      if Z r1 = r2;
      r2 = 138;
      Null = r0 - 276;
      if Z r1 = r2;
      Null = r0 - 138;
      if Z r1 = r2;
      M[packet_size_obtained] = r1;
      Null = M[packet_size_obtained];
      if Z jump packet_length_obtained;
      M[frame_proc_num_samples] = r1;
      M[$M.system_config.data.stream_map_left_in + $framesync_ind.FRAME_SIZE_FIELD] = r1;
      M[$M.system_config.data.stream_map_right_in + $framesync_ind.FRAME_SIZE_FIELD] = r1;
      M[$M.system_config.data.stream_map_primary_left_out + $framesync_ind.FRAME_SIZE_FIELD] = r1;
      M[$M.system_config.data.stream_map_primary_right_out + $framesync_ind.FRAME_SIZE_FIELD] = r1;

   packet_length_obtained:

   // post another timer event
   r1 = &$codec_timer_struc;
   r2 = M[$tmr_period_con_copy];
   r3 = &$codec_copy_handler;
   call $timer.schedule_event_in_period;

   // pop rLink from stack
   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $get_aptx_ll_params_from_vm1
//
// DESCRIPTION: message handler for receiving aptX ll parameters from VM
//
// INPUTS:
//    r1 = Target Codec Level
//    r2 = Initial Codec Level
//    r3 = SRA Max Rate
//    r4 = SRA averaging time
//
// OUTPUTS:
//  - none
//
// NOTES:
// *****************************************************************************
.MODULE $M.get_aptx_ll_params_from_vm1;
   .CODESEGMENT GET_APTX_LL_PARAMS_FROM_VM1_PM;
   .DATASEGMENT DM;

$get_aptx_ll_params_from_vm1:

   M[$target_codec_level]  = r1;
   M[$sra_averaging_time]  = r4;

   rMac = r3;
   r0 = 20000;     // Divide by 10000 in rMac
   Div = rMac/r0;
   r3 = DivResult;
   M[$sra_max_rate] = r3;

   rts;
.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $get_aptx_ll_params_from_vm2
//
// DESCRIPTION: message handler for receiving aptX ll parameters from VM
//
// INPUTS:
//    r1 = Good working Level
//
// OUTPUTS:
//  - none
//
// NOTES:
// *****************************************************************************
.MODULE $M.get_aptx_ll_params_from_vm2;
   .CODESEGMENT GET_APTX_LL_PARAMS_FROM_VM2_PM;
   .DATASEGMENT DM;

$get_aptx_ll_params_from_vm2:

   rMac = r1;
   if Z jump default_good_level;
   r0 = 2 * CODEC_CBUFFER_SIZE; // Divide by CBUFFER_SIZE to get as a fraction
   Div = rMac/r0;
   r1 = DivResult;
   jump updated_good_level;
   default_good_level:
   r1 = GOOD_WORKING_BUFFER_LEVEL;
   updated_good_level:
   M[$good_working_level] = r1;

   rts;
.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $set_dac_rate_from_vm
//
// DESCRIPTION: message handler for receiving DAC rate from VM
//
// INPUTS:
//  r1 = dac sampling rate/10 (e.g. 44100Hz is given by r1=4410)
//  r2 = maximum clock mismatch to compensate (r2/10)%
//       (Bit7==1 disables the rate control, e.g 0x80)
//  r3 = bit0: if long term mismatch rate saved bits(15:1): saved_rate>>5
//  [r4 = bits(1:0): DEPRECATED in multi-channel implementation
//                  audio output interface type:
//                  0 -> None (not expected)
//                  1 -> Analogue output (DAC)
//                  2 -> I2S output
//                  3 -> SPDIF output]
//
//  r4 = bit8: playback mode (0: remote playback, 1: local file play back)
//             local play back isn't relevant in this app and shall not be used
//
// OUTPUTS:
//  - none
//
// NOTES:
// *****************************************************************************
.MODULE $M.set_dac_rate_from_vm;
   .CODESEGMENT SET_DAC_RATE_FROM_VM_PM;

$set_dac_rate_from_vm:

   // Mask sign extension
   r1 = r1 AND 0xffff;

   // Scale to get sampling rate in Hz
   r1 = r1 * 10 (int);

   // Store the parameters
   M[$current_dac_sampling_rate] = r1;                // DAC sampling rate (e.g. 44100Hz is given by r1=44100)
   M[$max_clock_mismatch] = r2;                       // Maximum clock mismatch to compensate (r2/10)% (Bit7==1 disables the rate control, e.g 0x80)
   M[$long_term_mismatch] = r3;                       // bit0: if long term mismatch rate saved bits(15:1): saved_rate>>5
   r0 = r4 AND $LOCAL_PLAYBACK_MASK;                  // Mask for local file play back info
   M[$local_play_back] = r0;                          // NZ means local file play back (local file play back not used here)

   // update inverse of dac sample rate
   push rLink;
   r0 = M[$current_dac_sampling_rate];
   call $latency.calc_inv_fs;
   M[$inv_dac_fs] = r0;
   pop rLink;

   rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $set_codec_rate_from_vm
//
// DESCRIPTION: message handler for receiving codec rate from VM
//
// INPUTS:
//  r1 = forward channel codec sampling rate/10 (e.g. music @44100Hz is given by r1=4410)
//  r2 = back channel codec sampling rate/10 (e.g. voice @16000Hz is given by r2=1600)
//
// OUTPUTS:
//  - none
//
// NOTES:
// *****************************************************************************
.MODULE $M.set_codec_rate_from_vm;
   .CODESEGMENT SET_CODEC_RATE_FROM_VM_PM;

$set_codec_rate_from_vm:

   // Mask sign extension
   r1 = r1 AND 0xffff;

   // Scale to get sampling rate in Hz
   r1 = r1 * 10 (int);

   // Mask sign extension
   r2 = r2 AND 0xffff;

   // Scale to get sampling rate in Hz
   r2 = r2 * 10 (int);

   // Store the music forward channel codec sampling rate
   M[$current_codec_sampling_rate] = r1;

   // update inverse of codec sample rate
   push rLink;
   r0 = M[$current_codec_sampling_rate];
   call $latency.calc_inv_fs;
   M[$inv_codec_fs] = r0;
   pop rLink;

   rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $config_rate_matching
//
// DESCRIPTION: Configure the rate matching algorithm
//
// INPUTS:
//  - none
//
// OUTPUTS:
//  - none
//
// NOTES:
//    The DAC and codec sampling rates need to be set before calling this routine
//    The VM should send 2 messages that should result in the following handlers being
//    called: $set_dac_rate_from_vm $set_codec_rate_from_vm.
// *****************************************************************************
.MODULE $M.config_rate_matching;
   .CODESEGMENT CONFIG_RATE_MATCHING_PM;

$config_rate_matching:

   // push rLink onto stack
   $push_rLink_macro;

   // Get the dac sampling rate
   r1 = M[$current_dac_sampling_rate];     // Dac sampling rate (e.g. 44100Hz is given by r1=44100)

   r1 = r1 * SRA_AVERAGING_TIME (int);
   M[$sra_struct + $sra.AUDIO_AMOUNT_EXPECTED_FIELD] = r1;

   // Check if the rate matching is disabled (bit7==1 disables the rate matching e.g. r2=0x80)
   r2 = M[$max_clock_mismatch];              // Maximum clock mismatch to compensate (r2/10)% (Bit7==1 disables the rate control, e.g 0x80)
   r0 = r2 LSHIFT -7;
   r0 = r0 AND 1;
   M[&$rate_match_disable] = r0; // Enable: 0, Disable: 1

   // Local playback?
   r4 = M[$local_play_back];                 // NZ means local file play back
   if NZ jump $error;                        // Local playback not supported in this app.

   // If rate matching is disabled don't update rate
   null = M[&$rate_match_disable];
   if NZ jump end;

   // set maximum rate for clock mismatch compensation
   r2 = r2 AND 0x7F;
   r1 = r2 - 3;         // min 0.3% percent by default
   if NEG r2 = r2 -r1;
   r2 = r2 * 0.001(int);
   r1 = r2 * 0.25(frac); // to cover jitter
   r2 = r2 + r1;
   r1 = r2 - ABSOLUTE_MAXIMUM_CLOCK_MISMATCH_COMPENSATION;
   if POS r2 = r2 - r1;
   M[$sra_struct + $sra.MAX_RATE_FIELD] = r2;
   r2 = 0.5; // just a big number
   M[$sra_struct + $sra.LONG_TERM_RATE_FIELD] = r2;

   // see if clock mismatch rate received from vm
   r3 = M[$long_term_mismatch];              // bit0: if long term mismatch rate saved bits(15:1): saved_rate>>5
   r0 = r3 AND 0x1;
   if Z jump end;

   // get saved clock mismatch rate
   r3 = r3 ASHIFT -1;
   r3 = r3 ASHIFT 6;

   // make sure it is not out of range
   Null = r3 - M[$sra_struct + $sra.MAX_RATE_FIELD];
   if POS jump end;
   Null = r3 + M[$sra_struct + $sra.MAX_RATE_FIELD];
   if NEG jump end;

   // initialize some variables based on the saved rate
   M[$sra_struct + $sra.RATE_BEFORE_FIX_FIELD ] = r3;
   M[$sra_struct + $sra.SRA_RATE_FIELD ] = r3;
   r0 = M[$sra_struct + $sra.AUDIO_AMOUNT_EXPECTED_FIELD];
   r0 = r0 * r3 (frac);
   M[$sra_struct + $sra.HIST_BUFF_FIELD + 0] = r0;
   M[$sra_struct + $sra.HIST_BUFF_FIELD + 1] = r0;
   r0 = 2;
   M[$sra_struct + $sra.HIST_INDEX_FIELD] = r0;
   r0 = 1;
   M[$sra_struct + $sra.LONG_TERM_RATE_DETECTED_FIELD] = r0;

   Null = M[$chain0_hw_warp_enable];
   if Z jump end;
      // Initial hardware rate adjustment
      M[$hw_warp_struct + $hw_warp.CURRENT_RATE_FIELD] = r3;
      r4 = r3 * (-1.0/64.0)(frac);
      r2 = &$MESSAGE_WARP_DAC;
      r3 = 3;
      call $message.send_short;

   end:

   // pop rLink from stack
   jump $pop_rLink_and_rts;

.ENDMODULE;
// *****************************************************************************
// MODULE:
//    $M.copy_codec_stats_pointers
//
// DESCRIPTION:
//    Helper function to copy the codec specific stats pointers into the
//    music_example statistics pointer table
//
// INPUTS:
//    I0 = pointer to codec specific stats pointer table
//
// OUTPUTS:
//    none
//
// TRASHES: r0, r10, I0, I4
//
// *****************************************************************************
.MODULE $M.copy_codec_stats_pointers;
   .CODESEGMENT COPY_CODEC_STATS_POINTERS_PM;

$copy_codec_stats_pointers:

   // Length of the codec specific statistics pointer table
   r10 = $music_example.CODEC_STATS_SIZE;

   // Start of the codec statistics part of the table
   I4 = &$M.system_config.data.StatisticsPtrs + $M.MUSIC_MANAGER.STATUS.CODEC_FS_OFFSET;

   do assign_loop;

      // Copy over the stats pointer
      r0 = M[I0, 1];
      M[I4, 1] = r0;

   assign_loop:

   rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.conn_init_cbuffers
//
// DESCRIPTION:
//    Helper function to set up the cbuffers with given sizes
//    (note: the data buffer must start at an address that
//    is appropriate for the size of cbuffer requested)
//
// INPUTS:
//    r3 = Audio cbuffer size
//    r4 = Codec cbuffer size
//
// OUTPUTS:
//    none
//
// TRASHES: r0, r1, r2
//
// *****************************************************************************
.MODULE $M.conn_init_cbuffers;
   .CODESEGMENT PM;

$conn_init_cbuffers:

   $push_rLink_macro;

   // Set up the cbuffers specifically for the codec type
   r0 = $audio_out_left_cbuffer_struc;
   r1 = $audio_out_left;
   r2 = r3;
   call $cbuffer.buffer_configure;

   r0 = $audio_out_right_cbuffer_struc;
   r1 = $audio_out_right;
   r2 = r3;
   call $cbuffer.buffer_configure;

   r0 = $codec_in_cbuffer_struc;
   r1 = $codec_in;
   r2 = r4;
   call $cbuffer.buffer_configure;

   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    conn_init
//
// DESCRIPTION:
//    Helper routine to allow run-time selection of USB/A2DP operation
//
//    Sets up the cbuffer sizes according to the codec/USB
//
//    Sets the inputs in the Rate Adaptation (SRA) structure:
//
//    $sra.TAG_DURATION_FIELD                     0;    //input: duration of the rate calc (in number of interrupts)
//    $sra.CODEC_PORT_FIELD                       1;    //input: codec input port to check activity
//    $sra.CODEC_CBUFFER_TO_TAG_FIELD             2;    //input: codec input cbuffer to tag the times
//    $sra.AUDIO_CBUFFER_TO_TAG_FIELD             3;    //input: audio output cbuffer to count PCM samples
//    $sra.MAX_RATE_FIELD                         4;    //input: maximum possible rate adjustment
//    $sra.AUDIO_AMOUNT_EXPECTED_FIELD            5;    //input: amount of PCM sample expected to receive in one period (FS*TAG_DURATION_FIELD*interrupt_time)
//
//    Override the statistics table pointers
//
//    Initialize the USB/codec timer and codec_type
//
// INPUTS:
//    none
//
// OUTPUTS:
//    none
//
// TRASHES: r0, r1, r2, r3, r4, r10, I0 and those trashed by the codec initialise functions
//
// *****************************************************************************
.MODULE $M.conn_init;

   .CODESEGMENT PM;

$conn_init:

   $push_rLink_macro;

   // Set up the cbuffers for the codec
   r3 = AUDIO_CBUFFER_SIZE;
   r4 = CODEC_CBUFFER_SIZE;
   call $conn_init_cbuffers;

   // Get the connection type
   r0 = M[$app_config.io];

   // Check the connection type (only A2DP APTX_ACL_SPRINT allowed here) and initialize accordingly
   null = r0 - $APTX_ACL_SPRINT_IO;
   if NZ jump $error;

      // ------------------------------------------------------------------------
      // Set up the Rate Adaptation for A2DP

      #define TAG_DURATION_SCALE (1000000/TMR_PERIOD_CODEC_COPY)

      r0 = M[$sra_averaging_time];
      r0 = r0 * TAG_DURATION_SCALE (int);
      M[$sra_struct + $sra.TAG_DURATION_FIELD] = r0;
      r0 = $CODEC_IN_PORT;
      M[$sra_struct + $sra.CODEC_PORT_FIELD] = r0;
      r0 = &$codec_in_cbuffer_struc;
      M[$sra_struct + $sra.CODEC_CBUFFER_TO_TAG_FIELD] = r0;
      r0 = &$multi_chan_primary_left_out_cbuffer_struc;
      M[$sra_struct + $sra.AUDIO_CBUFFER_TO_TAG_FIELD] = r0;
      r0 = M[$sra_max_rate];
      M[$sra_struct + $sra.MAX_RATE_FIELD] = r0;

      r1 = M[$current_dac_sampling_rate];
      r0 = M[$sra_averaging_time];
      r0 = r1 * r0 (int); // Fs x SRA_AVERAGING_TIME
      // Number of samples collected over averaging time
      M[$sra_struct + $sra.AUDIO_AMOUNT_EXPECTED_FIELD] = r0;

      r0 = M[$good_working_level];
      M[$decoder_codec_stream_struc + $codec.stream_decode.GOOD_WORKING_BUFLEVEL_FIELD] = r0;

      r1 = M[$current_codec_sampling_rate];

      // Target buffer level is dependent on the codec sampling rate
      null = r1 - 44100;
      if NZ jump not_44100;

         // 44.1kHz codec operation

         // Set the target level
         r2 = M[$target_codec_level];
         r2 = r2 + 20;                // Add a fraction more to target (0.02)
         r0 = 2 * CODEC_CBUFFER_SIZE; // Divide by CBUFFER_SIZE
         rMac = r2;
         Div = rMac/r0;
         r0 = DivResult;
         M[$sra_struct + $sra.TARGET_LEVEL_FIELD] = r0;

         jump sra_conf_done;

      not_44100:

      null = r1 - 48000;
      if NZ jump not_48000;

         // 48kHz codec operation

         // Set the target level
         r2 = M[$target_codec_level];
         r2 = r2 + 20;                // Add a fraction more to target (0.02)
         r0 = 2 * CODEC_CBUFFER_SIZE; // Divide by CBUFFER_SIZE
         rMac = r2;
         Div = rMac/r0;
         r0 = DivResult;
         r1 = r1 * 0.0884 (frac);
         M[$sra_struct + $sra.TARGET_LEVEL_FIELD] = r0 + r1;

         jump sra_conf_done;

      not_48000:

      // Unsupported codec rate (must be 44.1kHz or 48kHz)
      jump $error;

      sra_conf_done:

      // ------------------------------------------------------------------------
      // Initialize the codec timer and codec type for A2DP

      // Initialize the timer period
      r0 = TMR_PERIOD_CODEC_COPY;
      M[$tmr_period_con_copy] = r0;

      // ------------------------------------------------------------------------
      // Codec specific initialisation

      // Get the connection type
      r0 = M[$app_config.io];

#ifdef APTX_ACL_SPRINT_ENABLE
      // Check the codec type and initialize accordingly
      null = r0 - $APTX_ACL_SPRINT_IO;
      if NZ jump skip_aptx_acl_sprint;

         // Set up the decoder structure for APTX_ACL_SPRINT
         r0 = $aptx_sprint.decode;
         M[$decoder_codec_stream_struc + $codec.av_decode.ADDR_FIELD] = r0;
         r0 = $aptx_sprint.reset_decoder;
         M[$decoder_codec_stream_struc + $codec.av_decode.RESET_ADDR_FIELD] = r0;
         r0 = $aptx_sprint.silence_decoder;
         M[$decoder_codec_stream_struc + $codec.av_decode.SILENCE_ADDR_FIELD] = r0;

         // Initialise the APTX_ACL_SPRINT decoder library
         call $aptx_sprint.init_decoder;

         // Initialise the statistics pointer table
         I0 = $codec_stats;
         call $copy_codec_stats_pointers;

         jump exit;

      skip_aptx_acl_sprint:
#endif

      // Unknown codec
      jump $error;

   exit:

   jump $pop_rLink_and_rts;
.ENDMODULE;


.MODULE $M.master_app_reset;
   .CODESEGMENT MASTER_APP_RESET_PM;
   .DATASEGMENT DM;

   $master_app_reset:

   // push rLink onto stack
   $push_rLink_macro;

   // local play back?
   Null = M[$local_play_back];
   if Z jump pause_happened;

   // notify VM about end of play_back
   r2 = PLAY_BACK_FINISHED_MSG;
   r3 = 0;
   r4 = 0;
   r5 = 0;
   r6 = 0;
   call $message.send_short;
   pause_happened:
   call $block_interrupts;

   // Purge the wired output multi-channel cbuffers
   call $multi_chan_purge_buffers;

   // Purge the buffers ahead of Frame Processing
   call $purge_input_pcm_cbuffers;

   // Re-initialise audio processing (copies MM params to modules and calls MM initialisation functions)
   call $music_example_reinitialize;

   // re-initialise delay lines
   r4 = &$M.system_config.data.delay_reinitialize_table;
   call $frame_sync.run_function_table;

   // Clear EQ delay memories
   r4 = &$M.system_config.data.filter_reset_table;
   call $frame_sync.run_function_table;

    #ifdef LATENCY_REPORTING
        r7 = &$encoded_latency_struct;
        call $latency.reset_encoded_latency;
        M[$first_packet_received] = 0;
    #endif // LATENCY_REPORTING

   call $unblock_interrupts;
   // pop rLink from stack
   jump $pop_rLink_and_rts;
.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.purge_input_pcm_cbuffers
//
// DESCRIPTION:
//    Purge (empty) the input PCM cbuffers
//
// INPUTS:
//    none
//
// OUTPUTS:
//    none
//
// TRASHES: r0
//
// *****************************************************************************
.MODULE $M.purge_input_pcm_cbuffers;
   .CODESEGMENT PURGE_INPUT_PCM_CBUFFERS_PM;

   $purge_input_pcm_cbuffers:

   // Push rLink onto stack
   $push_rLink_macro;

   // clear left input buffer
   r0 = M[$audio_out_left_cbuffer_struc + $cbuffer.WRITE_ADDR_FIELD];
   M[$audio_out_left_cbuffer_struc + $cbuffer.READ_ADDR_FIELD] = r0;

   // clear right input buffer
   r0 = M[$audio_out_right_cbuffer_struc + $cbuffer.WRITE_ADDR_FIELD];
   M[$audio_out_right_cbuffer_struc + $cbuffer.READ_ADDR_FIELD] = r0;

   // clear left resampler output buffer
   r0 = M[$codec_resamp_out_left_cbuffer_struc + $cbuffer.WRITE_ADDR_FIELD];
   M[$codec_resamp_out_left_cbuffer_struc + $cbuffer.READ_ADDR_FIELD] = r0;

   // clear right resampler output buffer
   r0 = M[$codec_resamp_out_right_cbuffer_struc + $cbuffer.WRITE_ADDR_FIELD];
   M[$codec_resamp_out_right_cbuffer_struc + $cbuffer.READ_ADDR_FIELD] = r0;

   // clear left rate adjustment output buffer
   r0 = M[$codec_rate_adj_out_left_cbuffer_struc + $cbuffer.WRITE_ADDR_FIELD];
   M[$codec_rate_adj_out_left_cbuffer_struc + $cbuffer.READ_ADDR_FIELD] = r0;

   // clear right rate adjustment output buffer
   r0 = M[$codec_rate_adj_out_right_cbuffer_struc + $cbuffer.WRITE_ADDR_FIELD];
   M[$codec_rate_adj_out_right_cbuffer_struc + $cbuffer.READ_ADDR_FIELD] = r0;

   // Clear the rate adjustment history buffers
   call $codec_rate_adj.clear_history_buffers;

   // Pop rLink from stack
   jump $pop_rLink_and_rts;

.ENDMODULE;
