/*
 * Copyright (c) 2016 Qualcomm Technologies International, Ltd.
 * This file was automatically generated for firmware version 19.5
 */

#ifndef __SRAM_H

#define __SRAM_H

#include <csrtypes.h>
/*! @file sram.h @brief Traps to map SRAM into VM memory space */


/*!
  @brief Maps external SRAM into the VM memory space

  @param sram_addr External SRAM physical address to map
  @param size Size (in uint16s) of SRAM to map

  @return Pointer to start of mapped SRAM, NULL if cannot map

  At the first invocation this finds a sufficient window in the VM's
  data address space and makes the requested portion of external SRAM
  visible; the application can then access external SRAM through the
  returned pointer (see below for the limitations). Subsequent
  invocations can map in different portions of SRAM, at which point
  any existing pointers into the old portion become invalid. (In
  general, accesses through such pointers cannot be trapped by the VM,
  so may cause subtly wrong behaviour rather than panicking the
  application).

  There are restrictions on the use of external SRAM mapped in this
  way. The application may read and write to the SRAM directly, but
  pointers into it may not be passed into most traps; if an attempt is
  made to do so, the application will be stopped. Only a few traps
  fully support operations on pointers into SRAM -- currently memset,
  memmove, and Util* traps.

  If the requested size is greater than the maximum supported, or
  mapping fails for some other reason, a NULL pointer is returned.

  To use external SRAM, the BlueCore device and firmware must both
  support it, and the application circuit must connect up the SRAM
  appropriately, including a PIO to select it (which will be
  unavailable for any other use). PSKEY_SRAM_PIO must be set to
  indicate this PIO to the firmware; by default the PSKEY is not set,
  and if not set, SramMap() will always return NULL. Please refer to
  appropriate product data sheet for the PIO value.
  Also, the devices which have SQIF support, PSKEY_SRAM_SIZE must be
  set to indicate actual size of SRAM connected. If this PSKey holds
  its default value (i.e. 0) then address and size passed to this trap
  are never validated against the value of this PSKey. This can lead
  to mapping a non-existent area in SRAM.

  External SRAM is only supported in certain BlueCore firmware.
  Please refer the appropriate product data sheet for external SRAM
  support and refer CS-307510-AN and CS-324244-AN for usage and other
  details.

  Example Usage:
        ptr = PanicNull(SramMap(...));

  @note
  Even if multiple mappings by application are not contiguous or mapping
  does not start from address 0, the available size for SRAM allocation
  will be restricted to the total SRAM size minus the highest address
  ever mapped using this trap.
*/
uint16 *SramMap(uint32 sram_addr, uint16 size);

/*!
  @brief Allocate memory from SRAM
  @param size of allocation (in uint16s)
  @return Pointer to the allocated memory

  This trap is used to allocate memory from SRAM. The pointer returned
  by this trap can be used to read/write data from/to SRAM. There are
  restrictions on the use of the pointer returned by this trap. SRAM pointer
  must be used within the VM application. The VM application must not pass the
  SRAM pointer to Bluecore firmware modules through trap/message interface since
  this can lead to panic. Only few standard memory operations related VM traps
  and utility operation related VM traps can handle this pointer directly.
  These traps are:
  1. free()
  2. memmove()
  3. realloc()
  4. UtilCompare()
  5. UtilFind()
  6. UtilGetNumber()
  7. UtilHash()
  8. UtilSwap()

  To use external SRAM, the BlueCore device and firmware must both
  support it, and the application circuit must connect up the SRAM
  appropriately, including a PIO to select it (which will be
  unavailable for any other use). PSKEY_SRAM_PIO must be set to
  indicate this PIO to the firmware; by default the PSKEY is not set.
  Please refer to appropriate product data sheet for the PIO value.
  Also, PSKEY_SRAM_SIZE must be set to indicate actual size of SRAM
  connected; this PSKEY holds a default value of zero. If either of
  these PSKEYs is not set, SramAlloc() will always return NULL.

  @note
  For memory allocations from SRAM, some space within SRAM is consumed for
  metadata information.
*/
void *SramAlloc(size_t sz);

#endif
