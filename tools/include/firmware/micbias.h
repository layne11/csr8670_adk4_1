/*
 * Copyright (c) 2016 Qualcomm Technologies International, Ltd.
 * This file was automatically generated for firmware version 19.5
 */

#ifndef __MICBIAS_H

#define __MICBIAS_H

#include <csrtypes.h>
#include <app/mic_bias/mic_bias_if.h>
/*! @file micbias.h @brief Traps to control the Microphone bias hardware */


/*! 
    @brief Control of Microphone bias hardware
    @param mic Which microphone bias pin to configure
    @param key Which aspect of the microphone bias to configure
    @param value Which value to use

    @return TRUE if the "mic" parameter is valid and the input value is in valid range, else FALSE.

    Note: The dedicated mic bias pin is only present on some BlueCore variants.
    Only chips from BC5 onwards have this feature. In addition at least one ADC
    or DAC on chip must be enabled for the MicBias hardware to operate.

    Consult the datasheet for your BlueCore variant for more information.
*/
bool MicbiasConfigure(mic_bias_id mic, mic_bias_config_key key, uint16 value);

#endif
