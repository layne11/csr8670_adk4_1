/*
Copyright (c) 2004 - 2016 Qualcomm Technologies International, Ltd.
*/

/*!
@file
@ingroup sink_app
@brief   
    Configuration manager for the sink device - responsible for extracting user information out of the
    PSKEYs and initialising the configurable nature of the devices' components

*/
#ifndef SINK_CONFIG_MANAGER_H
#define SINK_CONFIG_MANAGER_H

#include <stdlib.h>
#include <pblock.h>
#include <audio.h>
#include <hfp.h>
#include <vmtypes.h>

#include "sink_volume.h"

/*******************************************************************************************************/
/* REMEMBER TO UPDATE THESE VALUES WHEN MAKING CONFIGURATION CHANGES THAT WILL REQUIRE AN UPDATE TO THE
   CONFIGURATION TOOL XML FILE TO MAINTAIN FORWARDS AND BACKWARDS COMPATIBILITY                        */
/*******************************************************************************************************/
#define AUDIO_SINK_PRODUCT   0
#define PRODUCT_VERSION      AUDIO_SINK_PRODUCT
#define CONFIG_SET_VERSION   46

/*******************************************************************************************************/
/* REMEMBER TO UPDATE THESE VALUES WHEN MAKING CONFIGURATION CHANGES THAT WILL REQUIRE AN UPDATE TO THE
   CONFIGURATION TOOL XML FILE TO MAINTAIN FORWARDS AND BACKWARDS COMPATIBILITY                        */
/*******************************************************************************************************/


/* Conversion macros */
#define MAKEWORD(a, b)      ((uint16)(((uint8)((uint32)(a) & 0xff)) | ((uint16)((uint8)((uint32)(b) & 0xff))) << 8))
#define MAKELONG(a, b)      ((uint32)(((uint16)((uint32)(a) & 0xffff)) | ((uint32)((uint16)((uint32)(b) & 0xffff))) << 16))
#define LOWORD(l)           ((uint16)((uint32)(l) & 0xffff))
#define HIWORD(l)           ((uint16)((uint32)(l) >> 16))
#define LOBYTE(w)           ((uint8)((uint32)(w) & 0xff))
#define HIBYTE(w)           ((uint8)((uint32)(w) >> 8))

/* DSP keys */
#define CONFIG_DSP_BASE  (50)
#define CONFIG_DSP(x)    (CONFIG_DSP_BASE + x)
#define CONFIG_DSP_SESSION_KEY     (CONFIG_DSP(49))
#define CONFIG_DSP_SESSION_SIZE    (48) /* allow 4 x entry of 12 words currently used by CVC */

/* Persistent store key allocation  */
#define CONFIG_BASE  (0)


/***********************************************************************/
/***********************************************************************/
/* ***** do not alter order or insert gaps as device will panic ***** */
/***********************************************************************/
/***********************************************************************/
enum
{
 	CONFIG_SESSION_DATA     				  = CONFIG_BASE,
 	CONFIG_SUBWOOFER     				      = 1,
 	CONFIG_PHONE_NUMBER     		          = 2,
 	CONFIG_FM_FREQUENCY_STORE            	  = 3,
    CONFIG_IR_REMOTE_CONTROL_LEARNED_CODES    = 4,
 	CONFIG_SQIF_PARTITIONS                 	  = 5,
 	CONFIG_FEATURE_BLOCK     				  = 6,
	CONFIG_LENGTHS                    		  = 7,
	CONFIG_TIMEOUTS                 		  = 8,
 	CONFIG_DEVICE_ID         				  = 9,
	CONFIG_BATTERY   			              = 10,
 	CONFIG_RADIO      				          = 11,
 	CONFIG_VOLUME_CONTROL                  	  = 12,
 	CONFIG_HFP_INIT		     				  = 13,
    CONFIG_ADDITIONAL_HFP_SUPPORTED_FEATURES  = 14,
    CONFIG_RSSI_PAIRING             		  = 15,
    CONFIG_PIO                  		      = 16,
    CONFIG_BUTTON                             = 17,
    CONFIG_BUTTON_TRANSLATION   			  = 18,
 	CONFIG_BUTTON_PATTERN_CONFIG			  = 19,
 	CONFIG_LED_FILTERS      				  = 20,
 	CONFIG_LED_STATES      					  = 21,
 	CONFIG_TRI_COL_LEDS        				  = 22,
	CONFIG_LED_EVENTS      					  = 23,
 	CONFIG_EVENTS_A      					  = 24,
 	CONFIG_EVENTS_B    						  = 25,
 	CONFIG_EVENTS_C               			  = 26,
	CONFIG_TONES                     		  = 27,
 	CONFIG_USER_TONES	     				  = 28,
    CONFIG_ANC                                = 29,
 	CONFIG_AUDIO_PROMPTS       				  = 30,
 	CONFIG_SSR_PARAMS              			  = 31,
    CONFIG_USB_CONFIG   					  = 32,
    CONFIG_FM_CONFIG                		  = 33,
    CONFIG_AT_COMMANDS                        = 34,
    CONFIG_I2S_INIT_CONFIGURATION             = 35,
    CONFIG_I2S_INIT_DATA                      = 36,
	CONFIG_AUDIO_ROUTING 					  = 37,
    CONFIG_INPUT_MANAGER                      = 38,
    CONFIG_BLE_REMOTE_CONTROL                 = 39,
    CONFIG_IR_REMOTE_CONTROL                  = 40,
    CONFIG_PMU_MONITOR_CONFIG                 = 41,
    CONFIG_PEER_DEVICE_CUSTOM_UUID            = 42,
    CONFIG_MULTI_CHANNEL_AUDIO                = 43,
    CONFIG_ANC_SESSION_DATA                   = 44,
	CONFIG_RESERVED_45	                      = 45,
    CONFIG_RESERVED_46                        = 46,
    CONFIG_DEFRAG_TEST                        = 47,
    CONFIG_PERMANENT_PAIRING                  = 48,
    CONFIG_SOFTWARE_VERSION_ID                = 49,
    /* NOTE for PSKEY_USR50 to PSKEY_USR99 usage:-

     * The PS keys are divided into three blocks of equal size and function
     *- User configuration data (0 to 49).
     *- DSP configuration (50 to 99).
     *- VM Connection Library data (100 to 149) 
     *- New User configuration data (50 to 99) must initialise with the value 150 onwards.
     */
    CONFIG_BLE_DIS_MANUFACTURER_NAME   = 150
};

/* Macros related to the User PEQ band parameters*/
#define NUM_USER_EQ_BANDS 5
#define USER_EQ_BAND_PARAMS_SIZE 7       /* bytes*/
#define USER_EQ_PARAM_PRE_GAIN_SIZE 2 /* bytes*/

/* Byte offsets of the User EQ bank - Pre Gain field*/
#define PRE_GAIN_LO_OFFSET 0
#define PRE_GAIN_HI_OFFSET 1


/*Byte offsets used in mapping the parameters defined in the user_eq_band_t structure 
 * into a uint8 array.
 */
typedef enum
{
    BAND_FILTER_OFFSET,
    BAND_FREQ_LO_OFFSET,
    BAND_FREQ_HI_OFFSET,
    BAND_GAIN_LO_OFFSET,
    BAND_GAIN_HI_OFFSET,
    BAND_Q_LO_OFFSET,
    BAND_Q_HI_OFFSET
}user_eq_band_param_offset_t;


/* User defined EQ bank settings must be persistent, hence will be stored as part of PSKEY_USR0 */
typedef enum{
    BYPASS,
    FIRST_ORDER_LOW_PASS,
    FIRST_ORDER_HIGH_PASS,
    FIRST_ORDER_ALL_PASS,
    FIRST_ORDER_LOW_SHELF,
    FIRST_ORDER_HIGH_SHELF,
    FIRST_ORDER_TILT,
    SECOND_ORDER_LOW_PASS,
    SECOND_ORDER_HIGH_PASS,
    SECOND_ORDER_ALL_PASS,
    SECOND_ORDER_LOW_SHELF,
    SECOND_ORDER_HIGH_SHELF,
    SECOND_ORDER_TILT,
    PARAMETRIC_EQ,

    FORCE_ENUM_TO_MIN_16BIT(eq_filter)
}user_eq_filter_type_t;

typedef struct{
    user_eq_filter_type_t filter;
    uint16 freq;
    uint16 gain;
    uint16 Q;
}user_eq_band_t;

typedef struct{
    uint16 preGain;
    user_eq_band_t bands[NUM_USER_EQ_BANDS];
}user_eq_bank_t;

/* Persistant store vol orientation definition */
typedef struct
{
    /* word 1 */
    BITFIELD    vol_orientation:1 ;
    BITFIELD    led_enable:1 ;
	BITFIELD    audio_prompt_language:4;
    BITFIELD    multipoint_enable:1;
    BITFIELD    peer_link_reserved:1;
    BITFIELD    lbipm_enable:1;
    BITFIELD    ble_role:2 ;
    BITFIELD    audio_prompt_enable:1;
    BITFIELD    ssr_enabled:1;
    BITFIELD    audio_source:3 ;
    /* word 2 */
    BITFIELD    audio_enhancements:16;
    /* word 3 */
    BITFIELD    fm_frequency:16;
    /* word 4 */
    volume_info analog_volume;
    /* word 6 */
    volume_info spdif_volume;
    /* word 8 */
    volume_info usb_volume;
    /* word 10 */
    volume_info fm_volume;
    /* words 12 - 32 */
    user_eq_bank_t user_eq;

}session_data_type;

    /* Used to use pblock_key from pblock.h
     * but that should really be private.
     * Mostly unused anyway !
     */
typedef struct
{
    uint16      key;
    unsigned    len:8;
    unsigned    cur_len:8;
    uint16*     cache;
} pblock_info;


typedef struct
{
    pblock_info key;
    uint16      cache[CONFIG_DSP_SESSION_SIZE];
} dsp_data_type;

#define MAX_EVENTS ( EVENTS_MAX_EVENTS )
#define MAX_STATES (SINK_NUM_STATES)
#define MAX_LED_EVENTS (LM_MAX_NUM_PATTERNS)
#define MAX_LED_STATES (SINK_NUM_STATES)
#define MAX_LED_FILTERS (LM_NUM_FILTER_EVENTS)

/* LED patterns */
typedef struct
{
	uint16   event ;
	BITFIELD dummy:8 ;
	BITFIELD tone:8 ;
}tone_config_type ;

typedef struct
{
    uint16   event;
	BITFIELD unused:8 ;
	BITFIELD prompt_id:8 ;
    BITFIELD cancel_queue_play_immediate:1 ;
    BITFIELD sco_block:1;
    BITFIELD state_mask:14 ;
}audio_prompts_config_type;

#define MAX_VP_SAMPLES (15)

typedef struct
{
    uint16 event;
    uint16 pattern[6];
}button_pattern_config_type ;

typedef struct
{
    BITFIELD key_size:8;
    BITFIELD key_minimum:8;
} defrag_config;

typedef struct
{
    BITFIELD    input_manager_lookup_size:8;    /* 0-255 configurable entries */
    BITFIELD    ble_remote_lookup_size:4;       /* 0-16 inputs are supported */
    BITFIELD    ir_remote_lookup_size:4;        /* 0-16 inputs are supported */
} input_config_size;

typedef struct
{
    uint16              pdl_size;               /* DO NOT re-order this entry; pdl_size MUST be the first entry in this data structure */
    uint16              num_audio_prompt_events;
	uint16              num_audio_prompt_languages;
	uint16              no_led_filter;
	uint16              no_led_states;
	uint16              no_led_events;
	uint16              no_tones;
    uint16              num_audio_prompts;
    uint16              userTonesLength;
    uint16              size_at_commands;
    defrag_config       defrag;
    input_config_size   input_manager_size;
}lengths_config_type ;

#define SINK_DEVICE_ID_STRICT_SIZE     (sizeof(uint16) * 4)
#define SINK_DEVICE_ID_SW_VERSION_SIZE 4
typedef struct
{
    uint16 vendor_id_source;
    uint16 vendor_id;
    uint16 product_id;
    uint16 bcd_version;
    uint16 sw_version[SINK_DEVICE_ID_SW_VERSION_SIZE];   /* Original software version number, which is not part of the Device ID spec */
} sink_device_id;

typedef struct
{
    uint16 threshold ;   /* threshold (16 bit fractional value - aligned to MSB in DSP) */
    uint16 trigger_time; /* trigger time in seconds (16 bit int) */
} silence_detect_settings;

/* Pairing timeout action */
enum
{
 	PAIRTIMEOUT_CONNECTABLE   		= 0,
 	PAIRTIMEOUT_POWER_OFF     		= 1,
 	PAIRTIMEOUT_POWER_OFF_IF_NO_PDL = 2
};


/****************************************************************************
NAME
 	configManagerInit

DESCRIPTION
 	Initialises all subcomponents in order. If the full_init is set to false then only some of the
 	subcomponents are initialized which are required to display the LED pattern on EventUsrEnterDFUMode event.

RETURNS
 	void

*/
void configManagerInit(  bool full_init);


/****************************************************************************
NAME 
  	configManagerInitFeatures

DESCRIPTION
  	Read and configure the system features from PS

RETURNS
  	void
    
*/
void configManagerInitFeatures( void );


/****************************************************************************
NAME
  	configManagerInitMemory

DESCRIPTION
  	Init static size memory blocks that are required early in the boot sequence

RETURNS
  	void

*/

void configManagerInitMemory( void ) ;

/****************************************************************************
NAME
 	configManagerPioMap

DESCRIPTION
    Read in PIO config

RETURNS
 	void
*/
void configManagerPioMap(void);

/****************************************************************************
NAME
 	void configManagerReadI2SConfiguration(void)

    DESCRIPTION
    Gets the I2S pskey config and any associated I2C data packets

RETURNS
 	void

*/
void configManagerReadI2SConfiguration(void);


/****************************************************************************
NAME
 	configManagerHFP_SupportedFeatures

DESCRIPTION
    Gets the 1.5 Supported features set from PS - exposed as this needs to be done prior to a HFPInit()

RETURNS
 	void
*/
void configManagerHFP_SupportedFeatures( void ) ;

/****************************************************************************
NAME
 	configManagerHFP_Init

DESCRIPTION
    Gets the HFP initialisation parameters from PS

RETURNS
 	void
*/
void configManagerHFP_Init( hfp_init_params * hfp_params );

/****************************************************************************
NAME
 	configManagerEnableMultipoint

DESCRIPTION
    Enable or disable multipoint

RETURNS
 	void
*/
void configManagerEnableMultipoint(bool enable);

/****************************************************************************
NAME
 	configManagerWriteSessionData

DESCRIPTION
    Stores the persistent session data across power cycles.
	This includes information like volume button orientation
	Audio Prompt language etc.

RETURNS
 	void
*/
void configManagerWriteSessionData( void ) ;


/****************************************************************************
NAME
     configManagerReadSessionData

DESCRIPTION
    Reads the persistent session data after power cycles.
	This includes information like volume button orientation
	Audio Prompt language. last requested source etc.

RETURNS
 	void
*/
void configManagerReadSessionData( void );


/****************************************************************************
NAME
 	configManagerWriteDspData

*/
void configManagerWriteDspData( void );

/****************************************************************************
NAME
  	configManagerFillPs

DESCRIPTION
  	Fill PS to the point defrag is required (for testing only)

RETURNS
  	void
*/
void configManagerFillPs(void);

/****************************************************************************
NAME
    configManagerDefragIfRequired

DESCRIPTION
    Check if PS Defrag is required, if it is then flood PS to force a
    PS Defragment operation on next reboot.

RETURNS
    void
*/
void configManagerDefragIfRequired(void);

/****************************************************************************
NAME
  	configManagerRestoreDefaults

DESCRIPTION
    Restores default PSKEY settings.
	This function restores the following:
		1. CONFIG_SESSION_DATA
		2. CONFIG_PHONE_NUMBER
		3. Clears the paired device list
		4. Enables the LEDs

RETURNS
  	void
*/

void configManagerRestoreDefaults( void ) ;

/****************************************************************************
NAME
  	configManagerSqifPartitionsInit

DESCRIPTION
    Reads the PSKEY containing the SQIF partition configuration.

RETURNS
  	void
*/
#if defined ENABLE_SQIFVP || defined ENABLE_GAIA
void configManagerSqifPartitionsInit( void );
#endif


/****************************************************************************
NAME
  	configManagerGetSubwooferConfig

DESCRIPTION
    Reads the PSKEY containing the Subwoofer configuration

RETURNS
  	TRUE if configuration data was retrieved from CONFIG_SUBWOOFER
*/
#ifdef ENABLE_SUBWOOFER
bool configManagerGetSubwooferConfig( void );
#endif


/****************************************************************************
NAME
  	configManagerWriteSubwooferBdaddr

DESCRIPTION
    Writes the Subwoofer BDADDR to PSKEY CONFIG_SUBWOOFER

RETURNS
    True if data was written to they PSKEY

NOTE
    This function will not check the value of the BDADDR, if it is zero set,
    this function will write zero to the PS store.
*/
#ifdef ENABLE_SUBWOOFER
bool configManagerWriteSubwooferBdaddr( const bdaddr * addr );
#endif

/****************************************************************************
NAME
  	configManagerInputManagerInit

DESCRIPTION
    Reads the PSKEY containing the Input Manager configuration

RETURNS
  	void
*/
#if defined(ENABLE_IR_REMOTE) || (defined(GATT_ENABLED) && defined(GATT_HID_CLIENT))
bool configManagerInputManagerInit( void );
#endif

/****************************************************************************
NAME
  	configManagerIrRemoteInit

DESCRIPTION
    Reads the PSKEY containing the IR Remote Lookup Table

RETURNS
  	void
*/
#ifdef ENABLE_IR_REMOTE
void configManagerIrRemoteControlInit( void );
#endif

/****************************************************************************
NAME
  	configManagerAudioRouting

DESCRIPTION
    Reads the PSKEY containing the audio routing information

RETURNS
  	void
*/
void configManagerAudioRouting( void );

/****************************************************************************
NAME
  	configManagerSetVersionNo

DESCRIPTION
    Reads the PSKEY containing the version number and checks if it needs to be
    set or reset, will not be written if already the correct value

RETURNS
  	void
*/
void configManagerSetVersionNo( void );

/****************************************************************************
NAME
    configManagerSetUpgradeTransportType

DESCRIPTION
    Sets VM Upgrade transport type to the PSKEY.

RETURNS
    void
*/
void configManagerSetUpgradeTransportType( uint16 transport_type );

/****************************************************************************
NAME
    configManagerGetUpgradeTransportType

DESCRIPTION
    Reads VM Upgrade transport type from the PSKEY.

RETURNS
    transport type
*/
uint16 configManagerGetUpgradeTransportType( void );

/****************************************************************************
NAME
    configManagerProcessEventSysDefrag

DESCRIPTION
    Processes defrag related system events

RETURNS
    void
*/
void configManagerProcessEventSysDefrag(const MessageId defragEvent);

#endif
