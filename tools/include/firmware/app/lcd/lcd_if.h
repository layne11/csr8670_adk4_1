/****************************************************************************

        Copyright (c) 2010 - 2016 Qualcomm Technologies International, Ltd.
        All Rights Reserved.
        Qualcomm Technologies International, Ltd. Confidential and Proprietary.

FILE
    lcd_if.h

CONTAINS
    Definitions for the LCD subsystem.

DESCRIPTION
    This file is seen by the stack, and VM applications, and
    contains things that are common between them.
*/

/*!
 @file lcd_if.h
 @brief Parameters for LcdConfigure() and LcdSet().
**
**
 Some BlueCore variants have a dedicated LCD block which may be routed to
 PIO pads. This routing is controlled by the VM trap PioSetLcdPins().
 After PioSetLcdPins() has been successfully called, the LCD segments may be
 turned on/off using the VM trap LcdSet().

 It is not necessary to map pins using PioSetMapPins32() - this is handled by
 PioSetLcdPins().

 Consult the datasheet for your BlueCore variant to identify precisely how
 the LCD segments are internally connected to which PIOs.
*/

#ifndef __LCD_IF_H__
#define __LCD_IF_H__

/*! @brief General configuration function for LCD.
    These enumerations are used for key in the VM trap
    LcdConfigure( key, value ).

    #LCD_CLK_DIV is a 4-bit value used to set the LCD block's internal clock
    divide ratio, to enable a wide range of LCD parts to be accommodated.
 */
typedef enum
{
 /*!
     Sets lcd block clock divider using value1 in the VM trap
     LcdConfigure(LCD_CLK_DIV, value)
     The frequency of segment oscillation is
       Seg Freq = (XTAL-FREQ / 16 ) / 2^(value1 + 1).

     Value1 is a 4-bit value, this gives the following range of possible
     segment frequencies with 26MHz Xtal:
      value = 0x0,  Seg Freq = 812.5 kHz
      value = 0xF,  Seg Freq = 24.8 Hz
     No range-checking is performed on value and the trap always returns zero.
     */
    LCD_CLK_DIV
} lcd_config_key;

/*! @brief Using VM trap LcdSet(mask32, value32) to turn on/off LCD segments
    mask32 = bitwise representation of PIOs that need writing, where b31=PIO31
    thru b0 = PIO0. Only *segments* (not the common) that have been
    successfully routed to PIOs using PioSetLcdPins(..) may be set here.
    value32 = bitwise representation of PIOs' values. b31=PIO31, b0=PIO0.
    Use a 1 to enable the segment, a 0 to turn it off.
    Only those bits set in mask32 are acted upon.
    If the Trap call was successful it returns result32 = 0. Otherwise it
    returns erroneous bits and no action is performed.
 */

#endif /* __LCD_IF_H__ */
