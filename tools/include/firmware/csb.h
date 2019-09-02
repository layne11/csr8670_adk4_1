/*
 * Copyright (c) 2016 Qualcomm Technologies International, Ltd.
 * This file was automatically generated for firmware version 19.5
 */

#ifndef __CSB_H

#define __CSB_H

#include <csrtypes.h>
#include <bdaddr_.h>
#include <csb_.h>


/*!
  @brief Apply AFH map data on CSB receiver.
  @param remote_addr The remote device bluetooth address.
  @param lt_addr The logical transport address used for CSB link.
  @param afh_map New AFH channel map data.
  @param afh_instant Piconet clock at which new AFH channel map is to be
  applied.
  @return False if addr and lt_addr do not identify an existing CSB transport
  otherwise TRUE.
  \note
  1. If the afh_instant provided is already passed then new channel map is
     applied immediately.
  2. If afh_instant for previous set channel map command is not reached and
     new command is received, then new channel map and instant will overwrite
     the previous one. In this case the older channel map will not be applied.
     Firmware will only store and apply the latest channel map at it's instant.
  3. In order to apply AfhMap, minimum 20 channels are required to be set
     otherwise AfhMap will not be applied.
*/
bool CsbReceiverSetAfhMap(const bdaddr *remote_addr, uint16 lt_addr, AfhMap *afh_map, uint32 afh_instant);

/*!
  @brief Apply AFH map data on CSB transmitter.
  @param lt_addr The logical transport address used for CSB link.
  @param afh_map New AFH channel map data.
  @param afh_instant Piconet clock at which new AFH channel map is to be
  applied.
  @return False if lt_addr do not identify an existing CSB transport otherwise
  TRUE.
  \note
  1. If the afh_instant provided is already passed then new channel map is
     applied immediately.
  2. If afh_instant for previous set channel map command is not reached and
     new command is received, then new channel map and instant will overwrite
     the previous one. In this case the older channel map will not be applied.
     Firmware will only store and apply the latest channel map at it's instant.
  3. In order to apply AfhMap, minimum 20 channels are required to be set
     otherwise AfhMap will not be applied.
*/
bool CsbTransmitterSetAfhMap(uint16 lt_addr, AfhMap *afh_map, uint32 afh_instant);

/*!
  @brief Enables or disables new AFH map event at CSB transmitter/receiver.
  @param enable If enable (TRUE) then an event (i.e. HCI_CSB_AFH_MAP_AVAILABLE)
  should be raised when new AFH map is available at CSB transmitter/receiver.
*/
void CsbEnableNewAfhMapEvent(bool enable);

#endif
