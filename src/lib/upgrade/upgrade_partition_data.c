/****************************************************************************
Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.1

FILE NAME
    upgrade_partition_data.c

DESCRIPTION
    Upgrade file processing state machine.
    It is parsing and validating headers.
    All received data are passed to MD5 validation.
    Partition data are written to SQIF.

NOTES

*/

#include <string.h>
#include <stdlib.h>

#include <byte_utils.h>
#include <panic.h>
#include <print.h>

#include "upgrade_partition_data.h"
#include "upgrade_partition_data_priv.h"
#include "upgrade_ctx.h"
#include "upgrade_fw_if.h"
#include "upgrade_psstore.h"
#include "upgrade_partitions.h"

#define HEADER_FIRST_PART_SIZE 12
#define ID_FIELD_SIZE 8
#define PARTITION_SECOND_HEADER_SIZE 4
#define FIRST_WORD_SIZE 2
#define UPGRADE_HEADER_MIN_SECOND_PART_SIZE 14
#define EXPECTED_SIGNATURE_SIZE 128


static UpgradeHostErrorCode BigDataHandler(void);
static UpgradeHostErrorCode ParseCompleteData(uint8 *data, uint16 len, bool reqComplete);

static UpgradeHostErrorCode HandleGeneric1stPartState(uint8 *data, uint16 len, bool reqComplete);
static UpgradeHostErrorCode HandleHeaderState(uint8 *data, uint16 len, bool reqComplete);
static UpgradeHostErrorCode HandleDataHeaderState(uint8 *data, uint16 len, bool reqComplete);
static UpgradeHostErrorCode HandleDataState(uint8 *data, uint16 len, bool reqComplete);
static UpgradeHostErrorCode HandleFooterState(uint8 *data, uint16 len, bool reqComplete);

static void RequestData(uint32 size);

bool UpgradePartitionDataInit(void)
{
    UpgradePartitionDataCtx *ctx;

    ctx = UpgradeCtxGetPartitionData();
    if (!ctx)
    {
        ctx = malloc(sizeof(*ctx));
        if (!ctx)
        {
            PRINT(("\n"));
            return FALSE;
        }
        UpgradeCtxSetPartitionData(ctx);
    }
    memset(ctx, 0, sizeof(*ctx));
    ctx->state = UPGRADE_PARTITION_DATA_STATE_GENERIC_1ST_PART;

    RequestData(HEADER_FIRST_PART_SIZE);

    UpgradeFWIFInit();
    UpgradeFWIFValidateInit();
    
    return TRUE;
}

void UpgradePartitionDataDestroy(void)
{
    UpgradePartitionDataCtx *ctx = UpgradeCtxGetPartitionData();

    if(ctx)
    {
        if(ctx->signature)
        {
            free(ctx->signature);
            ctx->signature = 0;
        }

        free(ctx);
        UpgradeCtxSetPartitionData(0);
    }
}

uint32 UpgradePartitionDataGetNextReqSize(void)
{
    UpgradePartitionDataCtx *ctx = UpgradeCtxGetPartitionData();

    if(ctx->incompleteData.inProgress)
    {
        return 0;
    }
    else
    {
        return ctx->nextReqSize;
    }
}

uint32 UpgradePartitionDataGetNextOffset(void)
{
    UpgradePartitionDataCtx *ctx = UpgradeCtxGetPartitionData();

    return ctx->nextOffset;
}

/****************************************************************************
NAME
    UpgradePartitionDataParse  -  Handler for smaller than requested packets.

DESCRIPTION
    Checks whether the store requested is one of those that we support, then
    finds the length of the allocated data (if any), allocates storage and
    loads the data from Persistent store into memory.

RETURNS
    Upgrade library error code.
*/
UpgradeHostErrorCode UpgradePartitionDataParse(uint8 *data, uint16 len)
{
    switch (UpgradePartitionDataParseDataCopy(data,len))
    {
    case UPGRADE_PARTITION_DATA_XFER_ERROR:
    default:
        return UPGRADE_HOST_ERROR_BAD_LENGTH_PARTITION_PARSE;

    case UPGRADE_PARTITION_DATA_XFER_IN_PROGRESS:
        return UPGRADE_HOST_SUCCESS;
        
    case UPGRADE_PARTITION_DATA_XFER_COMPLETE:
        {
            return BigDataHandler();
        }
    }
}

/****************************************************************************
NAME
    UpgradePartitionDataParseDataCopy  -  Handler for smaller than requested packets.

DESCRIPTION
    Function to process incoming data and deal with according to the pending 
    request.

    The return value indicates if the packet, held in incompleteData variable 
    is now complete, in progress, or if an error occurred.

RETURNS
    UpgradePartitionDataPartialTransferState - state of the data transfer
*/
UpgradePartitionDataPartialTransferState UpgradePartitionDataParseDataCopy(uint8 *data, uint16 len)
{
    UpgradePartitionDataCtx *ctx = UpgradeCtxGetPartitionData();
    uint16 newLength = ctx->incompleteData.size + len;

    if (   (newLength > ctx->nextReqSize)
        || (0 == ctx->nextReqSize))
    {
        /* Ensure we generate errors from now on */
        ctx->nextReqSize = 0;
        /* And tidy up incompleteData */
        ctx->incompleteData.inProgress = FALSE;
        ctx->incompleteData.size = 0;
        return UPGRADE_PARTITION_DATA_XFER_ERROR;
    }

    /* Initialise for the special first case */
    if (0 == ctx->incompleteData.size)
    {
        memset(&ctx->incompleteData, 0, sizeof(ctx->incompleteData));
        ctx->incompleteData.inProgress = TRUE;
    }

    memmove(&ctx->incompleteData.data[ctx->incompleteData.size], data, len);
    ctx->incompleteData.size += len;

    if (newLength == ctx->nextReqSize)
    {
        /* We have completed the last packet */
        ctx->incompleteData.inProgress = FALSE;
        ctx->incompleteData.size = 0;
        
        return UPGRADE_PARTITION_DATA_XFER_COMPLETE;
    }

    return UPGRADE_PARTITION_DATA_XFER_IN_PROGRESS;
}

/****************************************************************************
NAME
    UpgradePartitionDataStopData  -  Stop processing incoming data

DESCRIPTION
    Function to stop the processing of incoming data.
*/
void UpgradePartitionDataStopData(void)
{
    UpgradeCtxGetPartitionData()->nextReqSize = 0;
}



/****************************************************************************
NAME
    loadFstab  -  Load FSTAB data from the requested store

DESCRIPTION
    Checks whether the store requested is one of those that we support, then
    finds the length of the allocated data (if any), allocates storage and
    loads the data from Persistent store into memory.
*/
void RequestData(uint32 size)
{
    UpgradePartitionDataCtx *ctx = UpgradeCtxGetPartitionData();

    ctx->bigReqSize = size;

    /* Determine size of the next data request. */
    ctx->nextReqSize = (ctx->bigReqSize > UPGRADE_PARTITION_DATA_BLOCK_SIZE) ?
            UPGRADE_PARTITION_DATA_BLOCK_SIZE : ctx->bigReqSize;

    ctx->bigReqSize -= ctx->nextReqSize;

    PRINT(("PART_DATA: RequestData size %ld, big %ld, next %ld\n", size, ctx->bigReqSize, ctx->nextReqSize));
}

/****************************************************************************
NAME
    BigDataHandler  -  Handling of requests of size bigger than the block size

DESCRIPTION
    It determines size of the next data request send to a host.
    It informs parser when the last parser's request for data is fulfilled,
    by setting reqComplete flag.

RETURNS
    Upgrade library error code.
*/
static UpgradeHostErrorCode BigDataHandler(void)
{
    UpgradePartitionDataCtx *ctx = UpgradeCtxGetPartitionData();
    uint8 *data = ctx->incompleteData.data;
    uint16 len = ctx->nextReqSize;

    bool reqComplete = FALSE;

    /* Since we have received some data offset should be cleared */
    ctx->nextOffset = 0;

    /* Determine size of the next data request. */
    ctx->nextReqSize = (ctx->bigReqSize > UPGRADE_PARTITION_DATA_BLOCK_SIZE) ?
            UPGRADE_PARTITION_DATA_BLOCK_SIZE : ctx->bigReqSize;

    /* Determine if this last packet in a 'big' request.
     * Decrement data counter otherwise.
     */
    if(ctx->bigReqSize == 0)
    {
        reqComplete = TRUE;
    }
    else
    {
        ctx->bigReqSize -= ctx->nextReqSize;
    }

    /* Parse received data */
    return ParseCompleteData(data, len, reqComplete);
}

/****************************************************************************
NAME
    ParseCompleteData  -  Parser state machine

DESCRIPTION
    Calls state handlers depending of current state.
    All state handlers are setting size of next data request.

RETURNS
    Upgrade library error code.
*/
UpgradeHostErrorCode ParseCompleteData(uint8 *data, uint16 len, bool reqComplete)
{
    UpgradePartitionDataCtx *ctx = UpgradeCtxGetPartitionData();

    UpgradeHostErrorCode rc = UPGRADE_HOST_ERROR_INTERNAL_ERROR_1;

    {
        uint16 i;
        for(i = 0; i < len; ++i)
        {
            PRINT(("data parse data[%d] 0x%x\n", i, data[i]));
        }
    }

    switch(ctx->state)
    {
    case UPGRADE_PARTITION_DATA_STATE_GENERIC_1ST_PART:
        PRINT(("PART_DATA: HandleGeneric1stPartState\n"));
        rc = HandleGeneric1stPartState(data, len, reqComplete);
        break;

    case UPGRADE_PARTITION_DATA_STATE_HEADER:
        PRINT(("PART_DATA: HandleHeaderState\n"));
        rc = HandleHeaderState(data, len, reqComplete);
        break;

    case UPGRADE_PARTITION_DATA_STATE_DATA_HEADER:
        PRINT(("PART_DATA: HandleDataHeaderState\n"));
        rc = HandleDataHeaderState(data, len, reqComplete);
        break;

    case UPGRADE_PARTITION_DATA_STATE_DATA:
        PRINT(("PART_DATA: HandleDataState len %d, complete %d\n", len, reqComplete));
        rc = HandleDataState(data, len, reqComplete);
        break;

    case UPGRADE_PARTITION_DATA_STATE_FOOTER:
        PRINT(("PART_DATA: HandleFooterState\n"));
        rc = HandleFooterState(data, len, reqComplete);
        break;
    }

    return rc;
}

/****************************************************************************
NAME
    HandleGeneric1stPartState  -  Parser for ID & length part of a header.

DESCRIPTION
    Parses common beginning of any header and determines which header it is.
    All headers have the same first two fields which are 'header id' and
    length.

RETURNS
    Upgrade library error code.
*/
UpgradeHostErrorCode HandleGeneric1stPartState(uint8 *data, uint16 len, bool reqComplete)
{
    UpgradePartitionDataCtx *ctx = UpgradeCtxGetPartitionData();
    uint32 length;
    UNUSED(reqComplete);

    if(len < HEADER_FIRST_PART_SIZE)
    {
        return UPGRADE_HOST_ERROR_BAD_LENGTH_TOO_SHORT;
    }

    length = ByteUtilsGet4BytesFromStream(&data[ID_FIELD_SIZE]);

    PRINT(("1st part header id %c%c%c%c%c%c%c%c len 0x%lx, 0x%x 0x%x 0x%x 0x%x\n", data[0], data[1], data[2], data[3],
            data[4], data[5], data[6], data[7], length, data[8], data[9], data[10], data[11]));

    if(0 == strncmp((char *)data, UpgradeFWIFGetHeaderID(), ID_FIELD_SIZE))
    {
        if(length < UPGRADE_HEADER_MIN_SECOND_PART_SIZE)
        {
            return UPGRADE_HOST_ERROR_BAD_LENGTH_UPGRADE_HEADER;
        }

        RequestData(length);
        ctx->state = UPGRADE_PARTITION_DATA_STATE_HEADER;
    }
    else if(0 == strncmp((char *)data, UpgradeFWIFGetPartitionID(), ID_FIELD_SIZE))
    {
        if(length < PARTITION_SECOND_HEADER_SIZE + FIRST_WORD_SIZE)
        {
            return UPGRADE_HOST_ERROR_BAD_LENGTH_PARTITION_HEADER;
        }
        RequestData(PARTITION_SECOND_HEADER_SIZE + FIRST_WORD_SIZE);
        ctx->state = UPGRADE_PARTITION_DATA_STATE_DATA_HEADER;
        ctx->partitionLength = length - PARTITION_SECOND_HEADER_SIZE;
    }
    else if(0 == strncmp((char *)data, UpgradeFWIFGetFooterID(), ID_FIELD_SIZE))
    {
        if(length != EXPECTED_SIGNATURE_SIZE)
        {
            /* The length of signature must match expected length.
             * Otherwise OEM signature checking could be omitted by just
             * setting length to 0 and not sending signature.
             */
            return UPGRADE_HOST_ERROR_BAD_LENGTH_SIGNATURE;
        }

        RequestData(length);

        ctx->signature = malloc(length/2 + length%2);
        if (!ctx->signature)
        {
            return UPGRADE_HOST_ERROR_OEM_VALIDATION_FAILED_MEMORY;
        }

        ctx->state = UPGRADE_PARTITION_DATA_STATE_FOOTER;
    }
    else
    {
        return UPGRADE_HOST_ERROR_UNKNOWN_ID;
    }

    if(!UpgradeFWIFValidateUpdate(data, len))
    {
        return UPGRADE_HOST_ERROR_OEM_VALIDATION_FAILED_HEADERS;
    }

    return UPGRADE_HOST_SUCCESS;
}

/****************************************************************************
NAME
    HandleHeaderState  -  Parser for the main header.

DESCRIPTION
    Validates content of the main header.

RETURNS
    Upgrade library error code.

NOTES
    Currently when main header size will grow beyond block size it won't work.
*/
UpgradeHostErrorCode HandleHeaderState(uint8 *data, uint16 len, bool reqComplete)
{
    UpgradePartitionDataCtx *ctx = UpgradeCtxGetPartitionData();
    UpgradeHostErrorCode rc = UPGRADE_HOST_SUCCESS;
    upgrade_version newVersion;
    upgrade_version currVersion;
    uint16 newPSVersion;
    uint16 currPSVersion;
    uint16 compatibleVersions;
    uint8 *ptr; /* Pointer into variable length portion of header */
    unsigned i;

    if(!reqComplete)
    {
        /* TODO: Handle a case when header is bigger than blockSize.
         * Currently such situation will cause this error.
         */
        return UPGRADE_HOST_ERROR_INTERNAL_ERROR_2;
    }

    if((strlen(UpgradeFWIFGetDeviceVariant()) > 0) &&
       (strncmp((char *)data, UpgradeFWIFGetDeviceVariant(), ID_FIELD_SIZE)))
    {
        return UPGRADE_HOST_ERROR_WRONG_VARIANT;
    }

    newVersion.major = ByteUtilsGet2BytesFromStream(&data[ID_FIELD_SIZE]);
    newVersion.minor = ByteUtilsGet2BytesFromStream(&data[ID_FIELD_SIZE+2]);
    compatibleVersions = ByteUtilsGet2BytesFromStream(&data[ID_FIELD_SIZE+4]);
    currVersion.major = UpgradeCtxGetPSKeys()->version.major;
    currVersion.minor = UpgradeCtxGetPSKeys()->version.minor;

    PRINT(("Current Version: %d.%d [%d]\n",currVersion.major,currVersion.minor,UpgradeCtxGetPSKeys()->config_version));
    PRINT(("Number compat %d\n",compatibleVersions));

    ptr = &data[ID_FIELD_SIZE+6];
    for (i = 0;i < compatibleVersions;i++)
    {
        upgrade_version version;
        version.major = ByteUtilsGet2BytesFromStream(ptr);
        version.minor = ByteUtilsGet2BytesFromStream(ptr+2);
        PRINT(("Test version: %d.%d\n",version.major,version.minor));
        if (    (version.major == currVersion.major)
            && (    (version.minor == currVersion.minor)
                 || (version.minor == 0xFFFFu)))
        {
            /* Compatible */
            break;
        }
        ptr += 4;
    }

    /* We failed to find a compatibility match */
    if (i == compatibleVersions)
    {
        return UPGRADE_HOST_WARN_APP_CONFIG_VERSION_INCOMPATIBLE;
    }

    ptr = &data[ID_FIELD_SIZE+6+4*compatibleVersions];
    currPSVersion = UpgradeCtxGetPSKeys()->config_version;
    newPSVersion = ByteUtilsGet2BytesFromStream(ptr);
    PRINT(("PS: Curr %d, New %d\n",currPSVersion,newPSVersion));
    if (currPSVersion != newPSVersion)
    {
        compatibleVersions = ByteUtilsGet2BytesFromStream(&ptr[2]);
        ptr+=4;
        PRINT(("Number of compatible PS %d\n",compatibleVersions));
        for (i = 0;i < compatibleVersions;i++)
        {
            uint16 version;
            version = ByteUtilsGet2BytesFromStream(ptr);
            PRINT(("Test PS compatibility %d\n",version));
            if (version == currPSVersion)
            {
                /* Compatible */
                break;
            }
            ptr += 2;
        }
        if (i == compatibleVersions)
        {
            return UPGRADE_HOST_WARN_APP_CONFIG_VERSION_INCOMPATIBLE;
        }
    }

    if(!UpgradeFWIFValidateUpdate(data, len))
    {
        return UPGRADE_HOST_ERROR_OEM_VALIDATION_FAILED_UPGRADE_HEADER;
    }

    /* Store the in-progress upgrade version */
    UpgradeCtxGetPSKeys()->version_in_progress.major = newVersion.major;
    UpgradeCtxGetPSKeys()->version_in_progress.minor = newVersion.minor;
    UpgradeCtxGetPSKeys()->config_version_in_progress = newPSVersion;

    PRINT(("Saving versions %d.%d [%d]\n",newVersion.major,newVersion.minor,newPSVersion));

    /* At this point, partitions aren't actually dirty - but want to minimise PSKEYS
     * @todo: Need to check this variable before starting an upgrade
     */
    UpgradeCtxGetPSKeys()->state_of_partitions = UPGRADE_PARTITIONS_UPGRADING;

    /*!
        @todo Need to minimise the number of times that we write to the PS
              so this may not be the optimal place. It will do for now.
    */
    UpgradeSavePSKeys();

    RequestData(HEADER_FIRST_PART_SIZE);
    ctx->state = UPGRADE_PARTITION_DATA_STATE_GENERIC_1ST_PART;

    return rc;
}

/****************************************************************************
NAME
    HandleHeaderState  -  Parser for the partition data header.

DESCRIPTION
    Validates content of the partition data header.

RETURNS
    Upgrade library error code.
*/
UpgradeHostErrorCode HandleDataHeaderState(uint8 *data, uint16 len, bool reqComplete)
{
    UpgradePartitionDataCtx *ctx = UpgradeCtxGetPartitionData();
    uint16 physPartNum, logicPartNum, firstWord;
    UpgradeFWIFPartitionType partType;
    UNUSED(reqComplete);

    if(len < PARTITION_SECOND_HEADER_SIZE + FIRST_WORD_SIZE)
    {
        return UPGRADE_HOST_ERROR_BAD_LENGTH_DATAHDR_RESUME;
    }

    logicPartNum = ByteUtilsGet2BytesFromStream(&data[2]);
    PRINT(("PART_DATA: logic part %u\n", logicPartNum));

    physPartNum = UpgradeFWIFGetPhysPartition(logicPartNum);
    PRINT(("PART_DATA: phys part %u\n", physPartNum));
    physPartNum = UPGRADE_PARTITION_PHYSICAL_PARTITION(physPartNum);
    PRINT(("PART_DATA: phys part %u\n", physPartNum));

    firstWord = data[PARTITION_SECOND_HEADER_SIZE + 1];
    firstWord |= (uint16)data[PARTITION_SECOND_HEADER_SIZE] << 8;
    PRINT(("PART_DATA: first word is 0x%x\n", firstWord));

    if(physPartNum >= UpgradeFWIFGetPhysPartitionNum())
    {
        return UPGRADE_HOST_ERROR_WRONG_PARTITION_NUMBER;
    }

    PRINT(("last closed %d, open next %d\n", UpgradeCtxGetPSKeys()->last_closed_partition, ctx->openNextPartition));

    if(UpgradeCtxGetPSKeys()->last_closed_partition && !ctx->openNextPartition)
    {
        PRINT(("not going to open this partition but we will open next %d\n", (UpgradeCtxGetPSKeys()->last_closed_partition == physPartNum + 1)));
        if(UpgradeCtxGetPSKeys()->last_closed_partition == physPartNum + 1)
        {
            ctx->openNextPartition = TRUE;
        }

        ctx->nextOffset = ctx->partitionLength - FIRST_WORD_SIZE;
        RequestData(HEADER_FIRST_PART_SIZE);
        ctx->state = UPGRADE_PARTITION_DATA_STATE_GENERIC_1ST_PART;

        return UPGRADE_HOST_SUCCESS;
    }
    else
    {
        ctx->openNextPartition = FALSE;
    }


    partType = ByteUtilsGet2BytesFromStream(data);
    if(!UpgradeFWIFValidPartitionType(partType, physPartNum))
    {
        return UPGRADE_HOST_ERROR_PARTITION_TYPE_NOT_MATCHING;
    }

    if (partType == UPGRADE_FW_IF_PARTITION_TYPE_DFU)
    {
        if (UpgradePartitionDataIsDfuUpdate() &&
                (UpgradePartitionDataGetDfuPartition() != physPartNum))
        {
            return UPGRADE_HOST_ERROR_PARTITION_TYPE_TWO_DFU;
        }

        PRINT(("PART_DATA: DFU detected in partition %d\n", physPartNum));

        /* DFU partition number will be stored in non volatile space upon partition close */
        UpgradeCtxGetPSKeys()->dfu_partition_num = physPartNum + 1;
    }

    if(ctx->partitionLength > UpgradeFWIFGetPhysPartitionSize(physPartNum))
    {
        return UPGRADE_HOST_ERROR_PARTITION_SIZE_MISMATCH;
    }

    if(!UpgradeFWIFValidateUpdate(data, len))
    {
        return UPGRADE_HOST_ERROR_OEM_VALIDATION_FAILED_PARTITION_HEADER1;
    }

    ctx->partitionHdl = UpgradeFWIFPartitionOpen(logicPartNum, physPartNum, firstWord);
    if(!ctx->partitionHdl)
    {
        return UPGRADE_HOST_ERROR_PARTITION_OPEN_FAILED;
    }

    ctx->nextOffset = UpgradeFWIFPartitionGetOffset(ctx->partitionHdl);

    PRINT(("PART_DATA: partition length is %ld and offset is: %ld\n", ctx->partitionLength, ctx->nextOffset));

    if(ctx->nextOffset == 0)
    {
        if(!UpgradeFWIFValidateUpdate(&data[PARTITION_SECOND_HEADER_SIZE], FIRST_WORD_SIZE))
        {
            return UPGRADE_HOST_ERROR_OEM_VALIDATION_FAILED_PARTITION_HEADER2;
        }

        if(FIRST_WORD_SIZE != UpgradeFWIFPartitionWrite(ctx->partitionHdl, &data[PARTITION_SECOND_HEADER_SIZE], FIRST_WORD_SIZE))
        {
            return UPGRADE_HOST_ERROR_PARTITION_WRITE_FAILED_HEADER;
        }
    }
    if(ctx->nextOffset < ctx->partitionLength)
    {

        if(ctx->nextOffset >= FIRST_WORD_SIZE)
        {
            ctx->nextOffset -= FIRST_WORD_SIZE;
        }
        RequestData(ctx->partitionLength - ctx->nextOffset - FIRST_WORD_SIZE);
        ctx->state = UPGRADE_PARTITION_DATA_STATE_DATA;
    }
    else if(ctx->nextOffset == ctx->partitionLength)
    {
        /* A case when all data are in but partition is not yet closed */
        UpgradeHostErrorCode closeStatus = UpgradeFWIFPartitionClose(ctx->partitionHdl);
        if(UPGRADE_HOST_SUCCESS != closeStatus)
        {
            return closeStatus;
        }

        ctx->openNextPartition = TRUE;

        ctx->nextOffset -= FIRST_WORD_SIZE;
        RequestData(HEADER_FIRST_PART_SIZE);
        ctx->state = UPGRADE_PARTITION_DATA_STATE_GENERIC_1ST_PART;
    }
    else
    {
        /* It is considered bad when offset is bigger than partition size */

        return UPGRADE_HOST_ERROR_INTERNAL_ERROR_3;
    }

    PRINT(("PART_DATA: partition length is %ld and offset is: %ld bigreq is %ld, nextReq is %ld\n", ctx->partitionLength, ctx->nextOffset, ctx->bigReqSize, ctx->nextReqSize));


    return UPGRADE_HOST_SUCCESS;
}

/****************************************************************************
NAME
    HandleDataState  -  Partition data handling.

DESCRIPTION
    Writes data to a SQIF and sends it the MD5 validation.

RETURNS
    Upgrade library error code.
*/
UpgradeHostErrorCode HandleDataState(uint8 *data, uint16 len, bool reqComplete)
{
    UpgradePartitionDataCtx *ctx = UpgradeCtxGetPartitionData();

    if(!UpgradeFWIFValidateUpdate(data, len))
    {
        return UPGRADE_HOST_ERROR_OEM_VALIDATION_FAILED_PARTITION_DATA;
    }

    if(len != UpgradeFWIFPartitionWrite(ctx->partitionHdl, data, len))
    {
        return UPGRADE_HOST_ERROR_PARTITION_WRITE_FAILED_DATA;
    }

    if(reqComplete)
    {
        UpgradeHostErrorCode closeStatus;
        RequestData(HEADER_FIRST_PART_SIZE);
        ctx->state = UPGRADE_PARTITION_DATA_STATE_GENERIC_1ST_PART;

        closeStatus = UpgradeFWIFPartitionClose(ctx->partitionHdl);
        if(UPGRADE_HOST_SUCCESS != closeStatus)
        {
            return closeStatus;
        }

        ctx->openNextPartition = TRUE;
    }

    return UPGRADE_HOST_SUCCESS;
}

/****************************************************************************
NAME
    HandleFooterState  -  Signature data handling.

DESCRIPTION
    Collects MD5 signature data and sends it for validation.
    Completion of this step means that data were download to a SQIF.

RETURNS
    Upgrade library error code.
*/
UpgradeHostErrorCode HandleFooterState(uint8 *data, uint16 len, bool reqComplete)
{
    UpgradePartitionDataCtx *ctx = UpgradeCtxGetPartitionData();

    PRINT(("HandleFooterState\n"));

    ByteUtilsMemCpyFromStream(&ctx->signature[ctx->signatureReceived/2], data, len);

    ctx->signatureReceived += len;

    if(reqComplete)
    {
        bool signatureValid;

        PRINT(("PART_DATA: Upgrade file download is finished\n"));

        signatureValid = UpgradeFWIFValidateFinalize(ctx->signature);

        free(ctx->signature);
        ctx->signature = 0;
        ctx->signatureReceived = 0;

        if(signatureValid)
        {
            return UPGRADE_HOST_OEM_VALIDATION_SUCCESS;
        }
        else
        {
            return UPGRADE_HOST_ERROR_OEM_VALIDATION_FAILED_FOOTER;
        }
    }

    return UPGRADE_HOST_SUCCESS;
}

bool UpgradePartitionDataIsDfuUpdate(void)
{
    return (UpgradeCtxGetPSKeys()->dfu_partition_num != 0);
}

uint16 UpgradePartitionDataGetDfuPartition(void)
{
    return UpgradeCtxGetPSKeys()->dfu_partition_num - 1;
}
