/****************************************************************************
Copyright (c) 2004 - 2016 Qualcomm Technologies International, Ltd.
Part of ADK 4.1
*/
/*!
    @file   gain_utils.h

    @brief Header file for the gain utility library. This library implements several utility volume
    functions
 
 */

/*@{*/

#ifndef GAIN_UTILS_H_
#define GAIN_UTILS_H_

#include <csrtypes.h>

typedef struct
{
    int16 dsp_db_scaled;
    uint8 dac_gain;
}hybrid_gains_t;

typedef struct __volume_group_config_t
{
    int16 no_of_steps;         /* number of steps of volume change permitted */
    int16 volume_knee_value_1;  /* volume point at which curve of dB conversion changes */  
    int16 volume_knee_value_2;  /* volume point at which curve of dB conversion changes */
    int16 dB_knee_value_1;      /* dB value for point at which curve of dB conversion changes */
    int16 dB_knee_value_2;      /* dB value for point at which curve of dB conversion changes */
    int16 dB_max;               /* dB value at maximum volume level */
    int16 dB_min;               /* dB value at minimum volume level */
} volume_group_config_t;

#define CODEC_STEPS 15          /* number of DAC steps */
#define DB_TO_DAC   180         /* DAC steps of 3 dB * scaling factor of dsp volume control which is 60 */
#define DB_DSP_SCALING_FACTOR 60/* DSP dB values are multiplied by 60 */ 
#define MIN_CODEC_GAIN_DB   -45
#define MIN_CODEC_GAIN_STEPS 0
#define MAXIMUM_DIGITAL_VOLUME_0DB 0
#define MINIMUM_DIGITAL_VOLUME_80dB (-80 * DB_DSP_SCALING_FACTOR)
#define DSP_VOICE_PROMPTS_LEVEL_ADJUSTMENT (-6 * DB_DSP_SCALING_FACTOR) /* voice prompts played through the dsp require 6 dB of attenuation */
#define DIGITAL_VOLUME_MUTE (-120 * DB_DSP_SCALING_FACTOR) /* mute is -120dB */

/*!
    @brief Defines used to indicate Codec Gain Ranges
*/

#define CODEC_INPUT_GAIN_RANGE      22
#define CODEC_OUTPUT_GAIN_RANGE     22


/*! 
    @brief The codecs channel being referred to, left, right or both.
*/
typedef enum
{
    /*! The left channel of the codec.*/
    left_ch,                        
    /*! The right channel of the codec.*/
    right_ch,                       
    /*! The left and right channel of the codec. */
    left_and_right_ch               
} codec_channel;


/*!
    @brief Update the codec input gain immediately. 
   
    @param volume The gain level (volume) to set the input channel to.

    @param channel The channel to use.
*/
void CodecSetInputGainNow(uint16 volume, codec_channel channel);


/*!
    @brief Update the codec output gain immediately. 

    @param volume The gain level (volume) to set the output left channel to.

    @param channel The channel to use.
*/
void CodecSetOutputGainNow(uint16 volume, codec_channel channel);
   


/****************************************************************************
NAME
 VolumeConvertDACGainToDB

DESCRIPTION
 Converts dac gain to dB value

RETURNS
 dB value

*/
int16 VolumeConvertDACGainToDB(int16 DAC_Gain);

/****************************************************************************
NAME
 GainDbToDacGain

DESCRIPTION
 Converts dB to dac gain

RETURNS
 dac gain

*/
uint16 GainDbToDacGain(int16 volume);

/****************************************************************************
NAME
 VolumeConvertStepsToDB

DESCRIPTION
 Converts decimal step values into dB values for DSP volume control scheme

RETURNS
 dB value

*/
int16 VolumeConvertStepsToDB(int16 Volume, volume_group_config_t * volMappingConfig);

/****************************************************************************
NAME
 codecCalcHybridValues

DESCRIPTION
 Converts decimal step values into dB values for DSP volume control scheme

RETURNS
 dB value

*/
void CodecCalcHybridValues(hybrid_gains_t * gains, int16 volume);

#endif /* GAIN_UTILS_H_ */
/*@}*/
