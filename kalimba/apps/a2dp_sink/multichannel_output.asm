// *****************************************************************************
// Copyright (c) 2003 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
//
// *****************************************************************************

// *****************************************************************************
// DESCRIPTION
//    Multi-channel output routines
//
// *****************************************************************************

#include "core_library.h"
#include "cbops_library.h"
#include "music_example.h"

#if defined(APTX_ACL_SPRINT_ENABLE)
#include "codec_decoder_aptx_acl_sprint.h"
#include "sr_adjustment_gaming.h"
#elseif defined (FASTSTREAM_ENABLE)
#include "codec_decoder_faststream.h"
#include "sr_adjustment_gaming.h"
#else
#include "codec_decoder.h"
#include "sr_adjustment.h"
#endif

#include "spdif.h"
#include "spdif_sink.h"
#include "spdif_frame_copy.h"
#include "multichannel_output.h"
#include "multichannel_output_macros.h"
#include "frame_sync_stream_macros.h"

.MODULE $M.multi_chan_output;
   .DATASEGMENT DM;

   // Multi-channel CHAIN0 output cbops copy struc (dynamically constructed)
   .VAR chain0_copy_struc[$MULTI_CHAN_CHAIN0_MAX_WIRED_CHANNELS + $MULTI_CHAN_CHAIN0_MAX_WIRED_CHANNELS + 3 + $cbops.AV_COPY_M_EXTEND_SIZE] =
      $chain0_processing_switch_op,                         // First operator block
      0 ...;                                                // Zero remaining struc elements (these are dynamically configured)

   // Multi-channel CHAIN1 output cbops copy struc (dynamically constructed)
   .VAR chain1_copy_struc[$MULTI_CHAN_CHAIN1_MAX_WIRED_CHANNELS + $MULTI_CHAN_CHAIN1_MAX_WIRED_CHANNELS + 3 + $cbops.AV_COPY_M_EXTEND_SIZE] =
      $chain1_processing_switch_op,                         // First operator block
      0 ...;                                                // Zero remaining struc elements (these are dynamically configured)

   // Create output cbuffers
   DeclareCBuffer($multi_chan_primary_left_out_cbuffer_struc,$multi_chan_primary_left_out,$OUTPUT_AUDIO_CBUFFER_SIZE);
   DeclareCBuffer($multi_chan_primary_right_out_cbuffer_struc,$multi_chan_primary_right_out,$OUTPUT_AUDIO_CBUFFER_SIZE);
#if defined(MULTI_CHANNEL_DISABLE)
   // Use minimal buffer sizes for secondary and aux channels when multi-channel is disabled
   DeclareCBuffer($multi_chan_secondary_left_out_cbuffer_struc,$multi_chan_secondary_left_out,1);
   DeclareCBuffer($multi_chan_secondary_right_out_cbuffer_struc,$multi_chan_secondary_right_out,1);
   DeclareCBuffer($multi_chan_aux_left_out_cbuffer_struc,$multi_chan_aux_left_out,1);
   DeclareCBuffer($multi_chan_aux_right_out_cbuffer_struc,$multi_chan_aux_right_out,1);
#else
   // Default is multi-channel
   DeclareCBuffer($multi_chan_secondary_left_out_cbuffer_struc,$multi_chan_secondary_left_out,$OUTPUT_AUDIO_CBUFFER_SIZE);
   DeclareCBuffer($multi_chan_secondary_right_out_cbuffer_struc,$multi_chan_secondary_right_out,$OUTPUT_AUDIO_CBUFFER_SIZE);
   DeclareCBuffer($multi_chan_aux_left_out_cbuffer_struc,$multi_chan_aux_left_out,$OUTPUT_AUDIO_CBUFFER_SIZE);
   DeclareCBuffer($multi_chan_aux_right_out_cbuffer_struc,$multi_chan_aux_right_out,$OUTPUT_AUDIO_CBUFFER_SIZE);
#endif
   DeclareCBuffer($multi_chan_sub_out_cbuffer_struc,$multi_chan_sub_out,1800);

   // Table of pointers to all possible cbuffer inputs for the output handler
   // (a subset of which will be routed to the CHAIN0/CHAIN1 wired outputs)
   // Keep the primary and aux in the first four table entries since these
   // can be used for tone mixing.
   .VAR wired_in_buffer_table[$MULTI_CHAN_MAX_WIRED_OUT_CHANNELS] =
      $multi_chan_primary_left_out_cbuffer_struc,
      $multi_chan_primary_right_out_cbuffer_struc,
      $multi_chan_aux_left_out_cbuffer_struc,
      $multi_chan_aux_right_out_cbuffer_struc,
      $multi_chan_secondary_left_out_cbuffer_struc,
      $multi_chan_secondary_right_out_cbuffer_struc,
      $multi_chan_sub_out_cbuffer_struc;

#ifdef SRA_TARGET_LATENCY
   .VAR wired_in_sil_buffer_table[$MULTI_CHAN_MAX_WIRED_OUT_CHANNELS] =
      $multi_chan_sil_primary_left_out_cbuffer_struc,
      $multi_chan_sil_primary_right_out_cbuffer_struc,
      $multi_chan_sil_aux_left_out_cbuffer_struc,
      $multi_chan_sil_aux_right_out_cbuffer_struc,
      $multi_chan_sil_secondary_left_out_cbuffer_struc,
      $multi_chan_sil_secondary_right_out_cbuffer_struc,
      $multi_chan_sil_sub_out_cbuffer_struc;
#endif

   // Table of ports for all possible 16bit resolution mode wired outputs
   .VAR wired_out_port_table[$MULTI_CHAN_MAX_WIRED_OUT_CHANNELS] =
      $MULTI_CHAN_PRIMARY_LEFT_OUT_PORT,
      $MULTI_CHAN_PRIMARY_RIGHT_OUT_PORT,
      $MULTI_CHAN_AUX_LEFT_OUT_PORT,
      $MULTI_CHAN_AUX_RIGHT_OUT_PORT,
      $MULTI_CHAN_SECONDARY_LEFT_OUT_PORT,
      $MULTI_CHAN_SECONDARY_RIGHT_OUT_PORT,
      $MULTI_CHAN_SUB_WIRED_OUT_PORT;

   // Table of ports for all possible 24bit resolution mode wired outputs
   .VAR wired_24_bit_out_port_table[$MULTI_CHAN_MAX_WIRED_OUT_CHANNELS] =
      $MULTI_CHAN_PRIMARY_24_BIT_LEFT_OUT_PORT,
      $MULTI_CHAN_PRIMARY_24_BIT_RIGHT_OUT_PORT,
      $MULTI_CHAN_AUX_24_BIT_LEFT_OUT_PORT,
      $MULTI_CHAN_AUX_24_BIT_RIGHT_OUT_PORT,
      $MULTI_CHAN_SECONDARY_24_BIT_LEFT_OUT_PORT,
      $MULTI_CHAN_SECONDARY_24_BIT_RIGHT_OUT_PORT,
      $MULTI_CHAN_SUB_WIRED_24_BIT_OUT_PORT;

   // Table of device types for the wired outputs
   .VAR wired_out_type_table[$MULTI_CHAN_MAX_WIRED_OUT_CHANNELS] = 0 ...;

   // Use a common structure for the signal detection
   .VAR signal_detect_coeffs[$cbops.signal_detect_op_coef.STRUC_SIZE] =
       SIGNAL_DETECT_THRESHOLD,                    // detection threshold
       SIGNAL_DETECT_TIMEOUT,                      // trigger time
       0,                                          // current max value
       0,                                          // signal detected timeout timer
       1,                                          // signal status (1 = playing audio)
       $music_example.VMMSG.SIGNAL_DETECT_STATUS;  // ID for VM status message

   // Number of channels on each cbops chain
   // (derived from ports that are enabled and VM message specifying channels that are connected to DACs)
   .VAR num_chain0_channels;
   .VAR num_chain1_channels;

   // Bit field flags used to switch cbops operators (bit0: channels 0, bit1: channel 1 etc)
   .VAR chain0_enables;
   .VAR chain1_enables;

   // Enable control flag for ANC
   .VAR chain0_anc_resampler_enable = $ANC_NONE;

   // Disable flag for DC remove operators
   // (allows dc remove bypassing for transparent output handler processing)
   .VAR dc_remove_disable;

   // Enables for cbops tone mixing
   .VAR chain0_tone_mix_en;
   .VAR chain1_tone_mix_en;

   // Mixing ratios for primary and aux channels
   .VAR prim_tone_mix_ratio;
   .VAR aux_tone_mix_ratio;

   // Flag indicating I2S master/slave operation (0: master; 1: slave)
   .VAR i2s_slave0;

   // Variables holding the output port from the port table that is enabled
   .VAR chain0_sync_port;
   .VAR chain1_sync_port;

   // Flag
   .VAR stereo_signal;

   // Copy of the tone cbuffer read pointer
   .VAR tone0_in_left_read_ptr;
   .VAR tone1_in_left_read_ptr;
   .VAR tone2_in_left_read_ptr;
   .VAR tone0_in_right_read_ptr;
   .VAR tone1_in_right_read_ptr;
   .VAR tone2_in_right_read_ptr;

   // Muting control variables
   .VAR channels_mute_en;
   .VAR chain0_mute_en; // For debug
   .VAR chain1_mute_en; // For debug

   // Lookup table used to determine the ANC output resampler given the ANC mode and the DAC rate
   // (ancMode, DAC rate, filter spec)
   .VAR filter_spec_lookup_table[] =
      //44100,   $ANC_NONE,     0,                                               // No resampling
      //48000,   $ANC_NONE,     0,                                               // No resampling
      //88200,   $ANC_NONE,     0,                                               // No resampling
      //96000,   $ANC_NONE,     0,                                               // No resampling

      44100,   $ANC_96K,      $M.iir_resamplev2.Up_320_Down_147.filter,          // 44100->96000
      48000,   $ANC_96K,      $M.iir_resamplev2.Up_2_Down_1.filter,              // 48000->96000
      88200,   $ANC_96K,      $M.iir_resamplev2.Up_160_Down_147.filter,          // 88200->96000
      //96000,   $ANC_96K,      0,                                               // No resampling

      44100,   $ANC_192K,     $M.iir_resamplev2.Up_640_Down_147.filter,          // 44100->192000
      48000,   $ANC_192K,     $M.iir_resamplev2.Up_4_Down_1.filter,              // 48000->192000
      88200,   $ANC_192K,     $M.iir_resamplev2.Up_320_Down_147.filter,          // 88200->192000
      96000,   $ANC_192K,     $M.iir_resamplev2.Up_2_Down_1.filter,              // 96000->192000
      0;

   // Structure to measure output port consuming rate for chain1
   .VAR $calc_chain1_actual_port_rate_struc[$calc_actual_port_rate.STRUC_SIZE] =
      DYNAMIC, // PORT_FIELD
      0 ...;

   // structure for rate-match calculation to synchronise chain1 to chain0
   .VAR $chain1_to_chain0_pcm_sync_struct[$pcm_sync.STRUC_SIZE] =
      0.01,                                      //MAX_RATE_FIELD
      5000,                                      //CALC_PERIOD_FIELD
      chain1_pcm_latency_input_struct,           //ADJ_CHANNEL_PCM_LATENCY_STRUCT_FIELD
      chain0_pcm_latency_input_struct,           //REF_CHANNEL_PCM_LATENCY_STRUCT_FIELD
      $calc_chain1_actual_port_rate_struc + $calc_actual_port_rate.SAMPLE_RATE_FIELD, //ADJ_CHANNEL_SAMPLE_RATE_PTR_FIELD
      $calc_chain0_actual_port_rate_struc + $calc_actual_port_rate.SAMPLE_RATE_FIELD, //REF_CHANNEL_SAMPLE_RATE_PTR_FIELD
      0,                                                                              //TARGET_LATENCY_US_FIELD
      0 ...;


   // switch variables showing whether a chain
   // shall use hw rate matching. At most
   // one chain should be enabled,and it should
   // be the chain that uses "only" DACs at the
   // output
   .VAR $chain0_hw_warp_enable;
   .VAR $chain1_hw_warp_enable;

   // hardware warp structure
   .VAR $hw_warp_struct[$hw_warp.STRUC_SIZE] =
       32000,
    #if defined(SPDIF_ENABLE)
       $spdif_sra_struct + $spdif_sra.SRA_RATE_FIELD,
    #else
       $sra_struct + $sra.SRA_RATE_FIELD,
    #endif
       128,
       0 ...;
   // ----------------------------------------------
   // Setting the chain specific latency structures
   // These latency structures must represent the
   // state of the chain output handlers. For this
   // case, the chains starts from where they split
   // from main input (currently MM output).
   // Note: if you add further buffering/processing
   // to the chains, make sure you update these latency
   // structures, otherwise the chain synchronisation
   // might not work properly
   //
   //                |<- chain 0  ->|
   //          +----+
   //          |    |--->buf--->port
   // input ---| MM |
   //          |    |--->buf--->port
   //          +----+
   //                |<- chain 1  ->|
   //-----------------------------------------------

   // latency structure for chain 1
   // needs to be configured when the chain is built
   //
   .VAR chain1_warp_ptr = chain1_sync_rate_adjustment_and_shift.param + $cbops.rate_adjustment_and_shift.SRA_CURRENT_RATE_FIELD;
   .VAR chain1_pcm_cbuffers_latency_measure[] =
       DYNAMIC, &$inv_dac_fs, chain1_warp_ptr,  // cbuffer
       DYNAMIC, &$inv_dac_fs, 0,                // port
       0;

   .VAR chain1_pcm_latency_input_struct[$pcm_latency.STRUC_SIZE] =
      &chain1_pcm_cbuffers_latency_measure,
      0;

   // latency structure for chain 0
   // needs to be configured when the chain is built
   .VAR chain0_pcm_cbuffers_latency_measure[] =
       DYNAMIC, &$inv_dac_fs, 0,           // cbuffer
       DYNAMIC, &$inv_dac_fs, 0,           // port
       0;

   .VAR chain0_pcm_latency_input_struct[$pcm_latency.STRUC_SIZE] =
      chain0_pcm_cbuffers_latency_measure,
      0;

   // Structure to control channel allocation
   .VAR $interface_map_struc[$INTERFACE_MAP_SIZE]=
      0,       // Bitmask for wired output channels:                       bit[n]: 0: disabled, 1: enabled
      0,       // DAC channel mask (sets which channels are on DACs)       bit[n]: 0: Digital, 1: DAC
      0,       // Bitmask for selecting wired output channels chain:       bit[n]: 0: CHAIN0, 1: CHAIN1
      0;       // S/PDIF channel mask (sets which channels are on S/PDIF)  bit[n]: 0: Other, 1: S/PDIF

   // Output handler timer period for 16bit output resolution
   .VAR handler_period_table_16bit[$OUTPUT_HANDLER_PERIOD_TABLE_SIZE] =
      TMR_PERIOD_AUDIO_COPY,              // output rate <= 48kHz
      TMR_PERIOD_HI_RATE_AUDIO_COPY,      // output rate > 48kHz
      TMR_PERIOD_HI_RATE_AUDIO_COPY,      // ANC @96kHz
      ANC_192K_TMR_PERIOD_AUDIO_COPY;     // ANC @192kHz

   // Output handler timer period for 24bit output resolution
   .VAR handler_period_table_24bit[$OUTPUT_HANDLER_PERIOD_TABLE_SIZE] =
      TMR_PERIOD_HI_RATE_AUDIO_COPY,      // output rate <= 48kHz
      TMR_PERIOD_HI_RATE_AUDIO_COPY,      // output rate > 48kHz
      TMR_PERIOD_HI_RATE_AUDIO_COPY,      // ANC @96kHz
      ANC_192K_TMR_PERIOD_AUDIO_COPY;     // ANC @192kHz

   // Output cbops chain (build time construction)
   // ************************************************

   // All the following channel cbops processing is done in-place except for the dither and shift
   // that does the copy from input buffer to output port.

   // Any CHAIN0 processing? - bypass if none
   TEMPLATE_SWITCH_ALT_OP(chain0_processing_switch_op, chain0_ch0_tone_switch_op, cbops.NO_MORE_OPERATORS, chain0_enables, $MULTI_CHAN_CHAIN0_CHAN0_EN)

   // Perform tone insertion for channel 0
   TEMPLATE_SWITCH_ALT_OP(chain0_ch0_tone_switch_op, chain0_ch0_mix_op, chain0_ch1_tone_switch_op, chain0_tone_mix_en, $MULTI_CHAN_CHAIN0_CHAN0_EN)
   TEMPLATE_MIX_OP(chain0_ch0_mix_op, chain0_ch1_tone_switch_op, $MULTI_CHAN_CHAIN0_CHAN0, $tone0_in_left_resample_cbuffer_struc)

   // Perform tone insertion for channel 1
   TEMPLATE_SWITCH_ALT_OP(chain0_ch1_tone_switch_op, chain0_ch1_mix_op, chain0_ch2_tone_switch_op, chain0_tone_mix_en, $MULTI_CHAN_CHAIN0_CHAN1_EN)
   TEMPLATE_MIX_OP(chain0_ch1_mix_op, chain0_ch2_tone_switch_op, $MULTI_CHAN_CHAIN0_CHAN1, $tone0_in_right_resample_cbuffer_struc)

   // Perform tone insertion for channel 2
   TEMPLATE_SWITCH_ALT_OP(chain0_ch2_tone_switch_op, chain0_ch2_mix_op, chain0_ch3_tone_switch_op, chain0_tone_mix_en, $MULTI_CHAN_CHAIN0_CHAN2_EN)
   TEMPLATE_MIX_OP(chain0_ch2_mix_op, chain0_ch3_tone_switch_op, $MULTI_CHAN_CHAIN0_CHAN2, $tone1_in_left_resample_cbuffer_struc)

   // Perform tone insertion for channel 3
   TEMPLATE_SWITCH_ALT_OP(chain0_ch3_tone_switch_op, chain0_ch3_mix_op, chain0_ch0_5_signal_detect_op, chain0_tone_mix_en, $MULTI_CHAN_CHAIN0_CHAN3_EN)
   TEMPLATE_MIX_OP(chain0_ch3_mix_op, chain0_ch0_5_signal_detect_op, $MULTI_CHAN_CHAIN0_CHAN3, $tone1_in_right_resample_cbuffer_struc)

   // Channel 0-5 signal detection
   TEMPLATE_SIGNAL_DETECT_OP(chain0_ch0_5_signal_detect_op, chain0_ch0_switch_op, DYNAMIC, 0, 1, 2, 3, 4, 5)

   // Channel 0
   TEMPLATE_SWITCH_ALT_OP(chain0_ch0_switch_op, chain0_ch0_mute_op, chain0_ch1_switch_op, chain0_enables, $MULTI_CHAN_CHAIN0_CHAN0_EN)
   TEMPLATE_MUTE_OP(chain0_ch0_mute_op, chain0_ch0_dc_remove_switch_op, DYNAMIC, 0, 0)
   TEMPLATE_SWITCH_ALT_OP(chain0_ch0_dc_remove_switch_op, chain0_ch1_switch_op, chain0_ch0_dc_remove_op, dc_remove_disable, 1)
   TEMPLATE_DC_REMOVE_OP(chain0_ch0_dc_remove_op, chain0_ch1_switch_op, 0, 0)

   // Channel 1
   TEMPLATE_SWITCH_ALT_OP(chain0_ch1_switch_op, chain0_ch1_mute_op, chain0_anc_switch_op, chain0_enables, $MULTI_CHAN_CHAIN0_CHAN1_EN)
   TEMPLATE_MUTE_OP(chain0_ch1_mute_op, chain0_ch1_dc_remove_switch_op, DYNAMIC, 1, 1)
   TEMPLATE_SWITCH_ALT_OP(chain0_ch1_dc_remove_switch_op, chain0_anc_switch_op, chain0_ch1_dc_remove_op, dc_remove_disable, 1)
   TEMPLATE_DC_REMOVE_OP(chain0_ch1_dc_remove_op, chain0_anc_switch_op, 1, 1)

   // Select standard operation or ANC operation
   TEMPLATE_SWITCH_ALT_OP(chain0_anc_switch_op, chain0_ch0_resamp_switch_op, chain0_ch0_dither_switch_op, chain0_anc_resampler_enable, $ANC_MASK)

   // ANC support on channel 0 and channel 1 (ONLY!)
   TEMPLATE_SWITCH_ALT_OP(chain0_ch0_resamp_switch_op, chain0_ch0_resamp_op, chain0_ch1_resamp_switch_op, chain0_enables, $MULTI_CHAN_CHAIN0_CHAN0_EN)
   TEMPLATE_IIR_RESAMPLEV2_OP(chain0_ch0_resamp_op, chain0_ch1_resamp_switch_op, 0, DYNAMIC, DYNAMIC, DYNAMIC, 0, DYNAMIC)
   TEMPLATE_SWITCH_ALT_OP(chain0_ch1_resamp_switch_op, chain0_ch1_resamp_op, cbops.NO_MORE_OPERATORS, chain0_enables, $MULTI_CHAN_CHAIN0_CHAN1_EN)
   TEMPLATE_IIR_RESAMPLEV2_OP(chain0_ch1_resamp_op, cbops.NO_MORE_OPERATORS, 1, DYNAMIC, DYNAMIC, DYNAMIC, 0, DYNAMIC)

   // Standard dither operation for channel 0 and channel 1
   TEMPLATE_SWITCH_ALT_OP(chain0_ch0_dither_switch_op, chain0_ch0_dither_and_shift_op, chain0_ch1_dither_switch_op, chain0_enables, $MULTI_CHAN_CHAIN0_CHAN0_EN)
   TEMPLATE_DITHER_AND_SHIFT_OP(chain0_ch0_dither_and_shift_op, chain0_ch1_dither_switch_op, 0, DYNAMIC, DYNAMIC, DYNAMIC)
   TEMPLATE_SWITCH_ALT_OP(chain0_ch1_dither_switch_op, chain0_ch1_dither_and_shift_op, chain0_ch2_switch_op, chain0_enables, $MULTI_CHAN_CHAIN0_CHAN1_EN)
   TEMPLATE_DITHER_AND_SHIFT_OP(chain0_ch1_dither_and_shift_op, chain0_ch2_switch_op, 1, DYNAMIC, DYNAMIC, DYNAMIC)

   // Channel 2
   TEMPLATE_SWITCH_ALT_OP(chain0_ch2_switch_op, chain0_ch2_mute_op, chain0_ch3_switch_op, chain0_enables, $MULTI_CHAN_CHAIN0_CHAN2_EN)
   TEMPLATE_MUTE_OP(chain0_ch2_mute_op, chain0_ch2_dc_remove_switch_op, DYNAMIC, 2, 2)
   TEMPLATE_SWITCH_ALT_OP(chain0_ch2_dc_remove_switch_op, chain0_ch2_dither_and_shift_op, chain0_ch2_dc_remove_op, dc_remove_disable, 1)
   TEMPLATE_DC_REMOVE_OP(chain0_ch2_dc_remove_op, chain0_ch2_dither_and_shift_op, 2, 2)
   TEMPLATE_DITHER_AND_SHIFT_OP(chain0_ch2_dither_and_shift_op, chain0_ch3_switch_op, 2, DYNAMIC, DYNAMIC, DYNAMIC)

   // Channel 3
   TEMPLATE_SWITCH_ALT_OP(chain0_ch3_switch_op, chain0_ch3_mute_op, chain0_ch4_switch_op, chain0_enables, $MULTI_CHAN_CHAIN0_CHAN3_EN)
   TEMPLATE_MUTE_OP(chain0_ch3_mute_op, chain0_ch3_dc_remove_switch_op, DYNAMIC, 3, 3)
   TEMPLATE_SWITCH_ALT_OP(chain0_ch3_dc_remove_switch_op, chain0_ch3_dither_and_shift_op, chain0_ch3_dc_remove_op, dc_remove_disable, 1)
   TEMPLATE_DC_REMOVE_OP(chain0_ch3_dc_remove_op, chain0_ch3_dither_and_shift_op, 3, 3)
   TEMPLATE_DITHER_AND_SHIFT_OP(chain0_ch3_dither_and_shift_op, chain0_ch4_switch_op, 3, DYNAMIC, DYNAMIC, DYNAMIC)

   // Channel 4
   TEMPLATE_SWITCH_ALT_OP(chain0_ch4_switch_op, chain0_ch4_mute_op, chain0_ch5_switch_op, chain0_enables, $MULTI_CHAN_CHAIN0_CHAN4_EN)
   TEMPLATE_MUTE_OP(chain0_ch4_mute_op, chain0_ch4_dc_remove_switch_op, DYNAMIC, 4, 4)
   TEMPLATE_SWITCH_ALT_OP(chain0_ch4_dc_remove_switch_op, chain0_ch4_dither_and_shift_op, chain0_ch4_dc_remove_op, dc_remove_disable, 1)
   TEMPLATE_DC_REMOVE_OP(chain0_ch4_dc_remove_op, chain0_ch4_dither_and_shift_op, 4, 4)
   TEMPLATE_DITHER_AND_SHIFT_OP(chain0_ch4_dither_and_shift_op, chain0_ch5_switch_op, 4, DYNAMIC, DYNAMIC, DYNAMIC)

   // Channel 5
   TEMPLATE_SWITCH_ALT_OP(chain0_ch5_switch_op, chain0_ch5_mute_op, cbops.NO_MORE_OPERATORS, chain0_enables, $MULTI_CHAN_CHAIN0_CHAN5_EN)
   TEMPLATE_MUTE_OP(chain0_ch5_mute_op, chain0_ch5_dc_remove_switch_op, DYNAMIC, 5, 5)
   TEMPLATE_SWITCH_ALT_OP(chain0_ch5_dc_remove_switch_op, chain0_ch5_dither_and_shift_op, chain0_ch5_dc_remove_op, dc_remove_disable, 1)
   TEMPLATE_DC_REMOVE_OP(chain0_ch5_dc_remove_op, chain0_ch5_dither_and_shift_op, 5, 5)
   TEMPLATE_DITHER_AND_SHIFT_OP(chain0_ch5_dither_and_shift_op, cbops.NO_MORE_OPERATORS, 5, DYNAMIC, DYNAMIC, DYNAMIC)

   // CHAIN1 output cbops chain (build time construction)
   // ************************************************

   // All the following channel cbops processing is done in-place except for the dither and shift
   // that does the copy from input buffer to output port.

   // Any CHAIN1 processing? - bypass if none
   TEMPLATE_SWITCH_ALT_OP(chain1_processing_switch_op, chain1_ch0_tone_switch_op, cbops.NO_MORE_OPERATORS, chain1_enables, $MULTI_CHAN_CHAIN1_CHAN0_EN)

   // Perform tone insertion for channel 0
   TEMPLATE_SWITCH_ALT_OP(chain1_ch0_tone_switch_op, chain1_ch0_mix_op, chain1_ch1_tone_switch_op, chain1_tone_mix_en, $MULTI_CHAN_CHAIN1_CHAN0_EN)
   TEMPLATE_MIX_OP(chain1_ch0_mix_op, chain1_ch1_tone_switch_op, $MULTI_CHAN_CHAIN1_CHAN0, $tone2_in_left_resample_cbuffer_struc)

   // Perform tone insertion for channel 1
   TEMPLATE_SWITCH_ALT_OP(chain1_ch1_tone_switch_op, chain1_ch1_mix_op, chain1_ch0_1_signal_detect_op, chain1_tone_mix_en, $MULTI_CHAN_CHAIN1_CHAN1_EN)
   TEMPLATE_MIX_OP(chain1_ch1_mix_op, chain1_ch0_1_signal_detect_op, $MULTI_CHAN_CHAIN1_CHAN1, $tone2_in_right_resample_cbuffer_struc)

   // Channel 0-1 signal detection
   TEMPLATE_SIGNAL_DETECT_OP(chain1_ch0_1_signal_detect_op, chain1_ch0_mute_op, DYNAMIC, 0, 1, 2, 3, 4, 5)

   // Channel 0
   TEMPLATE_MUTE_OP(chain1_ch0_mute_op, chain1_ch0_dc_remove_switch_op, DYNAMIC, 0, 0)
   TEMPLATE_SWITCH_ALT_OP(chain1_ch0_dc_remove_switch_op, chain1_ch1_switch_op, chain1_ch0_dc_remove_op, dc_remove_disable, 1)
   TEMPLATE_DC_REMOVE_OP(chain1_ch0_dc_remove_op, chain1_ch1_switch_op, 0, 0)

   // Channel 1
   TEMPLATE_SWITCH_ALT_OP(chain1_ch1_switch_op, chain1_ch1_mute_op, chain1_rm_type_switch_op, chain1_enables, $MULTI_CHAN_CHAIN1_CHAN1_EN)
   TEMPLATE_MUTE_OP(chain1_ch1_mute_op, chain1_ch1_dc_remove_switch_op, DYNAMIC, 1, 1)
   TEMPLATE_SWITCH_ALT_OP(chain1_ch1_dc_remove_switch_op, chain1_rm_type_switch_op, chain1_ch1_dc_remove_op, dc_remove_disable, 1)
   TEMPLATE_DC_REMOVE_OP(chain1_ch1_dc_remove_op, chain1_rm_type_switch_op, 1, 1)

   // decides on rate matching type
   TEMPLATE_SWITCH_ALT_OP(chain1_rm_type_switch_op, chain1_ch0_dither_and_shift_op, chain1_sync_rate_adjustment_and_shift, $chain1_hw_warp_enable, 0)

   // Channel 0 and 1
   TEMPLATE_RATE_ADJUSTMENT_AND_SHIFT_OP(chain1_sync_rate_adjustment_and_shift, cbops.NO_MORE_OPERATORS, 0, DYNAMIC, DYNAMIC, DYNAMIC, -8, $sra_coeffs, $chain1_to_chain0_pcm_sync_struct + $pcm_sync.SRA_RATE_FIELD)

   // Channel 0
   TEMPLATE_DITHER_AND_SHIFT_OP(chain1_ch0_dither_and_shift_op, chain1_ch1_switch2_op, 0, DYNAMIC, DYNAMIC, DYNAMIC)
   TEMPLATE_SWITCH_ALT_OP(chain1_ch1_switch2_op, chain1_ch1_dither_and_shift_op, cbops.NO_MORE_OPERATORS, chain1_enables, $MULTI_CHAN_CHAIN1_CHAN1_EN)

   // Channel 1
   TEMPLATE_DITHER_AND_SHIFT_OP(chain1_ch1_dither_and_shift_op, cbops.NO_MORE_OPERATORS, 1, 3, DYNAMIC, DYNAMIC)

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $multi_chan_config_cbops_copy_strucs
//
// DESCRIPTION:
//    Function to configure the multi-channel cbops structures
//    (this allows the configuration of the cbops chains at run-time)
//
// INPUTS:
//    none
//
// OUTPUTS:
//    none
//
// TRASHED:
//    r0, r1, r2, r3, r4, r5, r8, r10, I2, I3, DoLoop
//
// *****************************************************************************
.MODULE $M.multi_chan_config_cbops_copy_strucs;
   .CODESEGMENT MULTI_CHAN_CONFIG_CBOPS_COPY_STRUCS_PM;

   $multi_chan_config_cbops_copy_strucs:

   // Push rLink onto stack
   $push_rLink_macro;

   // CHAIN0 output cbops chain (run time configuration)
   // ***********************************************
   null = M[$M.multi_chan_output.num_chain0_channels];
   if Z jump skip_chain0_config;

      // Enable CHAIN0 tone mixing
      r0 = M[$interface_map_struc + $INTERFACE_MAP_ENABLED_CHANNELS_FIELD];      // Get the bitfield for enabled channels
      r1 = M[$interface_map_struc + $INTERFACE_MAP_CHAIN1_CHANNELS_FIELD];       // Get the bitfield for CHAIN1 channels
      r1 = r1 XOR -1;                                                            // Invert mask to get CHAIN0 channel bitfield
      r0 = r0 AND r1;                                                            // Mask for chain0 enabled channels
      r1 = $MULTI_CHAN_PRIMARY_CHANNELS_MASK | $MULTI_CHAN_AUX_CHANNELS_MASK;    // Tone channels

      // Input: r0=Enabled channels, r1=Chain0 tone channels; Output r3=Chain0 cbops tone channels
      call $multi_chan_calc_cbops_channel_enables;                               // Calculate the cbops chain channel enables

      // Set the CHAIN0 cbops chain tone mix enables
      M[$M.multi_chan_output.chain0_tone_mix_en] = r3;

      // Configure the number of channels of detection(max 6 channels!)
      r0 = M[$M.multi_chan_output.chain0_enables];
      r0 = r0 AND ($MULTI_CHAN_CHAIN0_CHAN5_EN | $MULTI_CHAN_CHAIN0_CHAN4_EN | $MULTI_CHAN_CHAIN0_CHAN3_EN | $MULTI_CHAN_CHAIN0_CHAN2_EN | $MULTI_CHAN_CHAIN0_CHAN1_EN | $MULTI_CHAN_CHAIN0_CHAN0_EN);
      r0 = ONEBITCOUNT r0;                                                       // Number of enabled CHAIN0 channels (from chan0 - chan5)
      TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch0_5_signal_detect_op, param, $cbops.signal_detect_op.NUM_CHANNELS, r0)

      // Configure the ANC resampler filter (supports 44.1kHz or 48kHz -> 96kHz or 192kHz)

      // Point to first record in the ANC resampler lookup table
      r1 = $M.multi_chan_output.filter_spec_lookup_table;
      r2 = M[$current_dac_sampling_rate];                                           // Output rate before ANC resampling
      r3 = M[$ancMode];                                                             // ANC mode
      call $lookup_2_in_1_out;                                                      // r0 = filter spec. pointer, r3 = status
      null = r3;                                                                    // Check status
      if NZ jump skip_resampler;                                                    // Skip if ANC resampling is not selected (or rates are invalid!)

         // Select ANC output resampling
         r1 = M[$ancMode];                                                          // ANC mode
         M[$M.multi_chan_output.chain0_anc_resampler_enable] = r1;                  // Enable chain0 output resampling (0: none, 1: 96kHz, 2: 192kHz)

         // Configure the ANC output resampler
         TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch0_resamp_op, param, $iir_resamplev2.FILTER_DEFINITION_PTR_FIELD, r0)
         TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch1_resamp_op, param, $iir_resamplev2.FILTER_DEFINITION_PTR_FIELD, r0)

         // Configure the ANC resampler output indices given the number of CHAIN0 channels enabled
         r0 = M[$M.multi_chan_output.num_chain0_channels];
         TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch0_resamp_op, param, $iir_resamplev2.OUTPUT_1_START_INDEX_FIELD, r0)
         r0 = r0 + 1;
         TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch1_resamp_op, param, $iir_resamplev2.OUTPUT_1_START_INDEX_FIELD, r0)

      skip_resampler:

      // Configure the dither and shift output indices given the number of CHAIN0 channels enabled
      r0 = M[$M.multi_chan_output.num_chain0_channels];
      TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch0_dither_and_shift_op, param, $cbops.dither_and_shift.OUTPUT_START_INDEX_FIELD, r0)
      r0 = r0 + 1;
      TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch1_dither_and_shift_op, param, $cbops.dither_and_shift.OUTPUT_START_INDEX_FIELD, r0)
      r0 = r0 + 1;
      TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch2_dither_and_shift_op, param, $cbops.dither_and_shift.OUTPUT_START_INDEX_FIELD, r0)
      r0 = r0 + 1;
      TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch3_dither_and_shift_op, param, $cbops.dither_and_shift.OUTPUT_START_INDEX_FIELD, r0)
      r0 = r0 + 1;
      TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch4_dither_and_shift_op, param, $cbops.dither_and_shift.OUTPUT_START_INDEX_FIELD, r0)
      r0 = r0 + 1;
      TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch5_dither_and_shift_op, param, $cbops.dither_and_shift.OUTPUT_START_INDEX_FIELD, r0)

#if defined(LATENCY_REPORTING) || defined(SPDIF_ENABLE)
      // Configure the output port to use for latency calculations
      //    r0 = address of latency calculation table
      //    r1 = port to use for latency calculation
      r0 = $M.main.cbuffers_latency_measure;
      r1 =  M[$M.multi_chan_output.chain0_sync_port];
      call $multi_chan_set_port_for_latency_calc;
#endif // defined(LATENCY_REPORTING) || defined(SPDIF_ENABLE)

   skip_chain0_config:

   // CHAIN1 output cbops chain (run time configuration)
   // ***********************************************

   null = M[$M.multi_chan_output.num_chain1_channels];
   if Z jump skip_chain1_config;

      // Enable CHAIN1 tone mixing
      r0 = M[$interface_map_struc + $INTERFACE_MAP_ENABLED_CHANNELS_FIELD];      // Get the bitfield for enabled channels
      r1 = M[$interface_map_struc + $INTERFACE_MAP_CHAIN1_CHANNELS_FIELD];       // Get the bitfield for CHAIN1 channels
      r0 = r0 AND r1;                                                            // Mask for chain1 enabled channels
      r1 = $MULTI_CHAN_PRIMARY_CHANNELS_MASK | $MULTI_CHAN_AUX_CHANNELS_MASK;    // Tone channels

      // Input: r0=Enabled channels, r1=Chain1 tone channels; Output r3=Chain1 cbops tone channels
      call $multi_chan_calc_cbops_channel_enables;                               // Calculate the cbops chain channel enables

      // Set the CHAIN1 cbops chain tone mix enables
      M[$M.multi_chan_output.chain1_tone_mix_en] = r3;

      // Configure the number of channels of detection(max 2 channels!)
      r0 = M[$M.multi_chan_output.chain1_enables];
      r0 = r0 AND ($MULTI_CHAN_CHAIN1_CHAN1_EN | $MULTI_CHAN_CHAIN1_CHAN0_EN);
      r0 = ONEBITCOUNT r0;    // Number of enabled CHAIN1 channels (from chan0 and chan1)
      TEMPLATE_OP_CONFIG($M.multi_chan_output.chain1_ch0_1_signal_detect_op, param, $cbops.signal_detect_op.NUM_CHANNELS, r0)

      // Configure rate adjustment operator/dither for chain 1
      r2 = 1;  // OUTPUT1_START_INDEX_FIELD
      r3 = -1; // INPUT2_START_INDEX_FIELD
      r4 = -1; // OUTPUT2_START_INDEX_FIELD
      r5 = 1;  // OUTPUT_START_INDEX_FIELD
      r0 = M[$M.multi_chan_output.num_chain1_channels];
      Null = r0 - 2;
      if NEG jump conf_chai1_sra_op;
          r2 = 2; // OUTPUT1_START_INDEX_FIELD
          r3 = 1; // INPUT2_START_INDEX_FIELD
          r4 = 3; // OUTPUT2_START_INDEX_FIELD
          r5 = 2; //
      conf_chai1_sra_op:
      TEMPLATE_OP_CONFIG($M.multi_chan_output.chain1_sync_rate_adjustment_and_shift, param, $cbops.rate_adjustment_and_shift.OUTPUT1_START_INDEX_FIELD, r2)
      TEMPLATE_OP_CONFIG($M.multi_chan_output.chain1_sync_rate_adjustment_and_shift, param, $cbops.rate_adjustment_and_shift.INPUT2_START_INDEX_FIELD, r3)
      TEMPLATE_OP_CONFIG($M.multi_chan_output.chain1_sync_rate_adjustment_and_shift, param, $cbops.rate_adjustment_and_shift.OUTPUT2_START_INDEX_FIELD, r4)
      TEMPLATE_OP_CONFIG($M.multi_chan_output.chain1_ch0_dither_and_shift_op, param, $cbops.dither_and_shift.OUTPUT_START_INDEX_FIELD, r5)

      // ****** Rate adjustment quality selection for output handler not currently used ******
      // Configure the cbops rate adjustment quality
      //call $multi_chan_config_quality;

   skip_chain1_config:

   // Configure the multi-channel muting control to all channels unmuted
   r3 = 0;
   call $multi_chan_soft_mute;

   // Configure the dither type for all channels to the current dither setting
   r1 = M[$M.system_config.data.dithertype];
   call $multi_chan_config_dither_type;

   // Configure the output resampler quality according to the processing resolution mode (16/24bit)
   r1 = M[$procResolutionMode];
   call $multi_chan_config_output_resampler_quality;

   // Configure the output scaling according to the output resolution mode (16/24bit)
   r1 = M[$outputResolutionMode];
   call $multi_chan_config_output_scaling;

   // Pop rLink from stack
   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $multi_chan_calc_cbops_channel_enables
//
// DESCRIPTION:
//    Helper function to calculate the selectors for a cbops chain.
//
// INPUTS:
//    r0 = channel bitfield (bitfield for enabled channels: 0: disabled, 1: enabled)
//    r1 = channel selection bitfield
//
// OUTPUTS:
//    r3 = cbops channel selection bitfield
//
// TRASHED:
//    r2, r4, r10, DoLoop
//
// *****************************************************************************
.MODULE $M.multi_chan_calc_cbops_channel_enables;
   .CODESEGMENT MULTI_CHAN_CALC_CBOPS_CHANNEL_ENABLES_PM;

   $multi_chan_calc_cbops_channel_enables:

   // Calculate enables for cbops chain processing
   r10 = $MULTI_CHAN_MAX_WIRED_OUT_CHANNELS;
   r2 = 1;                                                                    // Cbops chain channel bitfield mask selection mask
   r3 = 0;                                                                    // Cbops channel enables bitfield
   r4 = 1;                                                                    // Channel bitfield selection mask
   do loop_over_channels;

      null = r0 AND r4;                                                       // Is the channel enabled
      if Z jump skip_en;                                                      // No - jump

         null = r1 AND r4;                                                    // Is the channel selected
         if NZ r3 = r3 OR r2;                                                 // Set bitfield enable
         r2 = r2 LSHIFT 1;                                                    // Next bitfield mask

      skip_en:

      r4 = r4 LSHIFT 1;                                                       // Next channel to check

   loop_over_channels:

   rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $multi_chan_build_all_cbops_copy_strucs
//
// DESCRIPTION:
//    Function to build a cbops copy struc based on a given input cbuffer table
//    and a corresponding output port table. The number of input and output
//    channels are assumed to be the same and equal to the number of enabled
//    output ports.
//
// INPUTS:
//    r5 = Pointer to interface_map_struc containing port connection and chain routing masks
//         ([0]: Bitmask for wired output channels:                        bit[n]: 0: disabled, 1: enabled
//          [1]: DAC channel mask (sets which channels are on DACs)        bit[n]: 0: Digital, 1: DAC
//          [2]: Bitmask for selecting wired output channels chain:        bit[n]: 0: CHAIN0, 1: CHAIN1
//          [3]: S/PDIF channel mask (sets which channels are on S/PDIF)   bit[n]: 0: Other, 1: S/PDIF
//    r7 = resolution mode flag ($RESOLUTION_MODE_16BIT or $RESOLUTION_MODE_24BIT)
//
//
// OUTPUTS:
//    none
//
// TRASHED:
//    r0, r1, r2, r3, r4, r6, r10, I2, I3, DoLoop
//
// *****************************************************************************
.MODULE $M.multi_chan_build_all_cbops_copy_strucs;
   .CODESEGMENT MULTI_CHAN_BUILD_ALL_CBOPS_COPY_STRUCS_PM;

   $multi_chan_build_all_cbops_copy_strucs:

   // Push rLink onto stack
   $push_rLink_macro;

   r4 = M[r5 + $INTERFACE_MAP_ENABLED_CHANNELS_FIELD];// r4 = Port connection bitmask
   r5 = M[r5 + $INTERFACE_MAP_CHAIN1_CHANNELS_FIELD]; // r5 = CHAIN0/CHAIN1 select bitmask
                                                      //
   r0 = r5 XOR -1;                                    // Make mask for CHAIN0 channels
   push r4;                                           // Save the enabled channels mask
   r4 = r4 AND r0;                                    // Select CHAIN0 channels

   r0 = ONEBITCOUNT r4;                               // Number of active CHAIN0 channels
   M[$M.multi_chan_output.num_chain0_channels] = r0;  //
   r0 = 1 LSHIFT r0;                                  // CHAIN0 switch channel enable flags
   r0 = r0 - 1;                                       //
   M[$M.multi_chan_output.chain0_enables] = r0;       //

   r0 = $M.multi_chan_output.chain0_copy_struc;       // Addr of the cbops copy struc to be populated
   r1 = $M.multi_chan_output.wired_in_buffer_table;   // Src addr for the wired input cbuffer table
   r6 = $M.multi_chan_output.chain0_sync_port;        // Addr. of variable holding CHAIN0 port to be used for synchronisation purposes
                                                      // input r0=cbops copy struc, r1=cbuffer table, r4=bit mask, r6=addr. of CHAIN0 sync. port
                                                      //       r7 = resolution mode flag ($RESOLUTION_MODE_16BIT or $RESOLUTION_MODE_24BIT)
   call $multi_chan_build_cbops_copy_struc;           //

#ifdef SRA_TARGET_LATENCY
   // Configure the cbops copy strucs to source the silence data for initial latency adjustment

   r0 = $M.adjust_initial_latency.chain0_sil_copy_struc; // Addr of the cbops copy struc to be populated
   r1 = $M.multi_chan_output.wired_in_sil_buffer_table;  // Src addr for the wired silence input cbuffer table
   r6 = $M.multi_chan_output.chain0_sync_port;           // Addr. of variable holding CHAIN0 port to be used for synchronisation purposes
                                                         // input r0=cbops copy struc, r1=cbuffer table, r4=bit mask, r6=addr. of CHAIN0 sync. port
                                                         //       r7 = resolution mode flag ($RESOLUTION_MODE_16BIT or $RESOLUTION_MODE_24BIT)
   call $multi_chan_build_cbops_copy_struc;              //
#endif

   pop r4;                                            // Restore the enabled channels mask
   r4 = r4 AND r5;                                    // Select CHAIN1 channels

   r0 = ONEBITCOUNT r4;                               // Number of active CHAIN1 channels
   M[$M.multi_chan_output.num_chain1_channels] = r0;  //
   r0 = 1 LSHIFT r0;                                  // CHAIN1 switch channel enable flags
   r0 = r0 - 1;                                       //
   M[$M.multi_chan_output.chain1_enables] = r0;       //

   r0 = $M.multi_chan_output.chain1_copy_struc;       // Addr of the cbops copy struc to be populated
   r1 = $M.multi_chan_output.wired_in_buffer_table;   // Src addr for the wired input cbuffer table
   r6 = $M.multi_chan_output.chain1_sync_port;        // Addr. of variable holding CHAIN1 port to be used for synchronisation purposes
                                                      // input r0=cbops copy struc, r1=cbuffer table, r4=bit mask, r6=addr. of CHAIN1 sync. port
                                                      //       r7 = resolution mode flag ($RESOLUTION_MODE_16BIT or $RESOLUTION_MODE_24BIT)
   call $multi_chan_build_cbops_copy_struc;           //

#ifdef SRA_TARGET_LATENCY
   // Configure the cbops copy strucs to source the silence data for initial latency adjustment

   r0 = $M.adjust_initial_latency.chain1_sil_copy_struc; // Addr of the cbops copy struc to be populated
   r1 = $M.multi_chan_output.wired_in_sil_buffer_table;  // Src addr for the wired silence input cbuffer table
   r6 = $M.multi_chan_output.chain1_sync_port;           // Addr. of variable holding CHAIN1 port to be used for synchronisation purposes
                                                         // input r0=cbops copy struc, r1=cbuffer table, r4=bit mask, r6=addr. of CHAIN1 sync. port
                                                         //       r7 = resolution mode flag ($RESOLUTION_MODE_16BIT or $RESOLUTION_MODE_24BIT)
   call $multi_chan_build_cbops_copy_struc;              //
#endif

   // Pop rLink from stack
   jump $pop_rLink_and_rts;

.ENDMODULE;


// *****************************************************************************
// MODULE:
//    $multi_chan_build_cbops_copy_struc
//
// DESCRIPTION:
//    Function to build a cbops copy struc based on a given input cbuffer table
//    and a corresponding output port table. The number of input and output
//    channels are assumed to be the same and equal to the number of enabled
//    output ports. The cbops copy struc can thus be dynamically configured.
//    This must be supplemented with a compatible and dynamically configured
//    cbops operator chain.
//
// INPUTS:
//    r0 = Addr of the cbops copy struc to be populated
//    r1 = Src addr for the wired input cbuffer table
//    r4 = channel bit mask (0: disabled, 1: enabled; bit0: channel0, bit1: channel 1 etc)
//    r6 = address of variable holding the port to be used for synchronisation (output)
//    r7 = resolution mode flag ($RESOLUTION_MODE_16BIT or $RESOLUTION_MODE_24BIT)
//
// OUTPUTS:
//    none
//
// TRASHED:
//    r0, r1, r2, r3, r10, I2, I3, DoLoop
//
// *****************************************************************************
.MODULE $M.multi_chan_build_cbops_copy_struc;
   .CODESEGMENT MULTI_CHAN_BUILD_CBOPS_COPY_STRUC_PM;

   $multi_chan_build_cbops_copy_struc:

   // Push rLink onto stack
   $push_rLink_macro;

   r3 = ONEBITCOUNT r4;                            // Number of active channels

   I3 = r0 + 1;                                    // Dst addr for the number of inputs
   M[I3, 1] = r3;                                  // Set the number of inputs in the cbops copy struc
                                                   // I3=Dst addr for the list of input cbuffers

   r0 = r1;                                        // Src addr for the wired input cbuffer table
   r10 = $MULTI_CHAN_MAX_WIRED_OUT_CHANNELS;       // Maximum number of wired output channels
   call $multi_chan_copy_enabled_channels;         // Copy the (enabled) input cbuffers into the cbops copy struc

   M[r6] = 0;                                      // Set the sync port value to zero (so it will be changed)

                                                   // I3=Dst addr for the number of outputs
   M[I3, 1] = r3;                                  // Set the number of outputs in the cbops copy struc
                                                   // Dst addr for the list of output ports

   r0 = $M.multi_chan_output.wired_out_port_table; // Src addr for the wired 16bit output port table
   r1 = $M.multi_chan_output.wired_24_bit_out_port_table;   // Src addr for the wired 24bit output port table
   null = r7 - $RESOLUTION_MODE_24BIT;             // 24 bit reolution mode?
   if Z r0 = r1;                                   // Yes - use the 24bit output port table
   r10 = $MULTI_CHAN_MAX_WIRED_OUT_CHANNELS;       // Maximum number of wired output channels
   call $multi_chan_copy_enabled_channels;         // Copy the (enabled) output ports into the cbops copy struc

   // Pop rLink from stack
   jump $pop_rLink_and_rts;

.ENDMODULE;


// *****************************************************************************
// MODULE:
//    $multi_chan_copy_enabled_channels
//
// DESCRIPTION:
//    Helper function used to configure a cbops copy struc given a
//    table of cbuffers/ports and a channel enable bit mask
//
// INPUTS:
//    r0 = address of source buffer/port table
//    r4 = channel enable bit mask
//    r6 = address of variable holding the sync cbuffer/port (output)
//    r10 = maximum number of channels
//    I3 = address of destination buffer/port table (for enabled channels)
//
// OUTPUTS:
//    I3 = next location in destination buffer/port table
//
// TRASHED:
//    r0, r1, r2, r10, I2, DoLoop
//
// *****************************************************************************
.MODULE $M.multi_chan_copy_enabled_channels;
   .CODESEGMENT MULTI_CHAN_COPY_ENABLED_CHANNELS_PM;

   $multi_chan_copy_enabled_channels:

   I2 = r0;                                        //
   r1 = 1;                                         // Initialise enable bit mask selector

   do channel_loop;

      r0 = M[I2, 1];                               // Get a cbuffer/port from the source table

      null = r4 AND r1;                            // Check if the port is enabled (Z: disabled, NZ: enabled)
      if Z jump not_enabled;

         r2 = M[r6];                               // Get the saved sync cbuffer/port value
         if Z r2 = r0;                             // If zero, set this as the sync cbuffer/port
         M[r6] = r2;                               // Save it

         M[I3, 1] = r0;                            // Write the cbuffer/port to the cbops copy struc

      not_enabled:

      r1 = r1 LSHIFT 1;                            // Select next channel enable bit

   channel_loop:

   rts;

.ENDMODULE;


// *****************************************************************************
// MODULE:
//    $multi_chan_build_channel_enable_mask
//
// DESCRIPTION:
//    Helper function to build a bit mask representing the channels enabled
//
// INPUTS:
//    r0 = address of port table
//    r10 = maximum number of channels
//
// OUTPUTS:
//    r4 = channel enable bit mask (bit0: chan0, bit1: chan1 etc up to max 24 channels)
//
// TRASHED:
//    r0, r1, r10, I2, DoLoop
//
// *****************************************************************************
.MODULE $M.multi_chan_build_channel_enable_mask;
   .CODESEGMENT MULTI_CHAN_BUILD_CHANNEL_ENABLE_MASK_PM;

   $multi_chan_build_channel_enable_mask:

   // Push rLink onto stack
   $push_rLink_macro;

   I2 = r0;                                        // Address of port table
   r1 = 1;                                         // Initialise enable bit mask selector
   r4 = 0;                                         // Initialise enable bit mask (all disabled by default)

   // Loop over ports in table and set a bit flag for each one enabled
   do channel_loop;

      r0 = M[I2, 1];                               // Get an output port

      // Check if the port is enabled (Z: disabled, NZ: enabled)
      call $cbuffer.is_it_enabled;
      if Z jump not_enabled;

         r4 = r4 OR r1;                            // Set the channel enabled flag in enable bit mask

      not_enabled:

      r1 = r1 LSHIFT 1;                            // Select next channel enable bit

   channel_loop:

   // Pop rLink from stack
   jump $pop_rLink_and_rts;

.ENDMODULE;


// *****************************************************************************
// MODULE:
//    $multi_chan_config_tone_mixing
//
// DESCRIPTION:
//    Helper function to configure the prompt (tone) mixing
//    (this is called from a VM message handler)
//
// INPUTS:
//    None
//
// OUTPUTS:
//    none
//    [sets cbops tone mixer operator parameters]
//
// TRASHED:
//    r0, r1, r2, r3, r4
//
// *****************************************************************************
.MODULE $M.multi_chan_config_tone_mixing;
   .CODESEGMENT MULTI_CHAN_CONFIG_TONE_MIXING_PM;

   $multi_chan_config_tone_mixing:

   r3 = M[$interface_map_struc + $INTERFACE_MAP_ENABLED_CHANNELS_FIELD];      // Get the bitfield for channels enabled

   // Examine CHAIN0 tone mixing channels
   // -----------------------------------
   r4 = M[$interface_map_struc + $INTERFACE_MAP_CHAIN1_CHANNELS_FIELD];       // Get the bitfield for CHAIN1 channels
   r1 = r4 XOR -1;                                                            // Invert to get CHAIN0 channel bitfield
   r0 = r3 AND r1;                                                            // Mask for CHAIN0 enabled channels

   // Get the tone mix ratios for primary and aux
   r1 = M[$M.multi_chan_output.prim_tone_mix_ratio];                          // Nominally 0.5
   r2 = M[$M.multi_chan_output.aux_tone_mix_ratio];                           // Nominally 0.5

   // Set mix ratios for ch2 & ch3 for aux since these are only used for aux tone mixing
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch2_mix_op, param, $cbops.auto_resample_mix.TONE_MIXING_RATIO_FIELD, r2)
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch3_mix_op, param, $cbops.auto_resample_mix.TONE_MIXING_RATIO_FIELD, r2)

   // If chain0 contains the primary channels set ch0 & ch1 to primary tone ratio otherwise aux tone ratio
   null = r0 AND $MULTI_CHAN_PRIMARY_CHANNELS_MASK;
   if Z r1 = r2;                                                              // If not primary assume aux tone mixing
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch0_mix_op, param, $cbops.auto_resample_mix.TONE_MIXING_RATIO_FIELD, r1)
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch1_mix_op, param, $cbops.auto_resample_mix.TONE_MIXING_RATIO_FIELD, r1)

   // Examine CHAIN1 tone mixing channels
   // -----------------------------------
   r0 = r3 AND r4;                                                            // Mask for CHAIN1 enabled channels

   // Get the tone mix ratios for primary and aux
   r1 = M[$M.multi_chan_output.prim_tone_mix_ratio];                          // Nominally 0.5
   r2 = M[$M.multi_chan_output.aux_tone_mix_ratio];                           // Nominally 0.5

   // If chain0 contains the primary channels set ch0 & ch1 to primary tone ratio otherwise aux tone ratio
   null = r0 AND $MULTI_CHAN_PRIMARY_CHANNELS_MASK;
   if Z r1 = r2;                                                              // If not primary assume aux tone mixing
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain1_ch0_mix_op, param, $cbops.auto_resample_mix.TONE_MIXING_RATIO_FIELD, r1)
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain1_ch1_mix_op, param, $cbops.auto_resample_mix.TONE_MIXING_RATIO_FIELD, r1)

   rts;

.ENDMODULE;


// *****************************************************************************
// MODULE:
//    $multi_chan_set_prim_tone_mix_ratio
//
// DESCRIPTION:
//    Helper function to set the primary channel tone mixing ratio
//
// INPUTS:
//    r3 = primary channel tone mixing ratio
//
// OUTPUTS:
//    none
//
// TRASHED:
//    none
//
// *****************************************************************************
.MODULE $M.multi_chan_set_prim_tone_mix_ratio;
   .CODESEGMENT MULTI_CHAN_SET_PRIM_TONE_MIX_RATIO_PM;

   $multi_chan_set_prim_tone_mix_ratio:

   // Set the primary channel tone mixing ratio
   M[$M.multi_chan_output.prim_tone_mix_ratio] = r3;

   rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $multi_chan_set_aux_tone_mix_ratio
//
// DESCRIPTION:
//    Helper function to set the aux channel tone mixing ratio
//
// INPUTS:
//    r3 = aux channel tone mixing ratio
//
// OUTPUTS:
//    none
//
// TRASHED:
//    none
//
// *****************************************************************************
.MODULE $M.multi_chan_set_aux_tone_mix_ratio;
   .CODESEGMENT MULTI_CHAN_SET_AUX_TONE_MIX_RATIO_PM;

   $multi_chan_set_aux_tone_mix_ratio:

   // Set the aux channel tone mixing ratio
   M[$M.multi_chan_output.aux_tone_mix_ratio] = r3;

   rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $multi_chan_soft_mute
//
// DESCRIPTION:
//    Helper function to unmute/mute multi channel outputs
//
// INPUTS:
//    r3 = bitfield for muted channels
//
// OUTPUTS:
//    none
//
// TRASHED:
//    r0, r1, r2, r3, r4, r8
//
// *****************************************************************************
.MODULE $M.multi_chan_soft_mute;
   .CODESEGMENT MULTI_CHAN_SOFT_MUTE_PM;

   $multi_chan_soft_mute:

   // Push rLink onto stack
   $push_rLink_macro;

   M[$M.multi_chan_output.channels_mute_en] = r3;                          // Set the bitfield for muted channels

   // Chain 0 muting
   r0 = M[$interface_map_struc + $INTERFACE_MAP_ENABLED_CHANNELS_FIELD];   // Get the bitfield for enabled channels
   r1 = M[$interface_map_struc + $INTERFACE_MAP_CHAIN1_CHANNELS_FIELD];    // Get the bitfield for CHAIN1 channels
   r1 = r1 XOR -1;                                                         // Invert mask to get CHAIN0 channel bitfield
   r0 = r0 AND r1;                                                         // Mask for chain0 enabled channels
   r1 = r3;                                                                // Muted channels

   // Input: r0=Enabled channels, r1=Chain0 muted channels; Output r3=Chain0 cbops muted channels
   call $multi_chan_calc_cbops_channel_enables;                            // Calculate the chain0 muted cbops channels
   M[$M.multi_chan_output.chain0_mute_en] = r3;                            // Keep for debug

   r1 = 1;                                                                 // Default to unmuted
   null = r3 AND $MULTI_CHAN_CHAIN0_CHAN0_EN;                              // Cbops channel muted?
   if NZ r1 = -r1;                                                         // Yes, set mute direction to muted
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch0_mute_op, param, $cbops.soft_mute_op.MUTE_DIRECTION, r1)  // Configure the mute operator (-1: mute; +1: Unmute)

   r1 = 1;                                                                 //
   null = r3 AND $MULTI_CHAN_CHAIN0_CHAN1_EN;                              //
   if NZ r1 = -r1;                                                         //
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch1_mute_op, param, $cbops.soft_mute_op.MUTE_DIRECTION, r1)

   r1 = 1;                                                                 //
   null = r3 AND $MULTI_CHAN_CHAIN0_CHAN2_EN;                              //
   if NZ r1 = -r1;                                                         //
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch2_mute_op, param, $cbops.soft_mute_op.MUTE_DIRECTION, r1)

   r1 = 1;                                                                 //
   null = r3 AND $MULTI_CHAN_CHAIN0_CHAN3_EN;                              //
   if NZ r1 = -r1;                                                         //
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch3_mute_op, param, $cbops.soft_mute_op.MUTE_DIRECTION, r1)

   r1 = 1;                                                                 //
   null = r3 AND $MULTI_CHAN_CHAIN0_CHAN4_EN;                              //
   if NZ r1 = -r1;                                                         //
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch4_mute_op, param, $cbops.soft_mute_op.MUTE_DIRECTION, r1)

   r1 = 1;                                                                 //
   null = r3 AND $MULTI_CHAN_CHAIN0_CHAN5_EN;                              //
   if NZ r1 = -r1;                                                         //
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch5_mute_op, param, $cbops.soft_mute_op.MUTE_DIRECTION, r1)

   // Chain 1 muting
   r0 = M[$interface_map_struc + $INTERFACE_MAP_ENABLED_CHANNELS_FIELD];   // Get the bitfield for enabled channels
   r1 = M[$interface_map_struc + $INTERFACE_MAP_CHAIN1_CHANNELS_FIELD];    // Get the bitfield for CHAIN1 channels
   r0 = r0 AND r1;                                                         // Mask for chain1 enabled channels
   r1 = M[$M.multi_chan_output.channels_mute_en];                          // Get the bitfield for muted channels

   // Input: r0=Enabled channels, r1=Chain0 muted channels; Output r3=Chain0 cbops muted channels
   call $multi_chan_calc_cbops_channel_enables;                            // Calculate the chain1 muted cbops channels
   M[$M.multi_chan_output.chain1_mute_en] = r3;                            // Keep for debug

   r1 = 1;                                                                 // Default to unmuted
   null = r3 AND $MULTI_CHAN_CHAIN1_CHAN0_EN;                              // Cbops channel muted?
   if NZ r1 = -r1;                                                         // Yes, set mute direction to muted
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain1_ch0_mute_op, param, $cbops.soft_mute_op.MUTE_DIRECTION, r1)  // Configure the mute operator (-1: mute; +1: Unmute)

   r1 = 1;                                                                 //
   null = r3 AND $MULTI_CHAN_CHAIN1_CHAN1_EN;                              //
   if NZ r1 = -r1;                                                         //
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain1_ch1_mute_op, param, $cbops.soft_mute_op.MUTE_DIRECTION, r1)

   // Pop rLink from stack
   jump $pop_rLink_and_rts;
.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $multi_chan_tones_active
//
// DESCRIPTION:
//    Helper function to determine if tone playing is active
//
// INPUTS:
//    none
//
// OUTPUTS:
//    r0 = 0: no tone; other: tone playing is active
//
// TRASHED:
//    r1
//
// *****************************************************************************
.MODULE $M.multi_chan_tones_active;
   .CODESEGMENT MULTI_CHAN_TONES_ACTIVE_PM;

   $multi_chan_tones_active:

   // Check the status of all the mixers
   TEMPLATE_OP_STATE($M.multi_chan_output.chain0_ch0_mix_op, param, $cbops.auto_resample_mix.INPUT_STATE_FIELD, r0)
   TEMPLATE_OP_STATE($M.multi_chan_output.chain0_ch1_mix_op, param, $cbops.auto_resample_mix.INPUT_STATE_FIELD, r1)
   r0 = r0 OR r1;
   TEMPLATE_OP_STATE($M.multi_chan_output.chain0_ch2_mix_op, param, $cbops.auto_resample_mix.INPUT_STATE_FIELD, r1)
   r0 = r0 OR r1;
   TEMPLATE_OP_STATE($M.multi_chan_output.chain0_ch3_mix_op, param, $cbops.auto_resample_mix.INPUT_STATE_FIELD, r1)
   r0 = r0 OR r1;
   TEMPLATE_OP_STATE($M.multi_chan_output.chain1_ch0_mix_op, param, $cbops.auto_resample_mix.INPUT_STATE_FIELD, r1)
   r0 = r0 OR r1;
   TEMPLATE_OP_STATE($M.multi_chan_output.chain1_ch1_mix_op, param, $cbops.auto_resample_mix.INPUT_STATE_FIELD, r1)
   r0 = r0 OR r1;

   rts;
.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $multi_chan_clone_tone_cbuffers
//
// DESCRIPTION:
//    Helper function to duplicate the tone cbuffers for multi-channel usage
//    Here tone data buffers are used by more than one of cbuffer. This effectively
//    allows the same tone data to be re-read by a number of channels.
//
// INPUTS:
//    none
//
// OUTPUTS:
//    none
//
// TRASHED:
//    r0
//
// *****************************************************************************
.MODULE $M.multi_chan_clone_tone_cbuffers;
   .CODESEGMENT MULTI_CHAN_CLONE_TONE_CBUFFERS_PM;

   $multi_chan_clone_tone_cbuffers:

   // Left channels
   // -------------

   // Save the read pointers
   r0 = M[$tone0_in_left_resample_cbuffer_struc + $cbuffer.READ_ADDR_FIELD];
   M[$M.multi_chan_output.tone0_in_left_read_ptr] = r0;
   r0 = M[$tone1_in_left_resample_cbuffer_struc + $cbuffer.READ_ADDR_FIELD];
   M[$M.multi_chan_output.tone1_in_left_read_ptr] = r0;
   r0 = M[$tone2_in_left_resample_cbuffer_struc + $cbuffer.READ_ADDR_FIELD];
   M[$M.multi_chan_output.tone2_in_left_read_ptr] = r0;

   // Clone the write pointers
   r0 = M[$tone_in_left_resample_cbuffer_struc + $cbuffer.WRITE_ADDR_FIELD];
   M[$tone0_in_left_resample_cbuffer_struc + $cbuffer.WRITE_ADDR_FIELD] = r0;
   M[$tone1_in_left_resample_cbuffer_struc + $cbuffer.WRITE_ADDR_FIELD] = r0;
   M[$tone2_in_left_resample_cbuffer_struc + $cbuffer.WRITE_ADDR_FIELD] = r0;

   // Right channels
   // --------------

   // Save the read pointers
   r0 = M[$tone0_in_right_resample_cbuffer_struc + $cbuffer.READ_ADDR_FIELD];
   M[$M.multi_chan_output.tone0_in_right_read_ptr] = r0;
   r0 = M[$tone1_in_right_resample_cbuffer_struc + $cbuffer.READ_ADDR_FIELD];
   M[$M.multi_chan_output.tone1_in_right_read_ptr] = r0;
   r0 = M[$tone2_in_right_resample_cbuffer_struc + $cbuffer.READ_ADDR_FIELD];
   M[$M.multi_chan_output.tone2_in_right_read_ptr] = r0;

   // Clone the write pointers
   r0 = M[$tone_in_right_resample_cbuffer_struc + $cbuffer.WRITE_ADDR_FIELD];
   M[$tone0_in_right_resample_cbuffer_struc + $cbuffer.WRITE_ADDR_FIELD] = r0;
   M[$tone1_in_right_resample_cbuffer_struc + $cbuffer.WRITE_ADDR_FIELD] = r0;
   M[$tone2_in_right_resample_cbuffer_struc + $cbuffer.WRITE_ADDR_FIELD] = r0;

   rts;
.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $multi_chan_adjust_tone_cbuffers
//
// DESCRIPTION:
//    Helper function to adjust the master tone cbuffer read pointers.
//    Channels not being read are ignored and the master cbuffer is
//    updated with the read pointers corresponding to the fullest
//    slave cbuffer.
//
// INPUTS:
//    none
//
// OUTPUTS:
//    none
//
// TRASHED:
//    r0, r1, r2, r3, r4, r5, r6
//
// *****************************************************************************
.MODULE $M.multi_chan_adjust_tone_cbuffers;
   .CODESEGMENT MULTI_CHAN_ADJUST_TONE_CBUFFERS_PM;

   $multi_chan_adjust_tone_cbuffers:

   // Initialisation
   r2 = M[$tone_in_left_resample_cbuffer_struc + $cbuffer.SIZE_FIELD];           // Size of all the tone buffers
   r3 = 0;                                                                       // Initial the max fill level is zero

   // Left channels
   // -------------
   r4 = M[$tone_in_left_resample_cbuffer_struc + $cbuffer.WRITE_ADDR_FIELD];     // Common left wrPtr

   r0 = M[$tone0_in_left_resample_cbuffer_struc + $cbuffer.READ_ADDR_FIELD];
   null = r0 - M[$M.multi_chan_output.tone0_in_left_read_ptr];                   // Is this tone buffer being used?
   if Z jump skip_tone0_in_left;                                                 // No - skip it

      r0 = r4 - r0;                                                              // wrPtr - rdPtr
      if NEG r0 = r0 + r2;                                                       // Correct for buffer wrap
      r3 = MAX r0;                                                               // Find the largest fill level

   skip_tone0_in_left:

   r0 = M[$tone1_in_left_resample_cbuffer_struc + $cbuffer.READ_ADDR_FIELD];
   null = r0 - M[$M.multi_chan_output.tone1_in_left_read_ptr];                   // Is this tone buffer being used?
   if Z jump skip_tone1_in_left;                                                 // No - skip it

      r0 = r4 - r0;                                                              // wrPtr - rdPtr
      if NEG r0 = r0 + r2;                                                       // Correct for buffer wrap
      r3 = MAX r0;                                                               // Find the largest fill level

   skip_tone1_in_left:

   r0 = M[$tone2_in_left_resample_cbuffer_struc + $cbuffer.READ_ADDR_FIELD];
   null = r0 - M[$M.multi_chan_output.tone2_in_left_read_ptr];                   // Is this tone buffer being used?
   if Z jump skip_tone2_in_left;                                                 // No - skip it

       r0 = r4 - r0;                                                             // wrPtr - rdPtr
       if NEG r0 = r0 + r2;                                                      // Correct for buffer wrap
       r3 = MAX r0;                                                              // Find the largest fill level

   skip_tone2_in_left:

   // Right channels
   // --------------
   r5 = 0;                                                                       // Initial the max fill level is zero
   r6 = M[$tone_in_right_resample_cbuffer_struc + $cbuffer.WRITE_ADDR_FIELD];    // Common right wrPtr

   r0 = M[$tone0_in_right_resample_cbuffer_struc + $cbuffer.READ_ADDR_FIELD];
   null = r0 - M[$M.multi_chan_output.tone0_in_right_read_ptr];                  // Is this tone buffer being used?
   if Z jump skip_tone0_in_right;                                                // No - skip it

      r0 = r6 - r0;                                                              // wrPtr - rdPtr
      if NEG r0 = r0 + r2;                                                       // Correct for buffer wrap
      r5 = MAX r0;                                                               // Find the largest fill level

   skip_tone0_in_right:

   r0 = M[$tone1_in_right_resample_cbuffer_struc + $cbuffer.READ_ADDR_FIELD];
   null = r0 - M[$M.multi_chan_output.tone1_in_right_read_ptr];                  // Is this tone buffer being used?
   if Z jump skip_tone1_in_right;                                                // No - skip it

      r0 = r6 - r0;                                                              // wrPtr - rdPtr
      if NEG r0 = r0 + r2;                                                       // Correct for buffer wrap
      r5 = MAX r0;                                                               // Find the largest fill level

   skip_tone1_in_right:

   r0 = M[$tone2_in_right_resample_cbuffer_struc + $cbuffer.READ_ADDR_FIELD];
   null = r0 - M[$M.multi_chan_output.tone2_in_right_read_ptr];                  // Is this tone buffer being used?
   if Z jump skip_tone2_in_right;                                                // No - skip it

       r0 = r6 - r0;                                                             // wrPtr - rdPtr
       if NEG r0 = r0 + r2;                                                      // Correct for buffer wrap
       r5 = MAX r0;                                                              // Find the largest fill level

   skip_tone2_in_right:

   // If no left channel then adjust the read pointer as done for the right channel
   null = r3;
   if Z r3 = r5;

   // If no right channel then adjust the read pointer as done for the left channel
   null = r5;
   if Z r5 = r3;

   // Calculate new left common rd ptr (i.e. use the pointer from the buffer which is the the fullest)
   L0 = r2;                                                                      // Common buffer size
   M0 = -r3;                                                                     // Left read ptr adjustment
   I0 = r4;                                                                      // Left wrPtr
   r0 = M[I0,M0];                                                                // Dummy read, Left newRdPtr = wrPtr - maxFill
   r0 = I0;                                                                      // New left rdPtr
   M[$tone_in_left_resample_cbuffer_struc + $cbuffer.READ_ADDR_FIELD] = r0;      // Set the common left rdPtr

   // Calculate new right common rd ptr (i.e. use the pointer from the buffer which is the the fullest)
   M0 = -r5;                                                                     // Right read ptr adjustment
   I0 = r6;                                                                      // Right wrPtr
   r0 = M[I0,M0];                                                                // Dummy read, Right newRdPtr = wrPtr - maxFill
   r0 = I0;                                                                      // New right rdPtr
   M[$tone_in_right_resample_cbuffer_struc + $cbuffer.READ_ADDR_FIELD] = r0;     // Set the common right rdPtr
   L0 = 0;

   rts;
.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $multi_chan_set_port_for_latency_calc
//
// DESCRIPTION:
//    Helper function to set the port in the latency calculation table
//
// INPUTS:
//    r0 = address of latency calculation table
//    r1 = port to use for latency calculation
//
// OUTPUTS:
//    none
//
// TRASHED:
//    r0, I0, M0, M1
//
// *****************************************************************************
.MODULE $M.multi_chan_set_port_for_latency_calc;
   .CODESEGMENT MULTI_CHAN_SET_PORT_FOR_LATENCY_CALC_PM;

   .CONST $LATENCY_MEASURE_ELEMENTS_PER_ENTRY      3;

   $multi_chan_set_port_for_latency_calc:

   I0 = r0;
   M0 = $LATENCY_MEASURE_ELEMENTS_PER_ENTRY;    // Elements per entry in latency calculation table
   M1 = -$LATENCY_MEASURE_ELEMENTS_PER_ENTRY;   // Negative of the above

   next_entry:

   r0 = M[I0,M0];                               // Get the buffer address/port
   if Z jump exit;                              // Exit if at the end of table

      // Is it a port?
      Null = SIGNDET r0;                        // Z: port, NZ: cbuffer
      if NZ jump next_entry;                    // If it's a cbuffer check the next entry

      // It's a port so set the new value
      r0 = M[I0,M1];                            // Point at the table entry to change (reverse previous post increment)
      M[I0,M1] = r1;                            // Set the port

   exit:

   rts;
.ENDMODULE;
// *****************************************************************************
// MODULE:
//    $multi_chan_select_chain_usage
//
// DESCRIPTION:
//    Configure the cbops chain usage according to the configured wired output
//    device types, the configured nominal output rate, and the configured
//    master/slave operation. This is done as given in the following table:
//
//                         DAC @ 48kHz       DAC @44.1kHz         No DAC
//    I2S (SPDIF) Master:  Chain 0           Chain 0/Chain1       Chain 0
//    I2S Slave:           Chain 0/Chain1    Chain 0/Chain1       Chain 0
//    No Dig. output:      Chain 0           Chain 0              No chain
//
// INPUTS:
//    r3 = Pointer to interface_map_struc containing port connection, and DAC and chain routing masks
//    r4 = Channel table bit mask indicating channel usage
//    r5 = Channel table bit mask indicating DAC usage
//
// OUTPUTS:
//    r3 = Pointer to interface_map_struc containing port connection, and DAC and chain routing masks
//         (routine assigns chain routing entry to this mapping structure)
//
// TRASHED:
//    r0, r1
//
// *****************************************************************************
.MODULE $M.multi_chan_select_chain_usage;
   .CODESEGMENT MULTI_CHAN_SELECT_CHAIN_USAGE_PM;

   $multi_chan_select_chain_usage:

   r1 = 0;                                               // Default to zero chain1 channels

   null = r4 AND r5;                                     // Any enabled channels configured for DAC type outputs?
   if Z jump only_chain0;                                // No - use only chain0

   r0 = r5 XOR -1;                                       // Channel table bit mask indicating I2S (non-DAC) usage
   null = r4 AND r0;                                     // Any enabled channels using digital type (e.g. I2S) outputs?
   if Z jump only_chain0;                                // No - use only chain0

   // Check the rate and slave configuration
   r0 = M[$current_dac_sampling_rate];                   // Get the nominal output rate
   null = r0 - 44100;                                    // 44.1kHz?
   if Z jump chain0_and_chain1;                          // Yes - use both chains
   r0 = M[$M.multi_chan_output.i2s_slave0];              // Is master operation used?
   if Z jump only_chain0;                                // Yes - use only chain0

   chain0_and_chain1:
   r1 = r5;                                              // DAC outputs use chain1

   only_chain0:


   M[r3 + $INTERFACE_MAP_CHAIN1_CHANNELS_FIELD] = r1;    // Store CHAIN1 channel mask

   rts;
.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $multi_chan_build_channel_type_mask
//
// DESCRIPTION:
//    Build a bitfield according to the configured wired output types
//
// INPUTS:
//    r0 = address of wired output device table
//    r1 = wired output type from which to build mask
//    r10 = maximum number of channels
//
// OUTPUTS:
//    r5 = channel table bit mask indicating DAC usage
//    (where table entry 0: 1, table entry 1: 2, table entry 2: 4 etc)
//
// TRASHED:
//    r0, r2, r5, I2
//
// *****************************************************************************
.MODULE $M.multi_chan_build_channel_type_mask;
   .CODESEGMENT MULTI_CHAN_BUILD_CHANNEL_TYPE_MASK_PM;

   $multi_chan_build_channel_type_mask:

   I2 = r0;                                        // Wired output device type table
   r5 = 0;                                         // Cumulative table bitfield (used as the output)
   r2 = 1;                                         // Initialise port table bitfield selector
   do channel_loop;

      r0 = M[I2, 1];                               // Get a wired output device type
      r0 = r0 - r1;                                // Is this the specified output type?
      if Z r5 = r5 OR r2;                          // Yes - set the bit mask selector
      r2 = r2 LSHIFT 1;                            // Next port table bitfield mask

   channel_loop:

   rts;
.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.multi_chan_port_scan_and_routing_config
//
// DESCRIPTION:
//    Create Port Mapping and call configuration functions for output
//    copy handlers, rate matching and resampling, and frame processing
//
// INPUTS:
//    r3 = Pointer to interface_map_struc containing port connection and chain routing masks
//       ([0]: Bitmask for wired output channels:                       bit[n]: 0: disabled, 1: enabled
//        [1]: DAC channel mask (sets which channels are on DACs)       bit[n]: 0: Digital, 1: DAC
//        [2]: Bitmask for selecting wired output channels chain:       bit[n]: 0: CHAIN0, 1: CHAIN1
//        [3]: S/PDIF channel mask (sets which channels are on S/PDIF)  bit[n]: 0: Other, 1: S/PDIF
//    r7 = resolution mode flag ($RESOLUTION_MODE_16BIT or $RESOLUTION_MODE_24BIT)
//
// OUTPUTS:
//    none
//
// TRASHES:
//    Assume all
//
// *****************************************************************************
.MODULE $M.multi_chan_port_scan_and_routing_config;
   .CODESEGMENT MULTI_CHAN_PORT_SCAN_AND_ROUTING_CONFIG_PM;
   .DATASEGMENT DM;

   .VAR fp_config_input;

   $multi_chan_port_scan_and_routing_config:

   // Push rLink onto stack
   $push_rLink_macro;

   r0 = $M.multi_chan_output.wired_out_port_table;       // Src addr for the wired 16bit output port table
   r1 = $M.multi_chan_output.wired_24_bit_out_port_table;// Src addr for the wired 24bit output port table
   null = r7 - $RESOLUTION_MODE_24BIT;                   // 24 bit reolution mode?
   if Z r0 = r1;                                         // Yes - use the 24bit output port table

   // Determine which channels are active and build a corresponding channel bitfield mask
   // (active channels are determined by checking output ports)
                                                         // r0 = Src addr for the wired output port table
   r10 = $MULTI_CHAN_MAX_WIRED_OUT_CHANNELS;             // Maximum number of wired output channels
   call $multi_chan_build_channel_enable_mask;           // Build a bit mask for the enabled channels, (output: r4=bit mask)
   M[r3 + $INTERFACE_MAP_ENABLED_CHANNELS_FIELD] = r4;   // load bit mask for the enabled channels into interface structure

   // Determine which channels are using the DAC outputs
   // Build a channel bitfield according to the configured wired output types
   r0 = $M.multi_chan_output.wired_out_type_table;       // Wired output device type table
   r1 = $OUTPUT_INTERFACE_TYPE_DAC;                      // Wired output device type from which to build mask
   r10 = $MULTI_CHAN_MAX_WIRED_OUT_CHANNELS;             // Maximum number of wired output channels
   call $multi_chan_build_channel_type_mask;             // Determine DAC usage (output: r5=channel table bitfield)
   M[r3 + $INTERFACE_MAP_DAC_CHANNELS_FIELD] = r5;       // load bit mask for the DAC usage into interface structure

   // Configure the cbops chain usage according the wired output device types,
   // the rate and the master/slave operation
   // Input: r3 = pointer to interface map struc
   //        r4 = Channel table bit mask indicating channel usage
   //        r5 = Channel table bit mask indicating DAC usage
   // Output: r3 = pointer to interface map struc
   call $multi_chan_select_chain_usage;                  // Configure the cbops chain usage

   // Determine which channels are using the SPDIF outputs
   // Build a channel bitfield according to the configured wired output types
   r0 = $M.multi_chan_output.wired_out_type_table;       // Wired output device type table
   r1 = $OUTPUT_INTERFACE_TYPE_SPDIF;                    // Wired output device type from which to build mask
   r10 = $MULTI_CHAN_MAX_WIRED_OUT_CHANNELS;             // Maximum number of wired output channels
   call $multi_chan_build_channel_type_mask;             // Determine SPDIF usage (output: r5=channel table bitfield)
   M[r3 + $INTERFACE_MAP_SPDIF_CHANNELS_FIELD] = r5;     // load bit mask for the SPDIF usage into interface structure

   // needed to configure frame process based on port settings
   M[fp_config_input] = r3;

   // Select the type of rate matching appropriate for the configuration
   // Input: r5 = point to interface map struc
   r5 = r3;
   call $multi_chan_select_rate_matching;

   // Configure the output copy chains and build the cbops copy strucs for the handler
   // Input: r5 = point to interface map struc
   //    ([0]: Bitmask for wired output channels:                       bit[n]: 0: disabled, 1: enabled
   //     [1]: DAC channel mask (sets which channels are on DACs)       bit[n]: 0: Digital, 1: DAC
   //     [2]: Bitmask for selecting wired output channels chain:       bit[n]: 0: CHAIN0, 1: CHAIN1
   //     [3]: S/PDIF channel mask (sets which channels are on S/PDIF)  bit[n]: 0: Other, 1: S/PDIF
   //        r7 = resolution mode flag ($RESOLUTION_MODE_16BIT or $RESOLUTION_MODE_24BIT)
   call $multi_chan_build_all_cbops_copy_strucs;

   // Configure the multi-channel output cbops
   call $multi_chan_config_cbops_copy_strucs;

   // -- configure chain synchronisation --
   // chain0 buffer
   r0 = M[$M.multi_chan_output.chain0_copy_struc + 2];
   M[$M.multi_chan_output.chain0_pcm_cbuffers_latency_measure + 0] = r0;

   // chain0 sync port
   r0 = M[$M.multi_chan_output.chain0_sync_port];
   M[$M.multi_chan_output.chain0_pcm_cbuffers_latency_measure + 3] = r0;
   M[$calc_chain0_actual_port_rate_struc + $calc_actual_port_rate.PORT_FIELD] = r0;

   // chain1 buffer
   r0 = M[$M.multi_chan_output.chain1_copy_struc + 2];
   M[$M.multi_chan_output.chain1_pcm_cbuffers_latency_measure + 0] = r0;

   // chain1 sync port
   r0 = M[$M.multi_chan_output.chain1_sync_port];
   M[$M.multi_chan_output.chain1_pcm_cbuffers_latency_measure + 3] = r0;
   M[$calc_chain1_actual_port_rate_struc + $calc_actual_port_rate.PORT_FIELD] = r0;

   // configure frame process based on port settings
   r3 = M[fp_config_input];
   call $M.frame_proc_stream_configure.func;

   // Configure the port rate calclation for chain0
   r0 = M[$M.multi_chan_output.i2s_slave0];
   r1 = $calc_chain0_actual_port_rate_struc;
   call $config_calc_port_rate;

   // Configure the port rate calclation for chain1 (not currently supported)
   //r0 = M[$M.multi_chan_output.i2s_slave1];
   //r1 = $calc_chain1_actual_port_rate_struc;
   //call $config_calc_port_rate;

#if defined(MULTI_CHANNEL_DISABLE)
   // Check for the invalid use of multi-channel when it has been disabled

   // Get the bitfield for enabled channels
   r0 = M[$interface_map_struc + $INTERFACE_MAP_ENABLED_CHANNELS_FIELD];
   r1 = $MULTI_CHAN_SECONDARY_CHANNELS_MASK | $MULTI_CHAN_AUX_CHANNELS_MASK;

   // Are any secondary or aux channels selected?
   null = r0 AND r1;
   if NZ call $error;
#endif

   // Pop rLink from stack
   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.multi_chan_select_rate_matching
//
// DESCRIPTION:
//    Select the type of rate matching appropriate for the configuration
//
// INPUTS:
//    r5 = Pointer to interface_map_struc containing port connection and chain routing masks
//       ([0]: Bitmask for wired output channels:                       bit[n]: 0: disabled, 1: enabled
//        [1]: DAC channel mask (sets which channels are on DACs)       bit[n]: 0: Digital, 1: DAC
//        [2]: Bitmask for selecting wired output channels chain:       bit[n]: 0: CHAIN0, 1: CHAIN1
//        [3]: S/PDIF channel mask (sets which channels are on S/PDIF)  bit[n]: 0: Other, 1: S/PDIF
//
// OUTPUTS:
//    none
//    (but Sets M[$chain0_hw_warp_enable] and M[$chain1_hw_warp_enable] accordingly)
//
// TRASHES:
//    r0, r1, r2, r3, r4
//
// *****************************************************************************
.MODULE $M.multi_chan_select_rate_matching;
   .CODESEGMENT MULTI_CHAN_SELECT_RATE_MATCHING_PM;

   $multi_chan_select_rate_matching:

   // Default to software rate matching
   M[$chain0_hw_warp_enable] = 0;
   M[$chain1_hw_warp_enable] = 0;

   // If a wireless subwoofer may connect don't allow H/W rate matching
#ifdef WIRELESS_SUB_ENABLE
   jump exit;
#endif

   // If TWS may connect don't allow H/W rate matching
#ifdef TWS_ENABLE
   jump exit;
#endif

   // If rate matching is disabled don't select H/W rate matching
   null = M[$rate_match_disable];
   if NZ jump exit;

   r0 = M[r5 + $INTERFACE_MAP_ENABLED_CHANNELS_FIELD];   // Enabled channels mask
   r1 = M[r5 + $INTERFACE_MAP_DAC_CHANNELS_FIELD];       // DAC channels mask
   r2 = M[r5 + $INTERFACE_MAP_CHAIN1_CHANNELS_FIELD];    // Channels on chain 1 mask
   r3 = r2 XOR -1;                                       // Channels on chain 0 mask
   r3 = r3 AND r0;                                       // Enabled channels on chain 0 mask
   r2 = r2 AND r0;                                       // Enabled channels on chain 1 mask
   r4 = r1 XOR -1;                                       // Non-DAC channels mask

   // Can H/W rate matching be used on chain0?
   null = r3 AND r1;                                     // Enabled DACs on chain0?
   if Z jump check_chain1;                               // No - check chain1

      null = r3 AND r4;                                  // Enabled non-DACs on Chain0?
      if NZ jump exit;                                   // Yes - can't use H/W rate matching, exit

      r0 = 1;
      M[$chain0_hw_warp_enable] = r0;                    // Use H/W rate matching on chain 0
      jump exit;

   check_chain1:

   // Can H/W rate matching be used on chain1?
   null = r2 AND r1;                                     // Enabled DACs on chain1?
   if Z jump exit;                                       // No - exit

      null = r2 AND r4;                                  // Enabled non-DACs on Chain1?
      if NZ jump exit;                                   // Yes - can't use H/W rate matching, exit

      r0 = 1;
      M[$chain1_hw_warp_enable] = r0;                    // Use H/W rate matching on chain 1

   exit:

   rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.set_output_handler_timer
//
// DESCRIPTION:
//    Set the output handler timer according to the output rate and ANC setting
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
.MODULE $M.set_output_handler_timer;
   .CODESEGMENT SET_OUTPUT_HANDLER_TIMER_PM;

   $set_output_handler_timer:

   r1 = $M.multi_chan_output.handler_period_table_16bit;
   r2 = $M.multi_chan_output.handler_period_table_24bit;
   r3 = M[$outputResolutionMode];

   // Select the table according to the output resolution
   null = r3 - $RESOLUTION_MODE_24BIT;
   if NZ r2 = r1;

   r0 = M[r2+$OUTPUT_HANDLER_PERIOD_TABLE_LOW_RATE_INDEX];     // Fout <= 48kHz
   r1 = M[r2+$OUTPUT_HANDLER_PERIOD_TABLE_HIGH_RATE_INDEX];    // Fout > 48kHz

   // Configure according to the output sampling rate
   r3 = M[$current_dac_sampling_rate];
   Null = 48000 - r3;                                          // Fout > 48kHz?
   if NEG r0 = r1;                                             // Yes - use the hi-rate timer period

   // Check the ANC configuration
   r3 = M[$ancMode];
   null = r3 - $ANC_NONE;                                      // ANC enabled?
   if Z jump timer_value_calculated;                           // No - set the previously calculated timer value

   r0 = M[r2+$OUTPUT_HANDLER_PERIOD_TABLE_ANC_96K_INDEX];      // ANC @96kHz
   r1 = M[r2+$OUTPUT_HANDLER_PERIOD_TABLE_ANC_192K_INDEX];     // ANC @192kHz

   // Check the ANC output rate
   null = r3 - $ANC_192K;
   if Z r0 = r1;

   timer_value_calculated:
   M[$tmr_period_audio_copy] = r0;

   rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.multi_chan_purge_buffers
//
// DESCRIPTION:
//    Purge (empty) the multi-channel wired output cbuffers
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
.MODULE $M.multi_chan_purge_buffers;
   .CODESEGMENT MULTI_CHAN_PURGE_BUFFERS_PM;

   $multi_chan_purge_buffers:

   // Purge multi_chan_primary left buffer
   r0 = M[$multi_chan_primary_left_out_cbuffer_struc + $cbuffer.WRITE_ADDR_FIELD];
   M[$multi_chan_primary_left_out_cbuffer_struc + $cbuffer.READ_ADDR_FIELD] = r0;

   // Purge multi_chan_primary right buffer
   r0 = M[$multi_chan_primary_right_out_cbuffer_struc + $cbuffer.WRITE_ADDR_FIELD];
   M[$multi_chan_primary_right_out_cbuffer_struc + $cbuffer.READ_ADDR_FIELD] = r0;

   // Purge multi_chan_secondary left buffer
   r0 = M[$multi_chan_secondary_left_out_cbuffer_struc + $cbuffer.WRITE_ADDR_FIELD];
   M[$multi_chan_secondary_left_out_cbuffer_struc + $cbuffer.READ_ADDR_FIELD] = r0;

   // Purge multi_chan_secondary right buffer
   r0 = M[$multi_chan_secondary_right_out_cbuffer_struc + $cbuffer.WRITE_ADDR_FIELD];
   M[$multi_chan_secondary_right_out_cbuffer_struc + $cbuffer.READ_ADDR_FIELD] = r0;

   // Purge multi_chan_aux left buffer
   r0 = M[$multi_chan_aux_left_out_cbuffer_struc + $cbuffer.WRITE_ADDR_FIELD];
   M[$multi_chan_aux_left_out_cbuffer_struc + $cbuffer.READ_ADDR_FIELD] = r0;

   // Purge multi_chan_aux right buffer
   r0 = M[$multi_chan_aux_right_out_cbuffer_struc + $cbuffer.WRITE_ADDR_FIELD];
   M[$multi_chan_aux_right_out_cbuffer_struc + $cbuffer.READ_ADDR_FIELD] = r0;

   // Purge multi_chan_sub buffer
   r0 = M[$multi_chan_sub_out_cbuffer_struc + $cbuffer.WRITE_ADDR_FIELD];
   M[$multi_chan_sub_out_cbuffer_struc + $cbuffer.READ_ADDR_FIELD] = r0;

   rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.multi_chan_config_quality
//
// DESCRIPTION:
//    Set the multi-channel rate adjustment filter quality
//
// INPUTS:
//    none
//
// OUTPUTS:
//    none
//
// TRASHES: r2, r3
//
// *****************************************************************************
.MODULE $M.multi_chan_config_quality;
   .CODESEGMENT MULTI_CHAN_CONFIG_QUALITY_PM;

   $multi_chan_config_quality:

   // Configure SW warp to use HD or standard coefficients
   r2 = $sra_coeffs;
   r3 = $cbops.rate_adjustment_and_shift.SRA_COEFFS_SIZE;

   // Check music manager selection parameter
   Null = M[$M.system_config.data.CurParams + $M.MUSIC_MANAGER.PARAMETERS.OFFSET_SW_WARP_TYPE];
   if Z jump set_warp_operator;

      r2 = $sra_coeffs_hd_quality;
      r3 = $cbops.rate_adjustment_and_shift.SRA_HD_QUALITY_COEFFS_SIZE;

   // r2=coeffs, r3=coeff size
   set_warp_operator:

   M[chain1_sync_rate_adjustment_and_shift.param + $cbops.rate_adjustment_and_shift.FILTER_COEFFS_FIELD] = r2;
   M[chain1_sync_rate_adjustment_and_shift.param + $cbops.rate_adjustment_and_shift.FILTER_COEFFS_SIZE_FIELD] = r3;

   rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.multi_chan_config_dither_type
//
// DESCRIPTION:
//    Set the multi-channel dither type
//
// INPUTS:
//    r1 = dither type
//
// OUTPUTS:
//    none
//
// TRASHES: r0, r1, r2
//
// *****************************************************************************
.MODULE $M.multi_chan_config_dither_type;
   .CODESEGMENT MULTI_CHAN_CONFIG_DITHER_TYPE_PM;

   $multi_chan_config_dither_type:

   // Push rLink onto stack
   $push_rLink_macro;

   // Changes can be made from the foreground (prevent values being used while changes are made)
   call $block_interrupts;                            // Trashes r0

   r0 = r1;                                           // Requested dither type

   // Only use the supplied dither type fs >= 44.1kHz
   r2 = $cbops.dither_and_shift.DITHER_TYPE_NONE;     // No dither type
   r1 = M[$current_dac_sampling_rate];                // Get the output (DAC/I2S etc) sampling rate
   null = r1 - 44100;                                 // Output rate < 44.1kHz?
   if NEG r0 = r2;                                    // Disable dither if fs < 44.1kHz

   // Configure dither type for chain0
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch0_dither_and_shift_op, param, $cbops.dither_and_shift.DITHER_TYPE_FIELD, r0)
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch1_dither_and_shift_op, param, $cbops.dither_and_shift.DITHER_TYPE_FIELD, r0)
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch2_dither_and_shift_op, param, $cbops.dither_and_shift.DITHER_TYPE_FIELD, r0)
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch3_dither_and_shift_op, param, $cbops.dither_and_shift.DITHER_TYPE_FIELD, r0)
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch4_dither_and_shift_op, param, $cbops.dither_and_shift.DITHER_TYPE_FIELD, r0)
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch5_dither_and_shift_op, param, $cbops.dither_and_shift.DITHER_TYPE_FIELD, r0)

   // Configure dither type for chain1
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain1_ch0_dither_and_shift_op, param, $cbops.dither_and_shift.DITHER_TYPE_FIELD, r0)
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain1_ch1_dither_and_shift_op, param, $cbops.dither_and_shift.DITHER_TYPE_FIELD, r0)

   call $unblock_interrupts;

   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.multi_chan_config_output_resampler_quality
//
// DESCRIPTION:
//    Set the multi-channel output (ANC) resampler quality
//
// INPUTS:
//    r1 = resolution mode (16: 16bit, 24: 24bit)
//
// OUTPUTS:
//    none
//
// TRASHES: r0, r1, r2
//
// *****************************************************************************
.MODULE $M.multi_chan_config_output_resampler_quality;
   .CODESEGMENT MULTI_CHAN_CONFIG_OUTPUT_RESAMPLER_QUALITY_PM;

   $multi_chan_config_output_resampler_quality:

   // Push rLink onto stack
   $push_rLink_macro;

   // Changes can be made from the foreground (prevent values being used while changes are made)
   call $block_interrupts;                            // Trashes r0

   // Set the resampler precision flag according to the resolution mode
   r0 = 0;                                            // Precision flag for 16bit resolution mode
   r2 = 1;                                            // Precision flag for 24bit resolution mode
   null = r1 - $RESOLUTION_MODE_24BIT;                // 24bit?
   if Z r0 = r2;                                      //

   // Configure the precision for the ANC resampler on chain0
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch0_resamp_op, param, $iir_resamplev2.DBL_PRECISSION_FIELD, r0)
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch1_resamp_op, param, $iir_resamplev2.DBL_PRECISSION_FIELD, r0)

   call $unblock_interrupts;

   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.multi_chan_config_output_scaling
//
// DESCRIPTION:
//    Set the multi-channel output scaling
//
// INPUTS:
//    r1 = resolution mode (16: 16bit, 24: 24bit)
//
// OUTPUTS:
//    none
//
// TRASHES: r0, r1, r2
//
// *****************************************************************************
.MODULE $M.multi_chan_config_output_scaling;
   .CODESEGMENT MULTI_CHAN_CONFIG_OUTPUT_SCALING_PM;

   $multi_chan_config_output_scaling:

   // Push rLink onto stack
   $push_rLink_macro;

   // Changes can be made from the foreground (prevent values being used while changes are made)
   call $block_interrupts;                            // Trashes r0

   r0 = -8;                                           // Scaling for 16bit resolution mode
   null = r1 - $RESOLUTION_MODE_24BIT;                // 24bit?
   if Z r0 = 0;                                       // Scaling for 24bit resolution mode

   // Configure output (shift) scaling for the ANC resampler on chain0
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch0_resamp_op, param, $iir_resamplev2.INPUT_SCALE_FIELD, r0)
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch1_resamp_op, param, $iir_resamplev2.INPUT_SCALE_FIELD, r0)

   // Configure the output (shift) scaling for chain0
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch0_dither_and_shift_op, param, $cbops.dither_and_shift.SHIFT_AMOUNT_FIELD, r0)
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch1_dither_and_shift_op, param, $cbops.dither_and_shift.SHIFT_AMOUNT_FIELD, r0)
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch2_dither_and_shift_op, param, $cbops.dither_and_shift.SHIFT_AMOUNT_FIELD, r0)
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch3_dither_and_shift_op, param, $cbops.dither_and_shift.SHIFT_AMOUNT_FIELD, r0)
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch4_dither_and_shift_op, param, $cbops.dither_and_shift.SHIFT_AMOUNT_FIELD, r0)
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain0_ch5_dither_and_shift_op, param, $cbops.dither_and_shift.SHIFT_AMOUNT_FIELD, r0)

   // Configure output (shift) scaling for chain1
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain1_ch0_dither_and_shift_op, param, $cbops.dither_and_shift.SHIFT_AMOUNT_FIELD, r0)
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain1_ch1_dither_and_shift_op, param, $cbops.dither_and_shift.SHIFT_AMOUNT_FIELD, r0)

   // Configure rate adjustment and shift output (shift) scaling for chain1
   TEMPLATE_OP_CONFIG($M.multi_chan_output.chain1_sync_rate_adjustment_and_shift, param, $cbops.rate_adjustment_and_shift.SHIFT_AMOUNT_FIELD, r0)

   call $unblock_interrupts;

   jump $pop_rLink_and_rts;

.ENDMODULE;
