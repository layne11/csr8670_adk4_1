/****************************************************************************
Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.1

FILE NAME
    upgrade_ps_journal.h
    
DESCRIPTION
    As the final step of an upgrade it may be necessary to update the format
    of selected PS Keys in the implementation store to the new binary format
    that the updated app expects. In theory this should never happen but it
    practice it does and needs to be handled.
    
    Updating the keys is done in a two-step process, using the App PS Scratch
    partition to store the new keys until they have all been re-written to the
    PS implementation store. This way any interruption during the conversion, 
    e.g. power loss, will not affect the end result.
*/
#ifndef UPGRADE_PS_JOURNAL_H_
#define UPGRADE_PS_JOURNAL_H_

#include <csrtypes.h>
#include <message.h>

/*!
    @brief List of possible actions to perform on a PS key.
*/
typedef enum
{
    UPGRADE_PS_JOURNAL_ACTION_DELETE = 0,
    UPGRADE_PS_JOURNAL_ACTION_ADD = 1,
    UPGRADE_PS_JOURNAL_ACTION_REPLACE = 2,
    
    UPGRADE_PS_JOURNAL_ACTION_END_OF_LIST = 0xFF
} upgrade_ps_journal_action;


/*!
    @brief Initialise the PS Journal module
    
    @param task Task used to service PS Journal messages
    @param ps_scratch_partition External flash partition to store the
                                PS journal on.
*/
void UpgradePsJournalInit(Task task, uint16 partition);

/*!
    @brief Check if the App PS scratch partition holds a valid set of
           PS Key conversions.
           
    @return TRUE if there are PS Key conversions to be processed.
*/
bool UpgradePsJournalIsValid(void);

/*!
    @brief Apply the PS Journal changes stored in the App PS Scratch partition
           to the PS Implementation Store.

    @return TRUE if successful, FALSE otherwise.
*/
bool UpgradePsJournalApply(void);

/*!
    @brief Clear/delete the PS Journal in the App PS Scratch partition.
*/
void UpgradePsJournalDestroy(void);

/*!
    @brief Create the PS Journal (if needed) to store the PS Key
           changes to convert the PS Implementation store from the
           current version to the upgrade version.
           
    @return Valid sink if successful, NULL otherwise.
*/
Sink UpgradePsJournalCreate(uint16 curr_config_version, uint16 config_version);

bool UpgradePsJournalClose(Sink sink);

/*!
    @brief Write an action to perform on a PS key to the PS Journal.
    
    @return TRUE if action was written to the stream, FALSE otherwise.
*/
bool UpgradePsJournalWriteKeyAction(Sink sink, uint16 key, upgrade_ps_journal_action action, uint8 *data, uint16 len);

/*!
    @brief Get the PS Implementation Store (PSI) format version stored in the PSI.
    
    @return Current PSI format version.
*/
uint16 UpgradePsJournalGetPsiFormatVersion(void);

/*!
    @brief Set the PS Implementation Store (PSI) format version stored in the PSI.

    @param psi_version The PSI format version to store in the PSI.
*/
void UpgradePsJournalSetPsiFormatVersion(uint16 psi_version);

/*! 
    @brief Do any necessary PS Key format conversion for the in-progress upgrade.

    @param config_version The config version (CONFIG_SET_VERSION) of the
                          running app.
                          
    @return TRUE if the PS implementation store is now in the correct format
            for the new config version. FALSE otherwise.
*/
bool UpgradePsJournalUpdatePsStore(uint16 config_version);


#endif /* UPGRADE_PS_JOURNAL_H_ */
