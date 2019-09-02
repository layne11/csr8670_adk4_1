/******************************************************************************
Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.1
 
FILE NAME
    audio_output_private.h
 
DESCRIPTION
    Private data/functions for audio_output lib
*/

#ifndef __AUDIO_OUTPUT_PRIVATE_H__
#define __AUDIO_OUTPUT_PRIVATE_H__

/* Workaround GCC error where -DANC in command line becomes anc define */
#ifdef anc
#define ANC
#endif

/*============================================================================*
 *  Private Definitions
 *============================================================================*/

/* Convenience Function Macros */
#define forEachOutput(audio_out) for(audio_out=0; audio_out<audio_output_max; audio_out++)

/*============================================================================*
 *  Private Data Types
 *============================================================================*/

/*!
    @brief Audio Output State Variables
    
    Used internally by the plugin to maintain a record of its current state.
*/
typedef struct __audio_output_state
{
    unsigned connected:1;           /*! Whether connection has take place */
    unsigned i2s_devices_active:1;  /*! Whether any I2S devices are active */
    audio_output_params_t params;   /*! Latest params passed on connection */
    Source* sources;                /*! Store sources prior to connection */
} audio_output_state_t;

/*============================================================================*
 *  Private Function Prototypes
 *============================================================================*/
 
extern const audio_output_config_t* config;

/******************************************************************************
NAME:
    audioOutputGetSink

DESCRIPTION:
    Gets the sink for a specified output channel, if a mapping exists.

PARAMETERS:
    audio_out     Output to get the sink for.
    
RETURNS:
    The sink, or NULL if there was an error.
*/
Sink audioOutputGetSink(audio_output_t audio_out);

/******************************************************************************
NAME:
    audioOutputGainMuteHardware

DESCRIPTION:
    Mutes the external hardware (e.g. I2S amps, DAC) associated with an output,
    regardless of the gain type and trims set for it in the configuration. Also
    mutes it at the stream level. Used when connecting ports to ensure there
    are no pops, clicks, e.t.c.

PARAMETERS:
    audio_out     Output to hard mute.
    
RETURNS:
    Nothing.
*/
void audioOutputGainMuteHardware(audio_output_t audio_out);

#endif /*__AUDIO_OUTPUT_PRIVATE_H__*/
