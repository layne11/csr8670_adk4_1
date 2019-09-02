/* Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd. */
/* Part of ADK 4.1 */


#ifndef GATT_APPLE_NOTIFICATION_CLIENT_DEBUG_H_
#define GATT_APPLE_NOTIFICATION_CLIENT_DEBUG_H_

/* Macro used to generate debug version of this library */
#ifdef GATT_APPLE_NOTIFICATION_DEBUG_LIB


#ifndef DEBUG_PRINT_ENABLED
#define DEBUG_PRINT_ENABLED
#endif

#include <panic.h>
#include <print.h>
#include <stdio.h>

#define GATT_APPLE_NOTIFICAITON_DEBUG_INFO(x) {PRINT(("%s:%d - ", __FILE__, __LINE__)); PRINT(x);}
#define GATT_APPLE_NOTIFICATION_DEBUG_PANIC(x) {GATT_APPLE_NOTIFICAITON_DEBUG_INFO(x); Panic();}


#else /* GATT_APPLE_NOTIFICATION_DEBUG_LIB */


#define GATT_APPLE_NOTIFICAITON_DEBUG_INFO(x)
#define GATT_APPLE_NOTIFICATION_DEBUG_PANIC(x)

#endif /* GATT_APPLE_NOTIFICATION_DEBUG_LIB */


#endif /* GATT_APPLE_NOTIFICATION_CLIENT_DEBUG_H_ */
