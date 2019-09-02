// *****************************************************************************
// Copyright (c) 2003 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
//
// *****************************************************************************

// *****************************************************************************
// DESCRIPTION
//   This is the main file which provides support for the following:
//    - Start up sequence and configuration
//    - Steady state frame based processing for
//        - Decoding (SBC, APTX, AAC,MP3)
//        - Sample rate adjustment
//        - Resampling
//        - Latency reporting
//        - Relay TWS or ShareMe data to slave device
//        - Relay Soundbar data to wireless subwoofer
//    - Interrupt copy handlers to transfer
//        - Encoded data into the DSP
//        - PCM data to output peripherals
//        - Tones and voice prompts to be mixed in the DSP
//        - Signal detect informaion to the VM
//
// *****************************************************************************
#include "core_library.h"
#include "cbops_library.h"
#include "codec_library.h"
#include "frame_sync_library.h"
#include "music_example.h"
#include "spi_comm_library.h"
#include "sr_adjustment.h"
#include "codec_library.h"
#include "codec_decoder.h"
#include "audio_proc_library.h"
#include "multichannel_output.h"
#include "multichannel_output_macros.h"
#include "frame_sync_stream_macros.h"

#ifdef USB_ENABLE
   #include "usbio.h"
#endif

#if defined(SHAREME_ENABLE) || defined(TWS_ENABLE)
   #include "relay_conn.h"
#endif

#ifdef SBC_ENABLE
   #include "sbc_library.h"
#endif

#ifdef MP3_ENABLE
   #include "mp3_library.h"
   #include "mp3.h"
#endif

#ifdef AAC_ENABLE
   #include "aac_library.h"
   #include "aac.h"

   // Required for using the fft library
   #define FFT_TWIDDLE_NEED_512_POINT
   #include "fft_twiddle.h"
#endif

// Plugin type set from VM at run-time
.MODULE $app_config;
   .DATASEGMENT DM;

   .VAR io = $INVALID_IO;
.ENDMODULE;

.MODULE $M.main;
   .CODESEGMENT MAIN_PM;
   .DATASEGMENT DM;

$main:

// Macro to find the maximum value of two expressions
#define        MAX_MACRO(x,y)             ((x)>(y)?(x):(y))

   // Allocate memory for cbuffer structures
   DeclareCBuffer($codec_in_cbuffer_struc,$codec_in,CODEC_CBUFFER_SIZE);
   DeclareCBuffer($audio_out_left_cbuffer_struc,$audio_out_left,MAX_MACRO(AUDIO_CBUFFER_SIZE, USB_AUDIO_CBUFFER_SIZE));
   DeclareCBuffer($audio_out_right_cbuffer_struc,$audio_out_right,MAX_MACRO(AUDIO_CBUFFER_SIZE, USB_AUDIO_CBUFFER_SIZE));

   // ** allocate memory for timer structures **
   .VAR $con_in_timer_struc[$timer.STRUC_SIZE];
   .VAR $audio_out_timer_struc[$timer.STRUC_SIZE];
   .VAR $signal_detect_timer_struc[$timer.STRUC_SIZE];

// In order to share memory between the decoders use build libraries with the external memory
// option selected and enable application static allocation below
#ifdef MP3_USE_EXTERNAL_MEMORY
   .VAR $mp3_ext_memory_ptrs[$mp3dec.mem.NUM_FIELDS];
#endif

   // Variables to receive dac and codec sampling rates from the vm
   .VAR $current_dac_sampling_rate = 0;                              // Dac sample rate, set by message from VM
   .VAR $set_dac_rate_from_vm_message_struc[$message.STRUC_SIZE];    // Message structure for VM_SET_DAC_RATE_MESSAGE_ID message
   .VAR $current_codec_sampling_rate = 0;                            // codec data sample rate, set by vm
   .VAR $set_codec_rate_from_vm_message_struc[$message.STRUC_SIZE];  // Message structure for VM_SET_CODEC_RATE_MESSAGE_ID message

   // Rate matching control variables
   .VAR $local_encoded_play_back;   // local encoded(sbc, mp3 or aac) file is being played
   .VAR $aux_input_stream_available;       // local pcm file is being mixed
   .VAR $audio_if_mode; // output interface type
   .VAR $max_clock_mismatch;
   .VAR $long_term_mismatch;

  // Structure to measure output port consuming rate
   .VAR $calc_chain0_actual_port_rate_struc[$calc_actual_port_rate.STRUC_SIZE] =
      $MULTI_CHAN_PRIMARY_LEFT_OUT_PORT, // PORT_FIELD
      0 ...;

   // ** allocate memory for codec input cbops copy routine **
   .VAR $codec_in_copy_struc[] =
      &$codec_in_copy_op,              // first operator block
      1,                               // number of inputs
      $CON_IN_PORT,                    // input
      1,                               // number of outputs
      &$codec_in_cbuffer_struc;        // output

   .BLOCK $codec_in_copy_op;
      .VAR $codec_in_copy_op.next = $cbops.NO_MORE_OPERATORS;
      .VAR $codec_in_copy_op.func = &$cbops.copy_op;
      .VAR $codec_in_copy_op.param[$cbops.copy_op.STRUC_SIZE] =
         0,                            // Input index
         1;                            // Output index
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
#if defined(MP3_ENABLE) && defined(TWS_ENABLE)
      &$tws.bin_headers,
#else
      0,
#endif
      0,                                           // DECODE_TO_STALL_THRESHOLD
      $STALL_TO_DECODE_THRESHOLD,                   // STALL_TO_DECODE_THRESHOLD ( in usec)
      GOOD_WORKING_BUFFER_LEVEL,                   // STALL_BUFFER_LEVEL_FIELD
      0,                                           // NORMAL_BUFFER_LEVEL , POORLINK_DETECT_LEVEL - no longer used
      1,                                           // Enable codec in buffer purge when in pause
      &$master_app_reset,
      0 ...;                                       // Pad out remaining items with zeros

   // allocate meory for inverse of dac and codec sample rates
   .VAR $inv_codec_fs = $latency.INV_FS(48000);
   .VAR $inv_dac_fs = $latency.INV_FS(48000);

#if defined(WIRELESS_SUB_ENABLE) || defined(LATENCY_REPORTING) || defined(TWS_ENABLE)
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
#endif

#ifdef LATENCY_REPORTING
   // define encoded latency structure
   .VAR $encoded_latency_struct[$encoded_latency.STRUC_SIZE] =
      $pcm_latency_input_struct,
      &$codec_packets_info_cbuffer_struc,
      $codec_in_cbuffer_struc,
      &$audio_out_left_cbuffer_struc, &$inv_codec_fs, &$latency_calc_current_warp,
      3, // minimum over last three
      0 ...;
    // define cbuffers structure involved in pcm latency
#endif

   // This is the codec type being used
   .VAR $codec_type = -1;

   // This is the codec config being used
   .VAR $codec_config = -1;

   // Timer period variable for reading codec data
   // (this is modified according to the connection type and also to support local playback)
   .VAR $tmr_period_con_copy;

   // Output handler timer period (for writing output data)
   // (this is modified according to whether ANC is used)
   .VAR $tmr_period_audio_copy;

   // Rate matching data structure
   .VAR $sra_struct[$sra.STRUC_SIZE];
   .VAR $rate_match_disable = 0;
   .VAR $sra_rate_addr = $sra_struct + $sra.SRA_RATE_FIELD;
   // Statistics pointer tables (these pointers are copied to the statistics pointer array by the conn_init function)
   // (this fixed size array contains pointers to codec specific information)

#ifdef SBC_ENABLE
   .VAR $sbc_stats[$music_example.CODEC_STATS_SIZE] =
      &$sbc.sbc_common_data_array + $sbc.mem.SAMPLING_FREQ_FIELD,
      &$sbc.sbc_common_data_array + $sbc.mem.CHANNEL_MODE_FIELD,
      &$sbc.sbc_common_data_array + $sbc.mem.BITPOOL_FIELD,
      &$sbc.sbc_common_data_array + $sbc.mem.NROF_BLOCKS_FIELD,
      &$sbc.sbc_common_data_array + $sbc.mem.NROF_CHANNELS_FIELD,
      &$sbc.sbc_common_data_array + $sbc.mem.NROF_SUBBANDS_FIELD,
      &$sbc.sbc_common_data_array + $sbc.mem.ALLOCATION_METHOD_FIELD,
      &$M.system_config.data.ZeroValue,                        // CODEC_STAT6
      &$M.system_config.data.ZeroValue,                        // CODEC_STAT7
      &$M.system_config.data.ZeroValue,                        // CODEC_STAT8
      &$M.system_config.data.ZeroValue,                        // CODEC_STAT9
      &$M.system_config.data.ZeroValue,                        // CODEC_STATA
      &$M.system_config.data.ZeroValue;                        // CODEC_STATB

#endif

#ifdef MP3_ENABLE
   .VAR $mp3_stats[$music_example.CODEC_STATS_SIZE] =
      &$mp3dec.sampling_freq,                                  //
      &$mp3dec.mode,                                           //
      &$mp3dec.framelength,                                    // CODEC_STAT1
      &$mp3dec.bitrate,                                        // CODEC_STAT2
      &$mp3dec.frame_version,                                  // CODEC_STAT3
      &$mp3dec.frame_layer,                                    // CODEC_STAT4
      &$M.system_config.data.ZeroValue,                        // CODEC_STAT5
      &$M.system_config.data.ZeroValue,                        // CODEC_STAT6
      &$M.system_config.data.ZeroValue,                        // CODEC_STAT7
      &$M.system_config.data.ZeroValue,                        // CODEC_STAT8
      &$M.system_config.data.ZeroValue,                        // CODEC_STAT9
      &$M.system_config.data.ZeroValue,                        // CODEC_STATA
      &$M.system_config.data.ZeroValue;                        // CODEC_STATB

#endif

#if defined(APTX_ENABLE)
   .VAR $aptx_stats[$music_example.CODEC_STATS_SIZE] =
      &$music_example.SamplingRate,                            //
      &$music_example.aptx_channel_mode,                       //
      &$music_example.aptx_security_status,                    // CODEC_STAT1
      &$music_example.aptx_decoder_version,                    // CODEC_STAT2
      &$M.system_config.data.ZeroValue,                        // CODEC_STAT3
      &$M.system_config.data.ZeroValue,                        // CODEC_STAT4
      &$M.system_config.data.ZeroValue,                        // CODEC_STAT5
      &$M.system_config.data.ZeroValue,                        // CODEC_STAT6
      &$M.system_config.data.ZeroValue,                        // CODEC_STAT7
      &$M.system_config.data.ZeroValue,                        // CODEC_STAT8
      &$M.system_config.data.ZeroValue,                        // CODEC_STAT9
      &$M.system_config.data.ZeroValue,                        // CODEC_STATA
      &$M.system_config.data.ZeroValue;                        // CODEC_STATB

#endif

#ifdef AAC_ENABLE
   .VAR $aac_stats[$music_example.CODEC_STATS_SIZE] =
      &$music_example.SamplingRate,                            // Sampling frequency (from DAC config!)
      &$aacdec.sf_index,                                       // 0..15 - Index for sampling frequency
      &$aacdec.channel_configuration,                          // 1: mono, 2: stereo
      &$aacdec.audio_object_type,                              // 2: LC, 4: LTP, 5: SBR
      &$aacdec.extension_audio_object_type,                    //
      &$aacdec.sbr_present,                                    // 0: not SBR, 1: SBR
      &$aacdec.mp4_frame_count,                                // Count of MP4 frames
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

   // Input handler timer period for 16bit output resolution
   .VAR handler_period_table_16bit[$INPUT_HANDLER_PERIOD_TABLE_SIZE] =
      TMR_PERIOD_ANALOGUE_IN_COPY,                                   // Input rate <= 48kHz
      TMR_PERIOD_ANALOGUE_HI_RATE_IN_COPY;                           // Input rate > 48kHz


   // Input handler timer period for 24bit output resolution
   .VAR handler_period_table_24bit[$INPUT_HANDLER_PERIOD_TABLE_SIZE] =
      TMR_PERIOD_ANALOGUE_HI_RATE_IN_COPY,                           // Input rate <= 48kHz
      TMR_PERIOD_ANALOGUE_HI_RATE_IN_COPY;                           // Input rate > 48kHz

//------------------------------------------------------------------------------
// Program code
//------------------------------------------------------------------------------

   // Safety code to catch and prevent multiple initialisation
   .VAR $reset_count1 = 0;
   r0 = M[$reset_count1];
   r0 = r0 + 1;
   M[$reset_count1] = r0;
   null = r0 - 1;
   if NZ jump $error;
   // End of safety code

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
#if defined(APTX_ENABLE)
   // initialise licensing subsystem
   call $Security.Initialize;
#endif
   // initialise the wallclock library
   call $wall_clock.initialise;
#if defined(DEBUG_ON)
   // initialise the profiler library
   call $profiler.initialise;
#endif
   // init DM_flash
   call $flash.init_dmconst;

#ifdef MP3_USE_EXTERNAL_MEMORY
   // Set up the external memory pointer tables (MP3 shares AAC memory)
   r1 = $mp3_ext_memory_ptrs;
   r0 = $aacdec.overlap_add_left;                  // 576
   M[r1 + $mp3dec.mem.OABUF_LEFT_FIELD] = r0;      // 576
   r0 = $aacdec.overlap_add_right;                 // 576
   M[r1 + $mp3dec.mem.OABUF_RIGHT_FIELD] = r0;     // 576
   r0 = $aacdec.buf_left;                          // 1024  Circular
   M[r1 + $mp3dec.mem.SYNTHV_LEFT_FIELD] = r0;     // 1024  Circular
   r0 = $aacdec.buf_right;                         // 1024  Circular
   M[r1 + $mp3dec.mem.SYNTHV_RIGHT_FIELD] = r0;    // 1024  Circular
   r0 = $aacdec.frame_mem_pool;                    // 1697
   M[r1 + $mp3dec.mem.GENBUF_FIELD] = r0;          // 576
   r0 = $aacdec.tmp_mem_pool;                      // 2504  Circular
   M[r1 + $mp3dec.mem.BITRES_FIELD] = r0;          // 650   Circular
#endif

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

   // set up message handler for VM_SET_TONE_RATE_MESSAGE_ID message
   r1 = &$set_tone_rate_from_vm_message_struc;
   r2 = VM_SET_TONE_RATE_MESSAGE_ID;
   r3 = &$set_tone_rate_from_vm;
   call $message.register_handler;

   // intialize SPI communications library
   call $spi_comm.initialize;

   // Power Up Reset needs to be called once during program execution
   call $music_example.power_up_reset;

   r2 = $music_example.VMMSG.READY;
   r3 = $MUSIC_MANAGER_SYSID;
   // status
   r4 = M[$music_example.Version];
   r4 = r4 LSHIFT -8;
   call $message.send_short;

#if defined SHAREME_ENABLE || defined TWS_ENABLE
   call $relay.init;
#endif

   // re-initialise delay lines
   r4 = &$M.system_config.data.delay_reinitialize_table;
   call $frame_sync.run_function_table;

#if defined(WIRELESS_SUB_ENABLE) || defined(TWS_ENABLE) || defined(SHAREME_ENABLE)
    .VAR write_port_function_table[$cbuffer.NUM_PORTS] =
      #ifdef WIRELESS_SUB_ENABLE
        &$M.Subwoofer.esco_port_connected,
        &$M.Subwoofer.l2cap_port_connected,
      #else
         &dummy,
         &dummy,
      #endif
      &dummy, &dummy,&dummy ...;

    .VAR write_port_disconnected_function_table[$cbuffer.NUM_PORTS] =
       #ifdef WIRELESS_SUB_ENABLE
         &$M.Subwoofer.esco_port_disconnected,
         $M.Subwoofer.l2cap_port_disconnected,
       #else
         &dummy,
         &dummy,
      #endif
      #ifdef TWS_ENABLE
         &$M.relay.init.relay_port_disconnected_handler,
      #else
         &dummy,
      #endif
      &dummy, &dummy ...;

      r0 = &write_port_connected_handler;
      M[$cbuffer.write_port_connect_address] = r0;

      r0 = &write_port_disconnected_handler;
      M[$cbuffer.write_port_disconnect_address] = r0;

      jump skip_port_handlers;

      write_port_connected_handler:
         // r1 = port ID
         r2 = r1 - $cbuffer.WRITE_PORT_OFFSET;
         r0 = M[write_port_function_table + r2];
         jump r0;

      write_port_disconnected_handler:
         // r1 = port ID
         r2 = r1 - $cbuffer.WRITE_PORT_OFFSET;
         r0 = M[write_port_disconnected_function_table + r2];
         jump r0;

       dummy:
          // write port other than subwoofer or TWS relay
          rts;

      skip_port_handlers:
#endif

#ifdef LATENCY_REPORTING
   // reset encoded latency module
   r7 = &$encoded_latency_struct;
   call $latency.reset_encoded_latency;
#endif

#ifdef DEBUG_SPOOF_LATENCY_REPORTING_CONFIG
   // Turn on latency reporting
//     r1 = enable/disable (0 = disable / 1 = enable)
//     r2 = report period in ms (how often the VM would like to receive the
//          delay value from the DSP)
//     r3 = minimum change to report (the DSP will only send the latency if it
//          has changed by this amount).
//     r4 = the last known latency (This value will be used to seed the DSP’s
//          averaging routine.)
   r1 = 1;
   r2 = 2000;
   r3 = 0;
   r4 = 100;
   call $M.configure_latency_reporting.func;
#endif

   // tell vm we're ready and wait for the go message
   call $message.send_ready_wait_for_go;

   // Defer processing of VM messages until channel configuration is completed
   call $block_interrupts;

   // Initialize the USB/decoder according to the connection type (A2DP or USB)
   // (allows run-time selection of connection type)
   call $conn_init;

#if defined(ANALOGUE_ENABLE) || defined(I2S_ENABLE)
   // Set the input scaling for PCM audio inputs (i.e. Analogue, PCM, I2S)
   // according to the input resolution mode ($RESOLUTION_MODE_16BIT or $RESOLUTION_MODE_24BIT)
   r7 = M[$inputResolutionMode];
   call $config_input_scaling;
#endif // defined(ANALOGUE_ENABLE) || defined(I2S_ENABLE)

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

#ifdef USB_ENABLE
   r8 = $usb_audio_in_copy_struc;
   call $config_usb;
#endif

   // Channel complete so allow processing of VM messages
   call $unblock_interrupts;

   // Set the output handler timer according to whether ANC is configured
   call $set_output_handler_timer;

   // start timer that copies output samples
   r1 = &$audio_out_timer_struc;
   r2 = M[$tmr_period_audio_copy];
   r3 = &$audio_out_copy_handler;
   call $timer.schedule_event_in;

   // Start a timer that copies USB/codec input data according to the connection type
   r1 = &$con_in_timer_struc;
   r2 = M[$tmr_period_con_copy];
   r3 = &$con_in_copy_handler;
   call $timer.schedule_event_in;

   // start timer that copies tone/prompt samples
   r1 = &$tone_copy_timer_struc;
   r2 = M[$tmr_period_tone_copy];
   r3 = &$tone_copy_handler;
   call $timer.schedule_event_in;

    // post timer event for standby level detector
    r1 = &$signal_detect_timer_struc;
    r2 = SIGNAL_DETECT_TIMER_PERIOD;
    r3 = &$signal_detect_timer_handler;
    call $timer.schedule_event_in;

   // continually decode codec frames
   frame_loop:

      // Check Communication
      call $spi_comm.polled_service_routine;

      // Check the connection type (A2DP or USB/ANALOGUE) and process accordingly
      r0 = M[$app_config.io];

#ifdef USB_ENABLE
      null = r0 - $USB_IO;
      if NZ jump skip_usb;
         // Perform buffer reset if requested (from background)
         call $usb_cond_buffer_reset;
         jump skip_a2dp_decode;
      skip_usb:
#endif

#ifdef ANALOGUE_ENABLE
      null = r0 - $ANALOGUE_IO;
      if Z jump skip_a2dp_decode;
#endif

#ifdef I2S_ENABLE
      null = r0 - $I2S_IO;
      if Z jump skip_a2dp_decode;
#endif

      start_profiler:
      r8 = &$DecoderMips_data_block;
      call $M.mips_profile.mainstart;

#if defined SHAREME_ENABLE || defined TWS_ENABLE
      r5 = &$relay_struc;
      call $relay.start;
#endif

      r5 = &$decoder_codec_stream_struc;
      call $codec.av_decode;

#if defined(TWS_ENABLE)
      // Do not perform silence insertion when TWS is active
      r0 = M[$relay.mode];
      null = r0 - $TWS_MASTER_MODE;
      if Z jump skip_silence_insertion;
         null = r0 - $TWS_SLAVE_MODE;
      if Z jump skip_silence_insertion;
#endif

      // If not stalled prime the number of samples for zero insertion
      r0 = M[$decoder_codec_stream_struc + $codec.av_decode.CURRENT_RUNNING_MODE_FIELD];
      if NZ call $av_audio_out_silence_prime;

      // If stalled insert given number of zeros into audio buffer
      r0 = M[$decoder_codec_stream_struc + $codec.av_decode.CURRENT_RUNNING_MODE_FIELD];
      if Z call $av_audio_out_silence_insert;

#if defined(TWS_ENABLE)
     skip_silence_insertion:
#endif

#if defined SHAREME_ENABLE || defined TWS_ENABLE
      r5 = &$relay_struc;
      call $relay.stop;
      .VAR $audio_out_level = 0;
       r0 = &$audio_out_left_cbuffer_struc;
       call $cbuffer.calc_amount_data;
       M[$audio_out_level] = r0;
#endif

#ifdef TWS_WIRED_MODE_ENABLE
      null = M[$tws.wired_mode_enabled];
      if Z call $sra_calcrate;
#else
      call $sra_calcrate;
#endif

#ifdef LATENCY_REPORTING
      Null = M[$M.configure_latency_reporting.enabled];
      if Z jump skip_latency_measurement;
         r7 = &$encoded_latency_struct;
         call $latency.calc_encoded_latency;
      skip_latency_measurement:
#endif

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

      jump done_decode;

skip_a2dp_decode:

#if defined(USB_ENABLE) || defined(ANALOGUE_ENABLE) || defined(I2S_ENABLE)
      call $sra_calcrate;
#ifdef TWS_WIRED_MODE_ENABLE
      r0 = M[$relay.mode];
      null = r0 - $TWS_MASTER_MODE;
      if Z jump start_profiler;
#endif

      // Reduce power consumption and calculate total MIPS (for wired inputs)
      // Sleep and wait for sync (from output handler)
      call $SystemSleepAudio;
#endif // defined(USB_ENABLE) || defined(ANALOGUE_ENABLE) || defined(I2S_ENABLE)

done_decode:

      // run resampler if needed
      Null = M[$codec_resampler.resampler_active];
      if NZ call $codec_resampler.run_resampler;

      // run sw rate adjustment if needed
      Null = M[$chain0_hw_warp_enable];
      if Z call $codec_rate_adj.run_rate_adjustment;

      // compute adjust rate to synchronise chain1 if it's active
      r8 = $chain1_to_chain0_pcm_sync_struct;
      Null = M[$M.multi_chan_output.num_chain1_channels];
      if NZ call $pcm_sync_calc_rate;

      // data/space check only on left in/out primary buffers. These buffers are
      // assumed always active. The system around music manager guarantees
      // synchronized outputs
      r3 = M[$music_example.frame_processing_size];

#ifdef WIRELESS_SUB_ENABLE
      call $M.Subwoofer.set_frame_size;
#endif

      r0 = M[$M.system_config.data.stream_map_left_in + 0];
      call $cbuffer.calc_amount_data;
      null = r0 - r3;
      if NEG jump frame_loop;

      r0 = M[$M.system_config.data.stream_map_primary_left_out + 0];
      call $cbuffer.calc_amount_space;
      null = r0 - r3;
      if NEG jump frame_loop;

#if defined(USB_ENABLE)
      call $jitter_buffering;
      Null = M[$M.jitter_buffering.is_buffering];
      if NZ jump frame_loop;
#endif

      call $music_example_process;

#ifdef WIRELESS_SUB_ENABLE
      call $M.Subwoofer.transmit_wireless_subwoofer_audio;
#endif

   jump frame_loop;

.ENDMODULE;

//------------------------------------------------------------------------------
.MODULE $M.signal_detect_timer_handler;
//------------------------------------------------------------------------------
// timer handler to call signal detector processing every second
//------------------------------------------------------------------------------

    .CODESEGMENT PM;
    .DATASEGMENT DM;

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

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $audio_out_copy_handler
//
// DESCRIPTION:
//    Function called on a timer interrupt to perform the PCM data to
//    output MMU buffer copy for n (1 to 7) channels
// *****************************************************************************
.MODULE $M.audio_out_copy_handler;
   .CODESEGMENT AUDIO_OUT_COPY_HANDLER_PM;
   .DATASEGMENT DM;

   .VAR $multi_chain0_copy_struc_ptr = $M.multi_chan_output.chain0_copy_struc;
   .VAR $multi_chain1_copy_struc_ptr = $M.multi_chan_output.chain1_copy_struc;

   $audio_out_copy_handler:

   // push rLink onto stack
   $push_rLink_macro;

   // reset the sync flag to wake from idling in low power mode
   M[$frame_sync.sync_flag] = Null;

#ifdef WIRELESS_SUB_ENABLE
   // Check to see if TimeToPlay needs to be recomputed
   call $M.Subwoofer.validate_time_to_play;
#endif

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

   #if defined(WIRELESS_SUB_ENABLE) || defined(LATENCY_REPORTING) || defined(TWS_ENABLE)

      // set the warp value to be used for latency caluclation
      r8 = $codec_rate_adj.stereo + $cbops.rate_adjustment_and_shift.Process.SRA_CURRENT_RATE_FIELD;
      r7 = $hw_warp_struct + $hw_warp.CURRENT_RATE_FIELD;
      Null = M[$chain0_hw_warp_enable];
      if NZ r8 = r7;
      M[$latency_calc_current_warp] = r8;

   #endif //defined(WIRELESS_SUB_ENABLE) || defined(LATENCY_REPORTING) || defined(TWS_ENABLE)

   // care for initial latency
   #ifdef SRA_TARGET_LATENCY
      call $adjust_initial_latency;
   #endif

   // aux data are read in another ISR,
   // if aux level is low do another read
   // here, no action if we already have good
   // amount of aux data
   call $tone_copy_extra;

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

   // Relay copy operatoin for ShareMe and TWS
#if  defined SHAREME_ENABLE || defined TWS_ENABLE

   // Check Relay Port
   r0 = $RELAY_PORT;
   call $cbuffer.is_it_enabled;
   if Z jump done_with_relay;
      // copy data to relay port
      r8 = &$relay_copy_struc;
      call $cbops.copy;

   done_with_relay:

#endif

#ifdef WIRELESS_SUB_ENABLE
   // Check to see if TimeToPlay needs to be recomputed
   call $M.Subwoofer.compute_time_to_play;
#endif


   // Post another timer event
   r1 = &$audio_out_timer_struc;
   r2 = M[$tmr_period_audio_copy];
   r3 = &$audio_out_copy_handler;
   call $timer.schedule_event_in_period;

   // pop rLink from stack
   jump $pop_rLink_and_rts;

.ENDMODULE;

#ifdef AAC_ENABLE
// *****************************************************************************
// MODULE:
//    $set_aac_read_frame_function
//
// DESCRIPTION: set aac read frame function
//
// INPUTS:
//    None
// OUTPUTS:
//    None
// *****************************************************************************
.MODULE $M.set_aac_read_frame_function;
   .CODESEGMENT PM;
$set_aac_read_frame_function:
   // LATM for a2dp
   r0 = &$aacdec.latm_read_frame;
   // ADTS for local play back
   r1 = &$aacdec.adts_read_frame;
   Null = M[$local_encoded_play_back];
   if NZ r0 = r1;
   M[$aacdec.read_frame_function] = r0;
   rts;
.ENDMODULE;
// *****************************************************************************
// MODULE:
//    $read_bytes_from_port
//
// DESCRIPTION: reads input data from 8-bit port
//
// INPUTS:
//  r8 = port_byte_read structure
// OUTPUTS:
//  None
// NOTES:
//   - assume trashes every register
//   - run this only inside an ISR
//   - this is for reading aac data from port
// *****************************************************************************
   .CONST $port_byte_read.INPUT_PORT_FIELD               0; // 8-bit configured data port
   .CONST $port_byte_read.OUTPUT_BUFFER_FIELD            1; // buffer to write (16-bit) words
   .CONST $port_byte_read.HALF_WORD_VALID_PTR_FIELD      2; // address holding half word validity
   .CONST $port_byte_read.STRUCT_SIZE                    3;

.MODULE $M.read_bytes_from_port;
   .CODESEGMENT PM;
   .DATASEGMENT DM;
$read_bytes_from_port:
   // push rLink onto stack
   $push_rLink_macro;

   // read port level
   r0 = M[r8 + $port_byte_read.INPUT_PORT_FIELD];
   call $cbuffer.calc_amount_data;
   r7 = r0;

   // read space in the buffer
   r0 = M[r8 + $port_byte_read.OUTPUT_BUFFER_FIELD];
   call $cbuffer.calc_amount_space;
   r6 = r0 + r0;

   // get half word pointers
   r5 = M[r8 + $port_byte_read.HALF_WORD_VALID_PTR_FIELD];

   // minus one byte if half word remained from previous write
   r6 = r6 + M[r5];

   // get number of bytes can read from port
   r2 = r6 - r7;
   if POS r6 = r6 - r2;

   // continue only if at least one byte to read
   r10 = r6;
   if LE jump end_read_data_port;

   // get codec input port read pointer
   r0 = M[r8 + $port_byte_read.INPUT_PORT_FIELD];
   call $cbuffer.get_read_address_and_size;
   I0 = r0;
   L0 = r1;

   // get codec buffer write pointer
   r0 = M[r8 + $port_byte_read.OUTPUT_BUFFER_FIELD];
   call $cbuffer.get_write_address_and_size;
   I4 = r0;
   L4 = r1;

   // see if a byte left in previous read
   Null = M[r5];
   if Z jump prev_half_word_done;
      // write (prev byte)|a new byte
      r0 = M[I4,-1];
      r0 = M[I4,0];
      r1 = M[I0,1];
      r0 = r0 + r1;
      M[I4,1] = r0;
      // reduce number of bytes to read
      r10 = r10 - 1;
      M[r5] = Null;
   prev_half_word_done:


   r4 = r10 AND 1;
   r10 = r10 LSHIFT -1;
   if Z jump read_loop_done;
   // read 2*r10 bytes
   // write r10 words
   r10 = r10 - 1;
   r2 = 8;
   M0 = 1;
   r0 = M[I0,M0];
   r1 = M[I0,M0];
   do read_port_loop;
      // wordn = byte2n|byte2n+1
      r3 = r0 LSHIFT r2, r0 = M[I0,M0];
      r3 = r3 + r1, r1 = M[I0,M0];
      M[I4,1] = r3;
   read_port_loop:
      // build and write last word
      r3 = r0 LSHIFT r2;
      r3 = r3 + r1;
      M[I4,1] = r3;
   read_loop_done:
   // update half word valid flag
   M[r5] = r4;
   if Z jump read_half_word_done;
      // write last byte in the port into half word var
      r0 = M[I0,1];
      r0 = r0 LSHIFT 8;
      M[I4,1] = r0;
   read_half_word_done:

   // update input port
   r0 = M[r8 + $port_byte_read.INPUT_PORT_FIELD];
   r1 = I0;
   call $cbuffer.set_read_address;
   L0 = 0;

   // update output buffer
   r0 = M[r8 + $port_byte_read.OUTPUT_BUFFER_FIELD];
   r1 = I4;
   call $cbuffer.set_write_address;
   L4 = 0;

   end_read_data_port:

  // pop rLink from stack
   jump $pop_rLink_and_rts;
.ENDMODULE;

#endif
// *****************************************************************************
// MODULE:
//    $con_in_copy_handler
//
// DESCRIPTION:
//    Function called on a timer interrupt to perform the copying of data
//    from the input port (USB/I2S/ADC or A2DP)
//
// *****************************************************************************
.MODULE $M.con_in_copy_handler;
   .CODESEGMENT CON_IN_COPY_HANDLER_PM;
   .DATASEGMENT DM;

   .VAR $mono_adc_input = 0;

   $con_in_copy_handler:

   // push rLink onto stack
   $push_rLink_macro;

#ifdef LATENCY_REPORTING
   r0 = M[&$codec_in_cbuffer_struc + $cbuffer.WRITE_ADDR_FIELD];
   M[$codec_packets_info_last_write_address] = r0;
#endif
   call $sra_tagtimes;

   // Get the connection type
   r0 = M[$app_config.io];

   // Check the connection type (A2DP or USB/ANALOGUE) and set the structure inputs appropriately
#ifdef USB_ENABLE
   null = r0 - $USB_IO;
   if NZ jump skip_usb_copy;

      // Copy encoded data from the USB port to the cbuffer
      r8 = &$usb_audio_in_copy_struc;
      r7 = $frame_sync.usb_in_stereo_audio_copy;
      call $run_usb_state_mc;

      jump copy_done;

   skip_usb_copy:
#endif

#ifdef ANALOGUE_ENABLE
   null = r0 - $ANALOGUE_IO;
   if Z jump analogue_copy;
#endif

#ifdef I2S_ENABLE
   null = r0 - $I2S_IO;
   if Z jump i2s_copy;
#endif

#if defined(ANALOGUE_ENABLE) || defined(I2S_ENABLE)
   jump codec_copy;

   analogue_copy:
   i2s_copy:
      // Copy stereo/mono pcm input data from the ports to the cbuffers
      r8 = &$analogue_audio_in_copy_struc;
      r7 = &$analogue_audio_in_mono_copy_struc;
      r0 = $CON_IN_RIGHT_PORT;
      call $cbuffer.is_it_enabled;
      if Z r8 = r7;
      call $cbops.copy;

      jump copy_done;

   codec_copy:
#endif

#if defined(APTX_ENABLE)
   .VAR byte0;
   .VAR byte1;
   .VAR byte0_flag;

   // Force an MMU buffer set
   null = M[$PORT_BUFFER_SET];

   // Get amount of data (in bytes) in input port
   r0 = $CON_IN_PORT;
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
      M[($CON_IN_PORT & $cbuffer.PORT_NUMBER_MASK) + $READ_PORT0_CONFIG] = $BITMODE_8BIT_ENUM;

      // Read the second byte to complete the 16bit word
      r0 = M[($CON_IN_PORT & $cbuffer.PORT_NUMBER_MASK) + $READ_PORT0_DATA];
      r0 = r0 AND 0xff;
      M[byte1] = r0;

      // Update input port
      r0 = $CON_IN_PORT;
      r1 = ($CON_IN_PORT & $cbuffer.PORT_NUMBER_MASK) + $READ_PORT0_DATA;
      call $cbuffer.set_read_address;

      r0 = $CON_IN_PORT;
      call $cbuffer.calc_amount_data;

      // Switch to 16bit read mode
      r0 = $BITMODE_16BIT_ENUM + $BYTESWAP_MASK + $NOSIGNEXT_MASK;
      M[($CON_IN_PORT & $cbuffer.PORT_NUMBER_MASK) + $READ_PORT0_CONFIG] = r0;

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
#endif

#ifdef AAC_ENABLE
   r0 = M[$app_config.io];          // Get the run-time selection
   null = r0 - $AAC_IO;             // Is the AAC codec selected
   if Z jump aac_byte_read;         // Yes - start reading in octets

   null = r0 - $AAC_ELD_IO;         // Is the AAC_ELD codec selected
   if Z jump aac_byte_read;         // Yes - start reading in octets

   jump skip_aac_byte_read;         // Skip if not AAC

   aac_byte_read:

   // for AAC read from 8-bit port, this will make sure no data will be left
   // in the port during pause mode
   .CONST $CODEC_IN_PORT_8BIT            ($CON_IN_PORT|$cbuffer.FORCE_8BIT_WORD);
   .VAR $aac_port_byte_read_struct[$port_byte_read.STRUCT_SIZE] =
      $CODEC_IN_PORT_8BIT,
      &$codec_in_cbuffer_struc,
      &$aacdec.write_bytepos;

   r8 = &$aac_port_byte_read_struct;
   call $read_bytes_from_port;

   jump input_read_done;
   skip_aac_byte_read:
#endif

   // Codec handling
   // Copy encoded data from the port to the cbuffer
   r8 = &$codec_in_copy_struc;
   call $cbops.copy;

input_read_done:


#if defined(APTX_ENABLE)
   // Force an MMU buffer set
   null = M[$PORT_BUFFER_SET];

   // Get amount of data (in bytes) in input port
   r0 = $CON_IN_PORT;
   call $cbuffer.calc_amount_data;

   // Only 1 byte left?
   null = r1 - 1;
   if NZ jump skip_odd_byte;

      // Switch to 8bit read mode
      M[($CON_IN_PORT & $cbuffer.PORT_NUMBER_MASK) + $READ_PORT0_CONFIG] = $BITMODE_8BIT_ENUM;

      // Read the odd byte (byte0)
      r0 = M[($CON_IN_PORT & $cbuffer.PORT_NUMBER_MASK) + $READ_PORT0_DATA];
      r0 = r0 AND 0xff;
      M[byte0] = r0;

      // Update input port (this lets the firmware send more data)
      r0 = $CON_IN_PORT;
      r1 = ($CON_IN_PORT & $cbuffer.PORT_NUMBER_MASK) + $READ_PORT0_DATA;
      call $cbuffer.set_read_address;

      // Switch to 16bit read mode
      r0 = $BITMODE_16BIT_ENUM + $BYTESWAP_MASK + $NOSIGNEXT_MASK;
      M[($CON_IN_PORT & $cbuffer.PORT_NUMBER_MASK) + $READ_PORT0_CONFIG] = r0;

      // Flag an odd byte (byte0) has been read
      r0 = 1;
      M[byte0_flag] = r0;

   skip_odd_byte:
#endif

   .VAR $debug_codec_in_level = 0;
   r0 = &$codec_in_cbuffer_struc;
   call $cbuffer.calc_amount_data;
   M[$debug_codec_in_level] = r0;



   #ifdef SRA_TARGET_LATENCY
      // mark the arrival of first packet
      Null = M[$first_packet_received];
      if NZ jump first_packet_done;
      r0 = M[&$codec_in_cbuffer_struc + $cbuffer.WRITE_ADDR_FIELD];
      Null = r0 - M[$codec_packets_info_last_write_address];
      if Z jump first_packet_done;
         r0 = 1;
         r2 = M[$TIMER_TIME];
         M[$first_packet_received] = r0;
         M[$first_packet_time] = r2;
     first_packet_done:
   #endif

   copy_done:

   // apply hw rate matching
   r0 = M[$chain0_hw_warp_enable];
   r0 = r0 + M[$chain1_hw_warp_enable];
   if NZ call $apply_hardware_warp_rate;

#ifdef LATENCY_REPORTING
   Null = M[$M.configure_latency_reporting.enabled];
   if Z jump skip_packet_detection;
      call $media_packet_boundary_detection;
   skip_packet_detection:
#endif
   // post another timer event
   r1 = &$con_in_timer_struc;
   r2 = M[$tmr_period_con_copy];
   r3 = &$con_in_copy_handler;
   call $timer.schedule_event_in_period;

   // pop rLink from stack
   jump $pop_rLink_and_rts;

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
//
//  [r4 = bits(1:0): DEPRECATED in multi-channel implementation
//                  audio output interface type:
//                  0 -> None (not expected)
//                  1 -> Analogue output (DAC)
//                  2 -> I2S output
//                  3 -> SPDIF output]
//
//  r4 = bit8: playback mode (0: remote playback, 1: local file play back)
//       bit9: pcm playback, releavant only when b8==1
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
   // b8 b9    encoded   pcm
   // 0  x       N        N
   // 1  0       Y        N
   // 1  1       N        Y
   r1 = r4 AND $PCM_PLAYBACK_MASK;                    // Mask for pcm/coded bit
   r2 = r1 XOR $PCM_PLAYBACK_MASK;
   r0 = r4 AND $LOCAL_PLAYBACK_MASK;                  // Mask for local file play back info
   r2 = r2 * r0 (int)(sat);
   M[$local_encoded_play_back] = r2;                  // encoded play back
   r3 = 0x1;
   r1 = r1 * r0 (int)(sat);
   if NZ r1 = r3;
   M[$aux_input_stream_available] = r1;               // aux pcm stream play back

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
//  r1 = codec sampling rate/10 (e.g. 44100Hz is given by r1=4410)
//  r2 =
//  r3 =
//  r4 =
// *****************************************************************************
.MODULE $M.set_codec_rate_from_vm;
   .CODESEGMENT SET_CODEC_RATE_FROM_VM_PM;

$set_codec_rate_from_vm:

   // Mask sign extension
   r1 = r1 AND 0xffff;

   // Scale to get sampling rate in Hz
   r1 = r1 * 10 (int);

   // Store the codec sampling rate
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
   r4 = M[$local_encoded_play_back];                 // NZ means local file play back
   if Z jump is_remote_stream;
      // Set the timer period for local play back
      r0 = LOCAL_TMR_PERIOD_CODEC_COPY;
      M[$tmr_period_con_copy] = r0;

      // Force rate matching to be disabled for local playback
      r0 = 1;
      M[&$rate_match_disable] = r0; // Enable: 0, Disable: 1

   is_remote_stream:

   // If rate matching is disabled don't update rate
   null = M[&$rate_match_disable];
   if NZ jump end;

   // set maximum rate for clock mismatch compensation
   r2 = r2 AND 0x7F;
   r1 = r2 - 4;         // min 0.3% percent by default
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
//
// Master reset routine, called to clear garbage samples during a pause
//
// *****************************************************************************
.MODULE $M.master_app_reset;
   .CODESEGMENT MASTER_APP_RESET_PM;
   .DATASEGMENT DM;

   $master_app_reset:

   // push rLink onto stack
   $push_rLink_macro;

   // local play back?
   Null = M[$local_encoded_play_back];
   if Z jump pause_happened;

   // notify VM about end of play_back
   r2 = PLAY_BACK_FINISHED_MSG;
   r3 = 0;
   r4 = 0;
   r5 = 0;
   r6 = 0;
   call $message.send_short;
   M[$local_encoded_play_back] = 0;
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
      M[$sra_struct +  $sra.TARGET_LATENCY_MS_FIELD] = 0;
   #endif

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
// TRASHES: r1, r2
//
// *****************************************************************************
.MODULE $M.conn_init_cbuffers;
   .CODESEGMENT PM;

$conn_init_cbuffers:

   $push_rLink_macro;

   push r0;

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

   pop r0;

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
//    $sra.MAX_RATE_FIELD                         4;    //input: maximum possible rate
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
// TRASHES: r0, r10, I0 and those trashed by the codec initialise functions
//
// *****************************************************************************
.MODULE $M.conn_init;

   .CODESEGMENT PM;

$conn_init:

   $push_rLink_macro;

   // Get the connection type
   r0 = M[$app_config.io];

#ifdef USB_ENABLE
   // Check the connection type (A2DP or USB) and initialize accordingly
   null = r0 - $USB_IO;
   if NZ jump skip_usb_conn_init;

      // Set up the audio cbuffers for USB
      r3 = USB_AUDIO_CBUFFER_SIZE;
      r4 = CODEC_CBUFFER_SIZE;
      call $conn_init_cbuffers;

      // ------------------------------------------------------------------------
      // Set up the Rate Adaptation for USB

      // Set up the SRA struc for the USB connection
      r0 = (SRA_AVERAGING_TIME*1000000)/TMR_PERIOD_USB_IN_COPY;
      M[$sra_struct + $sra.TAG_DURATION_FIELD] = r0;
      r0 = $CON_IN_PORT;
      M[$sra_struct + $sra.CODEC_PORT_FIELD] = r0;
      r0 = &$audio_out_left_cbuffer_struc;
      M[$sra_struct + $sra.CODEC_CBUFFER_TO_TAG_FIELD] = r0;
      r0 = &$multi_chan_primary_left_out_cbuffer_struc;
      M[$sra_struct + $sra.AUDIO_CBUFFER_TO_TAG_FIELD] = r0;
      r0 = DEFAULT_MAXIMUM_CLOCK_MISMATCH_COMPENSATION;
      M[$sra_struct + $sra.MAX_RATE_FIELD] = r0;
      r0 = 48000*SRA_AVERAGING_TIME;
      M[$sra_struct + $sra.AUDIO_AMOUNT_EXPECTED_FIELD] = r0;

      // ------------------------------------------------------------------------
      // Initialize the USB timer and codec type for USB

      // Initialize the timer period
      r0 = TMR_PERIOD_USB_IN_COPY;
      M[$tmr_period_con_copy] = r0;

      // ------------------------------------------------------------------------
      // Override the statistics table pointers for USB

      // Initialise the statistics pointer table
      I0 = $usb_stats;
      call $copy_codec_stats_pointers;

#if defined(TWS_ENABLE)
      // Should Shareme handle Analogue and USB??
   #ifdef SBC_ENABLE
      r0 = $sbc.sbc_common_data_array + $sbc.mem.GET_BITPOS_FIELD;
      M[$relay_struc + $relay.CODEC_GET_BITPOS] = r0;
      r0 = $sbcdec.getbits;
      M[$relay_struc + $relay.CODEC_GETBITS] = r0;
   #endif

#endif

      jump exit;

   skip_usb_conn_init:
#endif

   // Set up the audio cbuffers for other input/codec types
   r3 = AUDIO_CBUFFER_SIZE;
   r4 = CODEC_CBUFFER_SIZE;
   call $conn_init_cbuffers;

#if defined(ANALOGUE_ENABLE) || defined(I2S_ENABLE)

#ifdef ANALOGUE_ENABLE
   r1 = $analogue_stats;
   null = r0 - $ANALOGUE_IO;
   if Z jump analogue_conn_init;
#endif

#ifdef I2S_ENABLE
   r1 = $i2s_stats;
   null = r0 - $I2S_IO;
   if Z jump i2s_conn_init;
#endif

   jump skip_analogue_i2s_conn_init;

   analogue_conn_init:
   i2s_conn_init:

      // Initialise the statistics table pointers for analogue inputs
      I0 = r1;
      call $copy_codec_stats_pointers;

      // ------------------------------------------------------------------------
      // Initialize the input handler timer for analogue/i2s
      call $set_input_handler_timer;

      // ------------------------------------------------------------------------
      // Set up the Rate Adaptation for analogue/i2s input

      r0 = (SRA_AVERAGING_TIME*1000000)/TMR_PERIOD_ANALOGUE_IN_COPY;
      r1 = (SRA_AVERAGING_TIME*1000000)/TMR_PERIOD_ANALOGUE_HI_RATE_IN_COPY;

      // Configure according to the input sampling rate
      r3 = M[$current_codec_sampling_rate];
      Null = 48000 - r3;                        // Input sampling rate > 48kHz?
      if NEG r0 = r1;                           // Yes - use the hi-rate tag duration

      M[$sra_struct + $sra.TAG_DURATION_FIELD] = r0;
      r0 = $CON_IN_PORT;
      M[$sra_struct + $sra.CODEC_PORT_FIELD] = r0;
      r0 = &$audio_out_left_cbuffer_struc;
      M[$sra_struct + $sra.CODEC_CBUFFER_TO_TAG_FIELD] = r0;
      r0 = &$multi_chan_primary_left_out_cbuffer_struc;
      M[$sra_struct + $sra.AUDIO_CBUFFER_TO_TAG_FIELD] = r0;
      r0 = DEFAULT_MAXIMUM_CLOCK_MISMATCH_COMPENSATION;
      M[$sra_struct + $sra.MAX_RATE_FIELD] = r0;
      r0 = 48000*SRA_AVERAGING_TIME;
      M[$sra_struct + $sra.AUDIO_AMOUNT_EXPECTED_FIELD] = r0;

      // ------------------------------------------------------------------------

#if defined(TWS_ENABLE)
      // Should Shareme handle Analogue and USB??
   #ifdef SBC_ENABLE
      r0 = $sbc.sbc_common_data_array + $sbc.mem.GET_BITPOS_FIELD;
      M[$relay_struc + $relay.CODEC_GET_BITPOS] = r0;
      r0 = $sbcdec.getbits;
      M[$relay_struc + $relay.CODEC_GETBITS] = r0;
   #endif

#endif

      jump exit;

   skip_analogue_i2s_conn_init:
#endif

      // ------------------------------------------------------------------------
      // Set up the Rate Adaptation for A2DP

      // Set up the SRA struc for the A2DP connection
      r0 = (SRA_AVERAGING_TIME*1000000)/TMR_PERIOD_CODEC_COPY;
      M[$sra_struct + $sra.TAG_DURATION_FIELD] = r0;
      r0 = $CON_IN_PORT;
      M[$sra_struct + $sra.CODEC_PORT_FIELD] = r0;
      r0 = &$codec_in_cbuffer_struc;
      M[$sra_struct + $sra.CODEC_CBUFFER_TO_TAG_FIELD] = r0;
      r0 = &$multi_chan_primary_left_out_cbuffer_struc;
      M[$sra_struct + $sra.AUDIO_CBUFFER_TO_TAG_FIELD] = r0;
      Null = M[$current_dac_sampling_rate];
      if NZ jump sra_conf_done;
         r0 = DEFAULT_MAXIMUM_CLOCK_MISMATCH_COMPENSATION;
         M[$sra_struct + $sra.MAX_RATE_FIELD] = r0;
         r0 = 48000*SRA_AVERAGING_TIME;
         M[$sra_struct + $sra.AUDIO_AMOUNT_EXPECTED_FIELD] = r0;
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

#ifdef SBC_ENABLE
      // Check the codec type and initialize accordingly
      null = r0 - $SBC_IO;
      if NZ jump skip_sbc;

         // Set up the decoder structure for SBC
         r0 = $sbcdec.frame_decode;
         M[$decoder_codec_stream_struc + $codec.av_decode.ADDR_FIELD] = r0;
         r0 = $sbcdec.reset_decoder;
         M[$decoder_codec_stream_struc + $codec.av_decode.RESET_ADDR_FIELD] = r0;
         r0 = $sbcdec.silence_decoder;
         M[$decoder_codec_stream_struc + $codec.av_decode.SILENCE_ADDR_FIELD] = r0;

         // Initialise the SBC decoder library, this will also set memory pointer
         // pass in the start of the decoder structure nested inside av_decode structure,
         // so init function will set the data object pointer field of it.
         r5 = $decoder_codec_stream_struc + $codec.av_decode.DECODER_STRUC_FIELD;
         call $sbcdec.init_static_decoder;

         // Initialise the statistics pointer table
         I0 = $sbc_stats;
         call $copy_codec_stats_pointers;

#if defined(SHAREME_ENABLE) || defined(TWS_ENABLE)
         r0 = $sbc.sbc_common_data_array + $sbc.mem.GET_BITPOS_FIELD;
         M[$relay_struc + $relay.CODEC_GET_BITPOS] = r0;
         r0 = $sbcdec.getbits;
         M[$relay_struc + $relay.CODEC_GETBITS] = r0;
#endif
         jump exit;

      skip_sbc:
#endif

#ifdef MP3_ENABLE
      null = r0 - $MP3_IO;
      if NZ jump skip_mp3;

#ifdef MP3_USE_EXTERNAL_MEMORY
         // Set up the external memory pointer tables
         r9 = $mp3_ext_memory_ptrs;
#endif
         // Set up the decoder structure for MP3
         r0 = $mp3dec.frame_decode;
         M[$decoder_codec_stream_struc + $codec.av_decode.ADDR_FIELD] = r0;
         r0 = $mp3dec.reset_decoder;
         M[$decoder_codec_stream_struc + $codec.av_decode.RESET_ADDR_FIELD] = r0;
         r0 = $mp3dec.silence_decoder;
         M[$decoder_codec_stream_struc + $codec.av_decode.SILENCE_ADDR_FIELD] = r0;

         // Initialise the MP3 decoder library
         call $mp3dec.init_decoder;

         // Initialise the statistics pointer table
         I0 = $mp3_stats;
         call $copy_codec_stats_pointers;

#if defined(SHAREME_ENABLE) || defined(TWS_ENABLE)
         r0 = &$mp3dec.get_bitpos;
         M[$relay_struc + $relay.CODEC_GET_BITPOS] = r0;
         r0 = &$mp3dec.getbits;
         M[$relay_struc + $relay.CODEC_GETBITS] = r0;
#endif

         jump exit;

      skip_mp3:
#endif

#ifdef APTX_ENABLE
      null = r0 - $APTX_IO;
      if Z jump init_aptx;

      null = r0 - $APTXHD_IO;
      if Z jump init_aptxhd;
      jump skip_aptx;

      init_aptx:
      init_aptxhd:

         // Set up the decoder structure for APTX
         r0 = $aptx.decode;
         M[$decoder_codec_stream_struc + $codec.av_decode.ADDR_FIELD] = r0;
         r0 = $aptx.reset_decoder;
         M[$decoder_codec_stream_struc + $codec.av_decode.RESET_ADDR_FIELD] = r0;
         r0 = $aptx.silence_decoder;
         M[$decoder_codec_stream_struc + $codec.av_decode.SILENCE_ADDR_FIELD] = r0;

         // Initialise the APTX decoder library
         call $aptx.init_decoder;

         // Initialise the statistics pointer table
         I0 = $aptx_stats;
         call $copy_codec_stats_pointers;

#if defined(SHAREME_ENABLE) || defined(TWS_ENABLE)
         r0=1;
         M[$relay_struc + $relay.IS_APTX_FIELD] = r0;     //Set APTX enabled in relay struc
         r0 = $aptxdec.get_bitpos;
         M[$relay_struc + $relay.CODEC_GET_BITPOS] = r0;
         r0 = $aptxdec.getbits;
         M[$relay_struc + $relay.CODEC_GETBITS] = r0;
#endif
         jump exit;

      skip_aptx:
#endif

#ifdef AAC_ENABLE
      null = r0 - $AAC_IO;
      if Z jump do_aac;

      null = r0 - $AAC_ELD_IO;
      if NZ jump skip_aac;

      do_aac:

         // Set up the decoder structure for AAC
         r0 = $aacdec.frame_decode;
         M[$decoder_codec_stream_struc + $codec.av_decode.ADDR_FIELD] = r0;
         r0 = $aacdec.reset_decoder;
         M[$decoder_codec_stream_struc + $codec.av_decode.RESET_ADDR_FIELD] = r0;
         r0 = $aacdec.silence_decoder;
         M[$decoder_codec_stream_struc + $codec.av_decode.SILENCE_ADDR_FIELD] = r0;

         // Initialise the AAC decoder library
         call $aacdec.init_decoder;

         // set read frame function
         call $set_aac_read_frame_function;

         // Initialise the statistics pointer table
         I0 = $aac_stats;
         call $copy_codec_stats_pointers;

#if defined(SHAREME_ENABLE) || defined(TWS_ENABLE)
         r0 = $aacdec.get_bitpos;
         M[$relay_struc + $relay.CODEC_GET_BITPOS] = r0;
         r0 = $aacdec.getbits;
         M[$relay_struc + $relay.CODEC_GETBITS] = r0;
#endif

         jump exit;

      skip_aac:
#endif

         // Unknown codec
         jump $error;
   exit:

   jump $pop_rLink_and_rts;
.ENDMODULE;

#ifdef SRA_TARGET_LATENCY

// Use multi channel version of $adjust_initial_latency

// *****************************************************************************
// MODULE:
//    $adjust_initial_latency
//
// DESCRIPTION:
//    Function to adjusts the initial latency
//
// INPUTS:
//    none
//
// OUTPUTS:
//    r0
//
// TRASHED:
//    Assume all
//
// *****************************************************************************
.MODULE $M.adjust_initial_latency;
   .CODESEGMENT AUDIO_OUT_COPY_HANDLER_PM;
   .DATASEGMENT DM;

   .CONST $OUTPUT_SILENCE_AUDIO_CBUFFER_SIZE                    5;

   // Create dummy silence output cbuffers
   DeclareCBuffer($multi_chan_sil_primary_left_out_cbuffer_struc,$multi_chan_sil_primary_left_out,$OUTPUT_SILENCE_AUDIO_CBUFFER_SIZE);
   DeclareCBuffer($multi_chan_sil_primary_right_out_cbuffer_struc,$multi_chan_sil_primary_right_out,$OUTPUT_SILENCE_AUDIO_CBUFFER_SIZE);
   DeclareCBuffer($multi_chan_sil_secondary_left_out_cbuffer_struc,$multi_chan_sil_secondary_left_out,$OUTPUT_SILENCE_AUDIO_CBUFFER_SIZE);
   DeclareCBuffer($multi_chan_sil_secondary_right_out_cbuffer_struc,$multi_chan_sil_secondary_right_out,$OUTPUT_SILENCE_AUDIO_CBUFFER_SIZE);
   DeclareCBuffer($multi_chan_sil_aux_left_out_cbuffer_struc,$multi_chan_sil_aux_left_out,$OUTPUT_SILENCE_AUDIO_CBUFFER_SIZE);
   DeclareCBuffer($multi_chan_sil_aux_right_out_cbuffer_struc,$multi_chan_sil_aux_right_out,$OUTPUT_SILENCE_AUDIO_CBUFFER_SIZE);
   DeclareCBuffer($multi_chan_sil_sub_out_cbuffer_struc,$multi_chan_sil_sub_out,$OUTPUT_SILENCE_AUDIO_CBUFFER_SIZE);

   // Multi-channel CHAIN0 output silence cbops copy struc (dynamically constructed)
   .VAR chain0_sil_copy_struc[$MULTI_CHAN_CHAIN0_MAX_WIRED_CHANNELS + $MULTI_CHAN_CHAIN0_MAX_WIRED_CHANNELS + 3 + $cbops.AV_COPY_M_EXTEND_SIZE] =
      $chain0_processing_switch_op,                         // First operator block
      0 ...;                                                // Zero remaining struc elements (these are dynamically configured)

   // Multi-channel chain1 output silence cbops copy struc (dynamically constructed)
   .VAR chain1_sil_copy_struc[$MULTI_CHAN_CHAIN1_MAX_WIRED_CHANNELS + $MULTI_CHAN_CHAIN1_MAX_WIRED_CHANNELS + 3 + $cbops.AV_COPY_M_EXTEND_SIZE] =
      $chain1_processing_switch_op,                         // First operator block
      0 ...;                                                // Zero remaining struc elements (these are dynamically configured)

   .VAR $init_latency_state = 0;
   .VAR $init_latency_table[] = il_idle, il_wait, il_play, il_pause;
   .VAR $no_audio_counter = 0;

   $adjust_initial_latency:

   $push_rLink_macro;

   // no action if target latency has not been defined from vm
   r6 = 0;
   r7 = $M.multi_chan_output.chain0_copy_struc;
   r8 = $M.multi_chan_output.chain1_copy_struc;
   Null = M[$target_latency_from_vm];
   if Z jump init_latency_done;

   // feature enabled for A2DP (SBC/MP3/AAC) only
   r0 = M[$app_config.io];
   #ifdef SBC_ENABLE
      // Check the codec type and initialize accordingly
      null = r0 - $SBC_IO;
      if Z jump init_latency_start;
   #endif

   #ifdef MP3_ENABLE
      // Check the codec type and initialize accordingly
      null = r0 - $MP3_IO;
      if Z jump init_latency_start;
   #endif

   #ifdef AAC_ENABLE
      // Check the codec type and initialize accordingly
      null = r0 - $AAC_IO;
      if Z jump init_latency_start;

      null = r0 - $AAC_ELD_IO;
      if Z jump init_latency_start;
   #endif

   #ifdef APTX_ENABLE
      // Check the codec type and initialize accordingly
      null = r0 - $APTX_IO;
      if Z jump init_latency_start;

      null = r0 - $APTXHD_IO;
      if Z jump init_latency_start;
   #endif

   jump init_latency_done;
  init_latency_start:

   // jump to proper state handler
   r6 = M[$init_latency_state];
   r0 = M[$init_latency_table + r6];
   jump r0;

   il_idle:
   // idle state:
   // copy silence and
   // stay here until first packet received
   r7 = chain0_sil_copy_struc;
   r8 = chain1_sil_copy_struc;
   Null = M[$first_packet_received];
   if Z jump init_latency_done;

   il_wait:
   // wait state:
   // play silence and
   // wait until 'target_latency' has passed since receiving first packet
   // then go to play mode
   r6 = 1;
   r7 = chain0_sil_copy_struc;
   r8 = chain1_sil_copy_struc;
   r0 = M[$first_packet_time];
   r1 = M[$TIMER_TIME];
   r1 = r1 - r0;
   if NEG r1 = -r1;
   r0 = M[$target_latency_from_vm];
   r1 = r1 * 0.001(frac);
   Null = r1 - r0;
   if NEG jump init_latency_done;
   // init time passed, enable target latency
   M[$sra_struct +  $sra.TARGET_LATENCY_MS_FIELD] = r0;
   r0 = &$encoded_latency_struct + $encoded_latency.TOTAL_LATENCY_US_FIELD;
   M[$sra_struct +  $sra.CURRENT_LATENCY_PTR_FIELD] = r0;
   // and start play

   il_play:
   // play mode:
   // play audio
   // go tpo pause mode when decoder stalls
   M[$no_audio_counter] = 0;
   r6 = 2;
   r7 = $M.multi_chan_output.chain0_copy_struc;
   r8 = $M.multi_chan_output.chain1_copy_struc;
   Null = M[$decoder_codec_stream_struc + $codec.av_decode.CURRENT_RUNNING_MODE_FIELD];
   if NZ jump init_latency_done;
   r6 = 3;
   M[$first_packet_received] = 0;

   il_pause:
   // pause mode:
   // play remaining audio
   // if stall ends return back to play mode
   // if no audio to play go to idle and wait for new packets to arrive
   r7 = $M.multi_chan_output.chain0_copy_struc;
   r8 = $M.multi_chan_output.chain1_copy_struc;
   Null = M[$decoder_codec_stream_struc + $codec.av_decode.CURRENT_RUNNING_MODE_FIELD];
   if NZ jump il_play;

   r0 = &$multi_chan_primary_left_out_cbuffer_struc;
   call $cbuffer.calc_amount_data;
   r1 = M[$no_audio_counter];
   r1 = r1 + 1;
   Null = r0 - 2;
   if POS r1 = 0;
   M[$no_audio_counter] = r1;
   Null = r1 - 2;
   if NEG jump init_latency_done;
   M[$sra_struct +  $sra.TARGET_LATENCY_MS_FIELD] = 0; // move to pause?
   r6 = 0;
   jump il_idle;

   init_latency_done:
   // set audio or silence buffers to play on CHAIN0 and CHAIN1
   M[$multi_chain0_copy_struc_ptr] = r7;
   M[$multi_chain1_copy_struc_ptr] = r8;
   M[$init_latency_state] = r6;

   // pop rLink from stack
   jump $pop_rLink_and_rts;
.ENDMODULE;

#endif // SRA_TARGET_LATENCY

// *****************************************************************************
// MODULE:
//    $M.jitter_buffering
//
// DESCRIPTION:
//    Used to buffer data in front of frame processing to overcome jitter.
//    Currently used on USB input, but could be used on any input.
// ******************************************************************************
.MODULE $M.jitter_buffering;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   .VAR is_buffering = 1;

   $jitter_buffering:

   $push_rLink_macro;

   Null = M[is_buffering];
   if Z jump done;

   r0 = M[$app_config.io];
   Null = r0 - $USB_IO;
   if NZ jump clear_buffering_flag;

   // Set amount of jitter based on sampling rate.
   r0 = M[$current_codec_sampling_rate];
   Null = r0 - 48000;
   if NZ jump check_fs_44100;
   r4 = 0;  // 0 USB packets @ 48 kHz
   jump buffering_state;

check_fs_44100:
   Null = r0 - 44100;
   if NZ jump check_fs_32000;
   r4 = 132;  // 3 USB packets @ 44.1 kHz
   jump buffering_state;

check_fs_32000:
   Null = r0 - 32000;
   if NZ jump check_fs_22050;
   r4 = 0;  // 0 USB packets @ 32 kHz
   jump buffering_state;

check_fs_22050:
   Null = r0 - 22050;
   if NZ jump check_fs_16000;
   r4 = 132;  // 6 USB packets @ 22.050 kHz
   jump buffering_state;

check_fs_16000:
   Null = r0 - 16000;
   if NZ jump check_fs_8000;
   r4 = 0;  // 0 USB packets @ 16 kHz
   jump buffering_state;

check_fs_8000:
   Null = r0 - 8000;
   if NZ jump clear_buffering_flag;
   r4 = 0;  // 0 USB packets @ 8 kHz

buffering_state:
   // buffering state
   // stop buffering once extra data has been buffered
   r3 = M[$music_example.frame_processing_size];
   r3 = r3 + r4;
   r0 = $audio_out_left_cbuffer_struc;
   call $cbuffer.calc_amount_data;
   null = r0 - r3;
   if NEG jump done;

clear_buffering_flag:
   // we've buffered enough or don't require a jitter buffer.
   M[is_buffering] = Null;

done:
    // pop rLink from stack
    jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $usb_sub_esco_process
//
// DESCRIPTION:
//    called in sub esco timer handler to process and copy sub data
//
// *****************************************************************************
.MODULE $M.usb_sub_esco_process;
   .CODESEGMENT PM;
   .DATASEGMENT DM;
   $usb_sub_esco_process:

   // push rLink onto stack
   $push_rLink_macro;
   r0 = M[$sub_link_port];
   Null = r0 - $AUDIO_ESCO_SUB_OUT_PORT;
   if NZ jump not_esco_sub;
         call $M.Subwoofer.esco_preprocess;
         call $M.Subwoofer.esco_post_process;
   not_esco_sub:
   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.av_audio_out_silence_prime
//
// DESCRIPTION:
//    Prime the number of zero samples to insert
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
.MODULE $M.av_audio_out_silence_prime;
   .CODESEGMENT PM;

   $av_audio_out_silence_prime:

   // Prime the number of silence data samples (125msec)
   r0 = M[$current_codec_sampling_rate];
   r0 = r0 ASHIFT -3;
   M[$M.av_audio_out_silence_insert.left_zero_samples] = r0;
   M[$M.av_audio_out_silence_insert.right_zero_samples] = r0;

   rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.av_audio_out_silence_insert
//
// DESCRIPTION:
//    Insert silence samples into the audio_out buffers
//
// INPUTS:
//    none
//
// OUTPUTS:
//    none
//
// TRASHES: r0, r1, r2, r3, r4, r10, I0, L0, DoLoop
//
// *****************************************************************************
.MODULE $M.av_audio_out_silence_insert;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   .VAR left_zero_samples;
   .VAR right_zero_samples;

   $av_audio_out_silence_insert:

   // Push rLink onto stack
   $push_rLink_macro;

   // Insert silence data into the audio buffers (r3=zero count pointer, r4=cbuffer pointer)
   r3 = left_zero_samples;
   r4 = $audio_out_left_cbuffer_struc;
   call $av_insert_silence;

   r3 = right_zero_samples;
   r4 = $audio_out_right_cbuffer_struc;
   call $av_insert_silence;

   // Pop rLink from stack
   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.av_insert_silence
//
// DESCRIPTION:
//    Insert silence samples into buffer
//
// INPUTS:
//    r3 = pointer to number of samples to insert
//    r4 = cbuffer structure address
//
// OUTPUTS:
//    none
//
// TRASHES: (for a cbuffer) r0, r1, r2, r10, I0, L0, DoLoop
//
// *****************************************************************************
.MODULE $M.av_insert_silence;
   .CODESEGMENT PM;

   $av_insert_silence:

   // Push rLink onto stack
   $push_rLink_macro;

   // Check for buffer space (and use as number of samples to insert)
   r0 = r4;
   call $cbuffer.calc_amount_space;

   // Get the number of zero samples to insert
   r1 = M[r3];
   r10 = r1;

   // Calculate max(number of samples to insert, buffer space)
   null = r0 - r10;
   if NEG r10 = r0;

   // Calculate and save the number of zero samples still to insert
   r0 = r1 - r10;
   M[r3] = r0;

   // Get audio cbuffer write address
   r0 = r4;
   call $cbuffer.get_write_address_and_size;
   I0 = r0;
   L0 = r1;

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
//    $M.set_input_handler_timer
//
// DESCRIPTION:
//    Set the input handler timer according to the input resolution and input rate
//
// INPUTS:
//    none
//
// OUTPUTS:
//    none
//
// TRASHES: r0, r1, r2, r3
//
// *****************************************************************************
.MODULE $M.set_input_handler_timer;
   .CODESEGMENT SET_INPUT_HANDLER_TIMER_PM;

   $set_input_handler_timer:

   r1 = $M.main.handler_period_table_16bit;
   r2 = $M.main.handler_period_table_24bit;
   r3 = M[$inputResolutionMode];

   // Select the table according to the input resolution
   null = r3 - $RESOLUTION_MODE_24BIT;
   if NZ r2 = r1;

   r0 = M[r2+$INPUT_HANDLER_PERIOD_TABLE_LOW_RATE_INDEX];      // Fin <= 48kHz
   r1 = M[r2+$INPUT_HANDLER_PERIOD_TABLE_HIGH_RATE_INDEX];     // Fin > 48kHz

   // Configure according to the input sampling rate
   r3 = M[$current_codec_sampling_rate];
   Null = 48000 - r3;                                          // Input sampling rate > 48kHz?
   if NEG r0 = r1;                                             // Yes - use the hi-rate timer period

   M[$tmr_period_con_copy] = r0;

   rts;

.ENDMODULE;

