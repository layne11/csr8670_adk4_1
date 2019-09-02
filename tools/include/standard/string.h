/*
 * Copyright (c) 2001 - 2016 Qualcomm Technologies International, Ltd.
 * All Rights Reserved.
 * Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 */

#ifndef __STRING_H

#define __STRING_H

#ifndef NULL
#define NULL ((void *)0)
#endif /* ndef NULL */

#include <csrtypes.h>

char *strcat(char *, const char *);
char *strncat(char *, const char *, size_t);
char *strcpy(char *, const char *);
char *strncpy(char *, const char *, size_t);

int strcmp(const char *, const char *);
int strncmp(const char *, const char *, size_t);

/*
 *  The C Standard does not define the sorting order to be used by these two
 *  functions.
 *
 *  In this implementation they provide a case-insensitive comparison.
 */
int strcoll(const char *, const char *);
size_t strxfrm(char *, const char *, size_t);

char *strchr(const char *, int);
char *strrchr(const char *, int);

void *memchr(const void *, int, size_t);

size_t strlen(const char *);

void *memset(void *, int, size_t);
void *memcpy(void *, const void *, size_t);
void *memmove(void *, const void *, size_t);
int memcmp(const void *x, const void *y, size_t n);

size_t strspn(const char *, const char *);
size_t strcspn(const char *, const char *);
char *strstr(const char *, const char *);

char *strpbrk(const char *, const char *);

char *strtok(char *, const char *);

char *strerror(int);

/* The following functions are not part of the ANSI/ISO standard */

char *strrpbrk(const char *, const char *);
void *mempbrk(const void *, const char *, size_t);
void *memrchr(const void *, int, size_t);
bool memmatch(const void *, const char *, size_t);
bool memrmatch(const void *, const char *, size_t);

#endif
