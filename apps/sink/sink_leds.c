/*
Copyright (c) 2007 - 2016 Qualcomm Technologies International, Ltd.
*/

/*!
@file
@ingroup sink_app
@brief   
    Module responsible for managing the LED outputs and the pios 
    configured as led outputs
*/

#include "sink_leds.h"
#include "sink_private.h"
#include "sink_led_manager.h"
#include "sink_statemanager.h"
#include "sink_pio.h"

#include <panic.h>
#include <stddef.h>
#include <led.h>
#include <string.h>

#ifdef DEBUG_LEDS
#define LED_DEBUG(x) {printf x;}
#else
#define LED_DEBUG(x) 
#endif

#ifdef HYDRACORE
#define ERROR_LEDS { LED_0, LED_1, LED_2, LED_3, LED_4, LED_5 }
#else
#define ERROR_LEDS { LED_0, LED_1, LED_2 }
#endif

#define LED_ON  TRUE    
#define LED_OFF FALSE

#define ERROR_ON_TIME_MS    300
#define ERROR_OFF_TIME_MS   300

#define LEDS_STATE_START_DELAY_MS 300

 /*internal message handler for the LED callback messages*/
static void LedsMessageHandler( Task task, MessageId id, Message message ) ;

 /*helper functions for the message handler*/
static uint16 LedsApplyFilterToTime     ( uint16 pTime )  ;
static LEDColour_t LedsGetPatternColour ( const LEDPattern_t * pPattern ) ;

 /*helper functions to change the state of LED pairs depending on the pattern being played*/
static void LedsTurnOffLEDPair ( LEDPattern_t * pPattern, bool pUseOveride  ) ;
static void LedsTurnOnLEDPair  ( LEDPattern_t * pPattern , LEDActivity_t * pLED );

    /*method to complete an event*/
static void LedsEventComplete ( LEDActivity_t * pPrimaryLed , LEDActivity_t * pSecondaryLed ) ;
    /*method to indicate that an event has been completed*/
static void LedsSendEventComplete ( sinkEvents_t pEvent , bool pPatternCompleted ) ;

    /*filter enable - check methods*/
static bool LedsIsFilterEnabled ( uint16 pFilter ) ;
static void LedsEnableFilter ( uint16 pFilter , bool pEnable) ;

static void LedsHandleOverideLED ( bool pOnOrOff ) ;

    /*Follower LED helper functions*/
static bool LedsCheckFiltersForLEDFollower( void ) ;

static uint16 LedsGetLedFollowerRepeatTimeLeft( LEDPattern_t* pPattern) ;


static uint16 LedsGetLedFollowerStartDelay( void ) ;

static void LedsSetEnablePin ( bool pOnOrOff ) ;

static void ledsTurnOnAltLeds(uint8 On_LedA, uint8 Off_LedB);

#ifdef DEBUG_DIM
#define DIM_DEBUG(x) DEBUG(x)
#else
#define DIM_DEBUG(x) 
#endif

#define DIM_NUM_STEPS (0xf)
#define DIM_MAX       (0xfff)
#define DIM_STEP_SIZE ((DIM_MAX + 1) / (DIM_NUM_STEPS + 1) ) 
#define DIM_PERIOD    (0x0)

/* Special case pPIOs for LEDS and pairs of pPIOs (Tri Colour LEDs)*/
/* Helper functions assume that HW LEDs are grouped before pairs of pPIOs */
#define PIO_LED_0         10
#define PIO_LED_1         11
#define PIO_LED_2         12

#define PIO_LEDPAIR_AB    13
#define PIO_LEDPAIR_BC    14
#define PIO_LEDPAIR_CA    15

#define PIO_LED_FIRST PIO_LED_0
#define PIO_LED_LAST  PIO_LED_2

#define LED_0_PIOPIN      30
#define LED_1_PIOPIN      31
#define LED_2_PIOPIN      29
#define MAP_LED_ID_TO_PIOPIN(ledId) ((ledId == LED_0) ? LED_0_PIOPIN : ((ledId == LED_1) ? LED_1_PIOPIN : LED_2_PIOPIN))

/* Helper functions for translating pPIO into hardware pins */
static void LedpPIOtoHW (uint16 pPIO, LEDpPIOInfo_t * pPIOInfo);
static void LedGetInfo (uint16 pPIO, LEDInfo_t * ledInfo);

/* Helper functions for setting hardware pins and global LED state */
static void PioSetLedAndgState (LEDInfo_t * led, bool pOnOrOff);
static void PioSetLedDim (LEDInfo_t * led, uint16 lDim, bool enable);
static void LedSetgState (LEDInfo_t * led, bool pOnOrOff);
static bool SinkLedConfigure(led_id led, led_config_key key, uint16 value);

/****************************************************************************
NAME
    LedpPIOtoHW

DESCRIPTION
    Helper function call LedGetInfo to fill a LEDpPIOInfo structure for all
    LEDs or PIOs based on pPIO.
    
RETURNS
    void
*/
static void LedpPIOtoHW (uint16 pPIO, LEDpPIOInfo_t * pPIOInfo)
{
    pPIOInfo->is_ledpair = FALSE; /* assume not a LED pair to start with */
    
    if ( pPIO <= PIO_LED_LAST ) /* single PIO or LED */
    {
        LedGetInfo ( pPIO, &pPIOInfo->led1 );
    }
    else /* pPIO is a pair of pPIOs */
    {
        pPIOInfo->is_ledpair = TRUE;
        switch(pPIO)
        {
            case PIO_LEDPAIR_AB:
                LedGetInfo ( theSink.theLEDTask->gTriColLeds.TriCol_a, &pPIOInfo->led1 );
                LedGetInfo ( theSink.theLEDTask->gTriColLeds.TriCol_b, &pPIOInfo->led2 );
                break;
            case PIO_LEDPAIR_BC:
                LedGetInfo ( theSink.theLEDTask->gTriColLeds.TriCol_b, &pPIOInfo->led1 );
                LedGetInfo ( theSink.theLEDTask->gTriColLeds.TriCol_c, &pPIOInfo->led2 );
                break;
            case PIO_LEDPAIR_CA:
                LedGetInfo ( theSink.theLEDTask->gTriColLeds.TriCol_c, &pPIOInfo->led1 );
                LedGetInfo ( theSink.theLEDTask->gTriColLeds.TriCol_a, &pPIOInfo->led2 );
                break;
            default:
                /* shouldn't get here */
                break;
        }
    }
}

/****************************************************************************
NAME
    SinkLedConfigure

DESCRIPTION
    Helper function to configure LEDS, only if LED pin is configured as PIO in PIO_MAP
	Note: If configuration not available, default behavior is to go ahead with LedConfigure for this LED pin
    
RETURNS
    The result of LedConfigure, if called, FALSE otherwise
*/
static bool SinkLedConfigure(led_id led, led_config_key key, uint16 value)
{
	if ((theSink.conf6 == NULL) ||
	    ((theSink.conf6 != NULL) && (!((theSink.conf6->PIOIO.pio_map >> MAP_LED_ID_TO_PIOPIN(led)) & 0x1))))
	{
		return LedConfigure(led, key, value);
	}
    
	return FALSE;
}	

/****************************************************************************
NAME
    LedGetInfo

DESCRIPTION
    Helper function to get info for a single LED or PIO based on pPIO to
    populate LEDInfo structure with hardware details.
    
RETURNS
    void
*/
static void LedGetInfo (uint16 pPIO, LEDInfo_t * ledInfo)
{  
    if ( pPIO < PIO_LED_FIRST ) /* pPIO is a PIO */
    {
        ledInfo->led = pPIO;
        ledInfo->is_led = 0;
        ledInfo->OnOff = 0;
        ledInfo->current_dim = 0;
    }
    else if( pPIO <= PIO_LED_LAST )/* pPIO is a LED */
    {
        /* LEDs start at PIO_LED_0 so hw led is pPIO - PIO_LED_FIRST (as LED_0 is first) */
        uint16 led = pPIO - PIO_LED_FIRST;
        
        ledInfo->led = led;
        ledInfo->is_led = 1;
        ledInfo->OnOff = theSink.theLEDTask->gLEDState[led].OnOff;
        ledInfo->current_dim = theSink.theLEDTask->gLEDState[led].current_dim;         
    }
}    

/****************************************************************************
NAME
    LedSetgState

DESCRIPTION
    Helper function set the gLEDState[led].OnOff based on the LED stored in a
    LEDInfo structure.
    Only use for hardware LEDs (not PIOs).
    
RETURNS
    void
*/
static void LedSetgState (LEDInfo_t * led, bool pOnOrOff)
{
    /* set theLEDTask led on off state with pOnOrOff */
    theSink.theLEDTask->gLEDState[led->led].OnOff = pOnOrOff;
}

/****************************************************************************
NAME    
    PioSetLedAndgState

DESCRIPTION
    Helper function set PIO or LED hardware (non dimming).
    For an LED it will call LedSetgState to update global LED state.
    
RETURNS
    void
*/
static void PioSetLedAndgState (LEDInfo_t * led, bool pOnOrOff)
{
    if ( led->is_led )
    {
        PioSetLedDim (led, DIM_MAX, pOnOrOff);
        LedSetgState (led, pOnOrOff) ;
    }
    else
    {
        PioSetPio (led->led , pio_drive, pOnOrOff) ;
    }
}

/****************************************************************************
NAME    
    PioSetLedDim

DESCRIPTION
    Helper function set dim value for LED hardware.
    Only use for hardware LEDs (not PIOs).
    Sets hardware and gLEDState[Led_x].current_dim value.
    
RETURNS
    void
*/
static void PioSetLedDim (LEDInfo_t * led, uint16 lDim, bool enable)
{
    /* Store dim value for later comparisons */
    theSink.theLEDTask->gLEDState[led->led].current_dim = lDim;
    /* Set LED */
    SinkLedConfigure(led->led, LED_DUTY_CYCLE, lDim);
    SinkLedConfigure(led->led, LED_PERIOD, DIM_PERIOD );  
    SinkLedConfigure(led->led, LED_ENABLE, (uint16)enable ) ;
}

/****************************************************************************
NAME    
    PioSetLedPin

DESCRIPTION
    Fn to change / set an LED attached to a PIO, a special LED pin , or a tricolour LED
    
RETURNS
    void
*/
void PioSetLedPin ( uint16 pPIO , bool pOnOrOff ) 
{
    LEDActivity_t * gActiveLED = &theSink.theLEDTask->gActiveLEDS[pPIO];
    LEDpPIOInfo_t * gpPIOInfo = &theSink.theLEDTask->gpPIOInfo;
    
    LED_DEBUG(("LM : SetLed [%x][%x] \n", pPIO , pOnOrOff )) ;

    /* find out what sort LED(s) / PIO(s) we are using */
    LedpPIOtoHW (pPIO, gpPIOInfo);

    /* gActiveLED is based on pPIO so check for dimming first.
       Only Dim pPIO if DimTime > 0 and all LEDs requested are HW LEDs (cannot dim PIO).
       If any PIOs are configured, turn on and off without dimming.
       gpPIOInfo holds led1 or led2 information so we can check if they are leds via is_led.
       led1 will always be used so can be &&'d with DimTime check.
       led2 should only block dimming if pPIO is a pair and led2 is not an led,
       so &&(!(!pPIOInfo->led1.is_led && pPIOInfo->is_ledpair)) */
     
    if (( gActiveLED->DimTime > 0 ) && (( gpPIOInfo->led1.is_led ) && (!(!gpPIOInfo->led2.is_led && gpPIOInfo->is_ledpair))))
    {
        /* if the request is to do the same as what we are doing then ignore.
           if we have a single led (not pair) then just check state of led1.
           if we have a pair of leds then only ignore if both are already the 
           requestedpOnOrOff state*/
        if ( ((!gpPIOInfo->is_ledpair) && ( gpPIOInfo->led1.OnOff != pOnOrOff )) || 
             ( gpPIOInfo->is_ledpair && (( gpPIOInfo->led1.OnOff != pOnOrOff ) || ( gpPIOInfo->led2.OnOff != pOnOrOff ))))
        {
            uint16 lDim = 0x0000 ;
            
            /*set led to max or min depending on whether we think the led is on or off*/
            gActiveLED->DimState = (DIM_NUM_STEPS * !pOnOrOff) ; 
            gActiveLED->DimDir   = pOnOrOff ; /*1=go up , 0 = go down*/
            
            lDim = gActiveLED->DimState * (DIM_STEP_SIZE);
            
            /* check led1 is not already new state */
            if ( gpPIOInfo->led1.OnOff != pOnOrOff )
            {
                /* Only set a new dim value if dimming down and new value < current dim value
                   or if dimming up and new value > current dim value */
                if ( pOnOrOff != ( lDim < gpPIOInfo->led1.current_dim ))
                {
                    /* set led1 with initial dim value and update gLEDState*/
                    PioSetLedDim (&gpPIOInfo->led1, lDim, TRUE);
                }
                /* Update the OnOff state for the hw led */
                LedSetgState (&gpPIOInfo->led1, pOnOrOff) ;
            }
            
            /* if pPIO is an LED pair, set the 2nd LED with the dim value */
            if (gpPIOInfo->is_ledpair)
            {
                /* check led2 is not already new state */
                if ( gpPIOInfo->led2.OnOff != pOnOrOff )
                {
                    /* Only set a new dim value if dimming down and new value < current dim value
                       or if dimming up and new value > current dim value */
                    if ( pOnOrOff != ( lDim < gpPIOInfo->led2.current_dim ))
                    {
                        /* set led2 with initial dim value and update gLEDState*/
                        PioSetLedDim (&gpPIOInfo->led2, lDim, TRUE);
                    }
                    /* Update the OnOff state for the hw led */
                    LedSetgState (&gpPIOInfo->led2, pOnOrOff) ;
                }
            }
            
            DIM_DEBUG(("DIM: Set pPIO [%d][%x][%d]\n" ,pPIO ,gActiveLED->DimState ,gActiveLED->DimDir  )) ;
            
            /*send the first message to continue the dimming sequence*/
            MessageCancelAll ( &theSink.theLEDTask->task, (MessageId)(DIM_MSG_BASE + pPIO) ) ;                
            MessageSendLater ( &theSink.theLEDTask->task, (MessageId)(DIM_MSG_BASE + pPIO) ,0 ,gActiveLED->DimTime ) ;
        }
        
    }
    else /* not a dimming pattern or there is at least one PIO */
    {
        DIM_DEBUG(("DIM: Set pPIO [%d] N:[%d]\n" , pPIO, pOnOrOff)) ;
        /* set PIO or LED and state */
        PioSetLedAndgState (&gpPIOInfo->led1, pOnOrOff) ;
        /* set 2nd PIO or LED and state if required */
        if ( gpPIOInfo->is_ledpair )
            PioSetLedAndgState (&gpPIOInfo->led2, pOnOrOff) ;
    }  
}	

/****************************************************************************
NAME
    PioSetDimState  
    
DESCRIPTION
    Update funtion for a led that is currently dimming
    
RETURNS
    void
*/
void PioSetDimState ( uint16 pPIO )
{
    uint16 lDim = 0x0000 ;
    LEDActivity_t *gActiveLED = &theSink.theLEDTask->gActiveLEDS[pPIO];
    LEDpPIOInfo_t * gpPIOInfo = &theSink.theLEDTask->gpPIOInfo;
    
    /* find the LED(s) we are using */
    LedpPIOtoHW (pPIO, gpPIOInfo);
    
    if (gActiveLED->DimDir && ( gActiveLED->DimState >= DIM_NUM_STEPS ) )
    {
        lDim = DIM_MAX;
        DIM_DEBUG(("DIM:+[F] [ON] pPIO [%d] \n", pPIO ));
    }
    else if ( !gActiveLED->DimDir && ( gActiveLED->DimState == 0x0 ) )
    {
        lDim = 0 ;
        DIM_DEBUG(("DIM:-[0] [OFF] pPIO [%d] \n", pPIO ));
    }
    else
    {
        if(gActiveLED->DimDir)
            gActiveLED->DimState++ ;
        else
            gActiveLED->DimState-- ;
        
        DIM_DEBUG(("DIM:Direction [%x], DimState:[%x], DimTime:[%x]\n", gActiveLED->DimDir, gActiveLED->DimState, gActiveLED->DimTime));
        
        lDim = (gActiveLED->DimState * (DIM_STEP_SIZE) ) ;

        MessageCancelAll ( &theSink.theLEDTask->task, (MessageId)(DIM_MSG_BASE + pPIO) ) ;
        MessageSendLater ( &theSink.theLEDTask->task, (MessageId)(DIM_MSG_BASE + pPIO) , 0 , gActiveLED->DimTime ) ;
    }

    /* Only set dim value if we are dimming the correct direction.
       We can have the case where an LED will dim up and down at the same time.
       gpPIOInfo->ledx.OnOff = gLEDState[led].OnOff can be used for the last direction
       set for the LED */
    if (gActiveLED->DimDir == gpPIOInfo->led1.OnOff)
    {
        /* Only set a new dim value if dimming down and new value < current dim value
           or if dimming up and new value > current dim value */
        if ( gActiveLED->DimDir != ( lDim < gpPIOInfo->led1.current_dim ))
        {
            /* set first LED with required lDim value and update gLEDState*/
            PioSetLedDim (&gpPIOInfo->led1, lDim, TRUE);
        }
    }
    /* if pPIO is an LED pair, set the 2nd LED with the required lDim value if dim direction is correct*/
    if ( gpPIOInfo->is_ledpair && (gActiveLED->DimDir == gpPIOInfo->led2.OnOff))
    {
        /* Only set a new dim value if dimming down and new value < current dim value
           or if dimming up and new value > current dim value */
        if ( gActiveLED->DimDir != ( lDim < gpPIOInfo->led2.current_dim ))
        {
            /* set first LED with required lDim value and update gLEDState*/
            PioSetLedDim (&gpPIOInfo->led2, lDim, TRUE) ;
        }
    }
}

/****************************************************************************
NAME 
 LedsInit

DESCRIPTION
 	Initialise the Leds data
RETURNS
 void
    
*/
void LedsInit ( void ) 
{
        /*Set the callback handler for the task*/
    theSink.theLEDTask->task.handler = LedsMessageHandler ;
    
    theSink.theLEDTask->gCurrentlyIndicatingEvent = FALSE ;
    	/*set the tricolour leds to known values*/
    theSink.theLEDTask->gTriColLeds.TriCol_a = 0 ;
    theSink.theLEDTask->gTriColLeds.TriCol_b = 0 ;
    theSink.theLEDTask->gTriColLeds.TriCol_c = 0 ;
    
    theSink.theLEDTask->gFollowing = FALSE ;
}

     
/****************************************************************************
NAME 
 LedsCheckForFilter

DESCRIPTION
 This function checksif a filter has been configured for the given event, 
    if it has then activates / deactivates the filter 
    
    Regardless of whether a filter has been activated or not, the event is signalled as 
    completed as we have now deaklt with it (only checked for a filter if a pattern was not
    associated.

RETURNS
 void
    
*/       
void LedsCheckForFilter ( sinkEvents_t pEvent ) 
{
    uint16 lFilterIndex = 0 ;
    
    for (lFilterIndex = 0 ; lFilterIndex < theSink.theLEDTask->gLMNumFiltersUsed ; lFilterIndex ++ )
    { 
        LEDFilter_t *lEventFilter = &(theSink.theLEDTask->pEventFilters [ lFilterIndex ]);
        
        if((uint16)(lEventFilter->Event) == pEvent && lEventFilter->FilterType != DISABLED)
        {
            if (lEventFilter->FilterType != CANCEL)
            {
                /* Check filter isn't already enabled */
                if (!LedsIsFilterEnabled(lFilterIndex))
                {
                    /* Enable filter */
                    LedsEnableFilter (lFilterIndex , TRUE) ;
            
                    /* If it is an overide LED filter and the currently playing pattern is OFF then turn on the overide led immediately*/
                    if ( lEventFilter->FilterType == OVERRIDE)
                    {                        
                        uint16 lOverideLEDIndex = lEventFilter->OverideLED ;                    
                        
                        /* this should only happen if the led in question is currently off*/
                        if ( theSink.theLEDTask->gActiveLEDS[lOverideLEDIndex].OnOrOff == LED_OFF)
                        {
                             LED_DEBUG(("LED: FilEnable Turn on[%d][%d] \n",lFilterIndex + 1 , lOverideLEDIndex  )) ;
                             PioSetLedPin ( lOverideLEDIndex , LED_ON) ;
                        }
                    }
                }
            }
            else
            {
                 uint16 lFilterToCancel = lEventFilter->FilterToCancel ;
                /*disable the according filter*/
                 if ( lFilterToCancel != 0 )
                 {
                     uint16 lFilterToCancelIndex = lFilterToCancel - 1 ;
                     LEDFilter_t *lEventFilter1  = &(theSink.theLEDTask->pEventFilters [ lFilterToCancelIndex ]);
                     uint16 lOverideLEDIndex     = lEventFilter1->OverideLED ;
                    
                     LED_DEBUG(("LED: FilCancel[%d][%d] [%d]\n",lFilterIndex + 1 , lFilterToCancel , lFilterToCancelIndex )) ;
                     
                        /*lFilter To cancel = 1-n, LedsEbnable filter requires 0-n */
                     LedsEnableFilter (lFilterToCancelIndex , FALSE ) ;
                     
                     if ( theSink.theLEDTask->gActiveLEDS[lOverideLEDIndex].OnOrOff == LED_OFF)
                     {   /*  LedsHandleOverideLED ( theSink.theLEDTask , LED_OFF) ;*/
                         if ( lEventFilter1->FilterType == OVERRIDE)
                         {
                             LED_DEBUG(("LED: FilCancel Turn off[%d][%d] [%d]\n",lFilterIndex + 1 , lFilterToCancel , lFilterToCancelIndex )) ;
	          	             PioSetLedPin ( lOverideLEDIndex, LED_OFF) ;                
                             
                             /* it is possible for the cancel filter to turn off leds used in a solid led
                                state indication such as a solid blue pairing indication, should the charger be
                                removed and then reinserted the solid blue state is turned off, this call will reset
                                the state indication and turn it back on again */
                             LEDManagerIndicateState ( stateManagerGetState () ) ;                       

                         }    
                     }                           
                 }
                 else
                 {
                    LED_DEBUG(("LED: Fil !\n")) ;
                 }
            }
            LED_DEBUG(("LM : Filter Found[%d]A[%x]\n", lFilterIndex + 1,  pEvent )) ;
       }      
    }
}


/****************************************************************************
NAME 
    LedsEnableFilterOverrides

DESCRIPTION
    Enable or disable filters overriding LEDs. This will not change which 
    filters are active, it will just turn off any LEDs the filters are 
    forcing on.
    
RETURNS
    void    
*/
void LedsEnableFilterOverrides(bool enable)
{
    uint16 lFilterIndex;
    /* Run through all filters */
    for (lFilterIndex = 0 ; lFilterIndex < theSink.theLEDTask->gLMNumFiltersUsed ; lFilterIndex ++ )
    {
        LEDFilter_t *lEventFilter = &(theSink.theLEDTask->pEventFilters [ lFilterIndex ]);
        /* If filter is overriding an LED turn it off */
        if (LedsIsFilterEnabled(lFilterIndex) && (lEventFilter->FilterType == OVERRIDE))
            PioSetLedPin(lEventFilter->OverideLED, (enable ? LED_ON : LED_OFF));
    }
    /* Restore state (ensures we haven't disabled any LEDs we shouldn't) */
    LEDManagerIndicateState ( stateManagerGetState () ) ;  
}


/****************************************************************************
NAME 
    ledsIndicateLedsPattern

DESCRIPTION
 	Given the indication type and leds pattern, Play the LED Pattern
RETURNS
    void
*/
void ledsIndicateLedsPattern(LEDPattern_t *lPattern, uint8 lIndex, IndicationType_t Ind_type)
{
    uint8 lPrimaryLED     = lPattern->pattern.LED_A;
    uint8 lSecondaryLED   = lPattern->pattern.LED_B;

    #ifdef DEBUG_LM
    	LMPrintPattern( lPattern ) ;
    #endif
        
    if(Ind_type == IT_EventIndication)
    {
        /*if the PIO we want to use is currently indicating an event then do interrupt the event*/
        MessageCancelAll (&theSink.theLEDTask->task, lPrimaryLED ) ;
        MessageCancelAll (&theSink.theLEDTask->task, lSecondaryLED ) ;
    }
        
    /*cancel all led state indications*/
    /*Find the LEDS that are set to indicate states and Cancel the messages,
      -do not want to indicate more than one state at a time */
    LedsIndicateNoState ( ) ;
    
    /*now set up and start the event indication*/
    LedsSetLedActivity ( &theSink.theLEDTask->gActiveLEDS[lPrimaryLED] , Ind_type , lIndex  , lPattern->pattern.DimTime ) ;
    /*Set the Alternative LED up with the same info*/
    LedsSetLedActivity ( &theSink.theLEDTask->gActiveLEDS[lSecondaryLED] , Ind_type , lIndex , lPattern->pattern.DimTime ) ;

    /* - need to set the LEDS to a known state before starting the pattern*/
    LedsTurnOffLEDPair (lPattern , TRUE ) ;
   
    /*Handle permanent output leds*/
    if ( lPattern->pattern.NumFlashes == 0 )
    {
        /*set the pins on or off as required*/
        if ( LED_SCALE_ON_OFF_TIME(lPattern->pattern.OnTime) > 0 )
        {
            LED_DEBUG(("LM :ST PIO_ON\n")) ;
            LedsTurnOnLEDPair ( lPattern , &theSink.theLEDTask->gActiveLEDS[lPrimaryLED]) ;
        }
        else if ( LED_SCALE_ON_OFF_TIME(lPattern->pattern.OffTime) > 0 )
        {
            LED_DEBUG(("LM :ST PIO_OFF\n")) ;
            LedsTurnOffLEDPair ( lPattern , TRUE) ;
            
            LedsSendEventComplete ( lPattern->StateOrEvent, TRUE ) ;
            /*If we are turning a pin off the revert to state indication*/
            LedsEventComplete ( &theSink.theLEDTask->gActiveLEDS[lPrimaryLED] , &theSink.theLEDTask->gActiveLEDS[lSecondaryLED] ) ;
        }   
    }
    else
    {
        if(Ind_type == IT_EventIndication)
        {
            MessageSend (&theSink.theLEDTask->task, lPrimaryLED, 0) ;
            theSink.theLEDTask->gCurrentlyIndicatingEvent = TRUE ;
        }
        else
        {
            /*send the first message for this state LED indication*/ 
            MessageSendLater (&theSink.theLEDTask->task , lPrimaryLED, 0 , LEDS_STATE_START_DELAY_MS ) ;
        }
    }
}

/****************************************************************************
NAME 
 LedsIndicateNoState

DESCRIPTION
	remove any state indications as there are currently none to be displayed
RETURNS
 void
    
*/
void LedsIndicateNoState ( void  )  
{
     /*Find the LEDS that are set to indicate states and Cancel the messages,
    -do not want to indicate more than one state at a time*/
    uint16 lLoop = 0;

    for ( lLoop = 0 ; lLoop < SINK_NUM_LEDS ; lLoop ++ )
    {
        LEDActivity_t *lActiveLeds = &theSink.theLEDTask->gActiveLEDS[lLoop];
        
        if (lActiveLeds->Type == IT_StateIndication)
        {
            LEDPattern_t tempPatternState;
            tempPatternState.StateOrEvent = theSink.theLEDTask->gStatePatterns[lActiveLeds->Index].state;
            tempPatternState.pattern = theSink.theLEDTask->gStatePatterns[lActiveLeds->Index];
            
            MessageCancelAll ( &theSink.theLEDTask->task, lLoop ) ; 
            lActiveLeds->Type =  IT_Undefined ;
            
            LED_DEBUG(("LED: CancelStateInd[%x]\n" , lLoop)) ;
            
            LedsTurnOffLEDPair ( &tempPatternState, TRUE) ;                    
        }
    }
}

/****************************************************************************
NAME 
 LedActiveFiltersCanOverideDisable

DESCRIPTION
    Check if active filters disable the global LED disable flag.
RETURNS 
 	bool
*/
bool LedActiveFiltersCanOverideDisable( void )
{
    uint16 lFilterIndex ;
    
    for (lFilterIndex = 0 ; lFilterIndex< LM_NUM_FILTER_EVENTS ; lFilterIndex++ )
    {
        if ( LedsIsFilterEnabled(lFilterIndex) )
        {
            /* check if this filter overides LED disable flag */
            if ( theSink.theLEDTask->pEventFilters[lFilterIndex].OverideDisable)
                return TRUE;
        }
    }
    return FALSE;
}

/****************************************************************************
NAME 
 LEDManagerMessageHandler

DESCRIPTION
 The main message handler for the LED task. Controls the PIO in question, then 
    queues a message back to itself for the next LED update.

RETURNS
 void
    
*/
static void LedsMessageHandler( Task task, MessageId id, Message message )
{  
    bool lOldState = LED_OFF ;
    uint16 lTime   = 0 ;
    LEDColour_t lColour ;    
    
    LEDActivity_t * lLED   = &theSink.theLEDTask->gActiveLEDS[id] ;
    LEDPattern_t  lPattern ;
    bool lPatternComplete = FALSE ;
    bool resetFollowingPending = FALSE;

    UNUSED(task);
    UNUSED(message);

    if (id < DIM_MSG_BASE )
    {
        
        /*which pattern are we currently indicating for this LED pair*/
        if ( lLED->Type == IT_StateIndication)
        {
            /* this is a STATE indication */        
            lPattern.StateOrEvent = theSink.theLEDTask->gStatePatterns[ lLED->Index ].state;
            lPattern.pattern = theSink.theLEDTask->gStatePatterns[ lLED->Index ] ;
        }
        else
        {   /*is an event indication*/
            lPattern = theSink.theLEDTask->pEventPatterns [ lLED->Index ] ;
        }
        
        /* get which of the LEDs we are interested in for the pattern we are dealing with */
        lColour = LedsGetPatternColour ( &lPattern ) ;
         
        /*get the state of the LED we are dealing with*/
        lOldState = theSink.theLEDTask->gActiveLEDS [ lPattern.pattern.LED_A ].OnOrOff ;
     
        LED_DEBUG(("LM : LED[%d] [%d] f[%d]of[%d]\n", id ,lOldState , lLED->NumFlashesComplete , lPattern.pattern.NumFlashes )) ;
             
        /*  is LED currently off? */
        if (lOldState == LED_OFF)
        {
            /* led is off so start the new pattern by turning LED on */
            lTime = LED_SCALE_ON_OFF_TIME(lPattern.pattern.OnTime) ;
            LED_DEBUG(("LED: set ON time [%x]\n",lTime)) ;   
                
            /*Increment the number of flashes*/
            lLED->NumFlashesComplete++ ;
                  
            LED_DEBUG(("LED: Pair On\n")) ;
            LedsTurnOnLEDPair ( &lPattern , lLED ) ;
            
        }
        else
        {   
            /*restart the pattern if we have palayed all of the required flashes*/
            if ( lLED->NumFlashesComplete >= lPattern.pattern.NumFlashes )
            {
                lTime = LED_SCALE_REPEAT_TIME(lPattern.pattern.RepeatTime) ;
                lLED->NumFlashesComplete = 0 ;       
                    /*inc the Num times the pattern has been played*/
                lLED->NumRepeatsComplete ++ ;
                LED_DEBUG(("LED: Pat Rpt[%d] [%d][%d]\n",lTime, lLED->NumRepeatsComplete , lPattern.pattern.TimeOut)) ;
          
                /*if a single pattern has completed*/
                if ( LED_SCALE_REPEAT_TIME(lPattern.pattern.RepeatTime) == 0 ) 
                {
                    LED_DEBUG(("LED: PC: Rpt\n")) ;
                    lPatternComplete = TRUE ;
                }
                   /*a pattern timeout has occured*/
                if ( ( lPattern.pattern.TimeOut !=0 )  && ( lLED->NumRepeatsComplete >= lPattern.pattern.TimeOut) )
                {
                    lPatternComplete = TRUE ;
                    LED_DEBUG(("LED: PC: Rpt b\n")) ;
                }              
                
                /*if we have reached the end of the pattern and are using a follower then revert to the orig pattern*/
                if (theSink.theLEDTask->gFollowing)
                {
                    /* resetting theSink.theLEDTask->gFollowing is delayed [done later in this function],
					   to allow turning off of the follower led */
                    resetFollowingPending = TRUE ; 
                    lTime = LedsGetLedFollowerRepeatTimeLeft( &lPattern ) ;    
                }
                else
                {
                    /*do we have a led follower filter and are we indicating a state, if so use these parameters*/
                    if (lLED->Type == IT_StateIndication)
                    {
                        if( LedsCheckFiltersForLEDFollower( ) )
                        {
                            lTime = LedsGetLedFollowerStartDelay( ) ;       
                            theSink.theLEDTask->gFollowing = TRUE ;
                        }
                    }    
                 }            
            } 
            else /*otherwise set up for the next flash*/
            {
                lTime = LED_SCALE_ON_OFF_TIME(lPattern.pattern.OffTime) ;
                LED_DEBUG(("LED: set OFF time [%x]\n",lTime)) ;   
        	} 
            
        						
    		lColour = LedsGetPatternColour ( &lPattern ) ;
				
			if ( lColour != LED_COL_LED_ALT )
			{
                /*turn off both LEDS*/
                LED_DEBUG(("LED: Pair OFF\n")) ;   

                if ( (lTime == 0 ) && ( lPatternComplete == FALSE ) )
   		        {
    	            /*ie we are switching off for 0 time - do not use the overide led as this results in a tiny blip*/
            	    LedsTurnOffLEDPair ( &lPattern , FALSE) ;
        	    }
   		        else
   	    	    {
            	    LedsTurnOffLEDPair ( &lPattern , TRUE) ;
            	}
			}
			else
			{
				/*signal that we are off even though we are not*/
			    theSink.theLEDTask->gActiveLEDS [ lPattern.pattern.LED_A ].OnOrOff  = FALSE ;                     
			}

            if (resetFollowingPending)
            {
                theSink.theLEDTask->gFollowing = FALSE;
            }
		}
      
       
        /*handle the completion of the pattern or send the next update message*/
        if (lPatternComplete)
        {
            LED_DEBUG(("LM : PatternComplete [%x][%x]  [%x][%x]\n" , theSink.theLEDTask->gActiveLEDS[lPattern.pattern.LED_B].Index, lLED->Index , theSink.theLEDTask->gActiveLEDS[lPattern.pattern.LED_B].Type , lLED->Type    )) ;
            /*set the type of indication for both LEDs as undefined as we are now indicating nothing*/
            if ( theSink.theLEDTask->gActiveLEDS[id].Type == IT_EventIndication )
            {
                      /*signal the completion of an event*/
                LedsSendEventComplete ( lPattern.StateOrEvent, TRUE ) ;
                    /*now complete the event, and indicate a new state if required*/        
                LedsEventComplete ( lLED , &theSink.theLEDTask->gActiveLEDS[lPattern.pattern.LED_B] ) ;
            }  
            else if (theSink.theLEDTask->gActiveLEDS[id].Type == IT_StateIndication )
            {
                /*then we have completed a state indication and the led pattern is now off*/    
                /*Indicate that we are now with LEDS disabled*/
               theSink.theLEDTask->gLEDSStateTimeout = TRUE ;
            }
            
            /* ensure leds are turned off when pattern completes as when using an alternating pattern
               leds are now left on to provide a better smoother transistion between colours */
            if ( lColour == LED_COL_LED_ALT )
            {
                LedsTurnOffLEDPair ( &lPattern , TRUE) ;
            }
        }
        else
        {   
            /*apply the filter in there is one  and schedule the next message to handle for this led pair*/
            lTime = LedsApplyFilterToTime ( lTime ) ;
            MessageSendLater (&theSink.theLEDTask->task , id , 0 , lTime ) ;
            LED_DEBUG(("LM : PatternNotComplete  Time=[%x] [%x][%x]  [%x][%x]\n" ,lTime, theSink.theLEDTask->gActiveLEDS[lPattern.pattern.LED_B].Index, lLED->Index , theSink.theLEDTask->gActiveLEDS[lPattern.pattern.LED_B].Type , lLED->Type    )) ;
        } 
        
    }
    else
    {
        /*DIMMING LED Update message */       
        PioSetDimState ( (id - DIM_MSG_BASE) );
    }
}


/****************************************************************************
NAME 
 LedsTurnOnAltLeds

DESCRIPTION
    Fn to turn on the LEDs with Alt LED colour pattern.
    
RETURNS
 void
*/
static void ledsTurnOnAltLeds(uint8 On_LedA, uint8 Off_LedB)
{
    /* PioSetLedPin will handle keeping common LEDs on if in a dimming case
       Must turn off before turning back on as the last request will override */
    PioSetLedPin ( Off_LedB , LED_OFF ) ;
    PioSetLedPin ( On_LedA , LED_ON );
}


/****************************************************************************
NAME 
 LMTurnOnLEDPair

DESCRIPTION
    Fn to turn on the LED associated with the pattern / LEDs depending upon the 
    colour 
    
RETURNS
 void
*/
static void LedsTurnOnLEDPair ( LEDPattern_t * pPattern , LEDActivity_t * pLED )
{
    LEDColour_t lColour = LedsGetPatternColour ( pPattern ) ;   
    
    /* to prevent excessive stack usage when compiled in native mode only perform one read and convert of these
       4 bit parameters */
    uint8 LedA = pPattern->pattern.LED_A; 
    uint8 LedB = pPattern->pattern.LED_B; 
    
    LED_DEBUG(("LM : TurnOnPair  " )) ;
    
    if (theSink.theLEDTask->gFollowing )
    {	 /*turn of the pair of leds (and dont use an overide LED */
        
        /* obtain the PIO to drive */
        uint16 lLED = theSink.theLEDTask->gFollowPin; 
                
        LedsTurnOffLEDPair ( pPattern , FALSE) ;
        
        /* check to ensure it was possible to retrieve PIO, the filter may have been cancelled */
        if(lLED <= SINK_NUM_LEDS)
        {
            /* set the LED specified in the follower filter */
            PioSetLedPin ( lLED , LED_ON );
        }
    }
    else
    {/*we are not following*/
            /*Turn on the LED enable pin*/    
        LedsSetEnablePin ( LED_ON )  ;

        switch (lColour )
        {
        case LED_COL_LED_A:
    
            LED_DEBUG(("LED: A ON[%x][%x]\n", LedA , LedB)) ;            
            if (LedA != LedB)
            {
                if(!isOverideFilterActive(LedB))
                {
                    PioSetLedPin ( LedB , LED_OFF )  ;
                }
            }
            PioSetLedPin ( LedA , LED_ON )  ;
        
        break;
        case LED_COL_LED_B:
    
            LED_DEBUG(("LED: B ON[%x][%x]\n", LedA , LedB)) ;
            if (LedA != LedB)
            {
                if(!isOverideFilterActive( LedA))
                {
                    PioSetLedPin ( LedA , LED_OFF )  ;
                }
            }
            PioSetLedPin ( LedB , LED_ON )  ;
            
        break;
        case LED_COL_LED_ALT:
                   
            if (pLED->NumFlashesComplete % 2 )
            {
                LED_DEBUG(("LED: A ALT On[%x],Off[%x]\n", LedB , LedA)) ;
                ledsTurnOnAltLeds(LedB, LedA);
            }
            else
            {
                LED_DEBUG(("LED: B ALT On[%x],Off[%x]\n", LedA , LedB)) ;
                ledsTurnOnAltLeds(LedA, LedB);
            }        
        break;
        case LED_COL_LED_BOTH:
    
            LED_DEBUG(("LED: AB Both[%x][%x]\n", LedA , LedB)) ;
            PioSetLedPin (  LedA , LED_ON )  ;
            PioSetLedPin (  LedB , LED_ON )  ;
        break;
        default:
            LED_DEBUG(("LM : ?Col\n")) ;
        break;
        }
    }
 
    /* only process the overide filter if not an alternating pattern or a permanently on pattern otherwise
       led will be turned off */
    if((lColour != LED_COL_LED_BOTH)&&(lColour != LED_COL_LED_ALT)&&(pPattern->pattern.NumFlashes))
    {
        /*handle an overide LED if there is one will also dealit is different to one of the pattern LEDS)*/
        if((!isOverideFilterActive(LedA)) || (!isOverideFilterActive(LedB)) )
        {
            LED_DEBUG(("LM : TurnOnPair - Handle Overide\n" )) ;
            LedsHandleOverideLED (   LED_OFF ) ;
        }
    }
    
    pLED->OnOrOff = TRUE ;
        
}


/****************************************************************************
NAME 
 LMTurnOffLEDPair

DESCRIPTION
    Fn to turn OFF the LEDs associated with the pattern
    
RETURNS
 void
*/
static void LedsTurnOffLEDPair ( LEDPattern_t * pPattern  , bool pUseOveride ) 
{
    LED_DEBUG(("LM : TurnOffPair \n" )) ;

    /*turn off both LEDS*/
    if(!isOverideFilterActive( pPattern->pattern.LED_A))
    {
        LED_DEBUG(("LM : TurnOffPair - OVR A%x \n", pPattern->pattern.LED_A )) ;
        PioSetLedPin ( pPattern->pattern.LED_A , LED_OFF )  ;
    }
    
    if(!isOverideFilterActive( pPattern->pattern.LED_B))
    {
        LED_DEBUG(("LM : TurnOffPair - OVR B %x \n", pPattern->pattern.LED_B )) ;
        PioSetLedPin ( pPattern->pattern.LED_B , LED_OFF )  ;
    }

    /* turn of follower led, if following */
    if((theSink.theLEDTask->gFollowing ) &&
	   (!isOverideFilterActive( theSink.theLEDTask->gFollowPin)))
    {
        /* set the LED specified in the follower filter */
        PioSetLedPin ( theSink.theLEDTask->gFollowPin , LED_OFF ) ;
    }

        /*handle an overide LED if we want to use one*/
    if ( pUseOveride )
    {
        LedsHandleOverideLED ( LED_ON ) ;
    }
    theSink.theLEDTask->gActiveLEDS [ pPattern->pattern.LED_A ].OnOrOff  = FALSE ;        
    
        /*Turn off the LED enable pin*/  

    LedsSetEnablePin ( LED_OFF )  ;
}


/****************************************************************************
NAME 
 LedsHandleOverideLED

DESCRIPTION
    Enables / diables any overide LEDS if there are some    
RETURNS
*/
static void LedsHandleOverideLED ( bool pOnOrOff ) 
{   
    uint16 lFilterIndex = 0 ;
    
    for (lFilterIndex = 0 ; lFilterIndex< LM_NUM_FILTER_EVENTS ; lFilterIndex++ )
    {
        if ( LedsIsFilterEnabled(lFilterIndex) )
        {
             if ( theSink.theLEDTask->pEventFilters[lFilterIndex].FilterType == OVERRIDE)
             {
                    /*Overide the Off LED with the Overide LED*/
                    LED_DEBUG(("LM: LEDOveride [%d] [%d]\n" , theSink.theLEDTask->pEventFilters[lFilterIndex].OverideLED , pOnOrOff)) ;    
                    PioSetLedPin ( theSink.theLEDTask->pEventFilters[lFilterIndex].OverideLED , pOnOrOff) ;   
             }    
        }
    }  
}


/****************************************************************************
NAME 
 LMGetPatternColour

DESCRIPTION
    Fn to determine the LEDColour_t of the LED pair we are currently playing
    takes into account whether or not a filter is currently active
    
RETURNS
 LEDColour_t
*/
static LEDColour_t LedsGetPatternColour ( const  LEDPattern_t * pPattern )
{
    uint16 lFilterIndex = 0 ;
        /*sort out the colour of the LED we are interested in*/
    LEDColour_t lColour = pPattern->pattern.Colour ;
   
    for (lFilterIndex = 0 ; lFilterIndex< LM_NUM_FILTER_EVENTS ; lFilterIndex++ )
    {
        if ( LedsIsFilterEnabled(lFilterIndex) )
        {
            if ( theSink.theLEDTask->pEventFilters[lFilterIndex].Colour != LED_COL_EITHER )
            {
                    /*Overide the Off LED with the Overide LED*/
                lColour = theSink.theLEDTask->pEventFilters[lFilterIndex].Colour;   
            } 
        }
    }
    return lColour ;
}


/****************************************************************************
NAME 
 LMApplyFilterToTime

DESCRIPTION
    Fn to change the callback time if a filter has been applied - if no filter is applied
    just returns the original time
    
RETURNS
 uint16 the callback time
*/
static uint16 LedsApplyFilterToTime ( uint16 pTime ) 
{
    uint16 lFilterIndex = 0 ;
    uint16 lTime = pTime ; 
    
    for (lFilterIndex = 0 ; lFilterIndex< LM_NUM_FILTER_EVENTS ; lFilterIndex++ )
    {
        if ( LedsIsFilterEnabled(lFilterIndex) )
        {
            LEDFilter_t *lEventFilter = &(theSink.theLEDTask->pEventFilters[lFilterIndex]);
            
            if ( lEventFilter->Speed )
            {
                if (lEventFilter->SpeedAction == SPEED_MULTIPLY)
                {
                    LED_DEBUG(("LED: FIL_MULT[%d]\n" , lEventFilter->Speed )) ;
                    lTime *= lEventFilter->Speed ;
                }
                else /*we want to divide*/
                {
                    if (lTime)
                    {
                       LED_DEBUG(("LED: FIL_DIV[%d]\n" , lEventFilter->Speed )) ;
                      lTime /= lEventFilter->Speed ;
                    }
                }
            }
        }
    }

    return lTime ;
}




/****************************************************************************
NAME 
 LEDManagerSendEventComplete

DESCRIPTION
    Sends a message to the main task thread to say that an event indication has been completed
    
    
RETURNS
 void
*/
void LedsSendEventComplete ( sinkEvents_t pEvent , bool pPatternCompleted )
{
    if ( (pEvent > EVENTS_MESSAGE_BASE) && (pEvent <= EVENTS_LAST_EVENT ) )
    {   
        LMEndMessage_t * lEventMessage = newDebugPanic( LMEndMessage_t );

        /*need to add the message containing the EventType here*/
        lEventMessage->Event = pEvent  ;
        lEventMessage->PatternCompleted =  pPatternCompleted ;
                        
        LED_DEBUG(("LM : lEvCmp[%x] [%x]\n",lEventMessage->Event , lEventMessage->PatternCompleted )) ;
            
        MessageSend ( &theSink.task , EventSysLEDEventComplete , lEventMessage ) ;
    }
}

/****************************************************************************
NAME 
 LedsEventComplete

DESCRIPTION
    signal that a given event indicatio has completed
RETURNS
 	void
*/
static void LedsEventComplete ( LEDActivity_t * pPrimaryLed , LEDActivity_t * pSecondaryLed ) 
{       
    pPrimaryLed->Type = IT_Undefined ;
    
    pSecondaryLed->Type = IT_Undefined ;
    
    theSink.theLEDTask->gCurrentlyIndicatingEvent = FALSE ;
}        
/****************************************************************************
NAME 
 LedsEnableFilter

DESCRIPTION
    enable / disable a given filter ID
RETURNS
 	void
*/
static void LedsEnableFilter ( uint16 pFilter , bool pEnable)
{
    uint32 lOldMask = LED_GETACTIVEFILTERS() ;
    
    if (pEnable)
    {
        /*to set*/
        LED_SETACTIVEFILTERS((lOldMask | ( 1UL << pFilter )));
        LED_DEBUG(("LED: EnF [%lx] [%lx] [%x]\n", lOldMask , LED_GETACTIVEFILTERS() , pFilter));
    }
    else
    {
        /*to unset*/
        LED_SETACTIVEFILTERS(lOldMask & ~( 1UL << pFilter ));
        LED_DEBUG(("LED: DisF [%lx] [%lx] [%x]\n", lOldMask , LED_GETACTIVEFILTERS() , pFilter));
    }
    
    /* Check if we should indicate state */
    if ((theSink.theLEDTask->pEventFilters[pFilter].OverideDisable) && (lOldMask != LED_GETACTIVEFILTERS()))
        LEDManagerIndicateState ( stateManagerGetState () ) ;                          
}

/****************************************************************************
NAME 
 LedsIsFilterEnabled

DESCRIPTION
    determine if a filter is enabled
RETURNS
 	bool - enabled or not
*/
static bool LedsIsFilterEnabled ( uint16 pFilter )
{
    bool lResult = FALSE ;
    
    if ( LED_GETACTIVEFILTERS() & ( 0x1UL << pFilter ) )
    {
        lResult = TRUE ;
    }
    
    return lResult ;
}

/****************************************************************************
NAME 
 LedsSetLedActivity

DESCRIPTION
    Sets a Led Activity to a known state
RETURNS
 void
*/
void LedsSetLedActivity ( LEDActivity_t * pLed , IndicationType_t pType , uint16 pIndex , uint16 pDimTime)
{
    
    
    pLed->Type               = pType ;
    pLed->Index              = pIndex ;
    pLed->DimTime            = pDimTime ;   
    LED_DEBUG(("LED[%d]\n" , pDimTime)) ; 
  
}
/****************************************************************************
NAME 
	LedsCheckFiltersForLEDFollower
DESCRIPTION
    determine if a follower is currently active
RETURNS
 	bool - active or not
*/
static bool LedsCheckFiltersForLEDFollower( void )
{
    uint16 lResult = FALSE ;    
    uint16 lFilterIndex = 0 ;
    
    for (lFilterIndex = 0 ; lFilterIndex< LM_NUM_FILTER_EVENTS ; lFilterIndex++ )
    {
        if ( LedsIsFilterEnabled(lFilterIndex) )
        {
            LEDFilter_t *lEventFilter = &(theSink.theLEDTask->pEventFilters[lFilterIndex]);
                
            /*if this filter defines a lefd follower*/
            if ( lEventFilter->FilterType == FOLLOW)
            {
                theSink.theLEDTask->gFollowPin = lEventFilter->OverideLED;
                lResult = TRUE ;
            }    
        }
    }
    return lResult ;
}
/****************************************************************************
NAME 
	LedsGetLedFollowerRepeatTimeLeft
DESCRIPTION
    calculate the new repeat time based upon the follower led delay and the normal repeat time
RETURNS
 	uint16 - updated repeat time to use
*/
static uint16 LedsGetLedFollowerRepeatTimeLeft( LEDPattern_t * pPattern) 
{
    uint16 lTime = LED_SCALE_REPEAT_TIME(pPattern->pattern.RepeatTime) ;
    uint16 lPatternTime = ( ( LED_SCALE_ON_OFF_TIME(pPattern->pattern.OnTime)  *  pPattern->pattern.NumFlashes) + 
                            ( LED_SCALE_ON_OFF_TIME( pPattern->pattern.OffTime) * (pPattern->pattern.NumFlashes - 1 ) )   +
                            ( LedsGetLedFollowerStartDelay() ) ) ;
                            
    if(lPatternTime < lTime )
    {
        lTime = lTime - lPatternTime ;
        LED_DEBUG(("LED: FOllower Rpt [%d] = [%d] - [%d]\n " , lTime , LED_SCALE_REPEAT_TIME(pPattern->pattern.RepeatTime) , lPatternTime)) ;
    }
    
    return lTime ;        
}
/****************************************************************************
NAME 
	LedsGetLedFollowerStartDelay
DESCRIPTION
    get the delay associated with a follower led pin
RETURNS
 	uint16 - delay to use for the follower
*/             
static uint16 LedsGetLedFollowerStartDelay( void )
{
    uint16 lDelay = 0 ;
    uint16 lFilterIndex =0 ;    
    
    for (lFilterIndex = 0 ; lFilterIndex< LM_NUM_FILTER_EVENTS ; lFilterIndex++ )
    {
        if ( LedsIsFilterEnabled(lFilterIndex) )
        {
                /*if this filter defines a lefd follower*/
            if ( theSink.theLEDTask->pEventFilters[lFilterIndex].FilterType == FOLLOW)
            {		 /*the led to use to follow with*/
                LED_DEBUG(("LM: LEDFollower Led[%d] Delay[%d]\n" , theSink.theLEDTask->pEventFilters[lFilterIndex].OverideLED ,
                                                                   FILTER_SCALE_DELAY_TIME(theSink.theLEDTask->pEventFilters[lFilterIndex].FollowerLEDDelay))) ;    
                lDelay = FILTER_SCALE_DELAY_TIME(theSink.theLEDTask->pEventFilters[lFilterIndex].FollowerLEDDelay) * 50 ;
            }    
        }
    }

    return lDelay ;
}

/****************************************************************************
NAME 
 LedsResetAllLeds

DESCRIPTION
    resets all led pins to off and cancels all led and state indications
RETURNS
 void
*/
void LedsResetAllLeds ( void ) 
{   
		/*Cancel all event indications*/ 
    uint16 lLoop = 0;
    
    for ( lLoop = 0 ; lLoop < SINK_NUM_LEDS ; lLoop ++ )
    {
        if (theSink.theLEDTask->gActiveLEDS[lLoop].Type == IT_EventIndication)
        {
            MessageCancelAll ( &theSink.theLEDTask->task, lLoop ) ; 
            theSink.theLEDTask->gActiveLEDS[lLoop].Type =  IT_Undefined ;
            
            LED_DEBUG(("LED: CancelEventInd[%x]\n" , lLoop)) ;
            LedsTurnOffLEDPair( &theSink.theLEDTask->pEventPatterns[theSink.theLEDTask->gActiveLEDS[lLoop].Index] ,TRUE) ;
        }
    }
    	/*cancel all state indications*/
    LedsIndicateNoState ()  ;   
}


/****************************************************************************
NAME 
 LedsSetEnablePin

DESCRIPTION
    if configured sets a pio as a led enable pin
RETURNS
 void
*/
static void LedsSetEnablePin ( bool pOnOrOff ) 
{
    if ( theSink.conf6->PIOIO.pio_outputs.LedEnablePIO < 0xFF ) 
    {
        PioSetLedPin ( theSink.conf6->PIOIO.pio_outputs.LedEnablePIO , pOnOrOff ) ;
    }
}


/****************************************************************************
NAME 
 isOverideFilterActive

DESCRIPTION
    determine if an overide filter is currently active and driving one of the
    leds in which case return TRUE to prevent it being turned off to display 
    another pattern, allows solid red with flashing blue with no interuption in
    red for example.
RETURNS
    true or false
*/
bool isOverideFilterActive ( uint8 Led ) 
{  
    uint16 lFilterIndex = 0 ;
 
    /* determine whether feature to make an overide filter drive the led permanently regardless of 
       any intended flash pattern for that led is enabled */
    if(theSink.features.OverideFilterPermanentlyOn)
    {
        /* permanent overide filter led indication is enabled, this means that an active override
           filter will drive its configured led regardless of any other patterns configured for that
           led */
        for (lFilterIndex = 0 ; lFilterIndex< LM_NUM_FILTER_EVENTS ; lFilterIndex++ )
        {
            /* look for any active filters */
            if ( LedsIsFilterEnabled(lFilterIndex) )
            {
                /* if this is an overide filter driving an led check to see if the passed in LED
                   requires that this led be turned off, if it does then stop the led being turned off
                   otherwise allow it to be turned off as usual */
                 if ( theSink.theLEDTask->pEventFilters[lFilterIndex].FilterType == OVERRIDE)
                 {
                    /* if overide led is active and is driving the passed in led stop this led being turned off */
                    if ((theSink.theLEDTask->pEventFilters[lFilterIndex].OverideLED)&&
                        (theSink.theLEDTask->pEventFilters[lFilterIndex].OverideLED == Led))
                    {
                        return TRUE;                    
                    }
                }    
            }
        }  
    }
    /* permanent overide filter led drive is diabled so allow led pattern to be indicated */
    else
    {
        return FALSE;
    }
    
    /* default case whereby led can be driven normally */
    return FALSE;    
}


/****************************************************************************
NAME 
    LedsIndicateError

DESCRIPTION
    Indicates a fatal application error by flashing each LED in turn the 
    number of times of the specified error id.
    This function never returns. Timers etc. can't be used as we don't want 
    to return to the message loop.

RETURNS
    void
*/
extern uint32 get_milli_time(void); /* VmgetClock() not on this branch yet */

void LedsIndicateError ( const uint16 errId ) 
{
    led_id error_leds[] = ERROR_LEDS;

    unsigned    led_idx;
    led_id      led;
    
    /* init leds */
#ifdef HYDRACORE_TODO
    for (led_idx = 0; led_idx < ARRAY_DIM(error_leds); led_idx++)
#else
    for (led_idx = 0; led_idx <= LED_2; led_idx++)
#endif /* HYDRACORE_TODO */       
    {          
        led = error_leds[led_idx];

        SinkLedConfigure(led, LED_DUTY_CYCLE, 0x0);
        SinkLedConfigure(led, LED_FLASH_ENABLE, 0);
        SinkLedConfigure(led, LED_ENABLE, 1);
    }

    /* Never leave this function */
    while (TRUE)
    {
#ifdef HYDRACORE_TODO
        for (led_idx = 0; led_idx < ARRAY_DIM(error_leds); led_idx++)
#else
        for (led_idx = 0; led_idx <= LED_2; led_idx++)
#endif /* HYDRACORE_TODO */
        {
            uint16 flashCount = 0;
            led = error_leds[led_idx];
            /* Flash the number of times corresponding to the config id error */
            for(flashCount = 0; flashCount < errId; flashCount++)
            {
#ifdef HYDRACORE_TODO
                uint32 clock;
#else
                uint16 waitCounter;
#endif /* HYDRACORE_TODO */
                
                SinkLedConfigure(led, LED_DUTY_CYCLE, 0xFFF);

                /* Use a delay loop checking time.
                 * Ideally there would be a sleep function so that battery doesn't go 
                 * pffft, but this is an error case.
                 * UNLIKELY TO BE APPROPRIATE ELSEWHERE IN CODE
                 */
#ifdef HYDRACORE_TODO
                clock = get_milli_time();
                while ((get_milli_time() - clock) < ERROR_ON_TIME_MS);
#else
                {
                    waitCounter = 0;
                    while (waitCounter < 0x09FF)
                    {
                        waitCounter++;              
                    }
                }                
#endif /* HYDRACORE_TODO */
               
                SinkLedConfigure(led, LED_DUTY_CYCLE, 0);
                
#ifdef HYDRACORE_TODO
                clock = get_milli_time();
                while ((get_milli_time() - clock) < ERROR_OFF_TIME_MS);
#else
                {
                    waitCounter = 0;
                    while (waitCounter < 0x0FFF)
                    {
                        waitCounter++;              
                    }
                }
#endif /* HYDRACORE_TODO */
            }
        }
    }
}

