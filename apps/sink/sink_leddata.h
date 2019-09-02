
/****************************************************************************
Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_leddata.h
    
DESCRIPTION
    data structures  /defines for use with the LED module
    
*/
#ifndef SINK_LED_DATA_H
#define SINK_LED_DATA_H

#include "sink_events.h"
#include "sink_states.h"

/****************************************************************************
Types
*/
    /* before increasing these note they effect memory usage and 
       are carefully optimised for VM slot availability
       LED state masks will also need enlarging */

/*the number of LEDS (including pin outputs) that we support*/
#define SINK_NUM_LEDS (16)
/*the number of HW LED pins we support */
#define SINK_NUM_HW_LEDS (3)
#define NO_STATE_OR_EVENT 0xff

/* Macros for the max number of LED events and filter patterns, capped at 20 each */
#define LM_MAX_NUM_PATTERNS ((uint16)(theSink.theLEDTask->gEventPatternsAllocated>20 ? 20:theSink.theLEDTask->gEventPatternsAllocated))  
#define LM_NUM_FILTER_EVENTS ((uint16)(theSink.theLEDTask->gLMNumFiltersUsed>20 ? 20:theSink.theLEDTask->gLMNumFiltersUsed))  


typedef enum LEDSpeedActionTag
{
    SPEED_MULTIPLY = 0,
    SPEED_DIVIDE  
}LEDSpeedActionTag ;

typedef enum LEDColTag
{
    LED_COL_EITHER ,
    LED_COL_LED_A ,
    LED_COL_LED_B ,
    LED_COL_LED_ALT ,    /*Alternate the colours*/
    LED_COL_LED_BOTH    /*use Both LEDS*/
}LEDColour_t ;

/* The order of values in the LEDFilterType_t shouldn't be changed */
typedef enum LEDFilterTypeTag
{
    DISABLED = 0,
    CANCEL ,
    SPEED ,
    OVERRIDE, 
    COLOUR ,
    FOLLOW
}LEDFilterType_t ;

typedef struct LedFilterTag
{
    uint16              Event;      /*The event to action the filter upon*/    
    
    unsigned            FilterType:3;
    unsigned            Colour:3 ;     /*Force LED to this colour pattern no matter what is defined in the state pattern*/    
    unsigned            FilterToCancel:5;
    unsigned            OverideLED:4;
    unsigned            OverideDisable:1 ; /* overide LED disable flag when filter active */

    unsigned            dummy:3;
    unsigned            FollowerLEDDelay:4 ; /*the Delay before the following pattern starts*/ /*50ms (0 - 750ms)*/
    unsigned            SpeedAction:1 ;/*which action to perform on the multiple  multiply or divide */
    unsigned            Speed:8 ;      /*speed multiple to apply - 0 =  no speed multiple*/    
}LEDFilter_t ;


    /*The LED state pattern type. This type is also used as pattern information for LED events */
typedef struct LEDPatternStateTag
{    
    unsigned          state:8;      /*state id or NULL if this is part of an event pattern*/
    unsigned          OnTime:8;     /*ms*/
    
    unsigned          OffTime:8;    /*ms*/
    unsigned          RepeatTime:8; /*ms*/   
    
    unsigned          DimTime:8;    /*Time to Dim this LED*/       
    unsigned          TimeOut:8;    /*number of repeats*/
    
    unsigned          NumFlashes:4; /*how many flashes in the pattern*/          
    unsigned          LED_A:4;      /*default first LED to use*/
    unsigned          LED_B:4;      /*second LED to use*/     
    unsigned          OverideDisable:1; /* overide LED disable flag for this pattern */
    unsigned          Colour:3;     /*which of the LEDS to use*/     
}LEDPatternState_t ;

    /*the led pattern type */
typedef struct LEDPatternStateEventTag
{
    uint16            StateOrEvent; /*State or Event ID*/
    
    LEDPatternState_t pattern;      /*LED pattern associated with this event*/
}LEDPattern_t ;


typedef enum IndicationTypeTag
{
    IT_Undefined = 0 ,
    IT_StateIndication,
    IT_EventIndication    
    
}IndicationType_t ;

    /*the information required for a LED to be updated*/
typedef struct LEDActivityTag
{  
    unsigned         Index:7; /*what this led is displaying*/
    unsigned         NumFlashesComplete:8 ; /*how far through the pattern we currently are*/        
    unsigned         OnOrOff:1 ;
    
    unsigned         FilterIndex:4 ;/*the filter curently attached to this LED (0-15)*/    
    unsigned         Type:2 ; /*what this LED is displaying*/
    unsigned         NumRepeatsComplete:10;
        /*dimming*/
    unsigned         DimState:7  ; /*how far through the dim pattern we are*/
    unsigned         DimDir:1    ; /*which direction we are going*/
    unsigned         DimTime:8   ;
    
}LEDActivity_t ;


    /*the event message sent on completion of an event */
typedef struct 
{
    uint16 Event ;  
    bool PatternCompleted ;
    
} LMEndMessage_t;
    

typedef sinkEvents_t LEDEventQueue_t[4];


/*the tricolour led information*/
typedef struct PioTriColLedsTag
{
	unsigned TriCol_a:4;
	unsigned TriCol_b:4;
	unsigned TriCol_c:4;
	unsigned Unused1 :4;
}PioTriColLeds_t ;

/* LED information structure */
typedef struct LEDInfo
{
    unsigned led:8; /* LED and PIOs are uint16, though we aren't going to have more than 256 for each */
    unsigned OnOff:1; /* On Off state */
    unsigned is_led:1; /* 1:LED, 0:PIO */
    unsigned unused:6;
    
    unsigned current_dim:12; /* current dim value of the LED if it is an LED */
    unsigned unused2:4;
}LEDInfo_t ;

/* pPIO information structure */
typedef struct LEDpPIOInfo
{
	LEDInfo_t led1;
	LEDInfo_t led2;
	bool is_ledpair; /* TRUE if pPIO is a pair of pPIOs */
}LEDpPIOInfo_t ;

/* LED State information structure */
typedef struct LEDState
{
    unsigned  current_dim:12; /* current dim value to compare with a new LED pattern or ramp direction */
    unsigned  OnOff:1; /* State of the LED, whether it is on off (or ramping up or down) */
    unsigned  unused:3;
}LEDState_t ;

   /*The LED task type*/
typedef struct
{
 	TaskData                task;
    LEDPatternState_t       gStatePatterns [SINK_NUM_STATES]; /*the array of pointers to the state patterns */
    LEDPattern_t*           pEventPatterns; /*pointer to array of LED patterns by the configurable event */
    LEDFilter_t*            pEventFilters; /* pointer to array of LED Filter patterns */
    
    LEDActivity_t           gActiveLEDS[SINK_NUM_LEDS]; /* the array of LED Activities*/
    LEDState_t              gLEDState[SINK_NUM_HW_LEDS]; /* array for LED hardware state information */
    
    unsigned                gStatePatternsAllocated:5;
    unsigned                gEventPatternsAllocated:5; 
    unsigned                gLMNumFiltersUsed:5 ;
    unsigned                gLEDSStateTimeout:1 ; /*this is set to true if a state pattern has completed - reset if new event occurs*/
    
    unsigned                unused:3 ;
    unsigned                gLEDSEnabled:1 ;      /*global LED overide  - event drivedn to enable / disable all LED Indications*/  
    unsigned                gLEDSSuspend:1;       /*LED indications suspended until this bit is cleared*/
    unsigned                gCurrentlyIndicatingEvent:1; /*if we are currently indicating an event*/
    unsigned 				gFollowing:1 ; /* do we currently have a follower active*/
    unsigned                gStateCanOverideDisable:1;    
    
    unsigned                gFollowPin:4;    
	unsigned                gTheActiveFiltersH :4; /*Mask of Filters Active - High nibble */   
        
    unsigned                gTheActiveFiltersL:16; /*Mask of Filters Active - Low word */
    
    LEDEventQueue_t         Queue ;
    PioTriColLeds_t         gTriColLeds ;
    LEDpPIOInfo_t           gpPIOInfo ; /* structure to be used by functions that require translation of pPIO to HW */
} LedTaskData;  

#define LED_SCALE_ON_OFF_TIME(x) (uint16)(((x) * 10) << theSink.features.LedTimeMultiplier )
#define LED_SCALE_REPEAT_TIME(x) (uint16)(((x) * 50) << theSink.features.LedTimeMultiplier )
#define FILTER_SCALE_DELAY_TIME(x) (uint16)((x) << theSink.features.LedTimeMultiplier )

#define LED_GETACTIVEFILTERS() (uint32)(((uint32)theSink.theLEDTask->gTheActiveFiltersH<<16) | theSink.theLEDTask->gTheActiveFiltersL)
#define LED_SETACTIVEFILTERS(x) {theSink.theLEDTask->gTheActiveFiltersH = ((uint32)(x)>>16) & 0xF ;\
                                 theSink.theLEDTask->gTheActiveFiltersL = (x) & 0xffff;}

#endif

