# Copyright (c) 2014 - 2016 Qualcomm Technologies International, Ltd.
# All Rights Reserved.
# Qualcomm Technologies International, Ltd. Confidential and Proprietary.
# Part of BlueLab-7.1-Release
# Part of the Python bindings for the kalaccess library.

from ctypes import c_int, c_char_p, c_uint, c_byte, c_void_p


class KaArch:
    def __init__(self, core):
        self._core = core
        self._cfuncs = {}
        self._core._add_cfunc(self._cfuncs, 'ka_get_chip_rev'         , c_int   , [c_void_p])
        self._core._add_cfunc(self._cfuncs, 'ka_get_global_chip_version', c_uint, [c_void_p])
        self._core._add_cfunc(self._cfuncs, 'ka_get_chip_name'        , c_char_p, [c_void_p])
        self._core._add_cfunc(self._cfuncs, 'ka_get_arch'             , c_uint  , [c_void_p])
        self._core._add_cfunc(self._cfuncs, 'ka_get_arch_from_name'   , c_uint  , [c_char_p])
        self._core._add_cfunc(self._cfuncs, 'ka_get_address_width'    , c_uint  , [c_void_p])
        self._core._add_cfunc(self._cfuncs, 'ka_get_data_width'       , c_uint  , [c_void_p])
        self._core._add_cfunc(self._cfuncs, 'ka_dm_subword_addressing', c_byte  , [c_void_p])
        self._core._add_cfunc(self._cfuncs, 'ka_pm_subword_addressing', c_byte  , [c_void_p])

    def get_chip_rev(self):
        return self._cfuncs['ka_get_chip_rev'](self._core._get_ka())

    def get_global_chip_version(self):
        return self._cfuncs['ka_get_global_chip_version'](self._core._get_ka())
        
    def get_chip_name(self):
        """Returns the internal code name of the chip."""
        return self._cfuncs['ka_get_chip_name'](self._core._get_ka())

    def get_arch(self):
        """Returns the architecture number of the attached Kalimba core. 1 is BC5 family, 2 is BC7 family, etc."""
        return self._cfuncs['ka_get_arch'](self._core._get_ka())

    def get_arch_from_name(self, name):
        """Returns the architecture number of the Kalimba core, given either an internal or external chip name."""
        return self._cfuncs['ka_get_arch_from_name'](name)

    def get_address_width(self):
        """Returns the number of bits in an address for either DM or PM."""
        return self._cfuncs['ka_get_address_width'](self._core._get_ka())

    def get_data_width(self):
        """Returns number of bits in one word of data memory."""
        return self._cfuncs['ka_get_data_width'](self._core._get_ka())

    def dm_subword_addressing(self):
        """Returns True if DM on the attached chip is byte addressable, or False if DM is word addressable."""
        return 0 != self._cfuncs['ka_dm_subword_addressing'](self._core._get_ka())

    def pm_subword_addressing(self):
        """Returns True if PM on the attached chip is byte addressable, or False if PM is word addressable."""
        return 0 != self._cfuncs['ka_pm_subword_addressing'](self._core._get_ka())
    
    def pm_address_inc_per_word(self):
        """Returns the increment to a PM address from one word to the next."""
        # Assumes words in program memory are always four octets wide.
        return 4 if self.pm_subword_addressing() else 1
        
    def dm_address_inc_per_word(self):
        """Returns the increment to a DM address from one word to the next."""
        return (self.get_data_width() >> 3) if self.dm_subword_addressing() else 1
        
