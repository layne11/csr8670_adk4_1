.linefile 1 "util.asm"
.linefile 1 "<command-line>"
.linefile 1 "util.asm"
.linefile 40 "util.asm"
.MODULE $M.lookup_2_in_1_out;
   .CODESEGMENT LOOKUP_2_IN_1_OUT_PM;

   .CONST record_size 3;

   $lookup_2_in_1_out:

   r4 = 1;


   lookup_loop:

      r0 = M[r1];
      if Z jump rates_not_supported;

      null = r0 - r2;
      if NZ jump skip;

      r0 = M[r1+1];
      null = r0 - r3;
      if Z jump match_found;

      skip:
      r1 = r1 + record_size;

   jump lookup_loop;

   match_found:
   r4 = 0;

   rates_not_supported:
   r3 = r4;

   r0 = M[r1+2];

   rts;

.ENDMODULE;
