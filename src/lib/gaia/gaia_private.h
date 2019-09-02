/*****************************************************************
Copyright (c) 2011 - 2015 Qualcomm Technologies International, Ltd.
*/

#ifndef _GAIA_PRIVATE_H_
#define _GAIA_PRIVATE_H_

#define MESSAGE_PMAKE(NAME,TYPE) TYPE * const NAME = PanicUnlessNew(TYPE)

#define FOREACH_TRANSPORT(I,T) \
for (I = 0; T = &gaia->transport[I], I < gaia->transport_count; ++I)

#include "gaia.h"

/* configure which transports to include in the library
 * start with RFCOMM and GATT defined unless removed by VARIANTS */
#if defined GAIA_TRANSPORT_NO_SPP
#undef GAIA_TRANSPORT_SPP
#elif !defined GAIA_TRANSPORT_SPP
#define GAIA_TRANSPORT_SPP
#endif

#if defined GAIA_TRANSPORT_NO_GATT
#undef GAIA_TRANSPORT_GATT
#elif !defined GAIA_TRANSPORT_GATT
#define GAIA_TRANSPORT_GATT
#endif


/* include the GATT and GATT Manager headers if we're having this transport */
#ifdef GAIA_TRANSPORT_GATT
#include <gatt.h>
#include <gatt_manager.h>
#endif

/* include the SPP Client header if we're having this transport */
#ifdef GAIA_TRANSPORT_SPP
#include <sppc.h>
#endif

/* sanity check; we must have at least one of the transports defined */
#if !defined(GAIA_TRANSPORT_RFCOMM) && !defined(GAIA_TRANSPORT_SPP) && !defined(GAIA_TRANSPORT_GATT)
#error : no transport defined
#endif

#define MAX_SUPPORTED_GAIA_TRANSPORTS   4           /*!< Maximum number of transports Gaia can support */
/* end transport configuration */

/* assume we have no (SQIF) PFS unless enabled by VARIANTS */
#ifdef GAIA_PARTITION_FILESYSTEM
#define HAVE_PARTITION_FILESYSTEM
#define HAVE_DFU_FROM_SQIF
#else
#undef HAVE_PARTITION_FILESYSTEM
#undef HAVE_DFU_FROM_SQIF
#endif

#define GAIA_RAW_CHUNK_SIZE (240)

/* configure what debug is generated */
#ifdef DEBUG_GAIA
#include <stdio.h>
#include <panic.h>
#define GAIA_DEBUG(x) printf x
#define GAIA_PANIC() Panic()
#else
#define GAIA_DEBUG(x)
#define GAIA_PANIC()
#endif

#ifdef DEBUG_GAIA_COMMAND
#include <stdio.h>
#define GAIA_CMD_DEBUG(x) printf x
#else
#define GAIA_CMD_DEBUG(x)
#endif

#ifdef DEBUG_GAIA_TRANSPORT                         /* transport specific debug */
#include <stdio.h>
#define GAIA_TRANS_DEBUG(x) printf x
#else
#define GAIA_TRANS_DEBUG(x)
#endif

#ifdef DEBUG_GAIA_DFU
#include <stdio.h>
#define GAIA_DFU_DEBUG(x) printf x
#else
#define GAIA_DFU_DEBUG(x)
#endif

#ifdef DEBUG_GAIA_THRESHOLDS
#include <stdio.h>
#define GAIA_THRES_DEBUG(x) printf x
#else
#define GAIA_THRES_DEBUG(x)
#endif

/* end debug configuration */

typedef enum
{
    PARTITION_INVALID,
    PARTITION_UNMOUNTED,
    PARTITION_MOUNTED
} gaia_partition_state;

#define BAD_SINK ((Sink) 0xFFFF)
#define BAD_SINK_CLAIM (0xFFFF)

#define PS_MAX_BOOTMODE (7)
#define PFS_MAX_PARTITION (15)
#define DFU_MAX_ENABLE_PERIOD (60)
#define DFU_PACKET_TIMEOUT (5)
#define DFU_PARTITION_OVERHEAD (16)

#define PSKEY_BDADDR (0x0001)
#define PSKEY_LOCAL_DEVICE_NAME (0x0108)
#define PSKEY_MODULE_ID (0x0259)
#define PSKEY_MODULE_DESIGN (0x025A)
#define PSKEY_MOD_MANUF0 (0x025E)
#define PSKEY_USB_VENDOR_ID (0x02BE)

#define BIGGISH_SLOT_SIZE (94)
#define DEB_BUFFER_SIZE BIGGISH_SLOT_SIZE

/* file packet header + data to fit in a 64-word pmalloc slot */
#define FILE_PACKET_HEADER_SIZE (5)
#define FILE_BUFFER_SIZE (58)

/* space allowed for GATT command and response */
#define GATT_BUFFER_SIZE (20)

#define TATA_TIME (1000)
#define APP_BUSY_WAIT_MILLIS (500)

#define HIGH(x) (x >> 8)
#define LOW(x) (x & 0xFF)
#define W16(x) (((*(x)) << 8) | (*((x) + 1)))
#define SEXT(x) ((x) & 0x80 ? (x) | 0xFF00 : (x))
#define ULBIT(x) (1UL << (x))

/*  It's that diagram again ... Gaia V1 protocol packet
 *  0 bytes  1        2        3        4        5        6        7        8          9    len+8      len+9
 *  +--------+--------+--------+--------+--------+--------+--------+--------+ +--------+--/ /---+ +--------+
 *  |  SOF   |VERSION | FLAGS  | LENGTH |    VENDOR ID    |   COMMAND ID    | | PAYLOAD   ...   | | CHECK  |
 *  +--------+--------+--------+--------+--------+--------+--------+--------+ +--------+--/ /---+ +--------+
 */
#define GAIA_OFFS_SOF (0)
#define GAIA_OFFS_VERSION (1)
#define GAIA_OFFS_FLAGS (2)
#define GAIA_OFFS_PAYLOAD_LENGTH (3)

#define GAIA_OFFS_VENDOR_ID (4)
#define GAIA_OFFS_VENDOR_ID_H GAIA_OFFS_VENDOR_ID
#define GAIA_OFFS_VENDOR_ID_L (GAIA_OFFS_VENDOR_ID + 1)
#define GAIA_GATT_OFFS_VENDOR_ID (0)
#define GAIA_GATT_OFFS_VENDOR_ID_H GAIA_GATT_OFFS_VENDOR_ID
#define GAIA_GATT_OFFS_VENDOR_ID_L (GAIA_GATT_OFFS_VENDOR_ID + 1)

#define GAIA_OFFS_COMMAND_ID (6)
#define GAIA_OFFS_COMMAND_ID_H GAIA_OFFS_COMMAND_ID
#define GAIA_OFFS_COMMAND_ID_L (GAIA_OFFS_COMMAND_ID + 1)
#define GAIA_GATT_OFFS_COMMAND_ID (2)
#define GAIA_GATT_OFFS_COMMAND_ID_H GAIA_GATT_OFFS_COMMAND_ID
#define GAIA_GATT_OFFS_COMMAND_ID_L (GAIA_GATT_OFFS_COMMAND_ID + 1)

#define GAIA_OFFS_PAYLOAD (8)
#define GAIA_GATT_OFFS_PAYLOAD (4)


#define GAIA_SOF (0xFF)
#define GAIA_VERSION_01 (1)
#define GAIA_VERSION GAIA_VERSION_01

#define GAIA_INVALID_ID (0xFFFF)

#define GAIA_ACK_NOTIFICATION (GAIA_EVENT_NOTIFICATION | GAIA_ACK_MASK)

#define GAIA_RSSI_HYSTERESIS (2)
#define GAIA_BATT_HYSTERESIS (10)
#define GAIA_CHECK_THRESHOLDS_PERIOD (D_SEC(5))

#define GAIA_STATUS_NONE (0x7FFE)

#define GAIA_API_VERSION_MINOR_MAX (15)

/*  The protocol defines feature bits 0..63 but they aren't all implemented  */
#define GAIA_IMP_MAX_FEATURE_BIT (31)
#define GAIA_IMP_MAX_EVENT_BIT (31)


/*  Internal message ids  */
typedef enum
{
    GAIA_INTERNAL_INIT = 1,
    GAIA_INTERNAL_MORE_DATA,
    GAIA_INTERNAL_SEND_REQ,
	/* Gaia now uses the power lib to get the battery voltage so this is no
	   no longer needed. It is left to keep the following enum values the same. */
    GAIA_INTERNAL_BATTERY_MONITOR_TIMER,
    GAIA_INTERNAL_CHECK_BATTERY_THRESHOLD_REQ,
    GAIA_INTERNAL_CHECK_RSSI_THRESHOLD_REQ,
    GAIA_INTERNAL_DISCONNECT_REQ,
    GAIA_INTERNAL_REBOOT_REQ,
    GAIA_INTERNAL_DFU_REQ,
    GAIA_INTERNAL_DFU_TIMEOUT
} gaia_internal_message;

typedef struct _gaia_transport gaia_transport;
typedef struct _GAIA_T GAIA_T;


typedef struct
{
    gaia_transport *transport;
} GAIA_INTERNAL_MORE_DATA_T;


typedef struct
{
    Task task;
    gaia_transport *transport;
    uint16 length;
    uint8 *data;
} GAIA_INTERNAL_SEND_REQ_T;


typedef struct
{
    gaia_transport *transport;
} GAIA_INTERNAL_DISCONNECT_REQ_T;

/*! @brief definition of internal timer message to check battery level for host on specified transport.
 */
typedef struct
{
    gaia_transport *transport;
} GAIA_INTERNAL_CHECK_BATTERY_THRESHOLD_REQ_T;

/*! @brief definition of internal timer message to check RSSI on a specified transport.
 */
typedef struct
{
    gaia_transport *transport;
} GAIA_INTERNAL_CHECK_RSSI_THRESHOLD_REQ_T;


#if defined GAIA_TRANSPORT_RFCOMM || defined GAIA_TRANSPORT_SPP
/*! @brief Gaia SPP transport state.
 */
typedef struct
{
    uint16 rfcomm_channel;          /*!< RFCOMM channel used by this transport. */
    Sink sink;                  /*!< Stream sink of this transport. */
} gaia_transport_spp_data;

/* helper macro to get SPP specific transport data from generic gaia_transport pointer */
#define SPP_TRANSPORT_DATA(transport)   ((gaia_transport_spp_data*)((transport)->transport_state))

#endif

#ifdef GAIA_TRANSPORT_GATT
/*! @brief GAIA GATT transport state.
*/
typedef struct
{
    uint16 cid;
    unsigned config_not:1; /*!< response notifications enabled. */
    unsigned config_ind:1; /*!< response indications enabled. */
    unsigned :14;
    uint16 size_response;
    uint8 response[GATT_BUFFER_SIZE];
} gaia_transport_gatt_data;
#endif /* GAIA_TRANSPORT_GATT */

/*! @brief Generic Gaia transport data structure.
 */
struct _gaia_transport
{
    gaia_transport_type type;       /*!< the transport type of this instance. */
    unsigned connected:1;           /*!< is this transport connected? */
    unsigned flags:1;               /*!< does this link require Gaia checksum? */
    unsigned enabled:1;             /*!< is this session enabled? */
    unsigned notify_dfu_state:1;    /*!< enable notification of DFU state changes */
    unsigned notify_vmup:1;         /*!< enable notification of VM Upgrade Protocol */
    unsigned :11;                   /*!< explicitly track unused bits in this word */

    int16 battery_lo_threshold[2];
    int16 battery_hi_threshold[2];

    int rssi_lo_threshold[2];
    int rssi_hi_threshold[2];

    unsigned threshold_count_lo_rssi:2;
    unsigned threshold_count_hi_rssi:2;

    unsigned threshold_count_lo_battery:2;
    unsigned threshold_count_hi_battery:2;

    unsigned sent_notification_lo_battery_0:1;
    unsigned sent_notification_lo_battery_1:1;
    unsigned sent_notification_hi_battery_0:1;
    unsigned sent_notification_hi_battery_1:1;

    unsigned sent_notification_lo_rssi_0:1;
    unsigned sent_notification_lo_rssi_1:1;
    unsigned sent_notification_hi_rssi_0:1;
    unsigned sent_notification_hi_rssi_1:1;

    union
    {
#if defined GAIA_TRANSPORT_RFCOMM || defined GAIA_TRANSPORT_SPP
        gaia_transport_spp_data spp;
        
#endif

#ifdef GAIA_TRANSPORT_GATT
        gaia_transport_gatt_data gatt;
#endif
    } state;

};


/*  WARNING: if adding values to this enum make sure that the pfs_state bitfield
 *  below is wide enough
 */
typedef enum
{
    PFS_NONE,
    PFS_PARTITION,
    PFS_FILE,
    PFS_DFU
} pfs_state;


/*  WARNING: if adding values to this enum make sure that the dfu_state bitfield
 *  below is wide enough
 */
typedef enum
{
    DFU_IDLE,
    DFU_WAITING,
    DFU_READY,
    DFU_DOWNLOAD,
    DFU_UPDATE
} dfu_state;


/*! @brief Gaia library main task and state structure. */
struct _GAIA_T
{
    TaskData task_data;
    Task app_task;
    uint32 command_locus_bits;
    uint32 event_locus_bits;
    gaia_transport *outstanding_request;
    gaia_transport *upgrade_transport;

    int16 battery_reference;
    int16 battery_voltage;
    int16 battery_trend;

    uint32 spp_sdp_handle;      /* not per-transport, there can be only one  */
    uint16 spp_listen_channel;  /* not per-transport, there can be only one  */

    /* 1st bitfield set */
    unsigned api_minor:4;
    unsigned rebooting:1;
    unsigned custom_sdp:1;
    unsigned have_pfs:1;
    pfs_state pfs_state:2;
    unsigned pfs_open_stream:4;
    unsigned :3;

    /* 2nd bitfield set */
    dfu_state dfu_state:3;
    unsigned :13;

    union
    {
        Source source;
        Sink sink;
    } pfs;

    uint32 pfs_sequence;
    uint32 pfs_raw_size;

    uint16 transport_count;
    gaia_transport transport[1];    /*!< storage for the gaia transports */
};

extern GAIA_T* gaia;

/* Process a GAIA command or pass it up as unhandled */
void gaiaProcessCommand(gaia_transport *transport, uint16 vendor_id, uint16 command_id, uint16 size_payload, uint8 *payload);

#endif /* ifdef _GAIA_PRIVATE_H_ */
