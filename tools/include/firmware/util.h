/*
 * Copyright (c) 2016 Qualcomm Technologies International, Ltd.
 * This file was automatically generated for firmware version 19.5
 */

#ifndef __UTIL_H

#define __UTIL_H

#include <csrtypes.h>
/*! @file util.h @brief Utility routines
**
**
These routines perform a number of frequently required tasks. They will execute at a greater speed than
similar routines written in VM application code.
*/


/*! 
  @brief Compare two blocks of memory of extent 'size', as uint16's. 

  @param a First memory block to compare.
  @param b Second memory block to compare.
  @param size Size of memory blocks to compare.
  @return > 0 if 'a' is lexicographically greater than 'b',
          < 0 if 'a' is lexicographically less than 'b',
            0 if 'a' and 'b' have identical contents.
*/
int UtilCompare(const uint16 *a, const uint16 *b, size_t size);

/*!
  @brief Compares two memory blocks.

  @param mask The bitmask to use.
  @param value The value to look for.
  @param data_start The memory location of the start of the table.
  @param offset The offset into each table entry that the search will be performed at.
  @param size The size of each table entry.
  @param count The number of entries in the table.

  Conceptually we have a table in memory starting at 'data_start', with 'count' entries where each entry is 'size'.
  UtilFind searches for 'value', at 'offset' from the start of each entry, using bitmask 'mask'.

  @return VM address of the table entry containing 'value' if found, else 0.
*/
const uint16 *UtilFind(uint16 mask, uint16 value, const uint16 *data_start, uint16 offset, uint16 size, uint16 count);

/*! 
  @brief Converts a string into a number. 

  @param start The start of the string to convert.
  @param end The 'end' of the string to convert.
  @param result Will contain the resulting number.
  @return A pointer into the string (the first character after
  the number) unless no number was found, in which case 0 is returned.

  Note that the string converted here will not include the character pointed to by 'end'.
  That is, to convert the string "123" you would need to call 'UtilGetNumber(start, start+3, &result)'.

  The number is expected to be an unsigned decimal in the range 0 to 2^16-1.
*/
const uint8 *UtilGetNumber(const uint8 *start, const uint8 *end, uint16 *result);

/*!
  @brief Compute a uint16 hash value for the memory at 'data', of extent 'size'
  uint16's, starting with the given 'seed'.

  @param data The start of the memory block to hash. 
  @param size The size of the memory block to hash.
  @param seed The seed value to use for the hash.
*/
uint16 UtilHash(const uint16 *data, uint16 size, uint16 seed);

/*!
  @brief Returns a 16-bit random number. 

  Uses a numerical approach, but the state is shared with the BlueCore firmware 
  which also makes calls into this function so predictability will be low.
*/
uint16 UtilRandom(void);

/*!
  @brief Exchanges the high and low bytes of 'size' words at 'data'.

  @param data The memory location to begin swapping from.
  @param size The number of swaps to perform.
*/
void UtilSwap(uint16 *data, uint16 size);

#endif
