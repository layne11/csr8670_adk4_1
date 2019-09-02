/*
 * Copyright (c) 2016 Qualcomm Technologies International, Ltd.
 * This file was automatically generated for firmware version 19.5
 */

#ifndef __CHARGER_H

#define __CHARGER_H

#include <csrtypes.h>
#include <app/charger/charger_if.h>
/*! @file charger.h @brief Configuration of the onchip battery charger present on some BlueCore variants */


/*!
  @brief Configuration of the onchip battery charger present on some BlueCore variants
  @param key Which aspect of the charger hardware to configure
  @param value Which value to use

  @return TRUE if the key is valid and the input value corresponding to the key is in valid, else FALSE.

  Consult the datasheet for your BlueCore variant and the documentation for the
  #charger_config_key type for more information.
*/
bool ChargerConfigure(charger_config_key key, uint16 value);

/*!
  @brief Get the status of the charging hardware.

  Reports information on the state of the on chip battery charger.
  This is only supported on BlueCore variants with charger hardware.
*/
charger_status ChargerStatus(void);

/*!
    @brief Enables charger events and configures their debouncing.
    @param events_to_enable Each bit position enables a charger event. See the
           #charger_events enum for details of the events.
    @param count The number of times to read from the charger pins before notifying the application
    @param period The delay in milliseconds between reads.

    A #MESSAGE_CHARGER_CHANGED message will be delivered to the task setup by MessageChargerTask()
    when the debounced state of the charger pins has changed.

    For example calling ChargerDebounce((CHARGER_VREG_EVENT | CHARGER_CONNECT_EVENT), 4, 1000)
    will enable the VREGEN_H and the charger attach/detach events. The application will
    receive a #MESSAGE_CHARGER_CHANGED message after the charger pins value has changed and then 
    remained stable for 4 consecutive reads 1000 milliseconds apart. 
    For CSR8670 or CSR8670-like chips this same call would also enable a #MESSAGE_CHARGER_CHANGED
    message to be received when VREG_EN has been detected pressed for four consecutive readings,
    each 1000mS apart.
    
    @return A charger_events bit mask where any bit set high indicates an invalid event bit 
    passed in the "events_to_enable" parameter. Any invalid event will result in the ChargerDebounce()
    request being rejected. Returns 0 on success.

*/
charger_events ChargerDebounce(charger_events events_to_enable, uint16 count, uint16 period);

/*!
    @brief Returns the state of the battery recorded at boot time.

    @return the status of the battery at boot, as described in #charger_battery_status
*/
charger_battery_status ChargerGetBatteryStatusAtBoot(void);

#endif
