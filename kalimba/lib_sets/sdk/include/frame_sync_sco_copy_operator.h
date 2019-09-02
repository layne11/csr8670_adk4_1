// *****************************************************************************
// Copyright (c) 2007 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
//
// *****************************************************************************

#ifndef FRAME_SYNC_SCO_COPY_OP_HEADER_INCLUDED
#define FRAME_SYNC_SCO_COPY_OP_HEADER_INCLUDED

   // Below must match shift copy for compatibity
   .CONST   $frame_sync.sco_copy_op.INPUT_START_INDEX_FIELD             0;
   .CONST   $frame_sync.sco_copy_op.OUTPUT_START_INDEX_FIELD            1;
   .CONST   $frame_sync.sco_copy_op.SHIFT_AMOUNT_FIELD                  2;
   // Extensions for sco copy
   .CONST   $frame_sync.sco_copy_op.COPY_LIMIT_FIELD                    3;

   .CONST   $frame_sync.sco_copy_op.STRUC_SIZE                          4;


#endif // FRAME_SYNC_SCO_COPY_OP_HEADER_INCLUDED
