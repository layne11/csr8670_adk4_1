# Copyright (c) 2014 - 2016 Qualcomm Technologies International, Ltd.
# All Rights Reserved.
# Qualcomm Technologies International, Ltd. Confidential and Proprietary.
# Part of BlueLab-7.1-Release
# Part of the Python bindings for the kalaccess library.

from ctypes import c_void_p, c_uint, POINTER
from ka_ctypes import ka_err, to_ctypes_array


class KaMemory:
    """Contains routines to access program and data memory on a Kalimba"""

    def _declare_cfuncs(self):
        self._core._add_cfunc(self._cfuncs, 'ka_read_pm_block',  POINTER(ka_err), [c_void_p, c_uint, POINTER(c_uint), c_uint])
        self._core._add_cfunc(self._cfuncs, 'ka_write_pm_block', POINTER(ka_err), [c_void_p, c_uint, POINTER(c_uint), c_uint])
        self._core._add_cfunc(self._cfuncs, 'ka_read_dm_block',  POINTER(ka_err), [c_void_p, c_uint, POINTER(c_uint), c_uint])
        self._core._add_cfunc(self._cfuncs, 'ka_write_dm_block', POINTER(ka_err), [c_void_p, c_uint, POINTER(c_uint), c_uint])

    def __init__(self, core):
        self._core = core
        self._cfuncs = {}
        self._declare_cfuncs()

    def read_pm_block(self, start_addr, num_words):
        """Reads the specified number of whole words from program memory. Start address must be word
        aligned on byte addressable platforms"""
        result = (c_uint * num_words)()
        err = self._cfuncs['ka_read_pm_block'](self._core._get_ka(), start_addr, result, num_words)
        self._core._handle_error(err)
        return map(lambda x: x, result)

    def write_pm_block(self, start_addr, data):
        """Writes the specified data to program memory. 'start_addr' must be word
        aligned on byte addressable platforms. 'data' is a list of integer values to write"""
        data_array = to_ctypes_array(data, c_uint)
        err = self._cfuncs['ka_write_pm_block'](self._core._get_ka(), start_addr, data_array, len(data))
        self._core._handle_error(err)

    def read_dm_block(self, start_addr, num_words):
        """Reads the specified number of whole words from data memory. Start address must be word
        aligned on byte addressable platforms"""
        result = (c_uint * num_words)()
        err = self._cfuncs['ka_read_dm_block'](self._core._get_ka(), start_addr, result, num_words)
        self._core._handle_error(err)
        return map(lambda x: x, result)

    def write_dm_block(self, start_addr, data):
        """Writes the specified data to data memory. 'start_addr' must be word
        aligned on byte addressable platforms. 'data' is a list of integer values to write"""
        data_array = to_ctypes_array(data, c_uint)
        err = self._cfuncs['ka_write_dm_block'](self._core._get_ka(), start_addr, data_array, len(data))
        self._core._handle_error(err)
