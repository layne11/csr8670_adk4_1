/****************************************************************************
Copyright (c) 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    vmal.h

DESCRIPTION
    Header file for the VMAL library.
*/

/*!
@file   vmal.h
@brief  Header file for the VM Abstraction Layer library.
*/

#ifndef VMAL_H_
#define VMAL_H_

#include <transform.h>
#include <message.h>

typedef struct
{
    uint16 key;   
    uint32 value;
} vmal_operator_keys_t;

/*
 *  The SIZEOF_OPERATOR_MESSAGE macro is used to determine the length of a message passed to the OperatorMessage API.
 *  On legacy (Gordon, Rick) systems sizeof() always returns the number of uint16 in a structure. On Crescendo,
 *  it returns the number of bytes. This is nort what is required for crescendo.
 *  The OperatorMessage API expects the message length to always be reported in number of uint16s, regardless of platform.
 *  When sending messages that contain no paramters always declare message id to be uint16, otherwise it is likely to
 *  default to a uint32.
 */
#define SIZEOF_OPERATOR_MESSAGE(msg) (size_t)(sizeof(msg)/sizeof(uint16))

/*!
    \brief Create an RTP decode transform and configure for SBC

    \param source The RTP packet Source
    \param sink The Sink to receive the decoded SBC packets

    \return The Transform created (or NULL if the operation failed)
*/
Transform VmalTransformRtpSbcDecode(Source source, Sink sink);

/*!
    \brief Create an RTP encode transform and configure for SBC

    \param source The SBC packet Source
    \param sink The Sink to receive the RTP encoded packets

    \return The Transform created (or NULL if the operation failed)
*/
Transform VmalTransformRtpSbcEncode(Source source, Sink sink);

/*!
    \brief Create an RTP decode transform and configure for MP3

    \param source The RTP packet Source
    \param sink The Sink to receive the decoded MP3 packets

    \return The Transform created (or NULL if the operation failed)
*/
Transform VmalTransformRtpMp3Decode(Source source, Sink sink);

/*!
    \brief Create an RTP encode transform and configure for MP3

    \param source The MP3 packet Source
    \param sink The Sink to receive the RTP encoded packets

    \return The Transform created (or NULL if the operation failed)
*/
Transform VmalTransformRtpMp3Encode(Source source, Sink sink);

/*!
    \brief Create an RTP decode transform and configure for AAC

    \param source The RTP packet Source
    \param sink The Sink to receive the decoded AAC packets

    \return The Transform created (or NULL if the operation failed)
*/
Transform VmalTransformRtpAacDecode(Source source, Sink sink);

/*!
    \brief Assign the Task which will receive messages relating to a Sink

    \param sink The Sink which will generate messages
    \param task The Task to which messages will be sent

    \return The Task previously associated with this Sink, or NULL if no Task 
            was previously associated.
*/
Task VmalMessageSinkTask(Sink sink, Task task);

/*!
    \brief Get the Task associated with a Sink

    \param sink The Sink from which to get the associated Task

    \return The Task associated with this Sink, or NULL if no Task is 
            associated.
*/
Task VmalMessageSinkGetTask(Sink sink);

/*!
    \brief Assign the Task which will receive messages relating to a Source

    \param source The Source which will generate messages
    \param task The Task to which messages will be sent

    \return The Task previously associated with this Source, or NULL if no Task
            was previously associated.
*/
Task VmalMessageSourceTask(Source source, Task task);

/*!
    \brief Get the Task associated with a Source

    \param source The Source from which to get the associated Task

    \return The Task associated with this Source, or NULL if no Task is 
            associated.
*/
Task VmalMessageSourceGetTask(Source source);

/*!
    \brief Create an operator

    \param cap_id The ID of the capability to create 
    
    \return The operator ID (or zero if it failed)
*/
uint16 VmalOperatorCreate(uint16 cap_id);

/*!
    \brief Create an operator with operator create keys

    \param cap_id The ID of the capability to create 
	\param keys The operator create keys
	\param num_keys The number of operator create keys specified
    
    \return The operator ID (or zero if it failed)
*/
uint16 VmalOperatorCreateWithKeys(uint16 capability_id, vmal_operator_keys_t* keys, uint16 num_keys);

/*!
    \brief Enable/Disable the main processor

    \param enable TRUE to turn main processor on, FALSE to turn it off
    
    \return TRUE if successful, otherwise FALSE.
*/
bool VmalOperatorFrameworkEnableMainProcessor(bool enable);

/*!
    \brief Send a message to an operator 

    \param opid The Operator ID
    \param send_msg The message to send
    \param send_len The size of the message to send
    \param recv_msg Pointer to populate with response message if required
    \param recv_len The expected length of the response message (or zero if not
                    required)
    
    \return TRUE if the message was sent successfully, otherwise FALSE.
*/
bool VmalOperatorMessage(uint16 opid, void * send_msg, uint16 send_len, 
                                      void * recv_msg, uint16 recv_len);

#endif /*VMAL_H_*/
