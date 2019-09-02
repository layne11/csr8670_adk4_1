/*
 * Copyright (c) 2016 Qualcomm Technologies International, Ltd.
 * This file was automatically generated for firmware version 19.5
 */

#ifndef __PARTITION_H

#define __PARTITION_H

#include <csrtypes.h>
#include <app/partition/partition_if.h>
#include <sink_.h>
#include <source_.h>


/*!
  @brief Mount a partition to the union file system

  @param device device to which to mount, see #partition_filesystem_devices
  @param partition number of partition to mount
  @param priority mount at higher or lower priority to already mounted file systems,
  see #partition_filesystem_priority

  @return TRUE if partition found and mounted successfully, otherwise FALSE
*/
bool PartitionMountFilesystem(partition_filesystem_devices device, uint16 partition,  partition_filesystem_priority priority);

/*!
  @brief Get information about a partition

  @param device device to query, see #partition_filesystem_devices
  @param partition number of partition to query
  @param key specifies information requested, see #partition_info_key
  @param value returned value as specified by #key

  @return TRUE if partition found and queried successfully, otherwise FALSE
*/
bool PartitionGetInfo(partition_filesystem_devices device, uint16 partition, partition_info_key key, uint32 *value);

/*!
  @brief Set a message digest for a stream writing to flash partition.
  Message digest data will be copied and stored by the firmware.
  
  When a flash partition is written to, the first word is not immediately
  written. Instead it is saved in RAM until the stream is closed. At this
  point the flash partition is read back and verified against the stored
  message digest. If the verification is successful, the first word is
  written to flash.
  
  This protects against partly or incorrectly written partitions.
  If this trap is not called before closing the partition sink then the
  verification will not be performed, also the first word will not be written.

  @param sink sink that is writing to the partition
  @param md_type the type of message digest, see #partition_message_digest_type
  @param data pointer to message digest
  @param value length of message digest

  @return TRUE if sink is valid, data is correct length and message digest set successfully, otherwise FALSE

  \note
  Message digest configuration using PartitionSetMessageDigest() can only be done once
  in the life time of the partition sink.
*/
bool PartitionSetMessageDigest(Sink sink, partition_message_digest_type md_type, uint16 *data, uint16 len);

/*!
  @brief This API will return the source with the contents of the specified raw serial partition 

  @param device device to query, see #partition_filesystem_devices
  @param partition number of partition to read raw data

  @return The source associated with the raw partition stream.
*/
Source PartitionGetRawSerialSource(uint16 device, uint16 partition);

/*!
  @brief Get the sink position of the partition sink stream.

  @param partition sink stream.

  @return sink position of the specified sink partition.

  This VM trap is used to get the sink position of the specified sink partition in 
  octets. Firmware returns a sink position from which, each word has a value of 
  0xFFFF till the end of the partition. Firmware assumes that the partition 
  contents before the sink position (except first word and last odd octet, if 
  any) are successfully written.

*/
uint32 PartitionSinkGetPosition(Sink sink);

/*!
  @brief Unmount a partition from the union file system

  @param device device from which to unmount, see #partition_filesystem_devices
  @param partition filesystem partition number to unmount

  @return TRUE if partition found and unmounted successfully, otherwise FALSE
*/
bool PartitionUnmountFilesystem(partition_filesystem_devices device, uint16 partition);

/*!
  @brief Creates raw source stream out of any closed or partially written partition.
  @param device device to query, see #partition_filesystem_devices
  @param partition partition number to read raw data
  @return Source associated with the partition stream.
  
  This trap creates source out of any closed partition, but any partially
  written partition would require a sink prior to creating a source from
  the same partition. Once the source stream is created out of a partially
  written partition, the associated sink stream will be made idle for the
  span of source stream lifetime. In other words, this sink will not be
  accessible to VM or transforms.

  @note
  Raw source cannot be created out of a PS partition.

  @note
  Raw source stream out of a partially written partition can only be created
  if there is no space claimed with associated sink stream.

  @note
  Raw source stream can also be created out of mounted Read only file system
  partition.
*/
Source PartitionGetRawSource(uint16 device, uint16 partition);

#endif
