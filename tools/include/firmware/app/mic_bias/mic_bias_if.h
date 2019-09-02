/****************************************************************************

        Copyright (c) 2009 - 2016 Qualcomm Technologies International, Ltd.
        All Rights Reserved.
        Qualcomm Technologies International, Ltd. Confidential and Proprietary.

FILE
    mic_bias_if.h

CONTAINS
    Definitions for the mic_bias subsystem.

DESCRIPTION
    This file is seen by the stack, and VM applications, and
    contains things that are common between them.
*/

/*!
 @file mic_bias_if.h
 @brief Parameters for use in the MicbiasConfigure() VM trap.
 **
 **
 The various #mic_bias_id values allow the VM application to configure
 any mic bias pins present of the BlueCore device. The exact number of
 mic bias pins present is device specific, so consult the datasheet of
 your BlueCore variant for more details. In addition at least one ADC
 or DAC on chip must be enabled for the MicBias hardware to operate.
*/

#ifndef __MIC_BIAS_IF_H__
#define __MIC_BIAS_IF_H__

/*! @brief MIC_BIAS pin identifiers */
typedef enum 
{
    MIC_BIAS_0,
    MIC_BIAS_1,
    NUM_OF_MIC_BIAS
}mic_bias_id;


/*! @brief MIC_BIAS config keys. */
typedef enum 
{
    /*!
Enable the microphone bias according to the passed in value (which is
a member of #mic_bias_enable_value).
     */
    MIC_BIAS_ENABLE,

    /*!
Set the microphone bias voltage.
When using this key, the exact mapping from value
to voltage is Bluecore dependent, so consult the datasheet. As an
example, for BC5-MM

      Value   Voltage

        0     1.71 V

        1     1.76 V

        2     1.82 V

        3     1.87 V

        4     1.95 V

        5     2.02 V

        6     2.10 V

        7     2.18 V

        8     2.32 V

        9     2.43 V

        10    2.56 V

        11    2.69 V

        12    2.90 V

        13    3.08 V

        14    3.33 V

        15    3.57 V
     */
    MIC_BIAS_VOLTAGE,

    /*!
Set the microphone bias current.
When using this key, the exact mapping from value
to current is Bluecore dependent, so consult the datasheet. As an
example, for BC5-MM

     Value   Current

        0    0.200 mA

        1    0.280 mA

        2    0.340 mA

        3    0.420 mA

        4    0.480 mA

        5    0.530 mA

        6    0.610 mA

        7    0.670 mA

        8    0.750 mA

        9    0.810 mA

        10   0.860 mA

        11   0.950 mA

        12   1.000 mA

        13   1.090 mA

        14   1.140 mA

        15   1.230 mA
     */
    MIC_BIAS_CURRENT

}mic_bias_config_key;

/*! @brief #MIC_BIAS_ENABLE key values. */
typedef enum 
{
    MIC_BIAS_OFF,           /*!< Disable the microphone bias immediately. */
    MIC_BIAS_AUTOMATIC_ON,  /*!< If PSKEY_CODEC_PIO is mapped to 
                                 MIC_BIAS_HARDWARE_MIC_BIAS_PIN, automatically turn
                                 the microphone bias on or off when the
                                 #AUDIO_HARDWARE_CODEC is enabled or disabled.*/
    MIC_BIAS_FORCE_ON       /*!< Enable the microphone bias immediately. */

}mic_bias_enable_value;


#endif /* __MIC_BIAS_IF_H__  */
