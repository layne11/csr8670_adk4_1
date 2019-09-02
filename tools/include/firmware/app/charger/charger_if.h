/****************************************************************************

        Copyright (c) 2005 - 2016 Qualcomm Technologies International, Ltd.
        All Rights Reserved.
        Qualcomm Technologies International, Ltd. Confidential and Proprietary.

FILE
    charger_if.h

CONTAINS
    Definitions for the charger subsystem.

DESCRIPTION
    This file is seen by the stack, and VM applications, and
    contains things that are common between them.
*/

/*!
 @file charger_if.h
 @brief Types used by the ChargerConfigure() and ChargerStatus() VM traps.

*/

#ifndef __APP_CHARGER_IF_H__
#define __APP_CHARGER_IF_H__

/*! @brief Status of the charger hardware as returned by the ChargerStatus()
    trap.
*/
typedef enum
{
    TRICKLE_CHARGE,    /*!< The battery is flat and being charged slowly. */
    FAST_CHARGE,       /*!< The battery is being charged quickly. */
    DISABLED_ERROR,    /*!< The charger is disabled or in an unreliable state. */
    STANDBY,           /*!< The battery is full and not being charged. */
    NO_POWER           /*!< The charger has no input power. */
} charger_status;

/*! @brief Charger event enables that can be passed to ChargerDebounce() */
typedef enum
{
    CHARGER_VREG_EVENT    = (1 << 0),
    CHARGER_CONNECT_EVENT = (1 << 1)
}charger_events;

/*! @brief Charger config keys - used to configure the charger.

For BlueCore chips which include battery charger hardware, the data
sheet will include a "Battery Charger" section which gives electrical
characteristics of the charger.
The "I-CTRL" values in this section should be used to set the
#CHARGER_CURRENT.
The Trickle charger voltage threshold and Float voltage for pre BlueCore7
chips are both set using #CHARGER_TRIM. On BlueCore7 and later devices
the Trickle charger voltage threshold and Float voltage are both set using
#CHARGER_TERM_VOLTAGE
 */
typedef enum
{

    /*!
When using this key, a zero value will disable the charger and
a non zero value will enable the charger.
     */
    CHARGER_ENABLE,

    /*!
Used to set the trim on pre BlueCore7 chips which have an onchip
battery charger. Using this key overrides any setting in PSKEY_CHARGER_TRIM.
Valid values are in the range 0..15 and the exact mapping from value to voltage
is chip dependent. Consult the datasheet for your chip.
     */
    CHARGER_TRIM,

    /*!
Used to set the charger current on BlueCore chips which have an onchip
battery charger. Using this key overrides any setting in
PSKEY_CHARGER_CURRENT. On BlueCore7 and later devices, the value
passed specifies the required charger current in mA and the supported
range is device dependent. Any invalid value results in rounding to
lower or upper limits. On earlier devices, Valid values are in the
range 0..15 and the exact mapping from value to current is chip
dependent. Consult the datasheet for your chip.
     */
    CHARGER_CURRENT,

    /*!
Used to remove control of LED0 from the battery charger
hardware and give control to the VM application. A zero value gives control
to the battery charger hardware (which may automatically flash the LED when
the battery charger is in operation) while a non zero value gives control to
the VM application.
     */
    CHARGER_SUPPRESS_LED0,

    /*!
Some BlueCore variants with battery charger hardware
have an option to boost the charging current. This function allows the 
application to switch the boost on or off. For this function to have any effect,
PSKEY_CHARGER_ENABLE_BOOST must be set to TRUE. In this case, as the
charger boost hardware requires use of the LED1 pin, the normal use of
LED1 is not available.
     */
    CHARGER_ENABLE_BOOST,

    /*!
Some BlueCore variants with battery charger hardware have
an option to use an external power transistor to boost the available charger current.
CHARGER_USE_EXT_TRAN selects between internal and external modes.  In
internal mode the current is limited but adjustable using CHARGER_CURRENT.
In external mode the current is fixed (set by a resistor on the PCB).
     */
    CHARGER_USE_EXT_TRAN,

    /*!
On BlueCore7 chips it is possible to reset the CPU on a charger-attach
event, this being enabled/disabled by PSKEY_RESET_ON_CHARGER_ATTACH.
In addition to the PSKEY, the VM may also independently enable or
disable the Charger-Attach-Reset using this VM Trap.
     */
    CHARGER_ATTACH_RESET_ENABLE,

    /*!
Used to set the termination voltage on Bluecore7 and later chips which
have on chip battery charger. The value passed to this function is in
millivolts and the supported range is device dependent. Any invalid
value results in rounding to lower or upper limits. The register
settings are calculated by using PSKEY_CHARGER_CALC_VTERM and
PSKEY_CHARGER_CALC_RTRIM for the given millivolts.
    */
    CHARGER_TERM_VOLTAGE,

    /*!
On some BlueCore variants like CSRB5730, this is being used to enable
the external FET to select higher external charge current when charger
hardware is configured for external mode.
    */
    CHARGER_ENABLE_HIGH_CURRENT_EXTERNAL_MODE,

    /*!
When in external charge mode, if the battery conditions are appropriate
for trickle charge the current used could be different to the current
used in internal trickle charge mode. Use this command to set the
trickle charge current value when in external mode. The value is in mA
and should be between 0 and 50. Values outside this range are ignored.
    */
    CHARGER_SET_EXTERNAL_TRICKLE_CURRENT,

    /*!
On some BlueCore variants like CSRB5730, When the battery has become
fully charged, the charger will entry its STANDBY phase. During this
phase it will monitor the battery's voltage. When it drops below a
threshold, it will restart fast charging. This threshold is configured
with the STANDBY_FAST_HYSTERESIS control.
The parameter is in millivolts and describes the threshold below VFLOAT
when the charger will begin charging again.
The limits are chip dependent.
On CSRB5730, the possible values are 120mV, 258mV, 344mV and 426mV below
VFLOAT. Attempting to set a value greater than 426mV will return FALSE 
and no changes will be made to the charger. Attempting to set a value
less than 120mV will cause the hysteresis to be set to 120mV below 
VFLOAT. Other values will cause the next lowest value from the list to
be selected.
    */
    CHARGER_STANDBY_FAST_HYSTERESIS



}charger_config_key;


/*! @brief Battery status keys - used to configure the charger.
*/
typedef enum
{
    CHARGER_BATTERY_UNKNOWN = 0,  /*!< The battery voltage was not checked  */
    CHARGER_BATTERY_OK = 1,       /*!< Battery voltage sufficient to use */
    CHARGER_BATTERY_DEAD = 2      /*!< Battery voltage too low at start (dead) */
} charger_battery_status;

#endif /* __APP_CHARGER_IF_H__  */
