/****************************************************************************

        Copyright (c) 2009 - 2016 Qualcomm Technologies International, Ltd.
        All Rights Reserved.
        Qualcomm Technologies International, Ltd. Confidential and Proprietary.

FILE
        stream_if.h  -  Stream Interface

CONTAINS
        Interface elements between the Stream firmware and VM Applications,
        that are not in sys.[ch]

DESCRIPTION
        This file is seen by the stack, and VM applications.
*/

/****************************************************************************/

/*!
 @file stream_if.h
 @brief Definitions used by the VM
*/

/*****************************************************************************/

#ifndef __APP_STREAM_IF_H__
#define __APP_STREAM_IF_H__
 

#include "app/bluestack/types.h"


/*!
  @brief defines radio type of Hid device
  @see   hid_type
*/
#define HID_TYPE_RADIO_MASK            0x80
#define HID_TYPE_RADIO_LE              0x00
#define HID_TYPE_RADIO_BT              HID_TYPE_RADIO_MASK

/*!
  @brief Template to register a handle for tranform pre-fix processing
         on from air BT/LE Hid Report.
   
         Indicate the value to be pre-fixed on the received data
         Useful for LE HID device in report mode where 
         the report-id has to be prefixed for LE HID reports
         
  @see   vm_transform_config_key
*/

typedef struct hid_add_pre_info_tag
{
    uint16_t  att_handle;
    uint8_t   report_id;
}hid_add_pre_info;


/* The stream_config_key enumeration combines and replaces the
 * vm_source_config_key and vm_sink_config_key enumerations that were
 * previously defined in app\vm\vm_if.h.
 *
 * The reason behind the move to the stream subsystem was to open up the
 * enumeration for use by HCI as well as VM commands, creating a unified set
 * of configuration IDs. For historical reasons the original enumerators are
 * still prefixed with VM_SOURCE_ and VM_SINK_. However, new additions should
 * be prefixed with STREAM_ (or STREAM_SOURCE_ / STREAM_SINK_).
 */ 

/*!
  @brief Keys used with SinkConfigure().

  The VM_SINK_USB_ keys are used to change certain properties associated
  with the USB bulk endpoint, intended primarily to allow the behaviour
  of bulk transfers to be configured in a manner suitable for use by a
  USB mass storage application. Other types of application will probably
  want to leave everything at its default value.
  A USB mass storage application would set the 
  following keys at initialisation:
    VM_SINK_USB_TERMINATION = 1
    VM_SINK_USB_VM_SUPPLIES_LENGTH = 1
    VM_SINK_USB_SUPPORT_LARGE_FILES = 1
  The application would then provide the length of data
  before each transfer:
    VM_SINK_USB_TRANSFER_LENGTH = data_length
  NB: VM_SINK_USB_TERMINATION must be set from the VM _init function
  and the termination type can not be changed later.

    STREAM_PCM_ keys are used to configure the audio PCM hardware obtained by
    invoking the StreamAudioSource()/StreamAudioSink() traps with #AUDIO_HARDWARE_PCM
    as the "hardware" parameter.

    STREAM_I2S_ keys are used to configure the audio I2S hardware obtained by
    invoking the StreamAudioSource()/StreamAudioSink() traps with #AUDIO_HARDWARE_I2S
    as the "hardware" parameter.

    STREAM_CODEC_ keys are used to configure the audio CODEC hardware obtained by
    invoking the StreamAudioSource()/StreamAudioSink() traps with #AUDIO_HARDWARE_CODEC
    as the "hardware" parameter.

    STREAM_FM_ keys are used to configure the audio FM hardware obtained by
    invoking the StreamAudioSource()/StreamAudioSink() traps with #AUDIO_HARDWARE_FM
    as the "hardware" parameter.

    STREAM_SPDIF_ keys are used to configure the audio SPDIF hardware obtained by
    invoking the StreamAudioSource()/StreamAudioSink() traps with #AUDIO_HARDWARE_SPDIF
    as the "hardware" parameter.

    STREAM_DIGITAL_MIC_ keys are used to configure the audio digital MIC hardware obtained by
    invoking the StreamAudioSource() trap with #AUDIO_HARDWARE_DIGITAL_MIC
    as the "hardware" parameter.

    STREAM_AUDIO_ keys are used to configure the audio hardware obtained by
    invoking the StreamAudioSource()/StreamAudioSink() traps with any kind of the #audio_hardware.

    STREAM_FASTPIPE_ keys are used to configure FastPipes.
*/

typedef enum
{
    VM_SOURCE_SCO_RATEMATCH_ENABLE = 0x0000, /*!< SCO ratematch enable.*/
    VM_SOURCE_FREQUENCY = 0x0001,            /*!< Frequency of source in Hz. Used to
                                              *   configure AudioStream sources.*/
    VM_SOURCE_MESSAGES = 0x0002,             /*!< Control of generation of
                                              *   #MESSAGE_MORE_DATA; value is a member of #vm_messages_settings */
    VM_SOURCE_SCO_METADATA_ENABLE = 0x0003,  /*!< SCO Stream Metadata enable */

    VM_SINK_MESSAGES = 0x0004,               /*!< Control of generation of
                                              *   #MESSAGE_MORE_SPACE; value is a member of #vm_messages_settings */
        
    VM_SINK_USB_TERMINATION = 0x0005,        /*!< How USB bulk transfer is terminated.
                                              *   Explicitly = 0, Implicitly = 1 */
    VM_SINK_USB_VM_SUPPLIES_LENGTH = 0x0006, /*!< Will VM supply the length for bulk
                                              *   transfers. No = 0, Yes = 1 */
    VM_SINK_USB_TRANSFER_LENGTH = 0x0007,    /*!< Length of next bulk transfer */
    VM_SINK_USB_SUPPORT_LARGE_FILES = 0x0008,/*!< Allow file larger than bulk endpoint
                                              *   buffer. No = 0, Yes = 1 */
    VM_SINK_SCO_SET_FRAME_LENGTH = 0x0009,   /*!< SCO rate matching frame length.
                                              *   0 = sample based. */

    STREAM_SOURCE_NOTIFY_WHEN_DRAINED = 0x000A,  /*!< Set the "Notify when Drained" flag. When set, firmware will generate HQ event when
                                                  the stream buffer has been drained completely.*/
    STREAM_DSP_DISABLE_METADATA = 0x000B,        /*!< Disables kalimba metadata for a stream_dsp */                                              

    STREAM_PCM_SYNC_RATE = 0x0100,                          /*!< PCM synchronisation rate in hertz.*/
    STREAM_PCM_MASTER_CLOCK_RATE = 0x0101,                  /*!< PCM master clock rate in hertz. If 0, master clock rate will be derived from synchronisation rate,
                                                                 slot count and sample format.*/
    STREAM_PCM_MASTER_MODE = 0x0102,                        /*!< Enable PCM master mode - clock and sync will be generated by the PCM hardware.*/
    STREAM_PCM_SLOT_COUNT = 0x0103,                         /*!< PCM slot count. Valid range 0 to 4 inclusive. If 0, slot count will be derived from master clock
                                                                 and synchronisation rate.*/
    STREAM_PCM_MANCH_MODE_ENABLE = 0x0104,                  /*!< Enable PCM Manchester encoding mode.*/
    STREAM_PCM_SHORT_SYNC_ENABLE = 0x0105,                  /*!< Enable PCM short synchronisation - Short frame sync (falling edge indicates start of frame), 
                                                                 rising edge indicates start of fame in long sync mode.*/
    STREAM_PCM_MANCH_SLAVE_MODE_ENABLE = 0x0106,            /*!< Enable PCM Manchester slave mode - Force transmit frames to follow receive frames with 
                                                                 constant delay. Requires extended features to be enabled.*/
    STREAM_PCM_SIGN_EXTEND_ENABLE = 0x0107,                 /*!< Enable PCM sign extend - Sign extend 13/8 bit sequence to 16 bit sequence,
                                                                 else pad with the STREAM_PCM_AUDIO_GAIN for 13-bits or 0s for 8 bits.*/
    STREAM_PCM_LSB_FIRST_ENABLE = 0x0108,                   /*!< Enable PCM LSB first - Transmit data LSB first.*/
    STREAM_PCM_TX_TRISTATE_ENABLE = 0x0109,                 /*!< Enable PCM Tx tristate - 0: drive PCM_OUT continuously. 
                                                                 1: tri-state PCM_OUT immediately after falling edge of
                                                                 PCM_CLK in the last bit of an active slot, assuming the next slot is not active.*/
    STREAM_PCM_TX_TRISTATE_RISING_EDGE_ENABLE = 0x010a,     /*!< Enable PCM Tx tristate rising edge -
                                                                 0: tri-state PCM_OUT immediately after falling edge of PCM_CLK in last bit of an active slot,
                                                                    assuming the next slot is also not active.
                                                                 1: tri-state PCM_OUT after rising edge of PCM_CLK.*/
    STREAM_PCM_SYNC_SUPPRESS_ENABLE = 0x010b,               /*!< Enable PCM synchronisation suppress - Suppress PCM_SYNC while generating PCM_CLK (in master mode). 
                                                                 Some CODECs (connected to the PCM interface) use this to enter a low power state.*/
    STREAM_PCM_GCI_MODE_ENABLE = 0x010c,                    /*!< Enable PCM GCI mode.*/
    STREAM_PCM_MUTE_ENABLE = 0x010d,                        /*!< Mute PCM_DATA output.*/
    STREAM_PCM_LONG_LENGTH_SYNC_ENABLE = 0x010e,            /*!< Enable PCM long length sync - Set PCM_SYNC to 8 or 16 PCM_CLK cycles.*/
    STREAM_PCM_SAMPLE_RISING_EDGE_ENABLE = 0x010f,          /*!< Enable PCM sample rising edge - Sample PCM_DATA on rising edge of PCM_CLK.*/


    STREAM_PCM_SAMPLE_FORMAT = 0x0114,                      /*!< PCM sample format - Valid values: 0 (13 bits in 16 cycle slot duration), 
                                                                 1 (16 bits in 16 cycle slot duration), 2 (8 bits in 16 cycle slot duration), 
                                                                 3 (8 bits in 8 cycle slot duration).*/

    STREAM_PCM_MANCH_RX_OFFSET = 0x0115,                    /*!< PCM Manchester receive offset - When in Manchester mode, selects the delay between receiving
                                                                 the start bit and sampling the first significant bit from the voice sample. Valid range: 0 to 3 inclusive.*/
    STREAM_PCM_AUDIO_GAIN = 0x0116,                         /*!< PCM audio gain. Valid range: 0 to 7. Used to pad the end 3 bits of a 13 bit PCM sample.  
                                                                 It is used by some CODECs (connected to the PCM interface) to allow their gain to be
                                                                 controlled.*/


    STREAM_I2S_SYNC_RATE = 0x0200,                         /*!< I2S synchronisation rate in hertz.*/
    STREAM_I2S_MASTER_CLOCK_RATE = 0x0201,                 /*!< I2S master clock rate in hertz.*/
    STREAM_I2S_MASTER_MODE = 0x0202,                       /*!< Enable I2S master mode - clock and sync will be generated by the I2S hardware.*/
    STREAM_I2S_JSTFY_FORMAT = 0x0203,                      /*!< I2S format - 0: left justified, 1: right justified.*/
    STREAM_I2S_LFT_JSTFY_DLY = 0x0204,                     /*!< I2S left justify format - 0: left justified formats: 
                                                                0 is MSB of SD data occurs in the first SCLK period following WS transition.
                                                                1: MSB of SD data occurs in the second SCLK period.*/
    STREAM_I2S_CHNL_PLRTY = 0x0205,                        /*!< I2S channel polarity. Valid values: 0 (SD data is left channel when WS is high), 
                                                                1 (SD data is right channel when WS is high).*/
    STREAM_I2S_AUDIO_ATTEN_ENABLE = 0x0206,                /*!< I2S audio attenuation enable - 0: 17 bit SD data is rounded down to 16 bits.
                                                                1: the audio attenuation defined in STREAM_I2S_AUDIO_ATTEN is applied over 24 and 20 bits
                                                                of incoming data with saturated rounding. Requires STREAM_I2S_CROP_ENABLE to be 0.*/
    STREAM_I2S_AUDIO_ATTEN = 0x0207,                       /*!< I2S attenuation in 6 dB steps. Valid range: 0 to 15 inclusive.*/
    STREAM_I2S_JSTFY_RES = 0x0208,                         /*!< I2S justify resolution - Resolution of data on SD_IN, 00=16 bit, 01=20 bit, 10=24 bit, 11=Reserved.
                                                                This is required for right justified format and with left justified LSB first.*/
    STREAM_I2S_CROP_ENABLE = 0x0209,                       /*!< I2S crop enable
                                                                0: 17 bit SD_IN data is rounded down to 16 bits.
                                                                1: only the most significant 16 bits of data are received.*/
    STREAM_I2S_BITS_PER_SAMPLE = 0x020a,                   /*!< I2S bits per sample. If larger than the internal format used by BlueCore, the additional
                                                                bits will be output as zeros in the least significant bits.*/
    STREAM_I2S_TX_START_SAMPLE = 0x020b,                   /*!< I2S start Tx sampling
                                                                0: during low wclk phase.
                                                                1: during high wclk phase.*/
    STREAM_I2S_RX_START_SAMPLE = 0x020c,                   /*!< I2S start Rx sampling
                                                                0: during low wclk phase.
                                                                1: during high wclk phase.*/

    STREAM_CODEC_INPUT_RATE = 0x0300,                      /*!< CODEC input sample rate in hertz.
                                                                Consult the datasheet of your BlueCore for supported sample rates.*/
    STREAM_CODEC_OUTPUT_RATE = 0x0301,                     /*!< CODEC output sample rate in hertz.
                                                                Consult the datasheet of your BlueCore for supported sample rates.*/
    STREAM_CODEC_INPUT_GAIN = 0x0302,                      /*!< CODEC input gain selection value. Valid range: 0 to #CODEC_INPUT_GAIN_RANGE.
                                                                Consult the datasheet of your BlueCore for further information.*/
    STREAM_CODEC_OUTPUT_GAIN = 0x0303,                     /*!< CODEC output gain selection value. Valid range: 0 to #CODEC_OUTPUT_GAIN_RANGE.
                                                                Consult the datasheet of your BlueCore for further information.*/
    STREAM_CODEC_RAW_INPUT_GAIN = 0x0304,                  /*!< CODEC raw input gain.
                                                                Consult the datasheet of your BlueCore for further information.*/
    STREAM_CODEC_RAW_OUTPUT_GAIN = 0x0305,                 /*!< CODEC raw output gain.
                                                                Consult the datasheet of your BlueCore for further information.*/
    STREAM_CODEC_OUTPUT_GAIN_BOOST_ENABLE = 0x0306,        /*!< Enable CODEC output gain boost. This "boost" provides an extra 3dB of gain 
                                                                when in differential output mode.*/
    STREAM_CODEC_SIDETONE_GAIN = 0x0307,                   /*!< CODEC sidetone gain. The side-tone hardware is only present on certain BlueCore variants.
                                                                Consult the datasheet of your BlueCore for further information.*/
    STREAM_CODEC_SIDETONE_ENABLE = 0x0308,                 /*!< Enable CODEC sidetone.*/
    STREAM_CODEC_MIC_INPUT_GAIN_ENABLE = 0x0309,           /*!< Enable CODEC MIC input gain. Passing TRUE enables the microphone pre-amp on BlueCore for
                                                                channel A/B. This results in an additional 20dB of input gain on that channel. 
                                                                In designs that use a microphone, enabling this functionality is recommended.*/
    STREAM_CODEC_LOW_POWER_OUTPUT_STAGE_ENABLE = 0x030a,   /*!< Enable CODEC low power output stage.*/

    STREAM_CODEC_QUALITY_MODE = 0x30b,                     /*!< Set the quality mode for the CODEC hardware. 
                                                                See below codec_quality_mode enumeration
                                                                for the different possible modes.
                                                                The configured quality mode will be effective 
                                                                from the next connection.*/
    STREAM_CODEC_OUTPUT_INTERP_FILTER_MODE = 0x30c,        /*!< Set the FIR mode for the DAC interpolation filter.
                                                                Valid values can be selected from #codec_output_interpolation_filter_mode enumeration. 
                                                                The FIR mode can be changed when the DAC is running, however this may cause audio
                                                                disturbances and is not recommended */
    STREAM_CODEC_OUTPUT_POWER_MODE = 0x30d,                /*!< Set the output power mode. 
                                                                See below codec_output_power_mode enumeration
                                                                for the different possible modes.
                                                                Selections are for impedance to 8ohm or 16ohm 
                                                                and the output power mode to normal or low power.*/
    STREAM_CODEC_SIDETONE_SOURCE = 0x030e,                 /*! The sidetone source. It should be one of the following
                                                                Codec A/B, Digital Mic A/B, Digital Mic C/D, Digital Mic E/F*/
    STREAM_CODEC_SIDETONE_SOURCE_POINT = 0x030f,           /*!< Selects the node from which Sidetone data is taken.
                                                                Sidetone Data source can be taken from output of either IIR Filter stage or Digital Gain stage. */
    STREAM_CODEC_SIDETONE_INJECTION_POINT = 0x0310,        /*!< The sidetone injection point. Selects sidetone injection point
                                                                 into DAC chain. */
    STREAM_CODEC_SIDETONE_SOURCE_MASK = 0x0311,            /*! The sidetone source mask. This key allows to select any combination
                                                               of 2 ADC channels to be used as sidetone source.*/
    STREAM_CODEC_INDIVIDUAL_SIDETONE_GAIN = 0x0312,        /*!< ADC sidetone gain of single sidetone source.*/
    STREAM_CODEC_INDIVIDUAL_SIDETONE_ENABLE = 0x0313,      /*!< Enable sidetone for a particular DAC channel. The side-tone hardware is only present
                                                                on certain BlueCore variants. Consult the datasheet of your BlueCore for further information.*/
    STREAM_CODEC_ADC_DATA_SOURCE_POINT = 0x0314,           /*!< Selects the node from which ADC data is taken and routed to buffers(or DSP).
                                                                ADC Data source can be taken from output of either IIR Filter stage or Digital Gain stage. */
    STREAM_CODEC_ADC_ROUTE = 0x0315,                       /*!< ADC chain routing. 0: IIR filter followed by Digital gain
                                                                                   1: Digital gain followed by IIR filter.*/
    STREAM_CODEC_SIDETONE_INVERT = 0x0316,                 /*!< Invert sidetone phase for a particular DAC channel.*/

    STREAM_CODEC_G722_FILTER_ENABLE = 0x0317,              /*!< Enable optional G722 filter in the ADC/MIC decimation chain. The filter is
                                                                primarily designed for G722 but can be used otherwise as well as it improves
                                                                noise performace.*/
    STREAM_CODEC_G722_FIR_ENABLE = 0x0318,                 /*!< Enable optional FIR filter in the G722 filter to droop the response slightly.*/

    STREAM_FM_INPUT_RATE = 0x0400,                         /*!< FM receiver sample rate in hertz.
                                                                Consult the datasheet of your BlueCore for further information.*/
    STREAM_FM_OUTPUT_RATE = 0x0401,                        /*!< FM transmitter sample rate in hertz.
                                                                Consult the datasheet of your BlueCore for further information.*/
    STREAM_FM_INPUT_GAIN = 0x0402,                         /*!< FM receiver gain.
                                                                Consult the datasheet of your BlueCore for further information.*/
    STREAM_FM_OUTPUT_GAIN = 0x0403,                        /*!< FM transmitter gain.
                                                                Consult the datasheet of your BlueCore for further information.*/

    STREAM_SPDIF_OUTPUT_RATE = 0x0500,                     /*!< SPDIF output sample rate in hertz. Supported rates: 32000, 44100 and 48000.*/
    STREAM_SPDIF_CHNL_STS_REPORT_MODE = 0x0501,            /*!< Set the reporting mode for the SPDIF Rx channel Status .*/
    STREAM_SPDIF_OUTPUT_CHNL_STS_DUP_EN = 0x0502,          /*!< Set the SPDIF Tx channel B status same as that of channel A.*/
    STREAM_SPDIF_OUTPUT_CHNL_STS_WORD = 0x0503,            /*!< Set the SPDIF Tx channel status word value.*/
    STREAM_SPDIF_AUTO_RATE_DETECT = 0x0504,                /*!< Set SPDIF RX in auto rate detect mode.*/

    STREAM_DIGITAL_MIC_INPUT_RATE = 0x0600,                /*!< Digital microphone sample rate in hertz.
                                                                Consult the datasheet of your BlueCore for further information.*/
    STREAM_DIGITAL_MIC_INPUT_GAIN = 0x0601,                /*!< Digital microphone input gain.
                                                                Consult the datasheet of your BlueCore for further information.*/
    STREAM_DIGITAL_MIC_SIDETONE_GAIN = 0x0602,             /*!< Sidetone gain. The side-tone hardware is only present on certain BlueCore variants.
                                                                Consult the datasheet of your BlueCore for further information.*/
    STREAM_DIGITAL_MIC_SIDETONE_ENABLE = 0x0603,           /*!< Enable MIC sidetone.*/ 
    STREAM_DIGITAL_MIC_CLOCK_RATE = 0x0604,                /*!< Digital MIC clock rate in kHz.
                                                                Valid values for clock rate :  500 (500 kHz),
                                                                                              1000 (1 MHz),
                                                                                              2000 (2 MHz),
                                                                                              4000 (4 MHz).*/
    STREAM_DIGITAL_MIC_SIDETONE_SOURCE_POINT = 0x0605,     /*!< The sidetone source point. Selects source point for Digital Mic data. */
    STREAM_DIGITAL_MIC_INDIVIDUAL_SIDETONE_GAIN = 0x0606,  /*!< DMic sidetone gain. The side-tone hardware is only present on certain BlueCore variants.
                                                                Consult the datasheet of your BlueCore for further information.*/
    STREAM_DIGITAL_MIC_DATA_SOURCE_POINT = 0x0607,        /*!< Digital Mic data source point selection. Digital Mic codec data source
                                                                can be selected from either IIR Filter out or Digital Gain filter out */
    STREAM_DIGITAL_MIC_ROUTE = 0x0608,                     /*!< DMic ADC chain routing. 0: IIR filter followed by Digital gain
                                                                                        1: Digital gain followed by IIR filter.*/
    STREAM_DIGITAL_MIC_G722_FILTER_ENABLE = 0x0609,        /*!< Enable optional G722 filter in the ADC/MIC decimation chain. The filter was
                                                                primarily designed for G722 but can be used otherwise as well as it improves
                                                                noise performace.*/
    STREAM_DIGITAL_MIC_G722_FIR_ENABLE = 0x060a,           /*!< Enable optional FIR filter in the G722 filter to droop the response slightly.*/
    STREAM_DIGITAL_MIC_AMP_SEL = 0x060b,                   /*!< DMic input amplitude select value. External Digital mic, which usually has a
                                                                sigma-delta modulator, provides one bit(1 or 0) per sampling interval.
                                                                However, DMIC hardware is capable of handling 3-bit(b000 to b111)
                                                                input per sampling interval. This key tells how a single bit coming from a
                                                                digital mic should be translated to a 3-bit value. Usually, logic 1 is
                                                                translated to a 7, and logic 0 is translated to 0. Values are choosen
                                                                symmetrical around 3.5 to avoid any DC offset in the incoming signal.
                                                                (i.e. in combination of 0,7 or 1,6 or 2,5 or 3,4 or 4,3 or 5,2 or 6,1 or 7,0).
                                                                The lower 16 bits sets the value for logic 0 and the higher 16 bits sets
                                                                the value for logic 1. The key can be used to invert an ADC channels signal
                                                                w.r.t. other ADC channel.*/

    STREAM_AUDIO_MUTE_ENABLE = 0x0700,                     /*!< Mute the audio. Valid values: 0 (un-mute), 1 (mute).*/

    STREAM_AUDIO_SAMPLE_SIZE = 0x0701,                     /*!< Set Audio Sample Size.
                                                                Valid values for PCM interface :   0 (13 bits in 16 cycle slot duration),
                                                                                                   1 (16 bits in 16 cycle slot duration),
                                                                                                   2 (8 bits in 16 cycle slot duration), 
                                                                                                   3 (8 bits in 8 cycle slot duration), 
                                                                                                   16 (16 bits)(same as value 1) 
                                                                                                   24 (24 bits).
                                                                Valid values for other interfaces: 16 (16 bit), 24 (24 bit).*/

    STREAM_SINK_SHUNT_L2CAP_ATU = 0x0800,                  /*!< Set the ATU on a shunt sink */
    STREAM_SINK_SHUNT_AUTOFLUSH_ENABLE = 0x0801,           /*!< Set the auto flushable packets flag on a shunt sink */
    STREAM_SINK_SHUNT_TOKENS_ALLOCATED = 0x0802,           /*!< Set num of tokens allocated to a shunt */
    STREAM_SOURCE_SHUNT_MAX_PDU_LENGTH = 0x0803,              /*!< Set the maximum PDU length. L2CAP packet that violates this maximum limit
                                                                will be discarded.*/
    STREAM_FASTPIPE_MIN_CREDIT_RETURN = 0x0900,            /*!< Set the minimum number of credits to return at a time */
    STREAM_FASTPIPE_MIN_DATA_PACKET = 0x0901               /*!< Set the minimum size of a data packet to send */
} stream_config_key;


/*!
  @brief Codec quality modes used in STREAM_CODEC_QUALITY_MODE config key.

*/
typedef enum 
{
    CODEC_TELEPHONY_MODE,     /*!< Codec configured with telephony quality mode. */
    CODEC_NORMAL_MODE,        /*!< Codec configured with normal quality mode. */
    CODEC_HIGH_MODE,          /*!< Codec configured with high quality mode. */
    CODEC_BYPASS_IN_MODE      /*!< Codec In configured with Bypass In Amp mode. */ 
}codec_quality_mode;


/*!
  @brief Audio output modes used in STREAM_AUDIO_OUTPUT_POWER_MODE config key.

NOTE: ADK2.0 and earlier releases had these parameters incorrectly named. The
      enumerations have been renamed to reflect the hardware, values are
      unchanged.
*/
typedef enum 
{
    CODEC_OUTPUT_POWER_MODE_16OHM,  /*!< Codec power mode for driving speakers with 16R (or higher) impedance */
    CODEC_OUTPUT_POWER_MODE_32OHM,  /*!< Codec power mode for driving speakers with 32R (or higher) impedance */
    CODEC_OUTPUT_POWER_MODE_LOW     /*!< Codec power mode for driving a high impedance input at upto 100mV */ 
}codec_output_power_mode; 


/*!
  @brief Codec output interpolation filter FIR modes for use with STREAM_CODEC_OUTPUT_INTERP_FILTER_MODE config key.

*/
typedef enum 
{
    CODEC_OUTPUT_INTERP_MODE_LONG_FIR,     /*!< Codec output interpolation filter configured for long FIR mode, not available at 96kHz. */
    CODEC_OUTPUT_INTERP_MODE_SHORT_FIR,    /*!< Codec output interpolation filter configured for short FIR mode. */
    CODEC_OUTPUT_INTERP_MODE_NARROW_FIR    /*!< Codec output interpolation filter configured for narrow FIR mode. */
}codec_output_interpolation_filter_mode;

/*!
  @brief adc data route for use with STREAM_CODEC_ADC_DATA_SOURCE_POINT and STREAM_DIGITAL_MIC_DATA_SOURCE_POINT config key.

*/
typedef enum
{
    ADC_DATA_SOURCE_FROM_IIR_FILTER,    /*!< ADC data is taken from output of IIR Filter. */
    ADC_DATA_SOURCE_FROM_DIGITAL_GAIN   /*!< ADC data is taken from output of Digital Gain Filter. */
}adc_data_source;

/*! @name Device mode for the tranform
  \{ */
typedef enum 
{ 
    VM_TRANSFORM_HID_TYPE_LE_MOUSE      = 1,/*!< HID LE Mouse */ 
    VM_TRANSFORM_HID_TYPE_LE_KEYBOARD,      /*!< HID LE Keyboard */ 
    VM_TRANSFORM_HID_TYPE_LE_ANY,           /*!< Any other LE HID device */ 
    VM_TRANSFORM_HID_TYPE_BT_MOUSE      = HID_TYPE_RADIO_BT | VM_TRANSFORM_HID_TYPE_LE_MOUSE, /*!< HID BT Mouse */     
    VM_TRANSFORM_HID_TYPE_BT_KEYBOARD,      /*!< HID BT Keyboard */ 
    VM_TRANSFORM_HID_TYPE_BT_ANY           /*!< Any other BT HID device */ 
}hid_type;
/*! \} */


/*! @name Set mode for the tranform
  \{ */
typedef enum 
{ 
    VM_TRANSFORM_HID_SET_MODE_BOOT = 1, /*!< Boot Mode */ 
    VM_TRANSFORM_HID_SET_MODE_REPORT    /*!< Report Mode */ 
}hid_set_mode;
/*! \} */


#endif  /* __APP_STREAM_IF_H__ */
