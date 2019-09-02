// *****************************************************************************
// Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
//
// *****************************************************************************


#ifndef FLASH_HEADER_INCLUDED
#define FLASH_HEADER_INCLUDED

   #ifdef DEBUG_ON
      #define FLASH_GET_FILE_ADDRESS_DEBUG_ON
   #endif


      .CONST $PM_FLASHWIN_SIZE_MAX 0x40000;


   // set the maximum possible number of handlers - this is only used to detect
   // corruption in the linked list, and so can be quite large
   .CONST   $flash.get_file_address.MAX_HANDLERS                   10;

   // structure fields
   .CONST   $flash.get_file_address.NEXT_ENTRY_FIELD               0;
   .CONST   $flash.get_file_address.FILE_ID_FIELD                  1;
   .CONST   $flash.get_file_address.HANDLER_ADDR_FIELD             2;
   .CONST   $flash.get_file_address.STRUC_SIZE                     3;

   .CONST   $flash.get_file_address.LAST_ENTRY                     -1;
   .CONST   $flash.get_file_address.REATTEMPT_TIME_PERIOD          10000;

   .CONST   $flash.get_file_address.MESSAGE_HANDLER_UNINITIALISED  -1;

 #endif
