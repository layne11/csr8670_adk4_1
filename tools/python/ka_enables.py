# Copyright (c) 2014 - 2016 Qualcomm Technologies International, Ltd.
# All Rights Reserved.
# Qualcomm Technologies International, Ltd. Confidential and Proprietary.
# Part of BlueLab-7.1-Release
# Part of the Python bindings for the kalaccess library.

from ctypes import c_int, c_byte, c_void_p, POINTER
from ka_ctypes import ka_err



class KaEnables(object):
    def __init__(self, core):
        self._core = core
        self._cfuncs = {}
        self._core._add_cfunc(self._cfuncs, 'ka_hal_read_dsp_enable'        , POINTER(ka_err), [c_void_p, POINTER(c_byte)])
        self._core._add_cfunc(self._cfuncs, 'ka_hal_write_dsp_enable'       , POINTER(ka_err), [c_void_p, c_byte])
        self._core._add_cfunc(self._cfuncs, 'ka_hal_read_dsp_clock_enable'  , POINTER(ka_err), [c_void_p, POINTER(c_byte)])
        self._core._add_cfunc(self._cfuncs, 'ka_hal_write_dsp_clock_enable' , POINTER(ka_err), [c_void_p, c_byte])

    def read_dsp_enable(self):
        """Returns True if the Kalimba processor is enabled."""
        result = c_byte()
        err = self._cfuncs['ka_hal_read_dsp_enable'](self._core._get_ka(), result)
        self._core._handle_error(err)
        return bool(result.value)

    def write_dsp_enable(self, enable):
        """Enables the Kalimba processor."""
        err = self._cfuncs['ka_hal_write_dsp_enable'](self._core._get_ka(), enable)
        self._core._handle_error(err)

    def read_dsp_clock_enable(self):
        """Returns True if the Kalimba processor is clocked."""
        result = c_byte()
        err = self._cfuncs['ka_hal_read_dsp_clock_enable'](self._core._get_ka(), result)
        self._core._handle_error(err)
        return bool(result.value)

    def write_dsp_clock_enable(self, enable):
        """Enables the clock on the Kalimba processor."""
        err = self._cfuncs['ka_hal_write_dsp_clock_enable'](self._core._get_ka(), enable)
        self._core._handle_error(err)

    def enable_and_clock_dsp(self):
        """Convenience function to enable the Kalimba and provide a clock. 
        Equivalent to write_dsp_clock_enable(True) and write_dsp_enable(True)."""
        self.write_dsp_clock_enable(True)
        self.write_dsp_enable(True)
    
