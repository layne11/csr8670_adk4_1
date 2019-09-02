/****************************************************************************
Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.1

FILE NAME
    upgrade_psstore.c

DESCRIPTION

    Implementation of an interface to Persistent Storage to get
    details of the file system and anything else related to the
    possibilities of upgrade.

NOTES
    Errors. Cause panics. End of. This behaviour in itself is problematic
            but if we are trying to switch applications then an error
            should indicate a reason to restart. We can't really
            delegate this to the VM app. We can't wind back to a previous
            application.
    Caching. Persistent store keys are not cached. There isn't a mechanism
            to subscribe to PSKEY changes. Since we don't actually expect
            to be called that frequently it makes sense to access the keys
            we need when we need them.
*/


#include <stdlib.h>
#include <string.h>
#include <csrtypes.h>
#include <panic.h>
#include <ps.h>

#include <print.h>
#include <upgrade.h>

#include "upgrade_ctx.h"
#include "upgrade_fw_if.h"
#include "upgrade_psstore.h"
#include "upgrade_psstore_priv.h"
#include "upgrade_partitions.h"
#include "upgrade_hydra.h"

/* details for accessing and storing the FSTAB from persistent store.
 *
 * we expect the FSTAB to consist of
 *      Entry 0 - partition for app to run on boot
 *      Entry ... - Other partitions
 *
 * The rest of the contents are not considered at present and
 * differences beyond the two application entries are ignored as they are
 * not important to upgrade. This would also allow for the application
 * revising the FS in future, by changing its size.
 *
 * As of writing, there is an issue with including the partition
 * to be used to store the upgraded app in the FSTAB. During boot all
 * partitions get mounted before checking if they are executable.
 * However, a mounted partition cannot be opened for writing, thus
 * blocking us from using any partition listed in the FSTAB for
 * storing the upgraded app in.
 *
 * So for now we workaround this by saying that the two app partitions
 * are "1001" and "1002" (SQIF partitions 1 and 2). The code below flips
 * the value of entry 0 in FSTAB to one or the other depending on which
 * partition the app is to be run from. This leaves the other partition
 * un-mounted and available for writing.
 */
#define PSKEY_FSTAB             (0x25E6)

#define VM_APP_INDEX_CURRENT  0

/* Minimum size is set as all known use cases of FSTAB require an
  entry of 0000. If that is the only entry (size 1) then there is 
  nothing for us to do so why bother loading. */
#define FSTAB_MINIMUM_SIZE    2

/* Utility functions */
static bool loadFstab(FSTAB_COPY *fstab,PsStores store);

/****************************************************************************
NAME
    UpgradeLoadPSStore  -  Load PSKEY on boot

DESCRIPTION
    Save the details of the PSKEY and offset that we were passed on 
    initialisation, and retrieve the current values of the key.

    In the unlikely event of the storage not being found, we initialise
    our storage to 0x00 rather than panicking.
*/
void UpgradeLoadPSStore(uint16 dataPskey,uint16 dataPskeyStart)
{
    union {
        uint16      keyCache[PSKEY_MAX_STORAGE_LENGTH];
        FSTAB_COPY  fstab;
        } stack_storage;
    uint16 lengthRead;

    UpgradeCtxGet()->upgrade_library_pskey = dataPskey;
    UpgradeCtxGet()->upgrade_library_pskeyoffset = dataPskeyStart;

    /* Worst case buffer is used, so confident we can read complete key 
     * if it exists. If we limited to what it should be, then a longer key
     * would not be read due to insufficient buffer
     * Need to zero buffer used as the cache is on the stack.
     */
    memset(stack_storage.keyCache,0,sizeof(stack_storage.keyCache));
    lengthRead = PsRetrieve(dataPskey,stack_storage.keyCache,PSKEY_MAX_STORAGE_LENGTH);
    if (lengthRead)
    {
        memcpy(UpgradeCtxGetPSKeys(),&stack_storage.keyCache[dataPskeyStart],
                UPGRADE_PRIVATE_PSKEY_USAGE_LENGTH_WORDS*sizeof(uint16));
    }
    else
    {
        memset(UpgradeCtxGetPSKeys(),0x0000,sizeof(UpgradeCtxGetPSKeys()) * sizeof(uint16));
    }

    /* Load the implementation FSTAB. */
    loadFstab(&stack_storage.fstab, ps_store_implementation);
}

/****************************************************************************
NAME
    UpgradeSavePSKeys  -  Save our PSKEYS

DESCRIPTION
    Save our PSKEYS into Persistent Storage.

    The existing contents of the key are read first. If they chance not to 
    exist the the value we do not control are set to 0x0000 (deemed safer 
    than panicking or using a marker such as 0xFACE)

    Note that the upgrade library initialisation has guaranteed that the 
    the pskeys fit within the 64 words allowed.
    
    Although not technically part of our API, safest if we allow for the 
    PSKEY to be longer than we use.
*/
void UpgradeSavePSKeys(void)
{
    uint16 keyCache[PSKEY_MAX_STORAGE_LENGTH];
    uint16 min_key_length = UpgradeCtxGet()->upgrade_library_pskeyoffset
                                    +UPGRADE_PRIVATE_PSKEY_USAGE_LENGTH_WORDS;

    /* Find out how long the PSKEY is */
    uint16 actualLength = PsRetrieve(UpgradeCtxGet()->upgrade_library_pskey,NULL,0);
    if (actualLength)
    {
        PsRetrieve(UpgradeCtxGet()->upgrade_library_pskey,keyCache,actualLength);
    }
    else
    {
        if (UpgradeCtxGet()->upgrade_library_pskeyoffset)
        {
            /* Initialise the portion of key before us */
            memset(keyCache,0x0000,sizeof(keyCache));
        }
        actualLength = min_key_length;
    }

    /* Correct for too short a key */
    if (actualLength < min_key_length)
    {
        actualLength = min_key_length;
    }

    memcpy(&keyCache[UpgradeCtxGet()->upgrade_library_pskeyoffset],UpgradeCtxGetPSKeys(),
                UPGRADE_PRIVATE_PSKEY_USAGE_LENGTH_WORDS*sizeof(uint16));
    PsStore(UpgradeCtxGet()->upgrade_library_pskey,keyCache,actualLength);
}


/****************************************************************************
NAME
    loadFstab  -  Load FSTAB data from the requested store

DESCRIPTION
    Checks whether the store requested is one of those that we support, then
    finds the length of the allocated data (if any), and loads the data from 
    Persistent store into the supplied structure (fixed size).

RETURNS
    TRUE if store was supported and an entry was found.
*/
static bool loadFstab(FSTAB_COPY *fstab,PsStores store)
{
PsStores    old_store;

    if (   (store == ps_store_implementation)
        || (store == ps_store_transient))
    {
        /* Since we are a library, get the old persistent store
         * so we can restore it  */
        old_store = PsGetStore();
        PsSetStore(store);

        /* Find size of storage */
        fstab->length = PsFullRetrieve(PSKEY_FSTAB,NULL,0);
        if (   (FSTAB_MINIMUM_SIZE <= fstab->length) 
            && (fstab->length <= sizeof(fstab->ram_copy)))
        {
            fstab->length = PsFullRetrieve(PSKEY_FSTAB,fstab->ram_copy,fstab->length);
            PsSetStore(old_store);
            return TRUE;
        }
        PsSetStore(old_store);
    }

    return FALSE;
}


/****************************************************************************
NAME
    UpgradeSetToTryUpgrades

DESCRIPTION
    Swaps the application entries in the temporary FSTAB so that the upgraded
    partitions are used on the next reboot.

    Ensures that the new temporary FSTAB is otherwise a copy of the
    implementation version.

RETURNS
    TRUE if the temporary FSTAB was updated with changed values.
*/
bool UpgradeSetToTryUpgrades(void)
{
    FSTAB_COPY fstab;
    bool changed = FALSE;

    PRINT(("UpgradeSetToTryUpgrades\n"));

    if (loadFstab(&fstab, ps_store_implementation))
    {
        uint16 i;

        for (i = 0; i < fstab.length;i++)
        {
            uint16 old = fstab.ram_copy[i];
            fstab.ram_copy[i] = UpgradePartitionsNewPhysical(fstab.ram_copy[i]);
            PRINT(("FSTAB[%d] = %04x (was %04x)\n",i,fstab.ram_copy[i],old));

            if (fstab.ram_copy[i] != old)
            {
                changed = TRUE;
            }
        }

        if (!PsStoreFsTab(fstab.ram_copy,fstab.length,FALSE))
        {
            changed = FALSE;
        }
    }
    
    return changed;
}


/****************************************************************************
NAME
    UpgradeCommitUpgrades

DESCRIPTION
    Writes the implementation store entry of the FSTAB so that the correct
    partitions are used after a power cycle.


    @TODO: This code MAY NOT BE good enough to deal with all the errors that can 
            happen.  Now includes synchronisation with the partitions table, but
            ...

RETURNS
    n/a
*/
void UpgradeCommitUpgrades(void)
{
    FSTAB_COPY fstab;

    if (   loadFstab(&fstab,ps_store_transient)
        && PsStoreFsTab(fstab.ram_copy,fstab.length,TRUE))
    {
        UpgradePartitionsCommitUpgrade();
        return;
    }
}


/****************************************************************************
NAME
    UpgradeRevertUpgrades

DESCRIPTION
    Sets the transient store entry of the FSTAB to match the implementation so 
    that the original partitions are used after a warm reset or power cycle.

    Note that the panic in the case of a failure to write will cause a reboot
    in normal operation - and a reboot after panic will clear the transient
    store.

RETURNS
    n/a
*/
void UpgradeRevertUpgrades(void)
{
    FSTAB_COPY fstab;

    if (loadFstab(&fstab,ps_store_implementation))
    {
        if (!PsStoreFsTab(fstab.ram_copy,fstab.length,FALSE))
        {
            Panic();    /** @todo Panic's bad */
        }
    }
}




/****************************************************************************
NAME
    UpgradePSSpaceForCriticalOperations

DESCRIPTION

    Checks whether there appears to be sufficient free space in the PSSTORE
    to allow upgrade PSKEY operations to complete.

RETURNS
    FALSE if insufficient space by some metric, TRUE otherwise.
*/
bool UpgradePSSpaceForCriticalOperations(void)
{
    uint16 keySize = UpgradeCtxGet()->upgrade_library_pskeyoffset 
                     + UPGRADE_PRIVATE_PSKEY_USAGE_LENGTH_WORDS;
    return (PsFreeCount(keySize) >= UPGRADE_PS_WRITES_FOR_CRITICAL_OPERATIONS);
}

