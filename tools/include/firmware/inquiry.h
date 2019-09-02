/*
 * Copyright (c) 2016 Qualcomm Technologies International, Ltd.
 * This file was automatically generated for firmware version 19.5
 */

#ifndef __INQUIRY_H

#define __INQUIRY_H

#include <csrtypes.h>
/*! @file inquiry.h @brief Configure Bluetooth inquiry procedure.
**
**
These functions can be used to change the details of how BlueCore
schedules Bluetooth inquiry relative to other Bluetooth activity.
They are equivalent to the Inquiry_Priority BCCMD.
*/
#include <app/bluestack/types.h>


/*!
    @brief Sets the priority level of Bluetooth inquiry.

    @param priority The desired priority level.

    @return TRUE if the level was successfully set, FALSE otherwise.
*/
bool InquirySetPriority(InquiryPriority priority);

/*!
    @brief Gets the current priority level of Bluetooth inquiry.

    @return The current priority level.
*/
InquiryPriority InquiryGetPriority(void);

#endif
