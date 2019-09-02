# Copyright (c) 2014 - 2016 Qualcomm Technologies International, Ltd.
# All Rights Reserved.
# Qualcomm Technologies International, Ltd. Confidential and Proprietary.
# Part of BlueLab-7.1-Release
# Part of the Python bindings for the kalaccess library.
# Python definitions matching C structures we need from kalaccess

from ctypes import c_int, c_char_p, c_uint, Structure


class ka_err(Structure):
    _fields_ = [("err_code", c_int),
                ("err_string", c_char_p)]


class ka_connection_details(Structure):
    _fields_ = [("transport_string", c_char_p),
                ("subsys_id", c_int),
                ("processor_id", c_uint),
                ("dongle_id", c_char_p)]


def to_ctypes_array(enumerable, c_types_type):
    transformed = (c_types_type * len(enumerable))()
    for index, value in enumerate(enumerable):
        transformed[index] = value
    return transformed
