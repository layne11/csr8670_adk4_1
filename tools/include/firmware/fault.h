/*
 * Copyright (c) 2016 Qualcomm Technologies International, Ltd.
 * This file was automatically generated for firmware version 19.5
 */

#ifndef __FAULT_H

#define __FAULT_H

#include <csrtypes.h>
/*! @file fault.h @brief Interface to firmware fault information.*/


/*!
  @brief Reads the last fault code and *can* clear it.
  @param clear_fault Value TRUE to clear the last fault code after reading it,
  otherwise FALSE.
  @return The last fault code.

  @note
  The fault code is not cleared at boot-up (due to soft reset or hard reset).
  Therefore, the VM application can get stale value from power-on or repeated
  value over multiple power cycles if it was not cleared after boot-up.

  When PSKEY_PANIC_ON_FAULT is set to TRUE, BlueCore system will
  panic immediately when any fault condition is encountered.
  VmGetResetSource call would return reset source as RESET_SOURCE_FIRMWARE.
  VM application can read the fault code and panic code to find the
  reset reason.
*/
uint16 FaultGetAndClearFirmwareLastCode(bool clear_fault);

#endif
