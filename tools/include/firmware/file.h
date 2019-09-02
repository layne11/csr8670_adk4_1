/*
 * Copyright (c) 2016 Qualcomm Technologies International, Ltd.
 * This file was automatically generated for firmware version 19.5
 */

#ifndef __FILE_H

#define __FILE_H

#include <csrtypes.h>
/*! @file file.h @brief Access to the read-only file-system */
/*! @sa #StreamFileSource, #KalimbaLoad */
#include <csrtypes.h>
#include <app/file/file_if.h>


/*!
  @brief Find the index for a named file or directory, relative to the starting directory.

  @param start index of directory in which to start the search
  @param name the name (possibly including a path) of the item to find
  @param length the number of characters in @a name

  @return The index of the item searched for, or #FILE_NONE if no such item exists.

  Leading and trailing directory separators in @a name are ignored.

  @a start is commonly #FILE_ROOT.
*/
FILE_INDEX FileFind(FILE_INDEX start, const char *name, uint16 length);

/*!
  @brief Find the type of a file specified by its index

  @param index the index of the file whose type is required

  @return The type of the specified file.
*/
FILE_TYPE FileType(FILE_INDEX index);

/*!
  @brief Find the index of the directory containing this file or directory.
  @param item The index of the item we know about.

  @return The index of the directory containing item, or #FILE_NONE.
*/
FILE_INDEX FileParent(FILE_INDEX item);

#endif
