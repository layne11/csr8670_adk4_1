/*
Copyright (c) 2004 - 2016 Qualcomm Technologies International, Ltd.
*/

/*!
@file
@ingroup sink_app
@brief
    state machine helper functions used for state changes etc - provide single 
    state change points etc for the sink device application
*/

#include "sink_statemanager.h"
#include "sink_led_manager.h"
#include "sink_buttonmanager.h"
#ifndef HYDRACORE_TODO
#include "sink_dut.h"
#endif /* HYDRACORE_TODO*/
#include "sink_pio.h"
#include "sink_audio.h"
#include "sink_scan.h"
#include "sink_slc.h"
#include "sink_inquiry.h"
#include "sink_devicemanager.h"
#include "sink_powermanager.h"
#include "sink_speech_recognition.h" 
#include "psu.h"
#include "sink_audio_routing.h"
#include "sink_auth.h"

#ifdef ENABLE_FM
#include "sink_fm.h"
#endif

#include "sink_display.h"

#ifdef ENABLE_AVRCP
#include "sink_avrcp.h"    
#endif

#ifdef ENABLE_SUBWOOFER
#include "sink_swat.h"
#include <bdaddr.h>
#endif

#ifdef TEST_HARNESS
#include "test_sink.h"
#endif

#include <stdlib.h>
#include <boot.h>

#ifdef DEBUG_STATES
#define SM_DEBUG(x) DEBUG(x)

const char * const gHSStateStrings [ SINK_NUM_STATES ] = {
                               "Limbo",
                               "Connectable",
                               "ConnDisc",
                               "Connected",
                               "Out",
                               "Inc",
                               "ActiveCallSCO",
                               "TESTMODE",
                               "TWCWaiting",
                               "TWCOnHold",
                               "TWMulticall",
                               "IncCallOnHold",
                               "ActiveNoSCO",
                               "deviceA2DPStreaming",
                               "deviceLowBattery"} ;
                               
#else
#define SM_DEBUG(x) 
#endif


/****************************************************************************
VARIABLES
*/

    /*the device state variable - accessed only from below fns*/
static sinkState gTheSinkState ;


/****************************************************************************
FUNCTIONS
*/

static void stateManagerSetState ( sinkState pNewState ) ;
static void stateManagerResetPIOs ( void ) ;

#define isPowerDownPioConfigured()  PioIsPioConfigured(PIO_POWER_ON)
#define isMutePioConfigured()       PioIsPioConfigured(PIO_AMP_MUTE)

/****************************************************************************
NAME	
	stateManagerSetState

DESCRIPTION
	helper function to Set the current device state
    provides a single state change point and passes the information
    on to the managers requiring state based responses
    
RETURNS
	
    
*/
static void stateManagerSetState ( sinkState pNewState )
{
    SM_DEBUG(("SM:[%s]->[%s][%d]\n",gHSStateStrings[stateManagerGetState()] , gHSStateStrings[pNewState] , pNewState ));

    if ( pNewState < SINK_NUM_STATES )
    {

        if (pNewState != gTheSinkState )
        {
            /* send message to initiate LED state indication to reduce stack usage */
            MAKE_LEDS_MESSAGE(LED_INDICATE_STATE);
            /* LED state to indicate */
            message->state = pNewState;        
            /* send message to reduce stack usage */          
            MessageSend(&theSink.task, EventSysLEDIndicateState, message); 
 
#ifdef TEST_HARNESS
            vm2host_send_state(pNewState);
#endif
            
            /* this should be called before the state is updated below */
            displayUpdateAppState(pNewState);
        }
        else
        {
            /*we are already indicating this state no need to re*/
        }
   
        gTheSinkState = pNewState ;
		UartSendStr("+HFSTA:");
		UartSendDigit(gTheSinkState);
		UartSendStr("\r\n");

    }
    else
    {
        SM_DEBUG(("SM: ? [%s] [%x]\n",gHSStateStrings[ pNewState] , pNewState)) ;
    }

}


/****************************************************************************
NAME	
	stateManagerGetState

DESCRIPTION
	helper function to get the current device state

RETURNS
	the Devie State information
    
*/
sinkState stateManagerGetState ( void )
{
    return gTheSinkState ;
}


/****************************************************************************
NAME	
	stateManagerEnterConnectableState

DESCRIPTION
	single point of entry for the connectable state - enters discoverable state 
    if configured to do so

RETURNS
	void
    
*/
void stateManagerEnterConnectableState ( bool req_disc )
{
    sinkState lOldState = stateManagerGetState() ;
    
    /* Don't go connectable if we're still pairing */
    if(!req_disc && theSink.inquiry.action == rssi_pairing)
        return;

    if ( stateManagerIsConnected() && req_disc )
    {       /*then we have an SLC active*/
       sinkDisconnectAllSlc();
    }
        /*disable the ring PIOs if enabled*/
    stateManagerResetPIOs();
        /* Make the device connectable */
    sinkEnableConnectable();
    stateManagerSetState ( deviceConnectable ) ;

        /* if remain discoverable at all times feature is enabled then make the device 
           discoverable in the first place */
    if (theSink.features.RemainDiscoverableAtAllTimes)
    {
        /* Make the device discoverable */  
        sinkEnableDiscoverable();    
    }
    
        /*determine if we have got here after a DiscoverableTimeoutEvent*/
    if ( lOldState == deviceConnDiscoverable )
    {
        /*disable the discoverable mode*/
        if (!theSink.features.RemainDiscoverableAtAllTimes)
        {
            sinkDisableDiscoverable();
        }
        MessageCancelAll ( &theSink.task , EventSysPairingFail ) ;        
    }
    else
    {
        uint16 lNumDevices = ConnectionTrustedDeviceListSize();
        
#ifdef ENABLE_SUBWOOFER
        /* deteremine if there is a subwoofer present in the PDL, this will need to be 
           discounted from the number of paired AG's */
        if (!BdaddrIsZero(&theSink.rundata->subwoofer.bd_addr))
        {
            /* remove subwoofer pairing from number of devices */
            if(lNumDevices)
                lNumDevices --;
        }
#endif        
        
        /*if we want to auto enter pairing mode*/
        if ( theSink.features.pair_mode_en )
        {
            theSink.inquiry.session = inquiry_session_normal;
            stateManagerEnterConnDiscoverableState( req_disc );
        }  
        SM_DEBUG(("SM: Disco %X RSSI %X\n", theSink.features.DiscoIfPDLLessThan, theSink.features.PairIfPDLLessThan));

        
        /* check whether the RSSI pairing if PDL < feature is set and there are less paired devices than the
           level configured, if so start rssi pairing */
        if((theSink.features.PairIfPDLLessThan)&&( lNumDevices < theSink.features.PairIfPDLLessThan ))
        {
            SM_DEBUG(("SM: NumD [%d] <= PairD [%d]\n" , lNumDevices , theSink.features.PairIfPDLLessThan))

            /* send event to enter pairing mode, that event can be used to play a tone if required */
            MessageSend(&theSink.task, EventSysEnterPairingEmptyPDL, 0);
            MessageSend(&theSink.task, EventUsrRssiPair, 0);
        }           
        /* otherwise if any of the other action on power on features are enabled... */
        else if (theSink.features.DiscoIfPDLLessThan)
        {
    		SM_DEBUG(("SM: Num Devs %d\n",lNumDevices));
            /* Check if we want to go discoverable */
    		if ( lNumDevices < theSink.features.DiscoIfPDLLessThan )
    		{
    			SM_DEBUG(("SM: NumD [%d] <= DiscoD [%d]\n" , lNumDevices , theSink.features.DiscoIfPDLLessThan))
                /* send event to enter pairing mode, that event can be used to play a tone if required */
                MessageSend(&theSink.task, EventSysEnterPairingEmptyPDL, 0);
    		}
#ifdef ENABLE_SUBWOOFER
            /* If there is only one paired device and it is the subwoofer, start pairing mode */
            else if ( (ConnectionTrustedDeviceListSize() == 1) && (!BdaddrIsZero(&theSink.rundata->subwoofer.bd_addr)) )
            {
                /* Check the subwoofer exists in the paired device list */
                theSink.rundata->subwoofer.check_pairing = 1;
                ConnectionSmGetAuthDevice(&theSink.task, &theSink.rundata->subwoofer.bd_addr);
            }
#endif
        }
	}
}

/****************************************************************************
NAME	
	stateManagerEnterConnDiscoverableState

DESCRIPTION
	single point of entry for the connectable / discoverable state 
    uses timeout if configured
RETURNS
	void
    
*/
void stateManagerEnterConnDiscoverableState ( bool req_disc )
{
    /* cancel any pending connection attempts when entering pairing mode */
    MessageCancelAll(&theSink.task, EventSysContinueSlcConnectRequest);

    if(theSink.features.DoNotDiscoDuringLinkLoss && HfpLinkLoss())
    {
        /*if we are in link loss and do not want to go discoverable during link loss then ignore*/                    
    }
    else
    {    
        /* if in connected state disconnect any still attached devices */
        if ( stateManagerIsConnected() && req_disc )
        {
            /* do we have an SLC active? */
            sinkDisconnectAllSlc();
            
            /* or an a2dp connection active? */
            if (theSink.inquiry.session == inquiry_session_peer)
            {   /* Entering Peer session initiation, ensure any Peer is also disconnected */
                disconnectAllA2dpAvrcp(TRUE);
            }
            else
            {   /* Non Peer discoverable mode - leave any Peer connected */
                disconnectAllA2dpAvrcp(FALSE);
            }
        }  
     
        /* Make the device connectable */
        sinkEnableConnectable();
    
        /* Make the device discoverable */  
        sinkEnableDiscoverable();    
        
        /* If there is a timeout - send a user message*/
		if ( theSink.conf1->timeouts.PairModeTimeoutIfPDL_s != 0 )
		{
			/* if there are no entries in the PDL, then use the second
			   pairing timer */
			uint16 lNumDevices = ConnectionTrustedDeviceListSize();
			if( lNumDevices != 0)
			{	/* paired devices in list, use original timer if it exists */
				if( theSink.conf1->timeouts.PairModeTimeout_s != 0 )
				{
					SM_DEBUG(("SM : Pair [%x]\n" , theSink.conf1->timeouts.PairModeTimeout_s )) ;
					MessageSendLater ( &theSink.task , EventSysPairingFail , 0 , D_SEC(theSink.conf1->timeouts.PairModeTimeout_s) ) ;
				}
			}
			else
			{	/* no paired devices in list, use secondary timer */
	            SM_DEBUG(("SM : Pair (no PDL) [%x]\n" , theSink.conf1->timeouts.PairModeTimeoutIfPDL_s )) ;
				MessageSendLater ( &theSink.task , EventSysPairingFail , 0 , D_SEC(theSink.conf1->timeouts.PairModeTimeoutIfPDL_s) ) ;
			}			            			
		}
        else if ( theSink.conf1->timeouts.PairModeTimeout_s != 0 )
        {
            SM_DEBUG(("SM : Pair [%x]\n" , theSink.conf1->timeouts.PairModeTimeout_s )) ;
            
            MessageSendLater ( &theSink.task , EventSysPairingFail , 0 , D_SEC(theSink.conf1->timeouts.PairModeTimeout_s) ) ;
        }
        else
        {
            SM_DEBUG(("SM : Pair Indefinetely\n")) ;
        }
        /* Disable the ring PIOs if enabled*/
        stateManagerResetPIOs();
       
    	/* The device is now in the connectable/discoverable state */
        stateManagerSetState(deviceConnDiscoverable);
    }
}


/****************************************************************************
NAME	
	stateManagerEnterConnectedState

DESCRIPTION
	single point of entry for the connected state - disables disco / connectable modes
RETURNS
	void
    
*/
void stateManagerEnterConnectedState ( void )
{
    if((stateManagerGetState () != deviceConnected) && (theSink.inquiry.action != rssi_pairing))
    {
            /*make sure we are now neither connectable or discoverable*/
        SM_DEBUG(("SM:Remain in Disco Mode [%c]\n", (theSink.features.RemainDiscoverableAtAllTimes?'T':'F') )) ;
        
        if (!theSink.features.RemainDiscoverableAtAllTimes)
        {
#ifdef ENABLE_SUBWOOFER     
            if(SwatGetSignallingSink(theSink.rundata->subwoofer.dev_id))
            {
               sinkDisableDiscoverable();            
            }        
#else
            sinkDisableDiscoverable();            
#endif        
        }
        
        /* for multipoint connections need to remain connectable after 1 device connected */
        if(!theSink.MultipointEnable)
        {
#ifdef ENABLE_SUBWOOFER     
            if(SwatGetSignallingSink(theSink.rundata->subwoofer.dev_id))
            {
               sinkDisableConnectable();            
            }        
#else
            sinkDisableConnectable();            
#endif        
        }
        else
        {
            /* if both profiles are now connected disable connectable mode */
            if(deviceManagerNumConnectedDevs() == MAX_MULTIPOINT_CONNECTIONS)
            {
                /* two devices connected */
                sinkDisableConnectable();                    
            }
            else
            {
                /* still able to connect another devices */
                sinkEnableConnectable();                    
                /* remain connectable for a further 'x' seconds to allow a second 
                   AG to be connected if non-zero, otherwise stay connecatable forever */

                /* cancel any currently running timers that would disable connectable mode */
                MessageCancelAll( &theSink.task, EventSysConnectableTimeout );
                
                if(theSink.conf1->timeouts.ConnectableTimeout_s)
                {
                    MessageSendLater(&theSink.task, EventSysConnectableTimeout, 0, D_SEC(theSink.conf1->timeouts.ConnectableTimeout_s));
                }
            }
        }
    
        switch ( stateManagerGetState() )
        {    
            case deviceIncomingCallEstablish:
                if (theSink.conf1->timeouts.MissedCallIndicateTime_s != 0 && 
                    theSink.conf1->timeouts.MissedCallIndicateAttemps != 0)
                {
                    theSink.MissedCallIndicated = theSink.conf1->timeouts.MissedCallIndicateAttemps;
                    
                    MessageSend   (&theSink.task , EventSysMissedCall , 0 ) ; 
                    
            		MessageSend ( &theSink.task , EventSysSpeechRecognitionStop , 0 ) ;
            
                }
            case deviceActiveCallSCO:            
            case deviceActiveCallNoSCO:       
            case deviceThreeWayCallWaiting:
            case deviceThreeWayCallOnHold:
            case deviceThreeWayMulticall:
            case deviceOutgoingCallEstablish:
                    /*then we have just ended a call*/
                MessageSend ( &theSink.task , EventSysEndOfCall , 0 ) ;
            break ;
            default:
            break ;
        }
    
	
        MessageCancelAll ( &theSink.task , EventSysPairingFail ) ;
        
            /*disable the ring PIOs if enabled*/
        stateManagerResetPIOs();
    
        /* when returning to connected state, check for the prescence of any A2DP instances
           if found enter the appropriate state */
        if((theSink.a2dp_link_data->connected[a2dp_primary])||(theSink.a2dp_link_data->connected[a2dp_secondary]))
        {
            SM_DEBUG(("SM:A2dp connected\n")) ;
            /* are there any A2DP instances streaming audio? */
            if((A2dpMediaGetState(theSink.a2dp_link_data->device_id[a2dp_primary], theSink.a2dp_link_data->stream_id[a2dp_primary]) == a2dp_stream_streaming)||
               (A2dpMediaGetState(theSink.a2dp_link_data->device_id[a2dp_secondary], theSink.a2dp_link_data->stream_id[a2dp_secondary]) == a2dp_stream_streaming))
            {
                SM_DEBUG(("SM:A2dp connected - still streaming\n")) ;
                stateManagerSetState( deviceA2DPStreaming );
            }
            else
                stateManagerSetState( deviceConnected );               
        }
        /* no A2DP connections, go to connected state */
        else 
            stateManagerSetState( deviceConnected );
   }     
}
/****************************************************************************
NAME	
	stateManagerEnterIncomingCallEstablishState

DESCRIPTION
	single point of entry for the incoming call establish state
RETURNS
	void
    
*/
void stateManagerEnterIncomingCallEstablishState ( void )
{
   
    stateManagerSetState( deviceIncomingCallEstablish );
        
    	/*if we enter this state directly*/
    MessageCancelAll ( &theSink.task , EventSysPairingFail ) ;
    PioDrivePio(theSink.conf6->PIOIO.pio_outputs.IncomingRingPIO, TRUE) ;
#ifdef ENABLE_SPEECH_RECOGNITION
    /* speech recognition not support on HSP profile AG's */
    if ((speechRecognitionIsEnabled()) && (!HfpPriorityIsHsp(HfpLinkPriorityFromCallState(hfp_call_state_incoming)))) 
        MessageSend ( &theSink.task , EventSysSpeechRecognitionStart , 0 ) ;    
#endif    
}

/****************************************************************************
NAME	
	stateManagerEnterOutgoingCallEstablishState

DESCRIPTION
	single point of entry for the outgoing call establish state
RETURNS
	void
    
*/
void stateManagerEnterOutgoingCallEstablishState ( void )
{
    stateManagerSetState( deviceOutgoingCallEstablish );
    
    	/*if we enter this state directly*/
    MessageCancelAll ( &theSink.task , EventSysPairingFail ) ;
	
    PioDrivePio(theSink.conf6->PIOIO.pio_outputs.OutgoingRingPIO, TRUE) ;
}
/****************************************************************************
NAME	
	stateManagerEnterActiveCallState

DESCRIPTION
	single point of entry for the active call state
RETURNS
	void
    
*/
void stateManagerEnterActiveCallState ( void )   
{
    Sink sinkAG1,sinkAG2;

	if((stateManagerGetState() == deviceOutgoingCallEstablish) ||
	   (stateManagerGetState() == deviceIncomingCallEstablish))
	{
		/* If a call is being answered then send call answered event */
		MessageSend ( &theSink.task , EventSysCallAnswered , 0 ) ;		
        MessageSend ( &theSink.task , EventSysSpeechRecognitionStop , 0 ) ;
	}
	
    /* get any SCO sinks */
    HfpLinkGetAudioSink(hfp_primary_link, &sinkAG1);
    HfpLinkGetAudioSink(hfp_secondary_link, &sinkAG2);
    
    /* Check current SCO audio status */
    if(sinkAG1 || sinkAG2)
    {	
        stateManagerSetState( deviceActiveCallSCO );
    }
    else
    {
        stateManagerSetState( deviceActiveCallNoSCO );
    }
	
        /*disable the ring PIOs if enabled*/
    PioDrivePio(theSink.conf6->PIOIO.pio_outputs.IncomingRingPIO, FALSE) ;
    PioDrivePio(theSink.conf6->PIOIO.pio_outputs.OutgoingRingPIO, FALSE) ;
    PioDrivePio(theSink.conf6->PIOIO.pio_outputs.CallActivePIO, TRUE) ;
	
    /*if we enter this state directly*/
    MessageCancelAll ( &theSink.task , EventSysPairingFail ) ;

}


/****************************************************************************
NAME	
	stateManagerEnterA2dpStreamingState

DESCRIPTION
    enter A2DP streaming state if not showing any active call states
RETURNS
	void
    
*/
void stateManagerEnterA2dpStreamingState(void)
{
    /* only allow change to A2DP connected state if not currently showing 
       any active call states */
    if(stateManagerGetState() == deviceConnected) 
	{
        stateManagerSetState(  deviceA2DPStreaming );
	}
}


/****************************************************************************
NAME	
	stateManagerPowerOn

DESCRIPTION
	Power on the deviece by latching on the power regs
RETURNS
	void
    
*/
void stateManagerPowerOn( void ) 
{
    SM_DEBUG(("--hello--\nSM : PowerOn\n"));

    /* Check for DUT mode enable */
#ifndef HYDRACORE_TODO
	if(!enterDUTModeIfDUTPIOActive())
#endif /* !HYDRACORE_TODO */
    {
        /* Reset caller ID flag */
        theSink.RepeatCallerIDFlag = TRUE;

        /*cancel the event power on message if there was one*/
    	MessageCancelAll ( &theSink.task , EventSysLimboTimeout ) ;
        MessageCancelAll(&theSink.task, EventSysContinueSlcConnectRequest);

    	PioSetPowerPin ( TRUE ) ;
		
        /* Turn on AMP pio drive */
        stateManagerAmpPowerControl(POWER_UP);
        
        stateManagerEnterConnectableState( TRUE );
        
        if(theSink.features.PairIfPDLLessThan || theSink.features.AutoReconnectPowerOn || theSink.panic_reconnect)
        {
            uint16 lNumDevices = ConnectionTrustedDeviceListSize();
        
            /* Check if we want to start RSSI pairing */
            if( lNumDevices < theSink.features.PairIfPDLLessThan )
            {
                SM_DEBUG(("SM: NumD [%d] <= PairD [%d]\n" , lNumDevices , theSink.features.PairIfPDLLessThan))

                /* send event to enter pairing mode, that event can be used to play a tone if required */
                MessageSend(&theSink.task, EventSysEnterPairingEmptyPDL, 0);
                MessageSend(&theSink.task, EventUsrRssiPair, 0);
            }
            else if ((theSink.features.AutoReconnectPowerOn)||theSink.panic_reconnect)
            {
                sinkEvents_t event = EventUsrEstablishSLC;
#ifdef ENABLE_AVRCP
                if(theSink.features.avrcp_enabled)
                {
                    sinkAvrcpCheckManualConnectReset(NULL);
                }
#endif                
                SM_DEBUG(("SM: Auto Reconnect\n")) ;
                if(theSink.panic_reconnect)
                {
					event = EventSysEstablishSLCOnPanic;
                }

                /*  Queue the event with a delay for the A2DP library to be ready  */
                sinkSendLater(event, 100);
            }
        }
	}
}

/****************************************************************************
NAME	
	stateManagerIsConnected

DESCRIPTION
    Helper method to see if we are connected or not   
*/
bool stateManagerIsConnected ( void )
{
    bool lIsConnected = FALSE ;
    
    switch (stateManagerGetState() )
    {
        case deviceLimbo:
        case deviceConnectable:
        case deviceConnDiscoverable:
        case deviceTestMode:
            lIsConnected = FALSE ;    
        break ;
        
        default:
            lIsConnected = TRUE ;
        break ;
    }
    return lIsConnected ;
}

/****************************************************************************
NAME	
	stateManagerEnterLimboState

DESCRIPTION
    method to provide a single point of entry to the limbo /poweringOn state
RETURNS	
    
*/
void stateManagerEnterLimboState ( void )
{
    
    SM_DEBUG(("SM: Enter Limbo State[%d]\n" , theSink.conf1->timeouts.LimboTimeout_s)); 
    
    /*set a timeout so that we will turn off eventually anyway*/
    MessageCancelAll ( &theSink.task , EventSysLimboTimeout ) ;
    MessageSendLater ( &theSink.task , EventSysLimboTimeout , 0 , D_SEC(theSink.conf1->timeouts.LimboTimeout_s) ) ;

    stateManagerSetState( deviceLimbo );

	/*Make sure panic reconnect flag is reset*/
    theSink.panic_reconnect = FALSE;
	
    if((isPowerDownPioConfigured() || isMutePioConfigured()) && !theSink.features.PlayUsbAndWiredInLimbo)
    {
    	/* Invoke the system event based on a timeout value to turn off AMP eventually once the timer expires,
    	if timeout interval is too short, audio prompts and tones may not be audible after this event is invoked */
        sinkSendLater(EventSysAmpPowerDown, D_SEC(theSink.conf1->timeouts.AudioAmpPowerDownTimeoutInLimbo_s));
    }

    /*Cancel inquiry if in progress*/
    inquiryStop();

    /* Disconnect any slc's and cancel any further connection attempts including link loss */
    MessageCancelAll(&theSink.task, EventSysStreamEstablish);
    MessageCancelAll(&theSink.task, EventSysContinueSlcConnectRequest);
    MessageCancelAll(&theSink.task, EventSysLinkLoss);
    sinkDisconnectAllSlc();     
    
    /* disconnect any a2dp signalling channels */
    disconnectAllA2dpAvrcp(TRUE);

    /* reset the pdl list indexes in preparation for next boot */
    slcReset();
    
    /*in limbo, the internal regs must be latched on (smps and LDO)*/
    PioSetPowerPin ( TRUE ) ;
    
    /*make sure we are neither connectable or discoverable*/
    sinkDisableDiscoverable();
#ifdef ENABLE_SUBWOOFER
    /* allow subwoofer to connect */
    sinkEnableConnectable();    
#else 
    sinkDisableConnectable();
#endif

#if defined ENABLE_PEER
    /* Ensure Peer device is at top of PDL when we power back on */
    AuthInitPermanentPairing();
#endif

    /* reconnect usb if applicable */
    audioUpdateAudioRouting();
    
#ifdef ENABLE_SUBWOOFER   
    MessageSend(&theSink.task, EventSysSubwooferCloseMedia, 0);                               
#endif    
}

bool stateManagerIsReadyForAudio(void)
{
    if(isPowerDownPioConfigured() || isMutePioConfigured())
    {
        if(stateManagerGetState() == deviceLimbo)
        {
            return theSink.audioAmpReady;
        }
    }
    return TRUE;
}

/****************************************************************************
NAME	
	stateManagerAmpPowerControl

DESCRIPTION
    Control the power supply for audio amplifier stage
    
RETURNS
    void
    
*/
void stateManagerAmpPowerControl(power_control_dir control)
{
    MessageCancelAll(&theSink.task, EventSysAmpPowerDown);
    MessageCancelAll(&theSink.task, EventSysAmpPowerUp);
    
    SM_DEBUG(("SM: amp: audioAmpReady=%u control=%u\n",
              theSink.audioAmpReady,
              control));
    
    if(isMutePioConfigured())
    {
        if (control == POWER_DOWN)
        {
            if (theSink.audioAmpReady)
            {
            /*  Mute the amplifier and queue an event to power it down  */
                PioDrivePio(PIO_AMP_MUTE, TRUE);
                sinkSendLater(EventSysAmpPowerDown, theSink.conf1->timeouts.AudioAmpMuteTime_ms);
                theSink.audioAmpReady = FALSE;
            }
            else
            {
            /*  Power down the amplifier  */
                PioDrivePio(PIO_POWER_ON, FALSE);            
            }
        }
        else
        {
            if (theSink.audioAmpReady)
            {
            /*  Unmute the amplifier  */
                PioDrivePio(PIO_AMP_MUTE, FALSE);
            }
            else
            {
            /*  Power up the amplifier and queue an event to unmute it  */
                PioDrivePio(PIO_POWER_ON, TRUE);            
                sinkSendLater(EventSysAmpPowerUp, theSink.conf1->timeouts.AudioAmpUnmuteTime_ms);
                theSink.audioAmpReady = TRUE;
            }
        }
    }
    else
    {
    /*  No muting required; drive power PIO if configured and set amp ready flag  */
        if (control == POWER_DOWN)
        {
            PioDrivePio(PIO_POWER_ON, FALSE);
            theSink.audioAmpReady = FALSE;
        }
        else
        {
            PioDrivePio(PIO_POWER_ON, TRUE);
            theSink.audioAmpReady = TRUE;
        }
    }
}

/****************************************************************************
NAME	
	stateManagerUpdateLimboState

DESCRIPTION
    method to update the limbo state and power off when necessary
RETURNS	
    
*/
void stateManagerUpdateLimboState ( void ) 
{
    /* determine if charger is still connected */
    bool power_off_request = FALSE;
    bool chg_connected = powerManagerIsChargerConnected();
    
    SM_DEBUG(("SM: Limbo Update\n"));
  
    /* determine if device can be turned off, hs can power off with VregEn High unless 
       feature is set specifying it should be low to allow power down, critical battery
       and over temperature alerts can also power down the device */
    if (!chg_connected)
    {
        /* charger not connected, check if Vbat is critical or temperature is outside of operating limits. */
        if(powerManagerIsVbatCritical() || powerManagerIsVthmCritical())
        {
            /* turn off power as outside of safe operating limits */
            power_off_request = TRUE;
        } 
        /* Do not physically power-off, if DisableCompletePowerOff is enabled */
        else if(!theSink.features.DisableCompletePowerOff)

        {
            

            /* When battery is used (headset & speaker applications) this is needed in order to 
               power off and not draining the battery. Unless the specific feature is enabled.*/ 
            if(    !sinkFmIsFmRxOn() 
                && (   ((!analogAudioConnected()) && (!spdifAudioConnected()) && (!i2sAudioConnected())) 
                    || (   theSink.features.PowerOffOnWiredAudioConnected 
                        && (analogAudioConnected() || i2sAudioConnected() || spdifAudioConnected()))
#ifdef ENABLE_BATTERY_OPERATION
                    || (!theSink.features.PlayUsbAndWiredInLimbo)
#endif
               ))
            {
                /* check power off only if vreg is low feature bit */
                if((!theSink.features.PowerOffOnlyIfVRegEnLow)||(theSink.features.PowerOffOnlyIfVRegEnLow && !PsuGetVregEn()))                    
                {
                    /* safe to power off */                    
                    power_off_request = TRUE;
                }
            }     
        }
    }
    /* check if need to power off unit */    
    if(power_off_request)    
    {
        SM_DEBUG(("SM : Power Off\n--goodbye--\n")) ;
        configManagerDefragIfRequired();
        /* Used as a power on indication if required */
        PioDrivePio(PIO_POWER_ON, FALSE) ;
        /* Physically power off */
        PioSetPowerPin(FALSE);
    }
    else
    {
        /* Device was not able to power down at this point, schedule another check.
           For soundbar application (where battery is not used) power_off_request 
           will never be set to TRUE and the following timer will always restart. */
        sinkSendLater(EventSysLimboTimeout, D_SEC(theSink.conf1->timeouts.LimboTimeout_s));
    }           
}

/****************************************************************************
NAME	
	stateManagerEnterTestModeState

DESCRIPTION
    method to provide a single point of entry to the test mode state
RETURNS	
    
*/
void stateManagerEnterTestModeState ( void )
{
    stateManagerSetState( deviceTestMode );
}

/****************************************************************************
NAME	
	stateManagerEnterCallWaitingState

DESCRIPTION
    method to provide a single point of entry to the 3 way call waiting state
RETURNS	
    
*/
void stateManagerEnterThreeWayCallWaitingState ( void ) 
{
    stateManagerSetState( deviceThreeWayCallWaiting );
	
    	/*if we enter this state directly*/
    MessageCancelAll ( &theSink.task , EventSysPairingFail ) ;
}


void stateManagerEnterThreeWayCallOnHoldState ( void ) 
{   
    stateManagerSetState( deviceThreeWayCallOnHold );
	
    	/*if we enter this state directly*/
    MessageCancelAll ( &theSink.task , EventSysPairingFail ) ;
}

void stateManagerEnterThreeWayMulticallState ( void ) 
{
    stateManagerSetState( deviceThreeWayMulticall );
	
    	/*if we enter this state directly*/
    MessageCancelAll ( &theSink.task , EventSysPairingFail ) ;
}


void stateManagerEnterIncomingCallOnHoldState ( void )
{
    switch ( stateManagerGetState() )
    {    
        case deviceIncomingCallEstablish:
        	MessageSend ( &theSink.task , EventSysSpeechRecognitionStop , 0 ) ;
        break ;
        default:
        break ;
    }    
     
    stateManagerSetState( deviceIncomingCallOnHold );
	
    	/*if we enter this state directly*/
    MessageCancelAll ( &theSink.task , EventSysPairingFail ) ;
}

static void stateManagerResetPIOs ( void )
{
    /*disable the ring PIOs if enabled*/
    PioDrivePio(theSink.conf6->PIOIO.pio_outputs.IncomingRingPIO, FALSE) ;
    PioDrivePio(theSink.conf6->PIOIO.pio_outputs.OutgoingRingPIO, FALSE) ;
    /*diaable the active call PIO if there is one*/
    PioDrivePio(theSink.conf6->PIOIO.pio_outputs.CallActivePIO, FALSE) ;
     
}

