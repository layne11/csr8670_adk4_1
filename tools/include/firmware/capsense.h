/*
 * Copyright (c) 2016 Qualcomm Technologies International, Ltd.
 * This file was automatically generated for firmware version 19.5
 */

#ifndef __CAPSENSE_H

#define __CAPSENSE_H

#include <csrtypes.h>
#include <app/capsense/capsense_if.h>


/*!
    @brief Configure general parameters of touch sensor hardware.
    @param key Keys are defined in #capsense_config_key.
    @param value The value to set \e key to.

    @return TRUE if successful, else FALSE.
*/
bool CapsenseConfigure(capsense_config_key key, uint16 value);

/*!
    @brief Configure pad parameters of touch sensor hardware.
    @param pad Pad to configure.
    @param key Keys are defined in #capsense_config_pad_key.
    @param value The value to set \e key to.

    @return TRUE if successful, else FALSE.
*/
bool CapsenseConfigurePad(uint16 pad, capsense_config_pad_key key, uint16 value);

/*!
    @brief Get the current status of the capacitive touch pads.
    @param count The number of pad states to read.
    @param pads count values each specifying a pad.
    @param results count values to be written to.
    @return TRUE if successful, else FALSE.

    The current state of each of the pads will be written to the
    corresponding element of the results array.
*/
bool CapsensePadQuery(uint16 count, const uint16* pads, capsense_pad_state *results);

#endif
