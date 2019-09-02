# Copyright (c) 2014 - 2016 Qualcomm Technologies International, Ltd.
# All Rights Reserved.
# Qualcomm Technologies International, Ltd. Confidential and Proprietary.
# Part of BlueLab-7.1-Release
# Python bindings for the kalelfreader library.
# $Change: 2383075 $

from ctypes import *
import os
import sys
from contextlib import contextmanager


class NoFileLoadedError(Exception):
    def __str__(self):
        return "No ELF file is loaded"


class LibraryVersionMismatchError(Exception):
    pass


class KerLibraryError(Exception):
    def __init__(self, code, message):
        Exception.__init__(self, message)
        self.code = code


class KerErrorCouldntOpenFile(KerLibraryError):
    pass


class KerErrorNoSuchFunction(KerLibraryError):
    pass


class KerErrorNoSuchVariable(KerLibraryError):
    pass


class KerErrorInvalidVariable(KerLibraryError):
    pass


class KerErrorInvalidIndex(KerLibraryError):
    pass


class KerErrorIncorrectTableSize(KerLibraryError):
    pass


# Indices correspond to the values of the C enum 'KerErrorId'.
KER_ERROR_MAP = [
    None,  # KerErrorNoError
    KerErrorCouldntOpenFile,
    KerErrorNoSuchFunction,
    KerErrorNoSuchVariable,
    KerErrorInvalidVariable,
    KerErrorInvalidIndex,
    KerErrorIncorrectTableSize
]


# ctypes Structure-derived classes, corresponding to the C structures
# defined in kalelfreader_c_wrapper.h.
class KerError(Structure):
    _fields_ = [("code", c_int),
                ("message", c_char_p)]


class KerElfSectionHeader(Structure):
    _fields_ = [("name", c_char_p),
                ("addr", c_uint),
                ("loadAddr", c_uint),
                ("num_bytes", c_uint),
                ("type", c_int)]


class KerStatement(Structure):
    _fields_ = [("module", c_char_p),
                ("source_line", c_int),
                ("addr", c_uint),
                ("source_file", c_char_p)]


class KerMember(Structure):
    _fields_ = [("type_id", c_uint),
                ("name", c_char_p),
                ("offset", c_uint),
                ("bit_offset_from_offset", c_uint),
                ("bit_size", c_uint)]


# form: 0-1-2-3-4-5-6-7-8 : Base-Struct-Union-Array-Typedef-Pointer-Const-Volatile-Enum
# type_id: index of self in the type table
# member_count: non-zero for structs or unions (form=1 or 2)
# members: valid for structs or unions (form=1 or 2)
# array_count: valid for arrays (form=3)
# array_type_id: valid for arrays (form=3)
class KerType(Structure):
    _fields_ = [("form", c_uint),
                ("type_id", c_uint),
                ("name", c_char_p),
                ("size_in_addressable_units", c_uint),
                ("ref_type_id", c_uint),
                ("member_count", c_int),
                ("members", POINTER(KerMember)),
                ("array_count", c_int),
                ("array_type_id", c_uint)]


class KerVariable(Structure):
    _fields_ = [("name", c_char_p),
                ("size_in_addressable_units", c_uint),
                ("addr", c_uint),
                ("type_id", c_uint)]


class KerConstant(Structure):
    _fields_ = [("name", c_char_p),
                ("value", c_uint)]


class KerLabel(Structure):
    _fields_ = [("name", c_char_p),
                ("addr", c_uint)]


class KerData(Structure):
    _fields_ = [("bit_width", c_int),
                ("byte_addressing", c_int),
                ("start_addr", c_uint),
                ("num_bytes", c_uint),
                ("data", POINTER(c_uint))]


class KerArchitecture(Structure):
    _fields_ = [("dsp_rev", c_int),
                ("addr_width", c_int),
                ("dm_data_width", c_int),
                ("pm_data_width", c_int),
                ("dm_byte_addressing", c_int),
                ("pm_byte_addressing", c_int)]


class KerReport(Structure):
    _fields_ = [("severity", c_int),
                ("text", c_char_p)]


class KerEnum(Structure):
    _fields_ = [("name", c_char_p),
                ("value", c_uint)]


class KerFunction(Structure):
    _fields_ = [("function_name", c_char_p),
                ("source_file", c_char_p),
                ("line_num", c_int),
                ("low_pc", c_uint),
                ("high_pc", c_uint)]


@contextmanager
def scoped_ker(file_path):
    ker = Ker()
    try:
        ker.open_file(file_path)
        yield ker
    finally:
        ker.close_file()


class Ker:
    """Python wrapper for the kalelfreader library"""

    def _load_library(self):
        # Find the absolute path of this script
        filename = __file__
        mydir = os.path.abspath(os.path.dirname(filename))

        # Load the C++ library. Give a bit of support to try to ease troubleshooting of common problems.
        if sys.platform.startswith('linux'):
            try:
                self._ker_dll = cdll.LoadLibrary(os.path.join(mydir, "libkalelfreader.so"))
            except Exception as ex:
                message = ("Could not find or load libkalelfreader.so (or one of its dependencies).\n"
                           "If the library is present, check that the your Python installation type (32/64-bit) matches"
                           "the architecture of the kalelfreader shared library (e.g. via the 'file' command)."
                           "The LD_LIBRARY_PATH used for the search was:\n    ")
                message += "\n    ".join(os.environ.get('LD_LIBRARY_PATH', '').split(":"))
                message += "\n\nInner Python exception : %r" % ex
                raise OSError(message)

        elif sys.platform.startswith('cygwin') or sys.platform.startswith('win32'):
            # On Cygwin, path elements are separated by colons, but on win32 it's a semi-colon.
            path_element_separator = ":" if sys.platform.startswith('cygwin') else ";"
            
            # Add the absolute path of this script to the system path
            if os.environ.get("PATH", "").find(mydir) == -1:
                os.environ["PATH"] = mydir + path_element_separator + os.environ.get("PATH", "")

            try:
                self._ker_dll = cdll.LoadLibrary("kalelfreader.dll")
            except Exception as ex:
                message = ("Could not find or load kalelfreader.dll (or one of its dependencies).\n"
                           "If the library is present, check that the your Python installation type (32/64-bit) matches"
                           "the kalelfreader DLL (likely 32-bit).\n"
                           "Sometimes this error can be fixed by installing a Visual C++ Redistributable package.\n"
                           "The system PATH used for the search was:\n    ")
                message += "\n    ".join(os.environ.get('PATH', '').split(path_element_separator))
                message += "\n\nInner Python exception : %r" % ex
                raise OSError(message)
        else:
            raise OSError("Cannot load the kalelfreader library. The system '%s' you are using is not supported."
                          % sys.platform)

    def _declare_cfuncs(self):
        self._cfuncs = {}

        def gen_prototype(name, argtypes, restype):
            func = getattr(self._ker_dll, name, None)
            if func is None:
                raise LibraryVersionMismatchError(
                    "Function '{0}' not found in the loaded kalelfreader library. Check that the Python bindings and"
                    " the C++ library are from the same release.".format(name))

            func.argtypes = argtypes
            func.restype = restype
            self._cfuncs[name] = func

        gen_prototype('ker_get_version',           [],                            c_char_p)
        gen_prototype('ker_open_file',             [c_char_p, POINTER(c_void_p)], POINTER(KerError))
        gen_prototype('ker_close_file',            [c_void_p],                    None)
        gen_prototype('ker_free_error',            [POINTER(KerError)],           None)

        gen_prototype('ker_get_function_count',    [c_void_p], c_int)
        gen_prototype('ker_get_function_by_index', [c_void_p, c_int,    POINTER(KerFunction)], POINTER(KerError))
        gen_prototype('ker_get_function_by_name',  [c_void_p, c_char_p, POINTER(KerFunction)], POINTER(KerError))
        gen_prototype('ker_get_function_by_addr',  [c_void_p, c_uint,   POINTER(KerFunction)], POINTER(KerError))
        gen_prototype('ker_get_variable_by_name',  [c_void_p, c_char_p, POINTER(KerVariable)], POINTER(KerError))

        gen_prototype('ker_get_elf_section_headers', [c_void_p, POINTER(c_int), POINTER(POINTER(KerElfSectionHeader))],
                      POINTER(KerError))
        gen_prototype('ker_get_build_tool_versions',  [c_void_p, POINTER(c_int), POINTER(POINTER(c_char_p))],
                      POINTER(KerError))
        gen_prototype('ker_get_statements', [c_void_p, POINTER(c_int), POINTER(POINTER(KerStatement))],
                      POINTER(KerError))
        gen_prototype('ker_get_variables',  [c_void_p, POINTER(c_int), POINTER(POINTER(KerVariable))],
                      POINTER(KerError))
        gen_prototype('ker_get_constants',  [c_void_p, POINTER(c_int), POINTER(POINTER(KerConstant))],
                      POINTER(KerError))
        gen_prototype('ker_get_labels',     [c_void_p, POINTER(c_int), POINTER(POINTER(KerLabel))],
                      POINTER(KerError))
        gen_prototype('ker_get_dm_data',    [c_void_p, POINTER(c_int), POINTER(POINTER(KerData))],
                      POINTER(KerError))
        gen_prototype('ker_get_pm_data',    [c_void_p, POINTER(c_int), POINTER(POINTER(KerData))],
                      POINTER(KerError))
        gen_prototype('ker_get_types',      [c_void_p, POINTER(c_int), POINTER(POINTER(KerType))],
                      POINTER(KerError))
        gen_prototype('ker_get_reports',    [c_void_p, POINTER(c_int), POINTER(POINTER(KerReport))],
                      POINTER(KerError))
        gen_prototype('ker_get_enums',      [c_void_p, POINTER(c_int), POINTER(POINTER(KerEnum)),
                      POINTER(c_int), POINTER(POINTER(KerEnum))],
                      POINTER(KerError))

        gen_prototype('ker_get_dsp_revision', [c_void_p], c_int)
        gen_prototype('ker_get_machine_id',   [c_void_p], c_uint)
        gen_prototype('ker_is_big_endian',    [c_void_p], c_byte)
        gen_prototype('ker_get_architecture', [c_void_p, POINTER(KerArchitecture)], POINTER(KerError))
        gen_prototype('ker_is_overlapping_statements',  [c_void_p], c_byte)
        gen_prototype('ker_get_overlapping_statements', [c_void_p, POINTER(POINTER(KerStatement))], POINTER(KerError))

        gen_prototype('ker_free_elf_section_headers', [POINTER(KerElfSectionHeader)], None)
        gen_prototype('ker_free_build_tool_versions', [POINTER(c_char_p)],            None)
        gen_prototype('ker_free_statements',          [POINTER(KerStatement)],        None)
        gen_prototype('ker_free_variables',           [POINTER(KerVariable)],         None)
        gen_prototype('ker_free_constants',           [POINTER(KerConstant)],         None)
        gen_prototype('ker_free_labels',              [POINTER(KerLabel)],            None)
        gen_prototype('ker_free_dm_data',             [POINTER(KerData)],             None)
        gen_prototype('ker_free_pm_data',             [POINTER(KerData)],             None)
        gen_prototype('ker_free_types',               [POINTER(KerType)],             None)
        gen_prototype('ker_free_reports',             [POINTER(KerReport)],           None)
        gen_prototype('ker_free_enums',               [POINTER(KerEnum), POINTER(KerEnum)], None)

        gen_prototype('ker_get_not_in_function_count',     [c_void_p], c_int)
        gen_prototype('ker_get_duplicate_statement_count', [c_void_p], c_int)

    def __init__(self):
        # ker will be non-null if we have a symbol file open (by a successful call to open_file)
        self._ker = None
        self._load_library()
        self._declare_cfuncs()

    def _cfunc(self, name):
        try:
            return self._cfuncs[name]
        except KeyError:
            raise AssertionError("kalelfreader library function '{0}' not declared. Please report this as a bug.".
                                 format(name))

    def get_version(self):
        return self._cfunc('ker_get_version')()

    def _handle_ker_error(self, err):
        """Internal routine to convert kalelfreader's error structures into Python exceptions."""
        if err.contents.code != 0:
            message = err.contents.message
            code = err.contents.code
            self._free_error(err)
            # Try to raise a specific exception type if the code is mapped to one.
            try:
                ex_type = KER_ERROR_MAP[code]
            except IndexError:
                ex_type = KerLibraryError
            raise ex_type(code, message)
            
    def _free_error(self, ker_error):
        """Internal function used to free the error structures returned by kalelfreader in the
        event of an error."""
        return self._cfunc('ker_free_error')(ker_error)

    def get_function_count(self):
        return self._cfunc('ker_get_function_count')(self._get_ker())

    def get_variable_by_name(self, name):
        result = KerVariable()
        err = self._cfunc('ker_get_variable_by_name')(self._get_ker(), name, result)
        self._handle_ker_error(err)
        return result

    def get_dsp_revision(self):
        return self._cfunc('ker_get_dsp_revision')(self._get_ker())

    def get_machine_id(self):
        return self._cfunc('ker_get_machine_id')(self._get_ker())

    def is_big_endian(self):
        return self._cfunc('ker_is_big_endian')(self._get_ker())

    def is_overlapping_statements(self):
        return self._cfunc('ker_is_overlapping_statements')(self._get_ker())

    def get_overlapping_statements(self, kst):
        err = self._cfunc('ker_get_overlapping_statements')(self._get_ker(), kst)
        self._handle_ker_error(err)

    def _free_elf_section_headers(self, table):
        return self._cfunc('ker_free_elf_section_headers')(table)

    def _free_build_tool_versions(self, table):
        return self._cfunc('ker_free_build_tool_versions')(table)

    def _free_statements(self, table):
        return self._cfunc('ker_free_statements')(table)

    def _free_variables(self, table):
        return self._cfunc('ker_free_variables')(table)

    def _free_constants(self, table):
        return self._cfunc('ker_free_constants')(table)

    def _free_labels(self, table):
        return self._cfunc('ker_free_labels')(table)

    def _free_dm_data(self, table):
        return self._cfunc('ker_free_dm_data')(table)

    def _free_pm_data(self, table):
        return self._cfunc('ker_free_pm_data')(table)

    def _free_types(self, table):
        return self._cfunc('ker_free_types')(table)

    def _free_reports(self, table):
        return self._cfunc('ker_free_reports')(table)

    def _free_enums(self, table1, table2):
        return self._cfunc('ker_free_enums')(table1, table2)

    def get_not_in_function_count(self):
        return self._cfunc('ker_get_not_in_function_count')(self._get_ker())

    def get_duplicate_statement_count(self):
        return self._cfunc('ker_get_duplicate_statement_count')(self._get_ker())

    def _get_ker(self):
        if self._ker is None:
            raise NoFileLoadedError()
        return self._ker

    def open_file(self, path):
        self._ker = c_void_p()
        try:
            err = self._cfunc('ker_open_file')(path, byref(self._ker))
            self._handle_ker_error(err)
        except:
            self._ker = None
            raise

    def close_file(self):
        rv = self._cfunc('ker_close_file')(self._get_ker())
        self._ker = None
        return rv

    def get_function_by_index(self, index):
        func = KerFunction()
        err = self._cfunc('ker_get_function_by_index')(self._get_ker(), index, byref(func))
        self._handle_ker_error(err)
        return func

    def get_function_by_name(self, name):
        func = KerFunction()
        err = self._cfunc('ker_get_function_by_name')(self._get_ker(), name, byref(func))
        self._handle_ker_error(err)
        return func

    def get_function_by_addr(self, addr):
        func = KerFunction()
        err = self._cfunc('ker_get_function_by_addr')(self._get_ker(), addr, byref(func))
        self._handle_ker_error(err)
        return func

    class KerElfSectionHeaderInfo(object):
        """Represents a section header in an ELF file. The following fields are available:
        (name, address, load_address, num_bytes, type). 
        The type field can be one of the following constants defined in this class:
        ELF_SECTION_TYPE_UNKNOWN, ELF_SECTION_TYPE_MAXIM, ELF_SECTION_TYPE_MINIM, ELF_SECTION_TYPE_DATA.
        """
        ELF_SECTION_TYPE_UNKNOWN = 0
        ELF_SECTION_TYPE_MAXIM   = 1
        ELF_SECTION_TYPE_MINIM   = 2
        ELF_SECTION_TYPE_DATA    = 3
        
        def __init__(self, name, address, load_address, num_bytes, type):
            self.name         = name
            self.address      = address
            self.load_address = load_address
            self.num_bytes    = num_bytes
            self.type         = type
            
        def __repr__(self):
            result = ["KerElfSectionHeaderInfo, section name: %s" % self.name]
            result.append("address: %#x" % self.address)
            result.append("load address: %#x" % self.load_address)
            result.append("num_bytes: %d" % self.num_bytes)
            result.append("type: %d" % self.type)
            return "\n".join(result)
            
    def get_elf_section_headers(self):
        """Returns a dict of information about ELF sections. Each dict key is section name. 
        Each entry is an KerElfSectionHeaderInfo instance.
        """
        count = c_int()
        table = POINTER(KerElfSectionHeader)()
        err = self._cfunc('ker_get_elf_section_headers')(self._get_ker(), byref(count), byref(table))
        self._handle_ker_error(err)
        results = {}
        for i in range(count.value):
            results[table[i].name] = Ker.KerElfSectionHeaderInfo(table[i].name, table[i].addr, table[i].loadAddr,
                                                                 table[i].num_bytes, table[i].type)
        self._free_elf_section_headers(table)
        return results

    def get_build_tool_versions(self):
        """
        Returns a list of strings containing the version info for each of the tools that were
        used to build the ELF file.
        """
        count = c_int()
        table = POINTER(c_char_p)()
        err = self._cfunc('ker_get_build_tool_versions')(self._get_ker(), byref(count), byref(table))
        self._handle_ker_error(err)
        rv = []
        for i in range(count.value):
            rv.append(table[i])
        self._free_build_tool_versions(table)
        return rv

    def get_statements(self):
        """Returns a list of statements. Each statement is represented by the following tuple:
        (function name, address, filename, line number)
        """
        count = c_int()
        table = POINTER(KerStatement)()
        err = self._cfunc('ker_get_statements')(self._get_ker(), byref(count), byref(table))
        self._handle_ker_error(err)
        rv = {}
        for i in range(count.value):
            rv[table[i].addr] = (table[i].module, table[i].source_file, table[i].source_line)
        self._free_statements(table)
        return rv

    def get_variables(self):
        """Returns a list of variables. Each variable is represented by the following tuple:
        (name, size_in_addressable_units, address, type_id)
        """
        count = c_int()
        table = POINTER(KerVariable)()
        err = self._cfunc('ker_get_variables')(self._get_ker(), byref(count), byref(table))
        self._handle_ker_error(err)
        rv = {}
        for i in range(count.value):
            rv[table[i].name] = (table[i].size_in_addressable_units, table[i].addr, table[i].type_id)
        self._free_variables(table)
        return rv

    def get_constants(self):
        """Returns a list of constants. Each constant is represented by the following tuple:
        (name, value)
        """
        count = c_int()
        table = POINTER(KerConstant)()
        err = self._cfunc('ker_get_constants')(self._get_ker(), byref(count), byref(table))
        self._handle_ker_error(err)
        rv = {}
        for i in range(count.value):
            rv[table[i].name] = table[i].value
        self._free_constants(table)
        return rv

    def get_labels(self):
        """Returns a list of labels. Each label is represented by the following tuple:
        (name, address)
        """
        count = c_int()
        table = POINTER(KerLabel)()
        err = self._cfunc('ker_get_labels')(self._get_ker(), byref(count), byref(table))
        self._handle_ker_error(err)
        rv = []
        for i in range(count.value):
            s = (table[i].name, table[i].addr)
            rv.append(s)
        self._free_labels(table)
        return rv

    def get_architecture(self):
        """Returns a the following tuple:
        (dsp_rev, addr_width, dm_data_width, pm_data_width, dm_byte_addressing, pm_byte_addressing)
        """
        a = KerArchitecture()
        err = self._cfunc('ker_get_architecture')(self._get_ker(), byref(a))
        self._handle_ker_error(err)
        return a.dsp_rev, a.addr_width, a.dm_data_width, a.pm_data_width, a.dm_byte_addressing, a.pm_byte_addressing

    @staticmethod
    def _copy_and_pad_memdata(ker_data):
        # Copy all the whole 32-bit words.
        num_whole_words = ker_data.num_bytes / 4
        data            = ker_data.data[:num_whole_words]
        remainder       = ker_data.num_bytes % 4
        
        # Round up to nearest 32-bit word. The last word will contain zero padding if needed.
        # Avoid reading beyond the buffer provided, by treating the last bytes as ubytes, not uints.
        if remainder > 0:
            as_ubyte_p = cast(ker_data.data, POINTER(c_ubyte))
            partial_last_word = 0
            partial_last_word_offset = ker_data.num_bytes - remainder
            for i in range(remainder):
                partial_last_word |= (as_ubyte_p[partial_last_word_offset + i] << (i << 3))
            data.append(partial_last_word)
        return data
        
    def get_pm_data(self):
        """Returns a list of tuples. Each tuple corresponds to a contiguous block of data.
        The tuple fields are (bit_width, byte_addressing, start_addr, num_bytes, data)"""
        count = c_int()
        table = POINTER(KerData)()
        err = self._cfunc('ker_get_pm_data')(self._get_ker(), byref(count), byref(table))
        self._handle_ker_error(err)
        rv = []
        for i in range(count.value):
            data = self._copy_and_pad_memdata(table[i])
            s = (table[i].bit_width, table[i].byte_addressing == 1, table[i].start_addr, table[i].num_bytes, data)
            rv.append(s)
        self._free_pm_data(table)
        return rv

    def get_dm_data(self):
        """Returns a list of tuples. Each tuple corresponds to a contiguous block of data.
        The tuple fields are (bit_width, byte_addressing, start_addr, num_bytes, data)"""
        count = c_int()
        table = POINTER(KerData)()
        err = self._cfunc('ker_get_dm_data')(self._get_ker(), byref(count), byref(table))
        self._handle_ker_error(err)
        rv = []
        for i in range(count.value):
            data = self._copy_and_pad_memdata(table[i])
            s = (table[i].bit_width, table[i].byte_addressing == 1, table[i].start_addr, table[i].num_bytes, data)
            rv.append(s)
        self._free_dm_data(table)
        return rv
                
    class KerMemberInfo(object):
        """Wrapper type for a structure or union member. The fields are the same as the ctypes ("raw" C)
        type KerMember; this class adds pretty printing.

        type_id: The ID of the type of this member; this may be looked up in the full list of types.
        offset: The offset of a non-bitfield member into the parent struct, measured in addressable units.
        bit_size: Zero for non-bitfield members. For bitfield members, bit_size will be non-zero, and
                  the offset of the bitfield into the structure may be calculated from

                  bitfield_position (in bits) = offset (converted to bits) + bit_offset_from_offset

                  where bit_offset_from_offset is measured in bits.
        """
        def __init__(self, c_member_info):
            self.type_id                = c_member_info.type_id
            self.name                   = c_member_info.name
            self.offset                 = c_member_info.offset
            self.bit_offset_from_offset = c_member_info.bit_offset_from_offset
            self.bit_size               = c_member_info.bit_size

        def list_repr(self):
            result = ["KerMemberInfo"]
            result.append("name:                   %s" % self.name)
            result.append("type id:                %d" % self.type_id)
            result.append("offset (addr. units)    %s" % self.offset)
            result.append("bit_offset_from_offset: %s" % self.bit_offset_from_offset)
            result.append("bit_size:               %s" % self.bit_size)
            return result

        def __repr__(self):
            return "\n".join(self.list_repr())
            
    class KerTypeInfo(object):
        """Represents a type present in the debugging information in the ELF file. The following fields are available:
        (form, type_id, name, size_in_addressable_units, ref_type_id, member_count, members, array_count, array_type_id).
        Each member is represented by a KerMemberInfo object as an element of "members"
        """
        FORM_BASE     = 0
        FORM_STRUCT   = 1
        FORM_UNION    = 2
        FORM_ARRAY    = 3
        FORM_TYPEDEF  = 4
        FORM_POINTER  = 5
        FORM_CONST    = 6
        FORM_VOLATILE = 7
        FORM_ENUM     = 8
        
        FORM_TO_STRING = { 
            FORM_BASE : '0 (base)',
            FORM_STRUCT : '1 (struct)',
            FORM_UNION : '2 (union)',
            FORM_ARRAY : '3 (array)',
            FORM_TYPEDEF : '4 (typedef)',
            FORM_POINTER : '5 (pointer)',
            FORM_CONST : '6 (const)',
            FORM_VOLATILE : '7 (volatile)',
            FORM_ENUM : '8 (enum)'
        }
        
        def __init__(self, c_type_info):
            self.form          = c_type_info.form
            self.type_id       = c_type_info.type_id
            self.name          = c_type_info.name
            self.size_in_addressable_units = c_type_info.size_in_addressable_units
            self.ref_type_id   = c_type_info.ref_type_id
            self.member_count  = c_type_info.member_count  # Redundant, but keeping for now.
            self.members       = [Ker.KerMemberInfo(c_type_info.members[i]) for i in range(c_type_info.member_count)]
            self.array_count   = c_type_info.array_count
            self.array_type_id = c_type_info.array_type_id
        
        def __repr__(self):
            result = ["KerTypeInfo"]
            result.append("name:                     %s" % self.name)
            result.append("form:                     %s" % self.FORM_TO_STRING[self.form])
            result.append("type id:                  %d" % self.type_id)
            result.append("size (addressable units): %d" % self.size_in_addressable_units)
            result.append("reference type id:        %s" % ("<none>" if self.ref_type_id == 0xffffffff else self.ref_type_id))
            result.append("array count:              %d" % self.array_count)
            result.append("array type id:            %s" % ("<none>" if self.array_type_id == 0xffffffff else self.array_type_id))
            result.append("member count:             %d" % self.member_count)
            if self.member_count > 0:
                result.append("members:")
                for m in self.members:
                    indented = map(lambda line: "    " + line, m.list_repr())
                    result += indented

            return "\n".join(result)
            
    def get_types(self):
        """Returns a list of KerTypeInfo objects, each of which represent a type present in the 
        debugging information in the ELF file. kalelfreader returns the types ordered by type id.
        """
        count = c_int()
        table = POINTER(KerType)()
        err = self._cfunc('ker_get_types')(self._get_ker(), byref(count), byref(table))
        self._handle_ker_error(err)
        
        try:
            results = [Ker.KerTypeInfo(table[i]) for i in range(count.value)]
            return results
        finally:
            self._free_types(table)

    def get_reports(self):
        """Returns a list of reports. Each report has a severity (0-1-2 information-warning-error) and text
        (severity, text)
        """
        count = c_int()
        table = POINTER(KerReport)()
        err = self._cfunc('ker_get_reports')(self._get_ker(), byref(count), byref(table))
        self._handle_ker_error(err)
        rv = []
        for i in range(count.value):
            t = table[i]
            s = (t.severity, t.text)
            rv.append(s)
        self._free_reports(table)
        return rv

    def get_enums(self):
        """Returns a dict of enums. Each enum is itself a dict mapping name to value.
        """
        count1 = c_int()
        table1 = POINTER(KerEnum)()
        count2 = c_int()
        table2 = POINTER(KerEnum)()
        err = self._cfunc('ker_get_enums')(self._get_ker(), byref(count1), byref(table1), byref(count2), byref(table2))
        self._handle_ker_error(err)
        enums = {}
        for i1 in range(count1.value):
            record1 = table1[i1]
            enum_name = record1.name
            enum_offset = record1.value
            next_enum_offset = count2.value
            if i1 < count1.value - 1:
                next_enum_offset = table1[i1 + 1].value
            enum_vals = {}
            for j in range(enum_offset, next_enum_offset):
                record2 = table2[j]
                enum_vals[record2.name] = record2.value
            enums[enum_name] = enum_vals
        self._free_enums(table1, table2)
        return enums

    def loadelf(self, filename):
        """This function returns:
            dsp_rev         int    DSP revision
            constants       dict   (value) by name
            source_lines    dict   (function_name, source_file, line_num) by address
            variables       dict   (size, address, type_id) by name
            labels          list   (name, address)
            dm_data         dict   (value) of static DM by address
            pm_data         dict   (value) of static PM by address
            machine_id      int    machine ID from the elf header e_machine field.
            is_big_endian   int    1 if the architecture is big-endian (most significant byte at lowest memory address)
            addr_width      int    address width in bits for either PM or DM
            dm_data_width   int    bits in one word of DM
            pm_data_width   int    bits in one word of PM
            types           list   (form, type_id, name, byte_size, ref_type_id, member_count, mbr_tids, mbr_names,
                                    array_count, array_type_id) by type_id
            reports         list   warning/error strings from elf file load
            enums           dict   each entry key is the enum name, value is a dict of enum values (keyed on name)
            elf_sec_hdrs    dict   (start_addr, num_bytes, type) by name
            funcs           dict   (source_file, line_num, low_pc, high_pc) by name
            dm_octet_addressing bool True if target's DM is octet addressable, False if not.
            pm_octet_addressing bool True if target's PM is octet addressable, False if not.
        """

        # Load and parse the elf file.
        self.open_file(filename)

        try:
            (dsp_rev, addr_width, dm_data_width, pm_data_width, dm_octet_addressing, pm_octet_addressing) = \
                self.get_architecture()
            
            constants     = self.get_constants()
            source_lines  = self.get_statements()
            variables     = self.get_variables()
            labels        = self.get_labels()
            dm_data       = {}
            pm_data       = {}
            machine_id    = self.get_machine_id()
            types         = self.get_types()
            enums         = self.get_enums()
            reports       = self.get_reports()
            is_big_endian = self.is_big_endian()
            elf_sec_hdrs  = self.get_elf_section_headers()

            # Retrieve the DM data.
            for (bit_width, byte_addressing, start_addr, num_bytes, data) in self.get_dm_data():
                # We store each word as 4 bytes in the Elf file, regardless of whether it is 32-bit, 24-bit or 16-bit
                entries = (num_bytes + 3) / 4
                inc = 1
                if byte_addressing != 0:
                    inc = 4
                for j in range(entries):
                    dm_data[start_addr + j * inc] = data[j]

            # Retrieve the PM data.
            for (bit_width, byte_addressing, start_addr, num_bytes, data) in self.get_pm_data():
                # We store each word as 4 bytes in the Elf file, regardless of whether it is 32-bit, 24-bit or 16-bit
                entries = (num_bytes + 3) / 4
                inc = 1
                if byte_addressing != 0:
                    inc = 4
                for j in range(entries):
                    pm_data[start_addr + j * inc] = data[j]

            # Retrieve functions.
            funcs = {}
            for i in xrange(self.get_function_count()):
                f = self.get_function_by_index(i + 1)
                funcs[f.function_name] = (f.source_file, f.line_num, f.low_pc, f.high_pc)

            # Warn on overlapping statements.
            if self.is_overlapping_statements():
                print 'warning: possible overlapping pm regions, eg:'
                table = POINTER(KerStatement)()
                err = self._cfunc('ker_get_overlapping_statements')(self._get_ker(), byref(table))
                self._handle_ker_error(err)
                for i in range(2):
                    print '  module:%s PC:%08x source_line:%d source_file:%s' % \
                          (table[i].module, table[i].addr, table[i].source_line, table[i].source_file)
                self._free_statements(table)

        finally:
            # Free the parsed elf file.
            self.close_file()

        # Return the ELF file data.
        return (dsp_rev, constants, source_lines, variables, labels,
                dm_data, pm_data, machine_id, is_big_endian, addr_width,
                dm_data_width, pm_data_width, types, enums, reports,
                elf_sec_hdrs, funcs, dm_octet_addressing, pm_octet_addressing)