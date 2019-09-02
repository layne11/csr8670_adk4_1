# Copyright (c) 2014 - 2016 Qualcomm Technologies International, Ltd.
# All Rights Reserved.
# Qualcomm Technologies International, Ltd. Confidential and Proprietary.
# Part of BlueLab-7.1-Release
# Part of the Python bindings for the kalaccess library.

from ctypes import c_void_p, c_uint, c_ushort, POINTER
from ka_ctypes import ka_err


class KaFw:
    def __init__(self, core):
        self._core = core
        self._cfuncs = {}
        self._core._add_cfunc(self._cfuncs, 'ka_read_onchip_signature'             , POINTER(ka_err), [c_void_p, POINTER(c_uint)])
        self._core._add_cfunc(self._cfuncs, 'ka_hal_get_interlock_host_status_addr', POINTER(ka_err), [c_void_p, POINTER(c_ushort)])
        self._core._add_cfunc(self._cfuncs, 'ka_hal_get_interlock_fw_status_addr'  , POINTER(ka_err), [c_void_p, POINTER(c_ushort)])
        self._core._add_cfunc(self._cfuncs, 'ka_hal_get_interlock_host_status'     , POINTER(ka_err), [c_void_p, POINTER(c_ushort)])
        self._core._add_cfunc(self._cfuncs, 'ka_hal_get_interlock_fw_status'       , POINTER(ka_err), [c_void_p, POINTER(c_ushort)])

    def read_onchip_signature(self):
        result = c_uint()
        err = self._cfuncs['ka_read_onchip_signature'](self._core._get_ka(), result)
        self._core._handle_error(err)
        return result.value

    def get_interlock_host_status_addr(self):
        result = c_ushort()
        err = self._cfuncs['ka_hal_get_interlock_host_status_addr'](self._core._get_ka(), result)
        self._core._handle_error(err)
        return result.value

    def get_interlock_fw_status_addr(self):
        result = c_ushort()
        err = self._cfuncs['ka_hal_get_interlock_fw_status_addr'](self._core._get_ka(), result)
        self._core._handle_error(err)
        return result.value

    def get_interlock_host_status(self):
        result = c_ushort()
        err = self._cfuncs['ka_hal_get_interlock_host_status'](self._core._get_ka(), result)
        self._core._handle_error(err)
        return result.value

    def get_interlock_fw_status(self):
        result = c_ushort()
        err = self._cfuncs['ka_hal_get_interlock_fw_status'](self._core._get_ka(), result)
        self._core._handle_error(err)
        return result.value
