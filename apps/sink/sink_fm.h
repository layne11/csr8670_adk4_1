/****************************************************************************
Copyright (c) 2012 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_fm.h
    
DESCRIPTION
    This file provides interfaces between sink fm app and fm library
    
    
*/

#ifndef SINK_FM_H
#define SINK_FM_H

#ifdef ENABLE_FM
#include <sink.h>
#include <fm_plugin_if.h>

#define FM_SINK                 ((Sink)0xFF00)
#define FM_SAMPLE_RATE          (48000)
#define FM_INVALID_FREQ         (0x0000)
#define FM_DISPLAY_STR_LEN      (12)
#define FM_VOLUME_SCALE_FACTOR  (4) /* FM receiver internal volume is scaled 0 to 64, default volume * 4 gives
                                       a volume level comparable to other audio sources */

typedef enum
{
    FM_SHOW_STATION,
    FM_ADD_FAV_STATION, /*Add special char for favourite station*/
    FM_DEL_FAV_STATION
} fm_display_type;

typedef enum
{
    FM_ENABLE_RX,    
    FM_ENABLE_TX, 
    FM_ENABLE_RX_TX
} fm_mode;

typedef struct
{
    bool fmRxOn;
    uint16 fmRxTunedFreq;
    Sink sink;
    fm_stored_freq fmStoredFreq;
    fm_rx_data_t *fm_plugin_data;
} fm_data;

#define SINK_FM_DATA(sink_fm_data)     fm_data sink_fm_data;

#else
#define SINK_FM_DATA(sink_fm_data)
#endif

#ifdef ENABLE_FM
void sinkFmRxPostConnectConfiguration(void);
#else
#define sinkFmRxPostConnectConfiguration()  ((void)0)
#endif

#ifdef ENABLE_FM
bool sinkFmRxPopulateConnectParameters(audio_connect_parameters *connect_parameters);
#else
#define sinkFmRxPopulateConnectParameters(x) (FALSE)
#endif

/****************************************************************************
NAME    
    sinkFmRxAudioPostDisconnectConfiguration

DESCRIPTION
    Handles disconnection of FM RX
*/
#ifdef ENABLE_FM
void sinkFmRxAudioPostDisconnectConfiguration(void);
#else
#define sinkFmRxAudioPostDisconnectConfiguration() ((void)(0))
#endif

/****************************************************************************
NAME 
    sinkFmAudioSinkMatch
    
DESCRIPTION
    Compare sink to FM audio sink.
    
RETURNS
    TRUE if Sink matches, FALSE otherwise
*/ 
#ifdef ENABLE_FM
bool sinkFmAudioSinkMatch(Sink sink);
#else
#define sinkFmAudioSinkMatch(x) (FALSE)
#endif

/****************************************************************************
NAME
    sinkFmIsFmRxOn

DESCRIPTION
    Check to see if the FM receiver is on.
*/
#ifdef ENABLE_FM
bool sinkFmIsFmRxOn(void);
#else
#define sinkFmIsFmRxOn() (FALSE)
#endif

/****************************************************************************
NAME
    sinkFmSetFmRxOn

DESCRIPTION
    Sets the FmRxOn flag.
*/
#ifdef ENABLE_FM
void sinkFmSetFmRxOn(bool isOn);
#else
#define sinkFmSetFmRxOn(x) ((void)(0))
#endif

/****************************************************************************
NAME
    sinkFmProcessEventUsrFmRx

DESCRIPTION
    processes FM related user events.

RETURN
    TRUE if handled, FALSE otherwise
*/
#ifdef ENABLE_FM
bool sinkFmProcessEventUsrFmRx(const MessageId EventUsrFmRx);
#else
#define sinkFmProcessEventUsrFmRx(x) (FALSE)
#endif

/****************************************************************************
NAME
    sinkFmHandleFmPluginMessage

DESCRIPTION
    processes messages received from the FM plugin.
*/
#ifdef ENABLE_FM
void sinkFmHandleFmPluginMessage(const MessageId id, const Message message);
#else
#define sinkFmHandleFmPluginMessage(id,message) ((void)(0))
#endif

/****************************************************************************
NAME
    sinkFmSetFmDataFromSessionData

DESCRIPTION
    Sets FM related data using stored session data values.
*/
#ifdef ENABLE_FM
void sinkFmSetFmDataFromSessionData(const session_data_type * const session_data);
#else
#define sinkFmSetFmDataFromSessionData(x) ((void)(0))
#endif

/****************************************************************************
NAME
    sinkFmSetSessionDataFromFmData

DESCRIPTION
    Sets FM related session data using using current data values.
*/
#ifdef ENABLE_FM
void sinkFmSetSessionDataFromFmData(session_data_type * const session_data);
#else
#define sinkFmSetSessionDataFromFmData(x) ((void)(0))
#endif

/****************************************************************************
NAME
    sinkFmReadConfig

DESCRIPTION
    Allocate memory and retrieve FM config from PS
*/
#ifdef ENABLE_FM
void sinkFmReadConfig(void);
#else
#define sinkFmReadConfig() ((void)(0))
#endif

/****************************************************************************
NAME
    sinkFmGetFmSink

DESCRIPTION
    Gets the FM audio Sink.
*/
#ifdef ENABLE_FM
#define sinkFmGetFmSink()     (FM_SINK)
#else
#define sinkFmGetFmSink()     ((Sink)NULL)
#endif

#endif /*SINK_FM_H*/
