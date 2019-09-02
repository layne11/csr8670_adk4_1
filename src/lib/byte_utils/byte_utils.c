/****************************************************************************
Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.1

FILE NAME
    byte_utils.c
    
DESCRIPTION
    Utility functions to deal with different byte sizes on 
    XAP and the rest of the world.
*/

#include "byte_utils.h"

uint16 ByteUtilsMemCpyToStream(uint8 *dst, uint8 *src, uint16 size)
{
    uint16 i;

    for(i = 0; i < size; ++i)
    {
        if(i%2)
        {
            dst[i] = src[i/2];
        }
        else
        {
            dst[i] = src[i/2] >> 8;
        }
    }

    return size;
}

uint16 ByteUtilsMemCpyFromStream(uint8 *dst, const uint8 *src, uint16 size)
{
    uint16 i;

    for(i = 0; i < size; ++i)
    {
        ByteUtilsSet1Byte(dst, i, src[i]);
    }

    return size;
}

uint16 ByteUtilsSet1Byte(uint8 *dst, uint16 byteIndex, uint8 val)
{
    uint16 *ptr2Byte = (uint16 *)dst;

    if(byteIndex%2)
    {
        ptr2Byte[byteIndex/2] |= val;
    }
    else
    {
        ptr2Byte[byteIndex/2] = val << 8;
    }

    return 1;
}

uint16 ByteUtilsSet2Bytes(uint8 *dst, uint16 byteIndex, uint16 val)
{
    uint16 *ptr2Byte = (uint16 *)dst;

    if(byteIndex%2)
    {
        ptr2Byte[byteIndex/2] |= val >> 8;
        ptr2Byte[byteIndex/2+1] = val << 8;
    }
    else
    {
        ptr2Byte[byteIndex/2] = val;
    }

    return 2;
}

uint16 ByteUtilsSet4Bytes(uint8 *dst, uint16 byteIndex, uint32 val)
{
    byteIndex += ByteUtilsSet2Bytes(dst, byteIndex, val >> 16);
    ByteUtilsSet2Bytes(dst, byteIndex, val);

    return 4;
}

uint8 ByteUtilsGet1ByteFromStream(const uint8 *src)
{
    return src[0];
}

uint16 ByteUtilsGet2BytesFromStream(const uint8 *src)
{
    uint16 val = 0;

    val = src[1];
    val |= (uint16)src[0] << 8;

    return val;
}

uint32 ByteUtilsGet4BytesFromStream(const uint8 *src)
{
    uint32 val = 0;

    val = ((uint32)src[3] & 0xff);
    val |= ((uint32)src[2] & 0xff) << 8;
    val |= ((uint32)src[1] & 0xff) << 16;
    val |= ((uint32)src[0] & 0xff) << 24;

    return val;
}


/*uint16 ByteUtilsGet1Byte(uint8 *src, uint16 byteIndex, uint8 *val)
{
    uint16 *ptr2Byte = (uint16 *)src;

    *val = ptr2Byte[byteIndex/2];


    return 1;
}

uint16 ByteUtilsGet2Bytes(uint8 *src, uint16 byteIndex, uint16 *val)
{
    uint16 *ptr2Byte = (uint16 *)src;

    if(byteIndex%2)
    {
        *val = ptr2Byte[byteIndex/2] << 8;
        *val |= ptr2Byte[byteIndex/2 + 1] >> 8;
    }
    else
    {
        *val = ptr2Byte[byteIndex/2];
    }

    return 2;
}

uint16 ByteUtilsGet4Bytes(uint8 *src, uint16 byteIndex, uint32 *val)
{
    uint16 msb, lsb;

    byteIndex += ByteUtilsGet2Bytes(src, byteIndex, &msb);
    ByteUtilsGet2Bytes(src, byteIndex, &lsb);

    *val = (uint32)msb << 16;
    *val |= lsb;

    return 4;
}*/
