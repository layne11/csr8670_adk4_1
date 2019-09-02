/*
Copyright (c) 2004 - 2016 Qualcomm Technologies International, Ltd.
*/

/*!
@file
@ingroup sink_app
*/

#ifndef _SINK_WIRED_H_
#define _SINK_WIRED_H_

#include <stdlib.h>

/****************************************************************************
NAME 
    wiredAudioInit
    
DESCRIPTION
    Set up wired audio PIOs and configuration
    
RETURNS
    void
*/ 
#ifdef ENABLE_WIRED
void wiredAudioInit(void);
#else
#define wiredAudioInit() ((void)(0))
#endif
        


#ifdef ENABLE_WIRED
bool analoguePopulateConnectParameters(audio_connect_parameters *connect_parameters);
#else
#define analoguePopulateConnectParameters(x) (FALSE)
#endif

#ifdef ENABLE_WIRED
bool spdifPopulateConnectParameters(audio_connect_parameters *connect_parameters);
#else
#define spdifPopulateConnectParameters(x) (FALSE)
#endif
        
#ifdef ENABLE_WIRED
bool i2sPopulateConnectParameters(audio_connect_parameters *connect_parameters);
#else
#define i2sPopulateConnectParameters(x) (FALSE)
#endif

/****************************************************************************
NAME 
    analogAudioSinkMatch
    
DESCRIPTION
    Compare sink to analog audio sink.
    
RETURNS
    TRUE if Sink matches, FALSE otherwise
*/ 
#ifdef ENABLE_WIRED
bool analogAudioSinkMatch(Sink sink);
#else
#define analogAudioSinkMatch(x) (FALSE)
#endif

/****************************************************************************
NAME 
    spdifAudioSinkMatch
    
DESCRIPTION
    Compare sink to spdif audio sink.
    
RETURNS
    TRUE if Sink matches, FALSE otherwise
*/ 
#ifdef ENABLE_WIRED
bool spdifAudioSinkMatch(Sink sink);
#else
#define spdifAudioSinkMatch(x) (FALSE)
#endif

/****************************************************************************
NAME 
    i2sAudioSinkMatch
    
DESCRIPTION
    Compare sink to i2s audio sink.
    
RETURNS
    TRUE if Sink matches, FALSE otherwise
*/ 
#ifdef ENABLE_WIRED
bool i2sAudioSinkMatch(Sink sink);
#else
#define i2sAudioSinkMatch(x) (FALSE)
#endif


/****************************************************************************
NAME 
    wiredAudioCheckDeviceTrimVol
    
DESCRIPTION
    Adjust wired audio volume if currently being routed
    
RETURNS
    TRUE if wired audio being routed, FALSE otherwise
*/ 
#ifdef ENABLE_WIRED
bool wiredAudioCheckDeviceTrimVol(volume_direction dir, tws_device_type tws_device);
#else
#define wiredAudioCheckDeviceTrimVol(dir, tws_device) (FALSE)
#endif


/****************************************************************************
NAME 
    analogAudioConnected
    
DESCRIPTION
    Determine if wired audio input is connected
    
RETURNS
    TRUE if connected, otherwise FALSE
*/ 
#ifdef ENABLE_WIRED
bool analogAudioConnected(void);
#else
#define analogAudioConnected()   (FALSE)
#endif

/****************************************************************************
NAME 
    analogGetAudioSink
    
DESCRIPTION
    Check analog state and return sink if available
    
RETURNS
    Sink if available, otherwise NULL
*/ 
#ifdef ENABLE_WIRED
Sink analogGetAudioSink(void);
#else
#define analogGetAudioSink() ((Sink)(NULL))
#endif

/****************************************************************************
NAME 
    analogGetAudioRate
    
DESCRIPTION
    Obtains the current defined sample rate for wired audio
    
RETURNS
    None
*/ 
#ifdef ENABLE_WIRED
void analogGetAudioRate (uint16 *rate);
#else
#define analogGetAudioRate(x) (*(x) = 0)
#endif

/****************************************************************************
NAME 
    spdifAudioConnected
    
DESCRIPTION
    Determine if spdif audio input is connected
    
RETURNS
    TRUE if connected, otherwise FALSE
*/ 
#ifdef ENABLE_WIRED
bool spdifAudioConnected(void);
#else
#define spdifAudioConnected()   (FALSE)
#endif

/****************************************************************************
NAME
    i2sAudioConnected
    
DESCRIPTION
    Determine if i2s audio input is connected
    
RETURNS
    TRUE if connected, otherwise FALSE
*/
#ifdef ENABLE_WIRED
bool i2sAudioConnected(void);
#else
#define i2sAudioConnected()   (FALSE)
#endif

/****************************************************************************
NAME 
    analogAudioCanPowerOff
    
DESCRIPTION
    Determine if analog audio blocks power off
    
RETURNS
    TRUE if device can power off, otherwise FALSE
*/ 
#define analogAudioCanPowerOff() ((!analogAudioConnected()) || powerManagerIsVbatCritical())

/****************************************************************************
NAME 
    spdifAudioCanPowerOff
    
DESCRIPTION
    Determine if spdif audio blocks power off
    
RETURNS
    TRUE if device can power off, otherwise FALSE
*/ 
#define spdifAudioCanPowerOff() ((!spdifAudioConnected()) || powerManagerIsVbatCritical())

#ifdef ENABLE_WIRED
bool wiredAnalogAudioIsRouted(void);
#else
#define wiredAnalogAudioIsRouted() (FALSE)
#endif

#ifdef ENABLE_WIRED
bool wiredSpdifAudioIsRouted(void);
#else
#define wiredSpdifAudioIsRouted() (FALSE)
#endif

#ifdef ENABLE_WIRED
bool wiredI2SAudioIsRouted(void);
#else
#define wiredI2SAudioIsRouted() (FALSE)
#endif

#endif /* _SINK_WIRED_H_ */
