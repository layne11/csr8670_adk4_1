// *****************************************************************************
// Copyright (c) 2009 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
//
// *****************************************************************************

// *****************************************************************************
// DESCRIPTION
//   TWS and ShareMe routines
//
// *****************************************************************************
#include "stream_relay_library.h"
#include "core_library.h"
#include "cbops_library.h"
#include "codec_library.h"
#include "sr_adjustment.h"
#include "relay_conn.h"
#include "sbc_library.h"
#include "music_example.h"
#include "frame_sync_stream_macros.h"
#include "multichannel_output_macros.h"
#include "tws_delay.h"

.MODULE $M.tws;
   .DATASEGMENT DM;

   .VAR/DMCIRC $relay_buffer[RELAY_CBUFFER_SIZE];
   .VAR $relay.mode_message_struc[$message.STRUC_SIZE];
   .VAR $tws.routing_mode_message_struc[$message.STRUC_SIZE];
   .VAR $tws.external_volume_enable_message_struc[$message.STRUC_SIZE];
   .VAR $tws.device_trim_message_struc[$message.STRUC_SIZE];
   .VAR $tws.relay_control_message_struc[$message.STRUC_SIZE];
   .VAR $tws.syncdelay_compat_message_struc[$message.STRUC_SIZE];

   .VAR $codec_in_cbuffer_int_struc[$cbuffer.STRUC_SIZE] =
      LENGTH($codec_in),             // size
      &$codec_in,                    // read pointer
      &$codec_in;                    // write pointer

   .VAR $relay_cbuffer_struc[$cbuffer.STRUC_SIZE] =
      LENGTH($relay_buffer),         // size
      &$relay_buffer,                // read pointer
      &$relay_buffer;                // write pointer

   // ** allocate memory for codec input cbops copy routine **
   .VAR $relay_copy_struc[] =
      &$relay_copy_op,               // first operator block
      1,                             // number of inputs
      &$relay_cbuffer_struc,         // input
      1,                             // number of outputs
      $RELAY_PORT;                   // output

   .BLOCK $relay_copy_op;
      .VAR $relay_copy_op.next = $relay_limited_amt_op;
      .VAR $relay_copy_op.func = &$cbops.copy_op;
      .VAR $relay_copy_op.param[$cbops.copy_op.STRUC_SIZE] =
         0,                          // Input index
         1;                          // Output index
   .ENDBLOCK;

    .BLOCK $relay_limited_amt_op;
      .VAR $relay_limited_amt_op.next = $cbops.NO_MORE_OPERATORS;
      .VAR $relay_limited_amt_op.func = &$cbops.limited_amount;
      .VAR $relay_limited_amt_op.param[$cbops.limited_amount.STRUC_SIZE] =
         floor($TWS_CHUNK_SIZE/2),
         $TWS_FLUSH_TIMEOUT,
         0;
   .ENDBLOCK;

   // zero(address or value) means not received
   .VAR $current_dac_sampling_frequency = 0;

   .VAR $get_tws_routing_from_vm_struc[$message.STRUC_SIZE];

   // define memory location for timer handler
   .VAR $send_vol_to_vm_timer_struc[$timer.STRUC_SIZE];
   .VAR $soft_mute_timer_struc[$timer.STRUC_SIZE];
   .VAR $soft_unmute_timer_struc[$timer.STRUC_SIZE];
   .VAR $soft_mute_duration;

   .VAR $relay_struc[$relay.struct_size]=
        &$codec_in_cbuffer_struc,
        &$codec_in_cbuffer_int_struc,
        &$relay_cbuffer_struc,
        &$decoder_codec_stream_struc,        //.CONST $relay.AV_DECODE_STRUC_FIELD
        0,                                   // & get_bitpos for selected codec
        0,                                   // & GET BITS FOR SELECTED CODEC
        0,                                   // zero for shareme, &$tws.header_struc for TWS
        0;                                   // Flag set if APTX enabled

   .VAR $tws.header_struc[$tws.header.struct_size]=    //Test header
        10,
        4412242,     // CSR ID
        $TWS_V3_PROTOCOL_ID,           // TWS Protocol ver|flags|Routing Mode
        0,           // Playback time
        0,           // sample rate adjustment value
        0,           // Sys Vol
        0,           // Master Vol
        0,           // Tone vol
        0,           // Left Trim
        0,           // Right trim   - For now use as debug
        0;           // Actual SRA

   .VAR $tws.wallclock_obj[$wall_clock.STRUC_SIZE] =
      0, 0, 0, 0, 0, 0, &$tws.roleswitch_detect, 0 ...;

   .VAR $tws.bdaddr_message_struc[$message.STRUC_SIZE];
   .VAR $tws.set_synchronized_data_message_struc[$timer.STRUC_SIZE];
   .VAR $tws.softmute_data_timer_struc[$timer.STRUC_SIZE];
   .VAR $tws.warp_timer_id;
   .VAR $tws.volume_timer_id;
   .VAR $tws.scheduled_sra;
   .VAR $tws.synchronised_sra;
   .VAR $tws.slave_sra;
   .VAR $tws.override_flag = 0;
   .VAR $tws.message_volume_struc[$MAIN_VOL_MESSAGE_SIZE]; // TBD Define $MAIN_VOL_MESSAGE_SIZE
   .VAR $tws.message_volume_struc_temp[$MAIN_VOL_MESSAGE_SIZE]; // TBD Define $MAIN_VOL_MESSAGE_SIZE
   .VAR $tws.scheduled_volume_struc[$MAIN_VOL_MESSAGE_SIZE];  // find a good define for volume message size?
   .VAR $tws.local_mute_volume_struc[$MAIN_VOL_MESSAGE_SIZE] =
        15,          // Sys Vol  - will maintain current value during mute...
        -7200,       // Master Vol    -60db
        0,           // Tone vol
        0,           // Left Pri Trim
        0,           // Right Pri trim
        0,           // Left Sec trim
        0,           // Right Sec Trim
        0;           // wired Sub Trim
   .VAR $tws.local_saved_volume_struc[$MAIN_VOL_MESSAGE_SIZE];
   .VAR $tws.device_trim_master = 0;
   .VAR $tws.device_trim_slave = 0;
   .VAR $tws.message_routing_mode_master = 0;
   .VAR $tws.message_routing_mode_slave = 0;
   .VAR $tws.scheduled_routing_mode = 0;
   .VAR $tws.routing_mode = 0;
   .VAR $tws.started_streaming = 0;
   .VAR $relay.mode = 0;
   .VAR $tws.external_volume_enable = 0;
   .VAR $tws.master_enabled = 0;
   .VAR $tws.wired_mode_enabled = 0;
   .VAR $tws.insertion_counter = 0;
   .VAR $tws.started_streaming_counter = 0;
   .VAR $tws.zero = 0;
   //---TWS Slave Rate match variables
   .VAR $tws.alpha = $TWS_ALPHA;
   .VAR $tws.average_playback_difference = 0;
   .VAR $tws.average_playback_difference_old = 0;
   .VAR $tws.slave_sra_tc = $TWS_SLAVE_SRA_TC;
   .VAR $tws.forcing_rate = 0;
   .VAR $tws.p_weighting = $PROPORTIONAL_WEIGHT;
   .VAR $tws.alignment_delay = $TWS_ALIGNMENT_DELAY;
   .VAR $tws.nudge_factor = $TWS_NUDGE_FACTOR;
   //-------------------
   .VAR $tws.wallclock_running=0;
   .VAR $tws.decode_cnt = 0;
   .VAR $tws.initial_holdoff = 0;
   .VAR $tws.timeout_cnt=0;
   .VAR $tws.local_mute = 0;
   .var $tws.Slave_GoodWorking_Time = $TWS_JITTER_THRESHOLD; // Target is master delay - GoodWorking_Time




#ifdef TWS_ENABLE
   #define ALIGNMENT_DELAY 0

   DeclareCBuffer(delay_buf_left_cbuffer_struc,delay_buf_left,($TWS_ALIGNMENT_DELAY_SIZE + 1));
   DeclareCBuffer(delay_buf_right_cbuffer_struc,delay_buf_right,($TWS_ALIGNMENT_DELAY_SIZE + 1));

  .VAR $delay_left[$tws.delay.STRUC_SIZE] =
      &$audio_out_left_cbuffer_struc,                 // OFFSET_INPUT_PTR
      &$audio_out_left_cbuffer_struc,                 // OFFSET_OUTPUT_PTR
      &delay_buf_left_cbuffer_struc,
      ALIGNMENT_DELAY,
   #ifdef DELAY_16BIT_MODE
      1,           // 16-bit samples stored in delay line
   #else
      0,           // 24-bit samples stored in the delay line
   #endif
      0;           // Write Byte Position Indicator

   .VAR $delay_right[$tws.delay.STRUC_SIZE] =
      &$audio_out_right_cbuffer_struc,                // OFFSET_INPUT_PTR
      &$audio_out_right_cbuffer_struc,                // OFFSET_OUTPUT_PTR
      &delay_buf_right_cbuffer_struc,
      ALIGNMENT_DELAY,
   #ifdef DELAY_16BIT_MODE
      1,           // 16-bit samples stored in delay line
   #else
      0,           // 24-bit samples stored in the delay line
   #endif
      0;           // Write Byte position Indicator
#endif


#ifdef TWS_WIRED_MODE_ENABLE
 // These scratch "registers" are used by various libraries (e.g. SBC)
   .VAR $scratch.s0;
   .VAR $scratch.s1;
   .VAR $scratch.s2;

// ** allocate memory for av encoder structure **
   .VAR/DM1 $av_encoder_codec_stream_struc[$codec.av_encode.STRUC_SIZE] =
      &$sbcenc.frame_encode,     // frame_encode function
      &$sbcenc.reset_encoder,    // reset_encoder function
      &$codec_in_cbuffer_struc,                // out cbuffer
      &$audio_in_left_cbuffer_struc,           // in left cbuffer
      &$audio_in_right_cbuffer_struc,          // in right cbuffer
      0 ...;                                    // will also contain new pointer to data object field

   .VAR $codec_reset_needed = 1;
   .VAR $sbc_dec_initialized = 0;

   DeclareCBuffer($audio_in_left_cbuffer_struc,$audio_in_left,AUDIO_CBUFFER_SIZE);
   DeclareCBuffer($audio_in_right_cbuffer_struc,$audio_in_right,AUDIO_CBUFFER_SIZE);

#endif

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.relay.init
//
// DESCRIPTION:
//    Registers message handlers required for TWS.
//
// INPUTS:
//    - none
//
// OUTPUTS:
//    - none
// *****************************************************************************
.MODULE $M.relay.init;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   $relay.init:

   $push_rLink_macro;

   // set up message handler for $MESSAGE_VM_STREAM_RELAY_MODE message
   r1 = &$relay.mode_message_struc;
   r2 = $MESSAGE_VM_STREAM_RELAY_MODE;
   r3 = &$relay.mode_message_handler;
   call $message.register_handler;

   // set up message handler for $MESSAGE_VM_STREAM_RELAY_MODE message
   r1 = &$tws.syncdelay_compat_message_struc;
   r2 = $MESSAGE_VM_TWS_COMPATIBILITY_MODE;
   r3 = &$tws.syncdelay_compat_message_handler;
   call $message.register_handler;

   // set up message handler for $MESSAGE_VM_RELAY_CONTROL_MODE message
   r1 = &$tws.relay_control_message_struc;
   r2 = $MESSAGE_VM_RELAY_CONTROL;
   r3 = &$tws.relay_control_message_handler;
   call $message.register_handler;

#ifdef TWS_ENABLE

#ifdef TWS_WIRED_MODE_ENABLE
   // Initialise the codec library. Stream codec structure has codec structure nested
   // inside it, so can pass start of codec structure to init, where data object pointer
   // is also set.

   r5 = &$av_encoder_codec_stream_struc + $codec.av_encode.ENCODER_STRUC_FIELD;
   call $sbcenc.init_static_encoder;

#endif

   jump done;



relay_port_disconnected_handler:
   $push_rLink_macro;
   // TODO: any clean up needed now that relay port has disconnected?
   // This will deregister the TWS volume stuff, the wall clock, and the routing
   //    The delay and the wired mode encoding will stay configured until we get
   //    a VM message telling us to go back to no_relay mode.


   // TODO: This will run for ShareMe to so need to check that this is okay.
   call $tws.disable;
   M[$tws.master_enabled] = null;

   // clear IF statistic
   M[$music_example.OTA_IF_Connections] = NULL;

   // disable volume return messages
   M[$aux_vol_msg_echo] = NULL;
   M[$multichannel_vol_msg_echo] = NULL;

   jump done;

#endif

done:

   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.tws.bluetooth_address_handler
//
// DESCRIPTION:
//    Get BT address
//
// INPUTS:
//      r1 = BT Address Type | (port)
//      r2 = BT ADdress Word 0
//      r3 = BT ADdress Word 1
//      r4 = BT ADdress Word 2
//
// OUTPUTS:
//
// RESERVED REGISTERS:
//
// TRASHED REGISTERS:
//    - all
//
// NOTES:
//
// *****************************************************************************
.MODULE $M.tws.bluetooth_address_handler;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

$tws.bluetooth_address_handler:
   $push_rLink_macro;

   // Save encoding mode and other parameters
   // note these can end up getting sign-extended... is this OK?
   r1 = r1 LSHIFT -8;
   M[$tws.wallclock_obj + $wall_clock.BT_ADDR_TYPE_FIELD]  = r1;
   M[$tws.wallclock_obj + $wall_clock.BT_ADDR_WORD0_FIELD] = r2;
   M[$tws.wallclock_obj + $wall_clock.BT_ADDR_WORD1_FIELD] = r3;
   M[$tws.wallclock_obj + $wall_clock.BT_ADDR_WORD2_FIELD] = r4;

   // start wallclock.
   r1 = &$tws.wallclock_obj;

   NULL = M[r1 + $wall_clock.NEXT_ADDR_FIELD];
   if Z call $wall_clock.enable;

   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $tws.audio_routing
//
// DESCRIPTION:
//    Handles TWS audio_routing
//
// INPUtS:
//    r2 =  tws audio routing mode
//    r3 =  number of samples to process
//
// OUTPUTS:
//    - none
// *****************************************************************************
.MODULE $M.tws.audio_routing;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   .VAR $num_samples;

   $tws.audio_routing:
   $push_rLink_macro;
   call $block_interrupts;

  r0  = M[$music_example.SystemVolume];
  M[$tws.local_mute_volume_struc +0] = r0;
  r0  = M[$music_example.Main.ToneVolume];
  M[$tws.local_mute_volume_struc +2] = r0;

   // Check if mode == stereo, if so, skip routing
   null = r2 - $TWS_ROUTING_STEREO;
   if Z jump done_with_routing;

   M0 = -r3;

   r0 = $audio_out_left_cbuffer_struc;
   call $cbuffer.get_write_address_and_size;
   I0 = r0;
   L0 = r1;

   r0 = $audio_out_right_cbuffer_struc;
   call $cbuffer.get_write_address_and_size;
   I4 = r0;
   L4 = r1;

   r0 = M[I0,M0];       // Dummy read to move I0  -  Left Channel
   r4 = M[I4,M0];       // Dummy read to move I4

   r10 = r3;
   do tws_audio_routing_loop;
      null = r2 - $TWS_ROUTING_LEFT;
      if NZ jump check_right;
      r0 = M[I0,1];
      M[I4,1] = r0;
      jump continue_loop;

   check_right:
      null = r2 - $TWS_ROUTING_RIGHT;
      if NZ jump check_dmix;

      r0 = M[I4,1];
      M[I0,1] = r0;
      jump continue_loop;

   check_dmix:
      null = r2 - $TWS_ROUTING_DMIX;
      if NZ jump done_with_routing;
      r0 = M[I0,0];
      r1 = M[I4,0];
      r0 = r0 ASHIFT -1;
      r1 = r1 ASHIFT -1;
      r0 = r1 + r0;
      M[I0,1] = r0;
      M[I4,1] = r0;

   continue_loop:
   nop;
   tws_audio_routing_loop:

   r0 = $audio_out_left_cbuffer_struc;
   r1 = I0;
   call $cbuffer.set_write_address;

   r0 = $audio_out_right_cbuffer_struc;
   r1 = I4;
   call $cbuffer.set_write_address;

   done_with_routing:
   L0 = 0;
   L4 = 0;
   M0 = 0;

   call $unblock_interrupts;

   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $shareme.enable
//
// DESCRIPTION:
//    Enable shareme by populting the av_decode relay fields
//
//
// INPUTS:
//
//
// OUTPUTS:
//    - none
// *****************************************************************************
.MODULE $M.shareme.enable;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   $shareme.enable:
   M[$relay_struc+ $relay.TWS_HEADER_FIELD] = NULL;

   r0 = $M.MUSIC_MANAGER.CONFIG.IFFUNC_SHAREME;
   M[$music_example.OTA_IF_Connections] = r0;
   rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $tws.master.enable
//
// DESCRIPTION:
//    Enable tws in master config.  Should only be called if mode is TWS.MASTER
//
//
// INPUTS:
//
//
// OUTPUTS:
//    - none
// *****************************************************************************
.MODULE $M.tws.master.enable;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   $tws.master.enable:

   $push_rLink_macro;

   call $tws.local_volume_mute;



   r0 = $RELAY_PORT;
   call $cbuffer.is_it_enabled;
   if Z jump done;

   r3 = $MESSAGE_PORT_BT_ADDRESS;
   call $message.unregister_handler;

   r1 = &$tws.bdaddr_message_struc;
   r2 = $MESSAGE_PORT_BT_ADDRESS;
   r3 = &$tws.bluetooth_address_handler;
   call $message.register_handler;

   r3 = $RELAY_PORT;
   r2 = $MESSAGE_GET_BT_ADDRESS;
   call $message.send;

   // cancel message handler
   r1 = &$tws.softmute_data_timer_struc;
   r2 = M[r1 + $timer.ID_FIELD];
   if NZ call $timer.cancel_event;

   r2 = $TWS_SOFTMUTE_TIME;
   r1 = &$tws.softmute_data_timer_struc;
   r3 = &$tws.local_volume_unmute;
   call $timer.schedule_event_in;
   r1 = M[$tws.decode_cnt];
   null = r1 - $TWS_STANDALONE_CNT;
   if NEG jump no_holdoff;

   r1 = $TWS_CONNECTION_HOLDOFF;
   M[$tws.initial_holdoff] = r1;

   no_holdoff:
   M[$tws.master_enabled] = r3;

   r1 = &$tws.header_struc;
   M[$relay_struc+ $relay.TWS_HEADER_FIELD] = r1;

   r3 = &$tws.local_saved_volume_struc;
   r2 = length($tws.message_volume_struc);
   r1 = &$tws.message_volume_struc;
   call $tws.copy_array;
   // Unregister Default Volume Message Handler
   call $block_interrupts;
   r3 = $M.music_example.VMMSG.VOLUME_S;
   call $message.unregister_handler;

   // Register TWS Volume Message Handler
   r1 = &$M.music_example_message.multi_volume_s_message_struc;
   r2 = $M.music_example.VMMSG.VOLUME_S;
   r3 = &$tws.volume_s_message_handler;
   call $message.register_handler;

   // enable volume return messages
   r2 = M[$tws.external_volume_enable];
   r1 = M[$DAC_conn_main];
   r1 = r1 OR r2;
   M[$multichannel_vol_msg_echo] = r1;

   // Initialize internal cbuffers
   r3 = &$relay_struc;

   r0 = M[r3 + $relay.INPUT_CBUFFER_FIELD];
   call $cbuffer.get_write_address_and_size;
   r1 = r0;
   r0 = M[r3 + $relay.INTERNAL_CBUFFER_PTR_FIELD];
   call $cbuffer.set_write_address;

   r0 = M[r3 + $relay.INPUT_CBUFFER_FIELD];
   call $cbuffer.get_read_address_and_size;
   r1 = r0;
   r0 = M[r3 + $relay.INTERNAL_CBUFFER_PTR_FIELD];
   call $cbuffer.set_read_address;

   call $unblock_interrupts;

   r1 = &$tws.routing_mode_message_struc;
   r2 = $MESSAGE_VM_TWS_ROUTING_MODE;
   r3 = &$tws.routing_message_handler;
   call $message.register_handler;

   r1 = &$tws.device_trim_message_struc;
   r2 = $MESSAGE_VM_DEVICE_TRIMS;
   r3 = &$tws.device_trim_message_handler;
   call $message.register_handler;

   r1 = &$tws.external_volume_enable_message_struc;
   r2 = $MESSAGE_VM_EXTERNAL_VOLUME_ENABLE;
   r3 = &$tws.external_volume_enable_message_handler;
   call $message.register_handler;

   call $tws.configure_synchronised_warping;

   r1 = M[$tws.alignment_delay];
   M[$M.main.samples_latency_measure + 0] = r1;

   // initialize manual delay processing here
   r8 = &$delay_left;
   call $tws.delay.initialize;
   r8 = &$delay_right;
   call $tws.delay.initialize;
   r1 = M[$tws.alignment_delay];
   M[$delay_left + $tws.delay.DELAY_FIELD] = r1;
   M[$delay_right + $tws.delay.DELAY_FIELD] = r1;

   r1 = $TWS_STALL_TO_DECODE_THRESHOLD;
   M[$decoder_codec_stream_struc + $codec.av_decode.STALL_TO_DECODE_THRESHOLD] = r1;


   call $M.relay.mode_message_handler.ack;

   r1 = 1;
   M[$tws.started_streaming] = r1;

   r0 = $M.MUSIC_MANAGER.CONFIG.IFFUNC_TWS;
   M[$music_example.OTA_IF_Connections] = r0;


   done:

   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $tws.wired_mode.enable
//
// DESCRIPTION:
//    Enable tws wired mode.
//
//
// INPUTS:
//
//
// OUTPUTS:
//    - none
// *****************************************************************************
.MODULE $M.tws.wired_mode.enable;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   $tws.wired_mode.enable:

   $push_rLink_macro;

   r0 = $RELAY_PORT;
   call $cbuffer.is_it_enabled;
   if Z jump done;

   Null = M[$tws.master_enabled];
   if Z jump done;

   Null = M[$tws.wired_mode_enabled];
   if NZ jump done;

   r0 = M[$app_config.io];
   null = r0 - $USB_IO;
   if Z jump enable;

   null = r0 - $I2S_IO;
   if Z jump enable;

   null = r0 - $ANALOGUE_IO;
   if NZ jump done;

   // want to prevent application from applying warp with analogue input
   r0 = &$tws.zero;
   M[$sra_rate_addr] = r0;

   enable:
   M[$tws.wired_mode_enabled] = r0; // set to any non-zero value

#ifdef USB_ENABLE
   r0 = &$audio_in_left_cbuffer_struc;
   M[$usb_audio_in_copy_struc + 1] = r0;
   r0 = &$audio_in_right_cbuffer_struc;
   M[$usb_audio_in_copy_struc + 2] = r0;
   r0 = M[$app_config.io];
   null = r0 - $USB_IO;
   if NZ jump sra_conf_done1;
      //Override SRA fields to use measurement for encoded data
      r0 = $CON_IN_PORT;
      M[$sra_struct + $sra.CODEC_PORT_FIELD] = Null;
      r0 = &$codec_in_cbuffer_struc;
      M[$sra_struct + $sra.CODEC_CBUFFER_TO_TAG_FIELD] = r0;
      r0 = &$audio_out_left_cbuffer_struc;
      M[$sra_struct + $sra.AUDIO_CBUFFER_TO_TAG_FIELD] = r0;
      Null = M[$current_dac_sampling_rate];
      if NZ jump sra_conf_done1;
         r0 = DEFAULT_MAXIMUM_CLOCK_MISMATCH_COMPENSATION;
         M[$sra_struct + $sra.MAX_RATE_FIELD] = r0;
         r0 = 48000*SRA_AVERAGING_TIME;
         M[$sra_struct + $sra.AUDIO_AMOUNT_EXPECTED_FIELD] = r0;
      sra_conf_done1:
#endif

#if defined(ANALOGUE_ENABLE) || defined(I2S_ENABLE)
   r0 = &$audio_in_left_cbuffer_struc;
   M[$analogue_audio_in_copy_struc + 5] = r0;
   M[$analogue_audio_in_mono_copy_struc + 4] = r0;
   r0 = &$audio_in_right_cbuffer_struc;
   M[$analogue_audio_in_copy_struc + 6] = r0;
   M[$analogue_audio_in_mono_copy_struc + 5] = r0;
#endif

   r0 = M[$sbc_dec_initialized];
   if NZ jump done;

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

   r1 = 1;
   M[$sbc_dec_initialized] =r1;

   done:
   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $tws.wired_mode.disable
//
// DESCRIPTION:
//    Disables wired mode's SBC decoder.
//
//
// INPUTS:
//
//
// OUTPUTS:
//    - none
// *****************************************************************************
.MODULE $M.tws.wired_mode.disable;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   $tws.wired_mode.disable:

   $push_rLink_macro;

   null = M[$tws.wired_mode_enabled];
   if Z jump done;

   M[$tws.wired_mode_enabled] = null;
   r0 = $sra_struct + $sra.SRA_RATE_FIELD;
   M[$sra_rate_addr] = r0;

   r1 = 1;
   M[$codec_reset_needed]=r1;

   r0 = &$audio_in_left_cbuffer_struc;
   call $cbuffer.get_read_address_and_size;
   r1 = r0;
   r0 = &$audio_in_left_cbuffer_struc;
   call $cbuffer.set_write_address;

   r0 = &$audio_in_right_cbuffer_struc;
   call $cbuffer.get_read_address_and_size;
   r1 = r0;
   r0 = &$audio_in_right_cbuffer_struc;
   call $cbuffer.set_write_address;
#if defined(ANALOGUE_ENABLE) || defined(I2S_ENABLE)
      r0 = &$audio_out_left_cbuffer_struc;
      M[$analogue_audio_in_copy_struc + 5] = r0;
      M[$analogue_audio_in_mono_copy_struc + 4] = r0;
      r0 = &$audio_out_right_cbuffer_struc;
      M[$analogue_audio_in_copy_struc + 6] = r0;
      M[$analogue_audio_in_mono_copy_struc + 5] = r0;
#endif

#ifdef USB_ENABLE
   r0 = &$audio_out_left_cbuffer_struc;
   M[$usb_audio_in_copy_struc + 1] = r0;
   r0 = &$audio_out_right_cbuffer_struc;
   M[$usb_audio_in_copy_struc + 2] = r0;
#endif

   done:
   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $tws.disable
//
// DESCRIPTION:
//    disable tws
//
//
// INPUTS:
//     r5 =  av_decode struct
//
// OUTPUTS:
//    - none
// *****************************************************************************
.MODULE $M.tws.disable;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   $tws.disable:

   $push_rLink_macro;

   call $tws.local_volume_mute;
   M[$relay_struc+ $relay.TWS_HEADER_FIELD] = NULL;

   // Unregister TWS Volume Message Handler
   call $block_interrupts;
   r3 = $M.music_example.VMMSG.VOLUME_S;
   call $message.unregister_handler;

   r3 = $MESSAGE_VM_TWS_ROUTING_MODE;
   call $message.unregister_handler;

   r3 = $MESSAGE_VM_DEVICE_TRIMS;
   call $message.unregister_handler;

   r3 = $MESSAGE_VM_EXTERNAL_VOLUME_ENABLE;
   call $message.unregister_handler;

   r3 = $MESSAGE_PORT_BT_ADDRESS;
   call $message.unregister_handler;

   call $unblock_interrupts;

   // Register default Volume Message Handler
   r1 = &$M.music_example_message.multi_volume_s_message_struc;
   r2 = $M.music_example.VMMSG.VOLUME_S;
   r3 = &$M.music_example_message.MainVolume_s.func;
   call $message.register_handler;

   r1 = $STALL_TO_DECODE_THRESHOLD;
   M[$decoder_codec_stream_struc + $codec.av_decode.STALL_TO_DECODE_THRESHOLD] = r1;

   // Set SRA to default averaging time
   r0 = 0;
   call $tws.set_avg_fraction;

   // Give warp control back to application.
   r0 = M[$sra_rate_addr];
   M[$codec_rate_adj.stereo + $cbops.rate_adjustment_and_shift.Process.SRA_TARGET_RATE_ADDR_FIELD] = r0;

   // disable wall clock
   r1 = &$tws.wallclock_obj;
   call $wall_clock.disable;
   r1 = &$tws.wallclock_obj;
   M[r1 + $wall_clock.NEXT_ADDR_FIELD]=NULL;
   M[r1 + $wall_clock.ADJUSTMENT_VALUE_FIELD]=NULL;
   M[$tws.wallclock_running] = NULL;

   r2 = M[$tws.warp_timer_id];
   if NZ call $timer.cancel_event;

   call $tws.local_volume_unmute;

   r3 = &$tws.local_saved_volume_struc;
   call $M.music_example_message.MainVolume.func;

   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $tws.slave.enable
//
// DESCRIPTION:
//    Enable tws in master config by populating the av_decode relay fields
//
//
// INPUTS:
//    r5 =  av_decode struct
//
// OUTPUTS:
//    - none
// *****************************************************************************
.MODULE $M.tws.slave.enable;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   $tws.slave.enable:

   $push_rLink_macro;

   r3 = &$tws.local_mute_volume_struc;
   call $M.music_example_message.MainVolume.func;

   r1 = &$tws.header_struc;
   M[$relay_struc+ $relay.TWS_HEADER_FIELD] = r1;

   // Unregister Default Volume Message Handler
   call $block_interrupts;
   r3 = $M.music_example.VMMSG.VOLUME_S;
   call $message.unregister_handler;
   call $unblock_interrupts;

   // Copy current volume to TWS volume
   I0 = &$tws.message_volume_struc;
   r1 = M[$music_example.SystemVolume];
   M[I0,1] = r1;
   r1 = M[$music_example.Main.MasterVolume] ;
   M[I0,1] = r1;
   r1 = M[$music_example.Main.ToneVolume];
   M[I0,1] = r1;
   r1 = M[$music_example.Main.PrimaryLeftTrimVolume];
   M[I0,1] = r1;
   r1 = M[$music_example.Main.PrimaryRightTrimVolume];
   M[I0,1] = r1;
   r1 = M[$music_example.Main.SecondaryLeftTrimVolume];
   M[I0,1] = r1;
   r1 = M[$music_example.Main.SecondaryRightTrimVolume];
   M[I0,1] = r1;
   r1 = M[$music_example.Main.SubTrimVolume];
   M[I0,1] = r1;


   // Register TWS Volume Message Handler
   r1 = &$M.music_example_message.multi_volume_s_message_struc;
   r2 = $M.music_example.VMMSG.VOLUME_S;
   r3 = &$tws.volume_s_message_handler;
   call $message.register_handler;



   // enable volume return messages
   r2 = M[$tws.external_volume_enable];
   r1 = M[$DAC_conn_main];
   r1 = r1 OR r2;
   M[$multichannel_vol_msg_echo] = r1;


   r1 = &$tws.routing_mode_message_struc;
   r2 = $MESSAGE_VM_TWS_ROUTING_MODE;
   r3 = &$tws.routing_message_handler;
   call $message.register_handler;

   r1 = &$tws.device_trim_message_struc;
   r2 = $MESSAGE_VM_DEVICE_TRIMS;
   r3 = &$tws.device_trim_message_handler;
   call $message.register_handler;

   r1 = &$tws.external_volume_enable_message_struc;
   r2 = $MESSAGE_VM_EXTERNAL_VOLUME_ENABLE;
   r3 = &$tws.external_volume_enable_message_handler;
   call $message.register_handler;

   r1 = $TWS_STALL_TO_DECODE_THRESHOLD;
   M[$decoder_codec_stream_struc + $codec.av_decode.STALL_TO_DECODE_THRESHOLD] = r1;

   call $tws.configure_synchronised_warping;


   // initialize foreground delay processing here
   r8 = &$delay_left;
   call $tws.delay.initialize;
   r8 = &$delay_right;
   call $tws.delay.initialize;


   r3 = $MESSAGE_PORT_BT_ADDRESS;
   call $message.unregister_handler;

   r1 = &$tws.bdaddr_message_struc;
   r2 = $MESSAGE_PORT_BT_ADDRESS;
   r3 = &$tws.bluetooth_address_handler;
   call $message.register_handler;

   // Note that DSP relies on VM connecting CODEC_L2CAP_IN_PORT before sending
   // sharing mode message.
   r0 = $CON_IN_PORT;
   call $cbuffer.is_it_enabled;
   if Z jump done;

   r3 = $CON_IN_PORT;
   r2 = $MESSAGE_GET_BT_ADDRESS;
   call $message.send;

   done:
   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $relay.mode_message_handler
//
// DESCRIPTION:
//    Handle VM relay mode change message
//
//
// INPUTS:
//     r1 =  Stream Relay mode
//             0 = none
//             1 = ShareMe
//             2 = TWS Master
//             3 = TWS Slave
//
// OUTPUTS:
//    - none
// *****************************************************************************
.MODULE $M.relay.mode_message_handler;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   .VAR last_mode;
      .VAR $reset_tws;
   $relay.mode_message_handler:

   $push_rLink_macro;

   push r1;

   r0 = M[$relay.mode];
   M[last_mode] = r0;

   M[$relay.mode] = r1;

#ifdef TWS_ENABLE
   r1 = M[last_mode];
   null = r1 AND 0x2;
   if Z jump not_tws;

   r1 = M[$relay.mode];
   if NZ jump not_tws;

   r1 = 1;
   M[$reset_tws] = r1;

   r1 = M[last_mode];
   M[$relay.mode] = r1;

   pop Null;// if we are waiting on a reset don't change state
   jump $pop_rLink_and_rts;
not_tws:

#endif

   r1 = M[$relay.mode];
   null = r1 - $NO_RELAY_MODE;
   if Z jump relay_disable;

   null = r1 - $SHAREME_MODE;
   if Z jump shareme_enable;

   null = r1 - $TWS_MASTER_MODE;
   if Z jump tws_master_enable;
   null = r1 - $TWS_SLAVE_MODE;
   if Z jump tws_slave_enable;

   // All other message values
   unsupported:
   // replace value on stack
   pop r1;
   r1 = $ERR_UNSUPPORTED_MODE;
   push r1;
   r0 = $NO_RELAY_MODE;
   M[$relay.mode] = r0;

   // Send ACK Message
send_ack:
   r2 = $MESSAGE_VM_STREAM_RELAY_MODE_ACK;
   pop r3;
   call $message.send_short;
done:
   jump $pop_rLink_and_rts;

relay_disable:
#ifdef TWS_ENABLE
   call $tws.disable;
#endif
   jump send_ack;

shareme_enable:
// Need to enable SHAREME when TWS is enabled in order to support PTS testing of TWS
#if defined(TWS_ENABLE) || defined(SHAREME_ENABLE)
   call $shareme.enable;
   jump send_ack;
#else
   jump unsupported;
#endif

tws_master_enable:
#ifdef TWS_ENABLE
   // call enable function and send ack after confirmation of valid relay port
   pop null;
   jump done;
#else
   jump unsupported;
#endif

tws_slave_enable:
#ifdef TWS_ENABLE
   call $tws.slave.enable;
   jump send_ack;
#else
   jump unsupported;
#endif

ack:
   $push_rLink_macro;
   r1 = M[$relay.mode];
   push r1;
   jump send_ack;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $relay.start
//
// DESCRIPTION:
//    If master, block input cbuffer, and extract bit position
//
//
// INPUTS:
//    r5 =  relay_struct
//
// OUTPUTS:
//    - none
// *****************************************************************************
.MODULE $M.relay.start;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   $relay.start:
   $push_rLink_macro;

#ifdef TWS_ENABLE
   Null = M[$reset_tws];
   if Z jump no_tws_reset;

   call $tws.reset;

   M[$reset_tws] = Null;
   M[$relay.mode] = Null;
   call $M.relay.mode_message_handler.ack;

no_tws_reset:
#endif

   r0 = M[$relay.mode];
   null = r0 - $ERR_RELAY_LOST;
   if Z jump no_buffer_blocking;

   null = r0 - $NO_RELAY_MODE;
   if Z jump no_buffer_blocking;

   null = r0 - $TWS_SLAVE_MODE;
   if NZ jump check_master_mode;

   // Check is Decoder is stalled
   call $block_interrupts;
   call $tws.check_timestamp;
   call $unblock_interrupts;
   jump no_buffer_blocking;
check_master_mode:
   null = r0 - $TWS_MASTER_MODE;
   if NZ jump shareme;

   // Check if master is enabled
#ifdef TWS_ENABLE
   push r5;
   null = M[$tws.master_enabled];
   if Z call $tws.master.enable;
   pop r5;

   null = M[$tws.master_enabled];
   if Z call no_buffer_blocking;
#endif

#ifdef TWS_WIRED_MODE_ENABLE
   // Check Wired Mode
   push r5;
   null = M[$tws.wired_mode_enabled];
   if Z call $tws.wired_mode.enable;

   null = M[$tws.wired_mode_enabled];
   if NZ call $tws.sbc_encoder;
   pop r5;
#endif

shareme:
   call $relay.update_internal_ptr;

no_buffer_blocking:
   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $relay.stop
//
// DESCRIPTION:
//
//
//
// INPUTS:
//    r5 =  relay_struct
//
// OUTPUTS:
//    - none
// *****************************************************************************
.MODULE $M.relay.stop;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   .VAR relay_temp;

   $relay.stop:
   $push_rLink_macro;

   M[relay_temp] = r5;   // Save struc address

   r0 = M[$relay.mode];
   null = r0 - $ERR_RELAY_LOST;
   if Z jump check_update_ptr;
   null = r0 - $NO_RELAY_MODE;
   if Z jump check_update_ptr;
   null = r0 - $TWS_SLAVE_MODE;
   if NZ jump check_master_shareme;

   // extract header
   r1 = &$decoder_codec_stream_struc;
   r0 = M[r1 + $codec.av_decode.MODE_FIELD];
   Null = r0 - $codec.SUCCESS;
   if NZ jump check_update_ptr;
   call $tws.extract_header;
   if NZ jump schedule_output_frame;

   // If header is bad, drop frame
   .VAR $bad_header_count = 0;
   r0 = M[$bad_header_count];
   r0 = r0 + 1;
   M[$bad_header_count] = r0;

   call $tws.bin_current_frame;

   jump done;

schedule_output_frame:


   call $tws.synchronise_slave;
   // process audio for routing
   r2 = M[$tws.routing_mode];
   r3 = M[$decoder_codec_stream_struc + $codec.av_decode.DECODER_NUM_OUTPUT_SAMPLES_FIELD];
   call $tws.audio_routing;

#ifdef TWS_ENABLE
   // Insert foreground delay processing here
   r3 = M[$decoder_codec_stream_struc + $codec.av_decode.DECODER_NUM_OUTPUT_SAMPLES_FIELD];
   r8 = &$delay_left;
   call $tws.delay.process;
   r8 = &$delay_right;
   call $tws.delay.process;
#endif

   jump done;

check_master_shareme:
   r1 = &$decoder_codec_stream_struc;
   r2 = M[r1 + $codec.av_decode.MODE_FIELD];
   Null = r2 - $codec.SUCCESS;
   if NZ jump check_update_ptr;

   null = r0 - $SHAREME_MODE;
   if Z jump relay_only;
   null = r0 - $TWS_MASTER_MODE;
   if NZ jump check_update_ptr;

   null = M[$tws.master_enabled];   // Only synchronize and fill relay buffer if port is connected
   if Z jump check_update_ptr;
   null = M[$tws.wallclock_running];
   if Z jump check_update_ptr;

   r2 = M[$tws.initial_holdoff];
   if Z jump initial_continue;
   r2 = r2 - 1;
   M[$tws.initial_holdoff]=r2;
   jump check_update_ptr;

   initial_continue:


   call $tws.synchronise_master;
   // process audio for routing
   r2 = M[$tws.routing_mode];
   r3 = M[$decoder_codec_stream_struc + $codec.av_decode.DECODER_NUM_OUTPUT_SAMPLES_FIELD];
   call $tws.audio_routing;

#ifdef TWS_ENABLE
   // Insert manual delay processing here
   r3 = M[$decoder_codec_stream_struc + $codec.av_decode.DECODER_NUM_OUTPUT_SAMPLES_FIELD];
   r8 = &$delay_left;
   call $tws.delay.process;
   r8 = &$delay_right;
   call $tws.delay.process;
#endif

relay_only:
   r5 = M[relay_temp];
   call $relay.copy_data;

check_update_ptr:

   r0 = M[$relay.mode];
   null = r0 - $TWS_MASTER_MODE;
   if NZ jump skip_decode_cnt;
   r1 = M[$tws.decode_cnt];
   r1 = r1 + 1;
   r0 = 200;
   null = r1 - r0;
   if POS r1 = r0;
   M[$tws.decode_cnt] = r1;

   skip_decode_cnt:
   r0 = M[r5 + $relay.INTERNAL_CBUFFER_PTR_FIELD];
   call $cbuffer.get_write_address_and_size;
   if Z jump done; // master did not block decoder cbuffer

   r5 = M[relay_temp];
   call $relay.update_codec_ptr;    // Unblock decoder cbuffer

   // clear write address
   r1 = 0;
   r0 = M[r5 + $relay.INTERNAL_CBUFFER_PTR_FIELD];
   call $cbuffer.set_write_address;

   done:
   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.tws.synchronise_master
//
// DESCRIPTION:
//    Calculates when master will play the current frame of audio and write
//    this value to its TWS Header Structure so it can be relayed to the slave.
//    Also schedules the sample rate adjusment value to be updated in the
//    application such that the updates occurs on the TWS master and slave
//    devices at the same time.
//
// INPUTS:
//    - none
//
// OUTPUTS:
//    - none
// *****************************************************************************
.MODULE $M.tws.synchronise_master;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   $tws.synchronise_master:
   // compute playback time and schedule warp
   $push_rLink_macro;

   call $block_interrupts; // to prevent av_dac_copy from inserting samples

   call $tws.check_started_streaming;
   Null = M[$tws.started_streaming];
   if NZ call $tws.feed_dac_buffer;

   r7 = &$pcm_latency_input_struct;
   call $latency.calc_pcm_latency;
   r1 = $tws.wallclock_obj;
   call $wall_clock.get_time;
   r1 = r1 + r6;
   M[$tws.header_struc + $tws.header.PLAYBACK_TIME_FIELD] = r1;

   // Copy Target Warp to header
   r0 = M[$tws.scheduled_sra];
   M[$tws.header_struc + $tws.header.SRA_TARGET_RATE_FIELD] = r0;

   // Copy Actual Warp to header
   r0 = M[$codec_rate_adj.stereo + $cbops.rate_adjustment_and_shift.Process.SRA_CURRENT_RATE_FIELD];
   M[$tws.header_struc + $tws.header.SRA_ACTUAL_RATE_FIELD]=r0;

   // Copy volume message to header - TBD
   r3 = &$tws.message_volume_struc;
   r2 = $NUM_SYNCED_VOLUMES_MASTER; //copy only system, master and tone
   r1 = &$tws.header_struc + $tws.header.SYS_VOL_FIELD;
   call $tws.copy_array;

   // Apply slave trims to header
   r0 = M[$tws.device_trim_slave];
   M[&$tws.header_struc + $tws.header.TRIM_LEFT_FIELD] = r0;

   r0 = M[$tws.device_trim_slave];
   M[&$tws.header_struc + $tws.header.TRIM_RIGHT_FIELD] = r0;

   r0 = M[$tws.message_routing_mode_slave];
   r0 = $DATA_NOT_SCHEDULED AND r0;
   r0 = r0 OR $TWS_V3_PROTOCOL_ID;
   M[$tws.header_struc + $tws.header.ROUTING_MODE_FIELD]=r0;

   // schedule warp update
   r1 = &$tws.set_synchronized_data_message_struc;
   null = M[r1 + $timer.ID_FIELD];
   if NZ jump do_not_set_timer;

#ifdef TWS_NO_FIX
   r0 = M[$sra_struct + $sra.RATE_BEFORE_FIX_FIELD ];
#else
   r0 = M[$sra_rate_addr];
   r0 = M[r0];
#endif

      M[$tws.header_struc + $tws.header.SRA_TARGET_RATE_FIELD] = r0;
      M[$tws.scheduled_sra] = r0;

      // Copy volume message to scheduled volume
      r3 = &$tws.message_volume_struc;
      r2 = length($tws.scheduled_volume_struc);
      r1 = &$tws.scheduled_volume_struc;
      call $tws.copy_array;

      // Apply master trim to all trims  - TBD make this a loop!!!

      I0 = &$tws.scheduled_volume_struc + $PRI_LEFT_FIELD;
      r0 = M[$tws.device_trim_master];

      r10  = $MAIN_VOL_MESSAGE_SIZE - 4;
      do apply_trim_loop;
         r1 = M[I0,0];
         r1 = r1 + r0;
         M[I0,1] = r1;
      apply_trim_loop:


      r0 = M[$tws.message_routing_mode_master];
      M[$tws.scheduled_routing_mode] = r0;

      r0 = M[$tws.header_struc + $tws.header.ROUTING_MODE_FIELD];
      r0 = $DATA_SCHEDULED OR r0;
      M[$tws.header_struc + $tws.header.ROUTING_MODE_FIELD] = r0;

      r0 = M[$tws.header_struc + $tws.header.PLAYBACK_TIME_FIELD];
      r1 = M[$tws.wallclock_obj + $wall_clock.ADJUSTMENT_VALUE_FIELD];
      r2 = r0 - r1;
      r1 = &$tws.set_synchronized_data_message_struc;
      r3 = &$tws.set_synchronized_data_handler;
      call $timer.schedule_event_at;
      M[$tws.warp_timer_id] = r3;

   sample_rate_not_supported: // TODO
   do_not_set_timer:

   call $unblock_interrupts;
   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.tws.syncrhonise_slave
//
// DESCRIPTION:
//    Uses values from the TWS header to syncrhonise audio playback and other
//    events, such as warp and volume updates.
//
// INPUTS:
//    - none
//
// OUTPUTS:
//    - none
// *****************************************************************************
.MODULE $M.tws.synchronise_slave;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   $tws.synchronise_slave:
   .VAR $tws.delay_intialized=0;
   .VAR $tws.debug_latency = 0;
   .VAR $tws.debug_M_ttp = 0;
   .VAR $tws.debug_S_ttp = 0;
   .VAR $tws.debug_delta_ttp = 0;



   // compute playback time and schedule warp
   $push_rLink_macro;

      // calculate wall clock time when slave will play this frame
      r7 = &$pcm_latency_input_struct;
      call $latency.calc_pcm_latency;
      // latency is returned in r6
      M[$tws.debug_latency] = r6;
      r1 = $tws.wallclock_obj;
      call $wall_clock.get_time;
      .var $local_wallclock;
      M[$local_wallclock] = r1;
      r1 = r1 + r6;
      M[$tws.debug_S_ttp] = r1;

      // compare when slave will play to when master will play
      r0 = M[$tws.header_struc + $tws.header.PLAYBACK_TIME_FIELD];
      M[$tws.debug_M_ttp] = r0;

      r0 = r0 - r1;
      M[$tws.debug_delta_ttp] = r0;
      if NEG jump slave_will_play_later_than_master;

      // if master will play much later than slave fix it by resetting delay value
      Null = r0 - $TWS_SYNC_TOLERANCE;
      if NEG jump schedule_events;

   // Need to set the delay because master is going to play at least $TWS_SYNC_TOLERANCE
   // us after the slave or vice versa.  Convert from microseconds to samples
   // and set delay buffer.  Bin the data if the delay buffer isn't big enough.
   set_synchronisation_delay:
      // check if delay is set

      // slave cannot precisely synchronise if there is a warp mismatch so
      // we need to wait for the scheduled warp
      //r0 = M[$tws.header_struc + $tws.header.ROUTING_MODE_FIELD];
      //r0 = $DATA_SCHEDULED AND r0;
      // if Z jump zero_frame;
      // TODO: If delay is already set it means we've been streaming.
      // We should try to soft mute over audio before binning it all.

      call $tws.local_volume_mute;

      call $tws.zero_input_audio;
      call $tws.feed_dac_buffer;
      M[$decoder_codec_stream_struc + $codec.av_decode.CURRENT_RUNNING_MODE_FIELD ] = 0;

      // Seed rate adjust operators with actual warp
      r0 = M[$tws.header_struc + $tws.header.SRA_TARGET_RATE_FIELD ];
      M[$tws.synchronised_sra] = r0;
      r0 = M[$tws.header_struc + $tws.header.SRA_ACTUAL_RATE_FIELD];
      M[$codec_rate_adj.stereo + $cbops.rate_adjustment_and_shift.Process.SRA_CURRENT_RATE_FIELD] = r0;

      // cancel message handler
      r1 = &$tws.set_synchronized_data_message_struc;
      r2 = M[r1 + $timer.ID_FIELD];
      if NZ call $timer.cancel_event;

      // Check Primary output cbuffer - if no data wait to set delay....
      r0 = &$multi_chan_primary_left_out_cbuffer_struc;
      call $cbuffer.calc_amount_data;
      null = r0;
      if NZ jump reset_delay;
      r0 = &$multi_chan_primary_right_out_cbuffer_struc;
      call $cbuffer.calc_amount_data;
      null = r0;
      if Z jump do_not_set_timer;

   reset_delay:
      // calculate wall clock time when slave will play this frame now that
      // slave's delay has been set to zero.
      .VAR $delay_reset_cnt = 0;

      r0 = M[$delay_reset_cnt];
      r0 = r0 + 1;
      M[$delay_reset_cnt] = r0;


      r7 = &$pcm_latency_input_struct;
      call $latency.calc_pcm_latency;
      // latency is returned in r6
      r1 = $tws.wallclock_obj;
      call $wall_clock.get_time;
      r1 = r1 + r6;

      // compare when slave will play to when master will play
      r0 = M[$tws.header_struc + $tws.header.PLAYBACK_TIME_FIELD];
      r0 = r0 - r1;

      // It's not very reasonable to have a delay of 0
      // it means there's a lot of delay (~170ms) between the master and slave
      // If it zero, likely the first frame was too old and we binned data until
      // we got to more recent data.  Rather than trigger on zero, we should bin until
      // we have some data that sets the delay.
      .VAR $bin_too_old = 0;

      Null = r0 - 10000;
      if GE jump sync_continue;
      r0 = M[$bin_too_old];
      r0 = r0 + 1;
      M[$bin_too_old] = r0;
      jump bin_frame;

      sync_continue:
      r1 = M[$current_codec_sampling_rate];
      Null = r1 - 48000;
      if NZ jump check_44100;
      r1 = 0.048;
      jump calculate_delay;

   check_44100:
      Null = r1 - 44100;
      if NZ jump check_32000;
      r1 = 0.0441;
      jump calculate_delay;

   check_32000:
      Null = r1 - 32000;
      if NZ jump check_16000;
      r1 = 0.032;
      jump calculate_delay;

   check_16000:
      Null = r1 - 16000;
      if NZ jump sample_rate_not_supported;
      r1 = 0.016;

   calculate_delay:
      r1 = r0 * r1 (frac);
      r0 = M[$tws.header_struc + $tws.header.SRA_ACTUAL_RATE_FIELD];
      r2 = r1 * r0 (frac);
      r1 = r1 - r2;

      .VAR $bin_too_fast = 0;
      r0 = M[$tws.alignment_delay];
      Null = r0 - r1;
      if GE jump recalc_continue; // delay buffer isn't big enough
      r0 = M[$bin_too_fast];
      r0 = r0 + 1;
      M[$bin_too_fast] = r0;
      jump bin_frame;
      recalc_continue:
#ifdef TWS_ENABLE
      M[$delay_left + $tws.delay.DELAY_FIELD] = r1;
      M[$delay_right + $tws.delay.DELAY_FIELD] = r1;
#endif

      M[$M.main.samples_latency_measure + 0] = r1;
      r1 = 1;
      M[$tws.delay_intialized] = r1;

      // cancel message handler
      r1 = &$tws.softmute_data_timer_struc;
      r2 = M[r1 + $timer.ID_FIELD];
      if NZ call $timer.cancel_event;

      r2 = 0;//$TWS_SOFTMUTE_TIME;
      r1 = &$tws.softmute_data_timer_struc;
      r3 = &$tws.local_volume_unmute;
      call $timer.schedule_event_in;

      // calculate wall clock time when slave will play this frame
      r7 = &$pcm_latency_input_struct;
      call $latency.calc_pcm_latency;
      // latency is returned in r6
      r1 = $tws.wallclock_obj;
      call $wall_clock.get_time;
      r1 = r1 + r6;
      r0 = M[$tws.header_struc + $tws.header.ROUTING_MODE_FIELD];
      r0 = r0 OR $DATA_SCHEDULED;
      M[$tws.header_struc + $tws.header.ROUTING_MODE_FIELD] = r0;
      // compare when slave will play to when master will play
      r0 = M[$tws.header_struc + $tws.header.PLAYBACK_TIME_FIELD];
      r0 = r0 - r1;
   schedule_events:

      //r0 -> delta TTP
     call $tws.calc_sra;

     r0 = M[$tws.header_struc + $tws.header.ROUTING_MODE_FIELD];
     r0 = $DATA_SCHEDULED AND r0;
     if Z jump do_not_set_timer;

         r1 = &$tws.set_synchronized_data_message_struc;
         r2 = M[r1 + $timer.ID_FIELD];
         if NZ call $timer.cancel_event;

         r0 = M[$tws.header_struc + $tws.header.ROUTING_MODE_FIELD];
         r1 = $VERSION_MASK AND r0;
         null = r1 - $TWS_V3_PROTOCOL_ID;
         if Z jump supported_tws_version;

         // Send message to VM reporting unsupported TWS version
         r2 = $MESSAGE_DSP_TWS_ERROR;
         r3 = $TWS_VERSION_UNSUPPORTED;
         r3 = r3 OR r1;          // combine error with supported version
         call $message.send;

         jump do_not_set_timer;

supported_tws_version:
         r0 = $MODE_MASK AND r0;
         M[$tws.scheduled_routing_mode] = r0;  // May need to add a mask

         // load scheduled sra value received from master
         r0 = M[$tws.header_struc + $tws.header.SRA_TARGET_RATE_FIELD];
         M[$tws.scheduled_sra] = r0;

         r1 = &$tws.scheduled_volume_struc;
         r2 = $MAIN_VOL_MESSAGE_SIZE; //copy all for trims
         r3 = &$tws.message_volume_struc;
         call $tws.copy_array;


         // Copy volume message to scheduled volume
         r1 = &$tws.scheduled_volume_struc;
         r2 = $NUM_SYNCED_VOLUMES_SLAVE; //copy only system, master
         r3 = &$tws.header_struc + $tws.header.SYS_VOL_FIELD;
         call $tws.copy_array;

         // Apply slave trim to all trims  - TBD make this a loop!!!
         I0 = &$tws.scheduled_volume_struc + $PRI_LEFT_FIELD;
         I4 = &$tws.header_struc + $tws.header.TRIM_LEFT_FIELD;
         m0 = 1;

         r10  = $MAIN_VOL_MESSAGE_SIZE - 4;
         do apply_slave_trim_loop;
            r1 = M[I0,0];
            r0 = M[I4,m0];
            r1 = r1 + r0;
            M[I0,1] = r1;
            m0 = 0 - m0;

      apply_slave_trim_loop:

         r0 = M[$tws.header_struc + $tws.header.PLAYBACK_TIME_FIELD];
         r1 = M[$tws.wallclock_obj + $wall_clock.ADJUSTMENT_VALUE_FIELD];
         r2 = r0 - r1;

         r1 = &$tws.set_synchronized_data_message_struc;
         r3 = &$tws.set_synchronized_data_handler;
         call $timer.schedule_event_at;
         M[$tws.warp_timer_id] = r3;

   do_not_set_timer:
   sample_rate_not_supported: // TODO
   done_synchronise:
   jump $pop_rLink_and_rts;

   slave_will_play_later_than_master:
      // If slave will play much later than master reset delay.  Otherwise schedule events.
      Null = r0 + $TWS_SYNC_TOLERANCE;
      if POS jump schedule_events;
      jump set_synchronisation_delay;

   bin_frame:

   call $tws.bin_current_frame;
   jump done_synchronise;
   zero_frame:

   call $tws.zero_current_frame;
      jump done_synchronise;

.ENDMODULE;
//**************************************************************
// MODULE $M.tws.sbc_encoder
//
// DESCRIPTION:
//       Encodes audio into SBC packets
// INPUTS:
//    - none
//
// OUTPUTS:
//    - none
// *****************************************************************************

.MODULE  $M.tws.sbc_encoder;
   .CODESEGMENT   PM;
   .DATASEGMENT   DM;

$tws.sbc_encoder:

$push_rLink_macro;

   // running reset function if required
   Null = M[$codec_reset_needed];
   if Z jump no_codec_reset;
      r5 = &$av_encoder_codec_stream_struc;
      r0 = M[r5 + $codec.stream_encode.RESET_ADDR_FIELD];
      //r5 has to be encoder data structure pointer, rather than stream encoder struct ptr
      r5 = r5 + $codec.av_encode.ENCODER_STRUC_FIELD;
      call r0;
      M[$codec_reset_needed] = null;

no_codec_reset:

   // encode a frame
   r5 = &$av_encoder_codec_stream_struc;
   call $codec.av_encode;

   // if for any reason (not enough input/output data/space) codec
   // doesnt generate any output then wait one milisecond, so conditions to
   // call the encoder would be met in next try
   r5 = &$av_encoder_codec_stream_struc;
   r0 = M[r5 + $codec.av_encode.MODE_FIELD];
   r0 = r0 - $codec.SUCCESS;

   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $tws.extract_header
//
// DESCRIPTION:
//    Extracts sync header
//
// INPUTS:
//    - r5 = pointer to sync structure
//
// OUTPUTS:
//    - r0 = 1 if header is found and frame is not too old to play
//      r0 = 0 if header is not found or if frame is too old to play
// *****************************************************************************
.MODULE $M.tws.extract_header;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   .VAR success = 0;
   .VAR $findsync_cnt = 0;
   .VAR struc_temp;

   $tws.extract_header:

   $push_rLink_macro;

   // assume sync header is not found
   M[success] = Null;

   M[$findsync_cnt] = Null;
   // load number of bytes available in bitstream
   r0 = M[r5 + $relay.INPUT_CBUFFER_FIELD];
   call $cbuffer.calc_amount_data;
   r4 = r0 ASHIFT 1;

   // get read pointer
   r0 = M[r5 + $relay.INPUT_CBUFFER_FIELD];
   call $cbuffer.get_read_address_and_size;
   I0 = r0;
   L0 = r1;

   // save read pointer and bitpos in case they need to be restored
   r7 = r0;

   // get current bitpos, corresponding to the end of frame
   r8 = M[r5 + $relay.CODEC_GET_BITPOS];
   r8 = M[r8];

  // Search for sync header
   r3 = 0;
   findsyncloop:
      //check to see if we can get another byte from the codec input buffer
      r4 = r4 - 1;
      if NEG jump buffer_underflow_occured;

      // read a byte
      r0 = 8;
      push r3;
      r2 = M[r5 + $relay.CODEC_GETBITS];
      call r2;
      pop r3;
      r3 = r3 LSHIFT 8;
      r3 = r3 + r1;

      r6 = M[$findsync_cnt];
      r6= r6 + 1;
      M[$findsync_cnt] = r6;

      // check to see if it is a sync word
      Null = r3 - 0x435352;
      if  Z jump continue1;

   jump findsyncloop;

continue1:

   r1 = M[r5 + $relay.TWS_HEADER_FIELD];
   I6 = r1 + $tws.header.ROUTING_MODE_FIELD;

   r10 = M[r1 + $tws.header.SIZE_FIELD];
   r10 = r10 - 1;
   do readloop;
      //check to see if we can get another word from the codec input buffer
      r4 = r4 - 3;
      if NEG jump buffer_underflow_occured;

      // read a word
      r0 = 8;
      push r3;
      r2 = M[r5 + $relay.CODEC_GETBITS];
      call r2;
      pop r3;
      r3 = r3 LSHIFT 8;
      r3 = r3 + r1;

      push r3;
      r2 = M[r5 + $relay.CODEC_GETBITS];
      call r2;
      pop r3;
      r3 = r3 LSHIFT 8;
      r3 = r3 + r1;

      push r3;
      r2 = M[r5 + $relay.CODEC_GETBITS];
      call r2;
      pop r3;
      r3 = r3 LSHIFT 8;
      r3 = r3 + r1;
      M[I6,1] = r3;

   readloop:
      r0 = M[$relay_struc + $relay.IS_APTX_FIELD];
      if Z jump continue;
      r0 = M[I0,1];        //Dummy read to increment I0
      r0 = 16;
      r1 = M[r5 + $relay.CODEC_GET_BITPOS];  // Set bitpos to 16, so that next read from memory is 16 bits
      M[r1] = r0;

continue:
   r1 = 1;
   M[success] = r1 ;
   jump done_getting_header;

buffer_underflow_occured:
   // restore pointers
   I0 = r7;
   r1 = M[r5 + $relay.CODEC_GET_BITPOS];
   M[r1] = r8;

   jump clean_up;

done_getting_header:
   r0 = M[r5 + $relay.INPUT_CBUFFER_FIELD];
   r1 = I0;
   call $cbuffer.set_read_address;

clean_up:
   L0 = 0;
   r0 = M[success];

   jump $pop_rLink_and_rts;

.ENDMODULE;
// *****************************************************************************
// MODULE:
//    $tws.local_volume_mute
//
// DESCRIPTION:
//    handle routing changes from  VM
//
// INPUTS:
//
//
// OUTPUTS:
//    - none
// *****************************************************************************
.MODULE $M.tws.local_volume_mute;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   $tws.local_volume_mute:
   $push_rLink_macro;

   r0 = M[$tws.local_mute];
   if NZ jump done;

   I0 = &$tws.local_saved_volume_struc;
   r1 = M[$music_example.SystemVolume];
   M[I0,1] = r1;
   r1 = M[$music_example.Main.MasterVolume];
   M[I0,1] = r1;
   r1 = M[$music_example.Main.ToneVolume];
   M[I0,1] = r1;
   r1 = M[$music_example.Main.PrimaryLeftTrimVolume];
   M[I0,1] = r1;
   r1 = M[$music_example.Main.PrimaryRightTrimVolume];
   M[I0,1] = r1;
   I0 = null;

   skip_save:

   r3 = &$tws.local_mute_volume_struc;
   call $M.music_example_message.MainVolume.func;
   r1 = 1;
   M[$tws.local_mute] = r1;
done:
   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $tws.local_volume_unmute
//
// DESCRIPTION:
//    restore audio from a local softmute
//
// INPUTS:
//   -none
//
// OUTPUTS:
//    - none
// *****************************************************************************
.MODULE $M.tws.local_volume_unmute;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   $tws.local_volume_unmute:
   $push_rLink_macro;

   M[$tws.local_mute] = Null;

done:
   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $tws.volume_s_message_handler
//
// DESCRIPTION:
//    Handler for the replacement short multi-channel configuration message.
//    (this uses the original corresponding long configuration message handler)
//    Message from VM->DSP to specify the volume status of all main wired outputs
//    (Note 1: all main wired multi-channel outputs are specified)
//    (Note 2: r1 selects the parameters being set by the message
//     Parameters are cached until a message with r1 = 0 is received
//     this sets all the parameters atomically from the cache.)
//
// INPUTS:
//
//    r1 = volume select = 0                       <=  This must be sent to synchronise
//    r2 = system volume index (i.e. DAC index)        other main volume changes
//    r3 = master volume (dB)*60
//    r4 = tone volume (dB)*60

//    r1 = volume select = 1
//    r2 = Left primary trim volume (dB)*60
//    r3 = Right primary trim volume (dB)*60
//    r4 = <not used>

//    r1 = volume select = 2
//    r2 = Left secondary trim volume (dB)*60
//    r3 = Right secondary trim volume (dB)*60
//    r4 = Wired subwoofer trim (dB)*60

// OUTPUTS:
//    none
//
// TRASHED REGISTERS:
//    r0, r1, r2, r3, r4, r5, r8, I0
//
// *****************************************************************************
.MODULE $M.tws.volume_s_message_handler;

   .CODESEGMENT PM;

$tws.volume_s_message_handler:

   // Point the temporary message payload cache
   r5 = $M.music_example_message_payload_cache.multi_volume_s;
   I0 = r5;

   null = r1;                          // Test the select
   if NZ jump skip_select0;

      // Load parameters into cache then process all the volume parameters from the cache
      M[I0,1] = r2;                    // System volume index
      M[I0,1] = r3;                    // Master volume (dB)*60
      M[I0,1] = r4;                    // Tone volume (dB)*60

      // Point at the payload cache
      r3 = r5;

      // Execute the original long message handler (input: r3 points at the payload cache)
      jump $tws.volume_message_handler;

   skip_select0:

   null = r1 - 1;                      // Test the select
   if NZ jump skip_select1;

      // Load parameters into cache then exit without processing
      M0 = 3;
      r0 = M[I0,M0];                   // Dummy read to skip other parameters in cache
      M[I0,1] = r2;                    // Left primary trim volume (dB)*60
      M[I0,1] = r3;                    // Right primary trim volume (dB)*60

      jump exit;                       // Exit without processing
   skip_select1:

   null = r1 - 2;                      // Test the select
   if NZ jump skip_select2;

      // Load parameters into cache then exit without processing
      M0 = 5;
      r0 = M[I0,M0];                   // Dummy read to skip other parameters in cache
      M[I0,1] = r2;                    // Left secondary trim volume (dB)*60
      M[I0,1] = r3;                    // Right secondary trim volume (dB)*60
      M[I0,1] = r4;                    // Wired subwoofer trim (dB)*60

   skip_select2:
   exit:

   // Exit without processing volume parameters
   rts;
.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $tws.volume_message_handler
//
// DESCRIPTION:
//    Handle VM schedule and relay volume change
//
//
// INPUTS:
//     r3 = message pay load
//     word0 = sytem volume index
//     word1 = master_volume_dB * 60
//     word2 = tone_volume_dB * 60
//     word3 = left_trim_volume_dB * 60
//     word4 = right_trim_volume_dB * 60
//
// OUTPUTS:
//    - none
// *****************************************************************************
.MODULE $M.tws.volume_message_handler;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   $tws.volume_message_handler:
   $push_rLink_macro;


   // Copy structure pointed to by r3 to local structure
   r1 = &$tws.message_volume_struc;
   r2 = length($tws.message_volume_struc);
   call $tws.copy_array;

   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $tws.device_trim_message_handler
//
// DESCRIPTION:
//    Handle device trim messages from VM
//
//
// INPUTS:
//     r1 - master_trim_vol_db * 60
//     r2 - slave_trim_vol_db * 60
//
// OUTPUTS:
//    - none
// *****************************************************************************
.MODULE $M.tws.device_trim_message_handler;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   $tws.device_trim_message_handler:
   $push_rLink_macro;

   M[$tws.device_trim_master] = r1;
   M[$tws.device_trim_slave] = r2;

   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $tws.relay_control_message_handler
//
// DESCRIPTION:
//    Handle relay port data flow control message from VM
//
//
// INPUTS:
//     r1 - Relay packet size  (in 16 bit words)
//     r2 - Relay packet time interval
//
// OUTPUTS:
//    - none
// *****************************************************************************
.MODULE $M.tws.relay_control_message_handler;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   $tws.relay_control_message_handler:
   $push_rLink_macro;
   r0 = $relay_limited_amt_op.param;
   M[r0 + $cbops.limited_amount.AMOUNT_FIELD] = r1;
   M[r0 + $cbops.limited_amount.FLUSH_THRESHOLD_FIELD ] = r2;

   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $tws.external_volume_enable_message_handler
//
// DESCRIPTION:
//    Handle request to use external volume control
//
//
// INPUTS:
//     r1 =  external volume enable flag
//
// OUTPUTS:
//    - none
// *****************************************************************************
.MODULE $M.tws.external_volume_enable_message_handler;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   $tws.external_volume_enable_message_handler:
   $push_rLink_macro;

   M[$tws.external_volume_enable] = r1;
      // enable volume return messages

   r2 = M[$DAC_conn_aux];
   r2 = r1 OR r2;
   M[$aux_vol_msg_echo] = r2;

   r2 = M[$DAC_conn_main];
   r2 = r1 OR r2;
   M[$multichannel_vol_msg_echo] = r2;


   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $tws.copy_array
//
// DESCRIPTION:
//    copy one array to another
//
// INPUTS:
//     r1  = address of destination
//     r2  = length
//     r3  = address of source
//
// OUTPUTS:
//    - none
// *****************************************************************************
.MODULE $M.tws.copy_array;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   $tws.copy_array:
   $push_rLink_macro;

   // Copy structure pointed to by r3 to structure pointed to by r1
   I0 = r1; //destination address
   I1 = r3; //source address
   r10 = r2; // number of elements to copy
   do copy_loop;
      r0 = M[I1,1];
      M[I0,1] = r0;
   copy_loop:

   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $tws.routing_message_handler
//
// DESCRIPTION:
//    handle routing changes from  VM
//
//
// INPUTS:
//    r1 - master routing mode from the VM
//    r2 - slave routing mode from the VM
//
// OUTPUTS:
//    - none
// *****************************************************************************
.MODULE $M.tws.routing_message_handler;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   .VAR $set_soft_mute;

   $tws.routing_message_handler:
   $push_rLink_macro;

   M[$tws.message_routing_mode_master] = r1;
   M[$tws.message_routing_mode_slave] = r2;

   r1 = -1;
   M[$set_soft_mute] = r1;

   jump $pop_rLink_and_rts;

.ENDMODULE;



// *****************************************************************************
// MODULE:
//    $tws.soft_mute_handler
//
// DESCRIPTION:
//    handle routing changes from  VM
//
// INPUTS:
//   r6 - Time in us to be passed to unmute handler
//
// OUTPUTS:
//    - none
// *****************************************************************************
.MODULE $M.tws.soft_mute_handler;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   $tws.soft_mute_handler:
   $push_rLink_macro;

   // mute Output Audio
   r1 = -1;
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch0_mute_op, param, $cbops.soft_mute_op.MUTE_DIRECTION, r1)
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch1_mute_op, param, $cbops.soft_mute_op.MUTE_DIRECTION, r1)
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch2_mute_op, param, $cbops.soft_mute_op.MUTE_DIRECTION, r1)
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch3_mute_op, param, $cbops.soft_mute_op.MUTE_DIRECTION, r1)
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch4_mute_op, param, $cbops.soft_mute_op.MUTE_DIRECTION, r1)
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch5_mute_op, param, $cbops.soft_mute_op.MUTE_DIRECTION, r1)
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain1_ch0_mute_op, param, $cbops.soft_mute_op.MUTE_DIRECTION, r1)
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain1_ch1_mute_op, param, $cbops.soft_mute_op.MUTE_DIRECTION, r1)

   // cancel message handler
   r1 = &$soft_unmute_timer_struc;
   r2 = M[r1 + $timer.ID_FIELD];
   if NZ call $timer.cancel_event;

   //Set timer to reset soft mute and resume audio out
   r1 = $soft_unmute_timer_struc;
   r2 = M[$soft_mute_duration];
   r3 = $tws.soft_unmute_handler;
   call $timer.schedule_event_in;

   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $tws.soft_unmute_handler
//
// DESCRIPTION:
//    handle routing changes from  VM
//
// INPUTS:
//   -none
//
// OUTPUTS:
//    - none
// *****************************************************************************
.MODULE $M.tws.soft_unmute_handler;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   $tws.soft_unmute_handler:
   $push_rLink_macro;

   // Unmute Output Audio
   r1 = 1;
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch0_mute_op, param, $cbops.soft_mute_op.MUTE_DIRECTION, r1)
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch1_mute_op, param, $cbops.soft_mute_op.MUTE_DIRECTION, r1)
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch2_mute_op, param, $cbops.soft_mute_op.MUTE_DIRECTION, r1)
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch3_mute_op, param, $cbops.soft_mute_op.MUTE_DIRECTION, r1)
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch4_mute_op, param, $cbops.soft_mute_op.MUTE_DIRECTION, r1)
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch5_mute_op, param, $cbops.soft_mute_op.MUTE_DIRECTION, r1)
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain1_ch0_mute_op, param, $cbops.soft_mute_op.MUTE_DIRECTION, r1)
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain1_ch1_mute_op, param, $cbops.soft_mute_op.MUTE_DIRECTION, r1)



   jump $pop_rLink_and_rts;

.ENDMODULE;


#ifdef APTX_ENABLE
// *****************************************************************************
// MODULE:
//    $aptxdec.getbits
//
// DESCRIPTION:
//    Get bits from encoded Stream.
//
// INPUTS:
//    - r0 = number of bits to get from buffer
//    - I0 = buffer pointer to read words from
//    - $get_bitpos = previous val (should be initialised to 16)
//
// OUTPUTS:
//    - r0 = unaffected
//    - r1 = the data read from the buffer
//    - I0 = buffer pointer to read words from (updated)
//    - $get_bitpos   = updated
//
// TRASHED REGISTERS:
//    r2, r3
//
// *****************************************************************************
.MODULE $M.aptxdec.getbits;
   .CODESEGMENT PM;
   .DATASEGMENT DM;
   .VAR $aptxdec.get_bitpos = 16;
   .VAR/DM1 $aptxdec.bitmask_lookup[17] =
                 0b0000000000000000,
                 0b0000000000000001,
                 0b0000000000000011,
                 0b0000000000000111,
                 0b0000000000001111,
                 0b0000000000011111,
                 0b0000000000111111,
                 0b0000000001111111,
                 0b0000000011111111,
                 0b0000000111111111,
                 0b0000001111111111,
                 0b0000011111111111,
                 0b0000111111111111,
                 0b0001111111111111,
                 0b0011111111111111,
                 0b0111111111111111,
                 0b1111111111111111;
   $aptxdec.getbits:
   r3 = M[$aptxdec.bitmask_lookup + r0];    // form a bit mask (r3)
   r2 = r0 - M[$aptxdec.get_bitpos];        // r2 = shift amount
   r1 = M[I0, 0];                       // r1 = the current word
   r1 = r1 LSHIFT r2;                   // shift current word
   r1 = r1 AND r3;                      // extract only the desired bits
   Null = r2 + 0;
// ??? Tests C and/or V flag, but previous instruction will not modify those flags
   if   LE jump one_word_only;          //check if we need to read the next word
   r3 = M[I0,1];                        // increment I0 to point to the next word
   r3 = M[I0,0];                        // get another word from buffer (r3)
   r2 = r2 - 16;                        // calc new shift amount
   r3 = r3 LSHIFT r2;                   // and shift
   r1 = r1 + r3;                        // combine the 2 parts
one_word_only:
   M[$aptxdec.get_bitpos] = Null - r2;      // update get_bitpos
   rts;
.ENDMODULE;
#endif   // SELECTED_CODEC_APTX

// *****************************************************************************
// MODULE:
//    $M.tws.configure_synchronised_rate_warping
//
// DESCRIPTION:
//    Take control of when the application's warp is applied so it can be
//    applied on the master and slave TWS devices at the same time.
//
// INPUTS:
//    - none
//
// OUTPUTS:
//    - none
//
// TRASHED REGISTERS:
//    r0
//
// *****************************************************************************
.MODULE $M.tws.configure_synchronised_warping;
   .CODESEGMENT PM;

   $tws.configure_synchronised_warping:
   $push_rLink_macro;
   // Set SRA to react faster when in TWS mode
   r0 = 1;
   call $tws.set_avg_fraction;

   // seed $tws.synchronised_sra with value that application is currently using.
   r0 = M[$sra_rate_addr];
   r0 = M[r0];
   M[$tws.synchronised_sra] = r0;

   // update rate adjusment operator to watch TWS-controlled rate
   r0 = &$tws.synchronised_sra;
   M[$codec_rate_adj.stereo + $cbops.rate_adjustment_and_shift.Process.SRA_TARGET_RATE_ADDR_FIELD] = r0;

   jump $pop_rLink_and_rts;;

.ENDMODULE;

//------------------------------------------------------------------------------
.MODULE $M.tws.set_synchronized_data_handler;
//------------------------------------------------------------------------------
    .CODESEGMENT PM;
    .DATASEGMENT DM;

$tws.set_synchronized_data_handler:
   .VAR $debug_sra_frac = 0;

    $push_rLink_macro;

   M[$tws.warp_timer_id] = Null;
   r0 = M[$tws.scheduled_sra];
   r1 = M[$relay.mode];

   null = r1 - $TWS_MASTER_MODE;
   if Z jump no_extra_warp_required;


      r1 = M[$tws.slave_sra];
      r0 = r1 + r0;

      r2 = M[$tws.nudge_factor];
      r1 = r1 * r2(frac);
      M[$debug_sra_frac] = r1;

      r3 = M[$codec_rate_adj.stereo + $cbops.rate_adjustment_and_shift.Process.SRA_CURRENT_RATE_FIELD];
      r3 = r3 + r1;
      M[$codec_rate_adj.stereo + $cbops.rate_adjustment_and_shift.Process.SRA_CURRENT_RATE_FIELD] = r3;
   no_extra_warp_required:

   M[$tws.synchronised_sra] = r0;

   // Check if volume has changed...
   // compare scheduled_volume_struc with current volume
   I0 = &$tws.scheduled_volume_struc;
   r1 = M[$music_example.SystemVolume];
   r0 = M[I0,1];
   null = r1 - r0;
   if NZ jump send_vol_change;
   r1 = M[$music_example.Main.MasterVolume] ;
   r0 = M[I0,1];
   null = r1 - r0;
   if NZ jump send_vol_change;
   r1 = M[$music_example.Main.ToneVolume];
   r0 = M[I0,1];
   null = r1 - r0;
   if NZ jump send_vol_change;
   r1 = M[$music_example.Main.PrimaryLeftTrimVolume];
   r0 = M[I0,1];
   null = r1 - r0;
   if NZ jump send_vol_change;
   r1 = M[$music_example.Main.PrimaryRightTrimVolume];
   r0 = M[I0,1];
   null = r1 - r0;
   if NZ jump send_vol_change;
   r1 = M[$music_example.Main.SecondaryLeftTrimVolume];
   r0 = M[I0,1];
   null = r1 - r0;
   if NZ jump send_vol_change;
   r1 = M[$music_example.Main.SecondaryRightTrimVolume];
   r0 = M[I0,1];
   null = r1 - r0;
   if NZ jump send_vol_change;
   r1 = M[$music_example.Main.SubTrimVolume];
   r0 = M[I0,1];
   null = r1 - r0;
   if NZ jump send_vol_change;
   jump schedule_routing;


send_vol_change:
   r0 = M[$tws.external_volume_enable];

   if Z jump digital_vol;

   r3 = $music_example.VMMSG.MESSAGE_MAIN_VOLUME_RESP;
   r4 = $MAIN_VOL_MESSAGE_SIZE;
   r5 = &$tws.scheduled_volume_struc;
   call $message.send_long;

   jump schedule_routing;

digital_vol:

   r0 = M[$tws.local_mute];
   if NZ jump update_saved_vol;
   r3 = &$tws.scheduled_volume_struc;
   call $M.music_example_message.MainVolume.func;

   jump schedule_routing;

update_saved_vol:

   r3 = &$tws.scheduled_volume_struc;
   r2 = length($tws.local_saved_volume_struc);
   r1 = &$tws.local_saved_volume_struc;
   call $tws.copy_array;

   r0  = M[$tws.local_saved_volume_struc +2];
   M[$tws.local_mute_volume_struc +2] = r0;
   r3 = &$tws.local_mute_volume_struc;
   call $M.music_example_message.MainVolume.tws_func;

schedule_routing:

   r0 = M[$tws.scheduled_routing_mode];
   M[$tws.routing_mode] = r0;

    // pop rLink from stack
    jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.tws.tws.feed_dac_buffer
//
// DESCRIPTION:
//    This function is meant to be called whenever TWS starts streaming.  It
//    puts a frame of zeros into the DAC cbuffer to prevent av_dac_copy from
//    inserting zeroes in between TWS computing its playback time and the
//    output copy handler running.
//
// INPUTS:
//    - none
//
// OUTPUTS:
//    - none
//
// *****************************************************************************
.MODULE $M.tws.feed_dac_buffer;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   .VAR offset;

   $tws.feed_dac_buffer:

   $push_rLink_macro;

   call $block_interrupts;
   r0 = $multi_chan_primary_left_out_cbuffer_struc;
   call $cbuffer.calc_amount_data;
   Null = r0 - M[$music_example.frame_processing_size];
   if POS jump done;  // no need to insert

   r1 = M[$tws.insertion_counter];
   r1 = r1 + 1;
   M[$tws.insertion_counter] = r1;

   M[offset] = Null;
   I4 = &$M.multi_chan_output.wired_in_buffer_table;

next_buffer:
   r0 = M[I4,0];
   call $cbuffer.get_write_address_and_size;
   I0 = r0;
   L0 = r1;

   r1 = 0;
   r10 = M[$music_example.frame_processing_size];
   do insert_frame_of_zeros;
      M[I0,1] = r1;
   insert_frame_of_zeros:

   r0 = M[I4,1];
   r1 = I0;
   call $cbuffer.set_write_address;

   r0 = M[offset];
   r0 = r0 + 1;
   M[offset] = r0;
   Null = r0 - $MULTI_CHAN_MAX_WIRED_OUT_CHANNELS;
   if NEG jump next_buffer;

   L0 = Null;


   M[$tws.started_streaming] = Null;

   done:
   call $unblock_interrupts;
   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.tws.roleswitch_detect
//
// DESCRIPTION:
//
//    This function needs to be called from the wall clock handler, which runs
//    every 100 ms. It uses the wallclock adjust to determine the BT link master
//    or slave role for each device and record any apparent role changes.
//
// INPUTS:
//   r2 = wall clock adjustment
//
// OUTPUTS:
//    - none
//
// *****************************************************************************
.MODULE $M.tws.roleswitch_detect;
   .CODESEGMENT   PM;
   .DATASEGMENT   DM;

$tws.roleswitch_detect:
   .VAR last_timer_time;
   .VAR last_adjustment;
   .VAR count;
   .VAR $curr_role = $UNKNOWN;

   $push_rLink_macro;
   r1=1;
   M[$tws.wallclock_running] = r1;

   r0 = M[$TIMER_TIME];
   r5 = M[last_adjustment];
   M[last_adjustment] = r2;
   r3 = r2 - r5; // current adjustment - last adjustment
   null = r3 - $ROLE_REVERSAL_THRESHOLD;
   if GT jump role_switch_log;   // Too large - role switch!
   null = r3 + $ROLE_REVERSAL_THRESHOLD;
   if LT jump role_switch_log;   // Too large - role switch!

   null = 0 - r3;
   if NZ jump set_slave;
   r1 = M[count];
   r1 = r1 + 1;
   null = r1 - $ROLE_TIMEOUT;
   if LT jump done;
   r2 = $MASTER;
   M[$curr_role] = r2;
   r1 = null;
   jump done;

set_slave:
   r2 = $SLAVE;
   M[$curr_role] = r2;
   r1 = null;

done:
   M[count] = r1;
   jump $pop_rLink_and_rts;

role_switch_log:
   .VAR $role_reversal_detected = 0;
   r1 = M[$role_reversal_detected];
   r1 = r1 + 1;
   M[$role_reversal_detected] = r1;

   jump done;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $tws.reset
//
// DESCRIPTION:
//    Completely clears TWS operation.
//
// INPUTS:
//    - none
//
// OUTPUTS:
//    - none
// *****************************************************************************
.MODULE $M.tws.reset;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   $tws.reset:

   $push_rLink_macro;
#ifdef TWS_WIRED_MODE_ENABLE
   call $tws.wired_mode.disable;
#endif
   call $tws.disable;

   M[$tws.wired_mode_enabled] = null;
   M[$tws.master_enabled] = null;


   M[$delay_left + $tws.delay.DELAY_FIELD] = NULL;
   M[$delay_right + $tws.delay.DELAY_FIELD] = NULL;


   M[$M.main.samples_latency_measure + 0] = null;

   // re-initialise audio processing
   call $music_example_reinitialize;

   // re-initialise delay lines
   r4 = &$M.system_config.data.delay_reinitialize_table;
   call $frame_sync.run_function_table;

   // Clear EQ delay memories
   r4 = &$M.system_config.data.filter_reset_table;
   call $frame_sync.run_function_table;

   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.tws.check_started_streaming
//
// DESCRIPTION:
//    Detects whether or not we've just started streaming.
//
// INPUTS:
//    - none
//
// OUTPUTS:
//    - none
// *****************************************************************************
.MODULE $M.tws.check_started_streaming;
   .CODESEGMENT PM;

   $tws.check_started_streaming:

   $push_rLink_macro;

   r0 = &$multi_chan_primary_left_out_cbuffer_struc;
   call $cbuffer.calc_amount_data;
   r4 = r0;

   null = r4 - 75;
   if POS jump done;

   r0 = &$audio_out_left_cbuffer_struc;
   call $cbuffer.calc_amount_data;
   null = r0 - $music_example.NUM_SAMPLES_PER_FRAME;
   if POS jump done;

   r4 = 1;
   M[$tws.started_streaming] = r4;

   r1 = M[$tws.started_streaming_counter];
   r1 = r1 + 1;
   M[$tws.started_streaming_counter] = r1;

   done:
   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.tws.calc_sra
//
// DESCRIPTION:
//    Calculates the rate mismatch between the master and slave TWS devices
//
// INPUTS:
//    - r0 = master's playback time - slave's playback time

//
// OUTPUTS:
//    - none
// *****************************************************************************
.MODULE $M.tws.calc_sra;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   $tws.calc_sra:

   $push_rLink_macro;

   .VAR $debug_alpha = 0;
   .VAR $debug_delta_t = 0;
   .VAR $debug_avg_delta = 0;
   .VAR $debug_avg_inp = 0;

   // Calculate alpha from tc, sample rate and frame size
   r1 = M[$decoder_codec_stream_struc + $codec.av_decode.DECODER_NUM_OUTPUT_SAMPLES_FIELD];
   r2 = M[$inv_codec_fs];
   rMAC = r1 * r2; // r1 = delta_t
   rMAC = rMAC ASHIFT 7;
   r1 = rMAC1;
   M[$debug_delta_t] = r1;// delta_t in microseconds
   rMAC = r1;
   r2 = M[$tws.slave_sra_tc];
   r2 = r2 + r1;

   r2 = r2 ASHIFT 1;

   div = rMAC/r2;
   r8 = DivResult;
   M[$debug_alpha] = r8;

   // averaging routine: alpha * instantaneous average + (1 - alpha) * average delta
   r3 = M[$tws.average_playback_difference];
   M[$tws.average_playback_difference_old] = r3;
   r1 = 1.0 - r8;
   rMAC = r8 * r0;
   rMAC = rMAC + r1 * r3;
   r3 = rMAC;
   M[$tws.average_playback_difference] = r3; // Proportional

   // Proportional term
   r2 = M[$tws.slave_sra_tc];
   r2 = r2 ASHIFT 3;  // Multiply time constant by 8
   rMAC = -r3;
   r2 = r2 ASHIFT 1;
   div = rMAC/r2;
   r8 = DivResult;
   M[$tws.forcing_rate] = r8;

   r1 = M[$tws.p_weighting];
   r3 = M[$tws.forcing_rate];

   r3 = r8 * r1(frac);

   M[$tws.slave_sra] = r3;

   done:
   jump $pop_rLink_and_rts;



.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.tws.zero_input_audio
//
// DESCRIPTION:
//    Clears the delay, processing, and  zeros data in the input buffers used in TWS.  This is
//    required when streaming starts to purge old 'trapped' data from the delay
//    lines.
//
// INPUTS:
//    - none
//
// OUTPUTS:
//    - none
// *****************************************************************************
.MODULE $M.tws.zero_input_audio;
   .CODESEGMENT PM;

   $tws.zero_input_audio:

   $push_rLink_macro;

   // Set mute duration for soft mute
   r6 = $TWS_QUICK_MUTE_TIME;
   M[$soft_mute_duration] = r6;
   call $tws.soft_mute_handler;

   // Set delay to zero
#ifdef TWS_ENABLE
   M[$delay_left + $tws.delay.DELAY_FIELD] = NULL;
   M[$delay_right + $tws.delay.DELAY_FIELD] = NULL;
#endif

   M[$M.main.samples_latency_measure + 0] = NULL;

   // clear input cbuffer into frame process or else old data will get played on slave
   // r3 = number of samples decoded - keep these...
   r3 = M[$decoder_codec_stream_struc + $codec.av_decode.DECODER_NUM_OUTPUT_SAMPLES_FIELD];

   //  Loop from (write pointer-r3) to read pointer
   r0 = &$audio_out_left_cbuffer_struc;
   call $cbuffer.get_write_address_and_size;
   I0 = r0;
   L0 = r1;
   M0 = -r3;
   r0 = &$audio_out_left_cbuffer_struc;
   call $cbuffer.calc_amount_data;
   r10 = r0 - r3; //set loop counter
   r0 = M[I0,M0]; // Dummy read to decrement pointer
   M0 = -1;
   do zero_left_loop;
      r0 = 0;
      M[I0,M0] = r0; // write 0
   zero_left_loop:

   //  Loop from (write pointer-r3) to read pointer
   r0 = &$audio_out_right_cbuffer_struc;
   call $cbuffer.get_write_address_and_size;
   I0 = r0;
   L0 = r1;
   M0 = -r3;
   r0 = &$audio_out_right_cbuffer_struc;
   call $cbuffer.calc_amount_data;
   r10 = r0 - r3; //set loop counter
   r0 = M[I0,M0]; // Dummy read to decrement pointer
   M0 = -1;
   do zero_right_loop;
      r0 = 0;
      M[I0,M0] = r0; // write 0
   zero_right_loop:

   L0 = 0;
   M0 = 0;

   // re-initialise audio processing
   r4 = 1;
   M[$music_example.reinit] = r4;

   // Clear EQ delay memories
   r4 = &$M.system_config.data.filter_reset_table;
   call $frame_sync.run_function_table;



   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.tws.clear_input_audio
//
// DESCRIPTION:
//    Clears the delay, processing, and  zeros data in the input buffers used in TWS.  This is
//    required when streaming starts to purge old 'trapped' data from the delay
//    lines.
//
// INPUTS:
//    - none
//
// OUTPUTS:
//    - none
// *****************************************************************************
.MODULE $M.tws.clear_input_audio;
   .CODESEGMENT PM;

   $tws.clear_input_audio:

   $push_rLink_macro;

   // Set mute duration for soft mute
   r6 = $TWS_QUICK_MUTE_TIME;
   M[$soft_mute_duration] = r6;
   call $tws.soft_mute_handler;

   // Set delay to zero
   M[$M.system_config.data.delay_left + $audio_proc.delay.DELAY_FIELD] = NULL;
   M[$M.system_config.data.delay_right + $audio_proc.delay.DELAY_FIELD] = NULL;

   M[$M.main.samples_latency_measure + 0] = NULL;

   // clear input cbuffer into frame process or else old data will get played on slave
   // r3 = number of samples decoded - keep these...
   r3 = M[$decoder_codec_stream_struc + $codec.av_decode.DECODER_NUM_OUTPUT_SAMPLES_FIELD];

   //  set read pointer to WP - r3
   r0 = &$audio_out_left_cbuffer_struc;
   call $cbuffer.get_write_address_and_size;
   I0 = r0;
   L0 = r1;
   M0 = -r3;
   r0 = &$audio_out_left_cbuffer_struc;
   r1 = M[I0,M0];  // decrement I0;
   r1 = I0;
   call $cbuffer.set_read_address;


   //  set read pointer to WP - r3
   r0 = &$audio_out_right_cbuffer_struc;
   call $cbuffer.get_write_address_and_size;
   I0 = r0;
   L0 = r1;
   M0 = -r3;
   r0 = &$audio_out_right_cbuffer_struc;
   r1 = M[I0,M0];  // decrement I0;
   r1 = I0;
   call $cbuffer.set_read_address;

   L0 = 0;
   M0 = 0;

   // re-initialise audio processing
   r4 = 1;
   M[$music_example.reinit] = r4;

   // Clear EQ delay memories
   r4 = &$M.system_config.data.filter_reset_table;
   call $frame_sync.run_function_table;

   M[$M.tws.calc_sra.last_timer_time] = null;

   jump $pop_rLink_and_rts;

.ENDMODULE;



// *****************************************************************************
// MODULE:
//    $M.tws.bin_current_frame
//
// DESCRIPTION:
//    Bins the current PCM frame of audio.  Typically used when the frame is
//    too old for the slave to play, or if the delay, processing, and input buffers used in TWS.  This is
//    required when streaming starts to purge old 'trapped' data from the delay
//    lines.
//
// INPUTS:
//    - none
//
// OUTPUTS:
//    - none
// *****************************************************************************
.MODULE $M.tws.bin_current_frame;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   $tws.bin_current_frame:

   $push_rLink_macro;


    // Remove decoded output samples from buffer
   // r3 = number of samples to remove
   r3 = M[$decoder_codec_stream_struc + $codec.av_decode.DECODER_NUM_OUTPUT_SAMPLES_FIELD];

   // back up left write pointer
   r0 = &$audio_out_left_cbuffer_struc;
   call $cbuffer.get_write_address_and_size;
   I0 = r0;
   L0 = r1;
   M0 = -r3;
   r0 = M[I0,M0]; // Dummy read to decrement pointer
   r1 = I0;
   r0 = &$audio_out_left_cbuffer_struc;
   call $cbuffer.set_write_address;

   // back up right write pointer
   r0 = &$audio_out_right_cbuffer_struc;
   call $cbuffer.get_write_address_and_size;
   I0 = r0;
   L0 = r1;
   M0 = -r3;
   r0 = M[I0,M0]; // Dummy read to decrement pointer
   r1 = I0;
   r0 = &$audio_out_right_cbuffer_struc;
   call $cbuffer.set_write_address;

   L0 = 0;
   M0 = 0;
   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.tws.zero_current_frame
//
// DESCRIPTION:
//    overwrites current audio frame with zeros. Used if a packet is very late
//    in order to avoid sudden discontinuities in latency.
// INPUTS:
//    - none
//
// OUTPUTS:
//    - none
// *****************************************************************************
.MODULE $M.tws.zero_current_frame;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   $tws.zero_current_frame:
   $push_rLink_macro;

    // zero decoded output samples from buffer
   // r3 = number of samples to zero
   r3 = M[$decoder_codec_stream_struc + $codec.av_decode.DECODER_NUM_OUTPUT_SAMPLES_FIELD];
   // back up left write pointer
   r0 = &$audio_out_left_cbuffer_struc;
   call $cbuffer.get_write_address_and_size;
   I0 = r0;
   L0 = r1;
   M0 = -r3;
   r0 = 0;
   M[I0,M0] = r0; // write 0
   // back up right write pointer
   r0 = &$audio_out_right_cbuffer_struc;
   call $cbuffer.get_write_address_and_size;
   I0 = r0;
   L0 = r1;
   M0 = -r3;
   r0 = 0;
   M[I0,M0]= r0; // Write 0
   L0 = 0;
   M0 = 0;
   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $tws.bin heasders
//
// DESCRIPTION:
//    This function fills audio_out_cbuffers with 0's to avoid startup problems
//
// INPUTS:
//    I0, L0, R5
//
// OUTPUTS:
//    I0
// *****************************************************************************
.MODULE $M.tws.bin_headers;
   .CODESEGMENT PM;
   .DATASEGMENT DM;
   $tws.bin_headers:
   $push_rLink_macro;
   call $block_interrupts;
   r0 = M[r5 + $codec.DECODER_IN_BUFFER_FIELD];
   call $cbuffer.calc_amount_data;
   null = r0 - 30;
   if NEG jump done_with_bin;
   M0 = 30;  // words in twoheaders
   r0 = M[I0,M0];
   M0 = null;
done_with_bin:
   call $unblock_interrupts;

   $pop_rLink_macro;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $tws.set_avg_fraction
//
// DESCRIPTION:
//    This function sets the SRA averaging fraction, based on if the device is
//    in TWS operation or standalone mode.
//
// INPUTS:
//    r0 - 0 if in standalone mode, 1 is in TWS.
//
// OUTPUTS:
//    -NONE
// *****************************************************************************
.MODULE $M.tws.set_avg_fraction;
   .CODESEGMENT PM;
   .DATASEGMENT DM;
   $tws.set_avg_fraction:
   $push_rLink_macro;

   // Set SRA to react faster when in TWS mode, return to default if not
   r2 = $TWS_AVG_FRACTION;
   r1 = $sra.DEFAULT_AVG_FRACTION;
   null = r0;
   if Z r2 = r1;
   M[$tag_averaging_fraction] = r2;

   jump $pop_rLink_and_rts;
.ENDMODULE;


// *****************************************************************************
// MODULE:
//    $tws.check_timestamp
//
// DESCRIPTION:
//    This function find the first TWS header in the Codec_in buffer, and compares
//    its time to play with the current time. If the difference is less than the
//    threshold, unstall the decoder.
//
// INPUTS:
//
//
// OUTPUTS:
//    -NONE
// *****************************************************************************
.MODULE $M.$tws.check_timestamp;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   .VAR $debug_data_size = 0;
   .VAR $debug_sync_found = 0;
   .VAR $debug_wclk = 0;
   .VAR $debug_TTP = 0;
   .VAR $debug_delta = 0;

   $tws.check_timestamp:

   $push_rLink_macro;
   M[$debug_sync_found] = 0;

#ifdef SBC_ENABLE
   r9 = &$sbc.sbc_common_data_array; // check this in all code!!!!!
#endif

   // load number of bytes available in bitstream
   r0 = &$codec_in_cbuffer_struc;
   call $cbuffer.calc_amount_data;
   r4 = r0 ASHIFT 1;
   M[$debug_data_size] = r4;

   // get read pointer
   r0 = &$codec_in_cbuffer_struc;
   call $cbuffer.get_read_address_and_size;
   I0 = r0;
   L0 = r1;

   // save read pointer and bitpos in case they need to be restored
   .VAR saved_read_pointer;
   M[saved_read_pointer] = r0;

   // get current bitpos, corresponding to the end of frame
   .VAR saved_bit_pos;
   r0 = M[&$relay_struc + $relay.CODEC_GET_BITPOS];
   r0 = M[r0];
   M[saved_bit_pos] = r0;



  // Search for sync header
   r3 = 0;
   findsyncloop:
      //check to see if we can get another byte from the codec input buffer
      r4 = r4 - 1;
      if LT jump check_timestamp_done;
      // read a byte
      r0 = 8;
      push r3;
      r2 = M[r5 + $relay.CODEC_GETBITS];
      call r2;
      pop r3;
      r3 = r3 LSHIFT 8;
      r3 = r3 + r1;
      // check to see if it is a sync word
      Null = r3 - 0x435352;
      if  Z jump continue1;

   jump findsyncloop;

continue1:

   r10 = 2;
   do readloop;
      //check to see if we can get another word from the codec input buffer
      r4 = r4 - 3;
      if NEG jump check_timestamp_done;

      // read a word
      r0 = 8;
      push r3;
      r2 = M[&$relay_struc  + $relay.CODEC_GETBITS];
      call r2;
      pop r3;
      r3 = r3 LSHIFT 8;
      r3 = r3 + r1;

      push r3;
      r2 = M[&$relay_struc  + $relay.CODEC_GETBITS];
      call r2;
      pop r3;
      r3 = r3 LSHIFT 8;
      r3 = r3 + r1;

      push r3;
      r2 = M[&$relay_struc  + $relay.CODEC_GETBITS];
      call r2;
      pop r3;
      r3 = r3 LSHIFT 8;
      r3 = r3 + r1;

   readloop:


   // TimeToPlay in r3...
   // compare to wallclock + threshold, if LT, unstall decoder
   r1 = $tws.wallclock_obj;
   call $wall_clock.get_time;
   M[$debug_wclk] = r1;
   M[$debug_TTP] = r3;
   r1 = r3 - r1;
   M[$debug_delta] = r1;



   r1 = M[$debug_wclk];
   r0 = M[$tws.Slave_GoodWorking_Time];
   r1 = r1 +r0;
   r0 = r3 - r1;
   if GT jump check_timestamp_done;

   // Unstall decoder
   r1 = M[$decoder_codec_stream_struc  +$codec.av_decode.CURRENT_RUNNING_MODE_FIELD];
   if NZ jump check_timestamp_done;
   r1 = 1;
   M[$decoder_codec_stream_struc + $codec.av_decode.CURRENT_RUNNING_MODE_FIELD] = r1;




check_timestamp_done:
   // restore pointers
   r0 = M[saved_bit_pos];
   r1 = M[$relay_struc + $relay.CODEC_GET_BITPOS];
   M[r1] = r0;
   r5 = &$relay_struc;
   I0 = 0;
   L0 = 0;

   jump $pop_rLink_and_rts;
.ENDMODULE;


//*****************************************************************************
// MODULE:
//    $tws.syncdelay_compat_message_handler
//
// DESCRIPTION:
//    If the VM determines that an older TWS device is connected as a slave,
//    force the master device to run in compatibility mode, that is use a
//    smaller synchronization delay
//
// INPUTS:
//    r1 - Compatibility Mode Flag:
//             0 = regular operation
//             1 = compatibility mode
//
// OUTPUTS:
//    - none
// *****************************************************************************
.MODULE $M.tws.syncdelay_compat_message_handler;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   $tws.syncdelay_compat_message_handler:
   $push_rLink_macro;
   push r0;
   push r2;
   r2 = $TWS_COMPAT_DELAY;
   r0 = $TWS_ALIGNMENT_DELAY;
   null = r1;
   if NZ  r0 = r2;
   M[$tws.alignment_delay] = r0;
   pop r2;
   pop r0;
   jump $pop_rLink_and_rts;

.ENDMODULE;
