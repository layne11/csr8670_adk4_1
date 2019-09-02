/****************************************************************************
Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.1

FILE NAME
    byte_utils.h
    
DESCRIPTION
    Header file for byte utilities.
*/

#ifndef BYTE_UTILS_H_
#define BYTE_UTILS_H_

#include <csrtypes.h>

uint16 ByteUtilsMemCpyToStream(uint8 *dst, uint8 *src, uint16 size);
uint16 ByteUtilsMemCpyFromStream(uint8 *dst, const uint8 *src, uint16 size);

uint16 ByteUtilsSet1Byte(uint8 *dst, uint16 byteIndex, uint8 val);
uint16 ByteUtilsSet2Bytes(uint8 *dst, uint16 byteIndex, uint16 val);
uint16 ByteUtilsSet4Bytes(uint8 *dst, uint16 byteIndex, uint32 val);

uint8 ByteUtilsGet1ByteFromStream(const uint8 *src);
uint16 ByteUtilsGet2BytesFromStream(const uint8 *src);
uint32 ByteUtilsGet4BytesFromStream(const uint8 *src);

#endif /* BYTE_UTILS_H_ */
