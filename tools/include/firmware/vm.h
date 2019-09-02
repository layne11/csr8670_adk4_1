/*
 * Copyright (c) 2016 Qualcomm Technologies International, Ltd.
 * This file was automatically generated for firmware version 19.5
 */

#ifndef __VM_H

#define __VM_H

#include <csrtypes.h>
#include <app/vm/vm_if.h>
#include <bdaddr_.h>
/*! @file vm.h @brief VM access to low-level firmware and hardware */


/*!
  @brief Output a character on the given channel.
  @param c The character to output.
  @param channel The channel to output to.
*/
void VmPutChar(uint16 channel, uint16 c);

/*!
  @brief Counts the maximum number of additional memory blocks which can be allocated
  @return The number of free slots in the VM memory map.
  Note that this does not correspond to physical memory; it's
  possible for malloc() and similar functions to fail even when
  this function returns non-zero; the value returned by this
  function is only an upper bound on the number of possible
  allocations.
  See CS-110364-ANP "VM Memory Mapping and Memory Usage" for more
  detail.
*/
uint16 VmGetAvailableAllocations(void);

/*!
  @brief Read the current value of a 32-bit millisecond timer.

  Don't poll this; using MessageSendLater is much more efficient.
*/
uint32 VmGetClock(void);

/*!
  @brief Enables or disables deep sleep.
  @param en FALSE will prevent the chip going into deep sleep.
  TRUE will permit the chip to go into deep sleep. It
  can still be blocked from doing so by other firmware components, or
  by PSKEY_DEEP_SLEEP_STATE).
  @return The previous status.
*/
bool VmDeepSleepEnable(bool en);

/*!
  @brief Request that the radio transmitter be enabled or disabled.

  @return TRUE if the request was satisfied, FALSE if it
  was not possible.

  Equivalent to using ENABLE_TX and DISABLE_TX over BCCMD from
  off-chip.
*/
bool VmTransmitEnable(bool enabled);

/*!
   @brief Allows the application to override the default specified by 
   PSKEY_LC_DEFAULT_TX_POWER.
   @param power int16 The new default power to use in dBm.

   The default transmit power is used for paging, inquiry, and their responses,
   and as the initial power for new acl links. The value passed is rounded down to
   the next available value when set, so the value returned by a call to 
   VmTransmitPowerGetDefault may be less than that previously passed to 
   VmTransmitPowerSetDefault.

   @return TRUE on success, else FALSE.
*/
bool VmTransmitPowerSetDefault(int16 power);

/*!
   @brief Allows the application to override the maximum specified by 
   PSKEY_LC_MAX_TX_POWER.
   @param power The new maximum power to use in dBm.

   The maximum transmit power is only referenced when increasing the transmit power,
   so if the transmit power on a link is already above this level the new value
   will not take effect until an attempt is made to increase the power.

   The value passed is rounded down to the next available value when set, so the value 
   returned by a call to VmTransmitPowerGetMaximum may be less than that previously passed 
   to VmTransmitPowerSetMaximum.

   @return TRUE on success, else FALSE.
*/
bool VmTransmitPowerSetMaximum(int16 power);

/*!
   @brief Returns the current default power setting.
*/
int16 VmTransmitPowerGetDefault(void);

/*!
   @brief Returns the current maximum power setting.
*/
int16 VmTransmitPowerGetMaximum(void);

/*!
  @brief Reads the internal temperature of BlueCore

  @return Approximate temperature in degrees Celsius, or INVALID_TEMPERATURE if it could not be read.
*/
int16 VmGetTemperature(void);

/*!
    @brief Enables the VM to set the PCM_CLK frequency when using
    the 4 mHz internal clock. 
    @param frequency The frequency PCM_CLK will output at as a vm_pcm_clock_setting type. 
    Valid values are PCM_CLK_OFF, PCM_CLK_128, PCM_CLK_256 and PCM_CLK_512 kHz.
    @return TRUE if the change was accepted, else FALSE.
*/
bool VmSetPcmClock(vm_pcm_clock_setting frequency);

/*!
    @brief Enables the VM to check the current PCM_CLK frequency,
    when it is being generated from the 4MHz internal clock.

    @return A vm_pcm_clock_setting type indicating the current PCM clock frequency.
*/
vm_pcm_clock_setting VmGetPcmClock(void);

/*!
    @brief Enables the amux clock output on AIO0.

    @param enabled Whether the clock is enabled.

    Note that this is only available on BC4 parts, excluding BC4-external.

    Note that PSKEY_AMUX_CLOCK must be have been set correctly, and
    PSKEY_AMUX_AIO0 must be set to ANA_AMUX_A_SEL_DIG_OUT_CLK_AMUX.
*/
void VmAmuxClockEnable(bool enabled);

/*!
    @brief Get the absolute value for Vref on this version of BlueCore

    @return The value of Vref (in mV) for the version of BlueCore the
    application is running on.

    This is a constant value for a given version of BlueCore. See adc.h for
    the intended use of this function.

    In version 23 and prior firmware, AdcRequest() would return a reading for
    Vref scaled as if its nominal value were 1250mV, and (for instance) the
    battery library could assume this.

    From version 25 firmware onwards, AdcRequest() will return a reading for
    Vref (#adcsel_vref) without scaling it to a nominal voltage. This function
    provides the information that the battery library needs to adjust its
    readings appropriately.
*/
uint16 VmReadVrefConstant(void);

/*!
    @brief Sends an RFCOMM Bluestack primitive.
    @param prim A pointer to the primitive to send. The memory must have been 
    dynamically allocated.
*/
void VmSendRfcommPrim(void *prim);

/*!
    @brief Sends an L2CAP Bluestack primitive.
    @param prim A pointer to the primitive to send. The memory must have been 
    dynamically allocated.
*/
void VmSendL2capPrim(void *prim);

/*!
    @brief Sends a DM Bluestack primitive.
    @param prim A pointer to the primitive to send. The memory must have been 
    dynamically allocated.
*/
void VmSendDmPrim(void *prim);

/*!
    @brief Sends an SDP Bluestack primitive.
*/
void VmSendSdpPrim(void *prim);

/*!
    @brief Converts a VM address space pointer to a handle
    @param pointer The pointer to convert.

    When assembling a primitive which includes indirect blocks, the application
    must:
    - Allocate a block
    - Convert it to a handle
    - Store the handle in the primitive rather than storing the pointer itself
*/
void *VmGetHandleFromPointer(void *pointer);

/*!
    @brief Converts a handle to a VM address space pointer.
    @param handle The handle to convert.

    If a Bluestack primitive refers to indirect blocks of memory, those are
    presented in the primitive as handles rather than real pointers. To access
    the data an application must pass the handle to VmGetPointerFromHandle()
    which will make the indirect block visible to the application. The application
    must call this exactly once for each such indirect block, and the resulting
    pointers must all be passed to free. Failure to perform this procedure will 
    result in a resource leak.
    Note - while VmGetHandleFromPointer() will successfully produce a handle from
    a pointer to a constant, VmGetPointerFromHandle() will not produce a pointer
    from such a handle. It will instead panic with VM_PANIC_READ_FROM_ILLEGAL_ADDRESS.
*/
void *VmGetPointerFromHandle(void *handle);

/*!
  @brief Reads the temperature sensors on BlueCore BC7+ chips

  @return Temperature in degrees Celsius, or INVALID_TEMPERATURE if
          it could not be read.

       Sensor       Action
    TSENSOR_MAIN    Reads cached temp from main sensor
    TSENSOR_PMU     Reads temp from PMU sensor
        xxx         All other sensor numbers return INVALID_SENSOR
*/
int16 VmGetTemperatureBySensor(vm_temp_sensor sensor);

/*!
  @brief Allows one #MESSAGE_TX_POWER_CHANGE_EVENT to be sent to the system task.
  @param enable TRUE to permit power change messages to be sent, FALSE to stop them.

  This is a one-shot enable, i.e., it allows exactly one power change
  message to be sent to the task registered with MessageSystemTask().
  Once that message has been sent, VmTransmitPowerMessagesEnable()
  must be called again to allow the next power change message to be
  sent. This gives the application the opportunity to limit the rate
  at which power change messages arrive, which can be quite frequent
  if unchecked.

  Calling VmTransmitPowerMessagesEnable(TRUE) again before a power
  change message has been received has no effect.
*/
void VmTransmitPowerMessagesEnable(bool enable);

/*!
    @brief Returns an enum value relating to the cause of the last reset.

           When a system reset occurs on BC7 chips a value is stored which 
           relates to the cause of the reset.  This value can be retrieved by a 
           VM app and takes the enumerated values defined in the #vm_reset_source 
           type. Any value not covered by this definition cannot be determined 
           and is deemed an unexpected reset.

           See the type definition for more information.

    @return  A #vm_reset_source type indicating the source of the last reset.
*/
vm_reset_source VmGetResetSource(void);

/*!
    @brief Sends an ATT Bluestack primitive.
    @param prim A pointer to the primitive to send. The memory must have been 
    dynamically allocated.
*/
void VmSendAttPrim(void *prim);

/*!
  @brief Removes all the advertising filters, allowing all advert reports
         through to the host.
*/
bool VmClearAdvertisingReportFilter(void);

/*!
  @brief This helps in controlling how BlueCore filters advertising 
         report events by BLE advertising data content. 

         Filtering is based only on the contents of advertising
         data. Since directed connectable adverts do not contain advertising
         data, and they are expressly intended for the receiving device,
         they are always passed to the host and are unaffected by this
         filter. The event type, Bluetooth device address of the sender and 
         other properties of the advertising reports are ignored by the 
         filter.

         With no filter present, all advertising packets received during 
         scanning are passed to the host in LE Advertising Report Events, 
         subject to advert flood protection. The filter is used to select 
         advertising reports based on the contents of the advertising data
         (AD) and send to the host only the matching reports, thus saving
         the host being woken up unnecessarily.

  @param operation  Describes the relationship between multiple filters. 
         Currently the only valid operation is OR (0x00), meaning that 
         adverts will be sent to the host if they are matched by any of the
         filters.

  @param ad_type The AD type of the AD structure to match. The filter will 
         only match adverts containing an AD structure of this type. Enum as 
         defined in Bluetooth Assigned Numbers.

  @param interval The interval for repeated attempts to match the pattern in the 
         data portion of the AD structure. For example, if the interval is 
         4 then we attempt to match at offsets, 0, 4, 8, 12, etc in the data
         portion of the AD structure. If interval is 0 then we only attempt 
         to match at offset 0. If interval is 0xffff, then exact match is 
         required.

  @param pattern_length The length of the pattern data, i.e. number of uint8s
         pattern data present in location pointed by @a pattern_addr.

  @param pattern_addr A pointer to the location where pattern data 
         is present. Each pattern data is 8bit data and no two pattern data 
         should be packed inside uint16. BlueCore will only consider
         lower 8 bit of each pattern data for pattern matching. If 
         pattern_addr is NULL, then no pattern matching.
*/
bool VmAddAdvertisingReportFilter(uint16 operation,uint16 ad_type, uint16 interval, uint16 pattern_length,uint16 pattern_addr);

/*!
    @brief Returns a bit pattern representing the source of the voltage regulator 
           enable signal on (re)boot.

           When the voltage regulators of a CSR8670 or CSR8670-like chip are 
           enabled as part of the turn-on sequence or during reset, a value is 
           stored indicating the source of the signals responsible for enabling 
           the chip's power supplies.  This value can be retrieved by a VM app and 
           is a bit-pattern composed of the enumerated values defined in the 
           #vm_power_enabler type.

           See the type definition for more information.

    @return  A bit pattern representing regulator enabling signals.  See the 
             #vm_power_enabler type definition.
*/
uint16 VmGetPowerSource(void);

/*!
    @brief Retrives the tp_bdaddr value from the given CID value.

    @param cid The connection identifier to fetch the Bluetooth address from.
    @param tpaddr If the address is found it will be returned to the 
    location pointed at by this value.
    
    @return TRUE if an address was found for a given CID, FALSE otherwise. 
*/
bool VmGetBdAddrtFromCid(uint16 cid, tp_bdaddr *tpaddr);

/*!
  @brief By default, the VM software watchdog is disabled. Calling this trap with
   a valid timeout value will initiate/reset the VM software watchdog timer.
   If the VM software watchdog timer expires, then a VM Panic will be raised 
   with Panic code VM_PANIC_SW_WD_EXPIRED and it will reset the BlueCore device.

   Whenever the VM software watchdog timer expires with the debugger attached,
   BlueCore firmware will never panic/reset the chip since it is unpredictable
   to calculate the time to extend the VM software watchdog timer due to the
   delay caused during debugging.
   
   Firmware will automatically extend the VM software watchdog timer during long
   firmware operations such as Serial flash erase, PSKey write on
   EEPROM device, so that the VM software watchdog timer is not unduly affected.

   For other operations at host which may take some time to complete, the BlueCore
   firmware does not automatically extend the VM software watchdog timer 
   because the time taken is dependent on the individual scenario with the Host.
   It is left to the application to extend/kick the VM software watchdog timer.
   As the UART Host operations are much slower than USB, it is suggested that
   longer VM software watchdog timeout values are used with UART Host operations.
   Typically, VM application is not blocked during the Host communication. During
   the Host communication, there is sufficient time left in the BlueCore firmware to
   schedule the VM application and thus, VM application can kick the VM software
   watchdog timer.

   The time taken to start the DSP application depends on the details of the 
   DSP application, which can not be predicted by the BlueCore firmware. If the 
   VM software watchdog is in use, the VM application should consider the time
   taken to start the DSP application when deciding on the timeout value. The
   BlueCore firmware will not automatically extend the timeout.

   The VM software watchdog is used to detect the misbehaving applications that have
   gone off the rails. If the BlueCore firmware disables VM software watchdog by calling
   the trap VmSoftwareWdKick() with single disable code zero, then it is possible for
   a rogue application to get there with AL set to zero (quite a common value to have
   in AL)which will disable the watchdog and prevent the system recovering. To avoid 
   this unintended behaviour and to be confident that firmware code will not disable
   the VM software watchdog even if VM application branches inappropriately, a 3-stage
   disable sequence is required.
   
   The 3-stage disable sequence to disable the VM software watchdog is as follows:
       VmSoftwareWdKick(VM_SW_WATCHDOG_DISABLE_CODE1)
       VmSoftwareWdKick(VM_SW_WATCHDOG_DISABLE_CODE2)
       VmSoftwareWdKick(VM_SW_WATCHDOG_DISABLE_CODE3)

   @param timeout The timeout period in seconds, in the range 1 to 300 or the specific
   disable codes VM_SW_WATCHDOG_DISABLE_CODE1, VM_SW_WATCHDOG_DISABLE_CODE2,
   VM_SW_WATCHDOG_DISABLE_CODE3.

   @return returns TRUE,
    1.Whenever the VM software watchdog is kicked i.e., valid range (1-300 seconds).
      Eg: VmsoftwareWDKick(20); - Returns TRUE
    2.Whenever the 3-stage disable sequence is followed as per the requirement.
    Eg: VmsoftwareWDKick(20); - Returns TRUE
        VmSoftwareWDKick(VM_SW_WATCHDOG_DISABLE_CODE1); - Returns FALSE
        VmSoftwareWDKick(VM_SW_WATCHDOG_DISABLE_CODE2); - Returns FALSE
        VmSoftwareWDKick(VM_SW_WATCHDOG_DISABLE_CODE3); - Returns TRUE
    
    returns FALSE,
    1.Whenever the VM software watchdog timeout doesn't fall under 1-300 seconds range.
    2.Whenever the VM software watchdog doesn't follow the 3-stage sequence or disabling
      the VM software watchdog fails.
    Eg: VmsoftwareWDKick(20); - Returns TRUE
        VmSoftwareWDKick(VM_SW_WATCHDOG_DISABLE_CODE2); - Returns FALSE
        VmSoftwareWDKick(VM_SW_WATCHDOG_DISABLE_CODE1); - Returns FALSE
        VmSoftwareWDKick(VM_SW_WATCHDOG_DISABLE_CODE3); - Returns FALSE

    \note
    The purpose of the 3-stage disable sequence is to ensure that rogue
    applications do not randomly disable the watchdog by kicking it with some
    single disable codes. The return value of the this trap is designed to
    indicate to the user that either the watchdog kick has succeeded due to a
    valid timeout within the specified range or that the entire 3-stage disable
    sequence has succeeded. Returning TRUE for a call on the first disable sequence
    alone would actually go against the notion that the operation of disabling the
    watchdog has succeeded. In-fact at that moment (after having kicked with the
    first disable code), there is no guarantee of whether the disabling is either
    being done deliberately (in a valid manner) or the kick has been called by
    some rouge code. Hence, unless the entire disable operation does not succeed,
    the VM must not return TRUE.

*/
bool VmSoftwareWdKick(uint16 timeout);

/*!
    @brief Retrives public address for a given random address.

    @param random_addr Random address.
    @param public_addr If the public address is found it will be returned to the
    location pointed at by this value.
    
    @return TRUE if public address was found for a given resolvable random
    address, FALSE otherwise.
*/
bool VmGetPublicAddress(const tp_bdaddr *random_addr, tp_bdaddr *public_addr);

#endif
