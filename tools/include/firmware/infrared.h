/*
 * Copyright (c) 2016 Qualcomm Technologies International, Ltd.
 * This file was automatically generated for firmware version 19.5
 */

#ifndef __INFRARED_H

#define __INFRARED_H

#include <csrtypes.h>
#include <app/infrared/infrared_if.h>
/*! @file infrared.h @brief Traps to control the Infrared receiver */


/*!
    @brief Control the infrared receiver 

    @param key Which Infrared parameter to configure Parameters are defined in #infrared_config_key
    @param value The value the parameter specified by "key" should be set to
  
    @return TRUE if the key is valid and the input value is in valid range, else FALSE.

    More detailed information on the keys and values can be found in irfrared_if.h
*/
bool InfraredConfigure(infrared_config_key key, uint32 value);

#endif
