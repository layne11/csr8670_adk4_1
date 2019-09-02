/*
Copyright (c) 2011 - 2016 Qualcomm Technologies International, Ltd.
*/

/*!
@file    sink_peer.h
@ingroup sink_app
@brief   Interface to the Peer device functionality. 
*/

#ifndef _SINK_PEER_H_
#define _SINK_PEER_H_

#include <bdaddr.h>
#include "audio_plugin_if.h"

/* Declare structure. Header file dependencies preclude including */
struct sink_attributes_str;


/* SDP Attribute UUIDs */
#define UUID_SERVICE_CLASS_ID_LIST (0x0001)
#define UUID_SUPPORTED_CODECS_SHAREME (0xFF01)
#define UUID_SUPPORTED_CODECS_TWS (0xFF02)
#define UUID_SUPPORTED_FEATURES (0x0311)
#define UUID_LEGACY_VERSION (0x0200)
#define UUID_CURRENT_VERSION (0x0201)


typedef enum
{
    PEER_PAIRING_MODE_TEMPORARY,
    PEER_PAIRING_MODE_NORMAL,
    PEER_PAIRING_MODE_PERMANENT
} PeerPairingMode;

typedef enum
{
    RELAY_SOURCE_NULL,      /* There is no source active that is providing audio to be relayed */
    RELAY_SOURCE_ANALOGUE,  /* Wired analogue is acting as the source for relayed audio data */
    RELAY_SOURCE_USB,       /* USB is acting as the source for relayed audio data */
    RELAY_SOURCE_A2DP       /* Bluetooth AV Source, i.e. an AG, is acting as the source for relayed audio data */
} RelaySource;

typedef enum
{
    RELAY_STATE_IDLE,
    RELAY_STATE_CLOSING,
    RELAY_STATE_OPENING,
    RELAY_STATE_OPEN,
    RELAY_STATE_SUSPENDING,
    RELAY_STATE_STARTING,
    RELAY_STATE_STREAMING
} RelayState;

typedef enum
{
    RELAY_EVENT_DISCONNECTED,
    RELAY_EVENT_CONNECTED,
    RELAY_EVENT_CLOSED,
    RELAY_EVENT_OPEN,
    RELAY_EVENT_OPENING,
    RELAY_EVENT_NOT_OPENED,
    RELAY_EVENT_OPENED,
    RELAY_EVENT_CLOSE,
    RELAY_EVENT_SUSPENDED,
    RELAY_EVENT_START,
    RELAY_EVENT_STARTING,
    RELAY_EVENT_NOT_STARTED,
    RELAY_EVENT_STARTED,
    RELAY_EVENT_SUSPEND
} RelayEvent;

typedef enum
{
    PEER_STATUS_CHANGE_CALL_INACTIVE     = 0x01,
    PEER_STATUS_CHANGE_CALL_ACTIVE       = 0x02,
    PEER_CALL_STATUS_CHANGE              = 0x03,
    PEER_STATUS_CHANGE_RELAY_UNAVAILABLE = 0x04,
    PEER_STATUS_CHANGE_RELAY_AVAILABLE   = 0x08,
    PEER_RELAY_STATUS_CHANGE             = 0x0C,
    PEER_STATUS_CHANGE_RELAY_CLAIMED     = 0x10,
    PEER_STATUS_CHANGE_RELAY_FREED       = 0x20,
    PEER_RELAY_OWNERSHIP_CHANGE          = 0x30
} PeerStatusChange;

typedef enum
{
    PEER_STATUS_IN_CALL    = 0x01,
    PEER_STATUS_DONT_RELAY = 0x02,
    PEER_STATUS_OWNS_RELAY = 0x04,
    PEER_STATUS_POWER_OFF = 0x08
} PeerStatus;

typedef enum
{
    PEER_TWS_ROUTING_STEREO,
    PEER_TWS_ROUTING_LEFT,
    PEER_TWS_ROUTING_RIGHT,
    PEER_TWS_ROUTING_DMIX
} PeerTwsAudioRouting;

typedef enum
{
    PEER_TRIM_VOL_NO_CHANGE,
    PEER_TRIM_VOL_MASTER_UP ,
    PEER_TRIM_VOL_MASTER_DOWN,
    PEER_TRIM_VOL_SLAVE_UP,
    PEER_TRIM_VOL_SLAVE_DOWN
}PeerTrimVolChangeCmd;

#define PEER_UNLOCK_DELAY 2000

#ifdef DEBUG_PEER_SM
#define PEER_UPDATE_REQUIRED_RELAY_STATE(str)  {DEBUG(("----\nPEER: Update[%s]\n",str)); peerUpdateRequiredRelayState();}
#else
#define PEER_UPDATE_REQUIRED_RELAY_STATE(str)  {peerUpdateRequiredRelayState();}
#endif


/*************************************************************************
NAME    
    peerRequestServiceRecord
    
DESCRIPTION
    Issues a request to obtain the attributes of a Peer Device SDP record from
    the specified device

RETURNS
    TRUE if a search requested, FALSE otherwise
    
**************************************************************************/
bool RequestRemotePeerServiceRecord (Task task, const bdaddr *bd_addr);

/*************************************************************************
NAME    
    peerSetRemoteSupportedCodecsFromServiceRecord
    
DESCRIPTION
    Extracts and stores the list of optional codecs supported by the remote peer device
    from the supplied Peer Device SDP record

RETURNS
    None
**************************************************************************/
void peerSetRemoteSupportedCodecsFromServiceRecord (const uint8 *attr_data, uint16 attr_data_size);

/*************************************************************************
NAME    
    peerGetRemoteVersion
    
DESCRIPTION
    Extracts the Peer Device version number from the supplied SDP record.
    
RETURNS
    The Peer Device support version number of a connected Peer, or 0 if not
    present in SDP record
    
**************************************************************************/
uint16 peerGetRemoteVersionFromServiceRecord (const uint8 *attr_data, uint16 attr_data_size);

/*************************************************************************
NAME    
    peerGetRemoteSupportedFeatures
    
DESCRIPTION
    Extracts the Peer Device supported features from the supplied SDP record.
    
RETURNS
    The Peer Device supported features of a connected Peer, or 0 if not
    present in SDP record
    
**************************************************************************/
remote_features peerGetRemoteSupportedFeaturesFromServiceRecord (const uint8 *attr_data, uint16 attr_data_size);

/*************************************************************************
NAME    
    peerDetermineCompatibleRemoteFeatures
    
DESCRIPTION
    Identifies the set of compatible features based on locally supported peer features 
    and the remote features obtained from a Peer during a Peer Device SDP record search 
    request.
    The compatible features are a set of mutually matching features i.e. if the local device
    supports a tws source role then the compatible feature would be the Peer supporting a tws
    sink role.

RETURNS
    The compatible set of features of the connected Peer
    
**************************************************************************/
remote_features peerDetermineCompatibleRemoteFeatures (remote_features supported_features, uint16 version);

/*************************************************************************
NAME    
    peerIsCompatibleDevice
    
DESCRIPTION
    Uses the Peer version number to determine if another Peer device is compatible

RETURNS
    TRUE if device is deemed compatible, FALSE otherwise
    
**************************************************************************/
bool peerIsCompatibleDevice (uint16 version);

/*************************************************************************
NAME    
    peerRegisterServiceRecord
    
DESCRIPTION
    Registers a Peer Device SDP record

RETURNS
    None
    
**************************************************************************/
void RegisterPeerDeviceServiceRecord ( void );

/****************************************************************************
NAME    
    peerCredentialsRequest
    
DESCRIPTION
    Request Peer device credentials (device id and/or peer device SDP record)
    from the specified device
    
RETURNS
    TRUE if credentials were requested, FALSE otherwise
*/
bool peerCredentialsRequest (bdaddr *device_addr);

/*************************************************************************
NAME    
    peerGetPeerSink
    
DESCRIPTION
    Obtains sink to relay channel

RETURNS
    Handle to relay channel, NULL otherwise
    
**************************************************************************/
Sink peerGetPeerSink (void);

/*************************************************************************
NAME    
    peerGetSourceSink
    
DESCRIPTION
    Obtains sink of the current source

RETURNS
    Handle to sink if there is a current streaming source, NULL otherwise
    
**************************************************************************/
Sink peerGetSourceSink (void);

/*************************************************************************
 NAME
 getPeerSource

 DESCRIPTION


 RETURNS


 **************************************************************************/
RelaySource getPeerSource(void);

/*************************************************************************
NAME
    updatePeerSourceBasedOnEvent

DESCRIPTION
    Function to match the source selected with the relevant user event
    and update the Peer state machine.

INPUTS
    User's source selection event.

RETURNS
	None

*************************************************************************/
void updatePeerSourceBasedOnEvent(MessageId nextSourceEvent);

/*************************************************************************
NAME    
    updateAudioGating
    
DESCRIPTION
    Updates gating used for "multimedia" (non-sco) types of audio based on
    call status of any connected Peer and audio source being relayed

RETURNS
    None
    
**************************************************************************/
void updateAudioGating (void);

/*************************************************************************
NAME    
    peerAdvanceRelayState
    
DESCRIPTION
    Updates the Peer state machine current state based on the supplied event.
    This function should be called when an event occurs that would cause a change to
    the actual Peer state machine status.
    
    This forms one of the two functions that should be used to drive the Peer state machine.

RETURNS
    None
    
**************************************************************************/
void peerAdvanceRelayState (RelayEvent relay_event);

/*************************************************************************
NAME    
    peerUpdateRequiredRelayState
    
DESCRIPTION
    Used to update the Peer state machine required state.  This should be called when any event occurs
    that would cause a change to the required streaming state or source.
    
    This forms one of the two functions that should be used to drive the Peer state machine.

RETURNS
    None
    
**************************************************************************/
void peerUpdateRequiredRelayState (void);

/*************************************************************************
NAME    
    peerIsRelayAvailable
    
DESCRIPTION
    Determines if relay channel is available for use

RETURNS
    TRUE if relay available, FALSE otherwise
    
**************************************************************************/
bool peerIsRelayAvailable (void);

/*************************************************************************
NAME    
    peerCheckSource
    
DESCRIPTION
    Determines if the provided source can be relayed

RETURNS
    TRUE if relay available, FALSE otherwise
    
**************************************************************************/

bool peerCheckSource(RelaySource new_source);

/*************************************************************************
NAME    
    peerUpdateLocalStatusChange
    
DESCRIPTION
    Updates the local relay availability and issues a status change to a connected Peer

RETURNS
    None
    
**************************************************************************/
void peerUpdateLocalStatusChange (PeerStatusChange peer_status_change);

/*************************************************************************
NAME    
    peerHandleStatusChangeCmd
    
DESCRIPTION
    Handles a relay availability status change from a Peer

RETURNS
    None
    
**************************************************************************/
void peerHandleStatusChangeCmd (PeerStatusChange peer_status_change);

/*************************************************************************
NAME    
    peerHandleAudioRoutingCmd
    
DESCRIPTION
    Hamdles a audio routing notification from a Peer

RETURNS
    None
    
**************************************************************************/
void peerHandleAudioRoutingCmd (PeerTwsAudioRouting master_routing_mode, PeerTwsAudioRouting slave_routing_mode);

/*************************************************************************
NAME    
    peerHandleVolumeCmd
    
DESCRIPTION
    Handle a volume change notification from a Peer

RETURNS
    None
    
**************************************************************************/
void peerHandleVolumeCmd (uint8 volume);

/*************************************************************************
NAME    
    peerUpdateMasterTrimVolume
    
DESCRIPTION
    Handle a trim volume change notification from a Peer : TWS slave

RETURNS
    None
    
**************************************************************************/
void peerUpdateTrimVolume(PeerTrimVolChangeCmd cmd);

/*************************************************************************
NAME    
    peerUpdateMasterTrimVolume
    
DESCRIPTION
   Sends the appropriate trim volume change command to the master peer device.

RETURNS
    None
    
**************************************************************************/
void peerSendDeviceTrimVolume(volume_direction dir, tws_device_type tws_device);

/*************************************************************************
NAME    
    peerSendAudioEnhancements
    
DESCRIPTION
    Sends audio enhancement setting to currently connected Peer

RETURNS
    None
    
**************************************************************************/
void peerSendAudioEnhancements(void);

/*************************************************************************
NAME    
    peerSendEqSettings
    
DESCRIPTION
    Sends DSP EQ setting to currently connected Peer

RETURNS
    None
    
**************************************************************************/
void peerSendUserEqSettings(void);

/*************************************************************************
NAME    
    peerRequestUserEqSetings
    
DESCRIPTION
    Request current DSP EQ setting from the connected Peer (Master)

RETURNS
    None
    
**************************************************************************/
void peerRequestUserEqSetings(void);

/*************************************************************************
NAME    
    peerConnectPeer
    
DESCRIPTION
    Attempts to connect a TWS Peer, if not already in a Peer session

RETURNS
    TRUE if a connection is attempted, FALSE otherwise
    
**************************************************************************/
bool peerConnectPeer (void);

/*************************************************************************
NAME    
    peerObtainPairingMode
    
DESCRIPTION
    Obtains the pairing mode used for the currently connected Peer device

RETURNS
    None
    
**************************************************************************/
PeerPairingMode peerObtainPairingMode (uint16 peer_id);

/*************************************************************************
NAME    
    peerUpdatePairing
    
DESCRIPTION
    Ensures permanent pairing data is updated

RETURNS
    None
    
**************************************************************************/
void peerUpdatePairing (uint16 peer_id, const struct sink_attributes_str *peer_attributes);

/*************************************************************************
NAME    
    peerInitPeerStatus
    
DESCRIPTION
    Inits peer data structure and sets the initial required state

RETURNS
    None
    
**************************************************************************/
void peerInitPeerStatus (void);

/*************************************************************************
NAME    
    peerHandleRemoteAGConnected
    
DESCRIPTION
    Checks if the sink device is already connected to an AG with the same bd address as the one connected to the peer,
    if yes, then disconnect the local AG connected if the bd addr of the sink device is lower than that of peer.

RETURNS
    None
    
**************************************************************************/
void peerHandleRemoteAgConnected (void);

/*************************************************************************
NAME    
    peerCompareBdAddr
    
DESCRIPTION
    Compares the first and the second bdaddr.
RETURNS
    TRUE if the first bd addr is greater than second, otherwise returns FALSE.
    
**************************************************************************/
bool peerCompareBdAddr(const bdaddr* first , const bdaddr* second);


/*************************************************************************
NAME    
    peerIsTwsMaster
    
DESCRIPTION
    Is the device a TWS Master
RETURNS
    TRUE if the device is a streaming TWS Master, otherwise returns FALSE.
    
**************************************************************************/
#ifdef ENABLE_PEER
bool peerIsTwsMaster(void);
#else
#define peerIsTwsMaster() (FALSE)
#endif

/*************************************************************************
NAME    
    peerClaimRelay
    
DESCRIPTION
    Helper function to update local relay availability(claim or free) and issue status change to Peer

RETURNS
    None
    
**************************************************************************/

void peerClaimRelay(bool claim);

/*************************************************************************
NAME
    peerUpdateBatteryLevel

DESCRIPTION
    This function is used by sink_avrcp to update the peer battery level when
    a AVRCP_PEER_CMD_UPDATE_BATTERY_LEVEL message is received from a connected
    peer.

RETURNS
    TRUE on success

**************************************************************************/
#ifdef ENABLE_PEER_BATTERY_LEVEL
bool peerUpdateBatteryLevel(uint16 battery_level);
#else
#define peerUpdateBatteryLevel(x) (FALSE)
#endif


/*************************************************************************
NAME
    peerGetBatteryLevel

DESCRIPTION
    This function can be used to retrive the cached battery level of connected
    peer device.

RETURNS
    Battery level in percentage
    BATTERY_LEVEL_INVALID on error

**************************************************************************/
#ifdef ENABLE_PEER_BATTERY_LEVEL
uint16 peerGetBatteryLevel(void);
#else
#define peerGetBatteryLevel() (BATTERY_LEVEL_INVALID)
#endif


/*************************************************************************
NAME
    updateCurrentState

DESCRIPTION
    Peer state machine helper function to help manage changes to the current state.

RETURNS
    None

**************************************************************************/
void updateCurrentState (RelayState relay_state);

/*************************************************************************
NAME    
    peerGetLocalSupportedCodecs
    
DESCRIPTION
    Returns the list of codec supported by the local peer device

RETURNS
    TRUE if the list is present othewise FALSE
    
**************************************************************************/
bool peerGetLocalSupportedCodecs(uint16 *codecs);

/*************************************************************************
NAME    
    peerGetRemoteSupportedCodecs
    
DESCRIPTION
    Returns the list of codec supported by the remote peer device

RETURNS
    TRUE if the list is present othewise FALSE

    
**************************************************************************/
bool peerGetRemoteSupportedCodecs(uint16 *codecs);

/*************************************************************************
NAME    
    peerSetLocalPeerSupportedCodecs
    
DESCRIPTION
    Stores the list of codec supported by the local peer device

RETURNS
    None
    
**************************************************************************/
void peerSetLocalSupportedCodecs(uint16 codecs);

/*************************************************************************
NAME    
    peerSetRemoteSupportedCodecs
    
DESCRIPTION
    Stores the list of codec supported by the remote peer device

RETURNS
    None
    
**************************************************************************/
void peerSetRemoteSupportedCodecs(uint16 codecs);

/****************************************************************************
NAME    
    peerLinkReservedCanDeviceConnect
    
DESCRIPTION
    Determine whether the given device can be connected based on whether we have reserved a link 
    for a peer device to connect.

RETURNS
    TRUE or FALSE
*/ 
bool peerLinkReservedCanDeviceConnect(const bdaddr *bd_addr);

/****************************************************************************
NAME    
    peerReserveLink
    
DESCRIPTION
    Turns on or off the feature to reserve a link for the peer device to connect.

RETURNS
    void
*/
void peerReserveLink(bool enable);


/*************************************************************************
NAME    
    peerUpdateMuteState
    
DESCRIPTION
    Responsible for updating the mute state according to the state
    of the peer state machine. Should mute when device is a TWS master
    during relay setup or tear down, or when devices are in idle state
    with no audio source.

RETURNS
    None
    
**************************************************************************/
#ifdef ENABLE_PEER
void peerUpdateMuteState(void);
#else
#define peerUpdateMuteState() ((void)0)
#endif

/*************************************************************************
NAME    
    peerUpdateLocalStatusOnCallEnd
    
DESCRIPTION
    Responsible for updating the relay state when a call ends. The function
    call handles the scenarios when the device is in Single Device Mode (SDM)
    or Non Single Device Mode.

RETURNS
    None
    
**************************************************************************/
void peerUpdateLocalStatusOnCallEnd(void);

/****************************************************************************
NAME	
	getPeerDSPBufferingRequired
    
DESCRIPTION
    function to deteremine whether TWS backwards compatibility mode within the 
    DSP is required, TWS version 3.1.0 uses an internal delay of 350ms to allow
    the use of larger buffers, this is incompatible with earlier versions of TWS
    which only use a 250ms internal delay, therefore to maintain backwards
    compatibility with older versions of TWS it is necessary to send a configuration
    message to the DSP to select the internal delay, STD or backwards compatibility
    required.
    
RETURNS
	peer_buffer_level enum 250 or 350ms 
*/
peer_buffer_level getPeerDSPBufferingRequired(uint16 remote_peer_version);

/*************************************************************************
NAME    
    peerAvrcpUpdateActiveConnection
    
DESCRIPTION
    Updates the active AVRCP connection based on what is currently connected.

RETURNS
	TRUE if the supplied AVRCP connection index is that of Peer device, else FALSE

**************************************************************************/
#ifdef ENABLE_PEER
bool peerAvrcpUpdateActiveConnection(uint8 active_avrcp);
#else
#define peerAvrcpUpdateActiveConnection(x) (FALSE)
#endif

/*************************************************************************
NAME    
    peerIsA2dpAudioConnected
    
DESCRIPTION
    Determines whether or not there is a peer with A2DP audio connected

RETURNS
	TRUE if have found a peer device with A2DP audio connected, else FALSE

**************************************************************************/
#ifdef ENABLE_PEER
bool peerIsA2dpAudioConnected(uint8 Index);
#else
#define peerIsA2dpAudioConnected(x) (TRUE)
#endif


/*************************************************************************
NAME
    peerUpdateRelaySource

DESCRIPTION
    Function to match the source selected with the relevant Peer source
    attribute which is essential to be updated along with the Peer
    state machine.

INPUTS
    Audio Source selection.

RETURNS
	None

*/
#ifdef ENABLE_PEER
void peerUpdateRelaySource(uint16 nextSource);
#else
#define peerUpdateRelaySource(x)
#endif /* ENABLE_PEER */

/*************************************************************************
NAME
    peerIsDeviceSlaveAndStreaming

DESCRIPTION
	Function call which confirms if the device is in Slave role and a Peer
	is connected and currently streaming.

INPUTS
    None

RETURNS
	TRUE if device is not a Master and peer is connected and streaming,
	else FALSE.

*/
#ifdef ENABLE_PEER
bool peerIsDeviceSlaveAndStreaming(void);
#else
#define peerIsDeviceSlaveAndStreaming() (FALSE)
#endif

/****************************************************************************
NAME
    peerRelaySourceStream

DESCRIPTION
    Checks for a Peer relay (source) stream and asks DSP to relay audio from any active AV Source

RETURNS
    true if audio relayed, false otherwise
*/
#ifdef ENABLE_PEER
bool peerRelaySourceStream (void);
#else
#define peerRelaySourceStream(x) (void)(FALSE)
#endif

#ifdef ENABLE_PEER
bool peerIsRemotePeerInCall(void);
#else
#define peerIsRemotePeerInCall() (FALSE)
#endif

#if defined ENABLE_PEER && defined PEER_TWS
void peerPopulatePluginConnectData(AudioPluginFeatures * features, uint16 sample_rate);
#else
#define peerPopulatePluginConnectData(x,y)    ((void)0)
#endif

#ifdef ENABLE_PEER
/*************************************************************************
NAME    
    peerPurgeTemporaryPairing
    
DESCRIPTION
    If in Temporary Pairing mode, remove any peer devices from the PDL

RETURNS
    None
    
**************************************************************************/
void peerPurgeTemporaryPairing(void);
#endif


#endif /* _SINK_PEER_H_ */
