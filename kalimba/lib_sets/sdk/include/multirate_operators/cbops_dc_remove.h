// *****************************************************************************
// Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.
// %%version
//
// *****************************************************************************

#ifndef CBOPS_DC_REMOVE_HEADER_INCLUDED
#define CBOPS_DC_REMOVE_HEADER_INCLUDED

   .CONST   $cbops.dc_remove.INPUT_START_INDEX_FIELD                0;
   .CONST   $cbops.dc_remove.OUTPUT_START_INDEX_FIELD               1;
   .CONST   $cbops.dc_remove.DC_ESTIMATE_FIELD                      2;
   .CONST   $cbops.dc_remove.STRUC_SIZE                             3;

   // with fs=48KHz. Value of 0.0005 gives a 3dB point at 4Hz. (0.1dB @ 25Hz)
   // this is assumed to be acceptable for all sample rates
   .CONST   $cbops.dc_remove.FILTER_COEF                            0.0005;

#endif // CBOPS_DC_REMOVE_HEADER_INCLUDED
