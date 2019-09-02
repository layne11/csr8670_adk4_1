/*
 * Copyright (c) 2016 Qualcomm Technologies International, Ltd.
 * This file was automatically generated for firmware version 19.5
 */

#ifndef __SINK_H

#define __SINK_H

#include <csrtypes.h>
/*! @file sink.h   @brief Operations on sinks which take 8-bit data */
#include <sink_.h>
#include <bdaddr_.h>
#include <app/vm/vm_if.h>
#include <app/stream/stream_if.h>


/*!
  @brief Report how many bytes can successfully be claimed in the corresponding sink.

  @param sink The Sink to check.
  @return Zero if the sink is not valid.

  @note This call will return zero if the sink stream is connected to another stream.
*/
uint16 SinkSlack(Sink sink);

/*!
  @brief Attempt to claim the indicated number of extra bytes in a sink.

  @param sink The sink to claim.
  @param extra The number of bytes to attempt to claim.
  @return The offset of the claimed region if the claim was successful, 
  0xFFFF otherwise.

  Claims will certainly fail if the sink is invalid, 
  or if SinkSlack indicates that the space is unavailable.

  @note
  This call will return zero if the sink stream is connected to another stream.
  
  @note
  This trap will block VM application if there are not sufficient buffer memory
  resources to support the claim.
  
*/
uint16 SinkClaim(Sink sink, uint16 extra);

/*!
  @brief Map the sink into the address map, returning a pointer to the first
  byte in the sink. 

  @param sink The sink to map into the address map.
  @return zero if the sink is invalid.

  Only the total number of claimed bytes (as
  returned by SinkClaim(sink,0)) are accessible. At most one sink can be
  mapped in at any time; pointers previously obtained from SinkMap become
  invalid when another call to SinkMap is made.

  @note This call will return zero if the sink stream is connected to another stream.
*/
uint8 *SinkMap(Sink sink);

/*!
  @brief Flush the indicated number of bytes out of the sink. 

  @param sink The Sink to flush.
  @param amount The number of bytes to flush.

  The specified bytes of data are passed to the corresponding byte stream,
  for example out to the UART, or into BlueStack as if sent by a
  RFC_DATA_IND for UART/RFCOMM sinks respectively.

  @return TRUE on success, or FALSE if the operation failed because the
  sink was invalid or amount exceeded the size of the sink as reported by
  SinkClaim(sink, 0).

  @note This call will return FALSE if the sink stream is connected to another stream.
*/
bool SinkFlush(Sink sink, uint16 amount);

/*!
  @brief Flush the indicated number of bytes out of the sink, with a header.

  @param sink The Sink to flush data from.
  @param amount The number of bytes of data to flush.
  @param header The header to use.
  @param length The size of the header.

  Associates the header with the message.
  The specified bytes of data are then passed to the corresponding byte stream, for
  example out to the UART, or into BlueStack as if sent by a
  RFC_DATA_IND for UART/RFCOMM sinks respectively.

  @return TRUE on success, or FALSE if the operation failed because the
  sink was invalid or amount exceeded the size of the sink as reported by
  SinkClaimed.

  @note This call will return FALSE if the sink stream is connected to another stream.
*/
bool SinkFlushHeader(Sink sink, uint16 amount, const uint16 *header, uint16 length);

/*!
    @brief Configure a particular sink.
    @param sink The Sink to configure.
    @param key The key to configure.
    @param value The value to write to 'key'

    @return FALSE if the request could not be performed, TRUE otherwise.

    See #stream_config_key for the possible keys and their meanings. Note that
    some keys apply only to specific kinds of sink. Configuration Values will
    be lost once sink is closed and key needs to be reconfigured if sink
    is reopened.
*/
bool SinkConfigure(Sink sink, stream_config_key key, uint32 value);

/*!
    @brief Request to close the sink
    @param sink The sink to close
    
    @return TRUE if the source could be closed, and FALSE
    otherwise.

    Sink stream can be closed only if the requested sink stream is not
    connected to any stream. If the requested stream is connected,
    then TransformDisconnect or StreamDisconnect should be called
    before SinkClose.

    Some sinks, such as RFCOMM connections or the USB
    hardware, have a lifetime defined by other means, and cannot be
    closed using this call. 
*/
bool SinkClose(Sink sink);

/*!
    @brief Request to alias two Sinks
    @param sink1 The first Sink to be aliased
    @param sink2 The second Sink to be aliased
    
    @return TRUE if the sinks are aliased successfully, else FALSE.

    @note This call will return FALSE if the sink stream is connected to another stream.
*/
bool SinkAlias(Sink sink1, Sink sink2);

/*!
    @brief Request to synchronise two Sinks
    @param sink1 The first Sink to be synchronised
    @param sink2 The second Sink to be synchronised
    
    @return TRUE if the Sinks are synchronised successfully, else FALSE.

    Call this function to synchronise timing drifts between two
    sink streams before calling a StreamConnect
*/
bool SinkSynchronise(Sink sink1, Sink sink2);

/*!
   @brief Find the SCO handle corresponding to a sink.
   @param sink The Sink to get the handle for
   @returns The handle, or 0 is the sink wasn't a SCO sink
*/
uint16 SinkGetScoHandle(Sink sink);

/*!
  @brief Find the RFCOMM connection corresponding to a sink.
  @param sink The Sink to get the connection identifier for.
*/
uint16 SinkGetRfcommConnId(Sink sink);

/*!
  @brief Find the L2CAP channel id corresponding to a sink.
  @param sink The Sink to get the connection identifier for.
*/
uint16 SinkGetL2capCid(Sink sink);

/*!
  @brief Get the Bluetooth address from a sink.

  @param sink The Sink to fetch the Bluetooth address from.
  @param taddr If the address is found it will be returned to the 
  location pointed at by this value.

  @return TRUE if such an address was found, FALSE otherwise.
*/
bool SinkGetBdAddr(Sink sink, tp_bdaddr *tpaddr);

/*!
  @brief Get the RSSI for the ACL for a sink.

  @param sink The Sink which uses the ACL, 
  @param rssi If the sink corresponds to an ACL the RSSI in dBm will be written to this location.

  @return TRUE if the RSSI was obtained, FALSE otherwise.
*/
bool SinkGetRssi(Sink sink, int16 *rssi);

/*!
  @brief Read the away time on the underlying ACL.
  @param sink identifies the underlying ACL
  @param msec receives the away time if the call succeeds (unmodified otherwise)

  @return TRUE if the sink identifies an ACL and the away time on that
  link could be read, FALSE otherwise.

  The away time is the time since any packet was received on that ACL
  and is reported in milliseconds. If the time exceeds 0xFFFF, 0xFFFF will
  be returned (this is unlikely with sensible link supervision
  timeouts.)
*/
bool SinkPollAwayTime(Sink sink, uint16 *msec);

/*! 
  @brief Return TRUE if a sink is valid, FALSE otherwise.

  @param sink The sink to check.
*/
bool SinkIsValid(Sink sink);

#endif
