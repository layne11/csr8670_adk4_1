/*
 * Copyright (c) 2016 Qualcomm Technologies International, Ltd.
 * This file was automatically generated for firmware version 19.5
 */

#ifndef __LED_H

#define __LED_H

#include <csrtypes.h>
#include <app/led/led_if.h>
/*! @file led.h @brief Traps to control the LED hardware present on some BlueCore variants */


/*!
    @brief Control the LED hardware present on some BlueCore variants

    @param led The LED to configure
    @param key Which LED parameter to configure
    @param value The value the parameter specified by "key" should be set to
  
    @return TRUE if the #led_id is valid and the input value is in valid range, else FALSE.

    More detailed information on the keys and values can be found in led_if.h
    See the data sheet for accurate information on which LED hardware a given BlueCore
    variant supports.

    @note
    Certain BlueCore devices like CSR8675 allows LED pins to be used as PIOs.
    However, once the led is configured/enabled, respective pin can't be used
    as PIOs until device is rebooted.
*/
bool LedConfigure(led_id led, led_config_key key, uint16 value);

#endif
