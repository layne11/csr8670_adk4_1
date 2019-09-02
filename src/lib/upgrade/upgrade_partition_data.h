/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.1

FILE NAME
    upgrade_partition_data.h

DESCRIPTION
    Upgrade file processing module.
    It parses headers, write partition data to the SQIF.
    As well as verifies OEM signature.
*/
#ifndef UPGRADE_PARTITION_DATA_H_
#define UPGRADE_PARTITION_DATA_H_

#include <csrtypes.h>

#include "upgrade_msg_host.h"

/*!
    @brief State of data transfer after last segment received
*/
typedef enum {
    UPGRADE_PARTITION_DATA_XFER_ERROR,
    UPGRADE_PARTITION_DATA_XFER_COMPLETE,
    UPGRADE_PARTITION_DATA_XFER_IN_PROGRESS
    } UpgradePartitionDataPartialTransferState;


/*!
    @brief Initialisation of PartitionData module.
    
    @return TRUE if ok, FALSE if out of memory.
*/
bool UpgradePartitionDataInit(void);

/*!
    @brief Free memory allocated for partition data.
*/
void UpgradePartitionDataDestroy(void);

/*!
    @brief UpgradePartitionDataGetNextReqSize

    @return Size of data request for a host.
            It will never be bigger than blockSize.
*/
uint32 UpgradePartitionDataGetNextReqSize(void);

/*!
    @brief UpgradePartitionDataGetNextOffset

    @return Offset, from current position in a file, from which data should be retrieved.
*/
uint32 UpgradePartitionDataGetNextOffset(void);

/*!
    @brief UpgradePartitionDataParse

    @param data Part of file conforming to upgrade file format.
    @param len  Size of data. It may be less than requested but
                it can't be more.

    @return The upgrade library error code
*/
UpgradeHostErrorCode UpgradePartitionDataParse(uint8 *data, uint16 len);


/*!
    @brief Handle an incoming data packet and move to internal buffer

    @note NOT NORMALLY CALLED DIRECTLY. Use UpgradePartitionDataParse()
    which handles the data and processes if complete.

    @param data Part of file conforming to upgrade file format.
    @param len  Size of data. It may be less than requested but
                it can't be more.

    @return Status of the transfer
*/
UpgradePartitionDataPartialTransferState UpgradePartitionDataParseDataCopy(uint8 *data, uint16 len);


/*!
    @brief Notify the data parser to stop future data handling.

    Causes @ref UpgradePartitionDataParseDataCopy to stop handling incoming 
    data and report errors. The error condition should be reset on a new 
    transfer/data request initiated by the library.
*/
void UpgradePartitionDataStopData(void);


/*!
    @brief UpgradePartitionDataIsDfuUpdate

    @return TRUE if one of a partitions contains a DFU file.
*/
bool UpgradePartitionDataIsDfuUpdate(void);

/*!
    @brief UpgradePartitionDataGetDfuPartition

    @return Partition number containing DFU file.
*/
uint16 UpgradePartitionDataGetDfuPartition(void);

#endif /* UPGRADE_PARTITION_DATA_H_ */
