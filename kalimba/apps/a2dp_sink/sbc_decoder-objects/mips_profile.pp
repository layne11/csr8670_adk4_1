.linefile 1 "mips_profile.asm"
.linefile 1 "<command-line>"
.linefile 1 "mips_profile.asm"
.linefile 47 "mips_profile.asm"
.linefile 1 "mips_profile.h" 1
.linefile 10 "mips_profile.h"
.CONST $mips_profile.MIPS.STAT_OFFSET 0;
.CONST $mips_profile.MIPS.TMAIN_OFFSET 1;
.CONST $mips_profile.MIPS.SMAIN_OFFSET 2;
.CONST $mips_profile.MIPS.TINT_OFFSET 3;
.CONST $mips_profile.MIPS.SINT_OFFSET 4;
.CONST $mips_profile.MIPS.SMAIN_INT_OFFSET 5;
.CONST $mips_profile.MIPS.MAIN_CYCLES_OFFSET 6;
.CONST $mips_profile.MIPS.INT_CYCLES_OFFSET 7;
.CONST $mips_profile.MIPS.TOT_CYCLES_OFFSET 8;
.CONST $mips_profile.MIPS.TEVAL_OFFSET 9;
.CONST $mips_profile.MIPS.BLOCK_SIZE 10;
.linefile 48 "mips_profile.asm" 2

.MODULE $M.mips_profile;
   .CODESEGMENT MIPS_PROFILE_PM;
   .DATASEGMENT DM;

   .VAR $DecoderMips_data_block[$mips_profile.MIPS.BLOCK_SIZE] =
     0,
     0,
     0,
     0,
     0,
     0,
     0,
     0,
     0,
     0;

   .VAR $FunctionMips_data_block[$mips_profile.MIPS.BLOCK_SIZE] =
     0,
     0,
     0,
     0,
     0,
     0,
     0,
     0,
     0,
     0;

   .VAR evalinterval_us = 100000;

mainstart:


   r0 = M[$NUM_RUN_CLKS_LS];
   M[r8 +$mips_profile.MIPS.TMAIN_OFFSET] = r0;


   M[r8 + $mips_profile.MIPS.SMAIN_INT_OFFSET] = 0;


   r0 = M[r8 + $mips_profile.MIPS.STAT_OFFSET];
   if Z jump init;

   rts;


init:

   r0 = M[$TIMER_TIME];
   M[r8 + $mips_profile.MIPS.TEVAL_OFFSET] = r0;

   r0 = 1;
   M[r8 + $mips_profile.MIPS.STAT_OFFSET] = r0;
   M[r8 + $mips_profile.MIPS.SMAIN_OFFSET] = 0;
   M[r8 + $mips_profile.MIPS.SINT_OFFSET] = 0;

   rts;


mainend:


   r0 = M[r8 + $mips_profile.MIPS.STAT_OFFSET];
   if Z rts;

   r0 = M[$NUM_RUN_CLKS_LS];
   r1 = M[r8 + $mips_profile.MIPS.TMAIN_OFFSET];
   r0 = r0 - r1;

   r1 = M[r8 + $mips_profile.MIPS.SMAIN_INT_OFFSET];
   r0 = r0 - r1;

   r1 = M[r8 + $mips_profile.MIPS.SMAIN_OFFSET];
   r0 = r0 + r1;
   M[r8 + $mips_profile.MIPS.SMAIN_OFFSET] = r0;


   r0 = M[$TIMER_TIME];
   r1 = M[r8 + $mips_profile.MIPS.TEVAL_OFFSET];

   r0 = r0 - r1;
   r1 = M[evalinterval_us];
   Null = r0 - r1;

   if NEG rts;


   r0 = M[r8 + $mips_profile.MIPS.SMAIN_OFFSET];
   M[r8 + $mips_profile.MIPS.MAIN_CYCLES_OFFSET] = r0;
   r1 = M[r8 + $mips_profile.MIPS.SINT_OFFSET];
   M[r8 + $mips_profile.MIPS.INT_CYCLES_OFFSET] = r1;
   r0 = r0 + r1;
   M[r8 + $mips_profile.MIPS.TOT_CYCLES_OFFSET] = r0;


   M[r8 + $mips_profile.MIPS.STAT_OFFSET] = 0;
   rts;


intstart:
   r0 = M[$NUM_RUN_CLKS_LS];
   M[r8 + $mips_profile.MIPS.TINT_OFFSET] = r0;
   rts;


intend:
   r0 = M[$NUM_RUN_CLKS_LS];

   r1 = M[r8 + $mips_profile.MIPS.TINT_OFFSET];
   r0 = r0 - r1;

   r1 = M[r8 + $mips_profile.MIPS.SINT_OFFSET];
   r1 = r0 + r1;
   M[r8 + $mips_profile.MIPS.SINT_OFFSET] = r1;

   r1 = M[r8 + $mips_profile.MIPS.SMAIN_INT_OFFSET];
   r1 = r0 + r1;
   M[r8 + $mips_profile.MIPS.SMAIN_INT_OFFSET] = r1;

   rts;


.ENDMODULE;
.linefile 183 "mips_profile.asm"
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

   r0 = 1;
   M[r2] = r0;


   r1 = M[$TIMER_TIME];
   r4 = M[$interrupt.total_time];

   r6 = M[$CLOCK_DIVIDE_RATE];

   r0 = $frame_sync.MAX_CLK_DIV_RATE;
   M[$CLOCK_DIVIDE_RATE] = r0;


jp_wait:
   Null = M[r2];
   if NZ jump jp_wait;


   M[$CLOCK_DIVIDE_RATE] = r6;


   r3 = M[$TIMER_TIME];
   r1 = r3 - r1;
   r4 = r4 - M[$interrupt.total_time];
   r1 = r1 + r4;
   r0 = M[&TotalTime];
   r1 = r1 + r0;
   M[&TotalTime]=r1;


   r0 = M[LastUpdateTm];
   r5 = r3 - r0;
   rMAC = 1000000;
   NULL = r5 - rMAC;
   if NEG rts;


   rMAC = rMAC ASHIFT -1;
   Div = rMAC/r5;

   rMAC = r5 - r1;

   M[LastUpdateTm]=r3;

   M[&TotalTime]=NULL;

   r3 = DivResult;
   rMAC = r3 * rMAC (frac);



   r3 = 0.008;
   rMAC = rMAC * r3 (frac);
   M[Mips]=rMAC;
   rts;

.ENDMODULE;
