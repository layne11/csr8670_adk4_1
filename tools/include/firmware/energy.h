/*
 * Copyright (c) 2016 Qualcomm Technologies International, Ltd.
 * This file was automatically generated for firmware version 19.5
 */

#ifndef __ENERGY_H

#define __ENERGY_H

#include <csrtypes.h>
/*! @file energy.h @brief Estimate the energy in a SCO connection */
#include <sink_.h>


/*!
   @brief Turn on energy estimation with both upper and lower thresholds.

   @param sco       The SCO stream to enable energy estimation on.
   @param lower     The lower bound on energy
   @param upper     The upper bound on energy

   One #MESSAGE_ENERGY_CHANGED message will be sent to the task associated
   with the SCO stream when estimated energy content goes outside the bounds.
   The message payload will indicate if the energy content went above the
   upper threshold, or below the lower one. The task must then
   reenable the energy estimation if it wishes to receive further messages.

   Estimation is automatically stopped when the SCO connection is closed.
   It can be stopped under program control using EnergyEstimationOff().

   @return          FALSE if the sink does not correspond to a SCO connection.
*/
bool EnergyEstimationSetBounds(Sink sco, uint16 lower, uint16 upper);

/*!
  @brief Disable estimation on the specified SCO connection.
  @param sco The SCO connection to disable energy estimation on.
  @return FALSE if the sink does not correspond to a SCO connection.
*/
bool EnergyEstimationOff(Sink sco);

#endif
