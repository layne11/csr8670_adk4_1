/****************************************************************************

        Copyright (c) 2001 - 2016 Qualcomm Technologies International, Ltd.
        All Rights Reserved.
        Qualcomm Technologies International, Ltd. Confidential and Proprietary.

FILE
        audio_if.h  -  Audio Interface

CONTAINS
        Interface elements between the Audio firmware and VM Applications, that are
        not in sys.[ch]

DESCRIPTION
        This file is seen by the VM applications and the Firmware.
*/

/****************************************************************************/

/*!
    @file audio_if.h
 
    @brief This file contains the definitions for the parameters used by the
    StreamAudioSource(), StreamAudioSink(), and CodecSetIirFilter() traps.

*/

/*****************************************************************************/

#ifndef __APP_AUDIO_IF_H__
#define __APP_AUDIO_IF_H__

/*!
    @brief Defines used to indicate Codec Gain Ranges (see #STREAM_CODEC_INPUT_GAIN and #STREAM_CODEC_OUTPUT_GAIN)
*/

#define CODEC_INPUT_GAIN_RANGE      22
#define CODEC_OUTPUT_GAIN_RANGE     22

/*!
  @brief audio_hardware used in StreamAudioSource()/StreamAudioSink() traps.
  
*/

typedef enum {
    AUDIO_HARDWARE_PCM, /*!< The audio PCM hardware. "instance" specifies the physical interface (AUDIO_INSTANCE_0 is the first PCM interface, AUDIO_INSTANCE_1 is the second, on chips which have two).
                         Audio hardware PCM, I2S and SPDIF are mutually exclusive for the same audio_instance.
                         "channel" specifies the PCM slot; only AUDIO_CHANNEL_SLOT_x are valid for this hardware type.*/
    AUDIO_HARDWARE_I2S, /*!< The audio I2S hardware. "instance" specifies the physical interface (AUDIO_INSTANCE_0 is the first I2S interface, AUDIO_INSTANCE_1 is the second, on chips which have two).
                         Audio hardware PCM, I2S and SPDIF are mutually exclusive for the same audio_instance.
                         "channel" specifies the I2S slot; only AUDIO_CHANNEL_SLOT_x are valid for this hardware type.*/
    AUDIO_HARDWARE_SPDIF, /*!< The audio SPDIF hardware. "instance" specifies the physical interface (AUDIO_INSTANCE_0 is the first SPDIF interface, AUDIO_INSTANCE_1 is the second, on chips which have two).
                           Audio hardware PCM, I2S and SPDIF are mutually exclusive for the same audio_instance.
                           "channel" specifies the SPDIF slot; only AUDIO_CHANNEL_SLOT_x are valid for this hardware type.*/
    AUDIO_HARDWARE_CODEC, /*!< The audio CODEC hardware. "instance" specifies the physical interface (AUDIO_INSTANCE_0 is the first CODEC interface, AUDIO_INSTANCE_1 is the second, on chips which have two).
                           "channel" specifies the CODEC channel; only AUDIO_CHANNEL_A or AUDIO_CHANNEL_B or AUDIO_CHANNEL_A_AND_B are valid for this hardware type.
                           On chips with stereo CODECs, an "instance" consists of a pair of channels (stereo).*/
    AUDIO_HARDWARE_DIGITAL_MIC, /*!< The audio digital MIC hardware. "instance" specifies the physical interface (AUDIO_INSTANCE_0 is the first digital MIC interface, AUDIO_INSTANCE_1 is the second,
                                     AUDIO_INSTANCE_2 is the third, on chips which have three).
                                     "channel" specifies the digital MIC channel; only AUDIO_CHANNEL_A or AUDIO_CHANNEL_B are valid for this hardware type.
                                     An "instance" consists of a pair of channels (stereo).*/
    AUDIO_HARDWARE_FM /*!< The audio FM hardware. "instance" specifies the physical interface (AUDIO_INSTANCE_0 is the FM interface).
                       The FM instance can be obtained either by StreamAudioSource (FM receiver) or by StreamAudioSink(FM transmitter).
                       "channel" specifies the FM channel; only AUDIO_CHANNEL_A or AUDIO_CHANNEL_B are valid for this hardware type.*/
} audio_hardware;

/*!
  @brief audio_instance used in StreamAudioSource(), StreamAudioSink() and CodecSetIirFilter() traps.

  The meaning of an "instance" depends on the hardware; see the descriptions in #audio_hardware.
*/

typedef enum {
    AUDIO_INSTANCE_0, /*!< The audio hardware instance 0*/
    AUDIO_INSTANCE_1, /*!< The audio hardware instance 1*/
    AUDIO_INSTANCE_2  /*!< The audio hardware instance 2*/
} audio_instance;

/*!
  @brief audio_channel used in StreamAudioSource(), StreamAudioSink() and CodecSetIirFilter() traps.
*/

typedef enum {
    AUDIO_CHANNEL_A, /*!< The audio channel A*/
    AUDIO_CHANNEL_B, /*!< The audio channel B*/
    AUDIO_CHANNEL_A_AND_B, /*!< The audio channel A and B (obtaining the stereo CODEC in mono-mode)*/
    AUDIO_CHANNEL_SLOT_0 = 0, /*!< The audio digital slot 0*/
    AUDIO_CHANNEL_SLOT_1, /*!< The audio digital slot 1*/
    AUDIO_CHANNEL_SLOT_2, /*!< The audio digital slot 2*/
    AUDIO_CHANNEL_SLOT_3, /*!< The audio digital slot 3*/
    SPDIF_CHANNEL_A = 0, /*!< The 1st SPDIF subframe*/
    SPDIF_CHANNEL_B, /*!< The 2nd SPDIF subframe*/
    SPDIF_CHANNEL_USER, /*!< The User data in SPDIF subframes*/
    SPDIF_CHANNEL_A_B_INTERLEAVED /*!< SPDIF channels to be interleaved*/
} audio_channel;


typedef struct
{
    uint16 coefficients[11];
} IIR_COEFFICIENTS;

#endif  /* __APP_AUDIO_IF_H__ */
