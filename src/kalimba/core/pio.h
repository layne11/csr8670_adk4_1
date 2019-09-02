// *****************************************************************************
// Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
//
// *****************************************************************************


#ifndef PIO_HEADER_INCLUDED
#define PIO_HEADER_INCLUDED

   #ifdef DEBUG_ON
      #define PIO_DEBUG_ON
   #endif

   // pio event handler structure fields
   .CONST   $pio.NEXT_ADDR_FIELD            0;
   .CONST   $pio.PIO_BITMASK_FIELD          1;
   .CONST   $pio.HANDLER_ADDR_FIELD         2;
   .CONST   $pio.STRUC_SIZE                 3;

   // set the maximum possible number of handlers - this is only used to detect
   // corruption in the linked list, and so can be quite large
   .CONST   $pio.MAX_HANDLERS               20;

   .CONST   $pio.LAST_ENTRY                 -1;

#endif

