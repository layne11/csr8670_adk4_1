/*
 * Copyright (c) 2016 Qualcomm Technologies International, Ltd.
 * This file was automatically generated for firmware version 19.5
 */

#ifndef __PIO_H

#define __PIO_H

#include <csrtypes.h>
#include <app/pio/pio_if.h>
/*! @file pio.h @brief Access BlueCore I/O lines
**
**
BlueCore variants from BlueCore2 onwards have twelve Programmable Input/Output(PIO)
pins and a further three or four pins (dependent on BlueCore variant) may
be available if they are not being used as AIO pins.
**
Some BlueCore variants from BlueCore5-MM onwards have 16 PIO pins and
can also control some of the chips UART/PCM lines by mapping them in
as PIO pins 16 and greater. Mapping a UART/PCM line as a PIO line stops
the line performing its original function, so these lines should only be
used as PIOs when these interfaces are not required.
**
Attempts to configure the upper 16 PIOs will be rejected if these
pins have not been mapped first.
**
Persistent Store keys may be set to hand control of certain PIO pins over
to the BlueCore firmware. Examples include PSKEY_USB_PIO_WAKEUP, PSKEY_USB_PIO_DETACH,
PSKEY_USB_PIO_PULLUP and PSKEY_USB_PIO_VBUS.
In such configurations the VM should not attempt to use these pins.
**
Various other hardware and software configurations make use of specific PIO pins.
Examples include
- Pio[1:0] Used to control external hardware on Class 1 modules.
- Pio[7:6] Used for I2C.
*/


/*!
   @brief Enables or disables the auxiliary DAC on BlueCore, setting the output
   level to the requested value. 

   @param enabled TRUE or FALSE.
   @param level The output level to use.

   @return TRUE if the request succeeded; FALSE if it
   could not be honoured because the DAC is in use by other parts of the firmware.

   Note that the auxiliary DAC level drops to 0V when BlueCore enters
   deep sleep for power-saving.
*/
bool PioSetAuxDac(bool enabled, uint8 level);

/*!
  @brief Configure PIO monitoring.

  @param mask Bitmask indicating which pins to monitor. Setting this
    to zero disables PIO monitoring.
  @param count How many times the monitored pins' state must be
    observed to be consistent before it is considered stable. Zero and
    one have special significance. If \e count is 1, then no
    debouncing is performed - any perceived change of the pins will
    raise the event. If \e count is 0, any transition raises the
    event, even if no change in pin state can be detected; in this
    case, the "state" members of #MessagePioChanged may show no change
    in state.
  @param period The delay in milliseconds between successive reads of
    the pins.
  @return A 32 bit mask. If any bit in this mask is high then
    monitoring could not be set up for that PIO; no action will have
    been taken on any PIOs (any previously set up monitoring will
    remain in force). The most likely reasons for a non-zero return
    are attempting to debounce nonexistent PIOs, or attempting to
    debounce unmapped PIOs. See the PioSetMapPins32() documentation
    for information on which PIOs can be mapped.

  This function configures a simple debounce engine for PIO input
  pins. It can detect transitions on PIOs and return stabilised
  readings for them, filtering out glitches.

  When the engine detects a change in state of one or more of the
  monitored pins, it repeatedly reads the pin state. If the state of
  all the monitored pins remains unchanged for \e count successive
  samples, it is considered stable and a #MESSAGE_PIO_CHANGED message
  is sent to the task registered with MessagePioTask().

  \note
  It is not possible to configure different debounce settings for
  different pins or groups of pins, or to debounce pins independently
  of each other.

  Examples:

  To enable events on PIO2 and PIO3, with 4 reads 2ms apart (6ms
  total):

  \code
  PioDebounce32(1u<<2 | 1u<<3, 4, 2);
  \endcode

  To enable events on any transition on PIO5 with no debouncing:

  \code
  PioDebounce32(1u<<5, 1, 0);
  \endcode
*/
uint32 PioDebounce32(uint32 mask, uint16 count, uint16 period);

/*!
  @brief Returns the contents of the PIO data input register.
  
  For PIOs set as outputs, this function will return the value last written 
  using PioSet32(). 
*/
uint32 PioGet32(void);

/*!
  @brief Modifies the contents of the PIO data output register.  
  PIO pins must be set to outputs via PioSetDir32() before they can be driven
  high or low through this trap.
  This trap also sets pull direction for PIOs used as inputs.

  @param mask Each bit in the mask corresponds to a PIO line. Bits set 
  to 1 in this mask will be modified. Bits set to 0 in this mask will
  not be modified.

  @param bits Each bit in the "bits" value corresponds to a PIO line. Bits 
  set to 1 in this value will result in that PIO line being driven high. Bits
  set to 0 in this value will result in that PIO line being driven low.

  @return A 32 bit mask. If any bit in this mask is high then that PIO could
  not be driven to the level specified; note that no action will have been 
  taken on any PIOs.

  Note that the upper 16 PIOs must be mapped in before they can be used.
  See the PioSetMapPins32() documentation for information on valid PIO directions
  and PIO mapping.  

  BlueCore has internal resistors which can be configured to either pull-up or 
  pull-down the pins used for input. This is controlled by the value 
  written to the output register using PioSet32().
  The resistors pull-down if the value is zero, and pull-up otherwise, so the 
  following fragment sets pins 1 and 2 to inputs with pin 1 configured to 
  pull-up and pin 2 configured to pull-down.

  \code

  PioSet32(2|4, 2);
  PioSetDir32(2|4, 0);

  \endcode
*/
uint32 PioSet32(uint32 mask, uint32 bits);

/*!
  @brief Read whether PIOs are set as inputs or outputs.
  @return Each bit in the return value corresponds to a PIO line. Bits set
  to 1 mean that PIO line is configured as an output. Bits set to 0 mean it is 
  configured as an input.
*/
uint32 PioGetDir32(void);

/*!
  @brief Set PIOs as inputs or outputs.

  @param mask Each bit in the mask corresponds to a PIO line. Bits set 
  to 1 in this mask will be modified. Bits set to 0 in this mask will
  not be modified.

  @param dir Each bit in the "dir" value corresponds to a PIO line. Bits set
  to 1 in this value will result in that PIO line being configured as an 
  output. Bits set to 0 in this value will result in that PIO line being 
  configured as an input.

  @return A 32 bit mask. If any bit in this mask is high then that PIO could
  not be set to the direction specified; note that no action will have been 
  taken on any PIOs.

  Note that the upper 16 PIOs must be mapped in before they can be used.
  See the PioSetMapPins32() documentation for information on valid PIO directions
  and PIO mapping.
*/
uint32 PioSetDir32(uint32 mask, uint32 dir);

/*!
  @brief Read whether PIOs are set to use strong or weak pull.
*/
uint32 PioGetStrongBias32(void);

/*!
  @brief Set PIOs to use strong or weak pull when used as inputs.
  
  @param mask Each bit in the mask corresponds to a PIO line. Bits set 
  to 1 in this mask will be modified. Bits set to 0 in this mask will
  not be modified.
  
  @param bits Each bit in the "bits" value corresponds to a PIO line. Bits set
  to 1 in this value will result in that PIO line being configured as having 
  strong bias. Bits set to 0 in this value will result in that PIO line being
  configured as not having strong bias.  

  @return A 32 bit mask. If any bit in this mask is high then that PIO could
  not be set to use strong bias; note that no action will have been taken on 
  any PIOs.
  
  BlueCore includes weak internal pull-ups or pull-downs on pins which are 
  being used as inputs (see PioSet32()). This function allows the pull-up or 
  pull-down to be made stronger on a per-pin basis.
  
  So to set pin 4 and 5 as inputs, 4 pulled up weakly, 5 pulled down strongly

  \code

   PioSetDir32((1<<4)|(1<<5), 0);
   PioSet32((1<<4)|(1<<5), (1<<4));
   PioSetStrongBias32((1<<4)|(1<<5), (1<<5));

  \endcode

*/
uint32 PioSetStrongBias32(uint32 mask, uint32 bits);

/*!
  @brief Returns a 32 bit value showing which PIO lines have been mapped to
  chip pins (see documentation for PioSetMapPins32() for more detail).
*/
uint32 PioGetMapPins32(void);

/*!
  @brief Cause the usual function of chip pins to be suppressed, and instead 
  make them behave as PIOs.

  @param mask Each bit in the mask corresponds to a PIO line. Bits set 
  to 1 in this mask will be modified. Bits set to 0 in this mask will
  not be modified.
  
  @param bits Each bit corresponds to a PIO line. A bit set to 1 will cause a 
  (non-PIO) chip pin to be behave as the corresponding PIO. A bit set to 0 
  will result in any mapped pin being returned to its original function.
  
  @return A 32 bit mask. If any bit in this mask is high then that PIO could
  not be mapped or unmapped; note that no action will have been taken on 
  any PIOs.

  For BC5-MM the PIO lines map to other pins as follows:

  (PIO 0..15) have no mapping. They are always PIO 0..15. They can be 
  configured as inputs or outputs.

  (PIO 16) maps to PCM_DATA. This can be configured as an input or an output.

  (PIO 17) maps to PCM_SYNC. This can be configured as an input or an output.

  (PIO 18) maps to UART_DATA_OUT. This can be configured as an input or an 
  output.

  (PIO 19) maps to PCM_CLK_OUT. Set this to output to the PCM_CLK pin. This 
  line is output only. Note that the PCM_CLK pin direction shadows PCM_SYNC 
  direction and ignores directions set via PioSetDir32().

  (PIO 20) maps to AIO0. Mapping this PIO will overwrite the value set in 
  PSKEY_AMUX_AIO0. Unmapping this PIO will restore the value set in 
  PSKEY_AMUX_AIO0. Note that on BC5-MM configuring AIO lines as PIO lines is 
  not recommended due to the low voltage level.

  (PIO 21) maps to AIO1. Mapping this PIO will overwrite the value set in 
  PSKEY_AMUX_AIO1. Unmapping this PIO will restore the value set in 
  PSKEY_AMUX_AIO1. Note that on BC5-MM configuring AIO lines as PIO lines is 
  not recommended due to their low voltage level.

  PIO lines above 21 map to nothing and cannot be mapped or written. 

  For CSR8670 the PIO lines map to other pins as follows:

  (PIO 0..12) have no mapping. They are always PIO 0..12.
  They can be configured as inputs or outputs.
  The smaller packages such as Chip Scale Package (CSP) does not have PIO8..12.

  (PIO 13..15) may be mapped if required. The exact signal routing is dependent
  on which package is being used. On smaller packages, such as CSP, you must
  map PIO13-15 if you want PIO instead of UART UART_RX, UART_TX and UART_CTS.
  On the BGA package PIO13..15 have their own pins, but if mapped, will be 
  connected to the UART_RX, UART_TX and UART_CTS pins as well. Whether mapped
  or not, these PIO pins may be configured as inputs or outputs. For each pin,
  if mapped and set as output, both (UART and PIO) pins are driven. If mapped
  and set as input, the UART pin is connected and the PIO pin is n/c.

  (PIO 16) maps to the UART_RTS pin. This can be configured as an input or an
  output.

  (PIO 17) maps to the PCM_IN pin. This can be configured as an input or an
  output.

  (PIO 18) maps to the PCM_OUT pin. This can be configured as an input or an 
  output.

  (PIO 19) maps to the PCM_SYNC pin. This can be configured as an input or an 
  output.

  (PIO 20) maps to the PCM_CLK pin. This can be configured as an input or an 
  output.

  (PIO 21) maps to the SQIF Flash Clock pin. This can be configured as an input
   or an output.

  (PIO 22) maps to the SQIF RAM Clock pin. This can be configured as an input
  or an output.  
  
  (PIO 23) maps to the SQIF Flash CS pin. This can be configured as an input
  or an output.  
  
  (PIO 24) maps to the SQIF RAM CS pin. This can be configured as an input or
  an output.

  (PIO 25) maps to the SQIF DB0 pin. This can be configured as an input or an 
  output.

  (PIO 26) maps to the SQIF DB1 pin. This can be configured as an input or an 
  output.
  
  (PIO 27) maps to the SQIF DB2 pin. This can be configured as an input or an 
  output.
  
  (PIO 28) maps to the SQIF DB3 pin. This can be configured as an input or an 
  output.

  PIO lines above 28 map to nothing and cannot be mapped or written.
*/
uint32 PioSetMapPins32(uint32 mask, uint32 bits);

/*!
   @brief Controls the settings of the RTS line

   @param level Set (TRUE) or Clear (FALSE).

   @return TRUE on success, and FALSE if the operation
   could not be performed. 

   When the host transport is set to none, so that the UART is not
   being used, the application has full control of the RTS line.

   When the user transport is in use, this function can be used to
   assert flow control when the firmware would not automatically do
   so. However, the firmware may also force RTS low to assert incoming
   flow control even if PioSetRts(TRUE) has been called.

   With other transports (including USB), it is not possible to
   control the RTS line, and FALSE will be returned.
*/
bool PioSetRts(bool level);

/*!
    @brief Lets the VM check the status of the Clear To Send UART pin

    @return TRUE if host transport is set to none and CTS input
    signal within the UART is set to active, else returns FALSE.
*/
bool PioGetCts(void);

/*!
  @brief Find out which pins are under kalimba control.
  
  Note: Only some BlueCore variants have access to more than 16 PIO lines.
*/
uint32 PioGetKalimbaControl32(void);

/*!
  @brief Modify which pins are under control of the Kalimba DSP.
  @param mask The bit mask to use.
  @param value The pins to set.

  Aspects the DSP has control over include the direction (input or output)
  of a PIO, and the level driven when used as an output.
  
  @return A 32 bit mask. If any bit in that mask is high then control 
  of that PIO could not be given to Kalimba and the trap call failed.
  
  Note: Only some BlueCore variants have access to more than 16 PIO lines.
*/
uint32 PioSetKalimbaControl32(uint32 mask, uint32 value);

/*!
  @brief  Sets a supported function for particular pio pin
  
  @param pin           - pin that requires a function change
                         the pin value ranges from (0..15) or (0..31)
                         depending upon the package
  @param function      - Supported function that needs to be set for 
                         the specified pin. Refer #pin_function_id

  @return TRUE if successful, else FALSE.

  if a pin can be mapped as UART_TX/UART_RX/PCM_IN etc,  
  then this function can be used to set the pin for one of the 
  supported functions.

  Consult the device's data sheet to understand what functions are 
  supported for each PIO pins.

  A pin can support only few functions. Trying to set a function 
  which is NOT supported by the pin will return FALSE without 
  affecting/modifying the existing pin function.

  If a pin supports only two functions i.e., PIO/UART_RX then 
  PioSetMapPins32() is sufficient to map the pin as UART_RX/PIO.
  See the PioSetMapPins32() documentation for more information.
  
  if a pin supports more than two functions and currently, it is 
  mapped to a pio line, then pin should be unmapped using 
  PioSetMapPins32() and then set the supported function for the 
  corresponding pin by calling PioSetFunction().

  PioSetFunction() Usage Example:
  if PIO[3] can be mapped to UART_RX/PCM_IN/PIO and 
  currently PIO[3] is mapped as PIO, then VM App should call the 
  PioSetMapPins32() to unmap it from PIO and then call 
  PioSetFunction() to map to (UART_RX/PCM_IN) function.

  Unmap PIO[3] so that it can be mapped to a function
  PioSetMapPins32(1<<3, 0<<3);

  This will map PIO[3] line as UART_RX
  PioSetFunction(3, UART_RX);  

  To map back the PIO[3] as a PIO, PioSetMapPins32() should be used.
  PioSetMapPins32(1<<3, 1<<3);

*/
bool PioSetFunction(uint16 pin, pin_function_id function);

/*!
    @brief Determine which physically existing PIOs are currently not
     in use by firmware or VM apps.
    
    @return Those PIOs which are available for use.
     b31 = PIO31 thru b0 = PIO0. A '1' indicates available.
*/
uint32 PioGetUnusedPins32(void);

/*!
    @brief Grabs PIOs for use by the LCD block.
    @param mask These are the required LCD segments. b0-PIO0, b31=PIO31.
    @param pins These should be set to one.
    @param common_pin The PIO that will be connected to the LCD common.
    @return zero if successful, otherwise bad bits returned and nothing done.
*/
uint32 PioSetLcdPins( uint32 mask, uint32 pins, uint16 common_pin );

#endif
