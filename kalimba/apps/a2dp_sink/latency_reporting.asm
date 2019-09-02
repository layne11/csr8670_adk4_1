// *****************************************************************************
// Copyright (c) 2008 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
//
// *****************************************************************************

#ifndef MEDIA_PACKET_BOUNDARY_DETECTION_INCLUDED
#define MEDIA_PACKET_BOUNDARY_DETECTION_INCLUDED

#include "core_library.h"
#include "frame_sync_stream_macros.h"

#define $encoded_latency.CODEC_PACKETS_INFO_CBUFFER_SIZE      50
#define $encoded_latency.CODEC_PACKETS_INFO_WRITES_PER_ENTRY   2
#define $encoded_latency.MAX_FILTER_LEN                        64
#define $encoded_latency.SMOOTHING_FACTOR_LOW                  0.2
#define $encoded_latency.SMOOTHING_FACTOR_HIGH                 0.6

.CONST $VMMSG.CONFIGURE_LATENCY_REPORTING                 0x1027;

// *****************************************************************************
// MODULE:
//    $M.$media_packet_boundary_detection
//
// DESCRIPTION:
//    Calculate over-the-air media packet boundaries and writes
//    boundary information to circular buffer.
//
// INPUTS:
//    - none
//
//
// OUTPUTS:
//    - none
//
// TRASHED REGISTERS:
//    r0, r1, r2, r3, r4
//
// *****************************************************************************

.MODULE $M.media_packet_boundary_detection;
   .CODESEGMENT MEDIA_PACKET_BOUNDARY_DETECTION_PM;
   .DATASEGMENT DM;
   
   DeclareCBuffer($codec_packets_info_cbuffer_struc,$codec_packets_info,$encoded_latency.CODEC_PACKETS_INFO_CBUFFER_SIZE);
  
   .VAR $codec_packets_info_last_write_address = &$codec_in;
   .VAR $codec_packets_info_last_timer_time = 0;
   .VAR $codec_packets_info_timer_gap = 5000; // 5 ms - Could consider using a counter instead of time.  Also could consider using more logic to determine packet boundaries - empirical observation required.
   .VAR $first_packet_received;
   .VAR $first_packet_time;
   .VAR $target_latency_from_vm;

$media_packet_boundary_detection:

   // push rLink onto stack
   $push_rLink_macro;
    
 
   r0 = $codec_packets_info_cbuffer_struc;
   call $cbuffer.calc_amount_data;
   Null = r0 - ($encoded_latency.CODEC_PACKETS_INFO_CBUFFER_SIZE - $encoded_latency.CODEC_PACKETS_INFO_WRITES_PER_ENTRY);
   if Z jump done_with_latency_reporting;  // no room to store info

      r0 = $codec_in_cbuffer_struc;
      call $cbuffer.calc_amount_space;
      Null = r0;
      if Z jump done_with_latency_reporting; // don't do anything if codec buffer is too full to receive new packets

      r0 = &$codec_in_cbuffer_struc;
      call $cbuffer.get_write_address_and_size;
      r4 = M[$codec_packets_info_last_write_address];

      //    - r0 = write address
      //    - r1 = buffer size
      //    - r4 = write address before copy handler ran      
      
      r0 = r0 - r4;
      if NEG r0 = r0 + r1;
      Null = r0 - 4;
      if NEG jump done_with_latency_reporting; 
         // New data has been written to the input cbuffer
         // Is this the start of a new packet?
         // If very little time has passed this is probably a continuation of the last packet.
         // Else, it's probably a new packet.
         r2 = M[$TIMER_TIME];
         r3 = r2 - M[$codec_packets_info_last_timer_time];
         if NEG r3 = -r3;
         M[$codec_packets_info_last_timer_time] = r2;

         Null = r3 - M[$codec_packets_info_timer_gap];
         if LS jump done_with_latency_reporting;
            //  large enough gap in time that we think this is a new packet
            r0 = &$codec_in_cbuffer_struc;
            call $cbuffer.get_write_address_and_size;
            I0 = r4;
            L0 = r1;
            r4 = M[I0, 1];
            r4 = I0; // start of new packet

            r0 = $codec_packets_info_cbuffer_struc;
            call $cbuffer.get_write_address_and_size;
            I0 = r0;
            L0 = r1;
            M[I0, 1] = r4; // $Write pointer position of codec input buffer
            M[I0, 1] = r2; // $TIMER_TIME
            // Reset for linear addressing
            L0 = 0;

            // Update output buffer
            r0 = $codec_packets_info_cbuffer_struc;
            r1 = I0;
            call $cbuffer.set_write_address;
done_with_latency_reporting:

   jump $pop_rLink_and_rts;

.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.report_latency_to_vm
//
// DESCRIPTION: Sends latency to VM.
//
// INPUTS:
//    - none
//
//
// OUTPUTS:
//    - none
//
// *****************************************************************************
.MODULE $M.report_latency_to_vm;
   .CODESEGMENT REPORT_LATENCY_TO_VM_PM;
   .DATASEGMENT DM;
   .VAR smoothing_factor_low = $encoded_latency.SMOOTHING_FACTOR_LOW;
   .VAR smoothing_factor_high = $encoded_latency.SMOOTHING_FACTOR_HIGH;   
   .VAR timer_struc[$timer.STRUC_SIZE];
   .VAR reported_latency = 0;
   .VAR latency_max;
   .VAR latency_max_counter;
   
init:
   $push_rLink_macro;
   M[$M.configure_latency_reporting.average_latency] = 0;
   M[latency_max_counter] = 0;
   r2 =  M[$M.configure_latency_reporting.report_period];
   r3 = &$report_latency_to_vm;
   call $timer.schedule_event_in;
   jump $pop_rLink_and_rts;

$report_latency_to_vm:
   $push_rLink_macro;
   r0 = &$encoded_latency_struct;

   // sample the latest calculated latency value
   r3 = M[r0 + $encoded_latency.TOTAL_LATENCY_US_FIELD];

   // ignore 0 values, can happen in init/reset
   if Z  jump do_not_send_msg;

   // max latency in a block of 0.5s
   r1 = M[latency_max];
   r3 = MAX r1;
   M[latency_max] = r3;    
   r0 = M[latency_max_counter];
   r0 = r0 + 1;
   Null = r0 - $encoded_latency.MAX_FILTER_LEN;
   if POS r0 = 0;
   M[latency_max_counter] = r0;
   if NZ jump do_not_send_msg;

   // reset max for next calc
   M[latency_max] = 0;
   
   // averaging routine: alpha * instantaneous average + (1 - alpha) * average latency
   r0 = M[$M.configure_latency_reporting.average_latency];
   if LE r0 = r3;
   r8 = M[smoothing_factor_low];
   r7 = M[smoothing_factor_high];   
   r1 = r3 - r0;   
   if POS r8 = r7;
   r1 = r1 * r8 (frac);
   r3 = r0 + r1;
   M[$M.configure_latency_reporting.average_latency] = r3;

   // see if latency has changed enough to report a new value
   r4 = M[reported_latency];	
   r4 = r3 - r4; // difference between current avg. latency and last reported latency
   if NEG r4 = -r4;
   Null = r4 - M[$M.configure_latency_reporting.min_change];
   if NEG jump do_not_send_msg;

   // report latency
   M[reported_latency] = r3;	//Save reported latency for next iteration	
   r3 = r3 * 0.01 (frac);       // convert to 0.1ms unit before sending to VM
   r0 = r3 - 0x7FFF;            // limit to 15-bit in case of false measurment
   if POS r3 = r3 - r0;
   r2 = $music_example.VMMSG.LATENCY_REPORTING;
   call $message.send_short;
do_not_send_msg:
   // Post another timer event
   r1 = &timer_struc;
   r2 =  M[$M.configure_latency_reporting.report_period];
   r3 = &$report_latency_to_vm;
   call $timer.schedule_event_in_period;
   // pop rLink from stack
   jump $pop_rLink_and_rts;
.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.configure_latency_reporting
//
// DESCRIPTION:
//       Handles message from VM to configure latency reporting
//
//  INPUTS:
//     r1 = enable/disable (0 = disable / 1 = enable)
//     r2 = report period in ms (how often the VM would like to receive the
//          delay value from the DSP)
//     r3 = minimum change to report (the DSP will only send the latency if it
//          has changed by this amount).
//
// *****************************************************************************
.MODULE $M.configure_latency_reporting;
   .CODESEGMENT CONFIGURE_LATENCY_REPORTING_PM;
   .DATASEGMENT DM;

   .VAR message_struct[$message.STRUC_SIZE];
   .VAR enabled = 0;
   .VAR report_period = 10000;
   .VAR min_change = 0;
   .VAR average_latency = -1;

func:
   $push_rLink_macro;

   // Convert values to microseconds
   r3 = r3 * 1000 (int);
   M[min_change] = r3;
   M[enabled] = r1;
   if NZ jump no_cancel_needed;
   r0 = -1;
   M[average_latency] = r0; // Tells UFE not to display data
   //Cancel timer
   r1 = &$M.report_latency_to_vm.timer_struc;
   r2 = M[r1 + $timer.ID_FIELD];
   if NZ call $timer.cancel_event;
   jump done;

no_cancel_needed:   
   // start timer that reports latency to VM
   r1 = &$M.report_latency_to_vm.timer_struc;
   call $M.report_latency_to_vm.init;

done:
   jump $pop_rLink_and_rts;
   
.ENDMODULE;

#endif
