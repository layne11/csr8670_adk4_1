/****************************************************************************

        Copyright (c) 2010 - 2016 Qualcomm Technologies International, Ltd.
        All Rights Reserved.
        Qualcomm Technologies International, Ltd. Confidential and Proprietary.

FILE
    capsense_if.h

CONTAINS
    Definitions for the capacitive touch sensor subsystem.

DESCRIPTION
    This file is seen by the stack, and VM applications, and
    contains things that are common between them.
*/

/*!
 @file capsense_if.h
 @brief Parameters for CapsenseConfigure(), CapsenseConfigurePad()
        and CapsensePadQuery().

 See also #MessageCapsenseChanged and MessageCapsenseTask() for documentation
 on receiving messages on Capsense events.
 For more information and recommended usage instructions, please read the
 Capacitive Touch Sense System Guide included in the ADK.
*/

#ifndef __CAPSENSE_IF_H__
#define __CAPSENSE_IF_H__

/*! @brief General configuration functions for capacitive touch sensor. */
typedef enum
{
    /*! Sets bulk capacitance, chip specific value */
    CAPSENSE_SET_BULK_CAPACITANCE,
    /*! Sets Cint update divider */
    CAPSENSE_SET_CINT_UPDATE_DIVIDER,
    /*! Sets number of ADC averages per measurement (0-7 = 1-128) */
    CAPSENSE_SET_NUM_ADC_AVERAGES,
    /*! Sets gap between measurements in milliseconds (0-16ms) */
    CAPSENSE_SET_MEASUREMENT_GAP
} capsense_config_key;


/*! @brief Pad specific configuration functions for capacitive touch sensor. */
typedef enum
{
    /*! Sets trigger level of the pad specified. The level change necessary to
     *  trigger an event.  This event will be delivered to the task registered
     *  with MessageCapsenseTask(). See #MessageCapsenseChanged.
     *  A non-zero threshold enables a pad.  A zero threshold disables a pad.
     *  If all pads are disabled, the capsense block will be shut down to
     *  conserve power.  This is the initial (power-on) state.
     *  Positive and negative thresholds are set equal.  The minimum threshold
     *  setting is 65fF and the maximum is 1023fF.  Enabling an uncalibrated
     *  pad or pad 0 when it is used as the analogue shield will result in an
     *  error.  Changing the threshold level on an enabled pad is permitted.
     *  The change will be effected the next time that pad is scanned.
     *  Any queued messages will still be delivered after a pad is disabled.
     */
    CAPSENSE_SET_TRIGGER_LEVEL
} capsense_config_pad_key;


/*! @brief The action which caused the event */
typedef enum
{
    /*! Pad delta value has crossed from below positive threshold to above it.
     */
    CAPSENSE_EVENT_POS,
    /*! Pad delta value has crossed from above negative threshold to below it.
     */
    CAPSENSE_EVENT_NEG
} capsense_event_type;


/*! @brief Returned values from CapsensePadQuery() */
typedef struct
{
    /*! Pad absolute capacitance. This is the sum of both internal (Cbulk and 
     *  Cint) and external (touch) capacitances.
     */
    uint16  Cint_fF;

    /*! Pad capacitance change compared to the slow moving average value.
     */
    int16   Cint_delta_fF;  
} capsense_pad_state;


/*! @brief Description of single event registered */
typedef struct
{
    unsigned int  pad :8;        /*!< Pad number */
    unsigned int  direction :8;  /*!< Whether positive or negative excursion */
    uint16        time_ms;       /*!< Timestamp of event */
} capsense_event;
#endif /* __CAPSENSE_IF_H__ */

