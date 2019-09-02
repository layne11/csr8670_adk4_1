/*
 * Copyright (c) 2001 - 2016 Qualcomm Technologies International, Ltd.
 * All Rights Reserved.
 * Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 */

#ifndef __STDLIB_H

#define __STDLIB_H

#ifndef NULL
#define NULL ((void *)0)
#endif /* ndef NULL */

#include <csrtypes.h>

void *malloc(size_t);
void free(void *);
void *calloc(size_t, size_t);
void *realloc(void *, size_t);

int atoi(const char *);

void exit(int);

#endif
