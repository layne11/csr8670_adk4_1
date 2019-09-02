// *****************************************************************************
// Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
//
// *****************************************************************************


#ifndef STACK_HEADER_INCLUDED
#define STACK_HEADER_INCLUDED

// *****************************************************************************
// MODULE:
//    $stack.push_rLink_macro
//
// DESCRIPTION:
//    Macro to push rLink onto the stack
//
// INPUTS:
//    - rLink = data to be pushed onto the stack
//    - r9 = the current stack pointer
//
// OUTPUTS:
//    - r9 = the updated stack pointer
//
// TRASHED REGISTERS:
//    none
//
// *****************************************************************************

#define $push_rLink_macro                       \
         push rLink






// *****************************************************************************
// MODULE:
//    $stack.pop_rLink_macro
//
// DESCRIPTION:
//    Macro to pop rLink from the stack
//
// INPUTS:
//    - r9 = the current stack pointer
//
// OUTPUTS:
//    - r9 = the updated stack pointer
//    - rLink = data popped off the stack
//
// TRASHED REGISTERS:
//    none
//
// *****************************************************************************

#define $pop_rLink_macro                        \
         pop rLink





// *****************************************************************************
// MODULE:
//    $stack.pop_rLink_and_rts_macro
//
// DESCRIPTION:
//    Macro to pop rLink from the stack and rts
//
// INPUTS:
//    - r9 = the current stack pointer
//
// OUTPUTS:
//    - r9 = the updated stack pointer
//    - rLink = data popped off the stack
//
// TRASHED REGISTERS:
//    none
//
// *****************************************************************************

#define $pop_rLink_and_rts_macro                   \
         pop rLink;                                \
         rts





// *****************************************************************************
// MODULE:
//    $stack.push_r0_macro
//
// DESCRIPTION:
//    Macro to push r0 onto the stack
//
// INPUTS:
//    - r0 = data to be pushed onto the stack
//    - r9 = the current stack pointer
//
// OUTPUTS:
//    - r9 = the updated stack pointer
//
// TRASHED REGISTERS:
//    none
//
// *****************************************************************************

#define $push_r0_macro                          \
         push r0





// *****************************************************************************
// MODULE:
//    $stack.pop_r0_macro
//
// DESCRIPTION:
//    Macro to pop r0 from the stack
//
// INPUTS:
//    r9 = the current stack pointer
//
// OUTPUTS:
//    r9 = the updated stack pointer
//    r0 = data popped off the stack
//
// TRASHED REGISTERS:
//    none
//
// *****************************************************************************

#define $pop_r0_macro                           \
      pop r0

#endif // STACK_HEADER_INCLUDED
