// *****************************************************************************
// Copyright (c) 2003 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
//
// *****************************************************************************

// ******************************************************************************************
// DESCRIPTION
//
//    This application plays audio streams coming from S/PDIF input
// ports. The input to the DSP app comes from two spdif LEFT and RIGHT ports,
// the SPDIF data can be either coded(IEC_61937 format) or PCM type. The coded
// data is decoded by proper decoder then played at the output interface. This
// application can also supports AC-3, MPEG2 AAC(stereo only) and MPEG1 MP3
// (stereo only) coded data types, any other coded data type will mute the output.
// Supporting codec data types must be enabled both in build time and in run time by VM.
//
// For AC-3 coded stream, The application can optionally mix the decoded LFE
// output channel into both output LEFT and RIGHT channels.
//
// Figure below shows how the spdif audio can be integrated into music manager
// system.
//                                                             +------------------+ LEFT output
//               +------------------+  PCM LEFT                |                  |----------->
// SPDIF IN LEFT |                  |---->------->+----------->|                  | RIGHT output
// ------------->|  SPDIF DECODE    | PCM RIGHT   |            |    Music         |----------->
//               |  PCM/IEC_61937   |---->--------|-->+------->|    Manager       |
// SPDIF IN RIGTH|  detection       |             |   |        |  (Time Domain)   | SUB output (optional)
// ------------->|                  |------->+    |   |        |                  |----------->
//               +------------------+ CODED  |    |   |        +-------^----------+
//                                    AUDIO  |    |   |                | LFE input
//       +-----------------------<-----------+    |   |                |
//       |                                        |   |                |
//       |                                        |   |                |
//       |                                        |   |                |
//       |                                        |   |                |
//       |                                        |   |                |
//       |        +------------------+            |   |                |
//       |        |                  | L          |   |                |
//       |        |                  |----------->+   |                |
//       |        |     DECODER      |                |                |
//       |        |                  |                |                |
//       |        |                  | R              |                |
//       +------->|                  |--------------->+                |
//  Coded Audio   | (5.1 to stereo   |                                 |
//                |    Downmix       |                                 |
//                |    for AC-3)     |                                 |
//                |                  | LFE (AC-3 only)                 |
//                |                  |------------------------>--------+
//                +------------------+
//
// ***********************************************************************************************

#include "frame_sync_library.h"
#include "music_example.h"
#include "spi_comm_library.h"
#include "mips_profile.h"
#include "codec_library.h"
#include "codec_decoder.h"
#include "audio_proc_library.h"
#include "core_library.h"
#include "cbops_library.h"
#include "spdif_library.h"
#include "multichannel_output.h"
#include "multichannel_output_macros.h"
#include "frame_sync_stream_macros.h"
#include "sr_adjustment.h"

#ifdef AC3_ENABLE
   #include "ac3_library.h"
#endif

#ifdef MP3_ENABLE
   #include "mp3_library.h"
#endif

#ifdef AAC_ENABLE
   #include "aac_library.h"
#endif

#if (defined(AC3_ENABLE)) && (!defined(AAC_ENABLE))
   // -- Required for using the fft library
   #define FFT_TWIDDLE_NEED_128_POINT
   #include "fft_twiddle.h"
#endif

// Run-time control of configuration
.MODULE $app_config;
   .DATASEGMENT DM;

// Plugin type set from VM at run-time
   .VAR io = $INVALID_IO;             // Control app i/o
.ENDMODULE;

.MODULE $M.main;
   .CODESEGMENT MAIN_PM;
   .DATASEGMENT DM;

   $main:

   // Allocate memory for cbuffer structures
   DeclareCBuffer($codec_in_cbuffer_struc,$codec_in,CODEC_CBUFFER_SIZE);
   DeclareCBuffer($audio_out_left_cbuffer_struc,$audio_out_left,AUDIO_CBUFFER_SIZE);
   DeclareCBuffer($audio_out_right_cbuffer_struc,$audio_out_right,AUDIO_CBUFFER_SIZE);
#if defined(AC3_ENABLE)
   DeclareCBuffer($audio_out_lfe_cbuffer_struc,$audio_out_lfe,AUDIO_CBUFFER_SIZE);
#endif // defined(AC3_ENABLE)
   DeclareCBuffer($spdif_in_cbuffer_struc,$spdif_in,SPDIF_CBUFFER_SIZE);

   // Variables to receive dac and codec sampling rates from the vm
   .VAR $current_dac_sampling_rate = 0;                              // sampling rate of DAC
   .VAR $current_codec_sampling_rate = 0;                            // sampling rate of spdif input
   .VAR $config_spdif_sink_vm_message_struc[$message.STRUC_SIZE];    // Message structure for VM_CONFIG_SPDIF_APP_MESSAGE_ID message
   .VAR $spdif_target_latency_setting = SPDIF_DEFAULT_LATENCY_MS;    // target latency
   .VAR $output_interface_operating_mode = -1;                       // slave output operation

   // Application resolution modes (default to 16bit mode)
   .VAR $inputResolutionMode = $RESOLUTION_MODE_16BIT;
   .VAR $procResolutionMode = $RESOLUTION_MODE_16BIT;
   .VAR $outputResolutionMode = $RESOLUTION_MODE_16BIT;

   // ANC mode
   .VAR $ancMode = $ANC_NONE;

   // Rate matching control variables
   .VAR $audio_if_mode;
   .VAR $max_clock_mismatch;
   .VAR $aux_input_stream_available;       // local pcm file is being mixed

   // ** allocate memory for timer structures **
   .VAR $spdif_in_timer_struc[$timer.STRUC_SIZE];
   .VAR $audio_out_timer_struc[$timer.STRUC_SIZE];
   .var $signal_detect_timer_struc[$timer.STRUC_SIZE];

  // Structure to measure output port consuming rate
   .VAR $calc_chain0_actual_port_rate_struc[$calc_actual_port_rate.STRUC_SIZE] =
      $MULTI_CHAN_PRIMARY_LEFT_OUT_PORT, // PORT_FIELD
      0 ...;

   // This is the codec type being used
   .VAR $codec_type = -1;

   // This is the codec config being used
   .VAR $codec_config = -1;

   // Output handler timer period (for writing output data)
   .VAR $tmr_period_audio_copy = TMR_PERIOD_AUDIO_COPY;

   .VAR $rate_match_disable = 0;
   .VAR $spdif_config_word;
   .VAR $spdif_stat_word;

   .VAR $spdif_stats[$music_example.CODEC_STATS_SIZE] =
      &$spdif_config_word,
      &$spdif_stat_word,
      $spdif_copy_struct + $spdif.frame_copy.UNSUPPORTED_BURST_COUNT_FIELD,
#ifdef AC3_ENABLE
     &$ac3dec_config_words +0,
     &$ac3dec_config_words +1,
#else
      &$M.system_config.data.ZeroValue,                        // CODEC_STAT2
      &$M.system_config.data.ZeroValue,                        // CODEC_STAT3
#endif
      &$M.system_config.data.ZeroValue,                        // CODEC_STAT4
      &$M.system_config.data.ZeroValue,                        // CODEC_STAT5
      &$M.system_config.data.ZeroValue,                        // CODEC_STAT6
      &$M.system_config.data.ZeroValue,                        // CODEC_STAT7
      &$M.system_config.data.ZeroValue,                        // CODEC_STAT8
      &$M.system_config.data.ZeroValue,                        // CODEC_STAT9
      &$M.system_config.data.ZeroValue,                        // CODEC_STATA
      &$M.system_config.data.ZeroValue;                        // CODEC_STATB


#ifdef AC3_ENABLE
// AC-3 specific stats
.VAR $ac3_stats[8] =
     $ac3dec_user_info_struct + $ac3dec.info.FRAME_LENGTH_FIELD,              // CODEC_STAT4
     $ac3dec_user_info_struct + $ac3dec.info.FRAME_ACMODE_FIELD,              // CODEC_STAT5
     $ac3dec_user_info_struct + $ac3dec.info.FRAME_DIALNORM_FIELD,            // CODEC_STAT6
     $ac3dec_user_info_struct + $ac3dec.info.FRAME_LFE_ON_FIELD,              // CODEC_STAT7
     $ac3dec_user_info_struct + $ac3dec.info.FRAME_DSURMOD_FIELD,             // CODEC_STAT8
     $ac3dec_user_info_struct + $ac3dec.info.FRAME_BSID_FIELD,                // CODEC_STAT9
     $ac3dec_user_info_struct + $ac3dec.info.TOTAL_CORRUPT_FRAME_FIELD,       // CODEC_STATA
     $ac3dec_user_info_struct + $ac3dec.info.FRAME_ANNEXD_INFO_FIELD;         // CODEC_STATB
#endif

#ifdef AAC_ENABLE
// AAC specific stats
.VAR $aac_stats[8] =
      &$aacdec.sf_index,                                // CODEC_STAT4
      &$aacdec.channel_configuration,                   // CODEC_STAT5
      &$aacdec.audio_object_type,                       // CODEC_STAT6
      &$aacdec.extension_audio_object_type,             // CODEC_STAT7
      &$aacdec.sbr_present,                             // CODEC_STAT8
      &$aacdec.mp4_frame_count,                         // CODEC_STAT9
      &$M.system_config.data.ZeroValue,                 // CODEC_STATA
      &$M.system_config.data.ZeroValue;                 // CODEC_STATB
#endif

#ifdef MP3_ENABLE
// MP3 specific stats
.VAR $mp3_stats[8] =
      &$mp3dec.mode,                                    // CODEC_STAT4
      &$mp3dec.framelength,                             // CODEC_STAT5
      &$mp3dec.bitrate,                                 // CODEC_STAT6
      &$mp3dec.frame_version,                           // CODEC_STAT7
      &$mp3dec.frame_layer,                             // CODEC_STAT8
      &$M.system_config.data.ZeroValue,                 // CODEC_STAT9
      &$M.system_config.data.ZeroValue,                 // CODEC_STATA
      &$M.system_config.data.ZeroValue;                 // CODEC_STATB
#endif

   // define spdif i   nput structure
   .VAR $spdif_copy_struct[$spdif.frame_copy.STRUC_SIZE] =
      $SPDIF_IN_LEFT_PORT,                                      //LEFT_INPUT_PORT_FIELD
      $SPDIF_IN_RIGHT_PORT,                                     //RIGHT_INPUT_PORT_FIELD
      &$spdif_in_cbuffer_struc,                                 //SPDIF_INPUT_BUFFER_FIELD
      &$audio_out_left_cbuffer_struc,                           //LEFT_PCM_BUFFER_FIELD
      &$audio_out_right_cbuffer_struc,                          //RIGHT_PCM_BUFFER_FIELD
      &$codec_in_cbuffer_struc,                                 //CODED_BUFFER_FIELD
      0,                                                        //SUPPORTED_CODEC_TYPES_BITS_FIELD
      0 ...;

   .VAR $spdif_stream_decode_struct[$spdif.stream_decode.STRUC_SIZE] =
      &$spdif_copy_struct,                                                   // SPDIF_FRAME_COPY_STRUCT_FIELD
      &$spdif_get_decoder_call_back,                                         // GET_DECODER_FUNCTION_PTR_FIELD
      &$spdif_codec_change_master_reset,                                     // MASTER_RESET_FUNCTION_FIELD
      $spdif.OUTPUT_INTERFACE_TYPE_NONE,                                     // OUTPUT_INTERFACE_TYPE_FIELD
      0,
      0,
      0,
      0,                                                                     // OUTPUT_INTERFACE_SETTING_RATE_FIELD
      $encoded_latency_struct,                                               // LATENCY_MEASUREMENT_STRUCT_FIELD
      0 ...;

      .VAR $spdif_sra_rate = 0;
      .VAR $sra_rate_addr = $spdif_sra_rate;

#if  defined(AC3_ENABLE) || defined(MP3_ENABLE) || defined(AAC_ENABLE)
   .VAR/DM1 $decoder_struct[$codec.DECODER_STRUC_SIZE] =
      &$codec_in_cbuffer_struc,                    // in cbuffer
      &$audio_out_left_cbuffer_struc,              // out left cbuffer
      &$audio_out_right_cbuffer_struc,             // out right cbuffer
      0 ...;
#endif

#ifdef AC3_ENABLE
   // user AC-3 decoder configuration
   .VAR $ac3dec_user_config[$ac3dec.config.STRUCT_SIZE] =
       0,                                   // cbuffer for output LFE channel
       $ac3dec.ACMOD_20,                    // the output mode, dictates how the input channel should be downmixed if needed
       $ac3dec.STEREO_LORO_MODE,            // stereo mode that should be used when downmixng to 2.0 output mode
       $ac3dec.DUAL_STEREO_MODE,            // dual mode
       $ac3dec.COMP_LINE_MODE,              // the compression mode that should be used for decoded audio samples
       $ac3dec.UNITY_SCALE,                 // optional compression scale
       $ac3dec.UNITY_SCALE,                 // optional compression scale
       $ac3dec.UNITY_SCALE,                 // pcm scale
       0 ...;

   // defining ac3 user info structure
   .VAR $ac3dec_user_info_struct[$ac3dec.info.STRUCT_SIZE];
   .VAR $config_ac3_decoder_vm_message_struc[$message.STRUC_SIZE];
   .VAR $ac3_user_info_request_vm_message_struc[$message.STRUC_SIZE];
   .VAR $ac3dec_reconfig_needed = 1;
   .VAR $ac3dec_config_words[2];
#endif

   // Rate matching data structure
   .VAR $spdif_sra_struct[$spdif_sra.STRUC_SIZE] =
       MAX_SPDIF_SRA_RATE,       //MAX_RATE_FIELD
       SPDIF_DEFAULT_LATENCY_MS, //TARGET_LATENCY_MS_FIELD
       &$encoded_latency_struct + $encoded_latency.TOTAL_LATENCY_US_FIELD, //CURRENT_LATENCY_PTR_FIELD
       0,                     //OFFSET_LATENCY_US_FIELD
       0 ...;

   .VAR/DMCIRC $encoded_packet_info[SPDIF_PACKET_BOUNDARY_INFO_SIZE];

   // packet info cbufer structure
   .VAR $encoded_packet_info_cbuffer_struc[$cbuffer.STRUC_SIZE] =
      LENGTH($encoded_packet_info),            // size
      &$encoded_packet_info,                   // read pointer
      &$encoded_packet_info;                   // write pointer

   .VAR $latency_calc_current_warp = $codec_rate_adj.stereo + $cbops.rate_adjustment_and_shift.Process.SRA_CURRENT_RATE_FIELD;

   // inverse of dac sample rate, used for latency calculation
   .VAR $inv_dac_fs = $latency.INV_FS(48000);

   // define latency structure, spdif needs coded structure
   .VAR $encoded_latency_struct[$encoded_latency.STRUC_SIZE] =
      $pcm_latency_input_struct,
      $encoded_packet_info_cbuffer_struc,
      $codec_in_cbuffer_struc,
      &$audio_out_left_cbuffer_struc,
      $spdif_copy_struct + $spdif.frame_copy.INV_SAMPLING_FREQ_FIELD, &$latency_calc_current_warp,
      0 ...;

    // define cbuffers structure involved in pcm latency
    .VAR cbuffers_latency_measure[] =
       $audio_out_left_cbuffer_struc, $spdif_copy_struct + $spdif.frame_copy.INV_SAMPLING_FREQ_FIELD, $latency_calc_current_warp,
       $codec_resamp_out_left_cbuffer_struc, $inv_dac_fs, $latency_calc_current_warp,
       $codec_rate_adj_out_left_cbuffer_struc, $inv_dac_fs, 0,
       $multi_chan_primary_left_out_cbuffer_struc, $inv_dac_fs, 0,
       $MULTI_CHAN_PRIMARY_LEFT_OUT_PORT, $inv_dac_fs, 0,
       0;

    // define samples structure involved in pcm latency
    .VAR samples_latency_measure[] =
       // place holder for delay buffer when sub is connected
       0, $inv_dac_fs, 0,
       0;

    // define pcm latency structure
    .VAR $pcm_latency_input_struct[$pcm_latency.STRUC_SIZE] =
      &cbuffers_latency_measure,
      &samples_latency_measure;

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
   // initialise the wallclock library
   call $wall_clock.initialise;
#if defined(DEBUG_ON)
   // initialise the profiler library
   call $profiler.initialise;
#endif
   // init DM_flash
   call $flash.init_dmconst;

   // intialize SPI communications library
   call $spi_comm.initialize;

#ifdef AC3_ENABLE
   // initialise ac3 decoder
   call $ac3dec.init_decoder;

   // set up message handler for VM_AC3_DECODER_CONFIG_MESSAGE_ID message
   r1 = &$config_ac3_decoder_vm_message_struc;
   r2 = VM_AC3_DECODER_CONFIG_MESSAGE_ID;
   r3 = &$config_ac3_decoder_message_handler;
   call $message.register_handler;

   // set up message handler for VM_AC3_USER_INFO_REQUEST_MESSGE_ID message
   r1 = &$ac3_user_info_request_vm_message_struc;
   r2 = VM_AC3_USER_INFO_REQUEST_MESSGE_ID;
   r3 = &$ac3_user_info_request_message_handler;
   call $message.register_handler;

#endif

#ifdef AAC_ENABLE
   // initialise aac decoder
   call $aacdec.init_decoder;
   r0 = &$aacdec.adts_read_frame;
   M[$aacdec.read_frame_function] = r0;
#endif

#ifdef MP3_ENABLE
   // initialise mp3 decoder
   call $mp3dec.init_decoder;;
#endif

   // set up message handler for VM_SET_TONE_RATE_MESSAGE_ID message
   r1 = &$set_tone_rate_from_vm_message_struc;
   r2 = VM_SET_TONE_RATE_MESSAGE_ID;
   r3 = &$set_tone_rate_from_vm;
   call $message.register_handler;

   // set up message handler for VM_CONFIG_SPDIF_APP_MESSAGE_ID message
   r1 = &$config_spdif_sink_vm_message_struc;
   r2 = VM_CONFIG_SPDIF_APP_MESSAGE_ID;
   r3 = &$config_spdif_sink_message_handler;
   call $message.register_handler;

   // Power Up Reset needs to be called once during program execution
   call $music_example.power_up_reset;

   r2 = $music_example.VMMSG.READY;
   r3 = $MUSIC_MANAGER_SYSID;
   // status
   r4 = M[$music_example.Version];
   r4 = r4 LSHIFT -8;
   call $message.send_short;

#if defined(WIRELESS_SUB_ENABLE)
   .VAR write_port_function_table[$cbuffer.NUM_PORTS] =
   #ifdef WIRELESS_SUB_ENABLE
      $M.Subwoofer.esco_port_connected,
      dummy,
   #else
      dummy,
      dummy,
   #endif
   dummy ...;

   .VAR write_port_disconnected_function_table[$cbuffer.NUM_PORTS] =
   #ifdef WIRELESS_SUB_ENABLE
      $M.Subwoofer.esco_port_disconnected,
      dummy,
   #else
      dummy,
      dummy,
   #endif
   dummy ...;

   r0 = write_port_connected_handler;
   M[$cbuffer.write_port_connect_address] = r0;

   r0 = write_port_disconnected_handler;
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
      // write port other than subwoofer
      rts;

   skip_port_handlers:
#endif

   // tell vm we're ready and wait for the go message
   call $message.send_ready_wait_for_go;

   // Defer processing of VM messages until channel configuration is completed
   call $block_interrupts;

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

   // Channel complete so allow processing of VM messages
   call $unblock_interrupts;

   // Set the output handler timer according to the output rate
   call $set_output_handler_timer;

   // start timer that copies output samples
   r1 = &$audio_out_timer_struc;
   r2 = M[$tmr_period_audio_copy];
   r3 = &$audio_out_copy_handler;
   call $timer.schedule_event_in;

   // Start a timer that copies spdif input
   r1 = &$spdif_in_timer_struc;
   r2 = TMR_PERIOD_SPDIF_COPY;
   r3 = &$spdif_in_copy_handler;
   call $timer.schedule_event_in;

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

    // continually decode codec frames
    frame_loop:

      // Need to dynamically set the codec rate according to the SPDIF input rate
      // (this is done in the main foreground loop for SPDIF multi-channel)

      // Update dac sampling rate
      r3 = 48000;
      r0 = M[$spdif_stream_decode_struct + $spdif.stream_decode.OUTPUT_INTERFACE_CURRENT_RATE_FIELD];
      if Z r0 = r3;
      r1 = M[$current_dac_sampling_rate];
      M[$current_dac_sampling_rate] = r0;
      r1 = r1 - r0;

      // Update codec sampling rate
      r3 = r0;
      r0 = M[$spdif_copy_struct + $spdif.frame_copy.SAMPLING_FREQ_FIELD];
      if Z r0 = r3;  // Default to the output rate if input rate is zero
      r2 = M[$current_codec_sampling_rate];
      M[$current_codec_sampling_rate] = r0;
      r2 = r2 - r0;

      // Reconfigure the codec resampler if needed
      r2 = r2 OR r1;
      if Z jump resample_config_done;

         // Set the output handler timer according to the output rate
         call $set_output_handler_timer;

         r0 = M[$current_dac_sampling_rate];
         call $latency.calc_inv_fs;
         M[$inv_dac_fs] = r0;

         // Configure the codec resampler according to the codec and DAC sampling rates and processing resolution mode
         r7 = M[$procResolutionMode];
         call $codec_resampler.config;

         // Configure which buffers are to be used for rate adjustment and resampling
         call $codec_rate_adj.config_buffers;

         // configure latency limits
         call $config_spdif_latency;

      resample_config_done:

      #ifdef AC3_ENABLE
         // configure AC-3 decoder
         call $block_interrupts;
         r0 = $ac3dec_user_config;
         r1 = $ac3dec_user_info_struct;
         Null = M[$ac3dec_reconfig_needed];
         if NZ call $ac3dec.set_user_config;
         M[$ac3dec_reconfig_needed] = 0;
         call $unblock_interrupts;
      #endif

       // Check Communication
       call $spi_comm.polled_service_routine;

       // Start profiler
       r8 = &$DecoderMips_data_block;
       call $M.mips_profile.mainstart;

       // run spdif decode
       r5 = $spdif_stream_decode_struct;
       call $spdif.decode;

#if defined(AC3_ENABLE)
       // Synchronize LFE to L/R channels
       call $syncronise_lfe_to_LR_channels;
#endif // defined(AC3_ENABLE)

        // run resampler if needed
        Null = M[$codec_resampler.resampler_active];
        if NZ call $codec_resampler.run_resampler;

       // Codec rate adjustment
       // copy target rate so both L/R and LFE use the same adjust rate
       // to stay syncronised
       r0 = M[$spdif_sra_struct + $spdif_sra.SRA_RATE_FIELD];
       M[$spdif_sra_rate] = r0;
       // run sw rate adjustment if needed
       Null = M[$chain0_hw_warp_enable];
       if Z call $codec_rate_adj.run_rate_adjustment;

       // Stop profiler
       r8 = &$DecoderMips_data_block;
       call $M.mips_profile.mainend;

       // Store Decoder MIPS
       r0 = M[r8 + $mips_profile.MIPS.MAIN_CYCLES_OFFSET];
       M[&$music_example.PeakMipsDecoder] = r0;

       // Synchronize frame process to audio
       // (Reduce power consumption and calculate total MIPS)
       Null = M[$spdif_stream_decode_struct + $spdif.stream_decode.MODE_FIELD];
       if NZ call $SystemSleepAudio;

       // data/space check only on left in/out primary buffers. These buffers are
       // assumed always active. The system around music manager guarantees
       // synchronized outputs
       // Get the frame size to be used in the Music Manager
       r3 = M[$music_example.frame_processing_size];

       r0 = M[$M.system_config.data.stream_map_left_in + $framesync_ind.CBUFFER_PTR_FIELD];
       call $cbuffer.calc_amount_data;
       null = r0 - r3;
       if NEG jump frame_loop;

#if defined(AC3_ENABLE)
       r0 = M[$M.system_config.data.stream_map_lfe_in + $framesync_ind.CBUFFER_PTR_FIELD];
       call $cbuffer.calc_amount_data;
       null = r0 - r3;
       if NEG jump frame_loop;
#endif // defined(AC3_ENABLE)

       // check for space
       r0 = $multi_chan_primary_left_out_cbuffer_struc;
       call $cbuffer.calc_amount_space;
       null = r0 - r3;
       if NEG jump frame_loop;

       call $music_example_process;

#ifdef WIRELESS_SUB_ENABLE
      // downsample the subwoofer audio and copy it to the port.
      call $M.Subwoofer.transmit_wireless_subwoofer_audio;
#endif


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
// *****************************************************************************
.MODULE $M.audio_out_copy_handler;
   .CODESEGMENT AUDIO_OUT_COPY_HANDLER_PM;
   .DATASEGMENT DM;

   $audio_out_copy_handler:

   // push rLink onto stack
   $push_rLink_macro;

   .VAR $multi_chain0_copy_struc_ptr = $M.multi_chan_output.chain0_copy_struc;
   .VAR $multi_chain1_copy_struc_ptr = $M.multi_chan_output.chain1_copy_struc;

   // If not using cbops.stream_copy, need to manually
   // reset the sync flag.
   M[$frame_sync.sync_flag] = Null;

   // Calculate the actual consuming rate of the output interface, for master outputs
   // this shall be the same as nominal output rate, but for slave mode this might
   // be slightly different. This routine allows the rate matching code to cope
   // with 'slave' output devices where the sample rate is driven by an external clock
   r8 = $calc_chain0_actual_port_rate_struc;
   call $calc_actual_port_rate;

   // adjust sra expected rate for resampler
   r0 = M[$calc_chain0_actual_port_rate_struc + $calc_actual_port_rate.SAMPLE_RATE_FIELD];
   if NZ call $apply_sra_resampling_adjustment;

   // set the warp value to be used for latency caluclation
   r8 = $codec_rate_adj.stereo + $cbops.rate_adjustment_and_shift.Process.SRA_CURRENT_RATE_FIELD;
   r7 = $hw_warp_struct + $hw_warp.CURRENT_RATE_FIELD;
   Null = M[$chain0_hw_warp_enable];
   if NZ r8 = r7;
   M[$latency_calc_current_warp] = r8;

   // calculate chain1 actual consuming rate
   r8 = $calc_chain1_actual_port_rate_struc;
   Null = M[$M.multi_chan_output.num_chain1_channels];
   if NZ call $calc_actual_port_rate;

   // care for initial latency
   call $adjust_initial_latency;

   // Clone the tone cbuffers to allow multiple sinks of tone data from the same tone data buffers
   call $multi_chan_clone_tone_cbuffers;

   // If there are enabled CHAIN0 channels, copy to output ports using multi-channel cbops chains
   r8 = $M.multi_chan_output.chain0_copy_struc;
   null = M[$M.multi_chan_output.num_chain0_channels];
   if NZ call $cbops.dac_av_copy_m;

   // If there are enabled DAC channels, copy to output ports using multi-channel cbops chains
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
//    $spdif_pause_detect
//
// DESCRIPTION:
//   when spdif stream is paused for any reason this function
//   generate some silence enough to push all the current audio
//   into the output interface.
// INPUT:
//   r7 = amount of data read from ports in last read
// TRASHED REGISTERS
//   r0-r4, r10, rMAC, DoLoop, I0
// *****************************************************************************
.MODULE $M.spdif_pause_detect;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   .VAR $spdif_silence_to_insert;
   .VAR $spdif_pause_timer;

   $spdif_pause_detect:

   // push rLink onto stack
   $push_rLink_macro;

   // r7 = 0 means no data read from spdif input ports
   Null = r7;
   if NZ jump spdif_stream_active;

   // see if pause threshold has reached
   r0 = M[$spdif_pause_timer];
   r0 = r0 + TMR_PERIOD_SPDIF_COPY;
   Null = r0 - SPDIF_PAUSE_THRESHOLD_US;
   if POS jump insert_silence;

   // pause time not passed the threshold yet
   M[$spdif_pause_timer] = r0;
   jump end;

   insert_silence:
   // pause detected
   r3 = M[$spdif_copy_struct + $spdif.frame_copy.SAMPLING_FREQ_FIELD];
   if Z jump end;
   r4 = M[$spdif_silence_to_insert];
   if LE jump end;

   // if buffer is almost full skip this copy
   r0 = M[$spdif_copy_struct + $spdif.frame_copy.SPDIF_INPUT_BUFFER_FIELD];
   call $cbuffer.calc_amount_space;
   r0 = r0 - $spdif.INPUT_BUFFER_MIN_SPACE;
   if NEG jump end;

   // work out how much silence to add this time
   rMAC = r3 * TMR_PERIOD_SPDIF_COPY;
   rMAC = rMAC ASHIFT 5 (56bit);
   r10 = rMAC * 0.52428799999999998(frac);
   r2 = 250000;
   rMAC = r2 * r10;
   Div = rMAC / r3;
   r3 = DivResult;
   M[$spdif_silence_to_insert] = r4 - r3;
   r10 = MIN r0;
   // even number of samples (L+R)
   r10 = r10 AND (~0x1);
   if Z  jump end;

   // insert silence to buffer, r10 samples
   r0 = M[$spdif_copy_struct + $spdif.frame_copy.SPDIF_INPUT_BUFFER_FIELD];
   call $cbuffer.get_write_address_and_size;
   I0 = r0;
   L0 = r1;
   r0 = 0;
   do sil_loop;
      M[I0, 1] = r0;
   sil_loop:
   L0 = 0;
   r0 = &$spdif_in_cbuffer_struc;
   r1 = I0;
   call $cbuffer.set_write_address;
   jump end;

   spdif_stream_active:
   // stream is active, reset threshold
   M[$spdif_pause_timer] = 0;
   r0 = (SPDIF_PAUSE_SILENCE_TO_ADD_MS*1000);
   M[$spdif_silence_to_insert] = r0;

   end:
   // pop rLink from stack
   jump $pop_rLink_and_rts;

.ENDMODULE;


// *****************************************************************************
// MODULE:
//    $spdif_in_copy_handler
//
// DESCRIPTION:
//    Function called on a timer interrupt to perform the copying of data
//    from the spdif input ports
//
// *****************************************************************************
.MODULE $M.spdif_in_copy_handler;
   .CODESEGMENT SPDIF_IN_COPY_HANDLER_PM;
   .DATASEGMENT DM;

   $spdif_in_copy_handler:

   // push rLink onto stack
   $push_rLink_macro;

   // read raw data from spdif LEFT and RIGHT ports
   r8 = &$spdif_copy_struct;
   call $spdif.copy_raw_data;

   // Note: r7 = amount copied,
   // used by next functions

   // set some variables used to adjust initial latency
   .VAR $first_audio_received;           // flag showing first valid SPDIF input audio received after an inactivity period
   .VAR $first_audio_time;               // the time of receiving first audio samples
   .VAR $first_audio_init_latency_offset;// latency ahead of first audio burst
   // mark the arrival of first packet
   Null = M[$first_audio_received];
   if NZ jump first_audio_done;
      Null = r7;  // new data received?
      if Z jump first_audio_done;
         // set the flag
         r0 = 1;
         M[$first_audio_received] = r0;
         // set arrival time
         r2 = M[$TIMER_TIME];
         M[$first_audio_time] = r2;
         r2 = M[$encoded_latency_struct + $encoded_latency.TOTAL_LATENCY_US_FIELD];
         M[$first_audio_init_latency_offset] = r0;
   first_audio_done:

   // insert some silence if
   // pause detected
   call $spdif_pause_detect;

   // update stat word for UFE
   call $spdif_update_stat_word;

   // calculate rate
   Null = M[$rate_match_disable];
   if Z call $spdif_sra.calc_rate;

   // apply hw rate matching
   r0 = M[$chain0_hw_warp_enable];
   r0 = r0 + M[$chain1_hw_warp_enable];
   if NZ call $apply_hardware_warp_rate;

   // just for debugging
   .VAR $codec_level;
   r0 = &$codec_in_cbuffer_struc;
   call $cbuffer.calc_amount_data;
   M[$codec_level] = r0;

   // post another timer event
   r1 = &$spdif_in_timer_struc;
   r2 = TMR_PERIOD_SPDIF_COPY;
   r3 = &$spdif_in_copy_handler;
   call $timer.schedule_event_in_period;

   // pop rLink from stack
   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $config_spdif_latency
//
// DESCRIPTION: Configure the latency limits
//
// INPUTS:
//  - none
//
// OUTPUTS:
//  - none
//
// *****************************************************************************
.MODULE $M.config_spdif_latency;
   .CODESEGMENT CONFIG_SPDIF_LATENCY_PM;

   $config_spdif_latency:

   // r3 = minimum target latency
   r3 = MIN_SPDIF_LATENCY_MS;
   #ifdef WIRELESS_SUB_ENABLE
      // if esco sub is connected then timing is controlled by sco connection
      r0 = M[$sub_link_port];
      Null = r0 - $AUDIO_ESCO_SUB_OUT_PORT;
      if NZ jump sub_check_done;
      // extra latency happens when sub is connected
      r3 = MIN_SPDIF_LATENCY_WITH_SUB_MS;
   sub_check_done:
   #endif // WIRELESS_SUB_ENABLE

   // check minimum target latency
   r0 = M[$spdif_target_latency_setting];
   r0 = MAX r3;

   // check maximum target latency
   r3 = MAX_SPDIF_LATENCY_PCM_MS;
   r2 = MAX_SPDIF_LATENCY_CODED_MS;
   r1 = M[$spdif_stream_decode_struct + $spdif.stream_decode.CURRENT_CODEC_TYPE_FIELD];
   Null = r1 - $spdif.DECODER_TYPE_PCM;
   if GT r3 = r2;
   r0 = MIN r3;

   // set target latency
   M[$spdif_sra_struct + $spdif_sra.TARGET_LATENCY_MS_FIELD] = r0;

   rts;
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

#if defined(AC3_ENABLE)
// *****************************************************************************
// MODULE:
//    $syncronise_lfe_to_LR_channels
//
// DESCRIPTION:
//    The LFE channel might become available and unavailable in run time, for example
//    when switching from PCM to coded and vice versa, this function makes sure
//    that the LFE is always synchronised to the LEFT and RIGHT channels
//
// *****************************************************************************
.MODULE $M.syncronise_lfe_to_LR_channels;
   .CODESEGMENT SYNCRONISE_LFE_TO_LR_CHANNELS_PM;
   .DATASEGMENT DM;

$syncronise_lfe_to_LR_channels:

   // push rLink onto stack
   $push_rLink_macro;

   // see if LFE has data
   r0 = $audio_out_lfe_cbuffer_struc;
   call $cbuffer.calc_amount_data;
   r6 = r0;

   // amount of data in left input buffer
   r0 = $audio_out_left_cbuffer_struc;
   call $cbuffer.calc_amount_data;
   r7 = r0;

   // amount of data in right input buffer
   r0 = $audio_out_right_cbuffer_struc;
   call $cbuffer.calc_amount_data;
   r7 = MIN r0;

   // already synced?
   r10 = r7 - r6;
   if Z jump $pop_rLink_and_rts;

   // Move LFE read pointer, write pointer
   // is always synchronised
   r0 = $audio_out_lfe_cbuffer_struc;
   call $cbuffer.get_read_address_and_size;
   I0 = r0;
   L0 = r1;
   M0 = -r10;
   r0 = M[I0, M0];
   r6 = I0;

   // in case of increase, new
   // samples must be silenced
   Null = r10;
   if NEG jump set_buf_address;

   // insert silence
   r0 = 0;
   do insert_silence;
      M[I0, 1] = r0;
   insert_silence:
   set_buf_address:
   r0 = $audio_out_lfe_cbuffer_struc;
   r1 = r6;
   call $cbuffer.set_read_address;

   L0 = 0;

   // pop rLink from stack
   jump $pop_rLink_and_rts;

.ENDMODULE;
#endif // defined(AC3_ENABLE)

// *****************************************************************************
// MODULE:
//   $spdif_get_decoder_call_back
//
// DESCRIPTION:
//    Call back function for providing decoder functions to SPDIF library. The
//    function is called by the spdif library when the coded data changes, it
//    should provide the library with proper decoder function, so the library
//    can decode the coded data properl.
//
// INPUTS:
//    r1 = coded data type
//
//  OUTPUTS:
//     r0 = decoder input structure
//     r1 = decoder frame decode function
//     r2 = decoder reset function
//
//  NOTES:
//     decoder library is not initialised by spdif library, application needs
//     to make sure it is initialised before passing info to spdif library
//
// *****************************************************************************
.MODULE $M.spdif_get_decoder_call_back;
   .CODESEGMENT SPDIF_GET_DECODER_CALL_BACK_PM;
   .DATASEGMENT DM;

   $spdif_get_decoder_call_back:
   #ifdef AC3_ENABLE
      // r1 = codec type:
      Null = r1 - $spdif.DECODER_TYPE_AC3;
      if NZ jump ac3_type_checked;
      I2 = $ac3_stats;
      r0 = &$decoder_struct;        //  decoder input struture
      r1 = &$ac3dec.frame_decode;   //  decoder frame decode function
      r2 = &$ac3dec.reset_decoder;  //  decoder reset function
      jump update_stats;
      ac3_type_checked:
   #endif

   #ifdef AAC_ENABLE
      // r1 = codec type:
      Null = r1 - $spdif.DECODER_TYPE_MPEG2_AAC;
      if NZ jump aac_type_checked;
      I2 = $aac_stats;
      r0 = &$decoder_struct;        //  decoder input struture
      r1 = &$aacdec.frame_decode;    //  decoder frame decode function
      r2 = &$aacdec.reset_decoder;  //  decoder reset function
      jump update_stats;
      aac_type_checked:
   #endif

   #ifdef MP3_ENABLE
      // r1 = codec type:
      Null = r1 - $spdif.DECODER_TYPE_MPEG1_LAYER23;
      if NZ jump mp3_type_checked;
      I2 = $mp3_stats;
      r0 = &$decoder_struct;        //  decoder input struture
      r1 = &$mp3dec.frame_decode;    //  decoder frame decode function
      r2 = &$mp3dec.reset_decoder;  //  decoder reset function
      jump update_stats;
      mp3_type_checked:
   #endif

   // we don't support any other codecs
   call $error;

   update_stats:
   // update the structure to use codec specific stats
   I6 = &$M.system_config.data.StatisticsPtrs + $M.MUSIC_MANAGER.STATUS.CODEC_FS_OFFSET + 5;
   r10 = 8;
   do update_codec_stars;
      r4 = M[I2, 1];
      M[I6, 1] = r4;
   update_codec_stars:
   rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//   $fade_out_buffer
// DESCRIPTION:
//   utility function, to apply a fade out on the remaining samples in a buffer
//
// INPUT:
//    r3 = input cbuffer
// *****************************************************************************
.MODULE $M.fade_out_buffer;
   .CODESEGMENT FADE_OUT_BUFFER_PM;
   .DATASEGMENT DM;

$fade_out_buffer:
   // push rLink onto stack
   $push_rLink_macro;
   #define pi 3.1415926535897931
   .VAR coefs[] = cos(2*pi/4.0/16),  cos(2*pi/4.0/32),  cos(2*pi/4.0/48),  cos(2*pi/4.0/64),
                  cos(2*pi/4.0/80),  cos(2*pi/4.0/96),  cos(2*pi/4.0/112), cos(2*pi/4.0/128),
                  cos(2*pi/4.0/144), cos(2*pi/4.0/160), cos(2*pi/4.0/176), cos(2*pi/4.0/192),
                  cos(2*pi/4.0/208), cos(2*pi/4.0/224), cos(2*pi/4.0/240), cos(2*pi/4.0/256);

   // see how much data is available for fading out
   r0 = r3;
   call $cbuffer.calc_amount_data;
   r4 = r0;

   r0 = r3;
   call $cbuffer.get_write_address_and_size;
   I4 = r0;
   L4 = r1;

   // maximum 256 samples to fade
   r2 = r4 LSHIFT -4;
   r1 = r2 - 15;
   if POS r2 = r2 - r1;
   r0 = r4 - 256;
   if POS r4 = r4 - r0;
   r10 = r4;
   if Z jump end_fade;

   M0 = Null - r4;
   r0 = M[I4,M0];

   // get the filter coef (c)
   r2 = M[r2 + coefs];
   r3 = 1.0;
   rMAC = r3;
   r0 = r2;
   r1 = M[I4,0];

   do fade_loop;
       // apply fading
       r4 = rMAC * rMAC (frac);
       r1 = r1 * r4 (frac);
       // calculate next sample fading coeff
       // alpha[n] = 2*alpha[n-1]*c - alpha[n-2]
       r4 = rMAC;
       rMAC = rMAC * r2,  M[I4,1] = r1;
       rMAC = rMAC + rMAC*r3;
       rMAC = rMAC - r0*r3, r1 = M[I4,0];
       r0 = r4;
    fade_loop:

   end_fade:
   L4 = 0;

   // pop rLink from stack
   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//   $spdif_codec_change_master_reset
// DESCRIPTION:
//   external reset function when codec is changed in the spdif stream, or
//   switch betweeen PCM and coded data happens
//
// *****************************************************************************
.MODULE $M.spdif_codec_change_master_reset;
   .CODESEGMENT SPDIF_CODEC_CHANGE_MASTER_RESET_PM;
   .DATASEGMENT DM;

$spdif_codec_change_master_reset:

   // push rLink onto stack
   $push_rLink_macro;

   // block interrupts
   call $block_interrupts;

   // fade out left buffer
   r3 = $audio_out_left_cbuffer_struc;
   call $fade_out_buffer;

   // fade out right buffer
   r3 = $audio_out_right_cbuffer_struc;
   call $fade_out_buffer;

#if defined(AC3_ENABLE)
   // fade out lfe buffer
   r3 = $audio_out_lfe_cbuffer_struc;
   call $fade_out_buffer;
#endif // defined(AC3_ENABLE)

   // unblock interrupts
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

   // clear spdif input buffer
   r0 = M[$spdif_in_cbuffer_struc + $cbuffer.WRITE_ADDR_FIELD];
   M[$spdif_in_cbuffer_struc + $cbuffer.READ_ADDR_FIELD] = r0;

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

   rts;

.ENDMODULE;


// *****************************************************************************
// MODULE:
//   $spdif_apply_realtime_config
// DESCRIPTION:
//  Not all spdif config parameters can change on the fly, at the moment only
//  target latency and supported codecs.
//
// Inputs:
//    r1: bit 7:0  -> Supported codec types
//        bit 15:8 ->  target latency in ms
//    r2: ac3 config word 1
//    r3: ac3 config word 2
// Outputs:
//    None
//
// *****************************************************************************
.MODULE $M.spdif_apply_realtime_config;
   .CODESEGMENT SPDIF_APPLY_REALTIME_CONFIG_PM;
   .DATASEGMENT DM;

$spdif_apply_realtime_config:

   // push rLink onto stack
   $push_rLink_macro;

   // extract target latency
   r0 = r1 LSHIFT -8;
   r0 = r0 AND 0xFF;
   M[$spdif_target_latency_setting] = r0;

   // set the supported data types
   r5 = 0;
   #ifdef AC3_ENABLE
      // Enable AC-3 data type if requested by VM
      r0 = $spdif.DATA_TYPE_SUPPORT($spdif.IEC_61937_DATA_TYPE_AC3);
      Null = r1 AND 1;
      if NZ r5 = r5 OR r0;
   #endif
   #ifdef AAC_ENABLE
      // Enable MPEG2 AAC data type if requested by VM
      r0 = $spdif.DATA_TYPE_SUPPORT($spdif.IEC_61937_DATA_TYPE_MPEG2_AAC);
      Null = r1 AND 2;
      if NZ r5 = r5 OR r0;
   #endif
   #ifdef MP3_ENABLE
      // Enable MP3(MPEG1 Layer3 only) data type if requested by VM
      r0 = $spdif.DATA_TYPE_SUPPORT($spdif.IEC_61937_DATA_TYPE_MPEG1_LAYER23);
      Null = r1 AND 4;
      if NZ r5 = r5 OR r0;
   #endif
   M[$spdif_copy_struct + $spdif.frame_copy.SUPPORTED_CODEC_TYPES_BITS_FIELD] = r5;

#ifdef AC3_ENABLE
   // configure AC3 decoder
   r1 = r2;
   r2 = r3;
   call $config_ac3_decoder_message_handler;
#endif

   // update spdif config word for UFE
   call $spdif_update_config_word;

   // pop rLink from stack
   jump $pop_rLink_and_rts;

.ENDMODULE;
// *****************************************************************************
// MODULE:
//   $spdif_update_stat_word
// DESCRIPTION:
//  SPDIF status is sent compressed to UFE, this function builds
//  the stat word
//
// INPUTS:
//    None
//
// OUTPUTS:
//    None
//
// TRASHED REGISTERS
//   r0, r1
// *****************************************************************************
.MODULE $M.spdif_update_stat_word;
   .CODESEGMENT SPDIF_UPDATE_STAT_WORD_PM;
   .DATASEGMENT DM;

$spdif_update_stat_word:

   // valid flag b0, 0 -> valid, 1 -> invalid
   r1 = M[$spdif_copy_struct + $spdif.frame_copy.STREAM_INVALID_FIELD];
   r1 = r1 AND 1;

   // data type, bit 7:1
   r0 = M[$spdif_stream_decode_struct + $spdif.stream_decode.CURRENT_CODEC_TYPE_FIELD];
   r0 = r0 AND 0x7F;
   r0 = r0 LSHIFT 1;
   r1 = r1 OR r0;

   // current latency in ms, bit 15:8
   r0 = M[$encoded_latency_struct + $encoded_latency.TOTAL_LATENCY_US_FIELD];
   r0 = r0 * 0.001(frac);
   r0 = r0 AND 0xFF;
   r0 = r0 LSHIFT 8;
   r1 = r1 OR r0;

   // channel status, bit 23:16
   r0 = M[$spdif_copy_struct + $spdif.frame_copy.CHSTS_FIRST_WORD_FIELD];
   r2 = r0 LSHIFT -8;
   r2 = r2 AND 0x80;
   r0 = r0 AND 0x7F;
   r0 = r0 OR r2;
   r0 = r0 LSHIFT 16;
   r1 = r1 OR r0;

   // save stat
   M[$spdif_stat_word] = r1;
   rts;
.ENDMODULE;

// *****************************************************************************
// MODULE:
//   $spdif_update_config_word
// DESCRIPTION:
//  SPDIF config is sent compressed to UFE, this function builds
//  the config word
//
// INPUTS:
//    None
//
// OUTPUTS:
//    None
//
// TRASHED REGISTERS
//   r0, r1
// *****************************************************************************
.MODULE $M.spdif_update_config_word;
   .CODESEGMENT SPDIF_UPDATE_CONFIG_WORD_PM;
   .DATASEGMENT DM;

$spdif_update_config_word:

   // target latency
   r0 = M[$spdif_target_latency_setting];
   r0 = r0 LSHIFT 8;

   // set the supported data types
   r2 = M[$spdif_copy_struct + $spdif.frame_copy.SUPPORTED_CODEC_TYPES_BITS_FIELD];
   #ifdef AC3_ENABLE
      // see if AC-3 enabled
      r1 = 1;
      Null = r2 AND $spdif.DATA_TYPE_SUPPORT($spdif.IEC_61937_DATA_TYPE_AC3);
      if NZ r0 = r0 OR r1;
   #endif

   #ifdef AAC_ENABLE
      // see if AAC enabled
      r1 = 2;
      Null = r2 AND $spdif.DATA_TYPE_SUPPORT($spdif.IEC_61937_DATA_TYPE_MPEG2_AAC);
      if NZ r0 = r0 OR r1;
   #endif

   #ifdef MP3_ENABLE
      // see if MP3 enabled
      r1 = 4;
      Null = r2 AND $spdif.DATA_TYPE_SUPPORT($spdif.IEC_61937_DATA_TYPE_MPEG1_LAYER23);
      if NZ r0 = r0 OR r1;
   #endif

   // update config word
   M[$spdif_config_word] = r0;
   rts;
.ENDMODULE;
// *****************************************************************************
// MODULE:
//   $config_spdif_sink_message_handler
// DESCRIPTION:
//   handling of VM_CONFIG_SPDIF_APP_MESSAGE message received from VM
//
// INPUTS:
//    r1 = config word 0
//         bit0      -> enable AC-3 data type support
//         bit1      -> enable MPEG2 AAC data type support
//         bit2      -> enable MP3 data type support
//         bits 3:4  -> reserved for future more data type support
//         bits 5:6  -> primary output interface type
//                      (note: this parameter is only needed for legacy stereo operation to allow input rate tracking
//                      it is DEPRECATED in the multi-channel implementation when resampling is used)
//                      0 -> none
//                      1 -> DAC
//                      2 -> I2S
//                      3 -> SPDIF
//         bit 7     -> fix output rate
//                      0 -> output rate follows input rate
//                      1 -> output rate always set at 48KHz
//         bit 8:15  -> desired latency in ms
//
//    r2 = config word 1
//         bit0      -> if set, vm will receive DSP_SPDIF_EVENT_MESSAGE
//                      whenever an interesting event happens, interesting
//                      events are:
//                         - spdif becomes active
//                         - spdif becomes inactive
//                         - data type of spdif stream changes
//                         - sample rate of spdif stream changes
//                       other changes won't trigger an event
//
//         bit 1:7:  -> delay time for reporting invalid stream (in seconds)
//         bit 8:    -> if set means disable rate matching
//         [bit 9:   -> DEPRECATED in multi-channel implementation
//                      Output interface operation mode
//                      0 -> Slave
//                      1 -> Master]
//        bit 10: -> input sample width
//                   0 -> 16 bit input
//                   1 -> 24 bit input
//
//   r3 = Output Interface Sample Rate
//   r4 = config word 3 (reserved)
//
//  OUTPUTS:
//     None
//
// TRASHED REGISTERS:
//    Assume everything
//
// *****************************************************************************
.MODULE $M.config_spdif_sink_message_handler;
   .CODESEGMENT CONFIG_SPDIF_SINK_MESSAGE_HANDLER_PM;
   .DATASEGMENT DM;

$config_spdif_sink_message_handler:

   // push rLink onto stack
   $push_rLink_macro;

   r1 = r1 AND 0xFFFF;

   r5 = 0;
   #ifdef AC3_ENABLE
      // Enable AC-3 data type if requested by VM
      r0 = $spdif.DATA_TYPE_SUPPORT($spdif.IEC_61937_DATA_TYPE_AC3);
      Null = r1 AND 1;
      if NZ r5 = r5 OR r0;
   #endif
   #ifdef AAC_ENABLE
      // Enable AC-3 data type if requested by VM
      r0 = $spdif.DATA_TYPE_SUPPORT($spdif.IEC_61937_DATA_TYPE_MPEG2_AAC);
      Null = r1 AND 2;
      if NZ r5 = r5 OR r0;
   #endif
   #ifdef MP3_ENABLE
      // Enable MP3(MPEG1 Layer3 only) data type if requested by VM
      r0 = $spdif.DATA_TYPE_SUPPORT($spdif.IEC_61937_DATA_TYPE_MPEG1_LAYER23);
      Null = r1 AND 4;
      if NZ r5 = r5 OR r0;
   #endif
   M[$spdif_copy_struct + $spdif.frame_copy.SUPPORTED_CODEC_TYPES_BITS_FIELD] = r5;

   #ifdef SPDIF_DISABLE_OUTPUT_RATE_CONTROL
      // for multi channel use case we disable output rate control by the library,
      // so the output rate will stay in whatever vm has set that to.
      // DSP expects that vm initially set the output rates at these values:
      // for DAC/SPDIF -> 48kHz
      r5 = 48000;
      // Scale to get the setting sampling rate in Hz
      r3 = r3 * 10 (int);
      if NZ r5 = r3;
      M[$spdif_stream_decode_struct + $spdif.stream_decode.OUTPUT_INTERFACE_CURRENT_RATE_FIELD] = r5;
      M[$spdif_stream_decode_struct + $spdif.stream_decode.OUTPUT_INTERFACE_TYPE_FIELD] = $spdif.OUTPUT_INTERFACE_TYPE_NONE;
   #else
      // This part of code remains here so we might want to have
      // an efficient stereo app later.
      //
      // extract output interface type
      //
      r0 = r1 AND 0x60;
      r0 = r0 LSHIFT -5;
      M[$spdif_stream_decode_struct + $spdif.stream_decode.OUTPUT_INTERFACE_TYPE_FIELD] = r0;

      // Scale to get the setting sampling rate in Hz
      r3 = r3 * 10 (int);
      M[$spdif_stream_decode_struct + $spdif.stream_decode.OUTPUT_INTERFACE_SETTING_RATE_FIELD] = r3;
      //Check if output interface is I2S
      Null = r0 - $spdif.OUTPUT_INTERFACE_TYPE_I2S;
      if NZ jump output_interface_not_i2s;

      // tell lib not control the output (i.e. we will use resampler)
      M[$spdif_stream_decode_struct + $spdif.stream_decode.OUTPUT_INTERFACE_TYPE_FIELD] =  $spdif.OUTPUT_INTERFACE_TYPE_NONE;
      // vm has already set the sample rate and we don't plan to change it
      M[$spdif_stream_decode_struct + $spdif.stream_decode.OUTPUT_INTERFACE_CURRENT_RATE_FIELD] = r3;
      if Z call $error;

      jump i2s_output_rate_set;
      output_interface_not_i2s:
      #ifdef OUTPUT_INTERFACE_INIT_ACTIVE
         // For DAC/SPDIF output interface initially is set to 48khz by vm
         r0 = 1;
         M[$spdif_stream_decode_struct + $spdif.stream_decode.OUTPUT_INTERFACE_INIT_ACTIVE_FIELD] = r0;
      #endif // #ifdef OUTPUT_INTERFACE_INIT_ACTIVE
   i2s_output_rate_set:
   #endif //#ifdef SPDIF_DISABLE_OUTPUT_RATE_CONTROL


   // set the latency level
   r0 = r1 LSHIFT -8;
   M[$spdif_target_latency_setting] = r0;

   // Enable/Disable event reporting
   r0 = r2 AND 1;
   M[$spdif_copy_struct + $spdif.frame_copy.ENABLE_EVENT_REPORT_FIELD] = r0;

   // invalid message postpone time
   r0 = r2 LSHIFT -1;
   r0 = r0 AND 0x3F;
   M[$spdif_copy_struct + $spdif.frame_copy.INVALID_MESSAGE_POSTPONE_TIME_FIELD] = r0;

   // invalid message postpone time
   r0 = r2 LSHIFT -8;
   r0 = r0 AND 0x1;
   M[$rate_match_disable] = r0;

   // check input width
   // default is 16-bit
   r0 = r2 AND (1<<10);
   M[$spdif_copy_struct + $spdif.frame_copy.INPUT_24BIT_ENABLE_FIELD] = r0;
   if Z jump port_width_set_done;

   // reconfigure ports for 24-bit inputs
   r0 = M[$spdif_copy_struct + $spdif.frame_copy.LEFT_INPUT_PORT_FIELD];
   r0 = r0 OR $cbuffer.FORCE_24B_PCM_AUDIO;
   M[$spdif_copy_struct + $spdif.frame_copy.LEFT_INPUT_PORT_FIELD] = r0;
   r0 = M[$spdif_copy_struct + $spdif.frame_copy.RIGHT_INPUT_PORT_FIELD];
   r0 = r0 OR $cbuffer.FORCE_24B_PCM_AUDIO;
   M[$spdif_copy_struct + $spdif.frame_copy.RIGHT_INPUT_PORT_FIELD] = r0;
   port_width_set_done:

   // A SPDIF config message means spdif mode
   r0 = $SPDIF_IO;
   M[$app_config.io] = r0;

   // Initialise the statistics pointer table
   I0 = $spdif_stats;
   call $copy_codec_stats_pointers;

   // For multi-channel the OUTPUT_INTERFACE_<xxx>_PORT_FIELD fields are zero (as initialised)
   M[$spdif_stream_decode_struct + $spdif.stream_decode.OUTPUT_INTERFACE_LEFT_PORT_FIELD] = 0;
   M[$spdif_stream_decode_struct + $spdif.stream_decode.OUTPUT_INTERFACE_RIGHT_PORT_FIELD] = 0;

   // update spdif config word for UFE
   call $spdif_update_config_word;

   // init library
   r5 = $spdif_stream_decode_struct;
   call $spdif.init;

   // pop rLink from stack
   jump $pop_rLink_and_rts;

.ENDMODULE;

#ifdef AC3_ENABLE
// *****************************************************************************
// MODULE:
//   $config_ac3_decoder_message_handler
// DESCRIPTION:
//   configures ac-3 decoder
//
// INPUTS:
//Word1 and Word2
//- cut and boost: each 7 bits
//- stereo mixing mode 2bit
//
//- channel routing 6bit
//- lfe on/off 1bit
//- karaoke mode 2
//- dual mode 2bit
//- compression mode 2bit
//- output mode 1bit
//
// handling of VM_CONFIG_SPDIF_APP_MESSAGE message received from VM
//    r1 = config word 0
//         bits 0:6    -> cut compression ratio
//            cut = (val/100.0)
//         bits 7:13    -> boost compression ratio
//            boost = (val/100.0)
//         bits 14:15   -> compression mode
//            0 -> custom 1
//            1 -> custom 2
//            2 -> line out
//            3 -> rf
//
//    r2 = config word 1
//         bit 0      -> lef output on/off flag
//         bit 1      -> karaoke on/off flag
//         bits 2:3   -> dual mono mode bit
//            0 -> stereo
//            1 -> left only
//            2 -> right only
//            3 -> mix//
//         bits 4:10  -> channel routing info
//           bit 4:  enable channel routing
//           bit 5:7 channel to be routed to left
//           bit 8:10 channel to be routed to right
//         bits 11:12   -> stereo mixing mode
//            0 -> auto mixing mode
//            1 -> Lt/Rt mixing mode
//            2 -> Lo/Ro mixing mode
//            3 -> reserved
//         bit 13   -> output mode
//            0 -> 2/0
//            1 -> multi channel (not supported in ADK3.0)
//
//         bits 14:15 -> unused
//
//   r3 = config word 2 (reserved)
//   r4 = config word 3 (reserved)
//
//  OUTPUTS:
//     None
//
// TRASHED REGISTERS:
//    Assume everything
//
// *****************************************************************************
.MODULE $M.config_ac3_decoder_message_handler;
   .CODESEGMENT CONFIG_AC3_DECODER_MESSAGE_HANDLER_PM;
   .DATASEGMENT DM;

$config_ac3_decoder_message_handler:

   // push rLink onto stack
   $push_rLink_macro;

   // save config words
   M[$ac3dec_config_words + 0] = r1;
   M[$ac3dec_config_words + 1] = r2;

   // compression cut scale
   r0 = r1 AND 0x7F;
   rMAC = r0 * 0.64;
   rMAC = rMAC ASHIFT 17 (56bit);
   M[$ac3dec_user_config + $ac3dec.config.COMP_CUT_SCALE_FIELD] = rMAC;

   // compression boost scale
   r0 = r1 LSHIFT -7;
   r0 = r0 AND 0x7F;
   rMAC = r0 * 0.64;
   rMAC = rMAC ASHIFT 17 (56bit);
   M[$ac3dec_user_config + $ac3dec.config.COMP_BOOST_SCALE_FIELD] = rMAC;

   // compression mode
   r0 = r1 LSHIFT -14;
   r0 = r0 AND 0x3;
   M[$ac3dec_user_config + $ac3dec.config.COMPRESSION_MODE_FIELD] = r0;

   // lfe mode
   r0 = 0;
   r1 = $audio_out_lfe_cbuffer_struc;
   Null = r2 AND 1;
   if NZ r0 = r1;
   M[$ac3dec_user_config + $ac3dec.config.DECODER_OUT_LFE_BUFFER_FIELD] = r0;

   // dual mono bits
   r0 = r2 LSHIFT -2;
   r0 = r0 AND 0x3;
   M[$ac3dec_user_config + $ac3dec.config.DUAL_MODE_FIELD] = r0;

   // channel routing
   r1 =  1<<23;
   r0 = r2 LSHIFT -5;
   r0 = r0 AND 0x3F;
   Null = r2 AND (1<<4);
   if Z r0 = 0;
   if NZ r0 = r0 OR r1;
   M[$ac3dec_user_config + $ac3dec.config.ROUTE_CHANNELS_FIELD] = r0;

   // stereo mixing mode
   r0 = r2 LSHIFT -11;
   r0 = r0 AND 0x3;
   M[$ac3dec_user_config + $ac3dec.config.STEREO_MODE_FIELD] = r0;

   // output mode
   r0 = $ac3dec.ACMOD_20;
   r5 = $ac3dec.ACMOD_32;
   Null = r2 AND (0x2000);
   if NZ r0 = r5;
   M[$ac3dec_user_config + $ac3dec.config.OUTPUT_MODE_FIELD] = r0;

   // enable reconfiguring the decoder
   r0 = 1;
   M[$ac3dec_reconfig_needed] = r0;

   // pop rLink from stack
   jump $pop_rLink_and_rts;

.ENDMODULE;
// *****************************************************************************
// MODULE:
//   $ac3_user_info_request_message_handler
// DESCRIPTION:
//   handle request for ac-3 user info
//
// INPUTS:
//    None
//  OUTPUTS:
//     None
//
// TRASHED REGISTERS:
//    Assume everything
//
// *****************************************************************************
.MODULE $M.ac3_user_info_request_message_handler;
   .CODESEGMENT AC3_USER_INFO_REQUEST_MESSAGE_HANDLER_PM;
   .DATASEGMENT DM;

$ac3_user_info_request_message_handler:

   // push rLink onto stack
   $push_rLink_macro;

   // send ac-3 user info to vm
   r3 = AC3_USER_INFO_MSG;
   r4 = length($ac3dec_user_info_struct);
   r5 = $ac3dec_user_info_struct;
   call $message.send_long;

   // pop rLink from stack
   jump $pop_rLink_and_rts;

.ENDMODULE;
#endif


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

   // Multi-channel DAC output silence cbops copy struc (dynamically constructed)
   .VAR chain1_sil_copy_struc[$MULTI_CHAN_CHAIN1_MAX_WIRED_CHANNELS + $MULTI_CHAN_CHAIN1_MAX_WIRED_CHANNELS + 3 + $cbops.AV_COPY_M_EXTEND_SIZE] =
      $chain1_processing_switch_op,                         // First operator block
      0 ...;                                                // Zero remaining struc elements (these are dynamically configured)

   .VAR $init_latency_state = 0;
   .VAR $init_latency_table[] = il_idle, il_wait, il_play, il_pause;
   .VAR $no_audio_counter = 0;
   .VAR $time_left_to_play;

   $adjust_initial_latency:

   $push_rLink_macro;

   // jump to proper state handler
   r6 = M[$init_latency_state];
   r0 = M[$init_latency_table + r6];
   jump r0;

   il_idle:
   // idle state:
   // copy silence and
   // stay here until first packet received
   r6 = 0;
   r7 = chain0_sil_copy_struc;
   r8 = chain1_sil_copy_struc;
   Null = M[$first_audio_received];
   if Z jump init_latency_done;

   il_wait:
   // wait state:
   // play silence and
   // wait until 'target_latency' has passed since receiving first packet
   // then go to play mode
   r6 = 1;
   r7 = chain0_sil_copy_struc;
   r8 = chain1_sil_copy_struc;

   // if latency cannot increase any more then go to play
   r0 = $spdif_in_cbuffer_struc;
   call $cbuffer.calc_amount_space;
   Null = r0 - 512;
   if NEG jump il_play;

   // stay in this mode up to "target latency" time
   r0 = M[$first_audio_time];
   r1 = M[$TIMER_TIME];
   r1 = r1 - r0;
   if NEG r1 = -r1;
   // add the init offset
   r1 = r1 + M[$first_audio_init_latency_offset];
   r1 = r1 - M[$tmr_period_audio_copy];
   r2 = M[$encoded_latency_struct + $encoded_latency.TOTAL_LATENCY_US_FIELD];
   r1 = MAX r2;
   r1 = r1 * 0.001(frac);
   r0 = M[$spdif_sra_struct + $spdif_sra.TARGET_LATENCY_MS_FIELD];
   r0 = r0 - r1;
   M[$time_left_to_play] = r0;
   if POS jump init_latency_done;
   il_play:
   // play mode:
   // play audio
   // go tpo pause mode when stream becomes invalid
   r6 = 2;
   r7 = $M.multi_chan_output.chain0_copy_struc;
   r8 = $M.multi_chan_output.chain1_copy_struc;
   // exit play mode if the input has become invalide
   M[$no_audio_counter] = 0;
   Null = M[$spdif_copy_struct + $spdif.frame_copy.STREAM_INVALID_FIELD];
   if Z jump init_latency_done;
   M[$first_audio_received] = 0;

   il_pause:
   // pause mode:
   // play remaining audio
   // if stall ends return back to play mode
   // if no audio to play go to idle and wait for new valid stream to arrive
   r6 = 3;
   r7 = $M.multi_chan_output.chain0_copy_struc;
   r8 = $M.multi_chan_output.chain1_copy_struc;

   // return to playing if stream becomes valid
   Null = M[$spdif_copy_struct + $spdif.frame_copy.STREAM_INVALID_FIELD];
   if Z jump il_play;

   // see if the final buffer is almost empty
   r0 = &$multi_chan_primary_left_out_cbuffer_struc;
   call $cbuffer.calc_amount_data;
   r1 = M[$no_audio_counter];
   r1 = r1 + 1;
   Null = r0 - 2;
   if POS r1 = 0;
   M[$no_audio_counter] = r1;
   Null = r1 - 2;
   if NEG jump init_latency_done;

   // no audio to play,
   // clear input buffers
   // and go idle
   r0 = &$audio_out_left_cbuffer_struc;
   call $cbuffer.empty_buffer;
   r0 = &$audio_out_right_cbuffer_struc;
   call $cbuffer.empty_buffer;

#if defined(AC3_ENABLE)
   r0 = &$audio_out_lfe_cbuffer_struc;
   call $cbuffer.empty_buffer;
#endif // defined(AC3_ENABLE)

   jump il_idle;

   init_latency_done:
   // set audio or silence buffers to play on CHAIN0 and DAC
   M[$multi_chain0_copy_struc_ptr] = r7;
   M[$multi_chain1_copy_struc_ptr] = r8;
   M[$init_latency_state] = r6;

   // pop rLink from stack
   jump $pop_rLink_and_rts;

.ENDMODULE;
