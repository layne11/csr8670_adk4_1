/*
Copyright (c) 2004 - 2016 Qualcomm Technologies International, Ltd.
*/

/*!
@file
@ingroup sink_app
@brief   a2dp initialisation and control functions
*/

#include "sink_private.h"
#include "sink_statemanager.h"
#include "sink_states.h"
#include "sink_a2dp.h"
#include "sink_devicemanager.h"
#include "sink_link_policy.h"
#include "sink_audio.h"
#include "sink_usb.h"
#include "sink_wired.h"
#include "sink_scan.h"
#include "sink_audio_routing.h"
#include "sink_slc.h"
#include "sink_device_id.h"
#include "sink_partymode.h"
#include "sink_config.h"
#include "sink_auth.h"
#include "sink_peer.h"
#include "sink_avrcp.h"
#include "sink_peer_qualification.h"
#include "sink_linkloss.h"
#include "sink_callmanager.h"
#ifdef ENABLE_GAIA
#include "sink_gaia.h"
#endif /* ENABLE_GAIA*/

#ifdef ENABLE_AVRCP
#include "sink_tones.h"
#endif        

#include <util.h>
#include <bdaddr.h>
#include <a2dp.h>
#include <connection.h>
#include <hfp.h>
#include <stdlib.h>
#include <panic.h>
#include <ps.h>
#include <message.h>
#include <kalimba.h>
#include <audio_plugin_music_variants.h>

#include "sink_slc.h"

#ifdef DEBUG_A2DP
#define A2DP_DEBUG(x) DEBUG(x)
#ifdef ENABLE_PEER
static const char * const rdname[] = { "Unknown", "Non-Peer", "Peer" };
#endif
#else
#define A2DP_DEBUG(x) 
#endif

extern const uint8 sbc_caps_sink[16];
extern const uint8 mp3_caps_sink[16];
extern const uint8 aac_caps_sink[18];
extern const uint8 faststream_caps_sink[14];
extern const uint8 aptx_caps_sink[19];
extern const uint8 aptx_acl_sprint_caps_sink[27];
extern const uint8 aptxhd_caps_sink[23];
extern const uint8 tws_sbc_caps[26];
extern const uint8 tws_mp3_caps[26];
extern const uint8 tws_aac_caps[28];
extern const uint8 tws_aptx_caps[29];

#ifdef ENABLE_PEER
#define SBC_SAMPLING_FREQ_16000        128
#define SBC_SAMPLING_FREQ_32000         64
#define SBC_SAMPLING_FREQ_44100         32
#define SBC_SAMPLING_FREQ_48000         16
#define SBC_CHANNEL_MODE_MONO            8
#define SBC_CHANNEL_MODE_DUAL_CHAN       4
#define SBC_CHANNEL_MODE_STEREO          2
#define SBC_CHANNEL_MODE_JOINT_STEREO    1

#define SBC_BLOCK_LENGTH_4             128
#define SBC_BLOCK_LENGTH_8              64
#define SBC_BLOCK_LENGTH_12             32
#define SBC_BLOCK_LENGTH_16             16
#define SBC_SUBBANDS_4                   8
#define SBC_SUBBANDS_8                   4
#define SBC_ALLOCATION_SNR               2
#define SBC_ALLOCATION_LOUDNESS          1

#define SBC_BITPOOL_MIN                  2
#define SBC_BITPOOL_MAX                250
#define SBC_BITPOOL_MEDIUM_QUALITY      35
#define SBC_BITPOOL_HIGH_QUALITY        53

/* Codec caps to use for a TWS Source SEP when 16KHz SBC is required */
const uint8 sbc_caps_16k[8] = 
{
    AVDTP_SERVICE_MEDIA_CODEC,
    6,
    AVDTP_MEDIA_TYPE_AUDIO<<2,
    AVDTP_MEDIA_CODEC_SBC,

    SBC_SAMPLING_FREQ_16000 |  SBC_CHANNEL_MODE_JOINT_STEREO,

    SBC_BLOCK_LENGTH_16 | SBC_SUBBANDS_8 | SBC_ALLOCATION_SNR,

    SBC_BITPOOL_MIN,
    SBC_BITPOOL_HIGH_QUALITY
};


/* Codec caps to use for a TWS Source SEP when 32KHz SBC is required */
const uint8 sbc_caps_32k[8] = 
{
    AVDTP_SERVICE_MEDIA_CODEC,
    6,
    AVDTP_MEDIA_TYPE_AUDIO<<2,
    AVDTP_MEDIA_CODEC_SBC,

    SBC_SAMPLING_FREQ_32000 |  SBC_CHANNEL_MODE_JOINT_STEREO,

    SBC_BLOCK_LENGTH_16 | SBC_SUBBANDS_8 | SBC_ALLOCATION_SNR,

    SBC_BITPOOL_MIN,
    SBC_BITPOOL_HIGH_QUALITY
};


/* Codec caps to use for a TWS Source SEP when 44.1KHz SBC is required */
const uint8 sbc_caps_44k1[8] = 
{
    AVDTP_SERVICE_MEDIA_CODEC,
    6,
    AVDTP_MEDIA_TYPE_AUDIO<<2,
    AVDTP_MEDIA_CODEC_SBC,

    SBC_SAMPLING_FREQ_44100 |  SBC_CHANNEL_MODE_JOINT_STEREO,

    SBC_BLOCK_LENGTH_16 | SBC_SUBBANDS_8 | SBC_ALLOCATION_SNR,

    SBC_BITPOOL_MIN,
    SBC_BITPOOL_HIGH_QUALITY
};


/* Codec caps to use for a TWS Source SEP when 48KHz SBC is required */
const uint8 sbc_caps_48k[8] = 
{
    AVDTP_SERVICE_MEDIA_CODEC,
    6,
    AVDTP_MEDIA_TYPE_AUDIO<<2,
    AVDTP_MEDIA_CODEC_SBC,

    SBC_SAMPLING_FREQ_48000 |  SBC_CHANNEL_MODE_JOINT_STEREO,

    SBC_BLOCK_LENGTH_16 | SBC_SUBBANDS_8 | SBC_ALLOCATION_SNR,

    SBC_BITPOOL_MIN,
    SBC_BITPOOL_HIGH_QUALITY
};
#endif

static const sep_config_type sbc_sep_snk = { SBC_SEID, KALIMBA_RESOURCE_ID, sep_media_type_audio, a2dp_sink, 1, 0, sizeof(sbc_caps_sink), sbc_caps_sink };
#ifdef ENABLE_PEER
	static const sep_config_type sbc_sep_src = { SOURCE_SEID_MASK | SBC_SEID, KALIMBA_RESOURCE_ID, sep_media_type_audio, a2dp_source, 0, 0, sizeof(sbc_caps_sink), sbc_caps_sink };   /* Source shares same caps as sink */
	static const sep_config_type tws_sbc_sep_snk = { TWS_SEID_MASK | SBC_SEID, KALIMBA_RESOURCE_ID, sep_media_type_audio, a2dp_sink, 1, 0, sizeof(tws_sbc_caps), tws_sbc_caps };
	static const sep_config_type tws_sbc_sep_src = { SOURCE_SEID_MASK | TWS_SEID_MASK | SBC_SEID, KALIMBA_RESOURCE_ID, sep_media_type_audio, a2dp_source, 0, 0, sizeof(tws_sbc_caps), tws_sbc_caps };
#endif

/* not all codecs are available for some configurations, include this define to have access to all codec types  */
#ifdef INCLUDE_A2DP_EXTRA_CODECS
    static const sep_config_type mp3_sep_snk = { MP3_SEID, KALIMBA_RESOURCE_ID, sep_media_type_audio, a2dp_sink, 1, 0, sizeof(mp3_caps_sink), mp3_caps_sink };
    static const sep_config_type aac_sep_snk = { AAC_SEID, KALIMBA_RESOURCE_ID, sep_media_type_audio, a2dp_sink, 1, 0, sizeof(aac_caps_sink), aac_caps_sink };
    static const sep_config_type aptx_sep_snk = { APTX_SEID, KALIMBA_RESOURCE_ID, sep_media_type_audio, a2dp_sink, 1, 0, sizeof(aptx_caps_sink), aptx_caps_sink };
    static const sep_config_type aptxhd_sep_snk = { APTXHD_SEID, KALIMBA_RESOURCE_ID, sep_media_type_audio, a2dp_sink, 1, 0, sizeof(aptxhd_caps_sink), aptxhd_caps_sink };
#ifdef ENABLE_PEER
#ifdef PEER_AS
    static const sep_config_type mp3_sep_src = { SOURCE_SEID_MASK | MP3_SEID, KALIMBA_RESOURCE_ID, sep_media_type_audio, a2dp_source, 0, 0, sizeof(mp3_caps_sink), mp3_caps_sink };   /* Source shares same caps as sink */
    static const sep_config_type aac_sep_src = { SOURCE_SEID_MASK | AAC_SEID, KALIMBA_RESOURCE_ID, sep_media_type_audio, a2dp_source, 0, 0, sizeof(aac_caps_sink), aac_caps_sink };   /* Source shares same caps as sink */
    static const sep_config_type aptx_sep_src = { SOURCE_SEID_MASK | APTX_SEID, KALIMBA_RESOURCE_ID, sep_media_type_audio, a2dp_source, 0, 0, sizeof(aptx_caps_sink), aptx_caps_sink };   /* Source shares same caps as sink */
#endif /* PEER_AS */
    static const sep_config_type tws_mp3_sep_snk = { TWS_SEID_MASK | MP3_SEID, KALIMBA_RESOURCE_ID, sep_media_type_audio, a2dp_sink, 1, 0, sizeof(tws_mp3_caps), tws_mp3_caps };
    static const sep_config_type tws_mp3_sep_src = { SOURCE_SEID_MASK | TWS_SEID_MASK | MP3_SEID, KALIMBA_RESOURCE_ID, sep_media_type_audio, a2dp_source, 0, 0, sizeof(tws_mp3_caps), tws_mp3_caps };
    static const sep_config_type tws_aac_sep_snk = { TWS_SEID_MASK | AAC_SEID, KALIMBA_RESOURCE_ID, sep_media_type_audio, a2dp_sink, 1, 0, sizeof(tws_aac_caps), tws_aac_caps };
    static const sep_config_type tws_aac_sep_src = { SOURCE_SEID_MASK | TWS_SEID_MASK | AAC_SEID, KALIMBA_RESOURCE_ID, sep_media_type_audio, a2dp_source, 0, 0, sizeof(tws_aac_caps), tws_aac_caps };
    static const sep_config_type tws_aptx_sep_snk = { TWS_SEID_MASK | APTX_SEID, KALIMBA_RESOURCE_ID, sep_media_type_audio, a2dp_sink, 1, 0, sizeof(tws_aptx_caps), tws_aptx_caps };
    static const sep_config_type tws_aptx_sep_src = { SOURCE_SEID_MASK | TWS_SEID_MASK | APTX_SEID, KALIMBA_RESOURCE_ID, sep_media_type_audio, a2dp_source, 0, 0, sizeof(tws_aptx_caps), tws_aptx_caps };
#endif /* ENABLE_PEER */
#ifdef INCLUDE_FASTSTREAM                
    static const sep_config_type faststream_sep = { FASTSTREAM_SEID, KALIMBA_RESOURCE_ID, sep_media_type_audio, a2dp_sink, 1, 0, sizeof(faststream_caps_sink), faststream_caps_sink };
#endif /* INCLUDE_FASTSTREAM */
#ifdef INCLUDE_APTX_ACL_SPRINT
    static const sep_config_type aptx_sprint_sep = { APTX_SPRINT_SEID, KALIMBA_RESOURCE_ID, sep_media_type_audio, a2dp_sink, 1, 0, sizeof(aptx_acl_sprint_caps_sink), aptx_acl_sprint_caps_sink };
#endif /* INCLUDE_APTX_ACL_SPRINT */
#endif /* INCLUDE_A2DP_EXTRA_CODECS */


#define NUM_SEPS (sizeof(codecList)/sizeof(codec_list_element))

typedef struct
{
    unsigned                enable_bit:8;      /* The bit position in CONFIG_USR_xxx to enable the codec. */
    unsigned                include_mask:8;    /* Bit mask to include a codec based on application feature set */
    const sep_config_type   *config;    /* The SEP config data. These configs are defined above. */
    const TaskData          *plugin;    /* The audio plugin to use. */
} codec_list_element;

    /* Table which indicates which A2DP codecs are avaiable on the device.
       Add other codecs in priority order, from top to bottom of the table.
    */
    static const codec_list_element codecList[] = 
    {
#ifdef INCLUDE_A2DP_EXTRA_CODECS
#ifdef INCLUDE_APTX_ACL_SPRINT
		/* AptX Sprint (low latency) Sink SEP */
        {APTX_SPRINT_CODEC_BIT, STD_SNK, &aptx_sprint_sep, &csr_aptx_acl_sprint_decoder_plugin.data},
#endif
		/* High quality Sink SEPs */
        {APTXHD_CODEC_BIT, STD_SNK, &aptxhd_sep_snk, &csr_aptxhd_decoder_plugin.data},
        {APTX_CODEC_BIT, STD_SNK, &aptx_sep_snk, &csr_aptx_decoder_plugin.data},
        {AAC_CODEC_BIT, STD_SNK, &aac_sep_snk, &csr_aac_decoder_plugin.data},
        {MP3_CODEC_BIT, STD_SNK, &mp3_sep_snk, &csr_mp3_decoder_plugin.data},
#ifdef ENABLE_PEER
#ifdef PEER_AS
		/* ShareMe Source SEPs */
        {APTX_CODEC_BIT, STD_SRC, &aptx_sep_src, &csr_aptx_decoder_plugin.data},
        {AAC_CODEC_BIT, STD_SRC, &aac_sep_src, &csr_aac_decoder_plugin.data},
        {MP3_CODEC_BIT, STD_SRC, &mp3_sep_src, &csr_mp3_decoder_plugin.data},
#endif
#ifdef PEER_TWS		
		/* TWS Source and Sink SEPs */ /* TODO: These could be moved as they don't need to be part of default search order - create lists for when using A2dpMedisOpenReq() ? */
        {APTX_CODEC_BIT, TWS_SRC, &tws_aptx_sep_src, &csr_tws_aptx_decoder_plugin.data},
        {APTX_CODEC_BIT, TWS_SNK, &tws_aptx_sep_snk, &csr_tws_aptx_decoder_plugin.data},
        {AAC_CODEC_BIT, TWS_SRC, &tws_aac_sep_src, &csr_tws_aac_decoder_plugin.data},
        {AAC_CODEC_BIT, TWS_SNK, &tws_aac_sep_snk, &csr_tws_aac_decoder_plugin.data},
        {MP3_CODEC_BIT, TWS_SRC, &tws_mp3_sep_src, &csr_tws_mp3_decoder_plugin.data},
        {MP3_CODEC_BIT, TWS_SNK, &tws_mp3_sep_snk, &csr_tws_mp3_decoder_plugin.data},
#endif
#endif
#ifdef INCLUDE_FASTSTREAM                
		/* Faststream Sink SEP */
        {FASTSTREAM_CODEC_BIT, STD_SNK, &faststream_sep, &csr_faststream_sink_plugin.data},
#endif
#endif
#ifdef ENABLE_PEER
#ifdef PEER_TWS
		/* TWS Source and Sink SEPs */

        {SBC_CODEC_BIT, TWS_SRC, &tws_sbc_sep_src, &csr_tws_sbc_decoder_plugin.data},
        {SBC_CODEC_BIT, TWS_SNK, &tws_sbc_sep_snk, &csr_tws_sbc_decoder_plugin.data},
#endif		
		/* Mandatory SBC Source SEP for both ShareMe and TWS */
        {SBC_CODEC_BIT, STD_SRC | TWS_SRC, &sbc_sep_src, &csr_sbc_decoder_plugin.data},
#endif
		/* Mandatory SBC Sink SEP */
        {SBC_CODEC_BIT, STD_SNK | TWS_SNK, &sbc_sep_snk, &csr_sbc_decoder_plugin.data}
    };

typedef struct 
{
    unsigned codec_bit:8;
    unsigned codec_seid:8;
}codec_seid_t;

/*This array maps the codec bits to SEID */
static const codec_seid_t optional_codec_seids[] = 
{
    {1 << MP3_CODEC_BIT         , MP3_SEID} ,
        
    {1 << AAC_CODEC_BIT         , AAC_SEID} ,
          
    {1 << FASTSTREAM_CODEC_BIT  , FASTSTREAM_SEID} ,  

    {1 << APTX_CODEC_BIT        , APTX_SEID} ,        
    
    {1 << APTX_SPRINT_CODEC_BIT , APTX_SPRINT_SEID},

    {0                          , INVALID_SEID}
};

#define TWS_SUPPORTED_CODECS ((1 << MP3_CODEC_BIT) | (1 << AAC_CODEC_BIT) | (1 << APTX_CODEC_BIT))

/* Extra time delay between a peer connecting and paging the next device in the pdl.
   It is to allow the peer relay state to change to streaming before we check if
   we can connect to the next device in the pdl. */
#define A2DP_PEER_PDL_PAGING_DELAY D_SEC(2)

#ifdef ENABLE_PEER
/* After a timeout try to force a paused non-peer a2dp device to suspend its
   a2dp media channel. This is to avoid dropouts on the peer a2dp stream
   if it starts playing before the device has suspended its 
   media channel itself. */
#define A2DP_PAUSE_SUSPEND_TIMER D_SEC(4)
#endif

/****************************************************************************
  FUNCTIONS
*/

#if 0
/* Calculates Volume = (Gain / Scale) * Range
   Gain is 0..Scale, thus Gain/Scale is 0..1  and hence Volume is 0..Range
   
   Method of calculation keeps everything as 16-bit arithmetic
*/
#define HI(n) ((uint8)((n) >> 8))
#define LO(n) ((uint8)(n))
static uint16 calculateVolume (uint16 gain, uint16 scale, uint16 range)
{
    volume = (HI(gain) * HI(range)) / HI(scale);
    volume += (HI(gain) * LO(range)) / HI(scale);
    volume += (LO(gain) * HI(range)) / LO(scale);
    volume += (LO(gain) * LO(range)) / LO(scale);
    
    return volume;
}
#endif


/*************************************************************************
NAME    
    a2dpIsIndexPeer
    
DESCRIPTION
    Determines whether the provided a2dp index is of a peer device or not.

RETURNS
    TRUE if the index is that of a peer device, false otherwise.
    
**************************************************************************/
#ifdef ENABLE_PEER

bool a2dpIsIndexPeer(uint16 index)
{
    if(theSink.a2dp_link_data && (theSink.a2dp_link_data->peer_device[index] == remote_device_peer))
    {
        return TRUE;
    }
    return FALSE;
}

#endif
/*************************************************************************
NAME    
    getCodecIncludeMask
    
DESCRIPTION
    Determines mask of codec types to use based on a set of application 
    features, namely whether TWS and/or ShareMe are supported and in which 
    AVDTP roles (Source or Sink).

RETURNS
    Bitmask of codec types to use
    
**************************************************************************/
static uint8 getCodecIncludeMask (void)
{
    uint8 include_mask;
    
    /* Always include standard Sink SEPs */
    include_mask = STD_SNK;
        
#ifdef ENABLE_PEER
    if (theSink.features.ShareMeSource)
    {
        include_mask |= STD_SRC;
    }

    if (theSink.features.TwsSink)
    {
        include_mask |= TWS_SNK;
    }
    
    if (theSink.features.TwsSource)
    {
        include_mask |= TWS_SRC;
    }
#endif

    return include_mask;
}

/*************************************************************************
NAME    
    findCurrentA2dpSource
    
DESCRIPTION
    Attempts to obtain index of a connected A2dp Source that has established
    a media channel

RETURNS
    TRUE if found, FALSE otherwise

**************************************************************************/
bool findCurrentA2dpSource (a2dp_link_priority* priority)
{
    PEER_DEBUG(("findCurrentA2dpSource\n"));
    
    if (theSink.a2dp_link_data)
    {
        uint16 i;
        for (i = 0; i<MAX_A2DP_CONNECTIONS; i++)
        {
            PEER_DEBUG(("... pri:%u\n", i));
            
            if ( theSink.a2dp_link_data->connected[i] )
            {   /* Found a connected device */
                uint8 device_id = theSink.a2dp_link_data->device_id[i];
                uint8 stream_id = theSink.a2dp_link_data->stream_id[i];
                
                PEER_DEBUG(("...... dev:%u str:%u state:%u\n", device_id, stream_id, A2dpMediaGetState(device_id, stream_id)));
            
                switch ( A2dpMediaGetState(device_id, stream_id) )
                {
                case a2dp_stream_opening:
                case a2dp_stream_open:
                case a2dp_stream_starting:
                case a2dp_stream_streaming:
                case a2dp_stream_suspending:
                    PEER_DEBUG(("......... role:%u\n",A2dpMediaGetRole(device_id, stream_id)));
                    if ( A2dpMediaGetRole(device_id, stream_id)==a2dp_sink )
                    {   /* We have a sink endpoint active to the remote device, therefore it is a source */
                        PEER_DEBUG(("............ found sink\n"));
                        
                        if (priority != NULL)
                        {
                            *priority = (a2dp_link_priority)i;
                        }
                        return TRUE;
                    }
                    break;
                    
                default:
                    break;
                }
            }
        }
    }
    
    return FALSE;
}

/*************************************************************************
NAME    
    findCurrentStreamingA2dpSource
    
DESCRIPTION
    Attempts to obtain index of a connected A2dp Source that has established
    a media channel and is streaming

RETURNS
    TRUE if found, FALSE otherwise

**************************************************************************/
bool findCurrentStreamingA2dpSource (a2dp_link_priority* priority)
{
    PEER_DEBUG(("findCurrentStreamingA2dpSource\n"));
    
    if (theSink.a2dp_link_data)
    {
        uint16 i;
        for (i = 0; i<MAX_A2DP_CONNECTIONS; i++)
        {
            PEER_DEBUG(("... pri:%u\n", i));
            
            if ( theSink.a2dp_link_data->connected[i] )
            {   /* Found a connected device */
                uint8 device_id = theSink.a2dp_link_data->device_id[i];
                uint8 stream_id = theSink.a2dp_link_data->stream_id[i];
                
                PEER_DEBUG(("...... dev:%u str:%u state:%u\n", device_id, stream_id, A2dpMediaGetState(device_id, stream_id)));
            
                switch ( A2dpMediaGetState(device_id, stream_id) )
                {
                case a2dp_stream_starting:
                case a2dp_stream_streaming:
                case a2dp_stream_suspending:
                    PEER_DEBUG(("......... role:%u\n",A2dpMediaGetRole(device_id, stream_id)));
                    if ( A2dpMediaGetRole(device_id, stream_id)==a2dp_sink )
                    {   /* We have a sink endpoint active to the remote device, therefore it is a source */
                        PEER_DEBUG(("............ found sink\n"));
                        
                        if (priority != NULL)
                        {
                            *priority = (a2dp_link_priority)i;
                        }
                        return TRUE;
                    }
                    break;
                    
                default:
                    break;
                }
            }
        }
    }
    
    return FALSE;
}

#ifdef ENABLE_PEER

/*************************************************************************
NAME    
    reconnectAvSource
    
DESCRIPTION
    If the SEID negotiated with the currently connected AV source is disabled by the sink device, 
    then attempt to reconnect to the AV source.
    
RETURNS
    
**************************************************************************/
static void reconnectAvSource(void)
{
    uint16 av_id;
    uint8  seid;
    a2dp_sep_status sep_status;
    a2dp_link_priority priority;
    
    if(a2dpGetSourceIndex(&av_id))
    {
        seid = theSink.a2dp_link_data->seid[av_id];
        sep_status = A2dpCodecGetAvailable(theSink.a2dp_link_data->device_id[av_id] , seid);
        
        if(theSink.features.ReconnectAgOnPeerConnection || 
          ((sep_status != A2DP_SEP_ERROR) && (sep_status & A2DP_SEP_UNAVAILABLE)))
        {
            A2DP_DEBUG(("A2DP: reconnectAvSource because AG seid = 0x%x\n", seid)); 

            if(findCurrentStreamingA2dpSource(&priority) && (priority == av_id))
            {
                theSink.a2dp_link_data->reconnected_ag_address = theSink.a2dp_link_data->bd_addr[av_id];                
            }
            else
            {
                BdaddrSetZero(&theSink.a2dp_link_data->reconnected_ag_address);
            }
            
            /* Kick off reconnection to this AV source to negotiate one of the enabled codecs */
            theSink.a2dp_link_data->media_reconnect[av_id] = TRUE;
            A2dpMediaCloseRequest(theSink.a2dp_link_data->device_id[av_id], theSink.a2dp_link_data->stream_id[av_id]);
        }
    }
}

/*************************************************************************
NAME    
    disableIncompatibleOptionalCodecs
    
DESCRIPTION
    Determines the incompatible codecs between the peer devices and disables them.

RETURNS
    void
    
**************************************************************************/
static void disableIncompatibleOptionalCodecs(void)
{
    uint16 local_optional_codecs;
    uint16 remote_optional_codecs;
    uint16 compatible_optional_codecs;
    uint16 index = 0;

    if(peerGetLocalSupportedCodecs(&local_optional_codecs) && peerGetRemoteSupportedCodecs(&remote_optional_codecs))
    {
        compatible_optional_codecs = local_optional_codecs & remote_optional_codecs & TWS_SUPPORTED_CODECS;
        
        A2DP_DEBUG(("A2DP: local_optional_codecs = 0x%x\n", local_optional_codecs));
        A2DP_DEBUG(("A2DP: remote_optional_codecs = 0x%x\n", remote_optional_codecs));
        A2DP_DEBUG(("A2DP: TWS_SUPPORTED_CODECS = 0x%x\n", TWS_SUPPORTED_CODECS));
        A2DP_DEBUG(("A2DP: compatible_optional_codecs = 0x%x\n", compatible_optional_codecs));
        
        if(compatible_optional_codecs != local_optional_codecs)
        {            
            while(optional_codec_seids[index].codec_seid != INVALID_SEID)            
            {
                if(!(compatible_optional_codecs & optional_codec_seids[index].codec_bit))
                {
                    A2DP_DEBUG(("A2DP: disableIncompatibleOptionalCodecs seid = 0x%x\n", optional_codec_seids[index].codec_seid));

                    /* Disable the incompatible codecs */
                    A2dpCodecSetAvailable(a2dp_primary, optional_codec_seids[index].codec_seid, FALSE);
                    A2dpCodecSetAvailable(a2dp_secondary, optional_codec_seids[index].codec_seid, FALSE);
                }
                index++;
            }
        }
    }
}

/*************************************************************************
NAME    
    enableOptionalCodecs
    
DESCRIPTION
    Enables the support for optional codecs.

RETURNS
    
**************************************************************************/
static void enableOptionalCodecs(void)
{
    uint16 local_optional_codecs;
    uint16 index = 0;
    
    if(peerGetLocalSupportedCodecs(&local_optional_codecs))
    {
        while(optional_codec_seids[index].codec_seid != INVALID_SEID)            
        {
            if(local_optional_codecs & optional_codec_seids[index].codec_bit)
            {
                A2DP_DEBUG(("A2DP: enableOptionalCodecs seid = 0x%x\n", optional_codec_seids[index].codec_seid));
                
                /* Enable the optional extra codec */
                A2dpCodecSetAvailable(a2dp_primary, optional_codec_seids[index].codec_seid, TRUE);
                A2dpCodecSetAvailable(a2dp_secondary, optional_codec_seids[index].codec_seid, TRUE);
            }
            index++;
        }
    }
}  

/*************************************************************************
NAME    
    a2dpGetPeerIndex
    
DESCRIPTION
    Attempts to obtain the index into a2dp_link_data structure for a currently 
    connected Peer device.
    
RETURNS
    TRUE if successful, FALSE otherwise
    
**************************************************************************/
bool a2dpGetPeerIndex (uint16* index)
{
    uint8 i;
    
    /* go through A2dp connection looking for device_id match */
    for_all_a2dp(i)
    {
        /* if the a2dp link is connected check its device id */
        if(theSink.a2dp_link_data && theSink.a2dp_link_data->connected[i])
        {
            /* if a device_id match is found return its value and a
               status of successful match found */
            if(theSink.a2dp_link_data->peer_device[i] == remote_device_peer)
            {
                if (index)
                {
                    *index = i;
                }
                
                return TRUE;
            }
        }
    }
    /* no matches found so return not successful */    
    return FALSE;
}


/*************************************************************************
NAME    
    a2dpGetSourceIndex
    
    Attempts to obtain the index into a2dp_link_data structure for a currently 
    connected A2dp Source device.
    
RETURNS
    TRUE if successful, FALSE otherwise
    
**************************************************************************/
bool a2dpGetSourceIndex (uint16* index)
{
    uint8 i;
    
    /* go through A2dp connection looking for device_id match */
    for_all_a2dp(i)
    {
        /* if the a2dp link is connected check its device id */
        if(theSink.a2dp_link_data && theSink.a2dp_link_data->connected[i])
        {
            /* if a device_id match is found return its value and a
               status of successful match found */
            if(theSink.a2dp_link_data->peer_device[i] == remote_device_nonpeer)
            {
                if (index)
                {
                    *index = i;
                }
                
                return TRUE;
            }
        }
    }
    /* no matches found so return not successful */    
    return FALSE;
}


/*************************************************************************
NAME    
    a2dpGetSourceSink
    
DESCRIPTION
    Attempts to obtain the media sink for a currently connected A2dp Source.

RETURNS
    Handle to media sink if present, NULL otherwise
    
**************************************************************************/
Sink a2dpGetSourceSink (void)
{
    uint16 av_id;
    
    if (a2dpGetSourceIndex(&av_id))
    {
        return A2dpMediaGetSink(theSink.a2dp_link_data->device_id[av_id], theSink.a2dp_link_data->stream_id[av_id]);
    }
    
    return (Sink)NULL;
}

/*************************************************************************
NAME    
    getCodecSettings
    
DESCRIPTION
    Attempts to obtain the locally supplied codec setting for the specified SEID

RETURNS
    Pointer to codec settings structure, if found. NULL otherwise

**************************************************************************/
static a2dp_codec_settings * getCodecSettings (uint8 seid)
{
    uint16 i;
    
    for (i=0; i<NUM_SEPS; i++)
    {
        if (codecList[i].config && (codecList[i].config->seid == seid))
        {
            a2dp_codec_settings * codec_settings = (a2dp_codec_settings *)PanicNull( malloc(sizeof(a2dp_codec_settings) + codecList[i].config->size_caps) );
            memset(codec_settings, 0, sizeof(a2dp_codec_settings) +  codecList[i].config->size_caps);  /* Zero the Codec settings */
            
            codec_settings->size_configured_codec_caps = codecList[i].config->size_caps;
            memmove(codec_settings->configured_codec_caps, codecList[i].config->caps, codecList[i].config->size_caps);
            
            codec_settings->seid = seid; 

            return codec_settings;
        }
    }
    return NULL;
}


/*************************************************************************
NAME    
    modifyCodecSettings
    
DESCRIPTION
    Modifies the supplied general codec capabilities to provide specific 
    capabilities for the rate specified.

RETURNS
    None

**************************************************************************/
static void modifyCodecSettings (a2dp_codec_settings * codec_settings, uint16 rate)
{
    uint8 * codec_caps = NULL;
    uint16 codec_caps_size;

    /* There is no codec configured, nothing to modify, return */
    if(codec_settings == NULL)
{
        return;
    }
    
    codec_caps = codec_settings->configured_codec_caps;
    codec_caps_size = codec_settings->size_configured_codec_caps;
    
    A2DP_DEBUG(("A2DP: modifyCodecSettings   codec_caps=0x%p   size=%u   rate=%u\n",codec_caps,codec_caps_size,rate));
    
    /* Scan through codec caps for the Media Codec service category */
    while (codec_caps_size && codec_caps && (codec_caps[0] != AVDTP_SERVICE_MEDIA_CODEC))
    {   
        uint16 category_size = codec_caps[1] + 2;
        
        codec_caps += category_size;
    }
    
    if (codec_caps_size && codec_caps && (codec_caps[0] == AVDTP_SERVICE_MEDIA_CODEC))
    {   /* Media Codec service category located */
        const uint8 *new_codec_caps = NULL;
        
        switch (rate)
        {
        case 16000:
            new_codec_caps = sbc_caps_16k;
            break;
        case 32000:
            new_codec_caps = sbc_caps_32k;
            break;
        case 44100:
            new_codec_caps = sbc_caps_44k1;
            break;
        case 48000:
            new_codec_caps = sbc_caps_48k;
            break;
        }
        
        A2DP_DEBUG(("A2DP: Found AVDTP_SERVICE_MEDIA_CODEC, attempting to modify caps...\n"));
        A2DP_DEBUG(("A2DP: new_caps=0x%p   codec_caps[1]=%u  codec_caps[3]=%u   new_caps[1]=%u  new_caps[3]=%u\n", new_codec_caps, codec_caps[1], codec_caps[3], new_codec_caps[1], new_codec_caps[3]));
        if (new_codec_caps && (codec_caps[1] == new_codec_caps[1]) && (codec_caps[3] == new_codec_caps[3]))
        {   /* Check category size and seid of new caps match current caps, before updating */
            memcpy(codec_caps, new_codec_caps, new_codec_caps[1] + 2);
        }
    }
}

/*************************************************************************
NAME    
    handleA2dpCodecConfigureIndFromPeer
    
DESCRIPTION
    All Peer specific SEPs are configured to ask the app to provide the appropriate parameters
    when attempting to issue an AVDTP_SET_CONFIGURATION_CMD for the relay stream.
    This function obtains the codec settings for the current source (Wired, USB or A2DP) and uses
    these to set the configuration for the relay stream.

RETURNS
    None
    
**************************************************************************/
static void handleA2dpCodecConfigureIndFromPeer (A2DP_CODEC_CONFIGURE_IND_T* ind)
{
    a2dp_link_priority priority;
    a2dp_codec_settings* codec_settings = NULL;
    RelaySource current_source = theSink.peer.current_source;
    uint8* conf_caps = NULL;
    uint16 size_conf_caps = 0;
    uint8 *peer_caps = ind->codec_service_caps;
    uint16 size_peer_caps = ind->size_codec_service_caps;
    
    PEER_DEBUG(("A2DP: handleA2dpCodecConfigureIndFromPeer dev:%u seid:0x%X\n", ind->device_id, ind->local_seid));
    
    switch (current_source)
    {
        case RELAY_SOURCE_NULL:
            PEER_DEBUG(("... Streaming Av Src not found\n"));
            break;

        case RELAY_SOURCE_ANALOGUE:
        {
            uint16 samp_freq;
            
            codec_settings = getCodecSettings( SBC_SEID );
            analogGetAudioRate(&samp_freq);
            modifyCodecSettings(codec_settings, samp_freq);
            break;
        }
        case RELAY_SOURCE_USB:
        {
#ifdef ENABLE_USB /* HYDRACORE_TODO */
            uint16 samp_freq;

            codec_settings = getCodecSettings( SBC_SEID );
            UsbDeviceClassGetValue(USB_DEVICE_CLASS_GET_VALUE_SPEAKER_SAMPLE_FREQ, &samp_freq);
            modifyCodecSettings(codec_settings, samp_freq);
#endif
            break;
        }
        case RELAY_SOURCE_A2DP:
            if (findCurrentA2dpSource( &priority ))
            {
            uint8 device_id = theSink.a2dp_link_data->device_id[priority];
            uint8 stream_id = theSink.a2dp_link_data->stream_id[priority];

            codec_settings = A2dpCodecGetSettings(device_id, stream_id);
            }
            break;
    }

    if (codec_settings)
    {
        if ( !(codec_settings->seid & (SOURCE_SEID_MASK | TWS_SEID_MASK)) &&	/* Double check AV Source is using a standard Sink SEP */
              ((ind->local_seid & SOURCE_SEID_MASK) == SOURCE_SEID_MASK) &&		/* Double check Relay stream is using Source SEP */
              (codec_settings->seid == (ind->local_seid & BASE_SEID_MASK)) )	/* Same base codec id being used by AV Source and Relay stream */
        {   
            conf_caps = codec_settings->configured_codec_caps;
            size_conf_caps = codec_settings->size_configured_codec_caps;
            /* Source and sink seids use matching codec */
                /* Request same codec configuration settings as AV Source for peer device */
            PEER_DEBUG(("... Configuring codec dev:%u local codec_caps_size=%u seid=0x%X\n",ind->device_id, size_peer_caps, ind->local_seid));
            PEER_DEBUG(("remote codec_caps_size=%u seid=0x%X\n", size_conf_caps, codec_settings->seid));

                if (ind->local_seid & TWS_SEID_MASK)
                {	/* TWS Source SEP - place Sink codec caps into TWS codec caps */
                uint16 size_to_copy = size_conf_caps-2;

                memcpy(peer_caps+12, conf_caps+2, size_to_copy);  /* TODO: Copy each service capability separately */
                size_to_copy = size_peer_caps;
                A2dpCodecConfigureResponse(ind->device_id, TRUE, ind->local_seid, size_to_copy, peer_caps);
            }
            else
            {   
                /* In some TC, PTS expects SRC to open the media channel, then ADK tries to configure audio SRC's caps.
                    But PTS does not expect delay reporting capabilities to be set. Since PTS does not support delay reporting, 
                    we need to remove that before configuring PTS (if at all the audio SRC's caps support delay reporting).
                    This wrapper function has been written to take care of the situation */
                if(peerQualificationReplaceDelayReportServiceCaps(peer_caps, &size_peer_caps, conf_caps, size_conf_caps))
                {
                    A2dpCodecConfigureResponse(ind->device_id, TRUE, ind->local_seid, size_peer_caps, peer_caps);
                }
                else
                {
                    /* ShareMe Source SEP - use Sink codec caps for ShareMe codec */
                    A2dpCodecConfigureResponse(ind->device_id, TRUE, ind->local_seid, size_conf_caps, conf_caps);
                }
                }
        }
        else
        {   /* Source and sink seids do not use matching codec */
            PEER_DEBUG(("... Non matching codecs dev:%u seid:0x%X\n",ind->device_id, ind->local_seid));
            A2dpCodecConfigureResponse(ind->device_id, FALSE, ind->local_seid, 0, NULL);
        }

            free(codec_settings);
    }
    else
    {   /* Reject as we don't have an active source */
            A2dpCodecConfigureResponse(ind->device_id, FALSE, ind->local_seid, 0, NULL);
    }
}


/*************************************************************************
NAME    
    openPeerStream
    
DESCRIPTION
    Requests to open a media channel to the currently connected Peer

RETURNS
    TRUE if request issued, FALSE otherwise
    
**************************************************************************/
static bool openPeerStream (uint16 Id, uint8 base_seid)
{
    PEER_DEBUG(("openPeerStream Dev=%u base_seid=%02X",Id, base_seid));
    
	base_seid &= BASE_SEID_MASK;
	
    /* Don't open AAC based stream to a peer device supporting ShareMe only */
    if (!(theSink.a2dp_link_data->peer_features[Id] & remote_features_tws_a2dp_sink) && (base_seid == AAC_SEID))
	{
        PEER_DEBUG(("  unsuitable\n"));
		return FALSE;
	}

	if (base_seid)
	{	
		uint8 seid_list[3];
		uint8 seid_list_size = 0;
		
        PEER_DEBUG(("  seid_list=["));
#ifdef PEER_TWS
		seid_list[seid_list_size++] = base_seid | (SOURCE_SEID_MASK | TWS_SEID_MASK);
        PEER_DEBUG(("%02X, ", seid_list[seid_list_size-1]));
#endif
#ifdef PEER_AS
		seid_list[seid_list_size++] = base_seid | SOURCE_SEID_MASK;
        PEER_DEBUG(("%02X, ", seid_list[seid_list_size-1]));
#endif
		seid_list[seid_list_size++] = SBC_SEID | SOURCE_SEID_MASK;		/* Always request a standard SBC source SEP, to support standard sink devices */
        PEER_DEBUG(("%02X]  size=%u\n", seid_list[seid_list_size-1], seid_list_size));
		
		/*return A2dpMediaOpenRequestEx(theSink.a2dp_link_data->device_id[Id], seid_list_size, seid_list, sizeof(a2dp_media_conftab), a2dp_media_conftab);*/
		return A2dpMediaOpenRequest(theSink.a2dp_link_data->device_id[Id], seid_list_size, seid_list);
	}

    PEER_DEBUG(("  unsuitable\n"));
	return FALSE;
}


/*************************************************************************
NAME    
    a2dpIssuePeerOpenRequest
    
DESCRIPTION
    Issues a request to opens a media stream to a currently connected Peer
    
RETURNS
    TRUE if request issued, FALSE otherwise
    
**************************************************************************/
bool a2dpIssuePeerOpenRequest (void)
{
    uint16 peer_id;
    
    if (a2dpGetPeerIndex(&peer_id))
    {
        uint8 seid = INVALID_SEID;
        uint16 av_id;
        RelaySource current_source = theSink.peer.current_source;
        
        peerClaimRelay(TRUE);
        
        PEER_DEBUG(("issuePeerOpenRequest peer=%u  av=%u\n",peer_id,current_source));
        
        switch (current_source)
        {
        case RELAY_SOURCE_NULL:
            break;
        case RELAY_SOURCE_ANALOGUE:
        case RELAY_SOURCE_USB:
            seid = SBC_SEID;
            break;
        case RELAY_SOURCE_A2DP:
            if (a2dpGetSourceIndex(&av_id))
            {
                seid = theSink.a2dp_link_data->seid[av_id];
            }
            break;
        }
        
        return openPeerStream(peer_id, seid);
    }
    
    return FALSE;
}


/*************************************************************************
NAME    
    a2dpIssuePeerCloseRequest
    
DESCRIPTION
    Issues a request to close the relay channel to the currently connected Peer

RETURNS
    TRUE if request issued, FALSE otherwise

**************************************************************************/
bool a2dpIssuePeerCloseRequest (void)
{
    uint16 peer_id;
    
    if (a2dpGetPeerIndex(&peer_id))
    {
        PEER_DEBUG(("issuePeerCloseRequest peer=%u\n",peer_id));
        
        peerClaimRelay(FALSE);
        
        /* Ensure suspend state is set as streaming has now ceased */
        a2dpSetSuspendState(peer_id, a2dp_local_suspended);
        avrcpUpdatePeerPlayStatus(avrcp_play_status_stopped);
        
        return A2dpMediaCloseRequest(theSink.a2dp_link_data->device_id[peer_id], theSink.a2dp_link_data->stream_id[peer_id]);
    }
    
    return FALSE;
}

/*************************************************************************
NAME    
    a2dpIssuePeerStartRequest
    
DESCRIPTION
    Issues a request to start the relay channel to the currently connected Peer

RETURNS
    TRUE if request issued, FALSE otherwise

**************************************************************************/
bool a2dpIssuePeerStartRequest (void)
{
    uint16 peer_id;
    
    if (a2dpGetPeerIndex(&peer_id))
    {
        PEER_DEBUG(("   issuing start request...\n"));
        
        avrcpUpdatePeerPlayStatus(avrcp_play_status_playing);
                
                return A2dpMediaStartRequest(theSink.a2dp_link_data->device_id[peer_id], theSink.a2dp_link_data->stream_id[peer_id]);
            }

    return FALSE;
}


/*************************************************************************
NAME    
    a2dpIssuePeerSuspendRequest
    
DESCRIPTIONDESCRIPTION
    Issues a request to suspend the relay channel to the currently connected Peer

RETURNS
    TRUE if request issued, FALSE otherwise

**************************************************************************/
bool a2dpIssuePeerSuspendRequest (void)
{
    uint16 peer_id;
    
    if (a2dpGetPeerIndex(&peer_id))
    {
        PEER_DEBUG(("issuePeerSuspendRequest peer=%u\n",peer_id));
        
        /* Ensure suspend state is set as streaming has now ceased */
        a2dpSetSuspendState(peer_id, a2dp_local_suspended);
        avrcpUpdatePeerPlayStatus(avrcp_play_status_paused);
        
        return A2dpMediaSuspendRequest(theSink.a2dp_link_data->device_id[peer_id], theSink.a2dp_link_data->stream_id[peer_id]);
    }
    
    return FALSE;
}


/*************************************************************************
NAME    
    a2dpIssuePeerStartResponse
    
DESCRIPTION
    Issues a start response to a Peer based on availability of the relay channel

RETURNS
    TRUE if response sent, FALSE otherwise
    
**************************************************************************/
bool a2dpIssuePeerStartResponse (void)
{
    uint16 peer_id;
    
    if (a2dpGetPeerIndex(&peer_id))
    {
        PEER_DEBUG(("issuePeerStartResponse peer=%u\n",peer_id));
        
        if ( peerIsRelayAvailable() )
        {   /* Accept start request */
            return A2dpMediaStartResponse(theSink.a2dp_link_data->device_id[peer_id], theSink.a2dp_link_data->stream_id[peer_id], TRUE);
        }
        else
        {   /* Reject start request as not in a state to receive audio */
            return A2dpMediaStartResponse(theSink.a2dp_link_data->device_id[peer_id], theSink.a2dp_link_data->stream_id[peer_id], FALSE);
        }
    }
    
    return FALSE;
}

#endif

/*************************************************************************
NAME    
    a2dpSetPlayingState
    
DESCRIPTION
    Logs the current AVRCP play status for the specified A2DP connection and
    updates the Suspend State for the Media channel so that it reflects the true
    media playing status.

    The Suspend State defines the overall state (suspended or not suspended) of a media stream.  
    It is determined by using a combination of the AVDTP stream state and the AVRCP play status.
    Ultimately, the AVDTP stream state drives the suspend state far more strongly than the AVRCP 
    play status, as the AVDTP stream state tells us if data is actually being transmitted over 
    an A2DP media channel.
    Certainly, when the AVDTP stream state moves from a suspended to a streaming state, then we 
    take that as a definitive indication that streaming has started and the A2DP media stream is 
    not in a suspended state.  In this instance AVRCP play status often lags behind the AVDTP 
    stream state and we can receive actual indications from a Source that the play status is 
    still stopped/paused even though the AVDTP stream state is set to streaming.  This is 
    probably down to latency between layers of the OS and applications on the Source.
    When an AG stops streaming then behaviour is a little different.  Here AVRCP play status will 
    often lead AVDTP stream state by several seconds.  This is the one instance we allow the AVRCP 
    play status to drive the Suspend State.  This is to allow an A2DP source to be marked as 
    suspended and allow far faster automatic changes to another Source (analogue/USB) that may be 
    streaming.
   
RETURNS
    None
    
**************************************************************************/
void a2dpSetPlayingState (uint16 id, bool playing)
{
    A2DP_DEBUG(("A2DP: a2dpSetPlayingState id %u playing %u\n", id, playing));

    if (theSink.a2dp_link_data && theSink.a2dp_link_data->playing[id] != playing)
    {
        theSink.a2dp_link_data->playing[id] = playing;
        
        if (!playing)
        {   /* Play state has changed from playing to not playing.  This change in AVRCP play status is likely to lead a change */
            /* to the AVDTP stream state by several seconds.  Mark the stream as suspended so we can allow automatic changes to */
            /* the relayed source far sooner than waiting for the AVDTP stream state to change.                                 */
            a2dpSetSuspendState(id, a2dp_remote_suspended);
            
#ifdef ENABLE_PEER
            if(!a2dpIsIndexPeer(id))
            {
                peerClaimRelay(FALSE);
            }
#endif

#ifdef ENABLE_PEER
            linklossProtectStreaming(linkloss_stream_protection_off);

            /* Re-enable stream protection on the link it is required on. */
            if (!theSink.features.PeerLinkRecoveryWhileStreaming)
            {
                linklossProtectStreaming(linkloss_stream_protection_on);
            }
#endif
        }
        else
        {   /* Play state has changed from not playing to playing.  In this instance we drive the suspend state directly from   */
            /* the AVDTP stream state                                                                                           */
            a2dp_stream_state a2dp_state = A2dpMediaGetState(theSink.a2dp_link_data->device_id[id], theSink.a2dp_link_data->stream_id[id]);

            if ((a2dp_state == a2dp_stream_starting) || (a2dp_state == a2dp_stream_streaming))
            {   /* A2DP media stream is streaming (or heading towards it) */
                a2dpSetSuspendState(id, a2dp_not_suspended);
                
#ifdef ENABLE_PEER
                if(!a2dpIsIndexPeer(id))
                {
                    peerClaimRelay(TRUE);
                }
#endif

                /*Ensure that the device is not currently streaming from a different A2DP, if its found to be streaming then pause this incoming stream  */
#ifdef ENABLE_AVRCP                
                a2dpPauseNonRoutedSource(id);
#endif
            }
            else
            {
                a2dpSetSuspendState(id, a2dp_remote_suspended);
                
#ifdef ENABLE_PEER            
                if(!a2dpIsIndexPeer(id))
                {
                    peerClaimRelay(FALSE);
            }
#endif 
        }
        }
    }
}

#ifdef ENABLE_PEER

/*************************************************************************
NAME    
    sinkA2dpHandlePeerAvrcpConnectCfm
    
DESCRIPTION
    Configure initial relay availability when a Peer connects

RETURNS
    None
    
**************************************************************************/
void sinkA2dpHandlePeerAvrcpConnectCfm (uint16 peer_id, bool successful)
{
    UNUSED(peer_id);

    if (successful)
    {   /* Set initial local status now peers have connected */
        if(theSink.routed_audio)
        {
            if ((sinkCallManagerGetHfpSink(hfp_primary_link) && (sinkCallManagerGetHfpCallState(hfp_primary_link) > hfp_call_state_idle)) 
                || (sinkCallManagerGetHfpSink(hfp_secondary_link) && (sinkCallManagerGetHfpCallState(hfp_secondary_link) > hfp_call_state_idle)))
            {   /* Call is active, so set flag to indicate that this device does not want the relay channel to be used */
                 peerUpdateLocalStatusChange(PEER_STATUS_CHANGE_CALL_ACTIVE | PEER_STATUS_CHANGE_RELAY_AVAILABLE | PEER_STATUS_CHANGE_RELAY_FREED);
            }
            else
            {   /* No call active, set relay channel as free for use */
                peerUpdateLocalStatusChange(PEER_STATUS_CHANGE_CALL_INACTIVE | PEER_STATUS_CHANGE_RELAY_AVAILABLE );
            }
        }
        else
        {   /* No audio routed, thus relay channel is completely free for use */
             peerUpdateLocalStatusChange(PEER_STATUS_CHANGE_CALL_INACTIVE | PEER_STATUS_CHANGE_RELAY_AVAILABLE | PEER_STATUS_CHANGE_RELAY_FREED);
        }
    }
}


/*************************************************************************
NAME    
    sinkA2dpSetPeerAudioRouting
    
DESCRIPTION
    Informs current Peer of the required routing modes and updates DSP

RETURNS
    TRUE if successful, FALSE otherwise
    
**************************************************************************/
bool sinkA2dpSetPeerAudioRouting (PeerTwsAudioRouting master_routing_mode, PeerTwsAudioRouting slave_routing_mode)
{
    uint16 peer_id;
    
    if ( a2dpGetPeerIndex(&peer_id) )
    {
#ifdef ENABLE_AVRCP            
        uint16 avrcp_id;
        
        /* does the device support AVRCP and is AVRCP currently connected to this device? */
        for_all_avrcp(avrcp_id)
        {    
            /* ensure media is streaming and the avrcp channel is that requested to be paused */
            if ((theSink.avrcp_link_data->connected[avrcp_id])&& 
                (BdaddrIsSame(&theSink.a2dp_link_data->bd_addr[peer_id], &theSink.avrcp_link_data->bd_addr[avrcp_id])))
            {
                uint8 routing_modes[2];
                
                /* Swap routing modes around for other device.  This will mean that the same channels are rendered by both devices no matter which is the actual TWS Master */
                routing_modes[0] = (uint8)slave_routing_mode;
                routing_modes[1] = (uint8)master_routing_mode;
                
                sinkAvrcpVendorUniquePassthroughRequest( avrcp_id, AVRCP_PEER_CMD_TWS_AUDIO_ROUTING, sizeof(routing_modes), (const uint8 *)routing_modes );
                break;
            }
        }
#endif
        peerHandleAudioRoutingCmd( master_routing_mode, slave_routing_mode );
        
        return TRUE;
    }
    
    return FALSE;
}

#ifdef PEER_TWS
/****************************************************************************
NAME 
 a2dpCheckDeviceTrimVol

DESCRIPTION
 check whether any a2dp connections are present and if these are currently active
 and routing audio to the device, if that is the case adjust the volume up or down
 as appropriate

RETURNS
 bool   TRUE if volume adjusted, FALSE if no streams found
    
*/
bool a2dpCheckDeviceTrimVol (volume_direction dir, tws_device_type tws_device)
{
    uint8 index;
    
    if (tws_device != tws_none)
    {
        /* check both possible instances of a2dp connection */
        for(index = a2dp_primary; index < (a2dp_secondary+1); index++)
        {
            /* is a2dp connected? */
            if(theSink.a2dp_link_data->connected[index])
            {
                /* check whether the a2dp connection is present and streaming data and that the audio is routed */
                if(theSink.routed_audio && (theSink.routed_audio == A2dpMediaGetSink(theSink.a2dp_link_data->device_id[index], theSink.a2dp_link_data->stream_id[index])))
                {
#ifdef ENABLE_AVRCP
                    if ((theSink.a2dp_link_data->peer_device[index] == remote_device_peer) && (theSink.a2dp_link_data->peer_features[index] & remote_features_tws_a2dp_source))
                    {
                        peerSendDeviceTrimVolume(dir , tws_device) ;                        
                    }
                    else
#endif
                    {
                        VolumeModifyAndUpdateTWSDeviceTrim(dir, tws_device);
                        return TRUE;
                    }
                }
            }
        }
    }    
    return FALSE;
}
#endif

#endif  /* ENABLE_PEER */

/*************************************************************************
NAME    
    a2dpSetSuspendState
    
DESCRIPTION
    Sets the suspend state for the specified device

RETURNS
    None
    
**************************************************************************/
void a2dpSetSuspendState (uint16 id, a2dp_suspend_state state)
{
    if (state == a2dp_not_suspended)
    {   /* Returning to the unsuspended state */
        theSink.a2dp_link_data->SuspendState[id] = state;
    }
    else
    {   /* Only update the suspend state of the device only if it was not set to remote or locally suspended  previously */
        if (theSink.a2dp_link_data->SuspendState[id] == a2dp_not_suspended)
        {
            theSink.a2dp_link_data->SuspendState[id] = state;
        }
    }
    A2DP_DEBUG(("A2dp: SuspendState[%u] = %d\n",id,theSink.a2dp_link_data->SuspendState[id])); 
}

#ifdef ENABLE_AVRCP
/*************************************************************************
NAME    
    a2dpPauseNonRoutedSource
    
DESCRIPTION
    Check whether the a2dp connection is present and streaming data and that the audio is routed, 
    if thats true then pause/stop the incoming stream corresponding to the input deviceId.

RETURNS
    None
    
**************************************************************************/
void a2dpPauseNonRoutedSource(uint16 id)
{
/* Only certain TWS use cases require us to attempt to pause source devices */
#ifdef ENABLE_PEER
    uint16 avrcpIndex;
    uint16 peerIndex;
    
#ifndef HYDRACORE_TODO
    A2DP_DEBUG(("A2dp: a2dpPauseNonRoutedSource\n"));
#endif /* HYDRACORE_TODO */

    if((theSink.a2dp_link_data->SuspendState[id] == a2dp_not_suspended)
       && (a2dpGetPeerIndex(&peerIndex))
       && (sinkAvrcpGetIndexFromBdaddr(&theSink.a2dp_link_data->bd_addr[id], &avrcpIndex, TRUE))
      )
    {
        /* Only pause source if we have a peer connected and is required for a TWS use case */
        if((theSink.a2dp_link_data->peer_device[id] == remote_device_nonpeer) &&
           (theSink.features.TwsSingleDeviceOperation)  &&
           (theSink.a2dp_link_data->remote_peer_status[peerIndex] & PEER_STATUS_IN_CALL)
          )
        {
            A2DP_DEBUG(("A2dp: SuspendState - Stop playing Id = %d \n",id));
            a2dpAvrcpStop(avrcpIndex);
            a2dpSetSuspendState(id , a2dp_local_suspended);
        }
        /* Only pause if we are the slave device and both master and slave are attempting to play*/
        if ((peerIndex != id)
            && (theSink.a2dp_link_data->link_role[peerIndex] == LR_CURRENT_ROLE_SLAVE)
            && (theSink.a2dp_link_data->playing[peerIndex])
            && (theSink.a2dp_link_data->playing[id])
           )
        {
            A2DP_DEBUG(("A2dp: stopping avrcpIndex=%u\n", avrcpIndex));
            a2dpAvrcpStop(avrcpIndex);
        }
    }
#endif /* ENABLE_PEER */
}


/*************************************************************************
NAME
    a2dpAvrcpStop

DESCRIPTION
    This function requests for playback to stop via avrcp,

RETURNS
    None
**************************************************************************/
void a2dpAvrcpStop(const uint16 avrcpIndex)
            {                
#ifdef ENABLE_PEER
                /* cancel any queued ff or rw requests and then stop the streaming*/
                MessageCancelAll (&theSink.task, EventUsrAvrcpFastForwardPress);
                MessageCancelAll (&theSink.task, EventUsrAvrcpRewindPress);

                sinkAvrcpStopRequest(avrcpIndex);
#endif /* ENABLE_PEER */
}
#endif

/*************************************************************************
NAME    
    InitA2dp
    
DESCRIPTION
    This function initialises the A2DP library and supported codecs

RETURNS
    A2DP_INIT_CFM message returned, handled by A2DP message handler
**************************************************************************/
void InitA2dp(void)
{
    uint16 i;
    sep_data_type seps[NUM_SEPS];
    uint8               number_of_seps = 0;
    pio_config_type* pio;
    a2dp_avrcp_block   *shared_memory;

    A2DP_DEBUG(("INIT: A2DP\n")); 
    A2DP_DEBUG(("INIT: NUM_SEPS=%u\n",NUM_SEPS)); 

  	/*allocate the memory for the a2dp link data */
    shared_memory = newZDebugPanic( a2dp_avrcp_block );
    theSink.a2dp_link_data = &shared_memory->a2dp;
    
    /* Make sure all references to mic parameters point to the right place */
    pio = &theSink.conf6->PIOIO;
    theSink.a2dp_link_data->a2dp_audio_connect_params.mic_params = &pio->digital;
    
    /* set default microphone source for back channel enabled dsp apps */
    theSink.a2dp_link_data->a2dp_audio_mode_params.external_mic_settings = EXTERNAL_MIC_NOT_FITTED;
    theSink.a2dp_link_data->a2dp_audio_mode_params.mic_mute = SEND_PATH_UNMUTE;
    
    /* Set default TWS audio routing modes */
    theSink.a2dp_link_data->a2dp_audio_mode_params.master_routing_mode = theSink.features.TwsMasterAudioRouting;
    theSink.a2dp_link_data->a2dp_audio_mode_params.slave_routing_mode = theSink.features.TwsSlaveAudioRouting;

    /* initialise device and stream id's to invalid as 0 is a valid value */
    theSink.a2dp_link_data->device_id[0] = INVALID_DEVICE_ID;
    theSink.a2dp_link_data->stream_id[0] = INVALID_STREAM_ID;   
    theSink.a2dp_link_data->device_id[1] = INVALID_DEVICE_ID;
    theSink.a2dp_link_data->stream_id[1] = INVALID_STREAM_ID;   

    theSink.a2dp_link_data->a2dp_audio_connect_params.silence_threshold = theSink.conf2->audio_routing_data.SilenceDetSettings.threshold;
    theSink.a2dp_link_data->a2dp_audio_connect_params.silence_trigger_time = theSink.conf2->audio_routing_data.SilenceDetSettings.trigger_time;
    
    /* only continue and initialise the A2DP library if it's actually required,
       library functions will return false if it is uninitialised */
    if(theSink.features.EnableA2dpStreaming)
    {
        uint8 codec_include_mask = getCodecIncludeMask();
        
        /* Only register codecs that are both included due to the correct application feature set and enabled via config */          
        for (i=0; i<NUM_SEPS; i++)
        {
            if (codecList[i].config && (codecList[i].include_mask & codec_include_mask))
            {
                if (codecList[i].enable_bit==SBC_CODEC_BIT || (theSink.features.A2dpOptionalCodecsEnabled & (1<<codecList[i].enable_bit)))
                {
                    seps[number_of_seps].sep_config = codecList[i].config;
                    seps[number_of_seps].in_use = FALSE;
                    number_of_seps++;
                    
                    A2DP_DEBUG(("INIT: Codec included (seid=0x%X)\n",codecList[i].config->seid));
                }
            }
        }

        /* No active connections yet so default link loss protection to off. */
        theSink.stream_protection_state = 0;
        theSink.linkloss_bd_addr = 0;

        /* Initialise the A2DP library */
#ifdef ENABLE_PEER

#ifdef INCLUDE_A2DP_EXTRA_CODECS
       peerSetLocalSupportedCodecs(theSink.features.A2dpOptionalCodecsEnabled);
#endif        

        if (theSink.features.ShareMeSource || theSink.features.TwsSource)
        {   /* We support some form of source role, so ensure A2DP library advertises A2DP Source SDP record */
            A2dpInit(&theSink.task, A2DP_INIT_ROLE_SINK | A2DP_INIT_ROLE_SOURCE, NULL, number_of_seps, seps, theSink.conf1->timeouts.A2dpLinkLossReconnectionTime_s);
        }
        else
#endif
        {
            A2dpInit(&theSink.task, A2DP_INIT_ROLE_SINK, NULL, number_of_seps, seps, theSink.conf1->timeouts.A2dpLinkLossReconnectionTime_s);
        }
    }
}

/*************************************************************************
NAME    
    getA2dpIndex
    
DESCRIPTION
    This function tries to find a device id match in the array of a2dp links 
    to that device id passed in

RETURNS
    match status of true or false
**************************************************************************/
bool getA2dpIndex(uint8 DeviceId, uint16 * Index)
{
    uint8 i;
    
    /* go through A2dp connection looking for device_id match */
    for_all_a2dp(i)
    {
        /* if the a2dp link is connected check its device id */
        if(theSink.a2dp_link_data->connected[i])
        {
            /* if a device_id match is found return its value and a
               status of successful match found */
            if(theSink.a2dp_link_data->device_id[i] == DeviceId)
            {
                *Index = i;
            	A2DP_DEBUG(("A2dp: getIndex = %d\n",i)); 
                return TRUE;
            }
        }
    }
    /* no matches found so return not successful */    
    return FALSE;
}

/*************************************************************************
NAME    
    getA2dpIndexFromPlugin
    
DESCRIPTION
    This function tries to find an A2dp instance based on the supplied active 
	audio plugin 

RETURNS
    match status of true or false
**************************************************************************/
bool getA2dpIndexFromPlugin(Task audio_plugin, uint16 *Index)
{
    uint8 i;
    
    if (!audio_plugin || !theSink.a2dp_link_data)
    {
        return FALSE;
    }
    
    /* go through A2dp connection looking for sink match */
    for_all_a2dp(i)
    {
        /* if the a2dp link is connected check its device id */
        if(theSink.a2dp_link_data->connected[i])
        {
			if (theSink.a2dp_link_data->seid[i] && (getA2dpPlugin(theSink.a2dp_link_data->seid[i]) == audio_plugin))
			{
				*Index = i;
				return TRUE;
			}
        }
    }
    
    /* no matches found so return not successful */    
    return FALSE;
}


/*************************************************************************
NAME    
    getA2dpIndexFromSink
    
DESCRIPTION
    This function tries to find the a2dp device associated with the supplied 
    sink.  The supplied sink can be either a signalling or media channel.

RETURNS
    match status of true or false
**************************************************************************/
bool getA2dpIndexFromSink(Sink sink, uint16 * Index)
{
    uint8 i;
    
    if (!sink || !theSink.a2dp_link_data)
    {
        return FALSE;
    }
    
    /* go through A2dp connection looking for sink match */
    for_all_a2dp(i)
    {
        /* if the a2dp link is connected check its device id */
        if(theSink.a2dp_link_data->connected[i])
        {
            /* if a device_id match is found return its value and a
               status of successful match found */
            if(A2dpSignallingGetSink(theSink.a2dp_link_data->device_id[i]) == sink)
            {
                *Index = i;
                return TRUE;
            }
            
            if(A2dpMediaGetSink(theSink.a2dp_link_data->device_id[i], theSink.a2dp_link_data->stream_id[i]) == sink)
            {
                *Index = i;
                return TRUE;
            }
        }
    }
    
    /* no matches found so return not successful */    
    return FALSE;
}

/*************************************************************************
NAME    
    getA2dpStreamData
    
DESCRIPTION
    Function to retreive media sink and state for a given A2DP source

RETURNS
    void
**************************************************************************/
void getA2dpStreamData(a2dp_link_priority priority, Sink* sink, a2dp_stream_state* state)
{
    *state = a2dp_stream_idle;
    *sink  = (Sink)NULL;

/*   	A2DP_DEBUG(("A2dp: getA2dpStreamData(%u)\n",(uint16)priority)); */
    if(theSink.a2dp_link_data)
    {
/*        A2DP_DEBUG(("A2dp: getA2dpStreamData - peer=%u connected=%u\n",theSink.a2dp_link_data->peer_device[priority],theSink.a2dp_link_data->connected[priority])); */
        if(theSink.a2dp_link_data->connected[priority])
        {
            *state = A2dpMediaGetState(theSink.a2dp_link_data->device_id[priority], theSink.a2dp_link_data->stream_id[priority]) ;
            *sink  = A2dpMediaGetSink(theSink.a2dp_link_data->device_id[priority], theSink.a2dp_link_data->stream_id[priority]) ;
/*            A2DP_DEBUG(("A2dp: getA2dpStreamData - state=%u sink=0x%X\n",*state, (uint16)*sink)); */
        }
    }
}

/*************************************************************************
NAME    
    getA2dpStreamRole
    
DESCRIPTION
    Function to retrieve the role (source/sink) for a given A2DP source

RETURNS
    void
**************************************************************************/
void getA2dpStreamRole(a2dp_link_priority priority, a2dp_role_type* role)
{
    *role = a2dp_role_undefined;
    
    if(theSink.a2dp_link_data)
    {
/*        A2DP_DEBUG(("A2dp: getA2dpStreamRole - peer=%u connected=%u\n",theSink.a2dp_link_data->peer_device[priority],theSink.a2dp_link_data->connected[priority])); */
        if(theSink.a2dp_link_data->connected[priority])
        {
            *role = A2dpMediaGetRole(theSink.a2dp_link_data->device_id[priority], theSink.a2dp_link_data->stream_id[priority]) ;
/*            A2DP_DEBUG(("A2dp: getA2dpStreamRole - role=%u priority=%u\n",*role,priority)); */
        }
    }
}

/*************************************************************************
NAME    
    getA2dpPlugin
    
DESCRIPTION
    This function returns the task of the appropriate audio plugin to be used
    for the selected codec type when connecting audio

RETURNS
    task of relevant audio plugin
**************************************************************************/
Task getA2dpPlugin(uint8 seid)
{
    uint16 i;
    
    for (i=0; i<NUM_SEPS; i++)
    {
        if (codecList[i].config && (codecList[i].config->seid == seid))
        {
            return (Task)codecList[i].plugin;
        }
    }
    
	/* No plugin found so Panic */
	Panic();
	return 0;
}


/*************************************************************************
NAME    
    openStream
    
DESCRIPTION
    

RETURNS
    TRUE if successful, FALSE otherwise
    
**************************************************************************/
static bool openStream (uint16 Id, uint8 seid)
{
	if ((seid & SOURCE_SEID_MASK) == 0)
	{	/* Ensure a source SEID has not been requested */
		if (seid)
		{	/* Request to use the specified seid only */
			/*return A2dpMediaOpenRequestEx(theSink.a2dp_link_data->device_id[Id], 1, &seid, sizeof(a2dp_media_conftab), a2dp_media_conftab);*/
			return A2dpMediaOpenRequest(theSink.a2dp_link_data->device_id[Id], 1, &seid);
		}
		else
		{	/* Use default seid list, as specified in call to A2dpInit() */
			/*return A2dpMediaOpenRequestEx(theSink.a2dp_link_data->device_id[Id], 0, NULL, sizeof(a2dp_media_conftab), a2dp_media_conftab);*/
			return A2dpMediaOpenRequest(theSink.a2dp_link_data->device_id[Id], 0, NULL);
		}
	}
    
    return FALSE;
}


/****************************************************************************
NAME    
    sinkA2dpSetLinkRole
    
DESCRIPTION
    Updates stored BT role for specified device

RETURNS
    None
    
**************************************************************************/
void sinkA2dpSetLinkRole (Sink sink, hci_role role)
{
    uint16 index;

    A2DP_DEBUG(("sinkA2dpSetLinkRole  sink=0x%X  role=%s\n", (Sink)sink, (role == hci_role_master) ? "master" : "slave"));

    if (getA2dpIndexFromSink(sink, &index))
    {
#ifdef ENABLE_PEER
        a2dp_link_role last_role = theSink.a2dp_link_data->link_role[index];
#endif
        
        if (role == hci_role_master)
        {
            A2DP_DEBUG(("    setting  link_role[%u] = LR_CURRENT_ROLE_MASTER\n",index));
            theSink.a2dp_link_data->link_role[index] = LR_CURRENT_ROLE_MASTER;
        }
        else
        {
            A2DP_DEBUG(("    setting  link_role[%u] = LR_CURRENT_ROLE_SLAVE\n",index));
            theSink.a2dp_link_data->link_role[index] = LR_CURRENT_ROLE_SLAVE;
        }
        
#ifdef ENABLE_PEER
        if (last_role == LR_CHECK_ROLE_PENDING_START_REQ)
        {
            A2DP_DEBUG(("    start was pending...\n"));
            a2dpIssuePeerStartRequest();
        }
#endif
    }
}

/****************************************************************************
NAME    
    sinkA2dpInitComplete
    
DESCRIPTION
    Sink A2DP initialisation has completed, check for success. 

RETURNS
    void
**************************************************************************/
void sinkA2dpInitComplete(const A2DP_INIT_CFM_T *msg)
{   
    /* check for successful initialisation of A2DP libraray */
    if(msg->status == a2dp_success)
    {
        A2DP_DEBUG(("A2DP Init Success\n"));
        UartSendStr("+A2DPINIT\r\n");
    }
    else
    {
	    A2DP_DEBUG(("A2DP Init Failed [Status %d]\n", msg->status));
        Panic();
    }
}


/****************************************************************************
NAME    
    findUnconnectedA2dpDevice
    
DESCRIPTION
    Attempts to locate a device that is paging us based on its bluetooth address.
    An entry will exist for devices we are considering allowing connection to in the a2dp_link_data
    structure.  However, the connected flag will be FALSE.

RETURNS
    TRUE if device found, FALSE otherwise.
**************************************************************************/
static bool isUnconnectedA2dpDevice (const bdaddr *bd_addr, uint16* idx)
{
    for_all_a2dp(*idx)
    {
        if (!theSink.a2dp_link_data->connected[*idx] && BdaddrIsSame(bd_addr, &theSink.a2dp_link_data->bd_addr[*idx]))
        {   /* Found the unconnected device */
            return TRUE;
        }
    }
    
    /* Device is either unknown to us or already connected */
    return FALSE;
}


/****************************************************************************
NAME    
    getKnownPeerDeviceAttributes
    
DESCRIPTION
    Reads the peer device attributes.

RETURNS
    Void
**************************************************************************/
#ifdef ENABLE_PEER
static void getKnownPeerDeviceAttributes(const uint16 priority , sink_attributes *attributes)
{    
    /* Get any known peer device attributes */
    deviceManagerGetDefaultAttributes(attributes, dev_type_ag);
    deviceManagerGetAttributes(attributes, &theSink.a2dp_link_data->bd_addr[priority]);
    theSink.a2dp_link_data->peer_device[priority] = attributes->peer_device;
    theSink.a2dp_link_data->peer_features[priority] = attributes->peer_features;
    theSink.a2dp_link_data->peer_version[priority] = attributes->peer_version;
    theSink.a2dp_link_data->peer_dsp_required_buffering_level[priority] = getPeerDSPBufferingRequired(attributes->peer_version);

}
#endif
/****************************************************************************
NAME    
    issueA2dpSignallingConnectResponse
    
DESCRIPTION
    Issue response to a signalling channel connect request, following discovery of the 
    remote device type. 

RETURNS
    void
**************************************************************************/
void issueA2dpSignallingConnectResponse(const bdaddr *bd_addr, bool accept)
{
    uint16 idx;
    
    A2DP_DEBUG(("issueA2dpSignallingConnectResponse\n"));
    
    if (!theSink.a2dp_link_data)
    {
        A2DP_DEBUG(("NULL a2dp_link_data\n"));
        return;
    }
     
    if (!isUnconnectedA2dpDevice(bd_addr, &idx))
    {   /* We have no knowledge of this device */
        A2DP_DEBUG(("device unknown or already connected!\n"));
        return;
    }
        
    if (!accept)
        {
        A2DP_DEBUG(("Reject\n"));
        A2dpSignallingConnectResponse(theSink.a2dp_link_data->device_id[idx], FALSE);
        return;
    }
        
            {
#ifdef ENABLE_PEER
                sink_attributes attributes;
                
        A2DP_DEBUG(("   peer = %u\n",theSink.a2dp_link_data->peer_device[idx]));
        A2DP_DEBUG(("   features = %u\n",theSink.a2dp_link_data->peer_features[idx]));
        A2DP_DEBUG(("   version = 0x%X\n",theSink.a2dp_link_data->peer_version[idx]));
      
        deviceManagerGetDefaultAttributes(&attributes, dev_type_ag);
                deviceManagerGetAttributes(&attributes, bd_addr);
                attributes.peer_device = theSink.a2dp_link_data->peer_device[idx];
                attributes.peer_features = theSink.a2dp_link_data->peer_features[idx];
        attributes.peer_version = theSink.a2dp_link_data->peer_version[idx];
        attributes.peer_supported_optional_codecs = theSink.a2dp_link_data->remote_peer_optional_codecs;
        
                deviceManagerStoreAttributes(&attributes, bd_addr);
                
        if(!peerLinkReservedCanDeviceConnect(bd_addr))
        {
            /* If there is already an AG connected and the PeerLinkReserved is set, then do not allow 
                             another AG to connect */
            A2DP_DEBUG(("Reject\n"));
            A2dpSignallingConnectResponse(theSink.a2dp_link_data->device_id[idx],FALSE);
            return;
        }
#endif  /*ENABLE_PEER */

                A2DP_DEBUG(("Accept\n"));
                A2dpSignallingConnectResponse(theSink.a2dp_link_data->device_id[idx],TRUE);
                
#ifdef ENABLE_AVRCP
                sinkAvrcpCheckManualConnectReset(bd_addr);        
#endif                
            }
}

/*************************************************************************
NAME    
    handleA2DPSignallingConnectInd
    
DESCRIPTION
    handle a signalling channel connect indication

RETURNS
    
**************************************************************************/
void handleA2DPSignallingConnectInd(uint8 DeviceId, bdaddr SrcAddr)
{
#ifdef ENABLE_PEER
    sink_attributes attributes;
#endif

    
    /* before accepting check there isn't already a signalling channel connected to another AG */		
    if ( (theSink.features.EnableA2dpStreaming) &&
         ((!theSink.a2dp_link_data->connected[a2dp_primary]) || (!theSink.a2dp_link_data->connected[a2dp_secondary])) )
    {
        /* store the device_id for the new connection in the first available storage position */
        uint16 priority = (!theSink.a2dp_link_data->connected[a2dp_primary]) ? a2dp_primary : a2dp_secondary;
        
        A2DP_DEBUG(("Signalling Success, Device ID = %x\n",DeviceId));
        theSink.a2dp_link_data->connected[priority] = FALSE;
        theSink.a2dp_link_data->device_id[priority] = DeviceId;
        theSink.a2dp_link_data->bd_addr[priority] = SrcAddr;
        theSink.a2dp_link_data->list_id[priority] = 0;  
        
#ifdef ENABLE_PEER
        /* Check if the bd address of the connected Ag is the same as that connected to the peer,
            if so then disconnect the ag  */
        if(BdaddrIsSame(&theSink.remote_peer_ag_bd_addr , &SrcAddr))
        {
            A2DP_DEBUG(("Reject\n"));
            A2dpSignallingConnectResponse(DeviceId,FALSE);
            sinkDisconnectSlcFromDevice(&SrcAddr);            
            return;
        }
    
        getKnownPeerDeviceAttributes(priority , &attributes);
        
        if (theSink.a2dp_link_data->peer_device[priority] == remote_device_unknown)
        {   /* Determine remote device type before accepting connection */
            if (!peerCredentialsRequest(&SrcAddr))
            {   /* Peer credentials not requested */
                if (theSink.inquiry.session == inquiry_session_peer)
                {   /* We are initiating a Peer session */
                    A2DP_DEBUG(("Peer Device SDP record not requested, assuming Peer\n"));
                        
                    /* Assume device is a ShareMe sink without custom avrcp operation and allow to connect */
                    attributes.peer_device = remote_device_peer;
                    attributes.peer_features = remote_features_shareme_a2dp_sink;
                    attributes.peer_version = 0;
                    deviceManagerStoreAttributes(&attributes, &SrcAddr);
                }
                else
                {
                    /* Assume device is a standard source */
                    attributes.peer_device = remote_device_nonpeer;
                    attributes.peer_features = remote_features_none;
                    attributes.peer_version = 0;
                    deviceManagerStoreAttributes(&attributes, &SrcAddr);
                }
                    
                if(!peerLinkReservedCanDeviceConnect(&SrcAddr))
                {
                    /* If there is already an AG connected and the PeerLinkReserved is set, then do not allow 
                                       another AG to connect */
                    A2DP_DEBUG(("Reject\n"));
                    A2dpSignallingConnectResponse(DeviceId,FALSE);
                }
                else
                {
                    A2DP_DEBUG(("Accept\n"));
                        
                    if (attributes.peer_device == remote_device_nonpeer)
                    {
                        /* indicate that this is a remote connection */
                        theSink.a2dp_link_data->remote_ag_connection = TRUE; 
                    } 
                      
                    A2dpSignallingConnectResponse(DeviceId,TRUE);
#ifdef ENABLE_AVRCP
                    sinkAvrcpCheckManualConnectReset(&SrcAddr);        
#endif
                }
            }
        }
        else
        {
            /* Update the optional codecs supported by the remote device */
            peerSetRemoteSupportedCodecs(attributes.peer_supported_optional_codecs);            
            
            if(!peerLinkReservedCanDeviceConnect(&SrcAddr))
            {
                /* If there is already an AG connected and the PeerLinkReserved is set, then do not allow 
                              another AG to connect */
                A2DP_DEBUG(("Reject\n"));
                A2dpSignallingConnectResponse(DeviceId,FALSE);
            }
            else
#else
             theSink.a2dp_link_data->peer_device[priority] = remote_device_nonpeer;
#endif
            {
                A2DP_DEBUG(("Accept\n"));
                /* indicate that this is a remote connection */
                if (theSink.a2dp_link_data->peer_device[priority] == remote_device_nonpeer)
                {
                    theSink.a2dp_link_data->remote_ag_connection = TRUE;
                }
           
                A2dpSignallingConnectResponse(DeviceId,TRUE);
#ifdef ENABLE_AVRCP
                sinkAvrcpCheckManualConnectReset(&SrcAddr);        
#endif
            }
#ifdef ENABLE_PEER           
        }
#endif
    }
    else
    {
        A2DP_DEBUG(("Reject\n"));
        A2dpSignallingConnectResponse(DeviceId,FALSE);
    }
}


/*************************************************************************
NAME    
    handleA2DPSignallingConnected
    
DESCRIPTION
    handle a successful confirm of a signalling channel connected

RETURNS
    
**************************************************************************/
void handleA2DPSignallingConnected(a2dp_status_code status, uint8 DeviceId, bdaddr SrcAddr, bool locally_initiated)
{
    sink_attributes attributes;

    /* Use default attributes if none exist is PS */
    deviceManagerGetDefaultAttributes(&attributes, dev_type_ag);
    deviceManagerGetAttributes(&attributes, &SrcAddr);

    /* Continue connection procedure */
    if(!theSink.a2dp_link_data->remote_ag_connection)    
    {
        uint32 delay = theSink.conf1->timeouts.SecondAGConnectDelayTime_s;

#ifdef ENABLE_PEER
        /* If we have just connected to a peer it may already have an active
           a2dp stream. If so, we need to delay polling the next device in 
           the PDL to give the peer relay state time to update to
           streaming. This is so we can tell it is streaming when
           deviceManagerCanConnect is called and stop the connection.

           Otherwise paging the device will interrupt the peer a2dp stream -
           a known limitation. */
        if (attributes.peer_device == remote_device_peer)
        {
            A2DP_DEBUG(("A2DP: Delaying next slc connect request by up to 2 seconds\n"));
            /* add 1 second reconnection delay */            
            delay += (A2DP_PEER_PDL_PAGING_DELAY/2);
            /* add a random delay of up to 1.024 second to prevent reconnection attempts on both
               sides occuring at the same time every reconnection attempt cycle */
            delay += (UtilRandom() >> 6);
        }
#endif

        MessageSendLater(&theSink.task, EventSysContinueSlcConnectRequest, 0, delay);
    }
    else
    {
        /* reset remote connection indication flag */    
        theSink.a2dp_link_data->remote_ag_connection = FALSE;
    }
            
    /* check for successful connection */
    if (status != a2dp_success)
    {
        uint16 priority;
        
        A2DP_DEBUG(("Signalling Failed device=%u [Status %d]\n", DeviceId, status));
        
#ifdef ENABLE_PEER        
        if ( (theSink.inquiry.session == inquiry_session_peer) && (status == a2dp_operation_fail) &&
             isUnconnectedA2dpDevice(&SrcAddr, &priority) &&
             (theSink.a2dp_link_data->peer_device[priority] == remote_device_peer) )
        {   /* A rejected pairing with a Peer device */
            A2DP_DEBUG(("Remove Peer %u from PDL\n", priority));
            ConnectionSmDeleteAuthDevice(&theSink.a2dp_link_data->bd_addr[priority]);
        }
        /* When there is a2dp failure with TWS device then decrement NoOfReconnectionAttempts if its non-zero*/
        else if(isTWSDeviceAvailable(DeviceId))
        {
            /* if set to repeat a connection attempt decrement this as an attempt has occured with 
              TWS device */
            if(theSink.NoOfReconnectionAttempts)
                  theSink.NoOfReconnectionAttempts--;
        }
#endif
        /* If necessary, clear appropriate link data structure which will have been filled on an incoming connection */
        if ((status != a2dp_wrong_state) && (status != a2dp_max_connections))   /* TODO: Temp fix */
        {
        if ( BdaddrIsSame(&SrcAddr,&theSink.a2dp_link_data->bd_addr[priority=a2dp_primary]) || 
             BdaddrIsSame(&SrcAddr,&theSink.a2dp_link_data->bd_addr[priority=a2dp_secondary]) )
        {
            A2DP_DEBUG(("Clearing link data for %u\n", priority));

            theSink.a2dp_link_data->peer_device[priority] = remote_device_unknown;
            theSink.a2dp_link_data->peer_features[priority] = remote_features_none;
            theSink.a2dp_link_data->peer_version[priority] = 0;
            theSink.a2dp_link_data->local_peer_status[priority] = 0;
            theSink.a2dp_link_data->remote_peer_status[priority] = 0;
            theSink.a2dp_link_data->connected[priority] = FALSE;
            theSink.a2dp_link_data->device_id[priority] = INVALID_DEVICE_ID;
            BdaddrSetZero(&theSink.a2dp_link_data->bd_addr[priority]);
            theSink.a2dp_link_data->list_id[priority] = 0;
            theSink.a2dp_link_data->av_source[priority] = RELAY_SOURCE_NULL;
            theSink.a2dp_link_data->link_role[priority] = LR_UNKNOWN_ROLE;
#ifdef ENABLE_PEER
            theSink.a2dp_link_data->peer_dsp_required_buffering_level[priority] = getPeerDSPBufferingRequired(0);
#endif
            linklossResetStreamProtection(priority);
        }
        }
        
#ifdef ENABLE_AVRCP
        sinkAvrcpCheckManualConnectReset(&SrcAddr);        
#endif

        /* if a failed inquiry connect then restart it */
        if((theSink.inquiry.action == rssi_pairing)&&(theSink.inquiry.session == inquiry_session_peer))        
        {
            inquiryStop();
            inquiryPair( inquiry_session_peer, FALSE );
        }
    }
    /* connection was successful */
    else
    {
        /* Send a message to request a role indication and make necessary changes as appropriate, message
           will be delayed if a device initiated connection to another device is still in progress */
        A2DP_DEBUG(("handleA2DPSignallingConnected: Asking for role check\n"));
        
        /* cancel any link loss reminders */
        linklossCancelLinkLossTone();
        
        /* cancel any pending messages and replace with a new one */
        MessageCancelFirst(&theSink.task , EventSysCheckRole);    
        MessageSendConditionally (&theSink.task , EventSysCheckRole , NULL , &theSink.rundata->connection_in_progress  );
    
        /* check for a link loss condition, if the device has suffered a link loss and was
           succesfully reconnected by the a2dp library a 'signalling connected' event will be 
           generated, check for this and retain previous connected ID for this indication */
        if(((theSink.a2dp_link_data->connected[a2dp_primary])&&(BdaddrIsSame(&SrcAddr, &theSink.a2dp_link_data->bd_addr[a2dp_primary])))||
           ((theSink.a2dp_link_data->connected[a2dp_secondary])&&(BdaddrIsSame(&SrcAddr, &theSink.a2dp_link_data->bd_addr[a2dp_secondary]))))
        {
            /* reconnection is the result of a link loss, don't assign a new id */    		
            A2DP_DEBUG(("Signalling Connected following link loss [Status %d]\n", status));
            /* Handle Link loss.  Inform Accessory that reconnection is made. If this is not done, Accessory connect is not made. */    		
            deviceManagerHandleLinkLoss(&SrcAddr);
        }
        else
        {
            /* store the device_id for the new connection in the first available storage position */
            if (BdaddrIsSame(&SrcAddr,&theSink.a2dp_link_data->bd_addr[a2dp_primary]) || 
                (BdaddrIsZero(&theSink.a2dp_link_data->bd_addr[a2dp_primary]) && !theSink.a2dp_link_data->connected[a2dp_primary]))
            {
            	A2DP_DEBUG(("Signalling Success, Primary ID = %x\n",DeviceId));
                theSink.a2dp_link_data->connected[a2dp_primary] = TRUE;
                theSink.a2dp_link_data->device_id[a2dp_primary] = DeviceId;
                theSink.a2dp_link_data->bd_addr[a2dp_primary] = SrcAddr;            
                theSink.a2dp_link_data->list_id[a2dp_primary] = deviceManagerSetPriority(&SrcAddr);
                theSink.a2dp_link_data->media_reconnect[a2dp_primary] = FALSE;
                theSink.a2dp_link_data->latency[a2dp_primary] = 0;
                theSink.a2dp_link_data->av_source[a2dp_primary] = RELAY_SOURCE_NULL;
                theSink.a2dp_link_data->local_peer_status[a2dp_primary] = 0;
                theSink.a2dp_link_data->remote_peer_status[a2dp_primary] = 0;
                theSink.a2dp_link_data->peer_features[a2dp_primary] = remote_features_none;
                theSink.a2dp_link_data->peer_version[a2dp_primary] = 0;
                theSink.a2dp_link_data->link_role[a2dp_primary] = LR_UNKNOWN_ROLE;
                theSink.a2dp_link_data->playing[a2dp_primary] = FALSE;
#ifdef ENABLE_PEER
                theSink.a2dp_link_data->peer_dsp_required_buffering_level[a2dp_primary] = getPeerDSPBufferingRequired(0);
#endif
                linklossResetStreamProtection(a2dp_primary);
            }
            /* this is the second A2DP signalling connection */
            else if (BdaddrIsSame(&SrcAddr,&theSink.a2dp_link_data->bd_addr[a2dp_secondary]) || 
                     (BdaddrIsZero(&theSink.a2dp_link_data->bd_addr[a2dp_secondary]) && !theSink.a2dp_link_data->connected[a2dp_secondary]))
            {
            	A2DP_DEBUG(("Signalling Success, Secondary ID = %x\n",DeviceId));
                theSink.a2dp_link_data->connected[a2dp_secondary] = TRUE;
                theSink.a2dp_link_data->device_id[a2dp_secondary] = DeviceId;
                theSink.a2dp_link_data->bd_addr[a2dp_secondary] = SrcAddr;            
                theSink.a2dp_link_data->list_id[a2dp_secondary] = deviceManagerSetPriority(&SrcAddr);
                theSink.a2dp_link_data->media_reconnect[a2dp_secondary] = FALSE;
                theSink.a2dp_link_data->latency[a2dp_secondary] = 0;
                theSink.a2dp_link_data->av_source[a2dp_secondary] = RELAY_SOURCE_NULL;
                theSink.a2dp_link_data->local_peer_status[a2dp_secondary] = 0;
                theSink.a2dp_link_data->remote_peer_status[a2dp_secondary] = 0;
                theSink.a2dp_link_data->peer_features[a2dp_secondary] = remote_features_none;
                theSink.a2dp_link_data->peer_version[a2dp_secondary] = 0;
                theSink.a2dp_link_data->link_role[a2dp_secondary] = LR_UNKNOWN_ROLE;
                theSink.a2dp_link_data->playing[a2dp_secondary] = FALSE;
#ifdef ENABLE_PEER
                theSink.a2dp_link_data->peer_dsp_required_buffering_level[a2dp_secondary] = getPeerDSPBufferingRequired(0);
#endif
                linklossResetStreamProtection(a2dp_secondary);
            }
        }
        
  	 /* Ensure the underlying ACL is encrypted */       
        ConnectionSmEncrypt( &theSink.task , A2dpSignallingGetSink(DeviceId) , TRUE );
        ConnectionSetLinkSupervisionTimeout(A2dpSignallingGetSink(DeviceId), SINK_LINK_SUPERVISION_TIMEOUT);
	
        /* If the device is off then disconnect */
        if (stateManagerGetState() == deviceLimbo)
        {
            A2dpSignallingDisconnectRequest(DeviceId);
        }
        else
        {
            a2dp_link_priority priority;
            
                
            /* For a2dp connected Tone only */
            MessageSend(&theSink.task,  EventSysA2dpConnected, 0);	
					
            /* find structure index of deviceId */
            if(getA2dpIndex(DeviceId, (uint16*)&priority))
            {
#ifdef ENABLE_PARTYMODE
                /* check whether party mode is enabled */
                if((theSink.PartyModeEnabled)&&(theSink.features.PartyMode))
                {
                    /* Cancel any existing partymode timer running for AG */
                    MessageCancelAll(&theSink.task,(EventSysPartyModeTimeoutDevice1 + priority));
                    /* start a timer when a device connects in party mode, if no music is played before the timeout
                       occurs the device will get disconnected to allow other devices to connect, this timer is a configurable 
                       item in Sink configuration tool */
                    MessageSendLater(&theSink.task,(EventSysPartyModeTimeoutDevice1 + priority),0,D_SEC(theSink.conf1->timeouts.PartyModeMusicTimeOut_s));
                    /* set paused flag */
                    if(priority == a2dp_primary)
                    {
                        theSink.rundata->partymode_pause.audio_source_primary_paused = FALSE;
                    }
                    else
                    {
                        theSink.rundata->partymode_pause.audio_source_secondary_paused = FALSE;
                    }
                }
#endif                        
                /* update master volume level */
                theSink.volume_levels.a2dp_volume[priority].main_volume = attributes.a2dp.volume;
                theSink.a2dp_link_data->clockMismatchRate[priority] = attributes.a2dp.clock_mismatch;
                
#ifdef ENABLE_PEER
                if ((slcDetermineConnectAction() & AR_Rssi) && theSink.inquiry.results)
                {   /* Set attributes when connecting from a Peer Inquiry */
                    remote_device new_device = theSink.inquiry.results[theSink.inquiry.attempting].peer_device;
                    
                    if (attributes.peer_device == remote_device_unknown && new_device != remote_device_unknown)
                    {
                        attributes.peer_device = new_device;
                    attributes.peer_supported_optional_codecs = theSink.a2dp_link_data->remote_peer_optional_codecs;
                    
                    if (attributes.peer_device == remote_device_peer)
                    {
                        attributes.peer_features = theSink.inquiry.results[theSink.inquiry.attempting].peer_features;
                        attributes.peer_version = theSink.inquiry.peer_version;
                    }
                }
                }
                
                if (attributes.peer_device != remote_device_unknown)
                {   /* Only update link data if device type is already known to us */
                    theSink.a2dp_link_data->peer_device[priority] = attributes.peer_device;
                    theSink.a2dp_link_data->peer_features[priority] = attributes.peer_features;
                    theSink.a2dp_link_data->peer_version[priority] = attributes.peer_version;
                    theSink.a2dp_link_data->peer_dsp_required_buffering_level[priority] = getPeerDSPBufferingRequired(attributes.peer_version);

                    /* If the remote device is a peer update the remote
                       supported codecs from the stored sink_attributes. */
                    if (attributes.peer_device == remote_device_peer)
                    {
                        peerSetRemoteSupportedCodecs(attributes.peer_supported_optional_codecs);
                    }
                }
                 /* Reset peer_link_loss_reconnect if a remote peer device connects or if both the a2dp links are connected */
                if ((theSink.a2dp_link_data->peer_device[priority] == remote_device_peer ) ||
                    (theSink.a2dp_link_data->connected[a2dp_primary] && theSink.a2dp_link_data->connected[a2dp_secondary]))
                {
                    theSink.a2dp_link_data->peer_link_loss_reconnect = FALSE;
                }
            	A2DP_DEBUG(("Remote device type = %u\n",theSink.a2dp_link_data->peer_device[priority]));
            	A2DP_DEBUG(("Remote device features = 0x%x\n",theSink.a2dp_link_data->peer_features[priority]));
                A2DP_DEBUG(("Remote device version = 0x%X\n",theSink.a2dp_link_data->peer_version[priority]));
#endif

                /* We are now connected */      
                if (stateManagerGetState() < deviceConnected && stateManagerGetState() != deviceLimbo)
                {
#ifdef ENABLE_PEER                    
                    if ((stateManagerGetState() == deviceConnDiscoverable) && 
                        (attributes.peer_device == remote_device_peer) &&
                        (theSink.inquiry.session != inquiry_session_peer))
                    {
                        theSink.inquiry.session = inquiry_session_normal;
                        stateManagerEnterConnDiscoverableState(TRUE);
                    }
                    else
#endif                        
                    {
                        stateManagerEnterConnectedState(); 	
                    }
                }

                /* Make sure we store this device */
                attributes.profiles |= sink_a2dp;
                deviceManagerStoreAttributes(&attributes, &SrcAddr);
                
                /* Update the linkloss managemt for this device*/
                linklossUpdateManagement(&theSink.a2dp_link_data->bd_addr[priority]);
                
                /* check on signalling check indication if the a2dp was previously in a suspended state,
                   this can happen if the device has suspended a stream and the phone has chosen to drop
                   the signalling channel completely, open the media connection or the feature to open a media
                   channel on signalling connected option is enabled */
#ifdef ENABLE_PEER
                if (theSink.a2dp_link_data->peer_device[priority] != remote_device_peer)
#endif
                {   /* Unknown or non-peer device */
#ifdef ENABLE_PEER
                    /* Check if the bd address of the connected Ag is the same as that connected to the peer,
                         if so then disconnect the ag  */
                    if(BdaddrIsSame(&theSink.remote_peer_ag_bd_addr , &SrcAddr) || !peerLinkReservedCanDeviceConnect(&SrcAddr))
                    {
                        sinkDisconnectSlcFromDevice(&SrcAddr);
                        disconnectA2dpAvrcpFromDevice(&SrcAddr);
                    } 
                    else 
                    {
                        /*If the A2DP has connected successfully to a non-peer device then notify this to the peer device */
                        sinkAvrcpUpdatePeerWirelessSourceConnected(A2DP_AUDIO, &SrcAddr);
                    }
                    
            
                    /* If peer is already streaming and a new AG is
                       connecting we need to turn on a2dp stream protection. */
                    if (!theSink.features.PeerLinkRecoveryWhileStreaming
                        && !peerIsTwsMaster()
                        && (theSink.peer.current_state >= RELAY_STATE_STARTING))
                    {
                        linklossProtectStreaming(linkloss_stream_protection_on);
                    }
                    
                    PEER_UPDATE_REQUIRED_RELAY_STATE("A2DP SOURCE CONNECTED");
#endif
                    if ((theSink.a2dp_link_data->SuspendState[priority] == a2dp_local_suspended) ||
                        (theSink.features.EnableA2dpMediaOpenOnConnection))
                    {
                        connectA2dpStream( priority, D_SEC(5) );
                    }
                }
#ifdef ENABLE_PEER
                else
                {   /* Peer device */
                    
                    /* Use the stored TWS audio routing preference */
                    sinkA2dpSetPeerAudioRouting(attributes.master_routing_mode, attributes.slave_routing_mode);
                    
                    /* Disable the incompatible codecs between the peer devices and also reconnect any 
                                    connected source negotiating one of the compatible codecs  */
                    disableIncompatibleOptionalCodecs();
                        
                        
                    /* Reconnect the AV source if required based on the supported codec set */
                    reconnectAvSource();
                    
                    /* Update permanent pairing */
                    peerUpdatePairing(priority, &attributes);
                    
                    /* Kick Peer state machine to start relaying audio, if necessary */
                    peerAdvanceRelayState(RELAY_EVENT_CONNECTED);

                    if (!theSink.features.PeerLinkRecoveryWhileStreaming)
                    {
                        linklossProtectStreaming(linkloss_stream_protection_on);
                    }
                }
#endif
            }
            
            /* if rssi pairing check to see if need to cancel rssi pairing or not */           
            if(theSink.inquiry.action == rssi_pairing)
            {
                /* if rssi pairing has completed then stop it progressing further */            
                if(!((theSink.features.PairIfPDLLessThan)&&( ConnectionTrustedDeviceListSize() < theSink.features.PairIfPDLLessThan )))
                {
                    inquiryStop();
                }
            }

#ifdef ENABLE_AVRCP

            if(theSink.features.avrcp_enabled)
            {
                if(BdaddrIsSame(&theSink.a2dp_link_data->reconnected_ag_address , &SrcAddr))
                {
                    theSink.avrcp_link_data->avrcp_manual_connect = TRUE;
                }
                
                if (theSink.avrcp_link_data->avrcp_manual_connect)
                {
                    theSink.avrcp_link_data->avrcp_play_addr = SrcAddr;
                }
                    
                if(getA2dpIndex(DeviceId, (uint16*)&priority))
                {
#ifdef ENABLE_PEER
                    /* Peer devices that support custom AVRCP operation do not need to delay initial AVRCP connection */
                    if ((theSink.a2dp_link_data->peer_device[priority] == remote_device_peer) && (theSink.a2dp_link_data->peer_features[priority] & remote_features_peer_avrcp) && locally_initiated)
                    {
                        sinkAvrcpConnect(&theSink.a2dp_link_data->bd_addr[priority], DEFAULT_AVRCP_NO_CONNECTION_DELAY);
                    }
                    else
#endif
                    {
                        sinkAvrcpConnect(&theSink.a2dp_link_data->bd_addr[priority], DEFAULT_AVRCP_1ST_CONNECTION_DELAY);     
                    }
                }
            }

#endif    
	    }
    }
}


/*************************************************************************
NAME    
    connectA2dpStream
    
DESCRIPTION
    Issues a request to the A2DP library to establish a media stream to a
    remote device.  The request can be delayed by a certain amount of time 
    if required.

RETURNS
    
**************************************************************************/
void connectA2dpStream (a2dp_link_priority priority, uint16 delay)
{
    A2DP_DEBUG(("A2dp: connectA2dpStream[%u] delay=%u\n", priority, delay)); 
    
    if (!delay)
    {
        if (theSink.a2dp_link_data && theSink.a2dp_link_data->connected[priority])
        {
#ifdef ENABLE_PEER
            if (theSink.a2dp_link_data->peer_device[priority] == remote_device_unknown)
            {   /* Still waiting for Device Id SDP search outcome issued in handleA2DPSignallingConnected() */
                EVENT_STREAM_ESTABLISH_T *message = PanicUnlessNew(EVENT_STREAM_ESTABLISH_T);
                
                message->priority = priority;
                MessageSendLater(&theSink.task, EventSysStreamEstablish, message, 200);  /* Ideally we'd send conditionally, but there isn't a suitable variable */
                
                A2DP_DEBUG(("local device is unknown, re-issue stream establish event\n")); 
            }
            else if (theSink.a2dp_link_data->peer_device[priority] == remote_device_nonpeer)
#endif
            {   /* Open media channel to AV Source */
                A2DP_DEBUG(("local device is non-peer (AV Source)\n"));
                if (A2dpMediaGetState(theSink.a2dp_link_data->device_id[priority], 0) == a2dp_stream_idle)
                {
                    A2DP_DEBUG(("AV Source stream idle\n"));
                    A2DP_DEBUG(("Send open req to AV Source, using defualt seid list\n"));
                    openStream(priority, 0);
                }
            }
        }
    }
    else
    {
        EVENT_STREAM_ESTABLISH_T *message = PanicUnlessNew(EVENT_STREAM_ESTABLISH_T);
        
        message->priority = priority;
        MessageSendLater(&theSink.task, EventSysStreamEstablish, message, delay);
        
        A2DP_DEBUG(("... wait for %u msecs\n", delay)); 
    }
}


/*************************************************************************
NAME    
    handleA2DPOpenInd
    
DESCRIPTION
    handle an indication of an media channel open request, decide whether 
    to accept or reject it

RETURNS
    
**************************************************************************/
void handleA2DPOpenInd(uint8 DeviceId, uint8 seid)
{
   	A2DP_DEBUG(("A2dp: OpenInd DevId = %d, seid = 0x%X\n",DeviceId, seid)); 

#ifdef ENABLE_PEER
    {
        uint16 Id;
        bdaddr bd_addr;       

        /*Get the A2DP index from the BD Address corresponding to the DeviceId */
        if(A2dpDeviceGetBdaddr(DeviceId, &bd_addr) && getA2dpIndexFromBdaddr(&bd_addr , &Id))
        {   /* Always accept an open indication, regardless of whether it comes from an AV Source / Peer */
            theSink.a2dp_link_data->seid[Id] = seid;
            
            if (theSink.a2dp_link_data->peer_device[Id] == remote_device_peer)
            {
                peerAdvanceRelayState(RELAY_EVENT_OPENING);
            }            
            
            A2DP_DEBUG(("Ind from %s,  Sending open resp\n", rdname[theSink.a2dp_link_data->peer_device[Id]]));
            A2dpMediaOpenResponse(DeviceId, TRUE);
        }
    }
#else        
    /* accept this media connection */
    if(A2dpMediaOpenResponse(DeviceId, TRUE))    
    {
        uint16 Id;
		A2DP_DEBUG(("Open Success\n"));
           
        /* find structure index of deviceId */
        if(getA2dpIndex(DeviceId, &Id))
            theSink.a2dp_link_data->device_id[Id] = DeviceId;

    }
#endif
}

/*************************************************************************
NAME    
    handleA2DPOpenCfm
    
DESCRIPTION
    handle a successful confirm of a media channel open

RETURNS
    
**************************************************************************/
void handleA2DPOpenCfm(uint8 DeviceId, uint8 StreamId, uint8 seid, a2dp_status_code status)
{
    bool status_avrcp = FALSE;
    
	/* ensure successful confirm status */
	if (status == a2dp_success)
	{        
        uint16 Id;
        bdaddr bd_addr;  
#ifdef ENABLE_AVRCP            
        uint16 i;
#endif
        A2DP_DEBUG(("Open Success\n"));
           
        /*Get the A2DP index from the BD Address corresponding to the DeviceId */
        if(A2dpDeviceGetBdaddr(DeviceId, &bd_addr) && getA2dpIndexFromBdaddr(&bd_addr , &Id))
        {
            A2DP_DEBUG(("Open Success - id=%u\n",Id));
            
            /* set the current seid */         
            theSink.a2dp_link_data->device_id[Id] = DeviceId;
            theSink.a2dp_link_data->stream_id[Id] = StreamId;
            theSink.a2dp_link_data->seid[Id] = seid;
            theSink.a2dp_link_data->media_reconnect[Id] = FALSE;
            
            /* update the link policy */      
            linkPolicyUseA2dpSettings(DeviceId, StreamId, A2dpSignallingGetSink(DeviceId));
       
#ifdef ENABLE_PEER
            if (theSink.a2dp_link_data->peer_device[Id] == remote_device_peer)
            {
                peerAdvanceRelayState(RELAY_EVENT_OPENED);
				
                /* Send the audio enhancement  and user eq settings(if present) to the peer(slave) if this device is a master*/
                peerSendAudioEnhancements();

#if defined ENABLE_GAIA && defined ENABLE_GAIA_PERSISTENT_USER_EQ_BANK                
                peerSendUserEqSettings();
#endif
            }
            else
            {               
                MessageCancelAll(&theSink.task, EventSysStreamEstablish);
                if(BdaddrIsSame(&theSink.a2dp_link_data->bd_addr[Id], &theSink.a2dp_link_data->reconnected_ag_address))
                {
                    BdaddrSetZero(&theSink.a2dp_link_data->reconnected_ag_address);
                }
                
                PEER_UPDATE_REQUIRED_RELAY_STATE("A2DP SOURCE OPENED");
            }
#endif
        
           /* Start the Streaming if if in the suspended state.  Don't issue any AVRCP/AVDTP commands to a Peer device as these are managed separately */
           if ((theSink.a2dp_link_data->SuspendState[Id] == a2dp_local_suspended) && (theSink.a2dp_link_data->peer_device[Id] != remote_device_peer))
           {          
#ifdef ENABLE_AVRCP            
                /* does the device support AVRCP and is AVRCP currently connected to this device? */
                for_all_avrcp(i)
                {    
                    /* ensure media is streaming and the avrcp channel is that requested to be paused */
                    if ((theSink.avrcp_link_data->connected[i])&& 
                        (BdaddrIsSame(&theSink.a2dp_link_data->bd_addr[Id], &theSink.avrcp_link_data->bd_addr[i])))
                    {
                        /* attempt to resume playing the a2dp stream */
                        status_avrcp = sinkAvrcpPlayPauseRequest(i,AVRCP_PLAY);
                        A2DP_DEBUG(("Open Success - suspended - avrcp play\n"));
                        break;
                    }
                }
#endif
                /* if not avrcp enabled, use media start instead */
                if(!status_avrcp)
                {
                    A2dpMediaStartRequest(DeviceId, StreamId);
              		A2DP_DEBUG(("Open Success - suspended - start streaming\n"));
                }
    
                /* reset suspended state once start is sent*/            
                a2dpSetSuspendState(Id, a2dp_not_suspended);
            }
    	}
    }
   	else
   	{
   		A2DP_DEBUG(("Open Failure [result = %d]\n", status));
#ifdef ENABLE_PEER
        {
            uint16 Id;
            bdaddr bd_addr;
           
            if(A2dpDeviceGetBdaddr(DeviceId, &bd_addr) && getA2dpIndexFromBdaddr(&bd_addr , &Id))
            {
                if ((theSink.a2dp_link_data->peer_device[Id] == remote_device_peer))
                {
                    if (status != a2dp_no_signalling_connection)
                    {
                        peerAdvanceRelayState(RELAY_EVENT_NOT_OPENED);
                    }
                }
                else
                {
                    PEER_UPDATE_REQUIRED_RELAY_STATE("A2DP SOURCE OPEN FAILED");
                }
            }
        }
#endif
	}	
}

/*************************************************************************
NAME    
    handleA2DPClose
    
DESCRIPTION
    handle the close of a media channel 

RETURNS
    
**************************************************************************/
static void handleA2DPClose(uint8 DeviceId, uint8 StreamId, a2dp_status_code status)
{		
    /* check the status of the close indication/confirm */    
    if((status == a2dp_success) || (status == a2dp_disconnect_link_loss))
    {
        Sink sink = A2dpSignallingGetSink(DeviceId);
        uint16 a2dp_index;
        bdaddr bd_addr;
        
        A2DP_DEBUG(("A2dp: Close DevId = %d, StreamId = %d status = %u\n",DeviceId,StreamId, status)); 

#ifdef ENABLE_PEER
        /* In a link loss we may not get a AVRCP command to stop playing,
           so make sure the a2dp stream protection is turned off. */
        if ((status == a2dp_disconnect_link_loss) && !peerIsTwsMaster()
            && (getA2dpIndex(DeviceId, &a2dp_index)
                && theSink.a2dp_link_data->peer_device[a2dp_index] == remote_device_peer))
        {
            linklossProtectStreaming(linkloss_stream_protection_off);

            /* Re-enable stream protection on the link it is required on. */
            if (!theSink.features.PeerLinkRecoveryWhileStreaming)
            {
                linklossProtectStreaming(linkloss_stream_protection_on);
            }
        }
#endif

        /* route the audio using the appropriate codec/plugin */
        audioUpdateAudioRouting();
		
        /* update the link policy */
	    linkPolicyUseA2dpSettings(DeviceId, StreamId, sink);

        /* change device state if currently in one of the A2DP specific states */
        if(stateManagerGetState() == deviceA2DPStreaming)
        {
            /* the enter connected state function will determine if the signalling
               channel is still open and make the approriate state change */
            stateManagerEnterConnectedState();
        }
        
        /* user configurable event notification */
        MessageSend(&theSink.task, EventSysA2dpDisconnected, 0);
 
        /*As the A2DP media channel has been closed */
        if(A2dpDeviceGetBdaddr(DeviceId, &bd_addr) && getA2dpIndexFromBdaddr(&bd_addr , &a2dp_index))
        {
#ifdef ENABLE_PEER
                /* Reset seid now that media channel has closed */
            theSink.a2dp_link_data->seid[a2dp_index] = 0;
                
            if (theSink.a2dp_link_data->peer_device[a2dp_index] == remote_device_peer)
            {   /* Peer device has closed its media channel, now look to see if AV source is trying to initiate streaming */

                if(status == a2dp_disconnect_link_loss)
                {
                    /* Reset the peer features if the media channel has closed due to a linkloss, since we will re-populate this field 
                       when the peer is recovered from the linkloss and signalling channel connects */
                    theSink.a2dp_link_data->peer_features[a2dp_index] = remote_features_none;
                }
                
                PEER_UPDATE_REQUIRED_RELAY_STATE("PEER RELAY CLOSE");
                peerAdvanceRelayState(RELAY_EVENT_CLOSED);                    
            }
            else if (theSink.a2dp_link_data->peer_device[a2dp_index] == remote_device_nonpeer)
            {   /* AV Source closed it's media channel, update the required state */
                PEER_UPDATE_REQUIRED_RELAY_STATE("A2DP SOURCE CLOSED");
                
                /* Cancel any pending media channel establishment timers */
                MessageCancelAll(&theSink.task, EventSysStreamEstablish);

                if (theSink.a2dp_link_data->media_reconnect[a2dp_index])
                {   /* Disconnect signalling channel to AV Source (for IOP reasons) */
                    A2dpSignallingDisconnectRequest(theSink.a2dp_link_data->device_id[a2dp_index]);
                }
            }
#endif
        
#ifdef ENABLE_AVRCP
            if(theSink.features.avrcp_enabled)
            {
                /* assume device is stopped for AVRCP 1.0 devices */
                sinkAvrcpSetPlayStatus(&theSink.a2dp_link_data->bd_addr[a2dp_index], avrcp_play_status_stopped);
            }
#endif
        }
    }
    else
    {
        A2DP_DEBUG(("A2dp: Close FAILED status = %d\n",status)); 
    }
}

/*************************************************************************
NAME    
    handleA2DPSignallingDisconnected
    
DESCRIPTION
    handle the disconnection of the signalling channel
RETURNS
    
**************************************************************************/
void handleA2DPSignallingDisconnected(uint8 DeviceId, a2dp_status_code status,  bdaddr SrcAddr)
{
    uint16 Id;
    bool reconnect = FALSE;
#ifdef ENABLE_PEER
    bool peer_disconnected = FALSE;
#endif

    A2DP_DEBUG(("A2DP: SigDiscon DevId = %d status %u SrcAddr [%x:%x:%lx]\n",
        DeviceId, status, SrcAddr.uap, SrcAddr.nap, SrcAddr.lap));

    /* Cover LinkLoss event case first and cancel any link loss reminders.
    * This takes place here since the check below using getA2dpIndex will
    * return FALSE because device is not connected.
    * Make sure this is not an HFP profile (A2DP instead). */
    if(!theSink.hfp_profiles)
    {
        linklossCancelLinkLossTone();
    }
    
    /* check for successful disconnection status */
    if(getA2dpIndex(DeviceId, &Id))
    {
#ifdef ENABLE_PARTYMODE
        {
            /* check whether party mode is enabled */
            if((theSink.PartyModeEnabled)&&(theSink.features.PartyMode))
            {
                /* device disconnected, cancel its play music timeout */
                MessageCancelAll(&theSink.task,(EventSysPartyModeTimeoutDevice1 + Id));                 
            }
        }
#endif         

        /* If the device is using a2dp only, i.e. no hfp, then we have to
           manage the link loss event here instead of leaving it to the
           hfp library. */
        if (!(conn_hfp & deviceManagerProfilesConnected(&SrcAddr)))
        {
            /* Cancel any repeating link loss tone */
            linklossCancelLinkLossTone();

#ifdef ENABLE_PEER
            /* If stream protection is on we do not get a A2DP_SIGNALLING_LINKLOSS_IND
               from the a2dp lib, so generate an EventSysLinkLoss here instead. */
            if(status == a2dp_disconnect_link_loss && linklossIsStreamProtected(&SrcAddr))
            {
                linklossSendLinkLossTone(&theSink.a2dp_link_data->bd_addr[Id], 0);
            }
#endif
        }

#ifdef ENABLE_PEER

        /* Update the peer device attribnutes if the disconnection is due to a linkloss */
        if(status == a2dp_disconnect_link_loss)
        {
            sink_attributes attributes;
            getKnownPeerDeviceAttributes(Id , &attributes);
        }
        
        if (theSink.a2dp_link_data->peer_device[Id] == remote_device_peer)
        {   /* A Peer device */
            
            if (peerObtainPairingMode(Id) == PEER_PAIRING_MODE_TEMPORARY)
            {   
                /* Check if we need to wait for some-time, before removing peer device from DM, to allow peer device to connect */           
                bdaddr *message = PanicUnlessNew(bdaddr);              
                *message = theSink.a2dp_link_data->bd_addr[Id];       
                theSink.a2dp_link_data->peer_link_loss_reconnect = FALSE;              
                if (linklossIsStreamProtected(&theSink.a2dp_link_data->bd_addr[Id]) && (status == a2dp_disconnect_link_loss) && theSink.conf1->timeouts.A2dpLinkLossReconnectionTime_s)
                {
                    /* Start timer to allow remote peer to reconnect before deleting the temporary pairing */
                    A2DP_DEBUG(("A2dp: Starting a link loss timer\n"));
                    theSink.a2dp_link_data->peer_link_loss_reconnect = TRUE;
                    MessageCancelFirst(&theSink.task , EventSysA2DPPeerLinkLossTimeout);
                    MessageSendLater( &theSink.task , EventSysA2DPPeerLinkLossTimeout , 0, D_SEC(theSink.conf1->timeouts.A2dpLinkLossReconnectionTime_s) );                          
                }   
                MessageCancelFirst(&theSink.task , EventSysRemovePeerTempPairing);
                MessageSendConditionally( &theSink.task , EventSysRemovePeerTempPairing ,message,  &theSink.a2dp_link_data->peer_link_loss_reconnect); 
            }

            /* Store knowledge of Peer disconnecting to use later once a2dp parameters reset */
            peer_disconnected = TRUE;

            BdaddrSetZero(&theSink.a2dp_link_data->reconnected_ag_address);

            /* As the peer has disconnected, reset the remote_peer_ag_bd_addr*/
            BdaddrSetZero(&theSink.remote_peer_ag_bd_addr);
            theSink.remote_peer_audio_conn_status = 0; 

            /* Store the attributes in PS */
            deviceManagerUpdateAttributes(&SrcAddr, sink_a2dp, 0, (a2dp_link_priority)Id);
        }
        else
#endif
        {   /* Not a Peer device */
            
#ifdef ENABLE_PEER
            /*If the A2DP has disconnected from a non-peer device then notify this to the peer device */
            sinkAvrcpUpdatePeerSourceDisconnected(A2DP_AUDIO);
#endif

            if (theSink.a2dp_link_data->media_reconnect[Id])
            {   /* A reconnect of signalling and media channel has been requested, due to AAC not being supported by ShareMe */
                reconnect = TRUE;
            }
        
            /* Store the attributes in PS */
            deviceManagerUpdateAttributes(&SrcAddr, sink_a2dp, 0, (a2dp_link_priority)Id);   
        }

        /* Reset the a2dp parameter values */
        theSink.a2dp_link_data->peer_device[Id] = remote_device_unknown;
        theSink.a2dp_link_data->peer_features[Id] = 0;
        if(!(theSink.a2dp_link_data->local_peer_status[Id] & PEER_STATUS_POWER_OFF))
        {
            theSink.a2dp_link_data->local_peer_status[Id] = 0;
        }
        theSink.a2dp_link_data->remote_peer_status[Id] = 0;
        theSink.a2dp_link_data->media_reconnect[Id] = FALSE;
        BdaddrSetZero(&theSink.a2dp_link_data->bd_addr[Id]);
        theSink.a2dp_link_data->connected[Id] = FALSE;
        a2dpSetSuspendState(Id, a2dp_not_suspended);
        theSink.a2dp_link_data->device_id[Id] = INVALID_DEVICE_ID;
        theSink.a2dp_link_data->stream_id[Id] = INVALID_STREAM_ID; 
        theSink.a2dp_link_data->list_id[Id] = INVALID_LIST_ID;
        theSink.a2dp_link_data->seid[Id] = 0;
        theSink.a2dp_link_data->av_source[Id] = RELAY_SOURCE_NULL;
        theSink.a2dp_link_data->playing[Id] = FALSE;
#ifdef ENABLE_AVRCP
        theSink.a2dp_link_data->avrcp_support[Id] = avrcp_support_unknown;
#endif        

        /* Sends the indication to the device manager to send an event out if a device has disconnected*/
        deviceManagerDeviceDisconnectedInd(&SrcAddr);
      
        /*if the device is off then this is disconnect as part of the power off cycle, otherwise check
          whether device needs to be made connectable */	
#ifdef ENABLE_PEER
        /* also account for this being part of a Single Device Mode power off */
        if ( (stateManagerGetState() != deviceLimbo)
            && !(theSink.a2dp_link_data->local_peer_status[Id] & PEER_STATUS_POWER_OFF) ) 
#else
        if ( stateManagerGetState() != deviceLimbo)
#endif
        {
            /* Kick role checking now a device has disconnected */
            linkPolicyCheckRoles();
            
            /* at least one device disconnected, re-enable connectable for another 60 seconds */
            sinkEnableMultipointConnectable();

            /* if the device state still shows connected and there are no profiles currently
               connected then update the device state to reflect the change of connections */
    	    if ((stateManagerIsConnected()) && (!deviceManagerNumConnectedDevs()))
    	    {
    	        stateManagerEnterConnectableState( FALSE ) ;
    	    }
        }
        
#ifdef ENABLE_PEER
        /* A Peer/Source disconnecting will/may affect relay state */
        if (peer_disconnected)
        {
            /*Re-enable the optional codecs if disabled for this peer session.*/
            enableOptionalCodecs();

            /* Peer signalling channel has gone so media channel has also.  Let state machine know */
            peerAdvanceRelayState(RELAY_EVENT_DISCONNECTED);
			
            if(theSink.a2dp_link_data->local_peer_status[Id] & PEER_STATUS_POWER_OFF)
            {                
                MessageSend(&theSink.task, EventUsrPowerOff, 0);
            }
        }
        else
        {
            PEER_UPDATE_REQUIRED_RELAY_STATE("A2DP SOURCE DISCONNECTED");
        }
#endif            

#ifdef ENABLE_AVRCP
        if(theSink.features.avrcp_enabled)
        {
            sinkAvrcpDisconnect(&SrcAddr);     
        }
#endif
        
        if (reconnect)
        {   /* Kick off reconnect now */
            A2dpSignallingConnectRequest(&SrcAddr);
        }
    }    
    else
       	A2DP_DEBUG(("A2dp: Sig Discon FAILED status = %d\n",status)); 

}
       
/*************************************************************************
NAME    
    handleA2DPLinklossReconnectCancel
    
DESCRIPTION
    handle the indication of a link loss reconnection cancel
RETURNS
    
**************************************************************************/
void handleA2DPLinklossReconnectCancel(uint8 DeviceId)
{
    A2DP_DEBUG(("A2dp: handleA2DPLinklossReconnectCancel DevId = %d\n",DeviceId)); 

    UNUSED(DeviceId); /* Used in debug only */

    /* on receiving the linkloss reconnection timeout cancelled, stop the tone reminder */
    linklossCancelLinkLossTone();
}

/*************************************************************************
NAME    
    handleA2DPSignallingLinkloss
    
DESCRIPTION
    handle the indication of a link loss
RETURNS
    
**************************************************************************/
void handleA2DPSignallingLinkloss(uint8 DeviceId)
{
    uint16 Id;
    A2DP_DEBUG(("A2dp: handleA2DPSignallingLinkloss DevId = %d\n",DeviceId)); 
    
    if (getA2dpIndex(DeviceId, &Id))
    {
        /* Kick role checking now a device has disconnected */
        linkPolicyCheckRoles();

       if(theSink.conf1->timeouts.A2dpLinkLossReconnectionTime_s == 0)
       {
           theSink.a2dp_link_data->connected[Id] = FALSE;
       }
        
#ifdef ENABLE_PEER
        /* A Peer/Source disconnecting will/may affect relay state */
        if (theSink.a2dp_link_data->peer_device[Id] == remote_device_peer)
        {   /* Peer signalling channel has gone so media channel has also.  Let state machine know */
            PEER_UPDATE_REQUIRED_RELAY_STATE("PEER LINKLOSS");
            peerAdvanceRelayState(RELAY_EVENT_CLOSED);
        }
        else if (theSink.a2dp_link_data->peer_device[Id] == remote_device_nonpeer)
        {
            PEER_UPDATE_REQUIRED_RELAY_STATE("A2DP SOURCE LINKLOSS");
        }
#endif            
        audioUpdateAudioRouting();
        
        if(theSink.features.GoConnectableDuringLinkLoss || (theSink.a2dp_link_data->peer_device[Id] == remote_device_peer))
        {   /* Go connectable if feature enabled or remote is a peer device */
            sinkEnableConnectable(); 
            MessageCancelAll(&theSink.task, EventSysConnectableTimeout);   /* Ensure connectable mode does not get turned off */
        }

        /* Send an event to notify the user of link loss */
        linklossCancelLinkLossTone();
        linklossSendLinkLossTone(&theSink.a2dp_link_data->bd_addr[Id], 0);
    }
}

/*************************************************************************
NAME    
    handleA2DPStartStreaming
    
DESCRIPTION
    handle the indication of media start ind
RETURNS
    
**************************************************************************/
void handleA2DPStartInd(uint8 DeviceId, uint8 StreamId)
{
#ifdef ENABLE_PEER
    uint16 Id;
	bdaddr bd_addr; 
	
	/*Get the A2DP index from the BD Address corresponding to the DeviceId */
    if(A2dpDeviceGetBdaddr(DeviceId, &bd_addr) && getA2dpIndexFromBdaddr(&bd_addr , &Id))
    {
       /* update the link policy */
        linkPolicyUseA2dpSettings(DeviceId, StreamId, A2dpSignallingGetSink(DeviceId));
        
        /* Ensure suspend state is cleared now streaming has started/resumed */
        a2dpSetSuspendState(Id, a2dp_not_suspended);

        if (theSink.a2dp_link_data->peer_device[Id] == remote_device_peer)
        {   /* Just accept media stream from peer device */
            peerAdvanceRelayState(RELAY_EVENT_STARTING);
            A2dpMediaStartResponse(DeviceId, StreamId, TRUE);
        }
        else
        {   /* Open ind from true AV source */
			peerClaimRelay(TRUE);
            PEER_UPDATE_REQUIRED_RELAY_STATE("A2DP SOURCE STARTING");
            if ( !peerIsRelayAvailable() || !peerCheckSource(RELAY_SOURCE_A2DP) )
            {   /* Either no peer device connected or we have a peer that has control of the relay channel. In either case just accept the media stream */
                A2DP_DEBUG(("Relay channel NOT available\n"));
                A2DP_DEBUG(("Send start resp to AV Source\n"));
                A2dpMediaStartResponse(DeviceId, StreamId, TRUE);
            }
        }
    }
#else /* ENABLE_PEER */  

    /* Always accept the media stream */
    A2dpMediaStartResponse(DeviceId, StreamId, TRUE);
    
#ifdef ENABLE_PARTYMODE
    {
        uint16 Id;

        /* check whether party mode is enabled */
        if((getA2dpIndex(DeviceId, &Id))&&(theSink.PartyModeEnabled)&&(theSink.features.PartyMode))
        {
            /* device streaming, cancel its play music timeout */
            MessageCancelAll(&theSink.task,(EventSysPartyModeTimeoutDevice1 + Id));                 
        }
    }
#endif  /* ENABLE_PARTYMODE */                       

#endif /* ENABLE_PEER */
}

/*************************************************************************
NAME    
    handleA2DPStartStreaming
    
DESCRIPTION
    handle the indication of media start cfm
RETURNS
    
**************************************************************************/
void handleA2DPStartStreaming(uint8 DeviceId, uint8 StreamId, a2dp_status_code status)
{   
    /* check success status of indication or confirm */
    if(status == a2dp_success)
    {
        uint16 Id;     
        Sink sink = A2dpMediaGetSink(DeviceId, StreamId);
        
        A2DP_DEBUG(("A2dp: StartStreaming DevId = %d, StreamId = %d\n",DeviceId,StreamId));    
        /* find structure index of deviceId */
        if(getA2dpIndex(DeviceId, &Id))
        {          

#ifdef ENABLE_PARTYMODE
            /* check whether party mode is enabled */
            if((theSink.PartyModeEnabled)&&(theSink.features.PartyMode))
            {
                /* device streaming, cancel its play music timeout */
                MessageCancelAll(&theSink.task,(EventSysPartyModeTimeoutDevice1 + Id));                 
            }
#endif                        
            /* Ensure suspend state is cleared now streaming has started/resumed */
            a2dpSetSuspendState(Id, a2dp_not_suspended);

            /*Ensure that the device is not currently streaming from a different A2DP, if its found to be streaming then pause this incoming stream  */
#ifdef ENABLE_AVRCP            
            a2dpPauseNonRoutedSource(Id);
#endif

            /* route the audio using the appropriate codec/plugin */
            audioUpdateAudioRouting();
            
            /* enter the stream a2dp state if not in a call */
            stateManagerEnterA2dpStreamingState();
            
            /* update the link policy */
            linkPolicyUseA2dpSettings(DeviceId, StreamId, sink);
            
            /* set the current seid */         
            theSink.a2dp_link_data->stream_id[Id] = StreamId;

#ifdef ENABLE_PEER
            if (theSink.a2dp_link_data->peer_device[Id] == remote_device_peer)
            {   /* Peer media channel has started */
                peerAdvanceRelayState(RELAY_EVENT_STARTED);
                /* Clear the protection on the peer link in case it was
                   enabled when not playing audio. */
                linklossResetStreamProtection(Id);
                linklossUpdateManagement(&theSink.a2dp_link_data->bd_addr[Id]);

                /* Re-enable stream protection on the link it is required on. */
                if (!theSink.features.PeerLinkRecoveryWhileStreaming)
                {
                    linklossProtectStreaming(linkloss_stream_protection_on);
            }
            }
            else
            {
                PEER_UPDATE_REQUIRED_RELAY_STATE("A2DP SOURCE STARTED");
            }
#endif
            
#ifdef ENABLE_AVRCP           
            if(theSink.features.avrcp_enabled)
            {
                /* assume device is playing for AVRCP 1.0 devices */
                sinkAvrcpSetPlayStatus(&theSink.a2dp_link_data->bd_addr[Id], avrcp_play_status_playing);
            }
#endif 

#ifdef ENABLE_PEER
            /* Only update EQ mode for non peer devices i.e. for streams to be rendered locally */
            if (theSink.a2dp_link_data->peer_device[Id] != remote_device_peer)
#endif
            {
                /* Set the Stored EQ mode, ensure the DSP is currently streaming A2DP data before trying to
               set EQ mode as it might be that the device has a SCO routed instead */
                if(theSink.routed_audio == sink)
                {
                    A2DP_DEBUG(("A2dp: StartStreaming Set EQ mode = %d\n",theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_processing));
                    
                    /* set both EQ and Enhancements enables */
                    AudioSetMode(AUDIO_MODE_CONNECTED, &theSink.a2dp_link_data->a2dp_audio_mode_params);
                }
                else
                {
                    A2DP_DEBUG(("A2dp: Wrong sink Don't Set EQ mode = %d\n",theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_processing)); 
                }
            }
            /* when using the Soundbar manual audio routing and subwoofer support, 
               check to see if the a2dp audio is being routed, if not check whether
               an esco subwoofer channel is currently in use, if this is the case it 
               will be necessary to pause this a2dp stream to prevent disruption
               of the subwoofer esco link due to link bandwidth limitations */
#ifdef ENABLE_SUBWOOFER
            suspendWhenSubwooferStreamingLowLatency(Id);
#endif            
        }
    }
    else
    {
       	A2DP_DEBUG(("A2dp: StartStreaming FAILED status = %d\n",status)); 
#ifdef ENABLE_PEER
        {
            uint16 Id;
            
            if (getA2dpIndex(DeviceId, &Id))
            {
                if (theSink.a2dp_link_data->peer_device[Id] == remote_device_peer)
                {   /* Peer has rejected start of media channel, need to respond to any outstanding request from AV source */
                    peerAdvanceRelayState(RELAY_EVENT_NOT_STARTED);
                }
                else
                {
                    PEER_UPDATE_REQUIRED_RELAY_STATE("A2DP SOURCE START FAILED");
                }
            }
        }
#endif
    }

}             

/*************************************************************************
NAME    
    handleA2DPSuspendStreaming
    
DESCRIPTION
    handle the indication of media suspend from either the ind or the cfm
RETURNS
    
**************************************************************************/
void handleA2DPSuspendStreaming(uint8 DeviceId, uint8 StreamId, a2dp_status_code status)
{
    Sink sink = A2dpMediaGetSink(DeviceId, StreamId);

    /*Restart APD timer*/
    sinkStartAutoPowerOffTimer();

    /* if the suspend was not successful, issue a close instead */
    if(status == a2dp_rejected_by_remote_device)
    {
       	A2DP_DEBUG(("A2dp: Suspend Failed= %x, try close DevId = %d, StreamId = %d\n",status,DeviceId,StreamId)); 
        /* suspend failed so close media streaming instead */
        A2dpMediaCloseRequest(DeviceId, StreamId);
    }
    /* check success status of indication or confirm */
    else 
    {
        uint16 Id;
        
        A2DP_DEBUG(("A2dp: Suspend Ok DevId = %d, StreamId = %d\n",DeviceId,StreamId)); 
 
        if(getA2dpIndex(DeviceId, &Id)) 
        {
            /* no longer streaming so enter connected state if applicable */    	
            if(stateManagerGetState() == deviceA2DPStreaming)
            {
                /* the enter connected state function will determine if the signalling
                   channel is still open and make the approriate state change */
                stateManagerEnterConnectedState();

#ifdef ENABLE_PARTYMODE
                /* check whether party mode is enabled */
                if((theSink.PartyModeEnabled)&&(theSink.features.PartyMode))
                {
                    /* Cancel any existing partymode timer running for AG */
                    MessageCancelAll(&theSink.task,(EventSysPartyModeTimeoutDevice1 + Id));
                    /* start a timer when music is suspended in party mode, if no music is played before the timeout
                       occurs the device will get disconnected to allow other devices to connect, this timer is a configurable 
                       item in Sink configuration tool */
                    MessageSendLater(&theSink.task,(EventSysPartyModeTimeoutDevice1 + Id),0,D_SEC(theSink.conf1->timeouts.PartyModeMusicTimeOut_s));
            }
#endif
            }
            
            /* Ensure suspend state is set as streaming has now ceased */
            a2dpSetSuspendState(Id, a2dp_remote_suspended);

            /* This flag is used for SNK Role PTS qualification test case.TC_INT_SNK_SIG_SMG_BV_13_C 
             * In case of sink role, application should suspend the stream. So the check added for not to resume
             * streaming again. This flag set to FALSE in case of normal scenarios.
             */
            if ((theSink.a2dp_link_data) && (!theSink.a2dp_link_data->qual_disable_stream_resume))
            {
                /* route the audio using the appropriate codec/plugin */
                audioUpdateAudioRouting();
            }

            /* update the link policy */
            linkPolicyUseA2dpSettings(DeviceId, StreamId, sink);

#ifdef ENABLE_PEER
            if (theSink.a2dp_link_data->peer_device[Id] == remote_device_nonpeer)
            {   /* AV Source suspended it's media channel, update the required state */
                PEER_UPDATE_REQUIRED_RELAY_STATE("A2DP SOURCE SUSPENDED");
            }
            else if (theSink.a2dp_link_data->peer_device[Id] == remote_device_peer)
            {   /* Peer suspended it's media channel, look to see if local device has a streaming AV source */
                peerAdvanceRelayState(RELAY_EVENT_SUSPENDED);
            }
#endif
            
#ifdef ENABLE_AVRCP
            if (theSink.features.avrcp_enabled)
            {    
                /* assume device is paused for AVRCP 1.0 devices */
                sinkAvrcpSetPlayStatus(&theSink.a2dp_link_data->bd_addr[Id], avrcp_play_status_paused);
            }

#ifdef ENABLE_PEER
            A2DP_DEBUG(("A2DP: handleA2DPSuspendStreaming - cancel pause-suspend timeout a2dp_idx %u\n", Id));
            a2dpCancelPauseSuspendTimeout((a2dp_link_priority)Id);
#endif

#endif
        }
    }
}
  
/*************************************************************************
NAME    
    SuspendA2dpStream
    
DESCRIPTION
    called when it is necessary to suspend an a2dp media stream due to 
    having to process a call from a different AG. If the device supports
    AVRCP then issue a 'pause' which is far more reliable than trying a
    media_suspend request.
    
RETURNS
    
**************************************************************************/
void SuspendA2dpStream(a2dp_link_priority priority)
{
    if (theSink.a2dp_link_data && (theSink.a2dp_link_data->peer_device[priority] != remote_device_peer))
    {
        bool status = FALSE;

#ifdef ENABLE_AVRCP            
        uint16 i;
#endif    
        A2DP_DEBUG(("A2dp: Suspend A2DP Stream %x\n",priority)); 

        /* set the local suspend status indicator */
        a2dpSetSuspendState(priority, a2dp_local_suspended);

#ifdef ENABLE_AVRCP            
        /* does the device support AVRCP and is AVRCP currently connected to this device? */
        for_all_avrcp(i)
        {    
            /* ensure media is streaming and the avrcp channel is that requested to be paused */
            if ((theSink.avrcp_link_data->connected[i])&&(theSink.a2dp_link_data->connected[priority])&&
                (BdaddrIsSame(&theSink.a2dp_link_data->bd_addr[priority], &theSink.avrcp_link_data->bd_addr[i])))
            {
                /* check whether the a2dp connection is streaming data */
                if (A2dpMediaGetState(theSink.a2dp_link_data->device_id[i], theSink.a2dp_link_data->stream_id[i]) == a2dp_stream_streaming)
                {
                    /* attempt to pause the a2dp stream */
                    status = sinkAvrcpPlayPauseRequest(i,AVRCP_PAUSE);
                }
                break;
            }
        }
#endif
    
        /* attempt to suspend stream if avrcp pause was not successful, if not successful then close it */
        if(!status)
        {
            if(!A2dpMediaSuspendRequest(theSink.a2dp_link_data->device_id[priority], theSink.a2dp_link_data->stream_id[priority]))
            {
                /* suspend failed so close media streaming */
                A2dpMediaCloseRequest(theSink.a2dp_link_data->device_id[priority], theSink.a2dp_link_data->stream_id[priority]);
            }
        }

        /* no longer streaming so enter connected state if applicable */    	
        if(stateManagerGetState() == deviceA2DPStreaming)
        {
            /* the enter connected state function will determine if the signalling
               channel is still open and make the approriate state change */
            stateManagerEnterConnectedState();
        }
    }
}

#ifdef ENABLE_PEER
/*************************************************************************
NAME    
     a2dpSuspendNonRoutedStream

DESCRIPTION
    Special-case of sending an A2DP suspend to an AG when in TWS extended
    mode (an AG connected to each peer).
    The intention is to force the AG to suspend its A2DP stream so that it
    does not intefere with the A2DP stream from the peer device.

    It is a workaround for a known bluetooth bandwidth issue when
    handling > 1 A2DP stream in the 'streaming' state.

RETURNS
    none

**************************************************************************/
void  a2dpSuspendNonRoutedStream(a2dp_link_priority priority)
{
    uint16 index;

    if (a2dpGetPeerIndex(&index)
        && !a2dpIsIndexPeer(priority)
        && !theSink.a2dp_link_data->playing[priority])    
    {
        A2DP_DEBUG(("A2dp:  a2dpSuspendNonRoutedStream priority %x\n",priority));

        /* set the local suspend status indicator */
        a2dpSetSuspendState(priority, a2dp_remote_suspended);

        /* attempt to suspend stream if avrcp pause was not successful, if not successful then close it */
        A2DP_DEBUG(("A2DP:   Sending suspend\n"));
        if(!A2dpMediaSuspendRequest(theSink.a2dp_link_data->device_id[priority], theSink.a2dp_link_data->stream_id[priority]))
        {
            A2DP_DEBUG(("A2DP:   Sending suspend failed\n"));
        }

        /* no longer streaming so enter connected state if applicable */        
        if(stateManagerGetState() == deviceA2DPStreaming)
        {
            /* the enter connected state function will determine if the signalling
                        channel is still open and make the approriate state change */
            stateManagerEnterConnectedState();
        }
    }
}
#endif

/*************************************************************************
NAME    
    a2dpSuspended
    
DESCRIPTION
    Helper to indicate whether A2DP is suspended on given source
RETURNS
    TRUE if A2DP suspended, otherwise FALSE
**************************************************************************/
a2dp_suspend_state a2dpSuspended(a2dp_link_priority priority)
{
    if(!theSink.a2dp_link_data) return FALSE;
    return theSink.a2dp_link_data->SuspendState[priority];
}
   

/*************************************************************************
NAME    
    ResumeA2dpStream
    
DESCRIPTION
    Called to resume a suspended A2DP stream
RETURNS
    
**************************************************************************/
bool ResumeA2dpStream(a2dp_link_priority priority)
{
    bool ready_to_connect = FALSE;
    if (theSink.a2dp_link_data && (theSink.a2dp_link_data->peer_device[priority] != remote_device_peer))
    {
        A2DP_DEBUG(("A2dp: ResumeA2dpStream\n" )) ;   

        /* need to check whether the signalling channel hsa been dropped by the AV/AG source */
        if(A2dpSignallingGetState(theSink.a2dp_link_data->device_id[priority]) == a2dp_signalling_connected)
        {
            a2dp_stream_state state = sinkA2dpGetStreamState(priority);
            bool status_avrcp = FALSE;
#ifdef ENABLE_AVRCP
            uint16 i;
#endif
            /* is media channel still open? or is it streaming already? */
            if(state == a2dp_stream_open)
            {
#ifdef ENABLE_AVRCP            
                /* does the device support AVRCP and is AVRCP currently connected to this device? */
                for_all_avrcp(i)
                {    
                    /* ensure media is streaming and the avrcp channel is that requested to be paused */
                    if ((theSink.avrcp_link_data->connected[i])&&(theSink.a2dp_link_data->connected[priority])&&
                        (BdaddrIsSame(&theSink.a2dp_link_data->bd_addr[priority], &theSink.avrcp_link_data->bd_addr[i])))
                    {
                        /* attempt to resume playing the a2dp stream */
                        status_avrcp = sinkAvrcpPlayPauseRequest(i,AVRCP_PLAY);
                        /* update state */
                        a2dpSetSuspendState(priority, a2dp_not_suspended);
                        break;
                    }
                }
#endif
                /* if not successful in resuming play via avrcp try a media start instead */  
                if(!status_avrcp)
                {
                    A2DP_DEBUG(("A2dp: Media Start\n" )) ;   
                    A2dpMediaStartRequest(theSink.a2dp_link_data->device_id[priority], theSink.a2dp_link_data->stream_id[priority]);
                    /* reset the SuspendState indicator */
                    a2dpSetSuspendState(priority, a2dp_not_suspended);
                }
            }
            /* media channel wasn't open, source not supporting suspend */            
            else if(state < a2dp_stream_open) 
            {
                A2DP_DEBUG(("A2dp: Media Open\n" )) ;   
                connectA2dpStream( priority, 0 );
            }
            /* recovery if media has resumed streaming reconnect its audio */
            else if(state == a2dp_stream_streaming)
            {
#ifdef ENABLE_AVRCP            
                /* does the device support AVRCP and is AVRCP currently connected to this device? */
                for_all_avrcp(i)
                {    
                    /* ensure media is streaming and the avrcp channel is that requested to be paused */
                    if ((theSink.avrcp_link_data->connected[i])&&(theSink.a2dp_link_data->connected[priority])&&
                        (BdaddrIsSame(&theSink.a2dp_link_data->bd_addr[priority], &theSink.avrcp_link_data->bd_addr[i])))
                    {
                        /* attempt to resume playing the a2dp stream */
                        status_avrcp = sinkAvrcpPlayPauseRequest(i,AVRCP_PLAY);
                        break;
                    }
                }
#endif
                a2dpSetSuspendState(priority, a2dp_not_suspended);
                ready_to_connect = TRUE;
            }
        }
        /* signalling channel is no longer present so attempt to reconnect it */
        else
        {
            A2DP_DEBUG(("A2dp: Connect Signalling\n" )) ;   
            A2dpSignallingConnectRequest(&theSink.a2dp_link_data->bd_addr[priority]);
        }
    }
    return ready_to_connect;
}

/*************************************************************************
NAME    
    handleA2DPStoreClockMismatchRate
    
DESCRIPTION
    handle storing the clock mismatch rate for the active stream
RETURNS
    
**************************************************************************/
void handleA2DPStoreClockMismatchRate(uint16 clockMismatchRate)   
{
    a2dp_stream_state a2dpStatePri = a2dp_stream_idle;
    a2dp_stream_state a2dpStateSec = a2dp_stream_idle;
    Sink a2dpSinkPri = 0;
    Sink a2dpSinkSec = 0;
        
    /* if a2dp connected obtain the current streaming state for primary a2dp connection */
    getA2dpStreamData(a2dp_primary, &a2dpSinkPri, &a2dpStatePri);

    /* if a2dp connected obtain the current streaming state for secondary a2dp connection */
    getA2dpStreamData(a2dp_secondary, &a2dpSinkSec, &a2dpStateSec);
 
    /* Determine which a2dp source this is for */
    if((a2dpStatePri == a2dp_stream_streaming) && (a2dpSinkPri == theSink.routed_audio))  
    {
        A2DP_DEBUG(("A2dp: store pri. clk mismatch = %x\n", clockMismatchRate));
        theSink.a2dp_link_data->clockMismatchRate[a2dp_primary] = clockMismatchRate;
    }
    else if((a2dpStateSec == a2dp_stream_streaming) && (a2dpSinkSec == theSink.routed_audio))  
    {
        A2DP_DEBUG(("A2dp: store sec. clk mismatch = %x\n", clockMismatchRate));
        theSink.a2dp_link_data->clockMismatchRate[a2dp_secondary] = clockMismatchRate;
    }
    else
    {
        A2DP_DEBUG(("A2dp: ERROR NO A2DP STREAM, clk mismatch = %x\n", clockMismatchRate));
    }
}
 

/*************************************************************************
NAME    
    handleA2DPStoreCurrentEqBank
    
DESCRIPTION
    handle storing the current EQ bank
RETURNS
    
**************************************************************************/
void handleA2DPStoreCurrentEqBank (uint16 currentEQ)   
{
    uint16 abs_eq = A2DP_MUSIC_PROCESSING_FULL_SET_EQ_BANK0 + currentEQ;
               
    A2DP_DEBUG(("A2dp: Current EQ = %x, store = %x\n", currentEQ, abs_eq));

    /* Make sure current EQ setting is no longer set to next and store it */
    if(theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_processing != abs_eq)
    {
        A2DP_DEBUG(("A2dp: Update Current EQ = %x\n", abs_eq));
        theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_processing = (A2DP_MUSIC_PROCESSING_T)abs_eq;
#ifdef ENABLE_PEER
        /* Send the audio enhancement settings to the peer(slave) if this device is a master*/
        peerSendAudioEnhancements();
#endif        
        configManagerWriteSessionData () ; 
    }
}

/*************************************************************************
NAME    
    handleA2DPStoreEnhancments
    
DESCRIPTION
    handle storing the current enhancements settings
RETURNS
    
**************************************************************************/
void handleA2DPStoreEnhancements(uint16 new_enhancements)
{
     uint16 old_enhancements = theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_enhancements;

    A2DP_DEBUG(("A2dp: store enhancements = %x was %x\n", new_enhancements, old_enhancements));

    /* Remove the MUSIC_CONFIG_DATA_VALID flag in the stored session data, before comparing with the DSP value. */
    old_enhancements &= (~MUSIC_CONFIG_DATA_VALID);

    /* Make sure current setting is no longer set to next and store it */
    if(old_enhancements != new_enhancements)
    {
        /*  Add the data valid flag, this signifies that the user has altered the 3d or bass boost enhancements,
         *  these values should now be used instead of dsp default values that have been created by the UFE.
         */
        new_enhancements |= MUSIC_CONFIG_DATA_VALID;

        A2DP_DEBUG(("A2dp: update enhancements = %x\n", new_enhancements));
        theSink.a2dp_link_data->a2dp_audio_mode_params.music_mode_enhancements = new_enhancements;
        configManagerWriteSessionData();
    }

#ifdef ENABLE_PEER
        /* Send the audio enhancement settings to the peer(slave) if this device is a master*/
        peerSendAudioEnhancements();
#endif

}

/*************************************************************************
 NAME    
    handleA2DPUserEqBankUpdate
    
DESCRIPTION
	Handle notification from an audio plugin for DSP ready for data message in order to update user defined EQ bank 
    when GAIA set EQ parameter commands are processed

RETURNS 
**************************************************************************/
void handleA2DPUserEqBankUpdate(void)
{
    if(theSink.PEQ && (theSink.PEQ->bands[0].Q != 0))
    {        
        /* User EQ settings message buffer and buffer index for copy operation */
        uint16* dspSetEqMessage;
        uint16 i = 0;
        
        /* User EQ always operates on DSP EQ Bank 1, hence the param ID base maps to GAIA command ID for Filter type of Band 1 in EQ Bank 1 */        
#define PARAM_ID_BASE 0x0110  
    /* Set the Kalimba long message size taking the first and last word into account plus 2 words per parameter to update*/
#define MESSAGE_SIZE  46
            
        dspSetEqMessage = (uint16*)callocDebugPanic(MESSAGE_SIZE,sizeof(uint16));

        /* First message element must contain the number of EQ parameters to update at all times */
        dspSetEqMessage[0]= 22; 
        
        /* Second message element contains parameter ID 0x0100 and the value which maps to the number of bands in user defined EQ Bank 1. 
        This data can be located anywhere in the message buffer, however, reading the GAIA message payload is easier this way
        while debugging */  
        dspSetEqMessage[1] = 0x0100;
        dspSetEqMessage[2] = 0x5;
        /* Third message element contains parameter ID 0x0101 and the value which maps to the master gain for user defined EQ Bank 1. 
        This data can be located anywhere in the message buffer, however, reading the GAIA message payload is easier this way
        while debugging */
        dspSetEqMessage[3] = 0x0101;
        dspSetEqMessage[4] = theSink.PEQ->preGain;            
         
        for(i=0; i<5; i++){
            dspSetEqMessage[(8*i)+5]= PARAM_ID_BASE + ((0x10)*i);
            dspSetEqMessage[(8*i)+6]= theSink.PEQ->bands[i].filter;
            dspSetEqMessage[(8*i)+7]= PARAM_ID_BASE + ((0x10)*i) +1;
            dspSetEqMessage[(8*i)+8]= theSink.PEQ->bands[i].freq;
            dspSetEqMessage[(8*i)+9]= PARAM_ID_BASE + ((0x10)*i) +2;
            dspSetEqMessage[(8*i)+10]= theSink.PEQ->bands[i].gain;
            dspSetEqMessage[(8*i)+11]= PARAM_ID_BASE + ((0x10)*i) +3;
            dspSetEqMessage[(8*i)+12]= theSink.PEQ->bands[i].Q;            
        }
        
        /* Parameter ID 0x0000 and the value which map to the number of active EQ banks, not for the time being but it may be needed in the future */
        /*KalimbaSendMessage(DSP_GAIA_MSG_SET_USER_PARAM, 0x0000, 6 , 0 , 0 );*/
        /*KalimbaSendMessage(DSP_GAIA_MSG_SET_USER_PARAM, 0x0101, theSink.PEQ->preGain, 0 , 0 );*/
        /*KalimbaSendMessage(DSP_GAIA_MSG_SET_USER_PARAM, 0x0100, 5 , 0 , 0 );*/
      
        /* Last message was intended for recalculation of filter coefficients, but is ignored in the existing GAIA DSP message handler implementation */
        dspSetEqMessage[45]= 0x0001;
        
        KalimbaSendLongMessage(DSP_GAIA_MSG_SET_USER_GROUP_PARAM,MESSAGE_SIZE,dspSetEqMessage);

        free(dspSetEqMessage);
    }
}

/*************************************************************************
NAME    
    getA2dpIndexFromBdaddr
    
DESCRIPTION
    Attempts to find a A2DP link data index based on the supplied bdaddr.
    
RETURNS
    TRUE if successful, FALSE otherwise
    
**************************************************************************/
bool getA2dpIndexFromBdaddr (const bdaddr *bd_addr, uint16 *index)
{
    /* go through A2dp connection looking for match */
    for_all_a2dp(*index)
    {
        /* if the a2dp link is connected check its bdaddr */
        if(theSink.a2dp_link_data /*&& theSink.a2dp_link_data->connected[*index]*/)
        {
            /* if a match is found indicate success */
            if(BdaddrIsSame(&theSink.a2dp_link_data->bd_addr[*index], bd_addr))
            {
                return TRUE;
            }
        }
    }
    /* no matches found so return not successful */    
    return FALSE;
}

/*************************************************************************
NAME    
    disconnectAllA2dpAVRCP
    
DESCRIPTION
    disconnect any a2dp and avrcp connections
    
RETURNS
    
**************************************************************************/
void disconnectAllA2dpAvrcp (bool disconnect_peer)
{
    uint8 i;

#ifdef ENABLE_AVRCP
    if(theSink.features.avrcp_enabled)    
    {
        sinkAvrcpDisconnectAll(disconnect_peer);
    }
#endif  
    if(theSink.a2dp_link_data)
    {
        /* disconnect any a2dp signalling channels */
        for_all_a2dp(i)
        {
            /* if the a2dp link is connected, disconnect it */
            if(theSink.a2dp_link_data->connected[i])
            {
                if (((theSink.a2dp_link_data->peer_device[i]!=remote_device_peer) || disconnect_peer))
                {
                    A2dpSignallingDisconnectRequest(theSink.a2dp_link_data->device_id[i]);
                }
            }
        }
    }  
}               

/*************************************************************************
NAME    
    disconnectA2dpAvrcpFromDevice
    
DESCRIPTION
    disconnect A2DP and AVRCP connections with the device provided.
    
RETURNS
    
**************************************************************************/
void disconnectA2dpAvrcpFromDevice(const bdaddr *bdaddr_non_gaia_device)
{
    uint16 index = 0;
    
#ifdef ENABLE_AVRCP
    if(theSink.features.avrcp_enabled)    
    {
        sinkAvrcpDisconnect(bdaddr_non_gaia_device);
    }
#endif

    if(getA2dpIndexFromBdaddr(bdaddr_non_gaia_device, &index))
    {
        A2dpSignallingDisconnectRequest(theSink.a2dp_link_data->device_id[index]);
    }
}


/*************************************************************************
NAME    
    a2dpGetNextAvBdAddress
    
DESCRIPTION
    Returns the BD address of the other connected non-peer device, if any.
    
RETURNS
    TRUE if successful, FALSE otherwise
**************************************************************************/
bool a2dpGetNextAvBdAddress(const bdaddr *bd_addr , bdaddr *next_bdaddr )
{
    uint16 index;
    
    if(getA2dpIndexFromBdaddr (bd_addr, &index))
    {
        a2dp_link_priority other_device_link_priority = 
            !BdaddrIsSame(&theSink.a2dp_link_data->bd_addr[a2dp_primary],
            &theSink.a2dp_link_data->bd_addr[index]) ? a2dp_primary : a2dp_secondary;

        if(next_bdaddr && theSink.a2dp_link_data->connected[other_device_link_priority] &&
           (theSink.a2dp_link_data->peer_device[other_device_link_priority] != remote_device_peer))
        {
            *next_bdaddr = theSink.a2dp_link_data->bd_addr[other_device_link_priority];
            if(!BdaddrIsSame(bd_addr , next_bdaddr))
            {
                return TRUE;
            }
        }
    }    
    return FALSE;
}


/*************************************************************************
NAME    
    disconnectAllA2dpPeerDevices
    
DESCRIPTION
    Disconnect any a2dp connections to any peer devices
    
RETURNS
    TRUE is any peer devices disconnected, FALSE otherwise
    
**************************************************************************/
bool disconnectAllA2dpPeerDevices (void)
{
    uint8 i;
    bool disc_req = FALSE;

    if(theSink.a2dp_link_data)
    {
        /* disconnect any a2dp signalling channels to peer devices */
        for_all_a2dp(i)
        {
            /* if the a2dp link is connected, disconnect it */
            if ((theSink.a2dp_link_data->connected[i]) && (theSink.a2dp_link_data->peer_device[i]==remote_device_peer))
            {
                A2dpSignallingDisconnectRequest(theSink.a2dp_link_data->device_id[i]);
                disc_req = TRUE;
            }
        }
    }  
    
    return disc_req;
}


/*************************************************************************
NAME    
    handleA2DPSyncDelayInd
    
DESCRIPTION
	Handle request from A2DP library for a Sink device to supply an initial
	Synchronisation Delay (audio latency) report.

RETURNS
    
**************************************************************************/
void handleA2DPSyncDelayInd (uint8 device_id, uint8 seid)
{
	Task audio_plugin;
	
	if ((audio_plugin = getA2dpPlugin(seid)) != NULL)
	{
		uint16 index;
		uint16 latency;
        bool estimated;
		
		if ( AudioGetLatency(audio_plugin, &estimated, &latency) && getA2dpIndex(device_id, &index) )
		{
			A2dpMediaAvSyncDelayResponse(theSink.a2dp_link_data->device_id[index], theSink.a2dp_link_data->seid[index], latency);
		}
	}
}

/*************************************************************************
NAME    
    handleA2DPLatencyReport
    
DESCRIPTION
	Handle notification from an audio plugin raised due to the DSP providing
	a measured audio latency value.

RETURNS
    
**************************************************************************/
void handleA2DPLatencyReport (Task audio_plugin, bool estimated, uint16 latency)
{
	uint16 index;
	
	if (getA2dpIndexFromPlugin(audio_plugin, &index))
	{
		theSink.a2dp_link_data->latency[index] = latency;
		
		if (estimated)
		{
			A2dpMediaAvSyncDelayResponse(theSink.a2dp_link_data->device_id[index], theSink.a2dp_link_data->seid[index], latency);
		}
		else
		{
			A2dpMediaAvSyncDelayRequest(theSink.a2dp_link_data->device_id[index], theSink.a2dp_link_data->seid[index], latency);
		}
	}
}


/*************************************************************************
NAME    
    handleA2DPMessage
    
DESCRIPTION
    A2DP message Handler, this function handles all messages returned
    from the A2DP library and calls the relevant functions if required

RETURNS
    
**************************************************************************/
void handleA2DPMessage( Task task, MessageId id, Message message )
{
    UNUSED(task);

    switch (id)
    {
/******************/
/* INITIALISATION */
/******************/
        
        /* confirmation of the initialisation of the A2DP library */
        case A2DP_INIT_CFM:
            A2DP_DEBUG(("A2DP_INIT_CFM : \n"));
            sinkA2dpInitComplete((const A2DP_INIT_CFM_T *) message);
        break;

/*****************************/        
/* SIGNALING CHANNEL CONTROL */
/*****************************/

        /* indication of a remote source trying to make a signalling connection */		
	    case A2DP_SIGNALLING_CONNECT_IND:
	        A2DP_DEBUG(("A2DP_SIGNALLING_CHANNEL_CONNECT_IND : \n"));
            handleA2DPSignallingConnectInd( ((const A2DP_SIGNALLING_CONNECT_IND_T *)message)->device_id,
                                            ((const A2DP_SIGNALLING_CONNECT_IND_T *)message)->addr );
		break;

        /* confirmation of a signalling connection attempt, successful or not */
	    case A2DP_SIGNALLING_CONNECT_CFM:
            A2DP_DEBUG(("A2DP_SIGNALLING_CHANNEL_CONNECT_CFM : \n"));
            handleA2DPSignallingConnected(((const A2DP_SIGNALLING_CONNECT_CFM_T*)message)->status, 
                                          ((const A2DP_SIGNALLING_CONNECT_CFM_T*)message)->device_id, 
                                          ((const A2DP_SIGNALLING_CONNECT_CFM_T*)message)->addr,
                                          ((const A2DP_SIGNALLING_CONNECT_CFM_T*)message)->locally_initiated);
	    break;
        
        /* indication of a signalling channel disconnection having occured */
    	case A2DP_SIGNALLING_DISCONNECT_IND:
            A2DP_DEBUG(("A2DP_SIGNALLING_CHANNEL_DISCONNECT_IND : \n"));
            handleA2DPSignallingDisconnected(((const A2DP_SIGNALLING_DISCONNECT_IND_T*)message)->device_id,
                                             ((const A2DP_SIGNALLING_DISCONNECT_IND_T*)message)->status,
                                             ((const A2DP_SIGNALLING_DISCONNECT_IND_T*)message)->addr);
		break;
        
/*************************/        
/* MEDIA CHANNEL CONTROL */        
/*************************/
        
        /* indication of a remote device attempting to open a media channel */      
        case A2DP_MEDIA_OPEN_IND:
            A2DP_DEBUG(("A2DP_OPEN_IND : \n"));
            handleA2DPOpenInd(((const A2DP_MEDIA_OPEN_IND_T*)message)->device_id,
                              ((const A2DP_MEDIA_OPEN_IND_T*)message)->seid);
        break;
		
        /* confirmation of request to open a media channel */
        case A2DP_MEDIA_OPEN_CFM:
            A2DP_DEBUG(("A2DP_OPEN_CFM : \n"));
            handleA2DPOpenCfm(((const A2DP_MEDIA_OPEN_CFM_T*)message)->device_id, 
                              ((const A2DP_MEDIA_OPEN_CFM_T*)message)->stream_id, 
                              ((const A2DP_MEDIA_OPEN_CFM_T*)message)->seid, 
                              ((const A2DP_MEDIA_OPEN_CFM_T*)message)->status);
        break;
        	
        /* Stream has been reconfigured */
        case A2DP_MEDIA_RECONFIGURE_CFM:
            A2DP_DEBUG(("A2DP_MEDIA_RECONFIGURE_CFM : \n"));
#ifdef ENABLE_PEER
            handlePeerQualificationReconfigureCfm(((const A2DP_MEDIA_RECONFIGURE_CFM_T*)message)->device_id,
                              ((const A2DP_MEDIA_RECONFIGURE_CFM_T*)message)->stream_id,
                              ((const A2DP_MEDIA_RECONFIGURE_CFM_T*)message)->status);
#endif
            break;

        /* indication of a request to close the media channel, remotely generated */
        case A2DP_MEDIA_CLOSE_IND:
            A2DP_DEBUG(("A2DP_CLOSE_IND : \n"));
            handleA2DPClose(((const A2DP_MEDIA_CLOSE_IND_T*)message)->device_id,
                            ((const A2DP_MEDIA_CLOSE_IND_T*)message)->stream_id,
                            ((const A2DP_MEDIA_CLOSE_IND_T*)message)->status);
        break;

        /* confirmation of the close of the media channel, locally generated  */
        case A2DP_MEDIA_CLOSE_CFM:
           A2DP_DEBUG(("A2DP_CLOSE_CFM : \n"));
           handleA2DPClose(0,0,a2dp_success);
        break;

/**********************/          
/*  STREAMING CONTROL */
/**********************/          
        
        /* indication of start of media streaming from remote source */
        case A2DP_MEDIA_START_IND:
            A2DP_DEBUG(("A2DP_START_IND : \n"));
            handleA2DPStartInd(((const A2DP_MEDIA_START_IND_T*)message)->device_id,
                               ((const A2DP_MEDIA_START_IND_T*)message)->stream_id);
        break;
		
        /* confirmation of a local request to start media streaming */
        case A2DP_MEDIA_START_CFM:
            A2DP_DEBUG(("A2DP_START_CFM : \n"));
            handleA2DPStartStreaming(((const A2DP_MEDIA_START_CFM_T*)message)->device_id,
                                     ((const A2DP_MEDIA_START_CFM_T*)message)->stream_id,
                                     ((const A2DP_MEDIA_START_CFM_T*)message)->status);
        break;
        
        case A2DP_MEDIA_SUSPEND_IND:
            A2DP_DEBUG(("A2DP_SUSPEND_IND : \n"));
            handleA2DPSuspendStreaming(((const A2DP_MEDIA_SUSPEND_IND_T*)message)->device_id,
                                       ((const A2DP_MEDIA_SUSPEND_IND_T*)message)->stream_id,
                                         a2dp_success);
        break;
		
        case A2DP_MEDIA_SUSPEND_CFM:
            A2DP_DEBUG(("A2DP_SUSPEND_CFM : \n"));
            handleA2DPSuspendStreaming(((const A2DP_MEDIA_SUSPEND_CFM_T*)message)->device_id,
                                       ((const A2DP_MEDIA_SUSPEND_CFM_T*)message)->stream_id,
                                       ((const A2DP_MEDIA_SUSPEND_CFM_T*)message)->status);
        break;

/*************************/
/* MISC CONTROL MESSAGES */
/*************************/
        
        case A2DP_MEDIA_AV_SYNC_DELAY_UPDATED_IND:
            A2DP_DEBUG(("A2DP_MEDIA_AV_SYNC_DELAY_UPDATED_IND : seid=0x%X delay=%u\n", 
                                ((const A2DP_MEDIA_AV_SYNC_DELAY_UPDATED_IND_T*)message)->seid, 
                                ((const A2DP_MEDIA_AV_SYNC_DELAY_UPDATED_IND_T*)message)->delay));
             /* Only received for source SEIDs.  Use delay value to aid AV synchronisation */                                                                        
        break;

        case A2DP_MEDIA_AV_SYNC_DELAY_IND:
            A2DP_DEBUG(("A2DP_MEDIA_AV_SYNC_DELAY_IND : seid=0x%X\n",
                                ((const A2DP_MEDIA_AV_SYNC_DELAY_IND_T*)message)->seid));
            handleA2DPSyncDelayInd(((const A2DP_MEDIA_AV_SYNC_DELAY_IND_T*)message)->device_id,
                                   ((const A2DP_MEDIA_AV_SYNC_DELAY_IND_T*)message)->seid);
        break;
        
        case A2DP_MEDIA_AV_SYNC_DELAY_CFM:
            A2DP_DEBUG(("A2DP_MEDIA_AV_SYNC_DELAY_CFM : \n"));
        break;
        
        /* link loss indication */
        case A2DP_SIGNALLING_LINKLOSS_IND:
            A2DP_DEBUG(("A2DP_SIGNALLING_LINKLOSS_IND : \n"));
            handleA2DPSignallingLinkloss(((const A2DP_SIGNALLING_LINKLOSS_IND_T*)message)->device_id);
        break;           
		
        case A2DP_CODEC_CONFIGURE_IND:
            A2DP_DEBUG(("A2DP_CODEC_CONFIGURE_IND : \n"));
#ifdef ENABLE_PEER
            handleA2dpCodecConfigureIndFromPeer((A2DP_CODEC_CONFIGURE_IND_T *)message);
#endif
        break;
            
	    case A2DP_ENCRYPTION_CHANGE_IND:
            A2DP_DEBUG(("A2DP_ENCRYPTION_CHANGE_IND : \n"));
		break;

        case A2DP_LINKLOSS_RECONNECT_CANCEL_IND:
            A2DP_DEBUG(("A2DP_LINKLOSS_RECONNECT_CANCEL_IND : \n"));
            handleA2DPLinklossReconnectCancel(((const A2DP_LINKLOSS_RECONNECT_CANCEL_IND_T*)message)->device_id);
        break;
			
        default:       
	    	A2DP_DEBUG(("A2DP UNHANDLED MSG: 0x%x\n",id));
        break;
    }    
}



#ifdef ENABLE_SUBWOOFER
/*************************************************************************
NAME    
    suspendWhenSubwooferStreamingLowLatency
    
DESCRIPTION
    funnction to determine if a2dp stream should be suspended due to the use
    of the low latency subwoofer link. Streaming a2dp media will adversely affect
    the quality of the sub low latency link due to bluetooth link bandwidth 
    limitations

RETURNS
    none
    
**************************************************************************/
void suspendWhenSubwooferStreamingLowLatency(uint16 Id)
{
    /* check whether this a2dp link got routed */
    if(theSink.rundata->routed_audio_source != audio_source_AG1 + Id)
    {
        /* this a2dp source is not currently routed, check for presence of subwoofer */                
        if (SwatGetMediaLLState(theSink.rundata->subwoofer.dev_id) == swat_media_streaming)
        {
            /* sub woofer is currently streaming and using an esco connection
               it is necessary to suspend this a2dp stream to prevent sub stream dissruption */
            audioSuspendDisconnectAllA2dpMedia();
        }              
    }
}
#endif

bool a2dpAudioSinkMatch(a2dp_link_priority a2dp_link, Sink sink)
{
    uint8 device_id = theSink.a2dp_link_data->device_id[a2dp_link];
    uint8 stream_id = theSink.a2dp_link_data->stream_id[a2dp_link];

    if(theSink.a2dp_link_data->connected[a2dp_link])
        if(A2dpMediaGetSink(device_id, stream_id) == sink)
            return TRUE;
    return FALSE;
}

bool a2dpA2dpAudioIsRouted(void)
{
    uint8 index;

    for(index = a2dp_primary; index < (a2dp_secondary+1); index++)
    {
        /* is a2dp connected? */
        if(theSink.a2dp_link_data->connected[index])
        {
            /* check whether the a2dp connection is present and streaming data and that the audio is routed */
            if(theSink.routed_audio && (theSink.routed_audio == A2dpMediaGetSink(theSink.a2dp_link_data->device_id[index], theSink.a2dp_link_data->stream_id[index])))
            {
                return TRUE;
            }
        }
    }
    return FALSE;
}

a2dp_link_priority a2dpGetRoutedInstanceIndex(void)
{
    a2dp_link_priority index;

    for(index = a2dp_primary; index < (a2dp_secondary+1); index++)
    {
        /* is a2dp connected? */
        if(theSink.a2dp_link_data->connected[index])
        {
            /* check whether the a2dp connection is present and streaming data and that the audio is routed */
            if(theSink.routed_audio && (theSink.routed_audio == A2dpMediaGetSink(theSink.a2dp_link_data->device_id[index], theSink.a2dp_link_data->stream_id[index])))
            {
                return index;
            }
        }
    }
    return a2dp_invalid;
}
#ifdef ENABLE_PEER
/*************************************************************************
NAME    
    HandlePeerRemoveAuthDevice
    
DESCRIPTION
    Delete the link key, if there is no peer device connected,
    or another peer device connected

RETURNS
    none
    
**************************************************************************/
void HandlePeerRemoveAuthDevice(const bdaddr* message)
{
        uint16 peerIndex = 0;         
        bdaddr addr = *message;
        A2DP_DEBUG(("EventSysRemovePeerTempPairing\n")); 
        /* delete the link key, if there is no peer device connected, or another peer device connected */
        if(!(a2dpGetPeerIndex(&peerIndex)) || !(BdaddrIsSame(&theSink.a2dp_link_data->bd_addr[peerIndex], &addr)))
        {
          ConnectionSmDeleteAuthDevice(&addr);
        }
}
#endif

#ifdef ENABLE_PEER
/*************************************************************************
NAME    
    a2dpStartPauseSuspendTimeout

DESCRIPTION
    Start a timeout after which we will ask the remote device to suspend
    its a2dp media channel.

    We don't send the suspend immediately because not all AG's behave
    well in that situation. Instead we delay it to give the AG time
    to send its own suspend.

RETURNS
    none

**************************************************************************/
void a2dpStartPauseSuspendTimeout(a2dp_link_priority priority)
{
    a2dpCancelPauseSuspendTimeout(priority);

    A2DP_DEBUG(("A2DP: a2dpStartPauseSuspendTimeout idx %u\n", priority));

    MessageSendLater(&theSink.task, (EventSysA2dpPauseSuspendTimeoutDevice1 + priority), 0, A2DP_PAUSE_SUSPEND_TIMER);
}

/*************************************************************************
NAME    
    a2dpCancelPauseSuspendTimeout

DESCRIPTION
    Cancel any outstanding suspend timeout for the given a2dp device.

RETURNS
    none

**************************************************************************/
void a2dpCancelPauseSuspendTimeout(a2dp_link_priority priority)
{
    A2DP_DEBUG(("A2DP: a2dpCancelPauseSuspendTimeout idx %u\n", priority));
    MessageCancelAll(&theSink.task, EventSysA2dpPauseSuspendTimeoutDevice1 + priority);
}
#endif

/*************************************************************************
NAME
    sinkA2dpGetA2dpVolumeFromBdaddr

DESCRIPTION
    Retrieve the A2DP volume for the connection to the device with the address specified.

RETURNS
    Returns TRUE if the volume was retrieved, FALSE otherwise.
    The actual volume is returned in the a2dp_volume variable.

**************************************************************************/
bool sinkA2dpGetA2dpVolumeFromBdaddr(const bdaddr *bd_addr, uint16 * const a2dp_volume)
{
    uint8 i;

    /* go through A2dp connection looking for match */
    for_all_a2dp(i)
    {
        /* if the a2dp link is connected check its bdaddr */
        if(theSink.a2dp_link_data->connected[i])
        {
            /* if a match is found return its volume level and a
               status of successful match found */
            if(BdaddrIsSame(&theSink.a2dp_link_data->bd_addr[i], bd_addr))
            {
                *a2dp_volume = theSink.volume_levels.a2dp_volume[i].main_volume;
                A2DP_DEBUG(("A2DP: getVolume = %d\n", i));
                return TRUE;
            }
        }
    }
    /* no matches found so return not successful */
    return FALSE;
}

/*************************************************************************
NAME
    sinkA2dpSetA2dpVolumeFromIndex

DESCRIPTION
    Sets the A2DP volume for the connection at the specified index.

**************************************************************************/
void sinkA2dpSetA2dpVolumeAtIndex(uint8 a2dp_index, uint16 a2dp_volume)
{
    theSink.volume_levels.a2dp_volume[a2dp_index].main_volume = a2dp_volume;
}

/*************************************************************************
NAME
    sinkA2dpGetA2dpVolumeInfoAtIndex

DESCRIPTION
    Gets the volume_info structure of the connection at the specified index.

**************************************************************************/
volume_info * sinkA2dpGetA2dpVolumeInfoAtIndex(uint8 a2dp_index)
{
    return &theSink.volume_levels.a2dp_volume[a2dp_index];
}

/*************************************************************************
NAME
    sinkA2dpAudioPrimaryOrSecondarySinkMatch

DESCRIPTION
    Checks the status of the two A2DP streams.

RETURNS
	TRUE if one of them is connected, otherwise FALSE.

**************************************************************************/
bool sinkA2dpAudioPrimaryOrSecondarySinkMatch(Sink sink)
{
    return (a2dpAudioSinkMatch(a2dp_primary, sink) ||
                a2dpAudioSinkMatch(a2dp_secondary, sink));
}

a2dp_stream_state sinkA2dpGetStreamState(a2dp_link_priority priority)
{
    if(theSink.a2dp_link_data)
    {
        if(theSink.a2dp_link_data->connected[priority])
        {
            return A2dpMediaGetState(theSink.a2dp_link_data->device_id[priority], theSink.a2dp_link_data->stream_id[priority]);
        }
    }
    return a2dp_stream_idle;
}

a2dp_role_type sinkA2dpGetRoleType(a2dp_link_priority priority)
{
    if(theSink.a2dp_link_data)
    {
        if(theSink.a2dp_link_data->connected[priority])
        {
            return A2dpMediaGetRole(theSink.a2dp_link_data->device_id[priority], theSink.a2dp_link_data->stream_id[priority]) ;
        }
    }
    return a2dp_role_undefined;
}

Sink sinkA2dpGetAudioSink(a2dp_link_priority a2dp_link)
{
    uint8 device_id = theSink.a2dp_link_data->device_id[a2dp_link];
    uint8 stream_id = theSink.a2dp_link_data->stream_id[a2dp_link];

    if(theSink.a2dp_link_data->connected[a2dp_link])
    {
        return A2dpMediaGetSink(device_id, stream_id);
    }
    return NULL;
}
