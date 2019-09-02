/*
Copyright (c) 2005 - 2016 Qualcomm Technologies International, Ltd.
*/

/*!
\file
\ingroup sink_app
\brief   Interface to the AVRCP profile functionality. 
*/

#ifndef _SINK_AVRCP_H_
#define _SINK_AVRCP_H_

#ifdef ENABLE_AVRCP


#include <message.h>
#include <app/message/system_message.h>
#include "sink_volume.h"
#include <avrcp.h>


#ifdef ENABLE_PEER

    typedef enum{
        None = 0x00,
        A2DP_AUDIO = 0x01,
        USB_AUDIO = 0x02,
        ANALOG_AUDIO = 0x04
    }AudioSrc;

    typedef struct
    {
        uint8 lap[3];        
        uint8 uap;
        uint8 nap[2];        
    }byte_aligned_bd_addr_t;

    typedef struct
    {
        AudioSrc src;
        uint8 isConnected;        
        byte_aligned_bd_addr_t bd_addr;
    }audio_src_conn_state_t;

#endif


/* Define to allow display of Now Playing information */
#define ENABLE_AVRCP_NOW_PLAYINGx
/* Define to allow display of Player Application Settings */
#define ENABLE_AVRCP_PLAYER_APP_SETTINGSx

/* Defines for general AVRCP operation */
#define MAX_AVRCP_CONNECTIONS 2                 /* max AVRCP connections allowed */
#define DEFAULT_AVRCP_1ST_CONNECTION_DELAY 2000 /* delay in millsecs before connecting AVRCP after A2DP signalling connection */
#define DEFAULT_AVRCP_2ND_CONNECTION_DELAY 500  /* delay in millsecs before connecting AVRCP with the 2nd attempt */
#define DEFAULT_AVRCP_NO_CONNECTION_DELAY 0     /* no delay before connecting AVRCP */

#define AVRCP_MAX_PENDING_COMMANDS 4            /* maximum AVRCP commands that can be queued */
#define for_all_avrcp(idx) for((idx) = 0; (idx) < MAX_AVRCP_CONNECTIONS; (idx)++)
#define AVRCP_MAX_LENGTH_CAPABILITIES 1         /* maximum no of capabilities supported by HS except for mandatory ones */

/* Macros for creating messages. */
#define MAKE_AVRCP_MESSAGE(TYPE) TYPE##_T *message = PanicUnlessNew(TYPE##_T);
#define MAKE_AVRCP_MESSAGE_WITH_LEN(TYPE, LEN) TYPE##_T *message = (TYPE##_T *)PanicUnlessMalloc(sizeof(TYPE##_T) + (LEN));

/* Defines for fragmentation */
#define AVRCP_PACKET_TYPE_SINGLE 0
#define AVRCP_PACKET_TYPE_START 1
#define AVRCP_PACKET_TYPE_CONTINUE 2
#define AVRCP_PACKET_TYPE_END 3

#define AVRCP_ABORT_CONTINUING_TIMEOUT 5000 /* amount of time to wait to receive fragment of Metadata packet before aborting */

#define AVRCP_GET_ELEMENT_ATTRIBUTES_CFM_HEADER_SIZE 8 /* amount of fixed data in Source of AVRCP_GET_ELEMENT_ATTRIBUTES_CFM_T message before variable length data */
#define AVRCP_GET_APP_ATTRIBUTES_TEXT_CFM_HEADER_SIZE 4 /* amount of fixed data in Source of AVRCP_GET_APP_ATTRIBUTE_TEXT_CFM_T message before variable length data */
#define AVRCP_GET_APP_VALUE_CFM_DATA_SIZE 2 /* amount of fixed data in Source of AVRCP_GET_APP_VALUE_CFM_T message before variable length data */

#define AVRCP_PLAYER_APP_SETTINGS_DATA_LEN    2   /* Minimum single attribute data len where 1-byte attribute and 1-byte setting */

/* Define for receiving Playback Position Changed information */
#define AVRCP_PLAYBACK_POSITION_TIME_INTERVAL 1

#define AVRCP_FF_REW_REPEAT_INTERVAL 1500

/* Define for power table settings for AVRCP*/
#define AVRCP_ACTIVE_MODE_INTERVAL  0x0a

/* Extend library messages with application specific messages */
typedef enum
{
    SINK_AVRCP_VENDOR_UNIQUE_PASSTHROUGH_REQ = AVRCP_MESSAGE_TOP,
    
    SINK_AVRCP_MESSAGE_TOP
} SinkAvrcpMessageId;

typedef struct
{
    uint16 avrcp_index;     /* Index into avrcp_link_data structure */
    uint16 cmd_id;          /* Vendor unique command id */
    uint16 size_data;       /* Size of any payload data */
    uint8 data[1];          /* The payload data */
} SINK_AVRCP_VENDOR_UNIQUE_PASSTHROUGH_REQ_T;

typedef enum
{
    AVRCP_PEER_CMD_NOP,
    AVRCP_PEER_CMD_PEER_STATUS_CHANGE,
    AVRCP_PEER_CMD_TWS_AUDIO_ROUTING,
    AVRCP_PEER_CMD_TWS_VOLUME,
    AVRCP_PEER_CMD_AUDIO_CONNECTION_STATUS,
    AVRCP_PEER_CMD_UPDATE_AUDIO_ENHANCEMENT_SETTINGS,
    AVRCP_PEER_CMD_UPDATE_USER_EQ_SETTINGS,
    AVRCP_PEER_CMD_UPDATE_TRIM_VOLUME,
    AVRCP_PEER_CMD_REQUEST_USER_EQ_SETTINGS,
    AVRCP_PEER_CMD_UPDATE_MUTE,
    AVRCP_PEER_CMD_UPDATE_BATTERY_LEVEL,
	AVRCP_PEER_CMD_UPDATE_LED_INDICATION_ON_OFF
} avrcp_peer_cmd;

#define VENDORDEPENDENT_COMPANY_ID  (0x001958)   /* Mandatory company id for vendor dependent commands */
#define VENDOR_CMD_HDR_SIZE         (2)
#define VENDOR_CMD_TOTAL_SIZE(n)    (uint16)((n) + (VENDOR_CMD_HDR_SIZE))

#define AVRCP_PAYLOAD_PEER_CMD_NOP                0
#define AVRCP_PAYLOAD_PEER_CMD_PEER_STATUS_CHANGE 1
#define AVRCP_PAYLOAD_PEER_CMD_TWS_AUDIO_ROUTING  2
#define AVRCP_PAYLOAD_PEER_CMD_TWS_VOLUME         1
#define AVRCP_PAYLOAD_PEER_CMD_AUDIO_CONNECTION_STATUS 8
#define AVRCP_PAYLOAD_PEER_CMD_UPDATE_AUDIO_ENHANCEMENT_SETTINGS 2
#define AVRCP_PAYLOAD_PEER_CMD_UPDATE_USER_EQ_SETTINGS 37
#define AVRCP_PAYLOAD_PEER_CMD_UPDATE_TRIM_VOLUME 1
#define AVRCP_PAYLOAD_PEER_CMD_REQUEST_USER_EQ_SETTINGS 0
#define AVRCP_PAYLOAD_PEER_CMD_UPDATE_MUTE  1
#define AVRCP_PAYLOAD_PEER_CMD_UPDATE_LED_INDICATION_ON_OFF 1
#define AVRCP_PAYLOAD_PEER_CMD_UPDATE_BATTERY_LEVEL 2

typedef enum
{
    AVRCP_CTRL_NOP,                     /* Does not cause a passthrough command to be generated */
    AVRCP_CTRL_PAUSE_PRESS,
    AVRCP_CTRL_PAUSE_RELEASE,
    AVRCP_CTRL_PLAY_PRESS,
    AVRCP_CTRL_PLAY_RELEASE,
    AVRCP_CTRL_FORWARD_PRESS,
    AVRCP_CTRL_FORWARD_RELEASE,
    AVRCP_CTRL_BACKWARD_PRESS,
    AVRCP_CTRL_BACKWARD_RELEASE,
    AVRCP_CTRL_STOP_PRESS,
    AVRCP_CTRL_STOP_RELEASE,
    AVRCP_CTRL_FF_PRESS,
    AVRCP_CTRL_FF_RELEASE,
    AVRCP_CTRL_REW_PRESS,
    AVRCP_CTRL_REW_RELEASE,
    AVRCP_CTRL_NEXT_GROUP_PRESS,
    AVRCP_CTRL_NEXT_GROUP_RELEASE,
    AVRCP_CTRL_PREVIOUS_GROUP_PRESS,
    AVRCP_CTRL_PREVIOUS_GROUP_RELEASE,
    AVRCP_CTRL_VOLUME_UP_PRESS,
    AVRCP_CTRL_VOLUME_UP_RELEASE,
    AVRCP_CTRL_VOLUME_DOWN_PRESS,
    AVRCP_CTRL_VOLUME_DOWN_RELEASE,
    AVRCP_CTRL_ABORT_CONTINUING_RESPONSE,
    AVRCP_CTRL_POWER_OFF
} avrcp_controls;

typedef enum
{
  AVRCP_CONTROL_SEND,
  AVRCP_CREATE_CONNECTION
} avrcp_ctrl_message;

typedef enum
{
  AVRCP_SHUFFLE_OFF = 1,
  AVRCP_SHUFFLE_ALL_TRACK,
  AVRCP_SHUFFLE_GROUP
} avrcp_shuffle_t;

typedef enum
{
  AVRCP_REPEAT_OFF = 1,
  AVRCP_REPEAT_SINGLE_TRACK,
  AVRCP_REPEAT_ALL_TRACK,
  AVRCP_REPEAT_GROUP
} avrcp_repeat_t;

typedef struct
{
    avrcp_controls control;
    uint16 index;
} AVRCP_CONTROL_SEND_T;

typedef struct
{
    bdaddr bd_addr;
} AVRCP_CREATE_CONNECTION_T;

typedef struct
{
    uint16 pdu_id;
} AVRCP_CTRL_ABORT_CONTINUING_RESPONSE_T;

typedef struct 
{
    TaskData            task;
    uint8               *data;
} SinkAvrcpCleanUpTask;

typedef enum
{
    /* Internally, we | FWD and REV onto the playback status so that
       when we clear it, we are left with the previous value (if any).
       As a result the previous play status is remembered and we can
       FWD/REV whilst stopped, paused or playing */
    sinkavrcp_play_status_stopped = avrcp_play_status_stopped,
    sinkavrcp_play_status_playing = avrcp_play_status_playing,
    sinkavrcp_play_status_paused = avrcp_play_status_paused,
    sinkavrcp_play_status_fwd_seek = (1 << avrcp_play_status_fwd_seek),
    sinkavrcp_play_status_rev_seek = (1 << avrcp_play_status_rev_seek),
    sinkavrcp_play_status_error = avrcp_play_status_error
} SinkAvrcp_play_status;

typedef struct
{
    TaskData avrcp_ctrl_handler[MAX_AVRCP_CONNECTIONS];
    SinkAvrcpCleanUpTask        dataCleanUpTask[MAX_AVRCP_CONNECTIONS];
    unsigned active_avrcp:2;
    unsigned avrcp_manual_connect:1;
    unsigned unused:13;
    uint16 extensions[MAX_AVRCP_CONNECTIONS];
    uint16 features[MAX_AVRCP_CONNECTIONS];
    uint16 event_capabilities[MAX_AVRCP_CONNECTIONS];
    bool connected[MAX_AVRCP_CONNECTIONS];
    AVRCP *avrcp[MAX_AVRCP_CONNECTIONS];
    uint16 pending_cmd[MAX_AVRCP_CONNECTIONS];
    uint16 cmd_queue_size[MAX_AVRCP_CONNECTIONS];
    bdaddr bd_addr[MAX_AVRCP_CONNECTIONS];
    uint16 registered_events[MAX_AVRCP_CONNECTIONS];
    SinkAvrcp_play_status play_status[MAX_AVRCP_CONNECTIONS];
    uint8 absolute_volume[MAX_AVRCP_CONNECTIONS];
    bdaddr avrcp_play_addr;
    bool   link_active[MAX_AVRCP_CONNECTIONS];
    uint8 *vendor_data[MAX_AVRCP_CONNECTIONS];
    uint16 browsing_channel[MAX_AVRCP_CONNECTIONS];   
#ifdef ENABLE_AVRCP_BROWSING
    TaskData avrcp_browsing_handler[MAX_AVRCP_CONNECTIONS];
    uint16 uid_counter[MAX_AVRCP_CONNECTIONS];
    uint16 media_player_features[MAX_AVRCP_CONNECTIONS];
    uint16 media_player_id[MAX_AVRCP_CONNECTIONS];
    uint16 browsing_scope[MAX_AVRCP_CONNECTIONS];
#endif
} avrcp_data;

typedef enum
{
    AVRCP_PAUSE,
    AVRCP_PLAY    
}avrcp_remote_actions;

/* initialisation */
void sinkAvrcpInit(void);

bool avrcpGetPeerIndex (uint16 *peer_index);

/* connection */
void sinkAvrcpConnect(const bdaddr *bd_addr, uint16 delay_time);

void sinkAvrcpDisconnect(const bdaddr *bd_addr);
        
void sinkAvrcpDisconnectAll(bool disconnect_peer);

void sinkAvrcpAclClosed(bdaddr bd_addr);  

uint16 getAvrcpQueueSpace(uint16 Index);

void avrcpSendControlMessage(avrcp_controls control, uint16 Index);

/* media commands */
#ifdef ENABLE_PEER
void sinkAvrcpVolumeUp (void);
void sinkAvrcpVolumeDown (void);
void sinkAvrcpPowerOff (void);
#endif

void sinkAvrcpPlay(void);

void sinkAvrcpPause(void);

void sinkAvrcpPlayPause(void);

void sinkAvrcpStop(void);

void sinkAvrcpSkipForward(void);

void sinkAvrcpSkipBackward(void);

void sinkAvrcpFastForwardPress(void);

void sinkAvrcpFastForwardRelease(void);

void sinkAvrcpRewindPress(void);

void sinkAvrcpRewindRelease(void);

void sinkAvrcpNextGroupPress(void);

void sinkAvrcpNextGroupRelease(void);

void sinkAvrcpPreviousGroupPress(void);

void sinkAvrcpPreviousGroupRelease(void);

/* general helper functions */
void sinkAvrcpToggleActiveConnection(void);

bool sinkAvrcpGetIndexFromInstance(const AVRCP *avrcp, uint16 *Index);

bool sinkAvrcpGetIndexFromBdaddr(const bdaddr *bd_addr, uint16 *Index, bool require_connection);

void sinkAvrcpSetActiveConnectionFromBdaddr(const bdaddr *bd_addr);

uint16 sinkAvrcpGetActiveConnection(void);

uint16 sinkAvrcpGetNumberConnections(void);

void sinkAvrcpSetPlayStatus(const bdaddr *bd_addr, avrcp_play_status play_status);

void avrcpUpdatePeerPlayStatus (avrcp_play_status play_status);

bool sinkAvrcpCheckManualConnectReset(const bdaddr *bd_addr);

void sinkAvrcpDisplayMediaAttributes(uint32 attribute_id, uint16 attribute_length, const uint8 *data);

#ifdef ENABLE_PEER
void sinkAvrcpUpdatePeerWirelessSourceConnected (AudioSrc audio_source, const bdaddr * bd_addr);
void sinkAvrcpUpdatePeerWiredSourceConnected (AudioSrc audio_source);
void sinkAvrcpUpdatePeerSourceDisconnected (AudioSrc audio_source);
void sinkAvrcpUpdatePeerMute (bool OnOrOff);
void sinkAvrcpUpdateLedIndicationOnOff(bool OnorOff);
bool sinkAvrcpIsAvrcpIndexPeer (uint16 avrcpIndex, uint16 *peerIndex);
#endif

#ifdef ENABLE_AVRCP_NOW_PLAYING
/* Now Playing information commands */
void sinkAvrcpRetrieveNowPlayingRequest(uint32 track_index_high, uint32 track_index_low, bool full_attributes);

void sinkAvrcpRetrieveNowPlayingNoBrowsingRequest(uint16 Index, bool full_attributes);
#endif /* ENABLE_AVRCP_NOW_PLAYING */

#ifdef ENABLE_AVRCP_PLAYER_APP_SETTINGS
/* Player Application Settings commands */ 
bool sinkAvrcpListPlayerAttributesRequest(void);

bool sinkAvrcpListPlayerAttributesTextRequest(uint16 size_attributes, Source attributes);

bool sinkAvrcpListPlayerValuesRequest(uint8 attribute_id);

bool sinkAvrcpGetPlayerValueRequest(uint16 size_attributes, Source attributes);

bool sinkAvrcpListPlayerValueTextRequest(uint16 attribute_id, uint16 size_values, Source values);

bool sinkAvrcpSetPlayerValueRequest(uint16 size_attributes, Source attributes);

bool sinkAvrcpInformBatteryStatusRequest(avrcp_battery_status status);
#endif /* ENABLE_AVRCP_PLAYER_APP_SETTINGS */

bool sinkAvrcpPlayPauseRequest(uint16 Index, avrcp_remote_actions action);

bool sinkAvrcpStopRequest(uint16 Index);

/* message handler for AVRCP */
void sinkAvrcpHandleMessage(Task task, MessageId id, Message message);

void sinkAvrcpManualConnect(void);

void sinkAvrcpDataCleanUp(Task task, MessageId id, Message message);

Source sinkAvrcpSourceFromData(SinkAvrcpCleanUpTask *avrcp, uint8 *data, uint16 length);

void sinkAvrcpShuffle(avrcp_shuffle_t type);

uint8 sinkAvrcpGetShuffleType(avrcp_shuffle_t type);

void sinkAvrcpRepeat(avrcp_repeat_t type);

uint8 sinkAvrcpGetRepeatType(avrcp_repeat_t type);

bool sinkAvrcpVendorUniquePassthroughRequest (uint16 avrcp_index, uint16 cmd_id, uint16 size_data, const uint8 *data);

#endif /* ENABLE_AVRCP */

#define AVRCP_MAX_ABS_VOL 127                   /* the maximum value for absolute volume as defined in AVRCP spec */
#define AVRCP_MIN_ABS_VOL 0

#ifdef ENABLE_AVRCP
void sinkAvrcpUpdateVolume(const int16 volume);
#else
#define sinkAvrcpUpdateVolume(volume) ((void)(0))
#endif

#ifdef ENABLE_AVRCP
bool avrcpAvrcpIsEnabled(void);
#else
#define avrcpAvrcpIsEnabled() (FALSE)
#endif


/****************************************************************************
NAME
    avrcpSetInstanceBasedOnSourceRequested

DESCRIPTION
    Called when manually selecting AG1 or AG2 input, this function determines
    if it is possible to ascertain which AVRCP connections are tied to which
    AG and sets the active AVRCP instance if it is able to do so based on the
    selected audio source

RETURNS
    None
*/
#ifdef ENABLE_AVRCP
void avrcpSetInstanceBasedOnSourceRequested(audio_sources source);
#else
#define avrcpSetInstanceBasedOnSourceRequested(x)
#endif

/****************************************************************************
NAME    
    sinkAvrcpCheckStateMatch
    
DESCRIPTION
    helper function to check an avrcp play status for a given a2dp link priority
    
RETURNS
    true is the avrcp play state matches that passed in
    false if state does not match or no matching avrcp link is found for a2dp profile index
*/
#ifdef ENABLE_AVRCP
bool sinkAvrcpCheckStateMatch(const bdaddr *bd_addr, avrcp_play_status play_status);
#else
#define sinkAvrcpCheckStateMatch(bd_addr, play_status) (TRUE)
#endif

#endif /* _SINK_AVRCP_H_ */

