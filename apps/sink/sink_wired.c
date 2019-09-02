/*
Copyright (c) 2005 - 2016 Qualcomm Technologies International, Ltd.
*/

/*!
@file
@ingroup sink_app
@brief
    Application level implementation of Wired Sink features

NOTES
    - Conditional on ENABLE_WIRED define
    - Output PIO is theSink.conf6->PIOIO.pio_outputs.wired_output
    - Input PIO is theSink.conf6->PIOIO.pio_inputs.wired_input
*/

#include "sink_private.h"
#include "sink_debug.h"
#include "sink_wired.h"
#include "sink_audio.h"
#include "sink_pio.h"
#include "sink_powermanager.h"
#include "sink_statemanager.h"
#include "sink_tones.h"
#include "sink_display.h"
#include "sink_audio_routing.h"
#include "sink_volume.h"

#ifdef ENABLE_SUBWOOFER
#include "sink_swat.h"
#endif

#include <panic.h>
#include <stream.h>
#include <source.h>
#include <sink.h>
#include <string.h>

#ifdef ENABLE_WIRED

#include <audio_plugin_music_params.h>
#include <audio_plugin_music_variants.h>

#ifdef DEBUG_WIRED
    #define WIRED_DEBUG(x) DEBUG(x)
#else
    #define WIRED_DEBUG(x) 
#endif

/* Wired PIOs */
#define PIO_ANALOG_DETECT (theSink.conf6->PIOIO.pio_inputs.analog_input)
#define PIO_SPDIF_DETECT (theSink.conf6->PIOIO.pio_inputs.spdif_detect)
#define PIO_I2S_DETECT (theSink.conf6->PIOIO.pio_inputs.i2s_detect)
#define PIO_WIRED_SELECT PIO_POWER_ON
#define PIO_SPDIF_INPUT (theSink.conf6->PIOIO.pio_inputs.spdif_input)
#define PIO_SPDIF_OUTPUT (theSink.conf6->PIOIO.pio_outputs.spdif_output)

#define PIN_WIRED_ALWAYSON  PIN_INVALID     /* Input PIO is disabled, always assumed on */
#define PIN_WIRED_DISABLED  0xFE            /* Entire input type is disabled */

/* Wired audio params */
#ifdef FORCE_ANALOGUE_INPUT
    #define ANALOG_CONNECTED  (TRUE)
    #define ANALOG_READY      (TRUE)
    #define SPDIF_CONNECTED   (FALSE)
    #define SPDIF_READY       (FALSE)
    #define I2S_CONNECTED     (FALSE)
    #define I2S_READY         (FALSE)
#else
    /* If the Detect PIO for S/PDIF or analog wired input is set to N/A (0xff), assume always connected */
    #define ANALOG_CONNECTED  (PIO_ANALOG_DETECT == PIN_WIRED_ALWAYSON?TRUE:!PioGetPio(PIO_ANALOG_DETECT))
    #define ANALOG_READY      ((theSink.conf6 != NULL) && (PIO_ANALOG_DETECT !=PIN_WIRED_DISABLED))
    #define SPDIF_CONNECTED   (PIO_SPDIF_DETECT == PIN_WIRED_ALWAYSON?TRUE:!PioGetPio(PIO_SPDIF_DETECT))
    #define SPDIF_READY       ((theSink.conf6 != NULL) && (PIO_SPDIF_DETECT !=PIN_WIRED_DISABLED))
    #define I2S_CONNECTED     (PIO_I2S_DETECT == PIN_WIRED_ALWAYSON?TRUE:!PioGetPio(PIO_I2S_DETECT))
    #define I2S_READY         ((theSink.conf6 != NULL) && (PIO_I2S_DETECT !=PIN_WIRED_DISABLED))
#endif

/* These values must never represent a "real" Sink.
 * Hydracore may have a wider range of possible Sink values. Although a clash is
 * considered unlikely, Hydracore values (pointers in most case) would be divisble
 * by 4. */
#define ANALOG_SINK      ((Sink)0xFFFF)
#define SPDIF_SINK       ((Sink)0xFFFE)
#define I2S_SINK         ((Sink)0xFFFD)

#define WIRED_RATE       (48000)

/* Indicates wired audio sampling rates */
typedef enum
{
    WIRED_RATE_44100 = 1,
    WIRED_RATE_48000 ,
    WIRED_RATE_88200 ,
    WIRED_RATE_96000
}WIRED_SAMPLING_RATE_T;


/****************************************************************************
NAME 
    wiredAudioInit
    
DESCRIPTION
    Set up wired audio PIOs and configuration
    
RETURNS
    void
*/ 
void wiredAudioInit(void)
{
    if(ANALOG_READY)
    {
        WIRED_DEBUG(("WIRE: analog Select %d Detect %d\n", PIO_WIRED_SELECT, PIO_ANALOG_DETECT));
        
        /* Pull detect high, audio jack will pull it low */
        if(PIO_ANALOG_DETECT != PIN_WIRED_ALWAYSON)
            PioSetPio(PIO_ANALOG_DETECT, pio_pull, TRUE);

        stateManagerAmpPowerControl(POWER_UP);
    }

    if(SPDIF_READY)
    {
        WIRED_DEBUG(("WIRE: spdif Select %d Detect %d\n", PIO_WIRED_SELECT, PIO_SPDIF_DETECT));
    
        if(PIO_SPDIF_DETECT != PIN_WIRED_ALWAYSON)
        {
            /* Pull detect high, audio jack will pull it low */
            PioSetPio(PIO_SPDIF_DETECT, pio_pull, TRUE);
        }
        stateManagerAmpPowerControl(POWER_UP);

        if((PIO_SPDIF_INPUT != PIN_WIRED_ALWAYSON) && (PIO_SPDIF_INPUT != PIN_WIRED_DISABLED))
        {
            /* configure SPDIF ports, required for CSR8675 */
            PioSetFunction(PIO_SPDIF_INPUT, SPDIF_RX);            
        }
        if(PIO_SPDIF_OUTPUT != PIN_INVALID) 
        {
            /* configure SPDIF output port, required for CSR8675 */
            PioSetFunction(PIO_SPDIF_OUTPUT, SPDIF_TX);
        }
    }

    if(I2S_READY)
    {
        WIRED_DEBUG(("WIRE: I2S Select %d Detect %d\n", PIO_WIRED_SELECT, PIO_I2S_DETECT));
        
        /* Pull detect high, audio jack will pull it low */
        if(PIO_I2S_DETECT != PIN_WIRED_ALWAYSON)
            PioSetPio(PIO_I2S_DETECT, pio_pull, TRUE);

        stateManagerAmpPowerControl(POWER_UP);
    }
}

static uint32 getWiredRate(uint8 wired_rate)
{
    uint32 rate = WIRED_RATE;
    
    switch(wired_rate)
        {
            case WIRED_RATE_44100:
                rate = 44100;
            break;

            case WIRED_RATE_48000: 
                rate = 48000;
            break;
#ifdef HI_RES_AUDIO
            /* Hi Res audio rates are only supported on CSR8675 and not on CSR8670 */
            case WIRED_RATE_88200:
                rate = 88200;
            break;

            case WIRED_RATE_96000:
                rate = 96000;
            break;
#endif
            default:
                break;
        }
    return rate;
}

static void populateCommonConnectParams(audio_connect_parameters *connect_parameters)
{
    AudioPluginFeatures PluginFeatures = theSink.conf2->audio_routing_data.PluginFeatures;
    uint32 input_rate = getWiredRate(theSink.conf2->audio_routing_data.wired_audio_input_rate);

    /* Make sure we're using correct parameters for Wired audio */
    theSink.a2dp_link_data->a2dp_audio_connect_params.mode_params = &theSink.a2dp_link_data->a2dp_audio_mode_params;
    /* pass in the AC3 configuration */
    theSink.a2dp_link_data->a2dp_audio_connect_params.spdif_ac3_config = &theSink.conf2->audio_routing_data.SpdifAC3Settings;
    /* pass in spdif configuration */
    theSink.a2dp_link_data->a2dp_audio_connect_params.spdif_config = &theSink.conf2->audio_routing_data.SpdifConfig;

    /* Read 24 bit configuration from ps key*/
    theSink.a2dp_link_data->a2dp_audio_connect_params.wired_audio_output_rate =
                        getWiredRate(theSink.conf2->audio_routing_data.wired_audio_output_rate);

    WIRED_DEBUG(("WIRE: Routing (Vol %ddB)\n", theSink.volume_levels.analog_volume.main_volume));

#if defined ENABLE_PEER && defined PEER_TWS
    input_rate = WIRED_RATE;
#endif

    peerPopulatePluginConnectData(&PluginFeatures, input_rate);

    connect_parameters->volume = TonesGetToneVolumeInDb(audio_output_group_main);
    connect_parameters->rate = input_rate,
    connect_parameters->features = PluginFeatures;
    connect_parameters->mode = AUDIO_MODE_CONNECTED;
    connect_parameters->route = AUDIO_ROUTE_INTERNAL;
    connect_parameters->power = powerManagerGetLBIPM();
    connect_parameters->params = &theSink.a2dp_link_data->a2dp_audio_connect_params;
    connect_parameters->app_task = &theSink.task;
}


bool analoguePopulateConnectParameters(audio_connect_parameters *connect_parameters)
{
    WIRED_DEBUG(("ANALOG: Audio "));

    if(ANALOG_READY && ANALOG_CONNECTED)
    {
        WIRED_DEBUG(("Connected [0x%04X]\n", (uint16)ANALOG_SINK));
        populateCommonConnectParams(connect_parameters);
        connect_parameters->audio_plugin = (Task)&csr_analogue_decoder_plugin;
        connect_parameters->sink_type = AUDIO_SINK_ANALOG;
        connect_parameters->audio_sink = ANALOG_SINK;
        return TRUE;
    }
    return FALSE;
}


bool spdifPopulateConnectParameters(audio_connect_parameters *connect_parameters)
{
    WIRED_DEBUG(("SPDIF: Audio "));

    if(SPDIF_READY && SPDIF_CONNECTED)
    {
        WIRED_DEBUG(("Connected [0x%04X]\n", (uint16)SPDIF_SINK));
        populateCommonConnectParams(connect_parameters);
        connect_parameters->audio_plugin = (Task)&csr_spdif_decoder_plugin;
        connect_parameters->sink_type = AUDIO_SINK_SPDIF;
        connect_parameters->audio_sink = SPDIF_SINK;
        return TRUE;
    }
    return FALSE;
}


bool i2sPopulateConnectParameters(audio_connect_parameters *connect_parameters)
{
    WIRED_DEBUG(("I2S: Audio "));

    if(I2S_READY && I2S_CONNECTED)
    {
        WIRED_DEBUG(("Connected [0x%04X]\n", (uint16)SPDIF_SINK));
        populateCommonConnectParams(connect_parameters);
        connect_parameters->audio_plugin = (Task)&csr_i2s_decoder_plugin;
        connect_parameters->sink_type = AUDIO_SINK_I2S;
        connect_parameters->audio_sink = I2S_SINK;
        return TRUE;
    }
    return FALSE;
}

/****************************************************************************
NAME 
    analogAudioSinkMatch
    
DESCRIPTION
    Compare sink to analog audio sink.
    
RETURNS
    TRUE if Sink matches, FALSE otherwise
*/ 
bool analogAudioSinkMatch(Sink sink)
{
    if(ANALOG_READY)
        return (sink == ANALOG_SINK && sink);
    return FALSE;
}

/****************************************************************************
NAME 
    spdifAudioSinkMatch
    
DESCRIPTION
    Compare sink to spdif audio sink.
    
RETURNS
    TRUE if Sink matches, FALSE otherwise
*/ 
bool spdifAudioSinkMatch(Sink sink)
{
    if(SPDIF_READY)
        return (sink == SPDIF_SINK && sink);
    return FALSE;
}

/****************************************************************************
NAME 
    i2sAudioSinkMatch
    
DESCRIPTION
    Compare sink to i2s audio sink.
    
RETURNS
    TRUE if Sink matches, FALSE otherwise
*/ 
bool i2sAudioSinkMatch(Sink sink)
{
    if(I2S_READY)
         return (sink == I2S_SINK && sink);
    return FALSE;
}

#if defined ENABLE_PEER && defined PEER_TWS 

/****************************************************************************
NAME 
    wiredAudioCheckDeviceTrimVol
    
DESCRIPTION
    Adjust wired audio volume if currently being routed
    
RETURNS
    TRUE if wired audio being routed, FALSE otherwise
*/ 

bool wiredAudioCheckDeviceTrimVol(volume_direction dir, tws_device_type tws_device)
{ 
    /* if either the analog or spdif inputs are routed, adjust the appropriate volume */
    if(!analogAudioSinkMatch(theSink.routed_audio))
    {
        return FALSE;
    }

    VolumeModifyAndUpdateTWSDeviceTrim(dir, tws_device);

    /* volume successfully updated */
    return TRUE;
}
#endif

    
/****************************************************************************
NAME 
    analogAudioConnected
    
DESCRIPTION
    Determine if analog audio input is connected
    
RETURNS
    TRUE if connected, otherwise FALSE
*/ 
bool analogAudioConnected(void)
{
    /* If Wired mode configured and ready check PIO */
    if (ANALOG_READY)
    {
        return ANALOG_CONNECTED;
    }

    /* Wired audio cannot be connected yet */
    return FALSE;
}

/****************************************************************************
NAME 
    analogGetAudioSink
    
DESCRIPTION
    Check analog state and return sink if available
    
RETURNS
    Sink if available, otherwise NULL
*/ 
Sink analogGetAudioSink(void)
{
    if (ANALOG_READY)
    {
        return ANALOG_SINK;
    }
    
    return NULL;
}


/****************************************************************************
NAME 
    analogGetAudioRate
    
DESCRIPTION
    Obtains the current defined sample rate for wired audio
    
RETURNS
    None
*/ 
void analogGetAudioRate (uint16 *rate)
{
    if (ANALOG_READY)
    {
        *rate = WIRED_RATE;
    }
    else
    {
        *rate = 0;
    }
}


/****************************************************************************
NAME 
    spdifAudioConnected
    
DESCRIPTION
    Determine if spdif audio input is connected
    
RETURNS
    TRUE if connected, otherwise FALSE
*/ 
bool spdifAudioConnected(void)
{
    /* If Wired mode configured and ready check PIO */
    if (SPDIF_READY)
    {
        return SPDIF_CONNECTED;
    }

    /* spdif audio cannot be connected yet */
    return FALSE;
}

/****************************************************************************
NAME
    i2sAudioConnected
    
DESCRIPTION
    Determine if i2s audio input is connected
    
RETURNS
    TRUE if connected, otherwise FALSE
*/ 
bool i2sAudioConnected(void)
{
 /* If Wired mode configured and ready check PIO */
    if (I2S_READY)
    {
        return I2S_CONNECTED;
    }

    /* Wired audio cannot be connected yet */
    return FALSE;
}

bool wiredAnalogAudioIsRouted(void)
{
    if(analogAudioSinkMatch(theSink.routed_audio))
    {
        return TRUE;
    }
    return FALSE;
}

bool wiredSpdifAudioIsRouted(void)
{
    if(spdifAudioSinkMatch(theSink.routed_audio))
    {
        return TRUE;
    }
    return FALSE;
}

bool wiredI2SAudioIsRouted(void)
{
    if(i2sAudioSinkMatch(theSink.routed_audio))
    {
        return TRUE;
    }
    return FALSE;
}
#endif /* ENABLE_WIRED */
