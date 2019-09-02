/*
 * Copyright (c) 2001 - 2016 Qualcomm Technologies International, Ltd.
 * All Rights Reserved.
 * Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 */

#ifndef __STDIO_H

#define __STDIO_H

#include <stdarg.h>

int printf(const char *, ...);
int vprintf(const char *, va_list arg);
int sprintf(char *, const char *, ...);
int vsprintf(char *, const char *, va_list arg);
int putchar(int c);
int puts(const char *s);

/* CSR additions: initial integer selects VmPutChar channel */
int cprintf(int, const char *, ...);
int vcprintf(int, const char *, va_list arg);
int cputs(int ch, const char *s);

#endif
