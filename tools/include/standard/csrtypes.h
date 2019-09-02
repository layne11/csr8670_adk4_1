/*
 * Copyright (c) 2001 - 2016 Qualcomm Technologies International, Ltd.
 * All Rights Reserved.
 * Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 */

/*!
	@mainpage VM and Native Reference Guide

	This section documents all the functions and types that are available to 
	an on-chip VM or Native application. Use the links above to navigate the documentation.
	 
*/

/*!
	@file csrtypes.h
	@brief Contains definitions of commonly required types.
*/

#ifndef __TYPES_H

#define __TYPES_H

/* ANSI types */

typedef unsigned int size_t;

/* XAP types */

typedef unsigned long  uint32;
typedef unsigned short uint16;
typedef long           int32;
typedef short          int16;
typedef signed char    int8;
typedef unsigned char  uint8;

/* Types for PAN */

typedef uint16         bool;

#ifndef TRUE
#define TRUE           ((bool)1)
#endif
#ifndef FALSE
#define FALSE          ((bool)0)
#endif

#endif
