/****************************************************************************
Copyright (c) 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.1

FILE NAME
    upgrade_ctx.c
    
DESCRIPTION
    Global state of Upgrade library.
*/

#include <print.h>

#include "upgrade_ctx.h"

UpgradeCtx *upgradeCtx;

/****************************************************************************
NAME
    UpgradeCtxGet

DESCRIPTION
    Sets a new library context
*/

void UpgradeCtxSet(UpgradeCtx *ctx)
{
    PRINT(("size of UpgradeCtx is %d\n", sizeof(*ctx)));
    PRINT(("size of UpgradePartitionDataCtx is %d of which data buffer is %d\n",
            sizeof(UpgradePartitionDataCtx), UPGRADE_PARTITION_DATA_BLOCK_SIZE));
    PRINT(("size of UPGRADE_LIB_PSKEY is %d\n", sizeof(UPGRADE_LIB_PSKEY)));
    upgradeCtx = ctx;
}

/****************************************************************************
NAME
    UpgradeCtxGet

RETURNS
    Context of upgrade library
*/
UpgradeCtx *UpgradeCtxGet(void)
{
    if(!upgradeCtx)
    {
        PRINT(("UpgradeGetCtx: You shouldn't have done that\n"));
    }

    return upgradeCtx;
}

/****************************************************************************
NAME
    UpgradeCtxSetPartitionData

DESCRIPTION
    Sets the partition data context in the upgrade context.
*/
void UpgradeCtxSetPartitionData(UpgradePartitionDataCtx *ctx)
{
    PRINT(("size of UpgradePartitionDataCtx is %d of which data buffer is %d\n",
            sizeof(UpgradePartitionDataCtx), UPGRADE_PARTITION_DATA_BLOCK_SIZE));
    upgradeCtx->partitionData = ctx;
}

/****************************************************************************
NAME
    UpgradeCtxGetPartitionData

RETURNS
    Context of upgrade partition data.
    Note that it may be NULL if it has not been set yet.
*/
UpgradePartitionDataCtx *UpgradeCtxGetPartitionData(void)
{
    return upgradeCtx->partitionData;
}

UpgradeFWIFCtx *UpgradeCtxGetFW(void)
{
    return &upgradeCtx->fwCtx;
}

UPGRADE_LIB_PSKEY *UpgradeCtxGetPSKeys(void)
{
    return &upgradeCtx->UpgradePSKeys;
}
