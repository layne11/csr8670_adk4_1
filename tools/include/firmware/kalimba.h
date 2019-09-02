/*
 * Copyright (c) 2016 Qualcomm Technologies International, Ltd.
 * This file was automatically generated for firmware version 19.5
 */

#ifndef __KALIMBA_H

#define __KALIMBA_H

#include <csrtypes.h>
/*! @file kalimba.h @brief Control of the Kalimba DSP */
#include <app/file/file_if.h>


/*!
  @brief Loads the specified DSP code into Kalimba and sets it running at full
  speed with Kalimba in control of the clock.

  @param file The DSP code to load.

  @return TRUE if the DSP was successfully started, FALSE otherwise.

  \note
  The time taken to start the DSP application depends on the details of the DSP
  application, which can not be predicted by the BlueCore firmware. If the VM
  software watchdog is in use, the VM application should consider the time
  taken to start the DSP application when deciding on the timeout value. The
  BlueCore firmware will not automatically extend the timeout.
*/
bool KalimbaLoad(FILE_INDEX file);

/*!
   @brief Turns off power to Kalimba 
*/
void KalimbaPowerOff(void);

/*!
  @brief Sends a four word message to Kalimba. 
*/
bool KalimbaSendMessage(uint16 message, uint16 a, uint16 b, uint16 c, uint16 d);

/*!
  @brief Send a long message to Kalimba

  @param message the id of the message
  @param len     the length of the data (limited to 64)
  @param data    the actual data to be sent

  @return TRUE if the message was sent, FALSE if the send failed.
*/
bool KalimbaSendLongMessage(uint16 message, uint16 len, const uint16 *data);

#endif
