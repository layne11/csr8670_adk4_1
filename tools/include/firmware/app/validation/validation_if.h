/****************************************************************************
    Copyright (c) 2014 - 2016 Qualcomm Technologies International, Ltd.
    All Rights Reserved.
    Qualcomm Technologies International, Ltd. Confidential and Proprietary.
FILE
    validation_if.h

CONTAINS
    Status code of the validation of executable FS partition
    and validation result of executable FS

DESCRIPTION
    This file is seen by the stack, and VM applications, and
    contains things that are common between them.

*/


#ifndef __VALIDATION_IF_H__
#define __VALIDATION_IF_H__

/*! @brief Status codes returned by ValidationInitiateExecutablePartition trap */
typedef enum
{
    /*! Bootup validation is running */
    VALIDATE_BOOTUP_VALIDATION_RUNNING = 0x0,
    /*! Previous partition validation running */
    VALIDATE_PARTITION_VALIDATION_RUNNING = 0x1,
    /*! Partition is not a filesystem partition */
    VALIDATE_PARTITION_TYPE_NOT_FS = 0x2,
    /*! File system is not an executable file-system */
    VALIDATE_FS_NOT_EXECUTABLE = 0x3,
    /*! Partition number mentioned has an executable file-system and the
        validation process is started */
    VALIDATE_PARTITION_VALIDATION_TRIGGERED = 0x4
} validation_init_status;

/*! @brief Validation result of an executable FS */
typedef enum
{
    /*! Partition validation is successful */
    VALIDATE_PARTITION_PASS = 0x0,
    /*! Partition validation is not successful */
    VALIDATE_PARTITION_FAIL = 0x1
} validation_result;

#endif /* __VALIDATION_IF_H__ */
