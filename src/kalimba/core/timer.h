// *****************************************************************************
// Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
//
// *****************************************************************************


#ifndef TIMER_HEADER_INCLUDED
#define TIMER_HEADER_INCLUDED

   // set the maximum possible number of handlers - this is only used to detect
   // corruption in the linked list, and so can be quite large
   .CONST   $timer.MAX_TIMER_HANDLERS        50;

   .CONST   $timer.LAST_ENTRY                -1;

   .CONST   $timer.NEXT_ADDR_FIELD           0;
   .CONST   $timer.TIME_FIELD                1;
   .CONST   $timer.HANDLER_ADDR_FIELD        2;
   .CONST   $timer.ID_FIELD                  3;
   .CONST   $timer.STRUC_SIZE                4;

   .CONST   $timer.n_us_delay.SHORT_DELAY    10;
   .CONST   $timer.n_us_delay.MEDIUM_DELAY   150;

#endif
