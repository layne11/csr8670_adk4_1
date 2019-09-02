/*
Copyright (c) 2004 - 2016 Qualcomm Technologies International, Ltd.
*/

/*!
@file
@ingroup sink_app
*/
#ifndef SINK_LED_MANAGER_H
#define SINK_LED_MANAGER_H


#include "sink_private.h"
#include "sink_states.h"
#include "sink_events.h"

#define LedManagerQueuedEvent() (theSink.theLEDTask->Queue[0])

#define MAKE_LEDS_MESSAGE(TYPE) TYPE##_T *message = PanicUnlessNew(TYPE##_T);

typedef struct
{
    sinkState  state;   /* LED state to indicate */
} LED_INDICATE_STATE_T;


    void LedManagerMemoryInit(void);
    void LEDManagerInit ( void ) ;

	void LEDManagerIndicateEvent ( MessageId pEvent ) ;

    void LedManagerIndicateQueuedEvent(void);
    
	void LEDManagerIndicateState ( sinkState pState )  ;


	void LedManagerDisableLEDS ( void ) ;
	void LedManagerEnableLEDS  ( void ) ;

	void LedManagerToggleLEDS  ( void )  ;

	void LedManagerResetLEDIndications ( void ) ;

	void LEDManagerResetStateIndNumRepeatsComplete  ( void ) ;


	void LEDManagerCheckTimeoutState( void );

	void LedManagerForceDisable( bool disable );
        
	#ifdef DEBUG_LM
		void LMPrintPattern ( LEDPattern_t * pLED ) ;
	#endif
		
#endif

