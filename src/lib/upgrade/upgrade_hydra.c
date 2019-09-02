/****************************************************************************
Copyright (c) 2016 Qualcomm Technologies International, Ltd.
Part of ADK 4.1

FILE NAME
    upgrade_hydra.c
    
DESCRIPTION
	Items used by upgrade not directly supported by Hydra
*/
#include <stdio.h>
#include <vmtypes.h>
#include "upgrade_hydra.h"
#ifdef HYDRACORE

/**
 *  \brief Get information about a partition
 *  \param device device to query, see \#partition_filesystem_devices 
 *  \param partition number of partition to query 
 *  \param key specifies information requested, see \#partition_info_key 
 *  \param value returned value as specified by \#key
 * 
 * \ingroup trapset_partition
 * 
 * WARNING: This trap is UNIMPLEMENTED
 */
bool PartitionGetInfo(partition_filesystem_devices device, uint16 partition, partition_info_key key, uint32 * value)
{
	UNUSED(device);
	UNUSED(partition);
	UNUSED(key);
	UNUSED(value);
	return TRUE;
}

/**
 *  \brief This API will return the source with the contents of the specified raw serial
 *  partition 
 *  \param device device to query, see \#partition_filesystem_devices 
 *  \param partition number of partition to read raw data
 * 
 * \ingroup trapset_partition
 * 
 * WARNING: This trap is UNIMPLEMENTED
 */
Source PartitionGetRawSerialSource(uint16 device, uint16 partition)
{
	UNUSED(device);
	UNUSED(partition);
	return NULL;
}

/**
 *  \brief Set a message digest for a stream writing to flash partition. Message digest
 *  data will be copied and stored by the firmware. When a flash partition is
 *  written to, the first word is not immediately written. Instead it is saved in
 *  RAM until the stream is closed. At this point the flash partition is read back
 *  and verified against the stored message digest. If the verification is
 *  successful, the first word is written to flash. This protects against partly
 *  or incorrectly written partitions. If this trap is not called against a
 *  partition write sink, the verification will not be performed, but the first
 *  word will not be written until the stream is closed, protecting against partly
 *  written partitions.
 *  \param sink sink that is writing to the partition 
 *  \param md_type the type of message digest, see \#partition_message_digest_type 
 *  \param data pointer to message digest 
 *  \param len length of message digest
 * 
 * \ingroup trapset_partition
 * 
 * WARNING: This trap is UNIMPLEMENTED
 */
bool PartitionSetMessageDigest(Sink sink, partition_message_digest_type md_type, uint16 * data, uint16 len)
{
	UNUSED(sink);
	UNUSED(md_type);
	UNUSED(data);
	UNUSED(len);
	return TRUE;
}

/**
 *  \brief Get the sink position of the partition sink stream.
 *  \param sink sink stream.
 * 
 * \ingroup trapset_partition
 * 
 * WARNING: This trap is UNIMPLEMENTED
 */
uint32 PartitionSinkGetPosition(Sink sink)
{
	UNUSED(sink);
    return 0;
}

/**
 *  \brief Copy the specified memory buffer into PSKEY_FSTAB within the persistent store
 *  \param buff The memory buffer to copy data from. 
 *  \param words The number of words to copy. 
 *  \param commit Write the new FSTAB to non-volatile memory or not. If FALSE then the new FSTAB
 *  will only be stable across warm reboots; this feature can be used to enable
 *  the newly upgrade application to perform system checks before setting the
 *  FSTAB in the non-volatile store.
 * 
 * \ingroup trapset_partition
 * 
 * WARNING: This trap is UNIMPLEMENTED
 */
bool PsStoreFsTab(const void * buff, uint16 words, bool commit)
{
	UNUSED(buff);
	UNUSED(words);
	UNUSED(commit);
	return TRUE;
}

/**
 *  \brief Open a sink to erase and write to an external flash partition. This function
 *  will perform a flash erase on the entire partition specified and then provide
 *  a Sink to allow it to be written from the start.
 *  \param device device to which to write, cannot be internal flash, see
 *  \#partition_filesystem_devices
 *  \param partition partition number to overwrite
 * 
 * \ingroup trapset_partition
 * 
 * WARNING: This trap is UNIMPLEMENTED
 */
Sink StreamPartitionOverwriteSink(partition_filesystem_devices device, uint16 partition)
{
	UNUSED(device);
	UNUSED(partition);
	return NULL;
}

/**
 *  \brief Resume external flash sink partition after a controlled power failure.
 *  \param device device to which to write, cannot be internal flash, see
 *  \#partition_filesystem_devices. 
 *  \param partition_no partition number of the sink partition to be resumed. 
 *  \param first_word first word of the sink partition to be resumed.
 * 
 * \ingroup trapset_partition
 * 
 * WARNING: This trap is UNIMPLEMENTED
 */
Sink StreamPartitionResumeSink(partition_filesystem_devices device, uint16 partition_no, uint16 first_word)
{
	UNUSED(device);
	UNUSED(partition_no);
	UNUSED(first_word);
	return NULL;
}

#endif /* HYDRACORE */

