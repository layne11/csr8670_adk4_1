/****************************************************************************

        Copyright (c) 2001 - 2016 Qualcomm Technologies International, Ltd.
        All Rights Reserved.
        Qualcomm Technologies International, Ltd. Confidential and Proprietary.

FILE
    pio_if.h

CONTAINS
    Definitions for the pio subsystem.

DESCRIPTION
    This file is seen by the stack, and VM applications, and
    contains things that are common between them.
*/

/*!
 @file pio_if.h
 @brief Parameter for PioSetFunction().

*/

#ifndef __PIO_IF_H__
#define __PIO_IF_H__

/*! @brief Functions which can be mapped onto a pin. This is used in PioSetFunction().
 *   The user must consult the relevant device datasheet for possible mappings
 */
typedef enum 
{
    UART_RX,
    UART_TX,
    UART_RTS,
    UART_CTS,
    PCM_IN,
    PCM_OUT,
    PCM_SYNC,
    PCM_CLK,
    SQIF,
    LED,
    LCD_SEGMENT,
    LCD_COMMON,
    PIO,
    SPDIF_RX,
    SPDIF_TX
}pin_function_id;

#endif /* __PIO_IF_H__  */
