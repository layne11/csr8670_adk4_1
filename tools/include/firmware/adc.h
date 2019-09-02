/*
 * Copyright (c) 2016 Qualcomm Technologies International, Ltd.
 * This file was automatically generated for firmware version 19.5
 */

#ifndef __ADC_H

#define __ADC_H

#include <csrtypes.h>
#include <message_.h>
#include <app/vm/vm_if.h>
#include <app/adc/adc_if.h>
/*! @file adc.h @brief Access to ADC hardware.
**
**
BlueCore has an analogue-to-digital converter (ADC) which can be
used to read the  voltage at a number of locations, such as:
- At AIO pins #adcsel_aio0 to #adcsel_aio3.
- From #adcsel_vref - the internal voltage in the chip.
- From #adcsel_vdd_bat - the battery.
.
(Note that this hardware is distinct from the ADCs that form part
of the analogue audio codec.)
**
Note that not all sources are available on all BlueCores.
**
The voltage (in mV) from a source is given by:
**
(reading) * VrefConstant / (vref_reading)
**
\a reading comes from the #MESSAGE_ADC_RESULT for the source in
question. \a vref_reading is a corresponding reading for
#adcsel_vref that has been taken relatively recently.
VmReadVrefConstant() should be used to determine \a VrefConstant.
This calculation determines how many mV each bit of \a reading
corresponds to, correcting for natural variation and systematic
errors in the ADC reading process.
**
For example, on CSR8670 VmReadVrefConstant() returns 700 (which is in
mV, and therefore represents 0.7 V). A reading of #adcsel_vref might
return 531 (this number will vary slightly with process and
temperature). In which case, 1 bit of ADC reading corresponds
to a real value of 700/531 mV. Hence, if a reading of 100 is
obtained from #adcsel_aio0, this corresponds to a true voltage of
**
100 * 700/531 = 132 mV on AIO0.
**
The underlying hardware has 8 bits of resolution on older BlueCores,
and 10 bits of resolution from BlueCore 5 onwards. The raw reading
may be conditioned by the firmware before being returned to the
application.
*/


/*!
  @brief Send a request to the ADC to make a reading from adc_source.
  When the reading is made, a #MESSAGE_ADC_RESULT message will be sent
  to the task passed. See adc.h for how to interpret the result.

  @param task The task which will receive the ADC result message.
  @param adc_source The source (#vm_adc_source_type) to take readings from.
  @return TRUE if request is accepted, else FALSE.
*/
bool AdcRequest(Task task, vm_adc_source_type adc_source);

#endif
