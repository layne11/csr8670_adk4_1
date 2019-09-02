/****************************************************************************
Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.1

FILE NAME
    upgrade_fw_if.c

DESCRIPTION
    Implementation of functions which (largely) interact with the firmware.

NOTES

*/

#include <stdlib.h>
#include <string.h>
#include "upgrade_partitions.h"


#include <byte_utils.h>
#include <csrtypes.h>
#include <panic.h>
#include <partition.h>
#include <print.h>
#include <sink.h>
#include <stream.h>
#if defined (UPGRADE_USE_OEM_SIGN_VALIDATION) || defined (UPGRADE_USE_CSR_SIGN_VALIDATION)
#include <validation.h>
#endif

#include "upgrade_ctx.h"

#include "upgrade_psstore.h"
#include "upgrade_partitions.h"
#include "upgrade_private.h"
#include "upgrade_msg_host.h"

#include "upgrade_fw_if.h"
#include "upgrade_fw_if_priv.h"
#include "upgrade_hydra.h"

/******************************************************************************
NAME
    UpgradeFWIFInit

DESCRIPTION
    Initialise the context for the Upgrade FW IF.

*/
void UpgradeFWIFInit(void)
{
    UpgradeFWIFCtx *fwctx = UpgradeCtxGetFW();

    memset(fwctx, 0, sizeof(*fwctx));
}

/******************************************************************************
NAME
    UpgradeFWIFGetHeaderID

DESCRIPTION
    Get the identifier for the header of an upgrade file.

RETURNS
    const char * Pointer to the header string.
*/
const char *UpgradeFWIFGetHeaderID(void)
{
    return "APPUHDR2";
}

/******************************************************************************
NAME
    UpgradeFWIFGetPartitionID

DESCRIPTION
    Get the identifier for a partition header within an upgrade file.

RETURNS
    const char * Pointer to the partition header string.
*/
const char *UpgradeFWIFGetPartitionID(void)
{
    return "PARTDATA";
}

/******************************************************************************
NAME
    UpgradeFWIFGetFooterID

DESCRIPTION
    Get the identifier for the footer of an upgrade file.

RETURNS
    const char * Pointer to the footer string.
*/
const char *UpgradeFWIFGetFooterID(void)
{
    return "APPUPFTR";
}

/******************************************************************************
NAME
    UpgradeFWIFGetDeviceVariant

DESCRIPTION
    Get the identifier for the current device variant.

RETURNS
    const char * Pointer to the device variant string.
*/
const char *UpgradeFWIFGetDeviceVariant(void)
{
    return ( const char * )UpgradeCtxGet()->dev_variant;
}

/******************************************************************************
NAME
    UpgradeFWIFGetAppVersion

DESCRIPTION
    Get the current (running) app version.

RETURNS
    uint16 The running app version.
*/
uint16 UpgradeFWIFGetAppVersion(void)
{
    return 2;
}

#ifndef UPGRADE_USE_FW_STUBS

/******************************************************************************
NAME
    UpgradeFWIFGetPhysPartition

DESCRIPTION
    Given a logical partition number return the physical partition number into 
    which we can write data. This includes the SQUIF selector, use 
    UPGRADE_PARTITION_PHYSICAL_PARTITION to extract partition ID.

RETURNS
    uint16 Number of the physical partition available for write.
*/
uint16 UpgradeFWIFGetPhysPartition(uint16 logicPartition)
{
    return UpgradePartitionsPhysicalPartition(logicPartition,UpgradePartitionUpgradable);
}

/******************************************************************************
NAME
    UpgradeFWIFGetPhysPartitionNum

DESCRIPTION
    Work out how many partitions there are in the serial flash.

    Just repeatedly ask the firmware for partition info, incremeting a counter
    until the firmware tells us it failed.

RETURNS
    uint16 Number of partitions in the serial flash.
*/
uint16 UpgradeFWIFGetPhysPartitionNum(void)
{
    uint32 value = 0;
    uint16 partition_count = 0;

    while (PartitionGetInfo(PARTITION_SERIAL_FLASH, partition_count,
                            PARTITION_INFO_TYPE, &value))
    {
        partition_count++;
    }

    return partition_count;
}

/******************************************************************************
NAME
    UpgradeFWIFGetPhysPartitionSize

DESCRIPTION
    Find out the size of a specified partition in bytes.

RETURNS
    uint32 Size of physPartition in bytes.
*/
uint32 UpgradeFWIFGetPhysPartitionSize(uint16 physPartition)
{
    uint32 size = 0;

    if (PartitionGetInfo(PARTITION_SERIAL_FLASH, physPartition,
                     PARTITION_INFO_SIZE, &size))
    {
        return (size * 2);
    }

    return 0;
}

/******************************************************************************
NAME
    UpgradeFWIFValidPartitionType

DESCRIPTION
    Determine if the partition type (in the flash) is a valid type that the
    upgrade library is expecting (and knows how to handle).

RETURNS
    bool TRUE if type is valid, FALSE if type is invalid
*/
bool UpgradeFWIFValidPartitionType(UpgradeFWIFPartitionType type, uint16 physPartition)
{
    uint32 ptn_type;
    static const partition_type valid_types[UPGRADE_FW_IF_PARTITION_TYPE_NUM] =
    {
        PARTITION_TYPE_FILESYSTEM, /* UPGRADE_FW_IF_PARTITION_TYPE_EXE */
        PARTITION_TYPE_RAW_SERIAL, /* UPGRADE_FW_IF_PARTITION_TYPE_DFU */
        PARTITION_TYPE_PS,         /* UPGRADE_FW_IF_PARTITION_TYPE_CONFIG */
        PARTITION_TYPE_FILESYSTEM, /* UPGRADE_FW_IF_PARTITION_TYPE_DATA */
        PARTITION_TYPE_RAW_SERIAL  /* UPGRADE_FW_IF_PARTITION_TYPE_DATA_RAW_SERIAL */
    };

    if ((type < UPGRADE_FW_IF_PARTITION_TYPE_EXE) ||
        (type >= UPGRADE_FW_IF_PARTITION_TYPE_NUM))
    {
        PRINT(("UPG: unknown partition header type %u\n", type));
        return FALSE;
    }

    /* Add check that header partition type is compatible with type in partition info */
    if (!PartitionGetInfo(PARTITION_SERIAL_FLASH, physPartition, PARTITION_INFO_TYPE, &ptn_type))
    {
        PRINT(("UPG: Failed to get partition info of partition %u\n", physPartition));
        return FALSE;
    }

    if (valid_types[type] != (partition_type)ptn_type)
    {
        PRINT(("UPG: valid partition type [%u] %u does not match actual type %lu\n",
            type, valid_types[type], ptn_type));
        return FALSE;
    }

    return TRUE;
}

/***************************************************************************
NAME
    UpgradeFWIFPartitionOpen

DESCRIPTION
    Open a write only handle to a physical partition on the external flash.
    For initial testing, the CRC check on the partition is also disabled.

PARAMS
    physPartition Physical partition number in external flash.
    firstWord First word of partition data.

RETURNS
    UpgradeFWIFPartitionHdl A valid handle or NULL if the open failed.
*/
UpgradeFWIFPartitionHdl UpgradeFWIFPartitionOpen(uint16 logic,uint16 physPartition, uint16 firstWord)
{
    Sink sink;

    PRINT(("UPG: Opening partition %u for resumee\n", physPartition));

    sink = StreamPartitionResumeSink(PARTITION_SERIAL_FLASH, physPartition, firstWord);
    if (!sink)
    {
        PRINT(("UPG: Failed to open raw partition %u for resume\n", physPartition));
        return (UpgradeFWIFPartitionHdl)(int)NULL;
    }

    /* Disable the crc check */
    PartitionSetMessageDigest(sink, PARTITION_MESSAGE_DIGEST_SKIP, NULL, 0);

    /* This can't be used in link loss scenario until FW will fix its bug */
#if 0
    if (!PartitionSetMessageDigest(sink, PARTITION_MESSAGE_DIGEST_SKIP, NULL, 0))
    {
        PRINT(("UPG: Failed to set message digest on partition %u 0x%x\n", physPartition, (uint16)sink));
        SinkClose(sink);
        return (UpgradeFWIFPartitionHdl)(int)NULL;
    }
#endif

    UpgradePartitionsMarkUpgrading(logic);

    PRINT(("Early offset is %ld\n",PartitionSinkGetPosition(sink)));

    UpgradeCtxGetFW()->partitionNum = physPartition;

    return (UpgradeFWIFPartitionHdl)(int)sink;
}

/***************************************************************************
NAME
    UpgradeFWIFPartitionWrite

DESCRIPTION
    Write data to an open external flash partition. Each byte of the data
    is copied to the partition in a byte by byte copy operation.

PARAMS
    handle Handle to a writeable partition.
    data Pointer to the data to write.
    len Number of bytes (not words) to write.

RETURNS
    uint16 The number of bytes written, or 0 if there was an error.
*/
uint16 UpgradeFWIFPartitionWrite(UpgradeFWIFPartitionHdl handle, uint8 *data, uint16 len)
{
    uint8 *dst;
    Sink sink = (Sink)(int)handle;

    if (!sink)
        return 0;

    /* For 1st pass don't worry about size of writes between flushes */

    dst = SinkMap(sink);
    if (!dst)
    {
        PRINT(("UPG: Failed to map sink %p\n", (void *)sink));
        return 0;
    }

    if (SinkClaim(sink, len) == 0xFFFF)
    {
        PRINT(("UPG: Failed to claim %u bytes for writing\n", len));
        return 0;
    }

    memmove(dst, data, len);

    if (!SinkFlush(sink, len))
    {
        PRINT(("UPG: Failed to flush data to partition\n"));
        return 0;
    }

    return len;
}

/***************************************************************************
NAME
    UpgradeFWIFPartitionClose

DESCRIPTION
    Close a handle to an external flash partition.

PARAMS
    handle Handle to a writeable partition.

RETURNS
    bool TRUE if a valid handle is given, FALSE otherwise.
*/
UpgradeHostErrorCode UpgradeFWIFPartitionClose(UpgradeFWIFPartitionHdl handle)
{

    Sink sink = (Sink)(int)handle;

    PRINT(("UPG: Closing partition\n"));

    if (!sink)
        return FALSE;

    if (!UpgradePSSpaceForCriticalOperations())
    {
        return UPGRADE_HOST_ERROR_PARTITION_CLOSE_FAILED_PS_SPACE;
    }

    if (!SinkClose(sink))
    {
        PRINT(("Unable to close SINK\n"));
        return UPGRADE_HOST_ERROR_PARTITION_CLOSE_FAILED;
    }
    /* last_closed_partition == partition_num + 1
     * so value 0 means no partitions were closed
     */
    UpgradeCtxGetPSKeys()->last_closed_partition = UpgradeCtxGetFW()->partitionNum + 1;
    UpgradeSavePSKeys();
    PRINT(("P&R: last_closed_partition is %d\n", UpgradeCtxGetPSKeys()->last_closed_partition));

    return UPGRADE_HOST_SUCCESS;
}

#else

uint16 UpgradeFWIFGetPhysPartition(uint16 logicPartition)
{
    return logicPartition;
}

uint16 UpgradeFWIFGetPhysPartitionNum(void)
{
    return 8;
}

uint32 UpgradeFWIFGetPhysPartitionSize(uint16 physPartition)
{
    UNUSED(physPartition);
    return 4000000;
}

bool UpgradeFWIFValidPartitionType(UpgradeFWIFPartitionType type, uint16 physPartition)
{
    UNUSED(type);
    UNUSED(physPartition);
    return TRUE;
}

UpgradeFWIFPartitionHdl UpgradeFWIFPartitionOpen(uint16 logic,uint16 physPartition, uint16 firstWord)
{
    UNUSED(logic);
    UNUSED(firstWord);
    return physPartition;
}

uint16 UpgradeFWIFPartitionWrite(UpgradeFWIFPartitionHdl handle, uint8 *data, uint16 len)
{
    UNUSED(handle);
    UpgradeCtxGetFW()->lastPartitionData = data;
    UpgradeCtxGetFW()->lastPartitionDataLen = len;
    return len;
}

UpgradeHostErrorCode UpgradeFWIFPartitionClose(UpgradeFWIFPartitionHdl handle)
{
    UNUSED(handle);
    return UPGRADE_HOST_SUCCESS;
}

#endif

#ifdef UPGRADE_USE_OEM_SIGN_VALIDATION
/***************************************************************************
NAME
    UpgradeFWIFValidateInit

DESCRIPTION
    Initialise the context for validating the MD5 checksum of some data.
    Only one context is supported at a time.

RETURNS
*/
void UpgradeFWIFValidateInit(void)
{
    PRINT(("UPG: UpgradeFWIFValidateInit\n"));

    if (UpgradeCtxGetFW()->vctx)
    {
        PRINT(("UPG: Warning validation context already exists.\n"));
    }

    UpgradeCtxGetFW()->vctx = ValidationInitialise();
    if (UpgradeCtxGetFW()->vctx == VALIDATION_INVALID_CONTEXT)
        Panic();
}

/***************************************************************************
NAME
    UpgradeFWIFValidateUpdate

DESCRIPTION
    Update the validation context with the next set of data.

PARAMS
    buffer Pointer to the next set of data.
    len length of the data.

RETURNS
    bool TRUE if a validation context is updated successfully, FALSE otherwise.
*/
bool UpgradeFWIFValidateUpdate(uint8 *buffer, uint16 len)
{
    return ValidationUpdate(UpgradeCtxGetFW()->vctx, buffer, len);
}

/***************************************************************************
NAME
    UpgradeFWIFValidateFinalize

DESCRIPTION
    Verify the accumulated data in the validation context against
    the given signature.

PARAMS
    signature Pointer to the signature to compare against.

RETURNS
    bool TRUE if validation is successful, FALSE otherwise.
*/
bool UpgradeFWIFValidateFinalize(uint8 *signature)
{
    bool rc = ValidationFinalise(UpgradeCtxGetFW()->vctx, (const uint16 *)signature);
    PRINT(("UPG: UpgradeFWIFValidateFinalize %s\n", rc ? "PASSED" : "FAILED"));
    return rc;
}
#else
void UpgradeFWIFValidateInit(void)
{
}

bool UpgradeFWIFValidateUpdate(uint8 *buffer, uint16 len)
{
    UNUSED(buffer);
    UNUSED(len);
    return TRUE;
}

bool UpgradeFWIFValidateFinalize(uint8 *signature)
{
    UNUSED(signature);
    return TRUE;
}
#endif /* UPGRADE_USE_OEM_SIGN_VALIDATION */

#ifdef UPGRADE_USE_CSR_SIGN_VALIDATION
/******************************************************************************
NAME
    UpgradeFWIFValidateExecutablePartition

DESCRIPTION
    Initiate validation of partition specified by physPartition. May or may
    not be an executable partition, we don't know until we try. Firmware will
    either start validation or report not an executable.

RETURNS
    UpgradeFWIFPartitionValidationStatus
*/
UpgradeFWIFPartitionValidationStatus UpgradeFWIFValidateExecutablePartition(uint16 physPartition)
{
    validation_init_status status;
    UpgradeFWIFPartitionValidationStatus rc = UPGRADE_FW_IF_PARTITION_VALIDATION_SKIP;

    status = ValidationInitiateExecutablePartition(physPartition);
    switch (status)
    {
        case VALIDATE_BOOTUP_VALIDATION_RUNNING:
            /* fall-thru */
        case VALIDATE_PARTITION_VALIDATION_RUNNING:
            rc = UPGRADE_FW_IF_PARTITION_VALIDATION_IN_PROGRESS;
            break;

        case VALIDATE_PARTITION_VALIDATION_TRIGGERED:
            rc = UPGRADE_FW_IF_PARTITION_VALIDATION_TRIGGERED;
            break;

        case VALIDATE_PARTITION_TYPE_NOT_FS:
            /* fall-thru */
        case VALIDATE_FS_NOT_EXECUTABLE:
            rc = UPGRADE_FW_IF_PARTITION_VALIDATION_SKIP;
            break;
    }

    return rc;
}
#else
UpgradeFWIFPartitionValidationStatus UpgradeFWIFValidateExecutablePartition(uint16 physPartition)
{
    UNUSED(physPartition);
    return UPGRADE_FW_IF_PARTITION_VALIDATION_SKIP;
}
#endif /* UPGRADE_USE_CSR_SIGN_VALIDATION */

/******************************************************************************
NAME
    UpgradeFWIFPartitionGetOffset
*/
uint32 UpgradeFWIFPartitionGetOffset(UpgradeFWIFPartitionHdl handle)
{
    Sink sink = (Sink)(int)handle;
    uint32 offset = PartitionSinkGetPosition(sink);

    PRINT(("UPG: Nonzero partition offset %ld\n", offset));

    return offset;
}
