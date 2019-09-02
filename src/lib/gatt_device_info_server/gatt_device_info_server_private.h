/* Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd. */
/* Part of ADK 4.1 */

#ifndef GATT_DEVICE_INFO_SERVER_PRIVATE_H
#define GATT_DEVICE_INFO_SERVER_PRIVATE_H

#include <csrtypes.h>
#include <message.h>
#include <panic.h>
#include <stdlib.h>

#include "gatt_device_info_server.h"
#include "gatt_device_info_server_debug.h"

/* Macros for creating messages */
#define MAKE_DEVICE_INFO_MESSAGE(TYPE) MESSAGE_MAKE(message,TYPE##_T);

#endif /* GATT_DEVICE_INFO_SERVER_PRIVATE_H */
