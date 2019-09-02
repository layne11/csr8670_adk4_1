/*
 * Copyright (c) 2016 Qualcomm Technologies International, Ltd.
 * This file was automatically generated for firmware version 19.5
 */

#ifndef __MESSAGE__H

#define __MESSAGE__H

#include <csrtypes.h>
/*! @file message_.h @brief Message types */
/*!
Message identifier type.
*/
typedef uint16 MessageId;
/*!
Message delay type.
*/
typedef uint32 Delay;
/*!
Message type.
*/
typedef const void *Message;
/*!
Task type.
*/
typedef struct TaskData *Task;
/*!
TaskData type.
*/
typedef struct TaskData { void (*handler)(Task, MessageId, Message); } TaskData;

#endif
