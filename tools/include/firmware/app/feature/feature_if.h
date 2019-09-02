/****************************************************************************

        Copyright (c) 2014 - 2016 Qualcomm Technologies International, Ltd.
        All Rights Reserved.
        Qualcomm Technologies International, Ltd. Confidential and Proprietary.

FILE
    feature_if.h

CONTAINS
    Interface elements between firmware (related to Security Decoder and
    Licensable features) and VM applications.

DESCRIPTION
    This file is seen by VM applications and the firmware.
*/

/*!
 @file feature_if.h
 @brief Parameter for FeatureVerifyLicense().

*/

#ifndef __FEATURE_IF_H__
#define __FEATURE_IF_H__

/*! @brief Identifiers of feature defined here. These are used by
 *   FeatureVerifyLicense().
 */
typedef enum 
{
    FEATURE_IIR,
    FEATURE_ANC
}feature_id;

#endif /* __FEATURE_IF_H__  */
