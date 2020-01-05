/*****************************************************************
Copyright (c) 2011 - 2015 Qualcomm Technologies International, Ltd.
 */

#include "gaia.h"
#include "gaia_private.h"
#include "gaia_transport.h"
#include "gaia_transport_common.h"
#include "upgrade.h"
#include <stream.h>
#include <audio.h>
#include <source.h>
#include <sink.h>
#include <uart.h>

#ifdef GAIA_TRANSPORT_RFCOMM
#include "gaia_transport_rfcomm.h"
#endif

#ifdef GAIA_TRANSPORT_SPP
#include "gaia_transport_spp.h"
#endif

#ifdef GAIA_TRANSPORT_GATT
#include "gaia_transport_gatt.h"
#endif

typedef struct {
	Source source;
	uint16 idx;
	uint16 expected;
	uint16 packet_length;
	uint16 data_length;
	uint8 *data;
	uint8 *packet;
	uint8 flags;
	uint8 check;
} gaia_transport_process_source_data_t;


/*! @brief Clear down state of given transport.
 */
static void gaiaTransportDropState(gaia_transport *transport);

/*! @brief Notify clients of disconnection.
 */
static void gaiaTransportNotifyClientsOfDisconnection(gaia_transport *transport);

/*! @brief Attempt to find a free transport slot
 */
gaia_transport *gaiaTransportFindFree(void) {
	uint16 idx;

	for (idx = 0; idx < gaia->transport_count; ++idx) {
		if (gaia->transport[idx].type == gaia_transport_none)
			return &gaia->transport[idx];
	}

	return NULL;
}

#if defined GAIA_TRANSPORT_RFCOMM || defined GAIA_TRANSPORT_SPP
/*! @brief Attempt to find the tranport associated with an RFCOMM channel
 */
gaia_transport *gaiaTransportFromRfcommChannel(uint16 channel) {
	uint16 idx;

	for (idx = 0; idx < gaia->transport_count; ++idx) {
		if (((gaia->transport[idx].type == gaia_transport_rfcomm)
				|| (gaia->transport[idx].type == gaia_transport_spp))
				&& (gaia->transport[idx].state.spp.rfcomm_channel == channel))
			return &gaia->transport[idx];
	}
	return NULL;
}
#endif /* def GAIA_TRANSPORT_SPP */

/*! @brief Attempt to find the tranport associated with a sink
 */
gaia_transport *gaiaTransportFromSink(Sink sink) {
	uint16 idx;

	for (idx = 0; idx < gaia->transport_count; ++idx)
		if (gaiaTransportGetSink(&gaia->transport[idx]) == sink)
			return &gaia->transport[idx];

	return NULL;
}

#ifdef GAIA_TRANSPORT_GATT
/*! @brief Attempt to find the tranport associated with a GATT CID
 */
gaia_transport *gaiaTransportFromCid(uint16 cid) {
	uint16 idx;

	for (idx = 0; idx < gaia->transport_count; ++idx) {
		if (gaia->transport[idx].type == gaia_transport_gatt
				&& gaia->transport[idx].state.gatt.cid == cid) {
			return &gaia->transport[idx];
		}
	}
	return NULL;
}
#endif /* GAIA_TRANSPORT_GATT */

/*! @brief Attempt to connect Gaia to a device over SPP transport.
 */
void gaiaTransportConnectReq(gaia_transport *transport, bdaddr *address)
{
#if defined GAIA_TRANSPORT_SPP && defined GAIA_HAVE_SPP_CLIENT
    if (transport == NULL)
    {
        transport = gaiaTransportFindFree();
        if (transport != NULL)
        {
            transport->type = gaia_transport_spp;
        }
    }
#endif

    if (transport == NULL)
    {
        gaiaTransportCommonSendGaiaConnectCfm(NULL, FALSE);
        return;
    }
    
    switch (transport->type)
    {
#if defined GAIA_TRANSPORT_SPP && defined GAIA_HAVE_SPP_CLIENT
        case gaia_transport_spp:
            gaiaTransportSppConnectReq(transport, address);
            break;
#endif

        default:
            GAIA_TRANS_DEBUG(("Unknown Gaia transport %d\n", transport->type));
            GAIA_PANIC();
            gaiaTransportCommonSendGaiaConnectCfm(transport, FALSE);
            break;
    }
}

/*! @brief Respond to an incoming Gaia connection indication over a given transport.
 */
void gaiaTransportConnectRes(gaia_transport *transport) {
	switch (transport->type) {
#ifdef GAIA_TRANSPORT_RFCOMM
	case gaia_transport_rfcomm:
		gaiaTransportRfcommConnectRes(transport);
		break;
#endif
#ifdef GAIA_TRANSPORT_SPP
		case gaia_transport_spp:
		gaiaTransportSppConnectRes(transport);
		break;
#endif
	default:
		GAIA_TRANS_DEBUG(("Unknown Gaia transport %d\n", transport->type));
		GAIA_PANIC();
		break;
	}

}

/*! @brief Attempt to disconnect Gaia over a given transport.
 */
void gaiaTransportDisconnectReq(gaia_transport *transport) {
	switch (transport->type) {
#ifdef GAIA_TRANSPORT_RFCOMM
	case gaia_transport_rfcomm:
		gaiaTransportRfcommDisconnectReq(transport);
		break;
#endif
#ifdef GAIA_TRANSPORT_SPP
		case gaia_transport_spp:
		gaiaTransportSppDisconnectReq(transport);
		break;
#endif
#ifdef GAIA_TRANSPORT_GATT
	case gaia_transport_gatt:
		gaiaTransportGattDisconnectReq(transport);
		break;
#endif
	default:
		GAIA_TRANS_DEBUG(("Unknown Gaia transport %d\n", transport->type));
		GAIA_PANIC();
		break;
	}
}

/*! @brief Respond to a Gaia disconnection indication over a given transport.
 */
void gaiaTransportDisconnectRes(gaia_transport *transport) {
	switch (transport->type) {
#ifdef GAIA_TRANSPORT_RFCOMM
	case gaia_transport_rfcomm:
		gaiaTransportRfcommDisconnectRes(transport);
		break;
#endif
#ifdef GAIA_TRANSPORT_SPP
		case gaia_transport_spp:
		gaiaTransportSppDisconnectRes(transport);
		break;
#endif
#ifdef GAIA_TRANSPORT_GATT
	case gaia_transport_gatt:
		gaiaTransportGattDisconnectRes(transport);
		break;
#endif
	default:
		GAIA_TRANS_DEBUG(("Unknown Gaia transport %d\n", transport->type));
		GAIA_PANIC();
		break;
	}
}

/*! @brief Clear down state of given transport.
 */
void gaiaTransportDropState(gaia_transport *transport) {
	if (transport) {
		GAIA_TRANS_DEBUG(("gaia: drop state t=%u %d o=%04X\n",
						(uint16) transport, transport->type, (uint16) gaia->outstanding_request));

		if (gaia->outstanding_request == transport)
			gaia->outstanding_request = NULL;

		if (gaia->upgrade_transport == transport)
			gaia->upgrade_transport = NULL;

		if (gaia->pfs_state != PFS_NONE) {
			GAIA_TRANS_DEBUG(("gaia: drop pfs %d s=%lu r=%lu\n",
							gaia->pfs_state, gaia->pfs_sequence, gaia->pfs_raw_size));

			gaia->pfs_sequence = 0;
			gaia->pfs_raw_size = 0;
			SinkClose(gaia->pfs.sink);
			gaia->pfs_state = PFS_NONE;
		}

		if (AudioBusyTask() == &gaia->task_data)
			SetAudioBusy(NULL);

		switch (transport->type) {
#ifdef GAIA_TRANSPORT_RFCOMM
		case gaia_transport_rfcomm:
			gaiaTransportRfcommDropState(transport);
			break;
#endif
#ifdef GAIA_TRANSPORT_SPP
			case gaia_transport_spp:
			gaiaTransportSppDropState(transport);
			break;
#endif
#ifdef GAIA_TRANSPORT_GATT
		case gaia_transport_gatt:
			gaiaTransportGattDropState(transport);
			break;
#endif
		default:
			GAIA_TRANS_DEBUG(("Unknown Gaia transport %d\n", transport->type));
			GAIA_PANIC();
			break;
		}

		transport->connected = FALSE;
		transport->enabled = FALSE;
		transport->type = gaia_transport_none;

		/* No longer have a Gaia connection over this transport, ensure we reset any threshold state */
		gaiaTransportCommonCleanupThresholdState(transport);
	} else {
		GAIA_TRANS_DEBUG(("gaia: no transport\n"));
	}
}

/*! @brief Start Gaia as a server on a given transport.
 */
void gaiaTransportStartService(gaia_transport_type transport_type) {
	switch (transport_type) {
#ifdef GAIA_TRANSPORT_RFCOMM
	case gaia_transport_rfcomm:
		gaiaTransportRfcommStartService();
		break;
#endif
#ifdef GAIA_TRANSPORT_SPP
		case gaia_transport_spp:
		gaiaTransportSppStartService();
		break;
#endif
#ifdef GAIA_TRANSPORT_GATT
	case gaia_transport_gatt:
		break;
#endif
	default:
		GAIA_TRANS_DEBUG(("Unknown Gaia transport %d\n", transport_type));
		GAIA_PANIC();
		gaiaTransportCommonSendGaiaStartServiceCfm(transport_type, NULL, FALSE);
		break;
	}

}

void gaiaTransportStartGattServer(uint16 start_handle, uint16 end_handle) {
#ifdef GAIA_TRANSPORT_GATT
	gaiaTransportGattRegisterServer(start_handle, end_handle);
#else
	GAIA_TRANS_DEBUG(("Gaia GATT transport not enabled\n"));
	GAIA_PANIC();
#endif    
}

/*! @brief Transmit a Gaia packet over a given transport.
 */
void gaiaTransportSendPacket(Task task, gaia_transport *transport,
		uint16 length, uint8 *data) {
	switch (transport->type) {
	case gaia_transport_none:
		/*  Transport has been cleared down  */
		GAIA_TRANS_DEBUG(("gaia: transport %04x gone\n", (uint16) transport));
		gaiaTransportCommonSendGaiaSendPacketCfm(transport, data, FALSE);
		break;

#ifdef GAIA_TRANSPORT_RFCOMM
	case gaia_transport_rfcomm:
		gaiaTransportRfcommSendPacket(task, transport, length, data);
		break;
#endif
#ifdef GAIA_TRANSPORT_SPP
		case gaia_transport_spp:
		gaiaTransportSppSendPacket(task, transport, length, data);
		break;
#endif
#ifdef GAIA_TRANSPORT_GATT
	case gaia_transport_gatt:
		gaiaTransportGattSendPacket(task, transport, length, data);
		break;
#endif
	default:
		GAIA_TRANS_DEBUG(("gaia: unknown transport %d\n", transport->type));
		GAIA_PANIC();
		break;
	}
}

/*! @brief Get the stream source for a given transport.
 */
Source gaiaTransportGetSource(gaia_transport *transport) {
	switch (transport->type) {
	case gaia_transport_none:
		break;

#ifdef GAIA_TRANSPORT_RFCOMM
	case gaia_transport_rfcomm:
		return StreamSourceFromSink(gaiaTransportRfcommGetSink(transport));
		break;
#endif

#ifdef GAIA_TRANSPORT_SPP
		case gaia_transport_spp:
		return StreamSourceFromSink(gaiaTransportSppGetSink(transport));
		break;
#endif

	default:
		GAIA_TRANS_DEBUG(("Unknown Gaia transport %d\n", transport->type));
		GAIA_PANIC();
		break;
	}

	return NULL;
}

/*! @brief Get the stream sink for a given transport.
 */
Sink gaiaTransportGetSink(gaia_transport *transport) {
	switch (transport->type) {
	case gaia_transport_none:
		break;

#ifdef GAIA_TRANSPORT_RFCOMM
	case gaia_transport_rfcomm:
		return gaiaTransportRfcommGetSink(transport);
		break;
#endif
#ifdef GAIA_TRANSPORT_SPP
    case gaia_transport_spp:
        return gaiaTransportSppGetSink(transport);
        break;
#endif
#ifdef GAIA_TRANSPORT_GATT
	case gaia_transport_gatt:
		break;
#endif
	default:
		GAIA_TRANS_DEBUG(("Unknown Gaia transport %d\n", transport->type));
		GAIA_PANIC();
		return BAD_SINK;
		break;
	}

	return BAD_SINK;
}

/*! @brief Analyse an inbound command packet and process the command
 */
void gaiaTransportProcessPacket(gaia_transport *transport, uint8 *packet) {
	/*  0 bytes  1        2        3        4        5        6        7        8      len+8      len+9
	 *  +--------+--------+--------+--------+--------+--------+--------+--------+ +--------+--/ /---+ +--------+
	 *  |  SOF   |VERSION | FLAGS  | LENGTH |    VENDOR ID    |   COMMAND ID    | | PAYLOAD   ...   | | CHECK  |
	 *  +--------+--------+--------+--------+--------+--------+--------+--------+ +--------+--/ /---+ +--------+
	 */
	uint8 protocol_version = packet[GAIA_OFFS_VERSION];
	uint8 payload_length = packet[GAIA_OFFS_PAYLOAD_LENGTH];
	uint16 vendor_id = W16(packet + GAIA_OFFS_VENDOR_ID);
	uint16 command_id = W16(packet + GAIA_OFFS_COMMAND_ID);
	uint8 *payload = packet + GAIA_OFFS_PAYLOAD;

	if (protocol_version == GAIA_VERSION) {
		gaiaProcessCommand(transport, vendor_id, command_id, payload_length,
				payload);
	}
}

/*! @brief Deserialise a command from a stream transport and send it for processing
 */
void gaiaTransportProcessSource(gaia_transport *transport) {
	gaia_transport_process_source_data_t *locals = PanicUnlessMalloc(sizeof(*locals));

	locals->source = gaiaTransportGetSource(transport);
	locals->idx = 0;
	locals->expected = GAIA_OFFS_PAYLOAD;
	locals->packet_length = 0;
	locals->data_length = SourceSize(locals->source);
	locals->data = (uint8 *) SourceMap(locals->source);
	locals->packet = NULL;
	locals->flags = 0;
	locals->check = 0;

	GAIA_TRANS_DEBUG(("gaia: process_source_data: %d\n", locals->data_length));

#ifdef DEBUG_GAIA_TRANSPORT
	if (locals->data_length == 0)
	GAIA_DEBUG(("gaia: done\n"));

	else
	{
		uint16 i;
		GAIA_DEBUG(("gaia: got"));
		for (i = 0; i < locals->data_length; ++i)
		GAIA_DEBUG((" %02x", locals->data[i]));
		GAIA_DEBUG(("\n"));
	}
#endif
	if(locals->data_length > 0){
		UartSendStr("+SPPDATA:");
		UartSendData(locals->data, locals->data_length);
		UartSendStr("\r\n");
	}

	if (locals->data_length >= GAIA_OFFS_PAYLOAD) {
		while ((locals->idx < locals->data_length)
				&& (locals->packet_length < locals->expected)) {
			if (locals->packet_length > 0) {
				if (locals->packet_length == GAIA_OFFS_FLAGS)
					locals->flags = locals->data[locals->idx];

				else if (locals->packet_length == GAIA_OFFS_PAYLOAD_LENGTH) {
					locals->expected = GAIA_OFFS_PAYLOAD
							+ locals->data[locals->idx]
							+ ((locals->flags & GAIA_PROTOCOL_FLAG_CHECK) ?
									1 : 0);
					GAIA_TRANS_DEBUG(("gaia: expect %d + %d + %d = %d\n",
									GAIA_OFFS_PAYLOAD, locals->data[locals->idx], (locals->flags & GAIA_PROTOCOL_FLAG_CHECK) ? 1 : 0, locals->expected));
				}

				locals->check ^= locals->data[locals->idx];
				++locals->packet_length;
			}

			else if (locals->data[locals->idx] == GAIA_SOF) {
				locals->packet = locals->data + locals->idx;
				locals->packet_length = 1;
				locals->check = GAIA_SOF;
			}

			++locals->idx;
		}

		if (locals->packet_length == locals->expected) {
			if (((locals->flags & GAIA_PROTOCOL_FLAG_CHECK) == 0)
					|| (locals->check == 0))
				gaiaTransportProcessPacket(transport, locals->packet);

			else
				GAIA_TRANS_DEBUG(("gaia: bad chk\n"));

			SourceDrop(locals->source, locals->idx);
		}

		else if (locals->packet_length == 0) {
			/*  No start-of-frame; drop the lot  */
			GAIA_TRANS_DEBUG(("gaia: no sof\n"));
			SourceDrop(locals->source, locals->data_length);
		}

		if (locals->idx < locals->data_length) {
			MESSAGE_PMAKE(more, GAIA_INTERNAL_MORE_DATA_T); GAIA_TRANS_DEBUG(("gaia: more: %d < %d\n", locals->idx, locals->data_length));
			more->transport = transport;
			MessageSendLater(&gaia->task_data, GAIA_INTERNAL_MORE_DATA, more,
					APP_BUSY_WAIT_MILLIS);
		}
	}

	free(locals);
	locals = 0;
}

/*! @brief Pass incoming message for handling by a given transport.
 */
bool gaiaTransportHandleMessage(Task task, MessageId id, Message message) {
#ifdef GAIA_TRANSPORT_RFCOMM
	if (gaiaTransportRfcommHandleMessage(task, id, message))
		return TRUE;
#endif
#ifdef GAIA_TRANSPORT_SPP
	if (gaiaTransportSppHandleMessage(task, id, message))
	return TRUE;
#endif
	return FALSE;
}

/*! @brief Notify clients of disconnection.
 */
void gaiaTransportNotifyClientsOfDisconnection(gaia_transport *transport)
{
    if (transport == gaia->upgrade_transport)
    {
        UpgradeTransportDisconnectRequest();
    }
}

/*! @brief Clean transport state on disconnection.
 */
void gaiaTransportTidyUpOnDisconnection(gaia_transport *transport)
{
    gaiaTransportNotifyClientsOfDisconnection(transport);
    gaiaTransportDropState(transport);
}
