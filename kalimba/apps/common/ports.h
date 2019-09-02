// *****************************************************************************
// Copyright (c) 2003 - 2015 Qualcomm Technologies International, Ltd.
// Part of ADK 4.1
//
// *****************************************************************************

#ifndef PORTS_HEADER_INCLUDED
#define PORTS_HEADER_INCLUDED

// The following constants define the common port number usage. These should be used by the
// application in conjunction with cbuffer masks (e.g. $cbuffer.WRITE_PORT_MASK etc. to
// control the port usage).

// Output port NUMBER definitions
.CONST $CODEC_ESCO_OUT_PORT_NUMBER                              0; // ESCO output (shared with sub esco output)
.CONST $SUB_ESCO_OUT_PORT_NUMBER                                0; // Subwoofer ESCO output (low latency)
.CONST $SUB_L2CAP_OUT_PORT_NUMBER                               1; // Subwoofer L2CAP output (standard latency)
.CONST $RELAY_L2CAP_OUT_PORT_NUMBER                             2; // Relay, TWS output port
.CONST $USB_OUT_PORT_NUMBER                                     3; // USB output port
// Multi-channel output channels
.CONST $SUB_WIRED_OUT_PORT_NUMBER                               4; // Wired subwoofer output
.CONST $PRIMARY_LEFT_OUT_PORT_NUMBER                            5; // ...other output channels
.CONST $PRIMARY_RIGHT_OUT_PORT_NUMBER                           6; //
.CONST $SECONDARY_LEFT_OUT_PORT_NUMBER                          7; //
.CONST $SECONDARY_RIGHT_OUT_PORT_NUMBER                         8; //
.CONST $AUX_LEFT_OUT_PORT_NUMBER                                9; //
.CONST $AUX_RIGHT_OUT_PORT_NUMBER                               10; //

#endif