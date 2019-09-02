/*
 * Copyright (c) 2016 Qualcomm Technologies International, Ltd.
 * This file was automatically generated for firmware version 19.5
 */

#ifndef __LCD_H

#define __LCD_H

#include <csrtypes.h>
#include <app/lcd/lcd_if.h>


/*!
    @brief Configures the LCD hardware.
    @param key Keys are defined in #lcd_config_key.
    @param value Depends on the key, and is defined in lcd_if.h
    @return TRUE if successful, otherwise FALSE
    More detailed information on the keys and values can be found in lcd_if.h
*/
bool LcdConfigure( uint16 key, uint16 value );

/*!
    @brief Controls the activation of lcd segments.
     These segments must have been specified using VM trap PioSetLcdPins().
    @param mask Each bit in the mask corresponds to a PIO line, where b0=PIO0.
    Bits set to 1 will be modified. Bits set to 0 will not be modified.
    @param value Each bit specifies a PIO's LCD activation: 1=on, 0=off.
    @return All zeros if successful, otherwise incorrect bits
*/
uint32 LcdSet(uint32 mask, uint32 value);

#endif
