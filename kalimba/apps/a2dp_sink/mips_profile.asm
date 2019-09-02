// *****************************************************************************
// Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
//
// *****************************************************************************

// *****************************************************************************
// MODULE:
//    $M.mips_profile
//
// INPUTS:
//    r8 - mips data block pointer
//
// OUTPUTS:
//    main_cycles :
//       num cycles used in your main function in a 100ms interval
//    int_cycles  :
//       num cycles used in your interrupt functions(s) in a 100ms interval
//    tot_cycles  :
//       total cycles used by your application in a 100ms interval
//
// TRASHED:
//    r0,r1
//
// CYCLES
//    $M.mips_profile.mainstart: 6
//    $M.mips_profile.mainend:   16
//    $M.mips_profile.intstart:  3
//    $M.mips_profile.intend:    10
//
//
// DESCRIPTION:
//    profiler. Calculate #cycles used in main and interrupt processes
//
//    MATLAB script to read MIPS:
//
//    cyc_m = kalreadval('$M.cvc_profile.main_cycles', 'uint', '24');
//    cyc_int = kalreadval('$M.cvc_profile.int_cycles', 'uint', '24');
//    cyc_tot = kalreadval('$M.cvc_profile.tot_cycles', 'uint', '24');
//
//    buf = sprintf('main MIPS\t%.2f\nint MIPS \t%.2f\ntotal MIPS\t%.2f\n',...
//       1e-5*cyc_m,1e-5*cyc_int,1e-5*cyc_tot);
//    disp(buf);
//
//
// *****************************************************************************
#include "mips_profile.h"

.MODULE $M.mips_profile;
   .CODESEGMENT MIPS_PROFILE_PM;
   .DATASEGMENT DM;

   .VAR $DecoderMips_data_block[$mips_profile.MIPS.BLOCK_SIZE] =
     0,                                 // STAT
     0,                                 // TMAIN
     0,                                 // SMAIN
     0,                                 // TINT
     0,                                 // SINT
     0,                                 // SMAIN_INT
     0,                                 // MAIN_CYCLES
     0,                                 // INT_CYCLES
     0,                                 // TOT_CYCLES
     0;                                 // TEVAL

   .VAR $FunctionMips_data_block[$mips_profile.MIPS.BLOCK_SIZE] =
     0,                                 // STAT
     0,                                 // TMAIN
     0,                                 // SMAIN
     0,                                 // TINT
     0,                                 // SINT
     0,                                 // SMAIN_INT
     0,                                 // MAIN_CYCLES
     0,                                 // INT_CYCLES
     0,                                 // TOT_CYCLES
     0;                                 // TEVAL

   .VAR evalinterval_us = 100000;

mainstart:
   // start profiling main process

   r0 = M[$NUM_RUN_CLKS_LS];
   M[r8 +$mips_profile.MIPS.TMAIN_OFFSET] = r0;


   M[r8 + $mips_profile.MIPS.SMAIN_INT_OFFSET] = 0;     // reset smain_int to interrupt cycles
                                                        // can be subtracted out from smain

   r0 = M[r8 + $mips_profile.MIPS.STAT_OFFSET];
   if Z jump init;

   rts;


init:
   // get first us timestamp
   r0 = M[$TIMER_TIME];
   M[r8 + $mips_profile.MIPS.TEVAL_OFFSET] = r0;

   r0 = 1;
   M[r8 + $mips_profile.MIPS.STAT_OFFSET] = r0;
   M[r8 + $mips_profile.MIPS.SMAIN_OFFSET] = 0;
   M[r8 + $mips_profile.MIPS.SINT_OFFSET] = 0;

   rts;


mainend:
   // stop profiling main process

   r0 = M[r8 + $mips_profile.MIPS.STAT_OFFSET];         // not initialized yet
   if Z rts;

   r0 = M[$NUM_RUN_CLKS_LS];  // calc deltat
   r1 = M[r8 + $mips_profile.MIPS.TMAIN_OFFSET];
   r0 = r0 - r1;

   r1 = M[r8 + $mips_profile.MIPS.SMAIN_INT_OFFSET];    // subtrace out interrupt cycles
   r0 = r0 - r1;

   r1 = M[r8 + $mips_profile.MIPS.SMAIN_OFFSET];        // store main cycles
   r0 = r0 + r1;
   M[r8 + $mips_profile.MIPS.SMAIN_OFFSET] = r0;


   r0 = M[$TIMER_TIME];
   r1 = M[r8 + $mips_profile.MIPS.TEVAL_OFFSET];

   r0 = r0 - r1;
   r1 = M[evalinterval_us];
   Null = r0 - r1;

   if NEG rts;

   // interval has elapsed. evaluate and reset;
   r0 = M[r8 + $mips_profile.MIPS.SMAIN_OFFSET];
   M[r8 + $mips_profile.MIPS.MAIN_CYCLES_OFFSET] = r0;
   r1 = M[r8 + $mips_profile.MIPS.SINT_OFFSET];
   M[r8 + $mips_profile.MIPS.INT_CYCLES_OFFSET] = r1;
   r0 = r0 + r1;
   M[r8 + $mips_profile.MIPS.TOT_CYCLES_OFFSET] = r0;


   M[r8 + $mips_profile.MIPS.STAT_OFFSET] = 0;          // not initislized
   rts;


intstart:
   r0 = M[$NUM_RUN_CLKS_LS];
   M[r8 + $mips_profile.MIPS.TINT_OFFSET] = r0;
   rts;


intend:
   r0 = M[$NUM_RUN_CLKS_LS];

   r1 = M[r8 + $mips_profile.MIPS.TINT_OFFSET];         // calc deltat
   r0 = r0 - r1;

   r1 = M[r8 + $mips_profile.MIPS.SINT_OFFSET];         // store sum(deltat) in sint
   r1 = r0 + r1;
   M[r8 + $mips_profile.MIPS.SINT_OFFSET] = r1;

   r1 = M[r8 + $mips_profile.MIPS.SMAIN_INT_OFFSET];    // store sum(deltat) in smain_int
   r1 = r0 + r1;
   M[r8 + $mips_profile.MIPS.SMAIN_INT_OFFSET] = r1;

   rts;


.ENDMODULE;

// *****************************************************************************
// MODULE:
//    $M.Sleep
//
// DESCRIPTION:
//    Place Processor in IDLE and compute system MIPS
//    To read total MIPS over SPI do ($M.Sleep.Mips*proc speed)/8000
//    proc speed is 80 for Gordon and 120 for Rick
//
// *****************************************************************************
.MODULE $M.Sleep;
   .CODESEGMENT PM;
   .DATASEGMENT DM;

   .VAR TotalTime=0;
   .VAR LastUpdateTm=0;
   .VAR Mips=0;
   .VAR sync_flag_esco=0;

$SystemSleepAudio:
   r2 = $frame_sync.sync_flag;
   jump SleepSetSync;

$SystemSleepEsco:
   r2 = sync_flag_esco;

SleepSetSync:
   // Set the sync flag to commence sleeping (wait for it being cleared)
   r0 = 1;
   M[r2] = r0;

   // Timer status for MIPs estimate
   r1 = M[$TIMER_TIME];
   r4 = M[$interrupt.total_time];
   // save current clock rate
   r6 = M[$CLOCK_DIVIDE_RATE];
   // go to slower clock and wait for task event
   r0 = $frame_sync.MAX_CLK_DIV_RATE;
   M[$CLOCK_DIVIDE_RATE] = r0;

   // wait in loop (delay) till sync flag is reset
jp_wait:
   Null = M[r2];
   if NZ jump jp_wait;

   // restore clock rate
   M[$CLOCK_DIVIDE_RATE] = r6;

   // r1 is total idle time
   r3 = M[$TIMER_TIME];
   r1 = r3 - r1;
   r4 = r4 - M[$interrupt.total_time];
   r1 = r1 + r4;
   r0 = M[&TotalTime];
   r1 = r1 + r0;
   M[&TotalTime]=r1;

   // Check for MIPs update
   r0 = M[LastUpdateTm];
   r5 = r3 - r0;
   rMAC = 1000000;
   NULL = r5 - rMAC;
   if NEG rts;

   // Time Period
   rMAC = rMAC ASHIFT -1;
   Div = rMAC/r5;
   // Total Processing (Time Period - Idle Time)
   rMAC = r5 - r1;
   // Last Trigger Time
   M[LastUpdateTm]=r3;
   // Reset total time count
   M[&TotalTime]=NULL;
   // MIPS
   r3  = DivResult;
   rMAC  = r3 * rMAC (frac);
   // Convert for UFE format
   // UFE uses STAT_FORMAT_MIPS - Displays (m_ulCurrent/8000.0*m_pSL->GetChipMIPS())
   // Multiply by 0.008 = 1,000,000 --> 8000 = 100% of MIPs
   r3 = 0.008;
   rMAC = rMAC * r3 (frac);  // Total MIPs Est
   M[Mips]=rMAC;
   rts;

.ENDMODULE;