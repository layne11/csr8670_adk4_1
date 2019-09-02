/****************************************************************************
Copyright (c) 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.1

FILE NAME
    upgrade_partition_data_priv.h
    
DESCRIPTION
    Definition of partition data processing state datatypes.
*/

#ifndef UPGRADE_PARTITION_DATA_PRIV_H_
#define UPGRADE_PARTITION_DATA_PRIV_H_

#include "upgrade_fw_if.h"

#define UPGRADE_PARTITION_DATA_BLOCK_SIZE 48

typedef enum
{
    UPGRADE_PARTITION_DATA_STATE_GENERIC_1ST_PART,
    UPGRADE_PARTITION_DATA_STATE_HEADER,
    UPGRADE_PARTITION_DATA_STATE_DATA_HEADER,
    UPGRADE_PARTITION_DATA_STATE_DATA,
    UPGRADE_PARTITION_DATA_STATE_FOOTER
} UpgradePartitionDataState;

typedef struct {
    bool inProgress;
    uint32 size;
    uint8 data[UPGRADE_PARTITION_DATA_BLOCK_SIZE];
} UpgradePartitionDataIncompleteData;

typedef struct {
    uint32 nextReqSize;
    uint32 nextOffset;
    uint32 bigReqSize;
    uint32 partitionLength;
    UpgradePartitionDataState state;
    UpgradeFWIFPartitionHdl partitionHdl;
    uint8 *signature;
    uint16 signatureReceived;
    UpgradePartitionDataIncompleteData incompleteData;
    bool openNextPartition;
} UpgradePartitionDataCtx;

#endif /* UPGRADE_PARTITION_DATA_PRIV_H_ */
