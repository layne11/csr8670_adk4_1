# Copyright (c) 2014 - 2016 Qualcomm Technologies International, Ltd.
# All Rights Reserved.
# Qualcomm Technologies International, Ltd. Confidential and Proprietary.
# Part of BlueLab-7.1-Release
# Part of the Python bindings for the kalaccess library.

from ctypes import c_int, c_void_p, c_char_p, c_uint, c_ushort, POINTER, byref
from ka_ctypes import ka_err, ka_connection_details
from itertools import groupby


class KaDevice(object):
    def __init__(self, transport_string, subsystem_id, processor_id, dongle_id):
        self.transport_string = transport_string
        self.subsystem_id     = subsystem_id
        self.processor_id     = processor_id
        self.dongle_id        = dongle_id
    
    def __repr__(self):
        return "Transport string: {0}\n\t   Subsystem id: {1}\n\t   Processor id: {2}".format(
            self.transport_string, self.subsystem_id, self.processor_id)


class KaDeviceList(object):
    def __init__(self, devices):
        self.devices = devices
        
    def __repr__(self):
        if len(self.devices) == 0:
            return "No connected Kalimbas"
        
        result = ["Connected Kalimbas:"]        
        sorted_devs = sorted(self.devices, key = lambda d: d.dongle_id)
        groups = groupby(sorted_devs, lambda d: d.dongle_id)
        for i, group in enumerate(groups):
            result.append("{0}. Debug dongle '{1}':".format(i + 1, group[0]))
            for j, device in enumerate(group[1]):
                result.append("\t%s. %r" % (chr(j + ord('a')), device))
        return "\n".join(result)


class KaTrans:
    def __init__(self, core):
        self._core = core
        self._cfuncs = {}
        self._core._add_cfunc(self._cfuncs, 'ka_trans_get_var'           , c_char_p       , [c_void_p, c_char_p])
        self._core._add_cfunc(self._cfuncs, 'ka_trans_set_var'           , None           , [c_void_p, c_char_p, c_char_p])
        self._core._add_cfunc(self._cfuncs, 'ka_trans_build_device_table', POINTER(ka_err), [POINTER(POINTER(ka_connection_details)), POINTER(c_int)])
        self._core._add_cfunc(self._cfuncs, 'ka_trans_free_device_table' , None           , [POINTER(ka_connection_details), c_int])
        self._core._add_cfunc(self._cfuncs, 'ka_hal_spi_read'            , POINTER(ka_err), [c_void_p, c_uint, POINTER(c_ushort)])
        self._core._add_cfunc(self._cfuncs, 'ka_hal_spi_write'           , POINTER(ka_err), [c_void_p, c_uint, c_ushort])

    def enumerate_transports(self):
        """Enumerates all connected Kalimba devices.
        
        Returns information in a KaDeviceList.
        """
        """Enumerates the connected Kalimba devices, returning the information in a KaDeviceList object."""
        table, count = self._trans_build_device_table()
        devices = [KaDevice(table[i].transport_string, table[i].subsys_id, 
                            table[i].processor_id,     table[i].dongle_id) for i in range(count)]
        self._trans_free_device_table(table, count)
        return KaDeviceList(devices)
        
    def trans_get_var(self, var):
        """Read a variable from the active pttransport SPI connection.
        Common variables are SPICLOCK, SPI_DELAY, SPI_DELAY_MODE, SPIMUL, SPIPORT."""
        return self._cfuncs['ka_trans_get_var'](self._core._get_ka(), var)

    def trans_set_var(self, var, val):
        """Set a variable on the active pttransport SPI connection.
        Common variables are SPICLOCK, SPI_DELAY, SPI_DELAY_MODE, SPIMUL, SPIPORT."""
        return self._cfuncs['ka_trans_set_var'](self._core._get_ka(), var, val)

    def _trans_build_device_table(self):
        con_details = POINTER(ka_connection_details)()
        count = c_int()
        err = self._cfuncs['ka_trans_build_device_table'](byref(con_details), byref(count))
        self._core._handle_error(err)
        return con_details, count.value

    def _trans_free_device_table(self, table, count):
        self._cfuncs['ka_trans_free_device_table'](table, count)

    def spi_read(self, addr):
        """Reads directly from the SPI map. You should NOT need to do this in normal circumstances."""
        data = c_ushort()
        err = self._cfuncs['ka_hal_spi_read'](self._core._get_ka(), addr, byref(data))
        self._core._handle_error(err)
        return data.value

    def spi_write(self, addr, data):
        """Reads directly from the SPI map. You should NOT need to do this in normal circumstances."""
        err = self._cfuncs['ka_hal_spi_write'](self._core._get_ka(), addr, data)
        self._core._handle_error(err)
