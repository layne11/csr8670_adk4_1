/****************************************************************************
Copyright (c) 2005 - 2016 Qualcomm Technologies International, Ltd.

FILE NAME
    csr_a2dp_decoder_common_sharing.c

DESCRIPTION
    Functionality to share audio with a second device.
*/

#include <audio.h>
#include <stdlib.h>
#include <panic.h>
#include <print.h>
#include <file.h>
#include <stream.h> /*for the ringtone_note*/
#include <kalimba.h>
#include <kalimba_standard_messages.h>
#include <message.h>
#include <ps.h>
#include <transform.h>
#include <string.h>
#include <pio_common.h>
#include <vmal.h>

#include "csr_i2s_audio_plugin.h"
#include "audio_plugin_if.h" /*for the audio_mode*/
#include "audio_plugin_common.h"
#include "csr_a2dp_decoder_common.h"
#include "csr_a2dp_decoder_common_if.h"
#include "csr_a2dp_decoder_common_sharing.h"
#include "csr_a2dp_decoder_if.h"


/* 
    Use a fixed packet size when using the RTP transform to encode the ShareMe data.
    It would be better if this packet size was passed into the CsrA2dpDecoderPluginForwardUndecoded() function,
    but the A2DP MTU is set to be this value for the Master to Slave ShareMe link.
*/
#define SHAREME_ENCODE_FIXED_PACKET_SIZE    672

/****************************************************************************
DESCRIPTION
    disconnect the forwarding sink
*/
void CsrA2dpDecoderPluginDisconnectForwardingSink(void)
{        
    DECODER_t * DECODER = CsrA2dpDecoderGetDecoderData();  

	csrA2dpDecoderSetStreamRelayMode(RELAY_MODE_NONE);
	
    /* if connected, disconnect forwarding sink */
    if(DECODER->forwarding_sink)
    {
        Transform transform = TransformFromSink( DECODER->forwarding_sink );

		if (transform)
		{
			(void)TransformStop( transform );
			TransformDisconnect( transform );   /* TODO: Are both needed? */
		}
		
        StreamDisconnect( 0, DECODER->forwarding_sink);
		DECODER->forwarding_sink = NULL;
    }
}


/****************************************************************************
DESCRIPTION
    Control fowarding of undecoded audio frames to another device.
*/
void CsrA2dpDecoderPluginForwardUndecoded(A2dpPluginTaskdata *task , bool enable, Sink sink, bool content_protection, peer_buffer_level buffer_level_required)
{
    DECODER_t * DECODER = CsrA2dpDecoderGetDecoderData();
    
    /* ensure dsp is loaded */
    if (DECODER)
    {
        PRINT(("DECODER: Forward Undecoded (%u)\n" , enable));
        /* check whether forwarding is enabled or not */
        if (enable)
        {
            PRINT(("DECODER: sink=0x%p forwarding_sink=0x%p sink_type=%u\n",
                    (void*)sink,
                    (void*)DECODER->forwarding_sink,
                    DECODER->sink_type));

            /* ensure sink is valid */            
            if ((sink != (Sink)NULL) && (DECODER->forwarding_sink == (Sink)NULL) &&
                ((DECODER->sink_type == AUDIO_SINK_AV) || (DECODER->sink_type == AUDIO_SINK_USB) || 
                (DECODER->sink_type == AUDIO_SINK_ANALOG) || (DECODER->sink_type == AUDIO_SINK_SPDIF) ||
                (DECODER->sink_type == AUDIO_SINK_I2S)))
            {
                DECODER->forwarding_sink = sink;

                /* determine codec type and start the data forwarding of the appropriate data type */
                switch ((A2DP_DECODER_PLUGIN_TYPE_T)task->a2dp_plugin_variant)
                {
                    case SBC_DECODER:
                    {
#ifdef SHAREME_MASTER
						Transform rtp_encode = 0;
						rtp_encode = rtp_encode;
                        StreamConnect(audioDecoderGetDspSource(dsp_source_forwarding),DECODER->forwarding_sink);
#else
                        Transform rtp_encode = VmalTransformRtpSbcEncode(audioDecoderGetDspSource(dsp_source_forwarding),DECODER->forwarding_sink);   /* Initialise the RTP SBC encoder */
                        TransformConfigure(rtp_encode, VM_TRANSFORM_RTP_ENCODE_PACKET_SIZE, SHAREME_ENCODE_FIXED_PACKET_SIZE);   /* Configure the RTP transform to generate the selected packet size */
                        TransformConfigure(rtp_encode, VM_TRANSFORM_RTP_ENCODE_MANAGE_TIMING, (uint16)FALSE);   /* Transform should not manage timings. */
                        TransformConfigure(rtp_encode, VM_TRANSFORM_RTP_SCMS_ENABLE, (uint16)content_protection);   /* Ensure transform supports content protection if reqd. */
                        (void)TransformStart( rtp_encode ) ;
#endif
						csrA2dpDecoderSetStreamRelayMode(RELAY_MODE_SHAREME);
                        PRINT(("DECODER: SBC forward scms=%u transform=0x%p\n",content_protection,(void*)rtp_encode));
                        break;
                    }
                    case MP3_DECODER:
                    {
#ifdef SHAREME_MASTER
						Transform rtp_encode = 0;
						rtp_encode = rtp_encode;
                        StreamConnect(audioDecoderGetDspSource(dsp_source_forwarding),DECODER->forwarding_sink);
#else
                        Transform rtp_encode = VmalTransformRtpMp3Encode(audioDecoderGetDspSource(dsp_source_forwarding),DECODER->forwarding_sink);   /* Initialise the RTP SBC encoder */
                        TransformConfigure(rtp_encode, VM_TRANSFORM_RTP_ENCODE_PACKET_SIZE, SHAREME_ENCODE_FIXED_PACKET_SIZE);   /* Configure the RTP transform to generate the selected packet size */
                        TransformConfigure(rtp_encode, VM_TRANSFORM_RTP_ENCODE_MANAGE_TIMING, (uint16)FALSE);   /* Transform should not manage timings. */
                        TransformConfigure(rtp_encode, VM_TRANSFORM_RTP_SCMS_ENABLE, (uint16)content_protection);   /* Ensure transform supports content protection if reqd. */
                        (void)TransformStart( rtp_encode ) ;
#endif
 						csrA2dpDecoderSetStreamRelayMode(RELAY_MODE_SHAREME);
                        PRINT(("DECODER: MP3 forward scms=%u transform=0x%p\n",content_protection,(void*)rtp_encode));
                        break;
                    }
                    case AAC_DECODER:
                    {
#ifdef SHAREME_MASTER
                        Transform rtp_encode = 0;
                        rtp_encode = rtp_encode;
                        StreamConnect(audioDecoderGetDspSource(dsp_source_forwarding),DECODER->forwarding_sink);
                        PRINT(("DECODER: AAC forward scms=%u transform=0x%p\n", content_protection, (void*)rtp_encode));
#else
                        /* Not currently supported */
                        PRINT(("DECODER: AAC forward not supported\n"));
#endif
                        break;
                    }
                    case FASTSTREAM_SINK:
                        /* Not currently supported */
                        PRINT(("DECODER: FASTSTREAM forward not supported\n"));
                        break;
                    case APTX_DECODER:
                        if (content_protection)
                        {
#ifdef SHAREME_MASTER
                            Transform rtp_encode = 0;
                            rtp_encode = rtp_encode;
                            StreamConnect(audioDecoderGetDspSource(dsp_source_forwarding),DECODER->forwarding_sink);
#else
                            Transform rtp_encode = TransformRtpEncode(audioDecoderGetDspSource(dsp_source_forwarding),DECODER->forwarding_sink);
                            TransformConfigure(rtp_encode, VM_TRANSFORM_RTP_ENCODE_PACKET_SIZE, SHAREME_ENCODE_FIXED_PACKET_SIZE);   /* Configure the RTP transform to generate the selected packet size */
                            TransformConfigure(rtp_encode, VM_TRANSFORM_RTP_ENCODE_MANAGE_TIMING, (uint16)FALSE);    /* Transform should not manage timings. */
                            TransformConfigure(rtp_encode, VM_TRANSFORM_RTP_SCMS_ENABLE, (uint16)content_protection);   /* Ensure transform supports content protection if reqd. */
                            (void)TransformStart( rtp_encode ) ;
#endif
							csrA2dpDecoderSetStreamRelayMode(RELAY_MODE_SHAREME);
                            PRINT(("DECODER: APTX forward scms=%u transform=0x%p\n", content_protection, (void*)rtp_encode));
                        }
                        else
                        {
                            StreamConnect(audioDecoderGetDspSource(dsp_source_forwarding),DECODER->forwarding_sink);
							csrA2dpDecoderSetStreamRelayMode(RELAY_MODE_SHAREME);
                            PRINT(("DECODER: APTX forward scms=%u transform=none\n",content_protection));
                        }
                        break;
                    case APTX_ACL_SPRINT_DECODER:
                        /* Not currently supported */
                        PRINT(("DECODER: APTX-LL forward not supported\n"));
                        break;
                    case APTXHD_DECODER:
                        /* Not currently supported */
                        PRINT(("DECODER: aptX-HD forward not supported\n"));
                        break;
					case TWS_SBC_DECODER:
                        A2DPConnectAndConfigureTWSAudio(buffer_level_required);
						PRINT(("DECODER: TWS_SBC packet_size=%u scms=%u transform=none\n",
                                DECODER->packet_size,
                                content_protection));
						break;
					case TWS_MP3_DECODER:
                        A2DPConnectAndConfigureTWSAudio(buffer_level_required);
						PRINT(("DECODER: TWS_MP3 packet_size=%u scms=%u transform=none\n",
                                DECODER->packet_size,
                                content_protection));
						break;
					case TWS_AAC_DECODER:
                        A2DPConnectAndConfigureTWSAudio(buffer_level_required);
						PRINT(("DECODER: TWS_AAC packet_size=%u scms=%u transform=none\n",
                                DECODER->packet_size,
                                content_protection));
						break;
					case TWS_APTX_DECODER:
                        A2DPConnectAndConfigureTWSAudio(buffer_level_required);
						PRINT(("DECODER: TWS_APTX packet_size=%u scms=%u transform=none\n",
                                DECODER->packet_size,
                                content_protection));
						break;
					case USB_DECODER:
                    case ANALOG_DECODER:
                    case SPDIF_DECODER:
                    case FM_DECODER:
                    case I2S_DECODER:
                    case NUM_DECODER_PLUGINS:
                    default:
                        PRINT(("DECODER: UNKNOWN\n"));
                        break;
                    }
                 }
       }
       /* forwarding no longer enabled */       
       else
       {
			csrA2dpDecoderSetStreamRelayMode(RELAY_MODE_NONE);
			
			/* if forwarding connected then disconnect it */
			if (DECODER->forwarding_sink)
			{
				Transform transform = TransformFromSink( DECODER->forwarding_sink );
				if (transform)
				{
					(void)TransformStop( transform );
					TransformDisconnect( transform );   /* TODO: Are both needed? */
				}
    
				StreamDisconnect( 0, DECODER->forwarding_sink);
    
				PRINT(("DECODER: Disabling forwarding_sink=0x%p transform=0x%p\n",
                       (void*)DECODER->forwarding_sink,
                        (void*)transform));
				DECODER->forwarding_sink = NULL;
            }
        }
    }
}

/****************************************************************************
DESCRIPTION
    Helper function to connect, route and configure TWS audio relay in the DSP
*/
void A2DPConnectAndConfigureTWSAudio(peer_buffer_level buffer_level_required)
{
    DECODER_t * DECODER = CsrA2dpDecoderGetDecoderData();

    if(DECODER)
    {
        /* configure the required internal DSP buffering delay */
        audioDecodeDspSendTwsCompatibilityMode(buffer_level_required);
        /* connect relay channel */
        StreamConnect(audioDecoderGetDspSource(dsp_source_forwarding),DECODER->forwarding_sink);
        /* change operating mode to relay */
        csrA2dpDecoderSetStreamRelayMode(RELAY_MODE_TWS_MASTER);
        /* update TWS audio routing */        
        csrA2dpDecoderSetTwsRoutingMode(DECODER->master_routing_mode, DECODER->slave_routing_mode);
    }
}
