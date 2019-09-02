/****************************************************************************

        Copyright (c) 2009 - 2016 Qualcomm Technologies International, Ltd.
        All Rights Reserved.
        Qualcomm Technologies International, Ltd. Confidential and Proprietary.

FILE
    led_if.h

CONTAINS
    Definitions for the led subsystem.

DESCRIPTION
    This file is seen by the stack, and VM applications, and
    contains things that are common between them.
*/

/*!
 @file led_if.h
 @brief Configuration parameters for LED control
 **
 **
 Some BlueCore variants (usually headset parts) have dedicated LED pads.
 In addition to simple ON/OFF controls, some BlueCore variants have hardware
 support for dimming and/or flashing these LEDs. The LEDs are configured using
 a key/value system with the LedConfigure() VM trap.

 Consult the datasheet for your BlueCore variant to identify how many LED pads
 it supports.
*/

#ifndef __LED_IF_H__
#define __LED_IF_H__

/* Note that code in bccmd_process.c and vm_trap_led.c relies on 
 * LED_0 == 0, LED_1 == 1, etc */

/*! @brief IDs used to configure the LED pads with the LedConfigure() trap. Some
BlueCore variants support no LED pads at all, some support two LED pads
(LED_0 and LED_1) and some support three LED pads.*/
typedef enum 
{
    LED_0,
    LED_1,
    LED_2,
 /* Some chips can have 6 LEDs */
    LED_3,
    LED_4,
    LED_5,
    NUM_OF_LEDS
}led_id;

/*! @brief LED config keys.

#LED_DUTY_CYCLE and #LED_PERIOD are used to dim the LED brightness. In order to use
these keys, an LED must be enabled by calling LedConfigure(led_id, LED_ENABLE, 1);

#LED_FLASH_ENABLE and #LED_FLASH_RATE are used to flash the LEDs.
In order to use these keys, an LED must be enabled.
 */
typedef enum 
{
    /*!
When using this key, a zero value will disable the LED pad and a
non zero value will enable the LED pad.

Examples:

\code
LedConfigure(LED_0, LED_ENABLE, 1);  Turn on LED 0

LedConfigure(LED_0, LED_ENABLE, 0);  Turn off LED 0
\endcode
     */
    LED_ENABLE,
    
    /*!
LED_DUTY_CYCLE controls the the PWM duty cycle. Valid values with this key
range from 0..0xFFF. 0x0 turns the LED off fully, 0xFFF turns the LED on fully. All other
values set the duty cycle to the value /4096.
     */
    LED_DUTY_CYCLE,
    
    /*!
LED_PERIOD controls the PWM period. Valid values with this key range from 0..0xF.
pwm_period is calculated as follows:

pwm_period = clock_period * 4096 * 2^LED_PERIOD

Where clock_period is 1/26MHz for CSR8670/CSR8610 and 1/16MHz for earlier.

CSR8670/CSR8610:  The resultant range of values is 157uS...5.162S at all times.
There is a normalisation adjustment that checks that
LED_FLASH_RATE >= LED_PERIOD. The normalisation check will increase
LED_FLASH_RATE if necessary, and occurs whenever LED_PERIOD is configured.
The normalisation adjustment can be circumvented by configuring LED_PERIOD 
before configuring LED_FLASH_RATE.

Pre-CSR8670/CSR8610:  The resultant range of values is 256us...8.39s when
BlueCore is running at full speed. The clock period will be longer when
BlueCore is in shallow or deep sleep.


     */
    LED_PERIOD,

    /*!
When using this key, a zero value will disable the LED
flashing hardware and a non zero value will enable the flashing hardware.
     */
    LED_FLASH_ENABLE,

    /*!
Set the rate to flash/pulse LED. Valid values to use with
this key range from 0..0xF.

LED_FLASH_RATE sets the flash pulse period as a multiple of pwm_period.

Led Flash Rate is calculated as follows:

Led Flash Period = pwm_period * 96 * 2^LED_FLASH_RATE

Where pwm_period is as calculated above.

And so, LED_FLASH_RATE maps to a multiplier as follows.

LED_FLASH_RATE = 0:     Led Flash Period = pwm_period * 96

LED_FLASH_RATE = 1:     Led Flash Period = pwm_period * 192

LED_FLASH_RATE = 2:     Led Flash Period = pwm_period * 384

LED_FLASH_RATE = 3:     Led Flash Period = pwm_period * 768

LED_FLASH_RATE = 4:     Led Flash Period = pwm_period * 1536

...

LED_FLASH_RATE = 15:    Led Flash Period = pwm_period * 3145728

     */
    LED_FLASH_RATE,

    /*!
Sets hold time for max PWM when flashing
     */
    LED_FLASH_MAX_HOLD,

    /*!
Sets hold time for min PWM when flashing
     */
    LED_FLASH_MIN_HOLD,

    /*!
Inverts the PWM output. When using this key, a zero
value will disable the feature and a non zero value will enable the feature.
     */
    LED_FLASH_SEL_INVERT,

    /*!
Selects the status of DRIVE pad if Drive EnableB is
used for LED output.  When using this key, a zero value will disable the feature
and a non zero value will enable the feature.
     */
    LED_FLASH_SEL_DRIVE,

    /*!
Selects that the LED PWM output controls the Pad drive
enableB rather than output. When using this key, a zero value will disable the 
feature and a non zero value will enable the feature.
     */
    LED_FLASH_SEL_TRISTATE

}led_config_key;


#endif /* __LED_IF_H__  */

