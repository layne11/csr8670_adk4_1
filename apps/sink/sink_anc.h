/*
Copyright (c) 2015 - 2016 Qualcomm Technologies International, Ltd.
Part of ADK 4.1
*/
/** 
\file 
\ingroup sink_app
\brief Support for Ambient Noise Cancellation (ANC). 

The ANC feature is included in
    an add-on installer and is only supported on CSR8675.
*/

#ifndef _SINK_ANC_H_
#define _SINK_ANC_H_


#include <csrtypes.h>


#define ANC_SINK ((Sink)0xFFFD)



#define SINK_ANC_DATA(anc)



/****************************************************************************
NAME
    sinkAncInit

DESCRIPTION
    Initialisation of ANC feature.
*/    
#define sinkAncInit() ((void)(0))


/****************************************************************************
NAME
    sinkAncHandlePowerOn

DESCRIPTION
    ANC specific handling due to the device Powering On.
*/    
#define sinkAncHandlePowerOn() ((void)(0))


/****************************************************************************
NAME
    sinkAncHandlePowerOff

DESCRIPTION
    ANC specific handling due to the device Powering Off.
*/    
#define sinkAncHandlePowerOff() ((void)(0))


/****************************************************************************
NAME
    sinkAncEnable

DESCRIPTION
    Enable ANC functionality.
*/    
#define sinkAncEnable() ((void)(0))


/****************************************************************************
NAME
    sinkAncDisable

DESCRIPTION
    Disable ANC functionality.
*/    
#define sinkAncDisable() ((void)(0))


/****************************************************************************
NAME
    sinkAncToggleEnable

DESCRIPTION
    Toggle between ANC Enabled and Disabled.
*/    
#define sinkAncToggleEnable() ((void)(0))


/****************************************************************************
NAME
    sinkAncSetLeakthroughMode

DESCRIPTION
    Set the operating mode of ANC to Leakthrough Mode.
*/    
#define sinkAncSetLeakthroughMode() ((void)(0))


/****************************************************************************
NAME
    sinkAncSetActiveMode

DESCRIPTION
    Set the operating mode of ANC to Active Mode.
*/    
#define sinkAncSetActiveMode() ((void)(0))


/****************************************************************************
NAME
    sinkAncSetNextMode

DESCRIPTION
    Cycle through the operating modes of ANC.
*/    
#define sinkAncSetNextMode() ((void)(0))


/****************************************************************************
NAME
    sinkAncVolumeDown

DESCRIPTION
    Decrease the ANC Volume.
*/    
#define sinkAncVolumeDown() ((void)(0))


/****************************************************************************
NAME
    sinkAncVolumeUp

DESCRIPTION
    Increase the ANC Volume.
*/    
#define sinkAncVolumeUp() ((void)(0))

/****************************************************************************
NAME
    sinkAncCycleAdcDigitalGain

DESCRIPTION
    Cycle through the ADC digital gain for fine tuning.
*/    
#define sinkAncCycleAdcDigitalGain() ((void)(0))

/****************************************************************************
NAME
    sinkAncAudioRoute

DESCRIPTION
    Routes the ANC audio when no other audio source is active.
*/
#define sinkAncAudioRoute() (FALSE)

/****************************************************************************
NAME
    sinkAncIsEnabled

DESCRIPTION
    Returns TRUE if ANC is enabled. FALSE otherwise.
*/
#define sinkAncIsEnabled() (FALSE)

#endif /* _SINK_ANC_H_ */
