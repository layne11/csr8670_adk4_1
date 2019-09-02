/*
Copyright (c) 2004 - 2016 Qualcomm Technologies International, Ltd.
*/

/*!
@file
@ingroup sink_app
@brief   
    Module responsible for reading multi-channel configuration and other 
    functionality related to multi-channel audio output.
*/

#include "sink_multi_channel.h"
#include "sink_private.h"
#include "sink_config.h"

#include <audio_output.h>

/*============================================================================*
 *  Multi-Channel PSKey
 *
 *  Unlike most PSKeys, the PSKey used to configure the multi-
 *  channel plugin does not map directly to the internal config structure
 *  used by the plugin. The data types contained in this file define the PSKey
 *  format and the functions that use them perform the translation between
 *  the PSKey format and the structure required by the plugin.
 *============================================================================*/

/*============================================================================*
 *  PSKey side data types (denoted by pskey_ prefix)
 *============================================================================*/
typedef enum __pskey_enable_audio_instance
{
	enable_audio_interface_false = 0,
	enable_audio_interface_true
} pskey_enable_audio_interface_t;

typedef enum __pskey_multi_channel_routing
{
	multi_channel_routing_none = 0,
	multi_channel_routing_primary,
	multi_channel_routing_secondary,
	multi_channel_routing_subwoofer,
	multi_channel_routing_aux
} pskey_multi_channel_routing_t;

typedef enum __pskey_output_format_if_digital
{
	output_format_if_digital_i2s = 0,
	output_format_if_digital_spdif
} pskey_output_format_if_digital_t;

typedef enum __pskey_a_b_channels_swapped
{
	a_b_channels_swapped_false = 0,
	a_b_channels_swapped_true
} pskey_a_b_channels_swapped_t;

typedef enum __pskey_enable_right_channel
{
	enable_right_channel_false = 0,
	enable_right_channel_true
} pskey_enable_right_channel_t;

typedef enum __pskey_enable_channel_trims
{
	enable_channel_trims_false = 0,
	enable_channel_trims_true
} pskey_enable_channel_trims_t;

typedef enum __pskey_volume_scaling_method
{
	volume_scaling_method_hardware = 0,
	volume_scaling_method_dsp,
	volume_scaling_method_hybrid
} pskey_volume_scaling_method_t;

typedef enum __pskey_audio_hardware_interface
{
	audio_hardware_interface_digital_0 = 0,
	audio_hardware_interface_digital_1,
	audio_hardware_interface_analogue,
	audio_hardware_interface_max         /* for iteration */
} pskey_audio_hardware_interface_t;

typedef enum __pskey_audio_channel
{
	audio_channel_primary_left = 0,
	audio_channel_primary_right,
	audio_channel_secondary_left,
	audio_channel_secondary_right,
	audio_channel_subwoofer,
	audio_channel_aux_left,
	audio_channel_aux_right,
	audio_channel_max         /* for iteration */
} pskey_audio_channel_t;

/*============================================================================*
 *  Structures that define the multi-channel PSKey (denoted by pskey_ prefix)
 *============================================================================*/
typedef struct __pskey_audio_interface
{
	unsigned unsused:9;
	pskey_output_format_if_digital_t output_format_if_digital:1;
	pskey_enable_right_channel_t enable_right_channel:1;
	pskey_a_b_channels_swapped_t a_b_channels_swapped:1;
	pskey_multi_channel_routing_t multi_channel_routing:3;
	pskey_enable_audio_interface_t enable_audio_interface:1;
} pskey_audio_interface_t;

typedef struct __pskey_volume_and_gain
{
	unsigned unused:11;
	pskey_enable_channel_trims_t enable_channel_trims:1;
	pskey_volume_scaling_method_t volume_scaling_method_aux:2;
	pskey_volume_scaling_method_t volume_scaling_method_main:2;

	int16 fixed_hardware_gain;
	int16 channel_trim[audio_channel_max];
} pskey_volume_and_gain_t;

typedef struct __pskey_multi_channel_config
{
	pskey_audio_interface_t audio_interface[audio_hardware_interface_max];
	pskey_volume_and_gain_t volume_and_gain;
    unsigned unused:15;
    unsigned enable_24_bit_resolution:1;
} pskey_audio_output_config_t;


/*============================================================================*
 *  sinkGetMapped functions return the mapped data from the PSKey structure
 *
 *  sinkPopulate functions write mapped data to the plugin config structure
 *============================================================================*/

static audio_output_hardware_type_t sinkGetMappedHardwareTypeFromPSKey(const pskey_audio_output_config_t* const psKeyConfig,
																			pskey_audio_hardware_interface_t interfaceRequested)
{
	audio_output_hardware_type_t hardware_type = audio_output_type_none;

	if(psKeyConfig->audio_interface[interfaceRequested].enable_audio_interface == enable_audio_interface_true)
	{
		switch(interfaceRequested)
		{
			case audio_hardware_interface_digital_0:
			case audio_hardware_interface_digital_1:
				hardware_type = audio_output_type_i2s;
				if(psKeyConfig->audio_interface[interfaceRequested].output_format_if_digital == output_format_if_digital_spdif)
				{
					hardware_type = audio_output_type_spdif;
				}
				break;
			case audio_hardware_interface_analogue:
				hardware_type = audio_output_type_dac;
				break;
            default:
                hardware_type = audio_output_type_none;
                break;
		}
	}
	return hardware_type;
}

/******************************************************************************/
static audio_output_hardware_instance_t sinkGetMappedHardwareInstanceFromPSKey(const pskey_audio_output_config_t* const psKeyConfig,
																				pskey_audio_hardware_interface_t interfaceRequested)
{
	audio_output_hardware_instance_t hardware_instance = audio_output_hardware_instance_0;

    UNUSED(psKeyConfig);    /* sensible to leave in the API */

    if(interfaceRequested == audio_hardware_interface_digital_1)
    {
        hardware_instance = audio_output_hardware_instance_1;
    }
	return hardware_instance;
}

/******************************************************************************/
static audio_output_channel_t sinkGetMappedLeftChannelFromPSKey(const pskey_audio_output_config_t* const psKeyConfig,
																pskey_audio_hardware_interface_t interfaceRequested)
{
	audio_output_channel_t left_channel = audio_output_channel_a;
	if(psKeyConfig->audio_interface[interfaceRequested].a_b_channels_swapped == a_b_channels_swapped_true)
	{
		left_channel = audio_output_channel_b;
	}
	return left_channel;
}

/******************************************************************************/
static audio_output_channel_t sinkGetMappedRightChannelFromPSKey(const pskey_audio_output_config_t* const psKeyConfig,
																	pskey_audio_hardware_interface_t interfaceRequested)
{
	audio_output_channel_t right_channel = audio_output_channel_b;
	if(psKeyConfig->audio_interface[interfaceRequested].a_b_channels_swapped == a_b_channels_swapped_true)
	{
		right_channel = audio_output_channel_a;
	}
	return right_channel;
}

/******************************************************************************/
static audio_output_t sinkGetMappedLeftMultiChannelOutFromPSKey(const pskey_audio_output_config_t* const psKeyConfig,
																		pskey_audio_hardware_interface_t interfaceRequested)
{
	audio_output_t leftMultiChannelOut = audio_output_primary_left;
	switch(psKeyConfig->audio_interface[interfaceRequested].multi_channel_routing)
	{
		case multi_channel_routing_secondary:
			leftMultiChannelOut = audio_output_secondary_left;
			break;
		case multi_channel_routing_subwoofer:
			leftMultiChannelOut = audio_output_wired_sub;
			break;
		case multi_channel_routing_aux:
			leftMultiChannelOut = audio_output_aux_left;
			break;
        case multi_channel_routing_primary:
        default:
            leftMultiChannelOut = audio_output_primary_left;
            break;
	}
	return leftMultiChannelOut;
}

/******************************************************************************/
static audio_output_t sinkGetMappedRightMultiChannelOutFromPSKey(const pskey_audio_output_config_t* const psKeyConfig,
																		pskey_audio_hardware_interface_t interfaceRequested)
{
	audio_output_t rightMultiChannelOut = audio_output_primary_right;
	switch(psKeyConfig->audio_interface[interfaceRequested].multi_channel_routing)
	{
		case multi_channel_routing_secondary:
			rightMultiChannelOut = audio_output_secondary_right;
			break;
		case multi_channel_routing_aux:
			rightMultiChannelOut = audio_output_aux_right;
			break;
        case multi_channel_routing_primary:
        default:
            rightMultiChannelOut = audio_output_primary_right;
            break;
	}
	return rightMultiChannelOut;
}

/******************************************************************************/
static int16 sinkGetMappedVolumeTrimFromPSKey(const pskey_audio_output_config_t* const psKeyConfig,
										pskey_audio_channel_t channelRequested)
{
	int16 channelTrim = 0;
	if(psKeyConfig->volume_and_gain.enable_channel_trims == enable_channel_trims_true)
	{
		channelTrim = psKeyConfig->volume_and_gain.channel_trim[channelRequested];
	}
	return channelTrim;
}

/******************************************************************************/
static int16 sinkGetMappedLeftChannelVolumeTrimFromPSKey(const pskey_audio_output_config_t* const psKeyConfig,
													pskey_audio_hardware_interface_t interfaceRequested)
{
	pskey_audio_channel_t leftChannelOfInterfaceRequested = audio_channel_primary_left;
	switch(psKeyConfig->audio_interface[interfaceRequested].multi_channel_routing)
	{
		case multi_channel_routing_secondary:
			leftChannelOfInterfaceRequested = audio_channel_secondary_left;
			break;
		case multi_channel_routing_aux:
			leftChannelOfInterfaceRequested = audio_channel_aux_left;
			break;
        case multi_channel_routing_primary:
        default:
            leftChannelOfInterfaceRequested = audio_channel_primary_left;
            break;
	}
	return sinkGetMappedVolumeTrimFromPSKey(psKeyConfig, leftChannelOfInterfaceRequested);
}

/******************************************************************************/
static int16 sinkGetMappedRightChannelVolumeTrimFromPSKey(const pskey_audio_output_config_t* const psKeyConfig,
													pskey_audio_hardware_interface_t interfaceRequested)
{
	pskey_audio_channel_t rightChannelOfInterfaceRequested = audio_channel_primary_right;
	switch(psKeyConfig->audio_interface[interfaceRequested].multi_channel_routing)
	{
		case multi_channel_routing_secondary:
			rightChannelOfInterfaceRequested = audio_channel_secondary_right;
			break;
		case multi_channel_routing_aux:
			rightChannelOfInterfaceRequested = audio_channel_aux_right;
			break;
        case multi_channel_routing_primary:
        default:
            rightChannelOfInterfaceRequested = audio_channel_primary_right;
            break;
	}
	return sinkGetMappedVolumeTrimFromPSKey(psKeyConfig, rightChannelOfInterfaceRequested);
}

/******************************************************************************/
static audio_output_gain_type_t sinkGetMappedGroupGainTypeFromPSKey(pskey_volume_scaling_method_t volume_scaling_method)
{
	audio_output_gain_type_t groupGain = audio_output_gain_hardware;
	switch(volume_scaling_method)
	{
		case volume_scaling_method_hardware:
			groupGain = audio_output_gain_hardware;
			break;
		case volume_scaling_method_dsp:
			groupGain = audio_output_gain_digital;
			break;
		case volume_scaling_method_hybrid:
			groupGain = audio_output_gain_hybrid;
			break;
	}
	return groupGain;
}

/******************************************************************************/
static audio_output_gain_type_t sinkGetMappedMainGroupGainTypeFromPSKey(const pskey_audio_output_config_t* const psKeyConfig)
{
	return sinkGetMappedGroupGainTypeFromPSKey(psKeyConfig->volume_and_gain.volume_scaling_method_main);
}

/******************************************************************************/
static audio_output_gain_type_t sinkGetMappedAuxGroupGainTypeFromPSKey(const pskey_audio_output_config_t* const psKeyConfig)
{
	return sinkGetMappedGroupGainTypeFromPSKey(psKeyConfig->volume_and_gain.volume_scaling_method_aux);
}

/******************************************************************************/
static int16 sinkGetMappedFixedHardwareGainFromPSKey(const pskey_audio_output_config_t* const psKeyConfig)
{
	return psKeyConfig->volume_and_gain.fixed_hardware_gain;
}

/******************************************************************************/
static audio_output_resolution_t sinkGetMappedOutputResolutionFromPSKey(const pskey_audio_output_config_t* const psKeyConfig)
{
	return psKeyConfig->enable_24_bit_resolution;
}

/******************************************************************************/
static void sinkPopulateMultiChannelHardwareMappingStructure(const pskey_audio_output_config_t* const psKeyConfig,
															pskey_audio_hardware_interface_t interfaceRequested,
															audio_output_mapping_t * const toMap)
{
	/* Type and instance, common across left and right channels */
	toMap->endpoint.type = sinkGetMappedHardwareTypeFromPSKey(psKeyConfig, interfaceRequested);
	toMap->endpoint.instance = sinkGetMappedHardwareInstanceFromPSKey(psKeyConfig, interfaceRequested);
}


/******************************************************************************/
static void sinkPopulateMultiChannelHardwareMappingStructureLeft(const pskey_audio_output_config_t* const psKeyConfig,
																	pskey_audio_hardware_interface_t interfaceRequested,
																	audio_output_config_t* const conf)
{
	audio_output_mapping_t * configToMap = &conf->mapping[sinkGetMappedLeftMultiChannelOutFromPSKey(psKeyConfig, interfaceRequested)];

	/* Map data common to left and right channels */
	sinkPopulateMultiChannelHardwareMappingStructure(psKeyConfig, interfaceRequested, configToMap);

	/* Map channel specific data */
	configToMap->endpoint.channel = sinkGetMappedLeftChannelFromPSKey(psKeyConfig, interfaceRequested);
	configToMap->volume_trim = sinkGetMappedLeftChannelVolumeTrimFromPSKey(psKeyConfig, interfaceRequested);
}

/******************************************************************************/
static void sinkPopulateMultiChannelHardwareMappingStructureRight(const pskey_audio_output_config_t* const psKeyConfig,
																	pskey_audio_hardware_interface_t interfaceRequested,
																	audio_output_config_t* const conf)
{
	audio_output_mapping_t * configToMap = &conf->mapping[sinkGetMappedRightMultiChannelOutFromPSKey(psKeyConfig, interfaceRequested)];

	/* Right channel is not valid on subwoofer routing, ignore */
	if(psKeyConfig->audio_interface[interfaceRequested].multi_channel_routing == multi_channel_routing_subwoofer)
	{
		return;
	}

	/* Map data common to left and right channels */
	sinkPopulateMultiChannelHardwareMappingStructure(psKeyConfig, interfaceRequested, configToMap);

	/* Map channel specific data */
	configToMap->endpoint.channel = sinkGetMappedRightChannelFromPSKey(psKeyConfig, interfaceRequested);
	configToMap->volume_trim = sinkGetMappedRightChannelVolumeTrimFromPSKey(psKeyConfig, interfaceRequested);
}

/******************************************************************************/
static bool sinkIsInterfaceConfiguredForPrimaryOrSecondary(const pskey_audio_interface_t * const interface)
{
	if(interface->enable_audio_interface == enable_audio_interface_true)
	{
		if(interface->multi_channel_routing == multi_channel_routing_primary ||
			interface->multi_channel_routing == multi_channel_routing_secondary)
		{
			return TRUE;
		}
	}
	return FALSE;
}

/******************************************************************************/
static bool sinkIsInterfaceMultichannelRoutingDuplicated(const pskey_audio_interface_t * const interface1,
															const pskey_audio_interface_t * const interface2)
{
	if(interface1->enable_audio_interface && interface2->enable_audio_interface)
	{
		if(interface1->multi_channel_routing == interface2->multi_channel_routing)
		{
			return TRUE;
		}
	}
	return FALSE;
}

/******************************************************************************/
static bool sinkIsPSKeyMultichhannelRoutingValid(const pskey_audio_output_config_t* const psKeyConfig)
{
	/* Check a single audio instance is not routed to multiple interfaces */
	if(sinkIsInterfaceMultichannelRoutingDuplicated(&psKeyConfig->audio_interface[audio_hardware_interface_digital_0],
				&psKeyConfig->audio_interface[audio_hardware_interface_digital_1]))
	{
		return FALSE;
	}

	if(sinkIsInterfaceMultichannelRoutingDuplicated(&psKeyConfig->audio_interface[audio_hardware_interface_digital_0],
				&psKeyConfig->audio_interface[audio_hardware_interface_analogue]))
	{
		return FALSE;
	}

	if(sinkIsInterfaceMultichannelRoutingDuplicated(&psKeyConfig->audio_interface[audio_hardware_interface_digital_1],
				&psKeyConfig->audio_interface[audio_hardware_interface_analogue]))
	{
		return FALSE;
	}
	return TRUE;
}

/******************************************************************************/
static bool sinkIsPSKeyMultichannelGroupingValid(const pskey_audio_output_config_t* const psKeyConfig)
{
	/* Check that primary and secondary are not split over interface types */
	if(sinkIsInterfaceConfiguredForPrimaryOrSecondary(&psKeyConfig->audio_interface[audio_hardware_interface_analogue]))
	{
		if(sinkIsInterfaceConfiguredForPrimaryOrSecondary(&psKeyConfig->audio_interface[audio_hardware_interface_digital_0]))
		{
			return FALSE;
		}
		if(sinkIsInterfaceConfiguredForPrimaryOrSecondary(&psKeyConfig->audio_interface[audio_hardware_interface_digital_1]))
		{
			return FALSE;
		}
	}
	return TRUE;
}

/*============================================================================*
 *  The multi channel section of the sink config tool has been designed so
 *  that it's pretty difficult to specify an invalid configuration. This
 *  deals with those things that couldn't be guarded against.
 *============================================================================*/
static void sinkVerifyPSKeyConfigIsValid(const pskey_audio_output_config_t* const psKeyConfig)
{
	if((sinkIsPSKeyMultichhannelRoutingValid(psKeyConfig) == FALSE) ||
	        (sinkIsPSKeyMultichannelGroupingValid(psKeyConfig) == FALSE))
	{
	    ConfigIndicateConfigError(CONFIG_MULTI_CHANNEL_AUDIO);
	}
}

/******************************************************************************/
static void sinkPopulateMultiChannelConfigStructure(const pskey_audio_output_config_t* const psKeyConfig,
													audio_output_config_t* const conf)
{
	pskey_audio_hardware_interface_t loopInterface;

	/* Check here for invalid PSKey configuration */
	sinkVerifyPSKeyConfigIsValid(psKeyConfig);

	for(loopInterface=audio_hardware_interface_digital_0; loopInterface < audio_hardware_interface_max; loopInterface++)
	{
		/* if interface is enabled and a routing has been configured */
		if(psKeyConfig->audio_interface[loopInterface].enable_audio_interface &&
				psKeyConfig->audio_interface[loopInterface].multi_channel_routing)
		{
			sinkPopulateMultiChannelHardwareMappingStructureLeft(psKeyConfig, loopInterface, conf);
			if(psKeyConfig->audio_interface[loopInterface].enable_right_channel == enable_right_channel_true)
			{
				sinkPopulateMultiChannelHardwareMappingStructureRight(psKeyConfig, loopInterface, conf);
			}
		}
	}
	/* map fixed hardware gain */
	conf->fixed_hw_gain = sinkGetMappedFixedHardwareGainFromPSKey(psKeyConfig);
   /* map output audio bit resolution */
   conf->output_resolution_mode = sinkGetMappedOutputResolutionFromPSKey(psKeyConfig);

	/* map volume scaling method for each group */
	conf->gain_type[audio_output_group_main] = sinkGetMappedMainGroupGainTypeFromPSKey(psKeyConfig);
	conf->gain_type[audio_output_group_aux] = sinkGetMappedAuxGroupGainTypeFromPSKey(psKeyConfig);
}

/******************************************************************************/
static void sinkPopulateMultiChannelConfigStructureFromPSKey(audio_output_config_t* const conf)
{
    pskey_audio_output_config_t* const psKeyConfig = PanicUnlessNew(pskey_audio_output_config_t);

	/* Read multi-channel user configuration key */
	(void)ConfigRetrieve(CONFIG_MULTI_CHANNEL_AUDIO, psKeyConfig , sizeof(*psKeyConfig));

	/* Populate the plugin config from the PSKey contents */
	sinkPopulateMultiChannelConfigStructure(psKeyConfig, conf);

	free(psKeyConfig);
}

/******************************************************************************/
void SinkMultiChannelReadConfig(void)
{
    audio_output_config_t* config = PanicUnlessNew(audio_output_config_t);
    memset(config, 0, sizeof(*config));

    /* Read multi-channel configuration key and populate config */
    sinkPopulateMultiChannelConfigStructureFromPSKey(config);

    /* Initialise multi-channel plugin */
    PanicFalse(AudioOutputInit(config));
}
