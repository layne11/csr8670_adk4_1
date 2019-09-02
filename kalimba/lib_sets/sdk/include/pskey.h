// *****************************************************************************
// Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
//
// *****************************************************************************


#ifndef PSKEY_HEADER_INCLUDED
#define PSKEY_HEADER_INCLUDED

   #ifdef DEBUG_ON
      #define PSKEY_DEBUG_ON
   #endif

   // structure fields
   .CONST   $pskey.NEXT_ENTRY_FIELD           0;
   .CONST   $pskey.KEY_NUM_FIELD              1;
   .CONST   $pskey.HANDLER_ADDR_FIELD         2;
   .CONST   $pskey.STRUC_SIZE                 3;

   // set the maximum possible number of handlers - this is only used to detect
   // corruption in the linked list, and so can be quite large
   .CONST   $pskey.MAX_HANDLERS               50;

   .CONST   $pskey.LAST_ENTRY                 -1;
   .CONST   $pskey.REATTEMPT_TIME_PERIOD      10000;

   .CONST   $pskey.FAILED_READ_LENGTH         -1;

#endif

