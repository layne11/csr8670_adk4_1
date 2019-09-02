/****************************************************************************

        Copyright (c) 2001 - 2016 Qualcomm Technologies International, Ltd.
        All Rights Reserved.
        Qualcomm Technologies International, Ltd. Confidential and Proprietary.

FILE
        adc_if.h  -  ADC interface
 
CONTAINS
        The GLOBAL definitions for the ADC subsystem from the VM
 
DESCRIPTION
        This file is seen by the stack, and VM applications, and
        contains things that are common between them.

*/

#ifndef __APP_ADC_IF_H__
#define __APP_ADC_IF_H__

/*!
    @file adc_if.h 
    
    @brief The interface to the ADC.
*/
/*!
   @brief The possible sources we can take readings from using AdcRequest().
 */
enum adcsel_enum {
        /*! Test pin AIO0.  Read with VM and BCCMD. */
        adcsel_aio0,

        /*! Test pin AIO1.  Read with VM and BCCMD. */
        adcsel_aio1,

        /*! Test pin AIO2.  Read with VM and BCCMD.
            Not available on BC7 or BC7-derived chips, e.g.
            CSR8670 */
        adcsel_aio2,

        /*! Test pin AIO3.  Read with VM and BCCMD.
            Not available on BC7 or BC7-derived chips, e.g.
            CSR8670 */
        adcsel_aio3,

        /*! The internal reference voltage in the chip.
            Read with VM and BCCMD. */
        adcsel_vref,

        /*! Battery voltage at output of the charger (CHG_VBAT).
            Read with VM and BCCMD. */
        adcsel_vdd_bat,

        /*! Input to bypass regulator (VCHG).
            Read with VM and BCCMD. */
        adcsel_byp_vregin,

        /*! Battery voltage sense (CHG_VBAT_SENSE).
            Read with VM and BCCMD. */
        adcsel_vdd_sense,

        /*! VREG_ENABLE voltage. Read with VM and BCCMD. */
        adcsel_vregen

};
typedef enum adcsel_enum vm_adc_source_type;

#endif /* ifndef __APP_ADC_IF_H__ */
