/*
 * Copyright (c) 2001 - 2016 Qualcomm Technologies International, Ltd.
 * All Rights Reserved.
 * Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 */

#ifndef __MEMORY_H

#define __MEMORY_H

#include <stdlib.h>
#include <string.h>

char *bcopy(const char *, char *, int);
void bzero(char *, int);
int memcmp8(const void *, const void *, size_t);

char *strdup(const char *);

#endif
