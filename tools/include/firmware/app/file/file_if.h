/****************************************************************************

        Copyright (c) 2002 - 2016 Qualcomm Technologies International, Ltd.
        All Rights Reserved.
        Qualcomm Technologies International, Ltd. Confidential and Proprietary.

FILE
        file_if.h  -  FILE interface

CONTAINS
        The GLOBAL definitions for the FILE subsystem from the VM

DESCRIPTION
    This file is seen by the stack, and VM applications, and
    contains things that are common between them.
*/

/*!
 @file file_if.h 
 @brief File system types
*/

#ifndef __APP_FILE_IF_H__
#define __APP_FILE_IF_H__

typedef uint16 FILE_INDEX;  /*!< File index type. */

#define FILE_NONE 0 /*!< No such file. */
#define FILE_ROOT 1 /*!< Root directory. */

/*!
    @brief The type of file.
*/
typedef enum
{
    FILE_TYPE_ERROR,           /*!< An error has occured.*/
    FILE_TYPE_DIRECTORY,       /*!< This is a directory within the filesystem.*/
    FILE_TYPE_NARROW_FILE,     /*!< A narrow (8 bit wide) file.*/
    FILE_TYPE_WIDE_FILE        /*!< A wide (16 bit wide) file.*/
} FILE_TYPE;

#endif
