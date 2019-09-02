/*****************************************************************
Copyright (c) 2011 - 2015 Qualcomm Technologies International, Ltd.
*/

#ifndef _GAIA_TRANSPORT_SPP_H
#define _GAIA_TRANSPORT_SPP_H

#ifdef GAIA_TRANSPORT_SPP

#ifdef GAIA_HAVE_SPP_CLIENT
/*! @brief
 */
void gaiaTransportSppConnectReq(gaia_transport *transport, bdaddr *address);
#endif /* GAIA_HAVE_SPP_CLIENT */

/*! @brief
 */
void gaiaTransportSppConnectRes(gaia_transport *transport);

/*! @brief
 */
void gaiaTransportSppDisconnectReq(gaia_transport *transport);

/*! @brief
 */
void gaiaTransportSppDisconnectRes(gaia_transport *transport);

/*! @brief
 */
void gaiaTransportSppDropState(gaia_transport *transport);

/*! @brief
 */
void gaiaTransportSppStartService(void);

/*! @brief
 */
void gaiaTransportSppSendPacket(Task task, gaia_transport *transport, uint16 length, uint8 *data);

/*! @brief
 */
Sink gaiaTransportSppGetSink(gaia_transport *transport);

/*! @brief
 */
bool gaiaTransportSppHandleMessage(Task task, MessageId id, Message message);

#endif /* GAIA_TRANSPORT_SPP */

#endif /* _GAIA_TRANSPORT_SPP_H */
