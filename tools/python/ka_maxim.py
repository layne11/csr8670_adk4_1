# Copyright (c) 2014 - 2016 Qualcomm Technologies International, Ltd.
# All Rights Reserved.
# Qualcomm Technologies International, Ltd. Confidential and Proprietary.
# Part of BlueLab-7.1-Release
# Part of the Python bindings for the kalaccess library.

from ctypes import c_void_p, c_uint, c_int, c_byte, POINTER
from ka_ctypes import ka_err


class KaMaxim:
    def __init__(self, core):
        self._core = core
        self._cfuncs = {}
        self._core._add_cfunc(self._cfuncs, 'maxim_is_valid_pm_break_location'     , POINTER(ka_err), [c_void_p, c_uint, POINTER(c_int)])
        self._core._add_cfunc(self._cfuncs, 'maxim_is_prefix_instruction'          , c_byte         , [c_void_p, c_uint])
        self._core._add_cfunc(self._cfuncs, 'maxim_is_do_instruction'              , c_byte         , [c_void_p, c_uint])
        self._core._add_cfunc(self._cfuncs, 'maxim_is_call_instruction'            , c_byte         , [c_void_p, c_uint])
        self._core._add_cfunc(self._cfuncs, 'maxim_get_break_instruction'          , c_uint         , [c_void_p])
        self._core._add_cfunc(self._cfuncs, 'maxim_is_break_instruction'           , c_byte         , [c_void_p, c_uint])
        self._core._add_cfunc(self._cfuncs, 'maxim_is_main_mem_read_instruction'   , c_byte         , [c_void_p, c_uint])
        self._core._add_cfunc(self._cfuncs, 'maxim_is_indexed_mem_read_instruction', c_byte         , [c_void_p, c_uint])
        self._core._add_cfunc(self._cfuncs, 'maxim_is_jump_instruction'            , c_byte         , [c_void_p, c_uint])
        self._core._add_cfunc(self._cfuncs, 'maxim_is_rts_instruction'             , c_byte         , [c_void_p, c_uint])
        self._core._add_cfunc(self._cfuncs, 'maxim_is_rti_instruction'             , c_byte         , [c_void_p, c_uint])
        self._core._add_cfunc(self._cfuncs, 'maxim_will_call_be_taken'             , c_byte         , [c_void_p, c_uint, c_uint])

    def is_valid_pm_break_location(self, addr):
        result = c_int()
        err = self._cfuncs['maxim_is_valid_pm_break_location'](self._core._get_ka(), addr, result)
        self._core._handle_error(err)
        return bool(result.value)

    def is_prefix_instruction(self, instr):
        return bool(self._cfuncs['maxim_is_prefix_instruction'](self._core._get_ka(), instr))

    def is_do_instruction(self, instr):
        return bool(self._cfuncs['maxim_is_do_instruction'](self._core._get_ka(), instr))

    def is_call_instruction(self, instr):
        return bool(self._cfuncs['maxim_is_call_instruction'](self._core._get_ka(), instr))

    def get_break_instruction(self):
        return self._cfuncs['maxim_get_break_instruction'](self._core._get_ka())

    def is_break_instruction(self, instr):
        return bool(self._cfuncs['maxim_is_break_instruction'](self._core._get_ka(), instr))

    def is_main_mem_read_instruction(self, instr):
        return bool(self._cfuncs['maxim_is_main_mem_read_instruction'](self._core._get_ka(), instr))

    def is_indexed_mem_read_instruction(self, instr):
        return bool(self._cfuncs['maxim_is_indexed_mem_read_instruction'](self._core._get_ka(), instr))

    def is_jump_instruction(self, instr):
        return bool(self._cfuncs['maxim_is_jump_instruction'](self._core._get_ka(), instr))

    def is_rts_instruction(self, instr):
        return bool(self._cfuncs['maxim_is_rts_instruction'](self._core._get_ka(), instr))

    def is_rti_instruction(self, instr):
        return bool(self._cfuncs['maxim_is_rti_instruction'](self._core._get_ka(), instr))

    def will_call_be_taken(self, instr, flags):
        return bool(self._cfuncs['maxim_will_call_be_taken'](self._core._get_ka(), instr, flags))
