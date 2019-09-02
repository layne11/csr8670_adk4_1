# Copyright (c) 2014 - 2016 Qualcomm Technologies International, Ltd.
# All Rights Reserved.
# Qualcomm Technologies International, Ltd. Confidential and Proprietary.
# Part of BlueLab-7.1-Release
# Part of the Python bindings for the kalaccess library.

from ctypes import c_int, c_uint, c_byte, c_void_p, POINTER, byref
from ka_ctypes import ka_err


class KaExec:
    """Class implementing execution-control functions, such as run, pause, step"""
    
    def _declare_cfuncs(self):
        self._core._add_cfunc(self._cfuncs, 'ka_get_instruction_set_from_pc', POINTER(ka_err), [c_void_p, POINTER(c_int)])
        self._core._add_cfunc(self._cfuncs, 'ka_run',                 POINTER(ka_err), [c_void_p, c_int, POINTER(c_byte)])
        self._core._add_cfunc(self._cfuncs, 'ka_run_to',              POINTER(ka_err), [c_void_p, c_uint, c_int])
        self._core._add_cfunc(self._cfuncs, 'ka_pause',               POINTER(ka_err), [c_void_p])
        self._core._add_cfunc(self._cfuncs, 'ka_step',                POINTER(ka_err), [c_void_p, c_int])
        self._core._add_cfunc(self._cfuncs, 'ka_step_over',           POINTER(ka_err), [c_void_p, POINTER(c_byte), c_int])

    def __init__(self, core):
        self._core = core
        self._cfuncs = {}
        self._declare_cfuncs()

    def _get_instruction_set_from_pc(self):
        instruction_set = c_int()
        err = self._cfuncs['ka_get_instruction_set_from_pc'](self._core._get_ka(), byref(instruction_set))
        self._core._handle_error(err)
        return instruction_set

    def run(self):
        # We discard the following out param -- the only interesting information is whether there was an error.
        run_was_actually_called = c_byte()
        # The second parameter is deprecated.
        unused = 0
        err = self._cfuncs['ka_run'](self._core._get_ka(), unused, byref(run_was_actually_called))
        self._core._handle_error(err)

    def run_to(self, address):
        err = self._cfuncs['ka_run_to'](self._core._get_ka(), address, 0)
        self._core._handle_error(err)

    def pause(self):
        err = self._cfuncs['ka_pause'](self._core._get_ka())
        self._core._handle_error(err)

    def step(self):
        instruction_set = self._get_instruction_set_from_pc()
        err = self._cfuncs['ka_step'](self._core._get_ka(), instruction_set)
        self._core._handle_error(err)

    def step_over(self):
        instruction_set = self._get_instruction_set_from_pc()
        #We discard the following out param -- the only interesting information is whether there was an error.
        run_was_actually_called = c_byte()
        err = self._cfuncs['ka_step_over'](self._core._get_ka(), byref(run_was_actually_called), instruction_set)
        self._core._handle_error(err)
