/*
 * Copyright (c) 2016 Qualcomm Technologies International, Ltd.
 * This file was automatically generated for firmware version 19.5
 */

#ifndef __BOOT_H

#define __BOOT_H

#include <csrtypes.h>
/*! @file boot.h @brief Control booting BlueCore with different active settings
**
**
These functions control the BlueCore bootmode on unified firmware.
Each bootmode has a unique view of the persistent store,
and so a single device could boot up as (for example) an HCI dongle
using USB in one bootmode and a HID dongle using BCSP in another bootmode.
Using BootSetPreservedWord(), it is possible to store a single word of
application specific state before booting the device into
a new bootmode. Once rebooted the application can use BootGetPreservedWord().
**
The word of preserved state will only be valid (non-zero) if the device
was warm reset using BootSetMode(). As any other form of reset will
invalidate the preserved word contents and thus, BootGetPreservedWord() will return zero.
We recommend using BootSetPreservedWord() followed closely by BootSetMode()
to ensure the relevance of the preserved word contents.
VmGetResetSource() can be used to find out other causes of a reset.
**
Incorrect use of these functions could produce a non-functional module, and so caution should be exercised.
**
*/


/*!
   @brief Read the current boot mode.
*/
uint16 BootGetMode(void);

/*!
  @brief Set the mode and force a reboot using a warm reset.
  @param newBootmode The new boot mode to use.

*/
void BootSetMode(uint16 newBootmode);

/*!
   @brief Read the (single) word preserved between warm resets caused by BootSetMode(). 

   \note
   Any other boot causes will invalidate the preserved word and return 0.
*/
uint16 BootGetPreservedWord(void);

/*!
   @brief Write the (single) word preserved between warm resets caused by BootSetMode().
   @param state The word of state to preserve.

   \note
   This word will be invalidated if the device resets for other reasons. See VmGetResetSource().
*/
void BootSetPreservedWord(uint16 state);

#endif
