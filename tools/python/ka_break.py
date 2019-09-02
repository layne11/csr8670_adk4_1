# Copyright (c) 2014 - 2016 Qualcomm Technologies International, Ltd.
# All Rights Reserved.
# Qualcomm Technologies International, Ltd. Confidential and Proprietary.
# Part of BlueLab-7.1-Release
# Part of the Python bindings for the kalaccess library.

from ctypes import c_int, c_uint, c_byte, c_void_p, POINTER, byref
from ka_ctypes import ka_err


class KaBreak:
    

    def __init__(self, core):
        self._core = core
        self._cfuncs = {}
        self._core._add_cfunc(self._cfuncs, 'ka_init_bp_system',             POINTER(ka_err), [c_void_p])
        self._core._add_cfunc(self._cfuncs, 'ka_is_valid_pm_break_location', POINTER(ka_err), [c_void_p, c_uint, POINTER(c_byte), c_int])
        
        self._core._add_cfunc(self._cfuncs, 'ka_set_pm_break',               POINTER(ka_err), [c_void_p, c_uint, c_int])
        self._core._add_cfunc(self._cfuncs, 'ka_clear_pm_break',             POINTER(ka_err), [c_void_p, c_uint])
        self._core._add_cfunc(self._cfuncs, 'ka_set_dm_break',               POINTER(ka_err), [c_void_p, c_uint, c_uint, c_byte, c_byte])
        self._core._add_cfunc(self._cfuncs, 'ka_clear_dm_break',             POINTER(ka_err), [c_void_p, c_uint, c_uint, c_byte, c_byte])
    
    def reinit_breakpoint_system(self):
        """Reinitialises the breakpoint system. Note: this function clears all existing PM and DM breakpoints on the target."""
        err = self._cfuncs['ka_init_bp_system'](self._core._get_ka())
        self._core._handle_error(err)
        
    def is_pm_break_hit(self):
        """Returns True if the processor is currently on a PM breakpoint."""
        state = self._core.other.get_dsp_state()
        return state.is_on_pm_break() or state.is_on_instruction_break()

    def is_valid_pm_break_location(self, address):
        """Returns True if the specified address is a valid location for a PM breakpoint."""
        ok = c_byte()
        err = self._cfuncs['ka_is_valid_pm_break_location'](self._core._get_ka(), address, byref(ok), 0)
        self._core._handle_error(err)
        return bool(ok.value)
        
    def is_dm_break_hit(self):
        """Returns True if the processor is currently on a DM breakpoint."""
        return self._core.other.get_dsp_state().is_on_dm_break()


    def set_pm_breakpoint(self, address):
        """Sets a PM breakpoint at, or as close as possible to, the supplied program address."""
        err = self._cfuncs['ka_set_pm_break'](self._core._get_ka(), address, 0)
        self._core._handle_error(err)

    def clear_pm_breakpoint(self, address):
        """Clears any PM breakpoint set at 'address'."""
        err = self._cfuncs['ka_clear_pm_break'](self._core._get_ka(), address)
        self._core._handle_error(err)

    def set_dm_breakpoint(self, start_address, end_address, trigger_on_read, trigger_on_write):
        """Sets a DM breakpoint, given a start address, end address, and whether to trigger on read 
           and/or write of this address range."""
        err = self._cfuncs['ka_set_dm_break'](self._core._get_ka(), start_address, end_address, trigger_on_read, trigger_on_write)
        self._core._handle_error(err)

    def clear_dm_breakpoint(self, start_address, end_address, trigger_on_read, trigger_on_write):
        """Clears any DM breakpoint matching the given start address, end address and triggering
           conditions."""
        err = self._cfuncs['ka_clear_dm_break'](self._core._get_ka(), start_address, end_address, trigger_on_read, trigger_on_write)
        self._core._handle_error(err)
