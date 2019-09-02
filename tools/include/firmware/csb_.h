/*
 * Copyright (c) 2016 Qualcomm Technologies International, Ltd.
 * This file was automatically generated for firmware version 19.5
 */

#ifndef __CSB__H

#define __CSB__H

#include <csrtypes.h>
/*!
AFH data type.
An array of uint16s whose lower octets represent the AFH channels.
Each bit of lower octet represents a AFH channel.
Most significant bit(MSB) of last octet of AfhMap array represents reserved channel
and shall be set to 0.
*/
typedef uint16 AfhMap[10];

#endif
