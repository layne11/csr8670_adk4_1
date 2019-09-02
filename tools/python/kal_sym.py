# Copyright (c) 2014 - 2016 Qualcomm Technologies International, Ltd.
# All Rights Reserved.
# Qualcomm Technologies International, Ltd. Confidential and Proprietary.
# Part of BlueLab-7.1-Release

import os
import kalelfreader_lib_wrappers
from kalelfreader_lib_wrappers import Ker
import types
import re
import sys
from glob import glob
import read_klo
from ctypes import create_string_buffer
from struct import pack_into, unpack_from

                                
class NoSymbolsError(Exception):
    def __str__(self):
        return "Symbol information not yet loaded. Call sym.load()"

class InvalidChoice(Exception):
    pass

class SymbolSearchError(Exception):
    def __init__(self, sym_type, sym_name, num_found):
        self.sym_type = sym_type
        self.sym_name = sym_name
        self.num_found = num_found

    def __str__(self):
        if self.num_found < 1:
            return "Could not find %s matching search string '%s'" % (self.sym_type, self.sym_name)
        else:
            return "Expected one %s matching search string '%s'. Found %d." % (self.sym_type, self.sym_name, self.num_found)

def toHex(data):
    """
    Returns a string representing an array/list of data in hexadecimal.
    """
    try:
        rv = []
        for i in data:
            rv.append("0x%x" % i)
        return " ".join(rv)
    except:
        return "0x%x" % data
 
def read_dm_string(addr, kal=None, dm_data_width=None, read_dm_block=None):
    if dm_data_width is None:
        if kal is None:
            raise Exception("Either kal or dm_data_width must not be None")
        dm_data_width = kal.sym.dm_data_width
    if read_dm_block is None:
        if kal is None:
            raise Exception("Either kal or read_dm_block must not be None")
    word_bytes = dm_data_width / 8
    a0 = addr & ~(word_bytes - 1)
    if read_dm_block:
        w = read_dm_block(a0, 1)[0]
    else:
        w = kal.dm[a0]
    return 'string at ' + toHex(a0) + ' word_bytes ' + ("%d" % word_bytes) + " w=" + toHex(w)
    
class Sym(object):
    """This class can be used to load a symbol file (either ELF or KLO). It then
    provides functions to query the details of constants, variables, code labels 
    and modules."""

    def __init__(self, core):
        self._core = core

    def assert_have_symbols(self):
        if not self.have_symbols():
            raise NoSymbolsError()

    def have_symbols(self):
        return self.__dict__.has_key('variables')

    def load(self, filename = ""):
        """
        Parses the symbol information in the specified KLO/ELF (.klo, .elf) file and stores it
        for the other Kalimba debug functions to make use of.
        If the specified filename is a directory, that directory is searched for KLO/ELF files.
        If no KLO/ELF file is specified, we search the current directory for one.
        """
        path = os.getcwd()
        if os.path.isdir(filename):
            print "Searching", filename, "for KLO/ELF files..."
            path = filename
            filename = ""

        if filename == "":
            # The caller didn't specify a file, so lets go fishing for one
            file_list = glob(os.path.join(path, "*.klo"))
            file_list.extend(glob(os.path.join(path, "*.elf")))
            file_index = 0
            
            if len(file_list) == 0:
                raise IOError("No KLO/ELF files present in this directory")
            elif len(file_list) > 1:
                print "Enter the number of the KLO/ELF file to load:\n"
                for index, file in enumerate(file_list):
                    print "%s. %s" % (index + 1, file)
                print "\n? ",
                try:
                    user_input = int(sys.stdin.readline().strip())
                    file_index = user_input - 1
                    filename = file_list[file_index] # Test validity of index
                except:
                    raise InvalidChoice
            filename = file_list[file_index]
        
        print "Loading", filename

        if not os.path.exists(filename):
            raise IOError("File (or path) not found '%s'" % filename)

        if filename[-4:] == ".klo":
            (self.constants, self.source_lines, self.variables, self.labels, self.static_dm, self.static_pm) = read_klo.load_klo_file(filename)
            self.dm_data_width = 24
            self.pm_data_width = 32
        elif filename[-4:] == ".elf" or filename[-2:] == ".o":
            ker = kalelfreader_lib_wrappers.Ker()
            (self.symfile_dsp_rev,  self.constants,     self.source_lines,
             self.variables,        self.labels,        self.static_dm,
             self.static_pm,        machine_id,         self.is_big_endian,
             self.addr_width,       self.dm_data_width, self.pm_data_width,
             self.types,            self.enums,         self.loadsym_reports,
             self.elf_sec_headers,  self.funcs, 
             self.dm_octet_addressing, self.pm_octet_addressing) = ker.loadelf(filename)
        else:
            raise IOError("Specified filename must end with .klo, .elf or .o.\n" +
                          "Alternatively, just specify the directory containing the symbol file and a search will be performed.")

        print "Symbols loaded"
            
    class FileLineAndPcResult(object):
        def __init__(self, pc, file, line):
            self.pc = pc
            self.file = file
            self.line = line
            
        def __repr__(self):
            return "Program address: %#x. File: %s. Line number: %d" % (self.pc, self.file, self.line)
    
    def fileline_to_pc(self, filename, line_num):
        """
        Searches for source lines that match the specified filename and line number.
          'filename' - A string to search for in filenames. Substring matches will be included. 
                       Regular expressions are supported. Pass "" to match all files.
          'line_num' - An integer line number. Passing -1 will match all lines.
        Returns a list of FileLineAndPcResult instances, containing program address, filename, and 
        line number information.
        """
        self.assert_have_symbols()
        
        matches = []
        for (pc, line_info) in self.source_lines.iteritems():
            if re.search(filename, line_info[1]) >= 0:
                if line_info[2] == line_num or line_num == -1:
                    matches.append(Sym.FileLineAndPcResult(pc, line_info[1], line_info[2]))
        
        if len(matches) == 0:
            raise Exception("Couldn't find any matches for '%s', %d" % (filename, line_num))
        return matches

    def modname(self, addr="pc"):
        """
        Returns the name of the module or function at the program address provided.
        If no address is specified, the value read from the PC is used.
        """
        self.assert_have_symbols()
        
        if isinstance(addr, str):
            if addr.lower() == "pc":
                addr = self._core.reg.read_register(addr)
            else:
                raise TypeError("address must be numerical, or 'pc'")
            
        if self.funcs:
            # We loaded an ELF file, therefore have proper function definitions
            for name, f in self.funcs.items():
                low_pc = f[2]
                high_pc = f[3]
                # "< high_pc" is correct: the high PC value is first location past the last
                # instruction of the subprogram, according to the DWARF spec.
                if addr >= low_pc and addr < high_pc:
                    return name
        else:
            # We loaded a KLO file, therefore have to use source_lines
            # Is this a valid address?
            if addr in self.source_lines:
                return self.source_lines[addr][0]
                
        raise Exception("Could not find a module matching the address %#x" % addr)
      
    def print_source_lines(self):
        """Prints all debug source line information. Columns correspond to 
        (program address, module/function name, file name, line number)."""
        for pc, line_info in self.source_lines.items():
            print "%08x %s %s %d" % (pc, line_info[0], line_info[1], line_info[2])
            
    def print_mems(self, mems, data_width):
        """Takes a dictionary representing some memory contents and pretty-prints it.
        Each dictionary key should be an address, and each value should be a word of data."""
        def clamp(x): return '.' if x < 32 or x > 127 else x
        resStr = "%%0%dX %%0%dX %%s" % (self.addr_width/4, data_width/4)
        for addr, word in sorted(mems.items()):
            c3 = clamp((word & 0xff000000) >> 24)
            c2 = clamp((word & 0x00ff0000) >> 16)
            c1 = clamp((word & 0x0000ff00) >> 8)
            c0 = clamp(word & 0x000000ff)
            val = "%c %c %c %c" % (c3, c2, c1, c0)
            if data_width == 24:
                val = val[2:]

            print resStr % (addr, word, val)

    def print_static_dm(self):
        self.print_mems(self.static_dm, self.dm_data_width)

    def print_static_pm(self):
        self.print_mems(self.static_pm, self.pm_data_width)

    def readval(self, name_pattern):
        """
        Searches registers, constants and variables for a match against the specified regular
        expression. Internally, this function calls varfind() and constfind(). See the docs
        for those functions for more information.
        Returns:
          a list of (name, [values], address) of all matches
          "address" is the address of the variable, or "const" if the match is a const, or
          "reg" if it is a register.
        """

        class readvalresults(list):
            """Simple sub class of the builtin list class used to store results
            from readval. The only addition is to extend the repr method to
            print results nicely."""
            def __repr__(self):
                "Display results nicely"
                res = ""
                for vals in self:
                    if type(vals[2]) == types.StringType:
                        if vals[2] == "val":
                            name = "Value (&%d - 0x%06X)" % (vals[0], vals[0])
                        elif vals[2] == "reg":
                            name = vals[0]
                        else:
                            # constant
                            name = vals[0] + " 'CONST'"
                    else:
                        name = "%s[%d] (&%d - 0x%06X)" % (vals[0], len(vals[1]), vals[2], vals[2])
                    res += "%-50s - " % name
                    res += ("\n"+" "*53).join("%7d (0x%06X)" % (i, i) for i in vals[1])
                    res += "\n"
                return res[:-1] # Remove newline and return
            def addval(self, name, value):
                self.addgeneric(name, value, "val")
            def addreg(self, name, value):
                self.addgeneric(name, value, "reg")
            def addconst(self, name, value):
                self.addgeneric(name, value, "const")
            def adddata(self, name, value, addr):
                self.addgeneric(name, value, addr)
            def addgeneric(self, name, value, typeAddr):
                if type(value) != types.ListType:
                    value = [value]
                self.append((name, value, typeAddr))

        rv = readvalresults()
        if type(name_pattern) in [types.IntType, types.LongType]:
            rv.addval(name_pattern, self._core.dm[name_pattern])
        else:
            # Is it a register?
            if self._core.reg.is_register(name_pattern):
                val = self._core.reg.read_register(self._core.reg.get_register_id(name_pattern))
                rv.addreg(name_pattern, val)

            # Is it a variable
            symbol_matches = self.varfind(name_pattern)
            if symbol_matches != None and len(symbol_matches) > 0:
                for i in symbol_matches:
                    vals = self._core._read_dm_block(i[2], i[1])
                    rv.adddata(i[0], vals, i[2])

            # Is it a const?
            symbol_matches = self.constfind(name_pattern)
            if symbol_matches != None and len(symbol_matches) > 0:
                for i in symbol_matches:
                    rv.addconst(i[0], i[1])

        rv.sort()
        return rv

    class FindResults(list):
        """Sub class of the builtin list class, used to contain results from
        searching symbols.

        Extra functionality provided by the class:
        - intelligent representation method that presents the search results
          in a legible format.
        - friendly attributes, e.g. 'foo.name', note these will return the
          relevant attribute for the first and only search result, if
          multiple results match then a SymbolSearchError is raised.
        """
        def __init__(self, search, kind, fields, *args):
            self.__dict__["_search"] = search
            self.__dict__["_kind"]   = kind
            self.__dict__["_fields"] = fields
            list.__init__(self, *args)
        def __repr__(self):
            format = "%-50s : %d - 0x%06X"
            return "\n".join(map(lambda (n,v): format % (n,v,v), self))
        def __getattr__(self, attr):
            if len(self) != 1:
                raise SymbolSearchError(self._kind, self._search, len(self))
            if self._fields.has_key(attr):
                return self[0][self._fields[attr]]
            raise AttributeError("%s has no attribute '%s'" % (str(self.__class__), attr))

    def constfind(self, name_pattern):
        """
        Searches for constants matching the specified regular expression pattern, returning
        a list of tuples, one for each match. The tuple is (name, value).


        Note: $ is treated specially. Normally $ in a regular expression
        tells the search to look for the end of a line. However, because global Kalimba
        variables look like "$foo", we escape the $ before passing it to the regular
        expression matching engine. The escaping only occurs if the $ is the first
        character in the search string.
        """
        class constfindresults(Sym.FindResults):
            """Used to contain results from searching the constants.

            Extra functionality provided by the class:
            - intelligent representation method that presents the search results
              in a legible format.
            - "name", and "value" attributes, note these will return the
              relevant attribute for the first and only search result, if
              multiple results match then a SymbolSearchError is raised.
            """
            def __init__(self, search, *args):
                Sym.FindResults.__init__(self, search, "constant", {"name":0, "value":1}, *args)

        self.assert_have_symbols()

        search_string = name_pattern.lower()
        results = constfindresults(search_string)
        if len(search_string) > 0 and search_string[0] == '$':
            search_string = "\\" + search_string

        for name, value in self.constants.items():
            if re.search(search_string, name.lower()) >= 0:
                results.append((name, value))

        results.sort()
        return results

    def labelfind(self, search_string):
        """
        Searches for labels matching the specified regular expression pattern, or for
        labels at the specified address. Returns a list of tuples, one for each match.
        The tuple is (name, address).


        Note: $ is treated specially in text searches. Normally $ in a regular expression
        tells the search to look for the end of a line. However, because global Kalimba
        variables look like "$foo", we escape the $ before passing it to the regular
        expression matching engine. The escaping only occurs if the $ is the first
        character in the search string.
        """
        class labelfindresults(Sym.FindResults):
            """Used to contain results from searching the labels.

            Extra functionality provided by the class:
            - intelligent representation method that presents the search results
              in a legible format.
            - "name", and "addr" attributes, note these will return the relevant
              attribute for the first and only search result, if multiple
              results match then a SymbolSearchError is raised.
            """
            def __init__(self, search, *args):
                Sym.FindResults.__init__(self, search, "label", {"name":0, "addr":1}, *args)

        if not self.__dict__.has_key("labels"):
            print "Label info not yet loaded. Call loadsym()"
            return

        results = labelfindresults(search_string)
        if type(search_string) in [types.IntType, types.LongType]:
            # Treat search string is an address
            addr = int(search_string)

            for i in self.labels:
                # i is (labelname, address)
                if i[1] == addr:
                    results.append(i)
        else:
            if len(search_string) > 0 and search_string[0] == '$':
                search_string = "\\" + search_string

            # Treat search string as a regexp to match against var name
            search_string = search_string.lower()
            for i in self.labels:
                # i is (labelname, address)
                if re.search(search_string, i[0].lower()) >= 0:
                    results.append(i)

        results.sort()
        return results

    def varfind(self, search_string):
        """
        Searches for variables matching the specified regular expression pattern, or for
        variables at the specified address. Returns a list of tuples, one for each match.
        The tuple is (name, length, address).

        Note: $ is treated specially in text searches. Normally $ in a regular expression
        tells the search to look for the end of a line. However, because global Kalimba
        variables look like "$foo", we escape the $ before passing it to the regular
        expression matching engine. The escaping only occurs if the $ is the first
        character in the search string.
        """
        class VarFindResults(Sym.FindResults):
            """Used to contain results from searching the variables.

            Named fields: "name", "size_in_addressable_units" and "addr".
            These are also accessible via list indices 0, 1, 2 (respectively).
            Note: these will return the relevant attribute for the first and only search result; if
            multiple results match then a SymbolSearchError will be raised.
            """
            def __init__(self, search, *args):
                Sym.FindResults.__init__(self, search, "variable",
                                         {"name": 0, "size_in_addressable_units": 1, "addr": 2}, *args)

            def __repr__(self):
                res = ""
                for val in self:
                    name = "%s[%d]" % (val[0], val[1])
                    res += "%-50s : &%d - 0x%06X\n" %(name, val[2], val[2])
                return res[:-1] # Remove newline and return

        self.assert_have_symbols()

        results = VarFindResults(search_string)
        if type(search_string) in [types.IntType, types.LongType]:
            # Treat search string as an address
            for i in self.variables.keys():
                offsetFromTargetAddr = search_string - self.variables[i][1]

                # Is target addr between the start and end addresses of this variable
                if offsetFromTargetAddr >= 0 and offsetFromTargetAddr < self.variables[i][0]:
                    results.append((i, self.variables[i][0], self.variables[i][1]))
        else:
            if len(search_string) > 0 and search_string[0] == '$':
                search_string = "\\" + search_string

            # Treat search string as a regexp to match against var name
            search_string = search_string.lower()
            for i in self.variables.keys():
                if re.search(search_string, i.lower()):
                    results.append((i, self.variables[i][0], self.variables[i][1]))

        results.sort()
        return results

    def writeval(self, name_pattern, val):
        "Writes the specified value into the specified variable/register"
        # Is it a register
        if self._core.reg.is_register(name_pattern):
            self._core.reg.write_register(self._core.reg.get_register_id(name_pattern), val)
        else:
            # Is it a variable
            symbol_addr = self.varfind(name_pattern).addr
            self._core._write_dm_block(symbol_addr, [val])

    def printvar(self, name):
        """
        Pretty-print a variable's contents, with support for complex types.

        We want to print complex data types so they look like the (struct)
        definitions: this means we have to store name information and recurse
        up the typedef chain until we find the form of each variable/member,
        so we can produce output such as:
          struct pos {
            int x = 3
            int y = 4
          }
        """
        class TypeDisplayer(object):
            def __init__(self, kal, enums, variables, types, dm_octet_addressing, dm_data_width):
                self._kal = kal
                self.enums  = enums
                self.variables = variables
                self.types = types
                self.dm_octet_addressing = dm_octet_addressing
                self.dm_data_width_octets = dm_data_width / 8
                self.pad = ''
                self.type_name = ''
                self.variable_name = ''
                self.named = False

            def _get_variable_subword_mode(self, type):
                # If necessary, wind back to the start of the enclosing word:
                offset_into_word = self.addr % self.dm_data_width_octets
                actual_read_addr = self.addr - offset_into_word
                # .. And see if we need to chop off any octets off the end:
                leftover_octets = (self.addr + type.size_in_addressable_units) % self.dm_data_width_octets
                # Determine how many words we need to read to cover everything, and read them:
                octets_to_read = offset_into_word + type.size_in_addressable_units + leftover_octets
                words_to_read = (octets_to_read + (self.dm_data_width_octets - 1)) / self.dm_data_width_octets
                values = self._kal._read_dm_block(actual_read_addr, words_to_read)
                
                # Chop off the octets which are not part of this variable, at the start and the end
                if offset_into_word != 0 or leftover_octets != 0:
                    octet_buffer = create_string_buffer(words_to_read * self.dm_data_width_octets)
                    pack_into('I' * words_to_read, octet_buffer, 0, *values)
                    
                    octet_buffer = octet_buffer[offset_into_word:]
                    if leftover_octets > 0:
                        octet_buffer = octet_buffer[:-leftover_octets]
                        
                    # Pad with zeros, so we can unpack a whole number of words
                    for i in range(offset_into_word + leftover_octets):
                        octet_buffer += '\x00'
                    
                    num_to_unpack = max(type.size_in_addressable_units, self.dm_data_width_octets) / self.dm_data_width_octets
                    unpacked = unpack_from('I' * num_to_unpack, octet_buffer)
                    values = list(unpacked)
                return values
                
            def recursive_print(self, type_id):
                self.level = self.level + 1
                if self.level > 15:
                    raise Exception("TypeDisplayer reached maximum recursion level")
                    
                type = self.types[type_id]
                assert(type_id == type.type_id) # Otherwise kalelfreader is not ordering types by ID, which is assumed elsewhere.

                if type.form == Ker.KerTypeInfo.FORM_BASE:
                    
                    if type.name == 'char':
                        addr = self._kal._read_dm_block(self.addr, 1)[0]
                        print "%s%s %s %s %s" % (self.pad, type.name if not self.named else "", 
                                                self.type_name, self.variable_name, read_dm_string(addr, kal = self._kal))
                    else:
                        if self.dm_octet_addressing:
                            values = self._get_variable_subword_mode(type)
                        else:
                            values = self._kal._read_dm_block(self.addr, type.size_in_addressable_units)
                            
                        # Convert to normal Python types, rather than ctypes types:
                        values = map(lambda v: int(v), values)
                        print "%s%s %s %s = %s" % (self.pad, type.name if not self.named else "",
                                                   self.type_name, self.variable_name, values)
                    self.addr += type.size_in_addressable_units

                elif type.form == Ker.KerTypeInfo.FORM_STRUCT or type.form == Ker.KerTypeInfo.FORM_UNION:
                    
                    fname = "struct" if type.form == Ker.KerTypeInfo.FORM_STRUCT else "union"
                    self.type_name = type.name + ' ' + self.type_name
                    print self.pad, fname, self.type_name, self.variable_name,
                    
                    if not self.pointer or not self.struct:
                        self.struct = True
                        print '{'
                        self.type_name = ''
                        self.pad = self.pad + '  '
                        for i in range(type.member_count):
                            self.variable_name = type.members[i].name
                            self.named = False
                            self.recursive_print(type.members[i].type_id)
                        self.pad = self.pad[:-2]
                        print self.pad, '}'
                    else:
                        print ''

                elif type.form == Ker.KerTypeInfo.FORM_ARRAY:
                
                    print "%s array %s, length %d" % (self.pad, self.variable_name, type.array_count)
                    self.variable_name = ''
                    for i in range(type.array_count):
                        print '[', i, ']',
                        self.recursive_print(type.array_type_id)

                elif type.form == Ker.KerTypeInfo.FORM_TYPEDEF:
                
                    if not self.named: self.type_name = self.type_name + type.name
                    self.named = True # don't add name info after a typedef
                    if type.ref_type_id != 0xffffffff: self.recursive_print(type.ref_type_id)

                elif type.form == Ker.KerTypeInfo.FORM_POINTER:
                
                    if type.size_in_addressable_units > 0 and type.ref_type_id != 0xffffffff:
                        # Dereference the pointer
                        pointee_address = self._kal._read_dm_block(self.addr, type.size_in_addressable_units)
                        self.addr = pointee_address[0]
                        if not self.named: self.type_name = self.type_name + '*'
                        self.pointer = True
                        self.recursive_print(type.ref_type_id)
                    else:
                        print self.pad,
                        if not self.named: print type.name,
                        print self.type_name, '*', self.variable_name

                elif type.form == Ker.KerTypeInfo.FORM_CONST:
                
                    if not self.named: self.type_name = 'const ' + self.type_name
                    if type.ref_type_id != 0xffffffff: self.recursive_print(type.ref_type_id)

                elif type.form == Ker.KerTypeInfo.FORM_VOLATILE:
                
                    if not self.named: self.type_name = 'volatile ' + self.type_name
                    if type.ref_type_id != 0xffffffff: self.recursive_print(type.ref_type_id)

                elif type.form == Ker.KerTypeInfo.FORM_ENUM:
                
                    vals = self._kal._read_dm_block(self.addr, 1)
                    val = vals[0]
                    enumval = ""
                    enumvals = self.enums[type.name]
                    for key in enumvals.keys():
                        if enumvals[key] == val:
                            enumval = key
                    print self.pad, 'enum ', self.type_name, self.variable_name, '=', val, (enumval)
                    self.addr += type.size_in_addressable_units

                self.type_name = ''
                self.variable_name = ''
                self.level = self.level - 1

            def print_variable(self, name):
                # Get the variable & data address.
                var = self.variables[name]
                self.level = 0
                self.variable_name = name
                self.addr = var[1]
                self.pointer = False
                self.struct = False
                type_id = var[2]
                self.recursive_print(type_id)

        sh = TypeDisplayer(self._core, self.enums, self.variables, self.types, self.dm_octet_addressing, self.dm_data_width)
        sh.print_variable(name)