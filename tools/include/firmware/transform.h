/*
 * Copyright (c) 2016 Qualcomm Technologies International, Ltd.
 * This file was automatically generated for firmware version 19.5
 */

#ifndef __TRANSFORM_H

#define __TRANSFORM_H

#include <csrtypes.h>
/*! @file transform.h @brief Transform data between sources and sinks */
#include <app/vm/vm_if.h>
#include <sink_.h>
#include <source_.h>
#include <transform_.h>


/*!
  @brief Start a transform; newly created transforms must be started.

  @param transform The transform to start.

  @return FALSE on failure, TRUE on success.

  @note
  Application shouldn't call this function for transforms created via 
  @a StreamConnect() call as those transforms are started implicitly.
*/
bool TransformStart(Transform transform);

/*!
  @brief Stop a transform,

  @param transform The transform to stop.

  @return FALSE on failure, TRUE on success.

  @note
  Application shouldn't call this function for transforms created 
  via @a StreamConnect() call. To stop data flow for such transforms, 
  application would typically call @a TransformDisconnect().
*/
bool TransformStop(Transform transform);

/*!
  @brief Disconnect and destroy a transform. 

  @param transform The transform to destroy.

  The transform can no longer be used after this call.
*/
void TransformDisconnect(Transform transform);

/*!
  @brief Report if any traffic has been handled by this transform.

  @param transform The transform to query.

  Reads and clears a bit that reports any activity on a
  transform. This can be used to detect activity on connect streams.

  @return TRUE if the transform exists and has processed data, FALSE otherwise.
*/
bool TransformPollTraffic(Transform transform);

/*!
  @brief Find the transform connected to a source.

  @param source The source to look for.

  @return The transform connected to the specified source, or zero if no transform or connection is active.
*/
Transform TransformFromSource(Source source);

/*!
  @brief Find the transform connected to a sink.

  @param sink The sink to look for.

  @return The transform connected to the specified sink, or zero if no transform or connection is active.
*/
Transform TransformFromSink(Sink sink);

/*!
  @brief Configure parameters associated with a transform. 

  @param transform The transform to configure.
  @param key Valid values depend on the transform.
  @param value Valid values depend on the transform.
  @return Returns FALSE if the key was unrecognised, or if the value was out of bounds.
*/
bool TransformConfigure(Transform transform, vm_transform_config_key key, uint16 value);

/*!
  @brief Create a transform between the specified source and sink. 

  @param source The Source to use in the transform.
  @param sink The Sink to use in the transform.

  @return 0 on failure, otherwise the transform.

  Copies data in chunks.
*/
Transform TransformChunk(Source source, Sink sink);

/*!
  @brief Create a transform between the specified source and sink. 

  @param source The Source to use in the transform.
  @param sink The Sink to use in the transform.

  @return 0 on failure, otherwise the transform.

  Removes bytes from start and end of packets.
*/
Transform TransformSlice(Source source, Sink sink);

/*!
   @brief Create an ADPCM decode transform between source and sink
   
   @param source The source containing ADPCM encoded data
   @param sink   The destination sink
   
   @return 0 on failure, otherwise the transform.
*/
Transform TransformAdpcmDecode(Source source, Sink sink);

/*!
  @brief Route an RTP stream carrying SBC data into Kalimba.

  @param source The media Source.
  @param sink   The sink (typically corresponding to a Kalimba port).

  @return The transform if successful, or zero on failure.
*/
Transform TransformRtpSbcEncode(Source source, Sink sink);

/*!
  @brief Route an RTP stream carrying SBC data out of Kalimba.

  @param source The source containing the SBC stream (typically corresponding to a Kalimba port).
  @param sink The media Sink.

  @return The transform if successful, or zero on failure.
*/
Transform TransformRtpSbcDecode(Source source, Sink sink);

/*!
  @brief Packs MP3 frames from the DSP into RTP packets

  @param source The media Source.
  @param sink A Kalimba port.

  @return The transform if successful, or zero on failure.
*/
Transform TransformRtpMp3Encode(Source source, Sink sink);

/*!
  @brief Unpacks MP3 frames from RTP-MP3 packets.

  @param source An L2CAP stream containing RTP MP3 packets.
  @param sink A stream that the MP3 frames will be written to.

  @return The transform if successful, or zero on failure.
*/
Transform TransformRtpMp3Decode(Source source, Sink sink);

/*!
  @brief Unpacks AAC frames from RTP-AAC packets.

  @param source An L2CAP stream containing RTP AAC packets.
  @param sink A stream that the AAC frames will be written to.

  @return The transform if successful, or zero on failure.
*/
Transform TransformRtpAacDecode(Source source, Sink sink);

/*!
  @brief Route an RTP stream carrying ATRAC data out of Kalimba.

  @param source The source containing the ATRAC stream (typically corresponding to a Kalimba port).
  @param sink The media Sink.

  @return The transform if successful, or zero on failure.
*/
Transform TransformRtpAtracDecode(Source source, Sink sink);

/*!
  @brief Generic HID transform supporting the following devices
        a) Boot mode mouse
        b) Report mode mouse
        c) Boot mode keyboard
        d) Report mode keyboard


  @param source The Source data will be taken from.
  @param sink The Sink data will be written to.

  @return An already started transform on success, or zero on failure.

*/
Transform TransformHid(Source source, Sink sink);

/*!
  @brief Packs Audio frames from the DSP into RTP packets. This
  trap attaches RTP stamping for the Audio packets arriving from DSP.
  Currently only AptX CODEC will be supported. In future the RTP encode
  traps for all codecs will be re-factored to this trap and also the existing
  traps for individual codecs will be deprecated.

  The sequence of function calls in a VM Application which is acting as an
  encoder would be:
  1. Call the TransformRtpEncode trap.
  2. Using the TransformConfigure trap configure the required parameters:
     2.1 Manage Timing (Yes/No)
     2.2 Payload header size
     2.3 SCMS Enable (Yes/No)
     2.4 SCMS Bits
     2.5 Frame period
     2.6 Packet size
  3. Call the TransformStart trap.

  @param source The media Source.
  @param sink The sink receiving the Audio Digital stream
                (typically corresponding to a Kalimba port).

  @return The transform if successful, or zero on failure.
*/
Transform TransformRtpEncode(Source source, Sink sink);

/*!
  @brief Unpacks Audio frames from Audio-RTP packets. Currently only 
  AptX CODEC will be supported. In future the RTP decode traps for all
  codecs will be re-factored to this trap and also the existing traps for 
  individual codecs will be deprecated.

  The sequence of function calls in a VM Application which is acting as a
  decoder would be:
  1. Call the TransformRtpDecode trap.
  2. Using the TransformConfigure trap configure the required parameters:
     2.1 SCMS Enable (Yes/No)
     2.2 Payload header size
     2.3 Is payload header required for DSP (Yes/No)
  3. Call the TransformStart trap

  @param source The source containing the Audio Digital stream 
                (typically corresponding to a Kalimba port).
  @param sink The media Sink.

  @return The transform if successful, or zero on failure.
*/
Transform TransformRtpDecode(Source source, Sink sink);

#endif
