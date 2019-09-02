/*
 * Copyright (c) 2016 Qualcomm Technologies International, Ltd.
 * This file was automatically generated for firmware version 19.5
 */

#ifndef __PANIC_H

#define __PANIC_H

#include <csrtypes.h>
/*! @file panic.h @brief Terminate the application unhappily.
**
**
These functions can be used to panic the application, forcing it to terminate abnormally.
*/
/*!
Panics the application if the value passed is FALSE.
*/
#define PanicFalse PanicZero
/*!
Panics the application if the value passed is zero.
*/
#define PanicZero(x) (unsigned int) PanicNull((void *) (x))
/*!
Panics the application if the value passed is not zero.
*/
#define PanicNotZero(x) PanicNotNull((const void *) (x))
/*!
Allocates memory equal to the size of T and returns a pointer to the memory if successful. If the
memory allocation fails, the application is panicked.
*/
#define PanicUnlessNew(T) (T*)PanicUnlessMalloc(sizeof(T))


/*!
    @brief Panics the application unconditionally.
*/
void Panic(void);

/*!
    @brief Panics the application if the pointer passed is NULL, otherwise returns the pointer.
*/
void *PanicNull(void *);

/*!
    @brief Panics the application if the pointer passed is not NULL, otherwise returns.
*/
void PanicNotNull(const void *);

/*!
    @brief Allocates sz words and returns a pointer to the memory if successful. If
    the memory allocation fails, the application is panicked.
*/
void *PanicUnlessMalloc(size_t sz);

/*!
  @brief Reads the last panic code and *can* clear it.
  @param clear_panic Value TRUE to clear the last panic code after reading it,
  otherwise FALSE.
  @return The last panic code.

  @note
  The panic code is not cleared at boot-up (due to soft reset or hard reset).
  Therefore, the VM application can get stale value from power-on or repeated
  value over multiple power cycles if it was not cleared after boot-up.

  VM application can check whether BlueCore has panicked previously first by
  invoking VmGetResetSource. If the reset source is RESET_SOURCE_FIRMWARE then
  panic code can be read using this API.
*/
uint16 PanicGetAndClearFirmwareLastCode(bool clear_panic);

#endif
