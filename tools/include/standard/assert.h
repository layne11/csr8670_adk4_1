/*
 * Copyright (c) 2001 - 2016 Qualcomm Technologies International, Ltd.
 * All Rights Reserved.
 * Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 */

#undef assert

#include <panic.h>

#ifdef NDEBUG
#define assert(e)  	((void)0)
#else
#define assert(e)       ((void)((e) || (Panic(),0)))
#endif
