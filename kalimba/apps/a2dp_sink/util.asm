// *****************************************************************************
// Copyright (c) 2015 - 2016 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
//
// *****************************************************************************

// *****************************************************************************
// MODULE:
//    $lookup_2_in_1_out
//
// DESCRIPTION:
//    Function to perform a table lookup on:
//       two input values
//    to deliver:
//       one output value
//
//    e.g.
//              | Input 0     Input 1     Output 0
//    --------------------------------------------
//    record 0: | val(0,0),   val(1,0),   out    0
//    record 1: | val(0,1),   val(1,1),   out    1
//    ..        | ..          ..          ..
//    record n: | val(0,n),   val(1,n),   out    n
//                0
//
// INPUTS:
//    r1 = address of lookup table (3 column per entry terminated by 0)
//    r2 = value 0
//    r3 = value 1
//
// OUTPUTS:
//    r0 = output lookup value for matching record
//    r1 = pointer to matching record
//    r3 = 0: match found, 1: not found
//
// TRASHED:
//    r0, r1, r4
//
// *****************************************************************************
.MODULE $M.lookup_2_in_1_out;
   .CODESEGMENT LOOKUP_2_IN_1_OUT_PM;

   .CONST record_size 3;

   $lookup_2_in_1_out:

   r4 = 1;                                            // Default flag to match failure

   // Loop over table entries
   lookup_loop:

      r0 = M[r1];                                     // Get column 0 value
      if Z jump rates_not_supported;                  // End of table?

      null = r0 - r2;                                 // column 0 == value 0?
      if NZ jump skip;                                // No - skip entry

      r0 = M[r1+1];                                   // column 1 == value 1?
      null = r0 - r3;
      if Z jump match_found;                          // Yes - match found

      skip:
      r1 = r1 + record_size;                          // Next record

   jump lookup_loop;

   match_found:
   r4 = 0;                                            // Flag success...

   rates_not_supported:                               // ... otherwise use default for failure
   r3 = r4;
   
   r0 = M[r1+2];                                      // Get the output value corresponding to the match

   rts;

.ENDMODULE;
