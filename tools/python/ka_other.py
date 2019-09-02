# Copyright (c) 2014 - 2016 Qualcomm Technologies International, Ltd.
# All Rights Reserved.
# Qualcomm Technologies International, Ltd. Confidential and Proprietary.
# Part of BlueLab-7.1-Release
# Part of the Python bindings for the kalaccess library.

from ctypes import c_uint, c_void_p, POINTER
from ka_ctypes import ka_err


class KaDspState(object):

    def __init__(self, raw_state):
        self.raw_state = raw_state
        
    def __repr__(self):
        state = ["DSP state: %#x" % self.raw_state]
        if self.is_running():
            state.append("Running")
        if self.is_on_pm_break():
            state.append("PM breakpoint")
        if self.is_on_dm_break():
            state.append("DM breakpoint")
        if self.is_on_instruction_break():
            state.append("Instruction break")
        if self.is_on_exception_break():
            state.append("Exception break")
        if self.is_on_external_break():
            state.append("External break")
        return "\n\t".join(state)
            
    def is_running(self):
        return self.raw_state & KaDspState.KAL_STATUS_RUNNING != 0
        
    def is_on_pm_break(self):
        return self.raw_state & KaDspState.KAL_STATUS_PM_BREAK != 0
        
    def is_on_dm_break(self):
        return self.raw_state & KaDspState.KAL_STATUS_DM_BREAK != 0
        
    def is_on_instruction_break(self):
        return self.raw_state & KaDspState.KAL_STATUS_INSTR_BREAK != 0
    
    def is_on_exception_break(self):
        return self.raw_state & KaDspState.KAL_STATUS_EXCEPTION != 0
        
    def is_on_external_break(self):
        return self.raw_state & KaDspState.KAL_STATUS_EXTERNAL_BREAK != 0
        
class KaOther:
    def __init__(self, core):
        self._core = core

        self._cfuncs = {}
        self._core._add_cfunc(self._cfuncs, 'ka_reset',             POINTER(ka_err), [c_void_p])
        self._core._add_cfunc(self._cfuncs, 'ka_hal_get_dsp_state', POINTER(ka_err), [c_void_p, POINTER(c_uint)])
        self._core._add_cfunc(self._cfuncs, 'ka_hal_pcprofile'    , POINTER(ka_err), [c_void_p, POINTER(c_uint), c_uint])
        
        KaDspState.KAL_STATUS_PM_BREAK       = self._core._extract_c_integer_constant("KAL_STATUS_PM_BREAK")
        KaDspState.KAL_STATUS_DM_BREAK       = self._core._extract_c_integer_constant("KAL_STATUS_DM_BREAK")
        KaDspState.KAL_STATUS_RUNNING        = self._core._extract_c_integer_constant("KAL_STATUS_RUNNING")
        KaDspState.KAL_STATUS_INSTR_BREAK    = self._core._extract_c_integer_constant("KAL_STATUS_INSTR_BREAK")
        KaDspState.KAL_STATUS_EXCEPTION      = self._core._extract_c_integer_constant("KAL_STATUS_EXCEPTION")
        KaDspState.KAL_STATUS_EXTERNAL_BREAK = self._core._extract_c_integer_constant("KAL_STATUS_EXTERNAL_BREAK")
            
    def reset(self):
        """Performs a reset by disabling the Kalimba, and then enabling it again."""
        err = self._cfuncs['ka_reset'](self._core._get_ka())
        self._core._handle_error(err)

    def get_dsp_state(self):
        """Reads the state of the Kalimba core. Returns a KaDspState object, which can be queried for various possible
        states."""
        result = c_uint()
        err = self._cfuncs['ka_hal_get_dsp_state'](self._core._get_ka(), result)
        self._core._handle_error(err)
        return KaDspState(result.value)

    def pcprofile(self, num_samples):
        """Reads Kalimba's program counter repeatedly, as fast as possible, until num_samples have
        been read. Returns the PC values as an array."""
        pc_samples = (c_uint * num_samples)()
        err = self._cfuncs['ka_hal_pcprofile'](self._core._get_ka(), pc_samples, num_samples)
        self._core._handle_error(err)
        return map(lambda x:x, pc_samples)