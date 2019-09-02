/****************************************************************************
Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.1

FILE NAME
    upgrade_ps_journal.c

DESCRIPTION

NOTES

*/

#include <stdlib.h>

#include <panic.h>
#include <partition.h>
#include <ps.h>
#include <sink.h>
#include <source.h>
#include <stream.h>
#include <string.h>

#include <byte_utils.h>
#include <print.h>
#include <upgrade.h>

#include "upgrade_fw_if.h"
#include "upgrade_ps_journal.h"
#include "upgrade_hydra.h"


/* PS Journal data format version */
#define PS_JOURNAL_VERSION 1

/* The PSI format version is stored as the 6th word in PSKEY_USER49
   (aka CONFIG_SOFTWARE_VERSION_ID in sink_configmanager.h) */
#define PSI_FORMAT_VERSION_KEY 49


typedef struct
{
    Task task;
    uint16 partition;
} upgrade_ps_journal_ctx;

typedef struct
{
    uint16 config_version;
    uint16 journal_version;

    uint16 unused[2];
} upgrade_ps_journal_header;

typedef struct
{
    uint16 key;
    upgrade_ps_journal_action action;
    uint16 len;
} upgrade_ps_journal_entry;


upgrade_ps_journal_ctx journal_ctx;


/****************************************************************************
NAME
    UpgradePsJournalReformatKeys

DESCRIPTION
    Custom function that converts the data format of relevant
    PS Keys in the PSI.

    The actual conversion to do depends on the the current version of
    the PSI data format and the upgraded version.

    The converted key and data are written to the PS Journal.

RETURNS
*/
static void UpgradePsJournalReformatKeys(Sink sink, uint16 curr_config_version, uint16 config_version)
{
    uint16 data[] = { 0xDEAD, 0xBEEF };
    UNUSED(curr_config_version);
    UNUSED(config_version);

/* taken from sink_configmanager.h */
#define CONFIG_SQIF_PARTITIONS 5
    UpgradePsJournalWriteKeyAction(sink, CONFIG_SQIF_PARTITIONS, UPGRADE_PS_JOURNAL_ACTION_REPLACE, (uint8 *)data, (sizeof(data) * 2));
    data[0] = 0xF;
    data[1] = 0x196;
    UpgradePsJournalWriteKeyAction(sink, CONFIG_SQIF_PARTITIONS, UPGRADE_PS_JOURNAL_ACTION_REPLACE, (uint8 *)data, (sizeof(data) * 2));
}

/****************************************************************************
NAME
    UpgradePsJournalInit

DESCRIPTION
    Initialise the PS Journal module

RETURNS
*/
void UpgradePsJournalInit(Task task, uint16 partition)
{
    journal_ctx.task = task;
    journal_ctx.partition = partition;

    PRINT(("UPG: ps journal task %p partition %u\n", (void *)task, partition));
}

/****************************************************************************
NAME
    UpgradePsJournalWriteKeyAction

DESCRIPTION
    Write out an action to be taken on a key to the PS Journal.

RETURNS
*/
bool UpgradePsJournalWriteKeyAction(Sink sink, uint16 key, upgrade_ps_journal_action action, uint8 *data, uint16 len)
{
    upgrade_ps_journal_entry entry;
    uint8 *dst;

    entry.key = key;
    entry.action = action;
    entry.len = len;

    PRINT(("UPG: Writing key %u entry %u, len %u\n", entry.key, (uint16)entry.action, entry.len));

    dst = SinkMap(sink);
    if (!dst)
    {
        PRINT(("UPG: Failed to map sink %p for key action\n", (void *)sink));
        return FALSE;
    }

    if (SinkClaim(sink, (sizeof(entry) * 2)) == 0xFFFF)
    {
        PRINT(("UPG: Failed to claim %u bytes for key entry\n", sizeof(entry)));
        return FALSE;
    }

    ByteUtilsMemCpyToStream(dst, (uint8 *)&entry, (sizeof(entry) * 2));

    if (!SinkFlush(sink, (sizeof(entry) * 2)))
    {
        PRINT(("UPG: Failed to flush key entry\n"));
        return FALSE;
    }

    if (data && len > 0)
    {
        if (SinkClaim(sink, len) == 0xFFFF)
        {
            PRINT(("UPG: Failed to claim %u bytes for key data\n", len));
            return FALSE;
        }

        ByteUtilsMemCpyToStream(dst, data, len);

        if (!SinkFlush(sink, len))
        {
            PRINT(("UPG: Failed to flush key data\n"));
            return FALSE;
        }
    }

    return TRUE;
}

/****************************************************************************
NAME
    UpgradePsJournalProcessNextKeyAction

DESCRIPTION
    Process the next action in the PS Journal. If it is the special
    'end of list' marker, return FALSE to inform that all actions have been
    processed.

RETURNS
*/
static bool UpgradePsJournalProcessNextKeyAction(Source source)
{
    upgrade_ps_journal_entry entry;
    const uint8 *src;

    src = SourceMap(source);
    if (!src)
    {
        PRINT(("UPG: Failed to map PS Journal partition\n"));
        return FALSE;
    }

    ByteUtilsMemCpyFromStream((uint8 *)&entry, src, (sizeof(entry) * 2));
    SourceDrop(source, (sizeof(entry) * 2));

    PRINT(("UPG: Process key %u action %u, len %u\n", entry.key, (uint16)entry.action, entry.len));

    switch (entry.action)
    {
    case UPGRADE_PS_JOURNAL_ACTION_DELETE:
    {
        /* Delete this key from PS Implementation store */
        PRINT(("UPG: ACTION: DELETE not implemented yet\n"));
        break;
    }
    case UPGRADE_PS_JOURNAL_ACTION_ADD:
    {
        /* Add this key to the PS Implementation Store */
        /* Note: don't actually expect this to be needed. */
        PRINT(("UPG: ACTION: ADD not implemented yet\n"));
        break;
    }
    case UPGRADE_PS_JOURNAL_ACTION_REPLACE:
    {
        /* Replace whatever is currently in this key, or add it
           if it does not exist */
        if (entry.len)
        {
            uint8 *data;

            data = (uint8 *)malloc(entry.len);
            if (!data)
            {
                PRINT(("UPG: Failed to allocate buffer to read key data\n"));
                return FALSE;
            }

            ByteUtilsMemCpyFromStream(data, src, entry.len);
            SourceDrop(source, entry.len);

            PRINT(("UPG: ACTION: REPLACE key %u with 0x%x\n", entry.key, data[0]));

            /*
            PsStores prev_store = PsGetStore();
            PsSetStore(ps_store_implementation);
            PsFullStore(key, data, entry.len);
            PsSetStore(prev_store);
            */

            free(data);
        }
        break;
    }
    case UPGRADE_PS_JOURNAL_ACTION_END_OF_LIST:
    {
        PRINT(("UPG: End of PS Journal reached\n"));
        return FALSE;
    }
    default:
        PRINT(("UPG: unknown PS Journal action 0x%x\n", entry.action));
        break;
    }

    return TRUE;
}

/****************************************************************************
NAME
    UpgradePsJournalIsValid

DESCRIPTION
    Check if the App PS Scratch partition contains a valid set of
    PS journal data.

RETURNS
    TRUE if the is valid, FALSE otherwise.
*/
bool UpgradePsJournalIsValid(void)
{
    Source source;

    /* Rely on the partition stream crc checking to validate the partition.
       i.e. we can only open the partition if it passed the crc check when
       it was written out. */
    source = PartitionGetRawSerialSource(PARTITION_SERIAL_FLASH, journal_ctx.partition);
    if (!source)
        return FALSE;

    SourceClose(source);

    return TRUE;
}

/****************************************************************************
NAME
    UpgradePsJournalCreate

DESCRIPTION
    Create the PS Journal (if needed) to store the PS Key
    changes to convert the PS Implementation store from the
    current version to the upgrade version.

RETURNS
    TRUE if completed successfully, FALSE is there is an error or if a
    PS Journal is not needed.
*/
Sink UpgradePsJournalCreate(uint16 curr_config_version, uint16 config_version)
{
    upgrade_ps_journal_header header;
    Sink sink;
    uint8 *dst;
    UNUSED(curr_config_version);

    /* Open the partition 
        @todo MIGHT need to call UpgradePartitionsMarkUpgrading() at some point here.
        Depends on whether the journal_ctx.partition is hard coded, or a logical
     */
    sink = StreamPartitionOverwriteSink(PARTITION_SERIAL_FLASH, journal_ctx.partition);
    if (!sink)
    {
        PRINT(("UPG: Failed to open PS Journal partition %u\n", journal_ctx.partition));
        return NULL;
    }

    dst = SinkMap(sink);
    if (!dst)
    {
        PRINT(("UPG: Failed to map sink %p for journal header\n", (void *)sink));
        SinkClose(sink);
        return NULL;
    }

    /* Fill in the header details */
    if (SinkClaim(sink, (sizeof(header) * 2)) == 0xFFFF)
    {
        PRINT(("UPG: Failed to claim space for header.\n"));
        SinkClose(sink);
        return NULL;
    }

    memset(&header, 0, sizeof(header));
    header.config_version = config_version;
    header.journal_version = PS_JOURNAL_VERSION;
    ByteUtilsMemCpyToStream(dst, (uint8 *)&header, (sizeof(header) * 2));

    if (!SinkFlush(sink, (sizeof(header) * 2)))
    {
        PRINT(("UPG: Failed to flush sink\n"));
        SinkClose(sink);
        return NULL;
    }

    return sink;
}

/****************************************************************************
NAME
    UpgradePsJournalClose

DESCRIPTION
    Complete a PS journal by writing the end marker and setting the
    message digest then close the sink.

RETURNS
    TRUE if closed ok, FALSE if an error writing to sink.
*/
bool UpgradePsJournalClose(Sink sink)
{
    if (!sink)
        return FALSE;

    /* Write the end of list marker */
    UpgradePsJournalWriteKeyAction(sink, 0, UPGRADE_PS_JOURNAL_ACTION_END_OF_LIST, NULL, 0);

    /* Disable the crc check */
    PartitionSetMessageDigest(sink, PARTITION_MESSAGE_DIGEST_SKIP, NULL, 0);

    /* Close the partition */
    SinkClose(sink);

    return TRUE;
}

/****************************************************************************
NAME
    UpgradePsJournalApply

DESCRIPTION
    Apply the changes stored in the PS Journal to the PS Implementation store

RETURNS
    TRUE if fully completed, FALSE otherwise.
*/
bool UpgradePsJournalApply(void)
{
    upgrade_ps_journal_header header;
    Source source;
    const uint8 *src;

    source = PartitionGetRawSerialSource(PARTITION_SERIAL_FLASH, journal_ctx.partition);
    if (!source)
    {
        PRINT(("UPG: Failed to open PS Journal partition\n"));
        return FALSE;
    }

    src = SourceMap(source);
    if (!src)
    {
        PRINT(("UPG: Failed to map PS Journal partition\n"));
        return FALSE;
    }

    /* parse header */
    ByteUtilsMemCpyFromStream((uint8 *)&header, src, (sizeof(header) * 2));
    SourceDrop(source, (sizeof(header) * 2));

    PRINT(("UPG: PS journal : config_ver %u journal_ver %u\n", header.config_version, header.journal_version));

    /* parse any PS key actions in the journal */
    while (UpgradePsJournalProcessNextKeyAction(source))
    {
    }

    SourceClose(source);

    return TRUE;
}

/****************************************************************************
NAME
    UpgradePsJournalDestroy

DESCRIPTION
    Delete/remove the PS Journal data and leave it invalid.

    @TODO:  Probably shouldn't be destroying here. Destroy as part of the 
           "clean up for an upgrade"

RETURNS
    n/a
*/
void UpgradePsJournalDestroy(void)
{
    Sink sink;

    /* Open partition for writing - this will overwrite it. */
    sink = StreamPartitionOverwriteSink(PARTITION_SERIAL_FLASH, journal_ctx.partition);
    if (!sink)
        PRINT(("UPG: Failed to destroy PS Journal data\n"));

    SinkClose(sink);
}

/****************************************************************************
NAME
    UpgradePsJournalGetPsiFormatVersion

DESCRIPTION
    Get the PSI format version stored in the PSI.

RETURNS
*/
uint16 UpgradePsJournalGetPsiFormatVersion(void)
{
    uint16 len;
    uint16 psi_version = 0;
    uint16 *version;

    len = PsRetrieve(PSI_FORMAT_VERSION_KEY, NULL, 0);
    if (len < 6)
    {
        PRINT(("UPG: PSI format version not found in PS key 49, using 0 instead\n"));
        return psi_version;
    }

    version = (uint16 *)malloc(len * sizeof(uint16));
    if (version)
    {
        PsRetrieve(PSI_FORMAT_VERSION_KEY, version, len);
        psi_version = version[5];
        free(version);
    }

    return psi_version;
}

/****************************************************************************
NAME
    UpgradePsJournalSetPsiFormatVersion

DESCRIPTION
    Set the PSI format version stored in the PSI.

RETURNS
*/
void UpgradePsJournalSetPsiFormatVersion(uint16 psi_version)
{
    uint16 len;
    uint16 *version;

    len = PsRetrieve(PSI_FORMAT_VERSION_KEY, NULL, 0);
    if (len < 6)
    {
        PRINT(("UPG: PSI format version not found in PS key 49, appending it\n"));
        len = 6;
    }

    version = (uint16 *)malloc(len * sizeof(uint16));
    if (version)
    {
        PsRetrieve(PSI_FORMAT_VERSION_KEY, version, len);
        version[5] = psi_version;
        if (PsStore(PSI_FORMAT_VERSION_KEY, version, len) < len)
        {
            PRINT(("UPG: Error storing updated PSI_FORMAT_VERSION_KEY\n"));
        }
        free(version);
    }
}

/****************************************************************************
NAME
    UpgradePsJournalUpdatePSStore

DESCRIPTION
    This function will check if the PS implementation store needs to be
    updated as part of the ongoing update and if so, update it as needed.

    If it is interrupted, e.g. by a power cut, it can be called again on
    the next boot and will continue without error.

RETURNS
    TRUE if the PS implementation store was updated, FALSE otherwise.
*/
bool UpgradePsJournalUpdatePsStore(uint16 config_version)
{
    Sink sink;

    if (!UpgradePsJournalIsValid())
    {
        /* Need to read a new "PSI format version" from USER49 */
        uint16 curr_config_version = UpgradePsJournalGetPsiFormatVersion();

        PRINT(("UPG: PS Journal not valid; checking if it is needed.\n"));

        if (curr_config_version == config_version)
        {
            /* No change needed */
            PRINT(("UPG: no PS config update needed for %u == %u\n", curr_config_version, config_version));
            return FALSE;
        }
        else if (curr_config_version < config_version)
        {
            /* Process the necessary keys and write the new values to
               PS scratch partition. */
            PRINT(("UPG: creating PS Journal for %u < %u\n", curr_config_version, config_version));
            sink = UpgradePsJournalCreate(curr_config_version, config_version);
            if (!sink)
            {
                PRINT(("UPG: Failed to create PS Journal\n"));
                return FALSE;
            }

            /* Write out the actions to take in the journal */
            UpgradePsJournalReformatKeys(sink, curr_config_version, config_version);

            /* Complete the journal by closing it */
            UpgradePsJournalClose(sink);

            /* Set USER49 with new config version */
            UpgradePsJournalSetPsiFormatVersion(config_version);
        }
        else
        {
            PRINT(("UPG: app config older than PS config\n"));
            return FALSE;
        }
    }

    /* PS scratch data must be valid by now */
    PRINT(("UPG: applying PS Journal\n"));
    UpgradePsJournalApply();

    /*PRINT(("UPG: destroying PS Journal\n"));
    UpgradePsJournalDestroy();*/

    return TRUE;
}
