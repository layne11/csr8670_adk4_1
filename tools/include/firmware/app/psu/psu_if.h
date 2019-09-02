/****************************************************************************

        Copyright (c) 2009 - 2016 Qualcomm Technologies International, Ltd.
        All Rights Reserved.
        Qualcomm Technologies International, Ltd. Confidential and Proprietary.

FILE
    psu_if.h

CONTAINS
    Definitions for the psu subsystem.

DESCRIPTION
    This file is seen by the stack, and VM applications, and
    contains things that are common between them.
*/

/*!
 @file psu_if.h
 @brief Parameters for use in the PsuConfigure VM trap.

 The various psu_id values allow the VM application to disabled the switchmode
 power supplies and linear regulators on BlueCore. The exact number and type of
 power supplies on BlueCore, as well as their intended purpose, vary between
 different BlueCore variants. Consult the datasheet of your BlueCore variant
 for more details.

 Example : PsuConfigure(PSU_SMPS0, PSU_ENABLE, 1); Enable Switchmode power supply0
 Example : PsuConfigure(PSU_ALL, PSU_ENABLE, 0); Shut down all PSUs
*/

#ifndef __PSU_IF_H__
#define __PSU_IF_H__

/*! @brief PSU regulator identifiers. */
typedef enum 
{
    PSU_SMPS0,       /*!< Switch mode regulator */
    PSU_SMPS1,       /*!< Switch mode regulator */
    PSU_LINEAR0,     /*!< Linear regulator */
    PSU_LINEAR1,     /*!< Linear regulator */
    PSU_LINEAR2,     /*!< Linear regulator */
    PSU_VBAT_SWITCH, /*!< VBYP/VBAT switch */
    PSU_ALL,
    NUM_OF_PSU
}psu_id;

/*! @brief PSU regulator configuration keys. */
typedef enum
{
    PSU_ENABLE = 0,               /*!< enable power regulator. */
    PSU_SMPS_INPUT_SEL_VBAT = 1,  /*!< force SMPS source = VBAT */
    PSU_VOLTAGE_SENSE_REMOTE = 3  /*!< force psu to use remote voltage sensing. */
} psu_config_key;

#endif /* __PSU_IF_H__  */

