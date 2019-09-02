/****************************************************************************

        Copyright (c) 2004 - 2016 Qualcomm Technologies International, Ltd.
        All Rights Reserved.
        Qualcomm Technologies International, Ltd. Confidential and Proprietary.

FILE
        system_messages.h

CONTAINS
        System message identifiers.

DESCRIPTION

        Defines the MessageId's for messages generated in the firmware but
        destined for the VM application.

*/

/*! @file system_message.h @brief System Message Identifiers

    Defines the MessageId's for messages generated in
    the firmware but destined for the VM application.
 */


#ifndef APP_SYSTEM_MESSAGE_H__
#define APP_SYSTEM_MESSAGE_H__

#include "app/adc/adc_if.h"
#include "app/usb/usb_if.h"
#include "app/vm/vm_if.h"
#include "app/capsense/capsense_if.h"
#include "app/partition/partition_if.h"
#include "app/infrared/infrared_if.h"


#include <source_.h>
#include <sink_.h>

#define SYSTEM_MESSAGE_BASE_               0x8000 /*!< All system message numbers are relative to this.*/

/*
   These Id's *must* correspond to the same primitive classes defined for
   BlueStack in app/bluestack/bluetooth.h, but we don't want to pull in those
   headers here.

   The message points to the actual primitive.
*/

/*!
  All Bluestack message numbers are relative to this.

  Bluestack messages are sent to the task registered with MessageBlueStackTask(),
  except MESSAGE_BLUESTACK_ATT_PRIM which are routed to the task registered with MessageAttTask().
*/
#define MESSAGE_BLUESTACK_BASE_           (SYSTEM_MESSAGE_BASE_)

#define MESSAGE_BLUESTACK_LC_PRIM         (MESSAGE_BLUESTACK_BASE_ + 1)  /*!< Link Controller primitive from Bluestack.*/
#define MESSAGE_BLUESTACK_LM_PRIM         (MESSAGE_BLUESTACK_BASE_ + 2)  /*!< Link Manager primitive from Bluestack.*/
#define MESSAGE_BLUESTACK_HCI_PRIM        (MESSAGE_BLUESTACK_BASE_ + 3)  /*!< Host Controller Interface primitive from Bluestack.*/
#define MESSAGE_BLUESTACK_DM_PRIM         (MESSAGE_BLUESTACK_BASE_ + 4)  /*!< Device Manager primitive from Bluestack.*/
#define MESSAGE_BLUESTACK_L2CAP_PRIM      (MESSAGE_BLUESTACK_BASE_ + 5)  /*!< L2CAP primitive from Bluestack.*/
#define MESSAGE_BLUESTACK_RFCOMM_PRIM     (MESSAGE_BLUESTACK_BASE_ + 6)  /*!< RFCOM primitive from Bluestack.*/
#define MESSAGE_BLUESTACK_SDP_PRIM        (MESSAGE_BLUESTACK_BASE_ + 7)  /*!< Service Discovery Protocol primitive.*/
#define MESSAGE_BLUESTACK_BCSP_LM_PRIM    (MESSAGE_BLUESTACK_BASE_ + 8)  /*!< BCSP LM primitive from Bluestack.*/
#define MESSAGE_BLUESTACK_BCSP_HQ_PRIM    (MESSAGE_BLUESTACK_BASE_ + 9)  /*!< BCSP HQ primitive from Bluestack.*/
#define MESSAGE_BLUESTACK_BCSP_BCCMD_PRIM (MESSAGE_BLUESTACK_BASE_ + 10) /*!< BCSP BCCMD primitive from Bluestack.*/
#define MESSAGE_BLUESTACK_CALLBACK_PRIM   (MESSAGE_BLUESTACK_BASE_ + 11) /*!< Callback primitive from Bluestack.*/
#define MESSAGE_BLUESTACK_TCS_PRIM        (MESSAGE_BLUESTACK_BASE_ + 12) /*!< TCS primitive from Bluestack.*/
#define MESSAGE_BLUESTACK_BNEP_PRIM       (MESSAGE_BLUESTACK_BASE_ + 13) /*!< BNEP primitive from Bluestack.*/
#define MESSAGE_BLUESTACK_TCP_PRIM        (MESSAGE_BLUESTACK_BASE_ + 14) /*!< TCP primitive from Bluestack.*/
#define MESSAGE_BLUESTACK_UDP_PRIM        (MESSAGE_BLUESTACK_BASE_ + 15) /*!< UDP primitive from Bluestack.*/
#define MESSAGE_BLUESTACK_FB_PRIM         (MESSAGE_BLUESTACK_BASE_ + 16) /*!< FB primitive from Bluestack.*/
#define MESSAGE_BLUESTACK_ATT_PRIM        (MESSAGE_BLUESTACK_BASE_ + 18) /*!< ATT primitive from Bluestack.*/

#define MESSAGE_BLUESTACK_END_            (MESSAGE_BLUESTACK_BASE_ + 19) /*!< End of Bluestack primitives.*/

/*
    Word-oriented message over BCSP#13. Message is the legacy
    word-format with length+type+payload.
*/

/*!
  Message received from the host. The message content has the format
  described at \ref host_messages.

  This message is sent to the task registered with MessageHostCommsTask().
 */
#define MESSAGE_FROM_HOST                 (SYSTEM_MESSAGE_BASE_ + 32)

/*!
  A Source has more data. The message content is a #MessageMoreData.

  This message is sent to any task associated with the Source using MessageSinkTask().

  The frequency of this message can be controlled with #VM_SOURCE_MESSAGES.
 */
#define MESSAGE_MORE_DATA                 (SYSTEM_MESSAGE_BASE_ + 33)

/*!
  A Sink has more space. The message content is a #MessageMoreSpace.

  This message is sent to any task associated with the Sink using MessageSinkTask().

  The frequency of this message can be controlled with #VM_SINK_MESSAGES.
 */
#define MESSAGE_MORE_SPACE                (SYSTEM_MESSAGE_BASE_ + 34)

/*!
  A PIO has changed. The message content is a #MessagePioChanged.

  This message is sent to the task registered with MessagePioTask().
 */
#define MESSAGE_PIO_CHANGED               (SYSTEM_MESSAGE_BASE_ + 35)

/*!
  A message from Kalimba has arrived. The message content is a
  #MessageFromKalimba.

  This message is sent to the task registered with MessageKalimbaTask().
 */
#define MESSAGE_FROM_KALIMBA              (SYSTEM_MESSAGE_BASE_ + 36)

/*!
  A requested ADC measurement has completed. The message content is
  a #MessageAdcResult.

  This message is sent to the task passed to AdcRequest().
 */
#define MESSAGE_ADC_RESULT                (SYSTEM_MESSAGE_BASE_ + 37)

/*!
  A stream has disconnected. The message content is a
  #MessageStreamDisconnect.

  This message is sent to any task associated with the stream using MessageSinkTask().
 */
#define MESSAGE_STREAM_DISCONNECT         (SYSTEM_MESSAGE_BASE_ + 38)

/*!
  The energy level in a SCO stream has changed. The message content is a
  #MessageEnergyChanged.

  This message is sent to any task associated with the stream using MessageSinkTask().

  It is enabled with EnergyEstimationSetBounds().
 */
#define MESSAGE_ENERGY_CHANGED            (SYSTEM_MESSAGE_BASE_ + 39)

/*!
  The values returned by StatusQuery() may have changed. (No message content.)

  This message is sent to the task registered with MessageStatusTask().
 */
#define MESSAGE_STATUS_CHANGED            (SYSTEM_MESSAGE_BASE_ + 40)

/*!
  A Source is empty. The message content is a #MessageSourceEmpty.

  This message is sent to any task associated with the Source using MessageSinkTask().
 */
#define MESSAGE_SOURCE_EMPTY              (SYSTEM_MESSAGE_BASE_ + 41)

/*!
  A long message from Kalimba has arrived. The message content is a
  #MessageFromKalimbaLong.

  This message is sent to the task registered with MessageKalimbaTask().
 */
#define MESSAGE_FROM_KALIMBA_LONG         (SYSTEM_MESSAGE_BASE_ + 42)

/*!
  BlueCore has enumerated as a USB device. The message content is a
  #MessageUsbConfigValue, telling you which configuration has been
  set by the host.

  This message is sent to the task registered with MessageSystemTask().
 */
#define MESSAGE_USB_ENUMERATED            (SYSTEM_MESSAGE_BASE_ + 43)

/*!
  BlueCore has suspended or resumed as a USB device; the message
  is a #MessageUsbSuspended and indicates which.

  This message is not sent if BlueCore is bus powered.

  Note that only the most recent change is reported, so adjacent
  messages may report the same status.

  This message is sent to the task registered with MessageSystemTask().
*/
#define MESSAGE_USB_SUSPENDED             (SYSTEM_MESSAGE_BASE_ + 44)

/*!
  The charger hardware has changed state.

  Note when user removes the charger, the firmware has already selected
  the battery as the power source, i.e. equivalent to VM executing
  PsuConfigure(PSU_VBAT_SWITCH, PSU_SMPS_INPUT_SEL_VBAT, 1).

  The message content is a #MessageChargerChanged.

  This message is sent to the task registered with MessageChargerTask().
*/
#define MESSAGE_CHARGER_CHANGED           (SYSTEM_MESSAGE_BASE_ + 45)

/*!
  A fault was detected in flash-based PS. (No message content.)

  This message is sent to the task registered with MessageSystemTask().
*/
#define MESSAGE_PSFL_FAULT                (SYSTEM_MESSAGE_BASE_ + 46) /*!< A fault was detected in flash-based PS */

/*!
  BlueCore has been deconfigured as a USB device. To maintain symmetry
  with the MESSAGE_USB_ENUMERATED case, this message also contains a
  #MessageUsbConfigValue, although in this case the config_value will
  always be zero.

  This message is sent to the task registered with MessageSystemTask().
*/
#define MESSAGE_USB_DECONFIGURED          (SYSTEM_MESSAGE_BASE_ + 47)

/*!
  BlueCore has changed to alternate settings for a USB interface. The
  message content is a #MessageUsbAltInterface.

  In order to receive MESSAGE_USB_ALT_INTERFACE, an application must first explicitly register
  an interest in this message by using StreamConfigure(#VM_STREAM_USB_ALT_IF_MSG_ENABLED, 1).

  This message is sent to the task registered with MessageSystemTask().
*/
#define MESSAGE_USB_ALT_INTERFACE         (SYSTEM_MESSAGE_BASE_ + 48)

/*!
  Bluecore has detected USB Vbus transition from low to high,
  indicating the device is now attached. (No message content.)

  In order to receive MESSAGE_USB_ATTACHED or MESSAGE_USB_DETACHED, an application must first
  explicitly register an interest in these messages by using StreamConfigure(#VM_STREAM_USB_ATTACH_MSG_ENABLED, 1).

  This message is sent to the task registered with MessageSystemTask().
*/
#define MESSAGE_USB_ATTACHED        (SYSTEM_MESSAGE_BASE_ + 49)

/*!
  Bluecore has detected USB Vbus transition from high to low,
  indicating the device is now detached. (No message content.)

  In order to receive MESSAGE_USB_ATTACHED or MESSAGE_USB_DETACHED, an application must first
  explicitly register an interest in these messages by using StreamConfigure(#VM_STREAM_USB_ATTACH_MSG_ENABLED, 1).

  This message is sent to the task registered with MessageSystemTask().
*/
#define MESSAGE_USB_DETACHED        (SYSTEM_MESSAGE_BASE_ + 50)

/*!
  Bluecore has detected a Kalimba watchdog event indicating
  that the DSP must be restarted. (No message content.)

  This message is sent to the task registered with MessageKalimbaTask().
*/
#define MESSAGE_KALIMBA_WATCHDOG_EVENT     (SYSTEM_MESSAGE_BASE_ + 51)

/*!
  Bluecore has changed Bluetooth transmission power. (No message
  content).

  In order to receive a MESSAGE_TX_POWER_CHANGE_EVENT an
  application must first explicitly register an interest in this
  message by using VmTransmitPowerMessagesEnable(TRUE).  This must be
  done each time the message is received to re-register interest.

  This message can apply to any BR/EDR link; it is the responsibility
  of the application to determine which links, if any, have actually
  changed (the application should be prepared to handle the case where
  there is no net change to the power on any links). Applications
  concerned to catch all power change events should reenable this
  message using VmTransmitPowerMessagesEnable() before checking the
  power on the links.

  If this message is explicitly disabled with
  VmTransmitPowerMessagesEnable(FALSE), it is possible for one last
  event to occur if it was already queued.  Applications concerned by
  this should cancel pending messages.

  This message is sent to the task registered with MessageSystemTask().
*/
#define MESSAGE_TX_POWER_CHANGE_EVENT     (SYSTEM_MESSAGE_BASE_ + 52)

/*!
   A Cap Sense pad has changed. The message content is a MessageCapsenseChanged.
   This message is sent to the task registered with MessageCapsenseTask()
*/
#define MESSAGE_CAPSENSE_CHANGED   (SYSTEM_MESSAGE_BASE_ + 53)


/*!
    Message Id 54 is not in use
*/

/*!
   Result of set message digest for serial flash partition.
   The message content is a MessageStreamSetDigest.
   This message is sent to the task registered with MessageSinkTask()
*/
#define MESSAGE_STREAM_SET_DIGEST (SYSTEM_MESSAGE_BASE_ + 55)

/*!
   Verification of a file system written to serial flash has completes.
   The message content is a MessageStreamPartitionVerify.
   This message is sent to the task registered with MessageSinkTask()
*/
#define MESSAGE_STREAM_PARTITION_VERIFY (SYSTEM_MESSAGE_BASE_ + 56)

/*!
   Verification of the reformatting of the serial flash has completed.
   The message content is a MessageStreamReformatVerify.
   This message is sent to the task registered with MessageSinkTask()
*/
#define MESSAGE_STREAM_REFORMAT_VERIFY (SYSTEM_MESSAGE_BASE_ + 57)

/*!
   An infrared button has been pressed or released. The message content
   is a MessageInfraRedEvent.
   This message is sent to the task registered with MessageInfraRedTask()
*/
#define MESSAGE_INFRARED_EVENT (SYSTEM_MESSAGE_BASE_ + 58)

/*!
   Reports the status of DFU from SQIF operation to the task registered with
   MessageSystemTask(). The message content is a #MessageDFUFromSQifStatus
   
*/
#define MESSAGE_DFU_SQIF_STATUS (SYSTEM_MESSAGE_BASE_ + 59)

/*!
  A message from Operator has arrived. The message content is a
  #MessageFromOperator.

  This message is sent to the task registered with specific operator ID.
 */
#define MESSAGE_FROM_OPERATOR              (SYSTEM_MESSAGE_BASE_ + 60)


/*!
  A message from the host that has been received as BCCMD packet. 

  This message is sent to the task registered with MessageHostBccmdTask().

 */
#define MESSAGE_HOST_BCCMD              (SYSTEM_MESSAGE_BASE_ + 62)

/*!
  #MESSAGE_MORE_DATA: Message type received when more data has arrived at a Source.
 */
typedef struct
{
    Source source; /*!< The source which has more data. */
} MessageMoreData;

/*!
  #MESSAGE_MORE_SPACE: Message type received when a Sink has more space.
*/
typedef struct
{
    Sink sink;    /*!< The sink which has more space. */
} MessageMoreSpace;

/*!
  #MESSAGE_PIO_CHANGED: Message type received when the PIO state has changed.
*/
typedef struct
{
    uint16 state;        /*!< The state of PIO lines 0..15 when this message was sent. */
    uint32 time;         /*!< The time at which that state was valid. */
    uint16 state16to31;  /*!< The state of PIO lines 16..31 when this message was sent. */
} MessagePioChanged;

/*!
  #MESSAGE_ADC_RESULT: Message type received when an ADC reading has been made.
*/
typedef struct
{
    vm_adc_source_type adc_source; /*!< The ADC source for which the reading was made. */
    uint16 reading; /*!< The reading derived from the hardware. See adc.h for how to interpret this reading. */
} MessageAdcResult;

/*!
  #MESSAGE_STREAM_DISCONNECT: Message type received when a Stream disconnects.
*/
typedef struct
{
    Source source; /*<! The source which was involved in the connection. */
    Sink sink;     /*<! The sink which was involved in the connection. */
} MessageStreamDisconnect;

/*!
  #MESSAGE_ENERGY_CHANGED: Message type received when a SCO energy level has passed the configured threshold.
*/
typedef struct
{
    Sink sco_sink; /*<! The SCO sink on which the estimated energy level has passed the configured threshold. */
    bool over_threshold; /*<! TRUE if the threshold was exceeded, else FALSE. */
} MessageEnergyChanged;

/*!
  #MESSAGE_SOURCE_EMPTY: Message type received when a Source is empty.
*/
typedef struct
{
    Source source; /*!< The source which is empty. */
} MessageSourceEmpty;

/*!
  #MESSAGE_FROM_KALIMBA: Message type received from Kalimba.
*/
typedef struct
{
    uint16 id;      /*!< The message id sent by Kalimba */
    uint16 data[4]; /*!< The message payload */
} MessageFromKalimba;

/*!
  #MESSAGE_FROM_KALIMBA_LONG: Long message type received from Kalimba.
*/
typedef struct
{
    uint16 id;      /*!< The message id sent by Kalimba */
    uint16 len;     /*!< The length of the message payload */
    uint16 data[1]; /*!< len words of message payload */
} MessageFromKalimbaLong;

/*!
  Message type received either when the host has set a configuration for the device,
  or when the device is deconfigured. In the latter case, config_value will always
  be zero.
*/
typedef struct
{
    uint16 config_value; /*!< bConfigurationValue for the selected configuration */
} MessageUsbConfigValue;

/*!
  Message type received when USB suspend state has changed.
*/
typedef struct
{
    bool has_suspended; /*!< TRUE if we have suspended, FALSE if we have unsuspended */
} MessageUsbSuspended;

/*!
  Message type received when an alternative setting is applied on a USB interface
*/
typedef struct
{
    uint16 interface;
    uint16 altsetting;
} MessageUsbAltInterface;

/*!
  Message type received when battery charger or VREG_EN pin states have changed.
*/
typedef struct
{
    bool charger_connected; /*!< TRUE if the charger is connected, FALSE if it is not */
    bool vreg_en_high;      /*!< TRUE if VREG_EN is high, FALSE if it is not */
}MessageChargerChanged;

/*!
  Message type received when an event is registered on the capacitive touch sensors
*/
typedef struct
{
    uint16  num_events;         /*!< number of events described in this message */
    capsense_event event[1];    /*!< array of events (length of num_events) */
} MessageCapsenseChanged;

typedef struct
{
    partition_filesystem_devices device; /*!< file system device written to */
    uint16 partition;                    /*!< partition written to */
    partition_set_digest_result set_digest_result;  /*!< result of set digest, see #partition_set_digest_result */
} MessageStreamSetDigest;

typedef struct
{
    partition_filesystem_devices device; /*!< file system device written to */
    uint16 partition;                    /*!< partition written to */
    partition_verify_result verify_result;  /*!< result of verification, see #partition_verify_result */
} MessageStreamPartitionVerify;

typedef struct
{
    partition_filesystem_devices device; /*!< file system device written to */
    partition_verify_result verify_result;  /*!< result of verification, see #partition_verify_result */
} MessageStreamReformatVerify;

/*!
  Message type received when DFU from SQIF process is complete.
*/
typedef struct
{
    vm_dfu_sqif_status status; /*!<  result of  DFU from SQIF process, see #vm_dfu_sqif_status*/
} MessageDFUFromSQifStatus;

/*!
  Message type received when a infra-red event has occurred
*/
typedef struct
{
    infrared_event    event;            /*!< Event type */
    infrared_protocol protocol;         /*!< Infra-red protocol */
    uint32 time;                        /*!< Time in milliseconds event occurred */
    uint16 address;                     /*!< Protocol specific device address */
    uint16 size_data;                   /*!< Size of data[] field */
    uint8 data[1];                      /*!< Protocol specific event data */
} MessageInfraRedEvent;

typedef struct
{
    uint16 len;                      /*!< The length of the message payload */
    uint16 message[1];                      /*!< Unsolicited message from operator */
} MessageFromOperator;


/*!
  #MESSAGE_HOST_BCCMD: Message block from Host received as BCCMD packet.
 */
typedef struct
{
    uint16 len;                         /*!< The length of the message block */
    uint16 message[1];                  /*!< Message block (payload) */
} MessageHostBccmd;
#endif

