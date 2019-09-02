/* Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd. */
/* Part of ADK 4.1 */

#ifndef GATT_APPLE_NOTIFICATION_CLIENT_MSG_HANDLER_H_
#define GATT_APPLE_NOTIFICATION_CLIENT_MSG_HANDLER_H_

#include <csrtypes.h>
#include <message.h>


/***************************************************************************
NAME
    appleNotificationClientMsgHandler

DESCRIPTION
    Handler for external messages sent to the library in the client role.
*/
void appleNotificationClientMsgHandler(Task task, MessageId id, Message payload);


#endif /* GATT_APPLE_NOTIFICATION_CLIENT_MSG_HANDLER_H_ */

