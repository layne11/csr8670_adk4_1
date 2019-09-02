/*
 * Copyright (c) 2016 Qualcomm Technologies International, Ltd.
 * This file was automatically generated for firmware version 19.5
 */

#ifndef __FEATURE_H

#define __FEATURE_H

#include <csrtypes.h>
/*! @file feature.h @brief Related to licensing of new features
*/
#include <app/feature/feature_if.h>


/*!
    @brief This trap checks whether a feature is licensed for a device.

    @param feature Identifier for a given feature. Refer #feature_id

    @return TRUE if device has valid license for feature, else FALSE.

    This trap verifies whether a device is licensed to use a given feature.
 
*/
bool FeatureVerifyLicense(feature_id feature);

#endif
