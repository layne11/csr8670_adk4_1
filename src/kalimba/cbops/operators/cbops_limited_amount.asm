// *****************************************************************************
// Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
//
// *****************************************************************************

// *****************************************************************************
// NAME:
//    Limited Amount To Use operator
//
// DESCRIPTION:
//    The CBOPS library copies the minimum of the amount of data in the input
// buffers and the amount of space in the output buffers. However this may not
// always be desireable. 
// This operator waits until there is a fixed amount of data to be transferred
// or a specified number of timer interrupts have elapsed since the last data 
// transfer. 
//
// NOTE:
//    To ensure that this operator is useful it should be at the end of the
// chain of operators.
//
// *****************************************************************************

#include "stack.h"
#include "cbops.h"

.MODULE $M.cbops.limited_amount;
   .DATASEGMENT DM;

   // ** function vector **
   .VAR $cbops.limited_amount[$cbops.function_vector.STRUC_SIZE] =
      $cbops.function_vector.NO_FUNCTION,   // reset function
      &$cbops.limited_amount.amount_to_use,   // amount to use function
      $cbops.function_vector.NO_FUNCTION;   // main function

.ENDMODULE;


// *****************************************************************************
// MODULE:
//   $cbops.limited_amount.amount_to_use
//
// DESCRIPTION:
//    Operator to limit the amount of data copied in each call, NOTE it does not
// actually copy any data, it simply limits the amount of data other operators
// will copy.
//
// This operator will force the copy operator to copy only if a certain amount
// of data is able to be copied, or if a certain amount of time has passed 
// since the last sucessful copy operation. These two parameters are specified
// in the operator structure. 
//
// INPUTS:
//    - r5 = the minimum of the number of input samples available and the
//      amount of output space available
//    - r6 = the number of input samples available
//    - r7 = the amount of output space available
//    - r8 = pointer to operator structure
//
//
// OUTPUTS:
//    - r5 = the number of samples to process
//
// TRASHED REGISTERS:
//    r0
//
// *****************************************************************************
.MODULE $M.cbops.limited_amount.amount_to_use;
   .CODESEGMENT CBOPS_LIMITED_AMOUNT_PM;
   .DATASEGMENT DM;


   // ** amount to use function **
   $cbops.limited_amount.amount_to_use:

   // if no amount then exit
   r0 = M[r8 + $cbops.limited_amount.AMOUNT_FIELD];
   Null = r0 - $cbops.limited_amount.NO_AMOUNT;
   if Z rts;

   // if r5 >= amount then set r5 to amount
   Null = r5 - r0;
   if LT jump small_r5;
   r5 = r0;
   M[r8 + $cbops.limited_amount.FLUSH_COUNTER_FIELD] = NULL;
   rts;

   small_r5:
   
   // increment flush counter
   r0 = M[r8 + $cbops.limited_amount.FLUSH_COUNTER_FIELD];
   r0 = r0 + 1;
   M[r8 + $cbops.limited_amount.FLUSH_COUNTER_FIELD] = r0;
   // if flush counter > threshold, transfer as many samples as possible.
   r1 = M[r8 + $cbops.limited_amount.FLUSH_THRESHOLD_FIELD];
   null = r0 - r1;
   if LT r5 = 0; // r5 < amount so zero amount to use
   null = r5;   
   if z jump done; 
    M[r8 + $cbops.limited_amount.FLUSH_COUNTER_FIELD] = NULL;
   done:
 
   rts;

.ENDMODULE;


