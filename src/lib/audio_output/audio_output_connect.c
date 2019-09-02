/******************************************************************************
Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.1
 
FILE NAME
    audio_output_connect.c
 
DESCRIPTION
    Plugin that implements multichannel audio by utilising multiple hardware
    audio outputs (onboard DAC, I2S, etc...).
*/
 
 
/*============================================================================*
 *  SDK Header Files
 *============================================================================*/

#include "audio_output.h"
#include "audio_output_private.h"

#ifdef ANC
#include "anc.h"
#endif

#include <stdlib.h>
#include <panic.h>
#include <pio.h>
#include <print.h>
#include <stream.h>
#include <stdlib.h>
#include <sink.h>
#include <string.h>
#include <transform.h>
#include <audio.h>

/*============================================================================*
 *  Local Header Files
 *============================================================================*/

#include "csr_i2s_audio_plugin.h"
#include "audio_plugin_if.h"

/*============================================================================*
 *  Private Definitions
 *============================================================================*/

/* Hardware Specific Details */
#define PIO_I2S_2_PCM_IN        (4)
#define PIO_I2S_2_PCM_OUT       (5)
#define PIO_I2S_2_PCM_SYNC      (6)
#define PIO_I2S_2_PCM_CLK       (7)

/* Error Return Values */
#define AUDIO_HARDWARE_NONE     (audio_hardware)(0xFFFF)
#define AUDIO_INSTANCE_NONE     (audio_instance)(0xFFFF)
#define AUDIO_CHANNEL_NONE      (audio_channel)(0xFFFF)

/* Macro to check whether an output has been disabled in disable_mask */
#define audioOutputDisabled(audio_out, disable_mask) \
    ((AudioOutputGetMask(audio_out) & disable_mask) == AudioOutputGetMask(audio_out))
#define audioOutputEnabled(audio_out, disable_mask) \
    (!audioOutputDisabled(audio_out, disable_mask))

/*============================================================================*
 *  Private Data Types
 *============================================================================*/



/*============================================================================*
 *  Private Data
 *============================================================================*/

/* Multichannel audio configuration data */
const audio_output_config_t* config = NULL;
/* Global state variable */
static audio_output_state_t state;


/*============================================================================*
 *  Private Function Prototypes
 *============================================================================*/

static void audioOutputResetSources(void);
static bool audioOutputValidateConnectParams(audio_output_params_t* params);
static bool connect(Source* sources, audio_output_params_t* params);
static bool i2sHardwareInitialise(void);
static bool i2sOutputEnabled(audio_output_params_t* params);
static void configureSink(Sink audio_sink,
                          audio_output_t audio_out,
                          audio_output_params_t* params);
static Sink audioOutputGetSinkIfNotDisabled(audio_output_t audio_out, 
                                            audio_output_params_t* params);
static audio_hardware getAudioHardwareType(audio_output_t audio_out);
static audio_instance getAudioHardwareInstance(audio_output_t audio_out);
static audio_channel getAudioHardwareChannel(audio_output_t audio_out);
static bool isDSPPresent(void);

/*============================================================================*
 * Public Function Implementations
 *============================================================================*/

/******************************************************************************/
bool AudioOutputInit(const audio_output_config_t* const conf)
{
    if (conf == NULL)
    {
        /* Passed in pointer was NULL, return error. */
        PRINT(("AUDIO OUT: Failed to initialise - NULL config\n"));
        return FALSE;
    }
    
    /* Store pointer for later access. */
    config = conf;
    
    /* Ensure all source mappings are cleared */
    audioOutputResetSources();
    
    /* Initialise global state variables. */
    memset(&state, 0, sizeof(audio_output_state_t));
    
    /* Initialise internal I2S hardware. */
    if (!i2sHardwareInitialise())
    {
        /* Failed to perform required initialisation, return error. */
        PRINT(("AUDIO OUT: Failed to initialise - I2S hardware\n"));
        return FALSE;
    }
    
    PRINT(("AUDIO OUT: Initialised successfully\n"));
    
    return TRUE;
}

/******************************************************************************/
bool AudioOutputAddSource(Source source, audio_output_t output)
{
    if(state.sources == NULL)
        state.sources = calloc(audio_output_max, sizeof(source));
    
    PanicNull(state.sources);
    
    if(state.sources[output] != NULL)
        return FALSE;
    
    state.sources[output] = source;
    
    return TRUE;
}

/******************************************************************************/
bool AudioOutputConnect(audio_output_params_t* params)
{
    bool success = FALSE;
    
    if(audioOutputValidateConnectParams(params))
    {
        /* Connect the DSP sources to their appropriate hardware outputs. */
        success = connect(state.sources, params);
        
        if (success)
        {
            PRINT(("AUDIO OUT: Connect DSP successful\n"));
            
            /* Update state variables */
            state.params = *params;
            state.connected = TRUE;
        }
    }
    
    /* Reset mappings whether successful or not */
    audioOutputResetSources();
    
    PRINT(("AUDIO OUT: Connect DSP failed - Connecting sinks error\n"));
    return success;
}

/******************************************************************************/
bool AudioOutputConnectStereoSource(Source source_left,
                                    Source source_right,
                                    audio_output_params_t* params)
{
    /* Reset any previous mappings */
    audioOutputResetSources();
    
    if(!AudioOutputAddSource(source_left, audio_output_primary_left))
        return FALSE;
    
    if(!AudioOutputAddSource(source_right, audio_output_primary_right))
        return FALSE;
    
    return AudioOutputConnect(params);
}

/******************************************************************************/
bool AudioOutputConnectSource(Source source,
                              audio_output_t output,
                              audio_output_params_t* params)
{
    /* Reset any previous mappings */
    audioOutputResetSources();
    
    if(!AudioOutputAddSource(source, output))
        return FALSE;
    
    return AudioOutputConnect(params);
}

/******************************************************************************/
bool AudioOutputDisconnect(void)
{
    audio_output_t audio_out;  /* Loop variable */
    bool at_least_one_mapping = FALSE;
    
    if (config == NULL)
    {
        /* Plugin is not yet initialised, return error. */
        PRINT(("AUDIO OUT: Disconnect failed - Not initialised\n"));
        return FALSE;
    }
    
    if (!state.connected)
    {
        /* Not connected yet, return error. */
        PRINT(("AUDIO OUT: Disconnect failed - Not connected\n"));
        return FALSE;
    }
    
    /* Disconnect each output in turn. */
    forEachOutput(audio_out)
    {
        Sink audio_sink = audioOutputGetSinkIfNotDisabled(audio_out, &state.params);
        
        if (audio_sink == (Sink)NULL)
        {
            /* No mapping for this output. Move on to the next. */
            continue;
        }
        
        at_least_one_mapping = TRUE;
        
        /* Mute before disconnecting streams so no 'pops' or 'clicks'. */
        audioOutputGainMuteHardware(audio_out);
        
        StreamDisconnect(0, audio_sink);
        PanicFalse(SinkClose(audio_sink));
    }
    
    if (!at_least_one_mapping)
    {
        /* No mappings in config, return error. */
        PRINT(("AUDIO OUT: Disconnect failed - No enabled mappings\n"));
        return FALSE;
    }
    
    /* Update state variables */
    state.connected = FALSE;
    
    if (state.i2s_devices_active)
    {
        /* Shut down I2S devices */
        PRINT(("AUDIO OUT: Shutting down I2S devices...\n"));
        CsrShutdownI2SDevice();
        state.i2s_devices_active = FALSE;
    }
    
    PRINT(("AUDIO OUT: Disconnect successful\n"));
    return TRUE;
}

/******************************************************************************/
Sink AudioOutputGetAudioSink(void)
{
    audio_output_t audio_out;
    
    /* Return the first valid audio sink */
    forEachOutput(audio_out)
    {
        Sink audio_sink = audioOutputGetSinkIfNotDisabled(audio_out, &state.params);
        
        if(audio_sink)
            return audio_sink;
    }
    
    return (Sink)NULL;
}

/******************************************************************************/
bool AudioOutputConfigRequiresI2s(void)
{
    audio_output_t audio_out;  /* Loop variable */
    
    if (config == NULL)
    {
        /* Plugin is not initialised, so no I2S required. */
        return FALSE;
    }
    
    /* Loop through and check each output mapping. */
    forEachOutput(audio_out)
    {
        if (config->mapping[audio_out].endpoint.type == audio_output_type_i2s)
        {
            /* I2S output found, no need to continue. */
            return TRUE;
        }
    }
    
    /* No I2S outputs detected. */
    return FALSE;
}

/******************************************************************************/
bool AudioOutputI2sActive(void)
{
    if (config == NULL)
    {
        /* Plugin is not initialised, so I2S can't be. */
        return FALSE;
    }
    
    return state.i2s_devices_active;
}

/******************************************************************************/
bool AudioOutput24BitOutputEnabled(void)
{
    if (config == NULL)
    {
        /* Plugin is not initialised. */
        return FALSE;
    }
    return (config->output_resolution_mode == audio_output_24_bit) ? TRUE: FALSE;
}
/******************************************************************************/
uint32 AudioOutputGetSampleRate(audio_output_params_t* params)
{
    /* If an I2S output is to be used then return the re-sampling frequency */
    if(i2sOutputEnabled(params) && !params->disable_resample)
    {
        return CsrI2SGetOutputResamplingFrequencyForI2s(params->sample_rate);
    }
    /* Return the sample_rate from params */
    return params->sample_rate;
}

/******************************************************************************/
AUDIO_OUTPUT_TYPE_T AudioOutputGetOutputType(audio_output_t audio_out)
{
    if ((config == NULL) || (audio_out == audio_output_max))
    {
        return OUTPUT_INTERFACE_TYPE_NONE;
    }
    
    switch (config->mapping[audio_out].endpoint.type)
    {
        case audio_output_type_dac:
            return OUTPUT_INTERFACE_TYPE_DAC;
        
        case audio_output_type_i2s:
            return OUTPUT_INTERFACE_TYPE_I2S;
            
        case audio_output_type_spdif:
            return OUTPUT_INTERFACE_TYPE_SPDIF;
        
        case audio_output_type_bt:
            /* Handled by specific decoder plugin, fall through. */
        case audio_output_type_none:
        default:
            /* No mapping or unsupported output type. */
            return OUTPUT_INTERFACE_TYPE_NONE;
    }
}

/******************************************************************************/
bool AudioOutputGetDspOutputTypesMsg(AUDIO_OUTPUT_TYPES_T* msg)
{
    audio_output_t audio_out;    /* Loop variable */
    
    if (config == NULL || msg == NULL)
    {
        return FALSE;
    }
    
    /* Loop through each output mapping. */
    forEachOutput(audio_out)
    {
        msg->out_type[audio_out] = AudioOutputGetOutputType(audio_out);
    }
    
    return TRUE;
}


/*============================================================================*
 *  Private Function Implementations
 *============================================================================*/

/******************************************************************************
NAME:
    audioOutputResetSources

DESCRIPTION:
    Reset list of mapped sources
*/
static void audioOutputResetSources(void)
{
    if(state.sources)
    {
        free(state.sources);
        state.sources = NULL;
    }
}

/******************************************************************************
NAME:
    audioOutputValidateConnectParams

DESCRIPTION:
    Check that parameters passed to connect function are valid

PARAMETERS:
    params      Connection parameters containing configuration data.
    
RETURNS:
    TRUE if parameters were okay and library has been configured correctly, 
    otherwise FALSE.
*/
static bool audioOutputValidateConnectParams(audio_output_params_t* params)
{
    if (config == NULL || params == NULL || state.sources == NULL)
    {
        return FALSE;
    }
    
    if (state.connected)
    {
        PRINT(("AUDIO OUT: Connect failed - Already Connected\n"));
        return FALSE;
    }
    
    if (params->sample_rate == 0)
    {
        PRINT(("AUDIO OUT: Connect failed - Invalid sample rate\n"));
        return FALSE;
    }
    return TRUE;
}

/******************************************************************************
NAME:
    audioOutputSetupI2sIfEnabled

DESCRIPTION:
    Power on external I2S DAC(s) if required

PARAMETERS:
    params      Connection parameters containing configuration data.
*/
static void audioOutputSetupI2sIfEnabled(audio_output_params_t* params)
{
    /* Carry out I2S-specific set up. */
    if (i2sOutputEnabled(params))
    {
        /* Check if we need to initialise I2S devices. */
        if (!state.i2s_devices_active)
        {
            PRINT(("AUDIO OUT: Initialising I2S devices...\n"));
            
            /* Initialise the I2S device hardware. */
            CsrInitialiseI2SDevice(params->sample_rate);
            
            /* Only needs to happen once. */
            state.i2s_devices_active = TRUE;
        }
    }
}

/******************************************************************************
NAME:
    connect

DESCRIPTION:
    Connects sinks for all output channels to the sources specified in the
    supplied parameters.

PARAMETERS:
    sources     Pointer to an array of audio_output_max sources.
    params      Connection parameters containing configuration data.
    
RETURNS:
    The sink that was successfully configured, NULL otherwise.
*/
static bool connect(Source* sources, audio_output_params_t* params)
{
    audio_output_t audio_out;  /* Loop variable */
    Sink audio_sink;
    Sink last_valid_sink = (Sink)NULL;
        
    params->sample_rate = AudioOutputGetSampleRate(params);

    audioOutputSetupI2sIfEnabled(params);
    
    /* Configure and sync the sink for each output. */
    forEachOutput(audio_out)
    {
        /* Check if we actually need to open the sink for this output. */
        if (sources[audio_out] == (Source)NULL)
        {
            /* No source for this output, or disabled, move onto the next. */
            continue;
        }
        
        audio_sink = audioOutputGetSinkIfNotDisabled(audio_out, params);
        
        if (audio_sink == (Sink)NULL)
        {
            /* No mapping or sink failed to open. Move on to the next. */
            continue;
        }
        
        configureSink(audio_sink, audio_out, params);
        
        if (last_valid_sink)
        {
            PanicFalse(SinkSynchronise(last_valid_sink, audio_sink));
        }
        
        last_valid_sink = audio_sink;
    }
    
    /* Check if there are any sinks to connect before proceeding. */
    if (last_valid_sink == (Sink)NULL)
    {
        /* No mappings in config, all disabled, or all failed, return error. */
        PRINT(("AUDIO OUT: Connect failed - No sinks to connect\n"));
        return FALSE;
    }
    
    /* Connect up each valid sink to its source and un-mute. Must happen after
     * synchronisation, hence the need for a second loop and duplicate checks.
     */
    forEachOutput(audio_out)
    {
        /* Check if we actually need to connect the sink for this output. */
        if (sources[audio_out] == (Source)NULL)
        {
            /* No source for this output, or disabled, move onto the next. */
            continue;
        }
        
        audio_sink = audioOutputGetSinkIfNotDisabled(audio_out, params);
    
        if (audio_sink == (Sink)NULL)
        {
            /* No mapping or sink failed to open. Move on to the next. */
            continue;
        }
        
        switch(params->transform)
        {
            case audio_output_tansform_adpcm:
                PanicFalse(TransformStart(TransformAdpcmDecode(sources[audio_out], audio_sink)));
            break;
            
            case audio_output_tansform_connect_and_dispose:
                PanicFalse(StreamConnectAndDispose(sources[audio_out], audio_sink));
            break;
            
            case audio_output_tansform_connect:
            default:
                PanicNull(StreamConnect(sources[audio_out], audio_sink));
            break;
        }
        
        /* Un-mute after connecting streams so no 'pops' or 'clicks'. */
        PanicFalse(SinkConfigure(audio_sink, STREAM_AUDIO_MUTE_ENABLE, FALSE));
    }
    
    return TRUE;
}

/******************************************************************************
NAME:
    i2sHardwareInitialise

DESCRIPTION:
    Performs internal hardware initialisation for I2S, such as configuring the 
    required PIOs. Note that external I2S devices e.g. I2S amplifiers are
    initialised separately, on a per connection basis. This is handled by the
    I2S plugin.
    
RETURNS:
    Whether the I2S hardware was initialised. TRUE = success, FALSE = error.
*/
static bool i2sHardwareInitialise(void)
{
    audio_output_t audio_out;  /* Loop variable */
    
    /* Loop through and check each output for usage of 2nd I2S instance. */
    forEachOutput(audio_out)
    {
        if (config->mapping[audio_out].endpoint.type == audio_output_type_i2s &&
            config->mapping[audio_out].endpoint.instance == audio_output_hardware_instance_1)
        {
            /* 2nd instance found, no need to continue searching. Return success
               based on whether all PIOs can be set to the correct function. */
            return (PioSetFunction(PIO_I2S_2_PCM_IN, PCM_IN) && 
                    PioSetFunction(PIO_I2S_2_PCM_OUT, PCM_OUT) &&
                    PioSetFunction(PIO_I2S_2_PCM_SYNC, PCM_SYNC) &&
                    PioSetFunction(PIO_I2S_2_PCM_CLK, PCM_CLK));
        }
    }
    
    /* No 2nd instance found, so no need to do anything. */
    return TRUE;
}

/******************************************************************************
NAME:
    i2sOutputEnabled

DESCRIPTION:
    Checks whether any output has been configured to use the I2S hardware and
    has not been disabled in the supplied parameters. Used to determine whether
    any I2S devices need to be initialised for a specific connection request.

PARAMETERS:
    params      Pointer to requested parameters that contain the disable mask.
    
RETURNS:
    Whether I2S is going to be used. TRUE = yes, FALSE = no.
*/
bool i2sOutputEnabled(audio_output_params_t* params)
{
    audio_output_t audio_out;  /* Loop variable */
    
    /* Loop through and check each output mapping. */
    forEachOutput(audio_out)
    {
        if (config->mapping[audio_out].endpoint.type == audio_output_type_i2s &&
            audioOutputEnabled(audio_out, params->disable_mask))
        {
            /* Enabled I2S output found, no need to continue. */
            return TRUE;
        }
    }
    
    /* No enabled I2S outputs detected. */
    return FALSE;
}

/******************************************************************************
NAME:
    isDSPPresent

DESCRIPTION:
    Check if the DSP is loaded

PARAMETERS:
    NULL
    
RETURNS:
    Returns TRUE if DSP is loaded, FALSE otherwise.
*/
static bool isDSPPresent(void)
{
    DSP_STATUS_INFO_T status = GetCurrentDspStatus();
    if(status != DSP_NOT_LOADED)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/******************************************************************************
NAME:
    configureSink

DESCRIPTION:
    Configures the sink mapped to an output with the required parameters.

PARAMETERS:
    audio_sink  The audio sink to configure
    audio_out   Output to configure the mapped hardware output of.
    params      Parameters to configure.
    
RETURNS:
    The sink that was successfully configured, NULL otherwise.
*/
static void configureSink(Sink audio_sink,
                          audio_output_t audio_out,
                          audio_output_params_t* params)
{
    uint16 bit_resolution = RESOLUTION_MODE_16BIT;

#ifdef ANC

    /* If using ANC then only DAC output type allowed. Configuration also needs
       to be done using the ANC library */
    PanicFalse(config->mapping[audio_out].endpoint.type == audio_output_type_dac);
    AncConfigureDacs(params->sample_rate);

#else /* ANC */

    /* Configure output rate */
    switch(config->mapping[audio_out].endpoint.type)
    {
        case audio_output_type_i2s:
            CsrI2SAudioOutputConnectConfigureSink(audio_sink, params->sample_rate);
        break;
        
        case audio_output_type_spdif:
            PanicFalse(SinkConfigure(audio_sink, STREAM_SPDIF_OUTPUT_RATE, params->sample_rate));
        break;
        
        case audio_output_type_dac:
            PanicFalse(SinkConfigure(audio_sink, STREAM_CODEC_OUTPUT_RATE, params->sample_rate));
        break;
        
        default:
            /* Should never get here as audio_sink would be NULL */
            Panic();
        break;
    }

#endif /* ANC */

    /* 24bit resolution supported only when playing through DSP 
        (standlone prompts/tones still works only on 16 bit)*/
    if((config->output_resolution_mode == audio_output_24_bit) && isDSPPresent())
    {
        bit_resolution = RESOLUTION_MODE_24BIT ;
    }
    PanicFalse(SinkConfigure(audio_sink, STREAM_AUDIO_SAMPLE_SIZE, bit_resolution));

    /* Mute before connecting streams so no 'pops' or 'clicks'. */
    audioOutputGainMuteHardware(audio_out);
}

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
Sink audioOutputGetSink(audio_output_t audio_out)
{
    return StreamAudioSink(getAudioHardwareType(audio_out),
                           getAudioHardwareInstance(audio_out),
                           getAudioHardwareChannel(audio_out));
}

/******************************************************************************
NAME:
    audioOutputGetSinkIfNotDisabled

DESCRIPTION:
    Gets the sink for a specified output channel, if a mapping exists and it is
    not disabled in the audio parameters

PARAMETERS:
    audio_out     Output to get the sink for.
    params        Configuration set at connection
    
RETURNS:
    The sink, or NULL if there was an error.
*/
static Sink audioOutputGetSinkIfNotDisabled(audio_output_t audio_out, 
                                            audio_output_params_t* params)
{
    if(audioOutputEnabled(audio_out, params->disable_mask))
    {
        return audioOutputGetSink(audio_out);
    }
    return NULL;
}

/******************************************************************************
NAME:
    getAudioHardwareType

DESCRIPTION:
    Gets the hardware type of an audio sink mapped to an output. Must only be
    called after successful initialisation (i.e. 'config' is valid).

PARAMETERS:
    audio_out     Output to get the hardware type of.
    
RETURNS:
    The hardware type, or AUDIO_HARDWARE_NONE if there was an error.
*/
static audio_hardware getAudioHardwareType(audio_output_t audio_out)
{
    switch (config->mapping[audio_out].endpoint.type)
    {
        case audio_output_type_dac:
            return AUDIO_HARDWARE_CODEC;
        
        case audio_output_type_i2s:
            return AUDIO_HARDWARE_I2S;
        
        case audio_output_type_spdif:
            return AUDIO_HARDWARE_SPDIF;
        
        case audio_output_type_none:
        default:
            /* No mapping or unsupported output type, return error. */
            return AUDIO_HARDWARE_NONE;
    }
}

/******************************************************************************
NAME:
    getAudioHardwareInstance

DESCRIPTION:
    Gets the hardware instance of an audio sink mapped to an output. Must only
    be called after successful initialisation (i.e. 'config' is valid).

PARAMETERS:
    audio_out     Output to get the hardware instance for.
    
RETURNS:
    The hardware instance, or AUDIO_INSTANCE_NONE if there was an error.
*/
static audio_instance getAudioHardwareInstance(audio_output_t audio_out)
{
    switch (config->mapping[audio_out].endpoint.instance)
    {
        case audio_output_hardware_instance_0:
            return AUDIO_INSTANCE_0;
        
        case audio_output_hardware_instance_1:
            return AUDIO_INSTANCE_1;
        
        default:
            /* Control should never reach here, return error. */
            return AUDIO_INSTANCE_NONE;
    }
}

/******************************************************************************
NAME:
    getAudioHardwareChannel

DESCRIPTION:
    Gets the hardware channel of an audio sink mapped to an output. Must only 
    be called after successful initialisation (i.e. 'config' is valid).

PARAMETERS:
    audio_out     Output to get the hardware channel for.
    
RETURNS:
    The hardware channel, or AUDIO_CHANNEL_NONE if there was an error.
*/
static audio_channel getAudioHardwareChannel(audio_output_t audio_out)
{
    /* Channel depends on hardware type */
    switch (config->mapping[audio_out].endpoint.type)
    {
        case audio_output_type_dac:
        {
            switch (config->mapping[audio_out].endpoint.channel)
            {
                case audio_output_channel_a:
                    return AUDIO_CHANNEL_A;
                
                case audio_output_channel_b:
                    return AUDIO_CHANNEL_B;
                
                default:
                break;
            }
        }
        break;
        
        case audio_output_type_i2s:
        {
            switch (config->mapping[audio_out].endpoint.channel)
            {
                case audio_output_channel_a:
                    return AUDIO_CHANNEL_SLOT_0;
                
                case audio_output_channel_b:
                    return AUDIO_CHANNEL_SLOT_1;
                
                default:
                break;
            }
        }
        break;
        
        case audio_output_type_spdif:
        {
            switch (config->mapping[audio_out].endpoint.channel)
            {
                case audio_output_channel_a:
                    return SPDIF_CHANNEL_A;
                    
                case audio_output_channel_b:
                    return SPDIF_CHANNEL_B;
                
                default:
                break;
            }
        }
        break;
        
        case audio_output_type_none:
        default:
        break;
    }
    
    /* No mapping or unsupported output type, return error. */
    return AUDIO_CHANNEL_NONE;
}
