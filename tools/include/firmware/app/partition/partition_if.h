/****************************************************************************

        Copyright (c) 2012 - 2016 Qualcomm Technologies International, Ltd.
        All Rights Reserved.
        Qualcomm Technologies International, Ltd. Confidential and Proprietary.

FILE
    partition_if.h

CONTAINS
    Definitions for the mounting and writing to file systems.

DESCRIPTION
    This file is seen by the stack, and VM applications, and
    contains things that are common between them.
*/

/*!
 @file partition_if.h
 @brief Parameters for StreamPartitionOverwriteSink() and
        PartitionMountFilesystem().

*/

#ifndef __PARTITION_IF_H__
#define __PARTITION_IF_H__

/*! @brief devices that can contain a file system */
typedef enum 
{
    PARTITION_INTERNAL_MEMORY = 0, /*!< Internal flash or ROM */
    PARTITION_SERIAL_FLASH    = 1  /*!< SPI or QSPI flash (SQIF) */
} partition_filesystem_devices;

/*! @brief where to mount in union file system */
typedef enum
{
    PARTITION_HIGHER_PRIORITY = 0, /*!< mount before existing file systems */
    PARTITION_LOWER_PRIORITY  = 1  /*!< mount after existing file systems */
} partition_filesystem_priority;

/*! @brief Keys used with PartitionGetInfo() */
typedef enum
{
    PARTITION_INFO_IS_MOUNTED = 0,  /*!< whether partition is mounted */
    /*!< size of partition in 16 bit (words).
     * In case partition is of type PARTITION_TYPE_RAW_SERIAL,
     * size of partition excludes raw header size (4 words).
     */
    PARTITION_INFO_SIZE       = 1,
    PARTITION_INFO_TYPE       = 2,  /*!< type of partition */
    PARTITION_INFO_STATE      = 3   /*!< state of partition */
} partition_info_key;

/*! @brief Keys used with PartitionSetMessageDigest() */
typedef enum
{
    PARTITION_MESSAGE_DIGEST_APP_SIGNATURE = 1, /*!< signed with app key */
    PARTITION_MESSAGE_DIGEST_CRC           = 2, /*!< file system CRC */
    PARTITION_MESSAGE_DIGEST_SKIP          = 3  /*!< do not perform verification */
} partition_message_digest_type;

/*! @brief result of set digest, passed in #MessageStreamSetDigest */
typedef enum
{
    PARTITION_SET_DIGEST_PASS            = 0, /*!< passed set digest */
    PARTITION_SET_DIGEST_INVALID_LEN     = 1, /*!< invalid digest length */
    PARTITION_SET_DIGEST_NO_MEMORY       = 2, /*!< not enough memory for digest */
    PARTITION_SET_DIGEST_INVALID_MD_TYPE = 3  /*!< invalid MD type */
} partition_set_digest_result;

/*! @brief result of verification, passed in #MessageStreamPartitionVerify */
typedef enum
{
    PARTITION_VERIFY_PASS             = 0, /*!< passed verification */
    PARTITION_VERIFY_CRC_FAILED       = 1, /*!< failed CRC verification */
    PARTITION_VERIFY_SIGNATURE_FAILED = 2, /*!< failed signature verification */
    PARTITION_VERIFY_OVERFLOW         = 3  /*!< partition overflow */

} partition_verify_result;

/*! @brief partition types, values from PartitionGetInfo(),
 *  when key is #PARTITION_INFO_TYPE
 */
typedef enum
{
    PARTITION_TYPE_UNUSED     = 0,   /*!< partition type not assigned */
    PARTITION_TYPE_FILESYSTEM = 1,   /*!< Read Only Filesystem */
    PARTITION_TYPE_PS         = 2,   /*!< Persistent Store */
    PARTITION_TYPE_RAW_SERIAL = 3    /*!< Rewriteable serial data */
}  partition_type;

/*! @brief partition state, values from PartitionGetInfo(),
 *  when key is #PARTITION_INFO_STATE
 */
typedef enum
{
    /*!< partition is closed and contents are valid */
    PARTITION_STATE_VALID      = 0,
    /*!< partition is either empty or incomplete, can be resumed */
    PARTITION_STATE_INCOMPLETE = 1,
    /*!< partition contents are invalid, will need to be overwritten */
    PARTITION_STATE_INVALID    = 2 
} partition_state;

#endif /* __PARTITION_IF_H__ */
