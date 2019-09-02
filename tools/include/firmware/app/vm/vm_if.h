/****************************************************************************

        Copyright (c) 2001 - 2016 Qualcomm Technologies International, Ltd.
        All Rights Reserved.
        Qualcomm Technologies International, Ltd. Confidential and Proprietary.

FILE
        vm_if.h  -  VM Interface

CONTAINS
        Interface elements between the VM and VM Applications, that are
        not in sys.[ch]

DESCRIPTION
        This file is seen by the stack, and VM applications, and
        contains event numbers that are common between them.

        Events can be read (and cleared) using EventCounter. If they are
        enabled, using EventEnable, then that event can also cause VmWait
        to return before the specified timeout.
*/

/****************************************************************************/

/*!
 @file vm_if.h
 @brief Definitions used by the VM
*/

/*****************************************************************************/

#ifndef __APP_VM_IF_H__
#define __APP_VM_IF_H__
 
/*!
  @brief Values used with #VM_STREAM_UART_CONFIG key of StreamConfigure().
*/
typedef enum
{
    VM_STREAM_UART_THROUGHPUT,  /*!< UART throughput.*/
    VM_STREAM_UART_LATENCY      /*!< UART latency.*/
} vm_stream_uart_config;

/*!
  @brief Keys for StreamConfigure().
*/
typedef enum
{
    /*! UART configuration. Value is a member of #vm_stream_uart_config.
     *  The default configuration is #VM_STREAM_UART_THROUGHPUT. */
    VM_STREAM_UART_CONFIG,

    /*! SCO enabled. */
    VM_STREAM_SCO_ENABLED,

    /*! #MESSAGE_USB_ALT_INTERFACE enabled. */
    VM_STREAM_USB_ALT_IF_MSG_ENABLED,

    /*! #MESSAGE_USB_ATTACHED and #MESSAGE_USB_DETACHED enabled. */
    VM_STREAM_USB_ATTACH_MSG_ENABLED,

    /*! USB Enable/Disable packet counter byte. (Not supported on BC4-EXT.) */
    VM_STREAM_USB_PACKET_COUNTER_ENABLED,

    /*! Enable large size buffer for a L2CAP channel for a PSM. */
    VM_STREAM_L2CAP_ADD_LARGE_BUFFER_ON_PSM,

    /*! Remove large size buffer for a L2CAP channel for a PSM. */
    VM_STREAM_L2CAP_REMOVE_LARGE_BUFFER,

    /*! Enables CSB data (TX/RX) path via STREAM. */
    VM_STREAM_CSB_ENABLED
} vm_stream_config_key;

/*!
  @brief Values used with #VM_SOURCE_MESSAGES and #VM_SINK_MESSAGES for SourceConfigure() and SinkConfigure() respectively.

  For instance SourceConfigure(source, VM_SOURCE_MESSAGES,
  VM_MESSAGES_SOME) will arrange that the task associated has at most
  one #MESSAGE_MORE_DATA queued for delivery at any one time. The
  default is VM_MESSAGES_ALL.
*/

typedef enum
{
    VM_MESSAGES_ALL,                /*!< Send all messages to the registered task */
    VM_MESSAGES_SOME,               /*!< Send at most one message at a time to the registered task */
    VM_MESSAGES_NONE                /*!< Send no messages to the registered task */
} vm_messages_settings;

/*!
  @brief Keys for TransformConfigure().
  @see   hid_set_mode
  @see   hid_type 
*/
typedef enum
{
    VM_TRANSFORM_CHUNK_CHUNK_SIZE,              /*!< Chunk size.*/
    VM_TRANSFORM_SLICE_FROM_START,              /*!< How many bytes to discard from start of packet */
    VM_TRANSFORM_SLICE_FROM_END,                /*!< How many bytes to discard from end of packet */
    VM_TRANSFORM_RTP_SBC_ENCODE_PACKET_SIZE,    /*!< RTP SBC Encode packet size.*/
    VM_TRANSFORM_RTP_SBC_ENCODE_MANAGE_TIMING,  /*!< RTP SBC Encode Manage Timing.*/
    VM_TRANSFORM_RTP_SBC_ENCODE_STATS,          /*!< RTP SBC Encode Statistics.*/
    VM_TRANSFORM_RTP_SCMS_ENABLE,               /*!< Enable SCMS content protection on RTP stream.*/
    VM_TRANSFORM_RTP_SCMS_SET_BITS,             /*!< Set SCMS bits (bit 0=L-bit, bit 1=Cp-bit)*/
    VM_TRANSFORM_RTP_MP3_ENCODE_PACKET_SIZE,    /*!< RTP MP3 Encode packet size.*/
    VM_TRANSFORM_RTP_MP3_ENCODE_MANAGE_TIMING,  /*!< RTP MP3 Encode Manage Timing.*/
    VM_TRANSFORM_RTP_MP3_ENCODE_STATS,          /*!< RTP MP3 Encode Statistics.*/
    VM_TRANSFORM_HID_KEYBOARD_IDLE_RATE,        /*!< HID Keyboard idle rate */
    VM_TRANSFORM_HID_TYPE,                      /*!< HID device type : key,mouse or any */
    VM_TRANSFORM_HID_SET_MODE,                  /*!< HID mode : boot or report */
    VM_TRANSFORM_HID_ADD_PRE,                   /*!< HID add pre : register a handle for 
                                                       tranform pre-fix processing
                                                       and octet to add in the beginning.

                                                       Use the "hid_add_pre_info" structure 
                                                       to pass information. It is upto the 
                                                       application to release the allocated 
                                                       memory. */
    VM_TRANSFORM_HID_DISCARD_PRE,                /*!< HID discard pre : octets to discard in the begining */
    VM_TRANSFORM_RTP_PAYLOAD_HEADER_SIZE,        /*!< Payload header size.*/
    VM_TRANSFORM_RTP_DECODE_DONT_REMOVE_PAYLOAD_HEADER, /*!< Is payload header required for RTP decode? */
    VM_TRANSFORM_RTP_ENCODE_PACKET_SIZE,        /*!< RTP Encode packet size.*/
    VM_TRANSFORM_RTP_ENCODE_MANAGE_TIMING,      /*!< RTP Encode Manage Timing */
    VM_TRANSFORM_RTP_ENCODE_STATS,              /*!< RTP Encode statistics */
    VM_TRANSFORM_RTP_ENCODE_FRAME_PERIOD,       /*!< Frame period per packet in us */
    VM_TRANSFORM_RTP_ENCODE_EARLY_LIMIT,        /*!< Early limit time in us*/
    VM_TRANSFORM_RTP_CODEC_TYPE,                /*!< RTP Codec type*/
    VM_TRANSFORM_HID_REPORT_MAX_SIZE            /*!< HID report maximum size */
} vm_transform_config_key;

typedef enum
{
    VM_TRANSFORM_RTP_CODEC_APTX,                /*!< Set CODEC to APTX */
    VM_TRANSFORM_RTP_CODEC_SBC,                 /*!< Set CODEC to SBC */
    VM_TRANSFORM_RTP_CODEC_ATRAC,               /*!< Set CODEC to ATRAC */
    VM_TRANSFORM_RTP_CODEC_MP3,                 /*!< Set CODEC to MP3 */
    VM_TRANSFORM_RTP_CODEC_AAC                  /*!< Set CODEC to AAC */
}vm_transform_rtp_codec_type;

typedef enum
{
    PCM_CLK_256,    /*!< Set PCM CLK rate to 256kHz.*/
    PCM_CLK_128,    /*!< Set PCM CLK rate to 128kHz.*/
    PCM_CLK_512,    /*!< Set PCM CLK rate to 512kHz.*/
    PCM_CLK_OFF     /*!< Disable PCM CLK output.*/
}vm_pcm_clock_setting;

enum {
    INVALID_SENSOR = -32766,        /*!< Returned by VmGetTemperatureBySensor() when called with an invalid sensor number.*/
    INVALID_TEMPERATURE = -32767    /*!< Returned by VmGetTemperature() and VmGetTemperatureBySensor() when the temperature cannot be determined.*/
};

typedef enum
{
    TSENSOR_MAIN,   /*!< Main temperature sensor.*/
    TSENSOR_PMU     /*!< PMU temperature sensor.*/
} vm_temp_sensor;

/*!
  @brief Enumerated type indicating reset source. This is the return type of
         VmGetResetSource().

         When a system reset occurs on BC7 chips a value is stored which
         relates to the cause of the reset.  This value can be retrieved by a
         VM app and is represented by the following enumerated values.
*/
typedef enum
{
    /*! Power on or RST# asserted. */
    RESET_SOURCE_POWER_ON,

    /*! Firmware reset. */
    RESET_SOURCE_FIRMWARE,

    /*! BREAK signal received on the UART.
     *  When a host computer is linked to the device by its UART, it can request
     *  a reset by issuing a BREAK request, i.e. holding the UART's Rx line low
     *  for a set period of time.  This period is given by a non-zero value of
     *  PSKEY_HOSTIO_UART_RESET_TIMEOUT. */
    RESET_SOURCE_UART_BREAK,

    /*! The charger has been plugged into the device.
     *  This is only valid on CSR8670 and CSR8670-like chips and is enabled
     *  using PSKEY_RESET_ON_CHARGER_ATTACH. */
    RESET_SOURCE_CHARGER,

    /*! Reset caused by other hardware.
     *  Any value not covered by this definition cannot be determined and is
     *  deemed an unexpected reset */
    UNEXPECTED_RESET
} vm_reset_source;

/*!
    @brief Bit masks representing the source of the voltage regulator enable
           signal on (re)boot. This type is used by VmGetPowerSource() to
           construct the bit-pattern which is its return value.

           On CSR8670 and CSR8670-like chips, there are three ways to keep the
           chip's power regulators enabled.  These are represented by the
           following enumerated values.

           Any combination of these is possible, dependent upon prevailing
           conditions and PSKEY settings.
*/
typedef enum
{
    /*! The charger has been plugged in to the device. */
    POWER_ENABLER_CHARGER_ATTACHED = 0x0001,

    /*! The Reset Protection Circuit has been triggered.
     *  This circuit determines whether the power regulator should be kept on
     *  during a reset, allowing the chip to reboot.  This would occur, for
     *  example, after an ESD strike.  This circuit is enabled using
     *  PSKEY_RESET_PROTECTION_CTRL_ENABLE. */
    POWER_ENABLER_RESET_PROTECTION = 0x0002,

    /*! The Vreg enable signal was active during the boot sequence. */
    POWER_ENABLER_VREG_EN = 0x0004
} vm_power_enabler;

/*!
    @brief  Dfu from SQIF operation status
*/
typedef enum {
    DFU_SQIF_STATUS_SUCCESS = 0x0001,  /*!< dfu from sqif status success.*/
    DFU_SQIF_STATUS_ERROR  = 0x0002  /*!< dfu from sqif file validation failure.*/
} vm_dfu_sqif_status;

/*!
    @brief
        To disable the VM software watchdog, Application has to call
        VmSoftwareWdKick trap with three disable codes(53281, 60681, 0)
        in a sequence.
        Eg: 
            VmSoftwareWdKick(VM_SW_WATCHDOG_DISABLE_CODE1)
            VmSoftwareWdKick(VM_SW_WATCHDOG_DISABLE_CODE2)
            VmSoftwareWdKick(VM_SW_WATCHDOG_DISABLE_CODE3)
*/
#define VM_SW_WATCHDOG_DISABLE_CODE1 ((uint16)0xD021)
#define VM_SW_WATCHDOG_DISABLE_CODE2 ((uint16)0xED09)
#define VM_SW_WATCHDOG_DISABLE_CODE3 ((uint16)0x0000)

#endif  /* __APP_VM_IF_H__ */
