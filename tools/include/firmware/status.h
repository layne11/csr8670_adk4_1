/*
 * Copyright (c) 2016 Qualcomm Technologies International, Ltd.
 * This file was automatically generated for firmware version 19.5
 */

#ifndef __STATUS_H

#define __STATUS_H

#include <csrtypes.h>
/*! @file status.h @brief Access to chip status information.
**
**
Functions to access the chip status information.
*/
#include <message_.h>
#include <app/status/status_if.h>


/*!
  @brief Queries the value of the specified status fields.

  @param count The number of status fields to read.
  @param fields count values each specifying one status value
  @param results count values which will be written to

  The current value of each of the fields will be written to the
  corresponding element of the results array.
*/
void StatusQuery(uint16 count, const status_field *fields, uint16 *results);

#endif
