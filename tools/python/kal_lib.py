# Copyright (c) 2014 - 2016 Qualcomm Technologies International, Ltd.
# All Rights Reserved.
# Qualcomm Technologies International, Ltd. Confidential and Proprietary.
# Part of BlueLab-7.1-Release

import ka_exceptions
import math


class MissingTimerSymbolError(Exception):
    def __init__(self, symbol_name):
        Exception.__init__(self, "Symbol required for timers not found: {0}".format(symbol_name))


class Lib(object):
    """This class provides functions to inspect on-chip data structures used by
    the Kalimba standard library code."""
    def __init__(self, core):
        self._core = core

    def _read_var_with_size_check(self, address, size_in_addressable_units):
        # Check that the variable is a whole number of words. Otherwise, something's wrong.
        if size_in_addressable_units % self._core.arch.dm_address_inc_per_word() != 0:
            raise Exception("Symbol has size {0} octets, which is not a whole number of words"
                            .format(size_in_addressable_units))

        size_in_words = size_in_addressable_units / self._core.arch.dm_address_inc_per_word()
        return self._core._read_dm_block(address, size_in_words)

    def _format_address_for_arch(self, address):
        # Output at a fixed width, appropriate for the target arch, e.g. 6(8) hex digits for 24(32)-bit data width.
        return "0x{addr:0{width}X}".format(addr=address, width=2*self._core.arch.get_data_width() / 8)
        
    def _print_timer_results(self, timers, time_field_offset, handler_field_offset):
        # Produce a printable set of data, formatted with the right column widths and whatnot.
        longest_timer_len   = 0
        longest_handler_len = 0
        prev_expiry_time    = 0
        printable_info      = []
        for (timer_address, timer_memory) in timers:
            timer_symbol        = self._core.sym.varfind(timer_address)
            timer_name          = timer_symbol.name if len(timer_symbol) > 0 else "<unknown>"
            timer_name_and_addr = "{0} ({1})".format(timer_name, self._format_address_for_arch(timer_address))
            longest_timer_len   = max(longest_timer_len, len(timer_name_and_addr))

            handler_address       = timer_memory[handler_field_offset]
            handler_name_and_addr = "{0} ({1})".format(self._core.sym.modname(handler_address),
                                                       self._format_address_for_arch(handler_address))
            longest_handler_len   = max(longest_handler_len, len(handler_name_and_addr))

            expiry_time      = timer_memory[time_field_offset]
            expiry_offset    = expiry_time - prev_expiry_time
            prev_expiry_time = expiry_time

            printable_info.append((timer_name_and_addr, handler_name_and_addr, expiry_offset))

        if len(printable_info) == 0:
            print "No timers found"
        else:
            print "%-*s %-*s Offset(us) [first absolute]" % \
                  (longest_timer_len, "Timer Structure", longest_handler_len, "Handler")
            for timer_name_and_addr, handler_name_and_addr, expiry_offset in printable_info:
                print "%-*s %-*s %d" % \
                      (longest_timer_len, timer_name_and_addr, longest_handler_len, handler_name_and_addr, expiry_offset)

    def timers(self):
        """Reports the status of timers registered with the Kalimba standard library."""

        # Strategy is to grab all the data in one go, then print it afterwards.
        def get_const_field(name):
            symbol = self._core.sym.constfind(name)
            if not symbol:
                raise MissingTimerSymbolError(name)
            return symbol.value

        # We need the offsets for various structure fields
        next_field_offset    = get_const_field("$timer.NEXT_ADDR_FIELD")
        time_field_offset    = get_const_field("$timer.TIME_FIELD")
        handler_field_offset = get_const_field("$timer.HANDLER_ADDR_FIELD")
        timer_structure_size = get_const_field("$timer.STRUC_SIZE")

        # Get the address of the timer at the head of the linked list.
        head_timer_symbol = self._core.sym.varfind("$timer.last_addr")
        if len(head_timer_symbol) == 0:
            raise MissingTimerSymbolError("$timer.last_addr")

        # If the processor is running, pause it while we collect data.
        was_running = self._core.is_running()
        if was_running:
            self._core.pause()

        # Read each timer structure in the linked list of timers
        current_timer_address = self._core.dm[head_timer_symbol.addr]
        timers = []
        end_marker = 2**self._core.arch.get_data_width() - 1
        while current_timer_address != end_marker:
            timer_struct_memory = self._core._read_dm_block(current_timer_address, timer_structure_size)
            timers.append((current_timer_address, timer_struct_memory))
            current_timer_address = timer_struct_memory[next_field_offset]

        if was_running:
            self._core.run()

        self._print_timer_results(timers, time_field_offset, handler_field_offset)

    def readcbuffer(self, cbuffer_name, n=0, do_ud='noupdate'):
        """
        Returns the contents of a cbuffer on the Kalimba.
           cbuffer_name can be a regular expression, but must end with 'cbuffer_struc'.
           n specifies the maximum number of words to read.
           Set do_ud to 'update' if the read address should be updated by this function (the default
           is 'noupdate').
        """
        if not self._core.is_connected():
            raise ka_exceptions.NotConnectedError()

        if not cbuffer_name.endswith("cbuffer_struc"):
            print "cbuffer_name must end with 'cbuffer_struc'"
            return

        cbuff_list = self._core.sym.varfind(cbuffer_name)
        if len(cbuff_list) == 0:
            print "No such cbuffer found"
            return

        if len(cbuff_list) > 1:
            print "More than one cbuffer found with that name. Match must be unique."
            print "Matching names were:"
            for i in cbuff_list:
                print "   ", i[0]
            return

        cbuff_tuple = cbuff_list[0]
        cbuff_addr = cbuff_tuple[2] # Fields are name, size, addr

        (sz, rd, wr) = self._core._read_dm_block(cbuff_addr, 3)

        if sz != 0:
            # start and end addresses
            mask = int(2**math.ceil(math.log(sz, 2)) - 1)
            start_addr = rd - (rd & mask)
            end_addr = start_addr + sz - 1

            # available data in buffer
            amount_data = wr - rd
            if amount_data < 0:
                amount_data += sz

            if amount_data == 0:
                return []
            else:
                if n > 0:
                    if n > amount_data:
                        n = amount_data
                        print "Warning: Not enough data in the cbuffer"
                else:
                    n = amount_data

                # end_addr is index of last item of buf
                if (rd + n - 1) > end_addr:
                    data = self._core._read_dm_block(rd, end_addr - rd + 1)
                    m = len(data)
                    data.append(self._core._read_dm_block(start_addr, m))
                    rd = start_addr + m
                else:
                    data = self._core._read_dm_block(rd, n)
                    rd += n

                if do_ud.lower() == 'update':
                    self._core._write_dm_block(cbuff_addr+1, [rd])

                return data
        else:
            print "The size of the cbuffer is 0."
            return []

    def writecbuffer(self, cbuffer_name, data, do_ud='update'):
        """
        Writes data to the specified cbuffer. Returns the left over data after
        writing as much as it can.
           cbuffer_name can be a regular expression, but must end with 'cbuffer_struc'.
           data is a list that contains the data to be written.
           Set do_ud to 'noupdate' if the write address should not be updated (the default is
           'update').
        """
        if not self._core.is_connected():
            raise ka_exceptions.NotConnectedError()

        if not cbuffer_name.endswith("cbuffer_struc"):
            print "cbuffer_name must end with 'cbuffer_struc'"
            return

        cbuff_list = self._core.sym.varfind(cbuffer_name)
        if len(cbuff_list) == 0:
            print "No such cbuffer found"
            return

        if len(cbuff_list) > 1:
            print "More than one cbuffer found with that name. Match must be unique."
            print "Matching names were:"
            for i in cbuff_list:
                print "   ", i[0]
            return

        cbuff_tuple = cbuff_list[0]
        cbuff_addr = cbuff_tuple[2] # Fields are name, size, addr

        (sz, rd, wr) = self._core._read_dm_block(cbuff_addr, 3)

        if sz != 0:
            # start and end addresses
            mask = int(2**math.ceil(math.log(sz, 2)) - 1)
            start_addr = rd - (rd & mask)
            end_addr = start_addr + sz - 1

            # available data in buffer
            amount_data = wr - rd
            if amount_data < 0:
                amount_data += sz
            amount_space = sz - amount_data - 1

            if amount_space == 0:
                return data
            else:
                n = min(len(data), amount_space)

                # end_addr is index of last item of buf
                if (wr + n - 1) >= end_addr:
                    self._core._write_dm_block(wr, data[0:(end_addr - wr + 1)])
                    written_size = (end_addr - wr + 1)
                    if written_size<n:
                        self._core._write_dm_block(start_addr, data[written_size:n])
                    wr = start_addr + n - written_size
                else:
                    self._core._write_dm_block(wr, data[0:n])
                    wr += n

                data = data[n:]
                if do_ud.lower() == 'update':
                    self._core._write_dm_block(cbuff_addr+2, [wr])

                return data
        else:
            print "The size of the cbuffer is 0."
            return data

    def cbuffers(self):
        """
        Returns the status of cbuffers on the Kalimba.
        Considers only structures following the naming convention '*cbuffer_struc'.
        """
        self._core.sym.assert_have_symbols()

        cbuffer_name_key = "cbuffer_struc$"
        cbuffer_name_key_len = len(cbuffer_name_key)
        cbuffers = self._core.sym.varfind(cbuffer_name_key)
        cbuffers.sort()

        print " " * 30 + "C-Buffer  Size  Read Addr  Write Addr      Start   Space   Data"

        # The results from varfind are unfortunately tuples -- one simply has to know the correct indices.
        for (struct_name, size_in_addressable_units, struct_address) in cbuffers:
            # Order of fields in the structure: size, read pointer, write pointer.
            (sz, rd, wr) = self._read_var_with_size_check(struct_address, size_in_addressable_units)

            struct_name = struct_name[:-cbuffer_name_key_len]
            print "%38s" % struct_name,

            if sz != 0:
                # Find start address of the buffer
                mask = int(2**math.ceil(math.log(sz, 2)) - 1)
                start_addr = rd - (rd & mask)
                if start_addr != (wr - (wr & mask)):
                    print '\n\n** Warning the start address calculated from the read',
                    print 'and write pointers is different! **'

                amount_space = rd - wr
                if amount_space <= 0:
                    amount_space += sz
                amount_space -= 1
                amount_data = wr - rd
                if amount_data < 0:
                    amount_data += sz

                print ' %4d   0x%06x    0x%06x   0x%06x    %4d   %4d' % \
                       (sz,   rd,       wr,     start_addr, amount_space, amount_data)
            else:
                print "size=0"

    def ports(self):
        """
        Displays information about each DSP port including format and amount of data/space.
        """
        # check connected to chip
        if not self._core.is_connected():
            raise ka_exceptions.NotConnectedError()

        # check symbols are loaded
        self._core.sym.assert_have_symbols()

        # get the symbols we need
        READ_OFFSET_ADDR  = self._core.sym.varfind('$cbuffer.read_port_offset_addr')
        WRITE_OFFSET_ADDR = self._core.sym.varfind('$cbuffer.write_port_offset_addr')
        READ_LIMIT_ADDR   = self._core.sym.varfind('$cbuffer.read_port_limit_addr')
        WRITE_LIMIT_ADDR  = self._core.sym.varfind('$cbuffer.write_port_limit_addr')
        READ_BUFFER_SIZE  = self._core.sym.varfind('$cbuffer.read_port_buffer_size')
        WRITE_BUFFER_SIZE = self._core.sym.varfind('$cbuffer.write_port_buffer_size')

        def read_dm(addr):
            return self._core.dm[addr]

        # get the read and write offset
        read_offset_addr  = self._read_var_with_size_check(READ_OFFSET_ADDR.addr, READ_OFFSET_ADDR.size_in_addressable_units)
        write_offset_addr = self._read_var_with_size_check(WRITE_OFFSET_ADDR.addr, WRITE_OFFSET_ADDR.size_in_addressable_units)
        read_offset       = map(read_dm, read_offset_addr)
        write_offset      = map(read_dm, write_offset_addr)

        # get the read and write limit
        read_limit_addr   = self._read_var_with_size_check(READ_LIMIT_ADDR.addr, READ_LIMIT_ADDR.size_in_addressable_units)
        write_limit_addr  = self._read_var_with_size_check(WRITE_LIMIT_ADDR.addr, WRITE_LIMIT_ADDR.size_in_addressable_units)
        read_limit        = map(read_dm, read_limit_addr)
        write_limit       = map(read_dm, write_limit_addr)

        # get the port size
        read_size         = self._read_var_with_size_check(READ_BUFFER_SIZE.addr, READ_BUFFER_SIZE.size_in_addressable_units)
        write_size        = self._read_var_with_size_check(WRITE_BUFFER_SIZE.addr, WRITE_BUFFER_SIZE.size_in_addressable_units)
        # calculate size mask (size-1) for non-zero sizes
        read_mask         = map(lambda s: s - (s>0), read_size)
        write_mask        = map(lambda s: s - (s>0), write_size)

        # calculate data/space in port
        read_data         = map(lambda l,o,m: (l - o) & m, read_limit, read_offset, read_mask)
        write_space       = map(lambda l,o,m: (l - o) & m - 1, write_limit, write_offset, write_mask)

        # read port configs
        READ_CONF_BASE   = self._core.sym.constfind('$READ_PORT0_CONFIG').value
        WRITE_CONF_BASE  = self._core.sym.constfind('$WRITE_PORT0_CONFIG').value
        read_conf        = self._read_var_with_size_check(READ_CONF_BASE,  READ_OFFSET_ADDR.size_in_addressable_units)
        write_conf       = self._read_var_with_size_check(WRITE_CONF_BASE, WRITE_OFFSET_ADDR.size_in_addressable_units)

        # extract data size (in octets) from config
        read_data_size   = map(lambda c: (c & 0x3) + 1, read_conf)
        write_space_size = map(lambda c: (c & 0x3) + 1, write_conf)

        # decode configs into strings
        read_conf_str  = map(lambda c,s: ("8" if s==1 else ("16" if s==2 else ("24" if s==3 else "??"))) + "-bit, " \
                                        + ("Big Endian" if (c & 0x4)  else "Little Endian") + ", " \
                                        + ("No Sign Ext" if (c & 0x8)  else "Sign Ext"   ), \
                                        read_conf,  read_data_size)
        write_conf_str = map(lambda c,s: ("8" if s==1 else ("16" if s==2 else ("24" if s==3 else "??"))) + "-bit, " \
                                        + ("Big Endian" if (c & 0x4)  else "Little Endian") + ", " \
                                        + ("Saturate"    if (c & 0x8)  else "No Saturate"), \
                                        write_conf, write_space_size)

        # print information
        print "Read ports:\n  Port    Status      Offset Address       Size(Bytes)      Data      Config"
        for i in range(len(read_offset_addr)):
            if read_offset_addr[i]:
                print "   %2i     Enabled   %6i (0x%04X)  %5i (0x%04X)    %5i      %s" % \
                            (i, read_offset_addr[i], read_offset_addr[i], read_size[i], read_size[i], read_data[i]/read_data_size[i], read_conf_str[i])
            else:
                print "   %2i     Disabled" % i

        print "Write ports:\n  Port    Status      Offset Address       Size(Bytes)     Space      Config"
        for i in range(len(write_offset_addr)):
            if write_offset_addr[i]:
                print "   %2i     Enabled   %6i (0x%04X)  %5i (0x%04X)    %5i      %s" % \
                            (i, write_offset_addr[i], write_offset_addr[i], write_size[i], write_size[i], write_space[i]/write_space_size[i], write_conf_str[i])
            else:
                print "   %2i     Disabled" % i


    def profiler(self):
        """Read the built-in profiler data. Requires the profiler library to be
        used."""

        class Task(object):
            "Private class to nicely wrap up the profile data"
            def __init__(self, block, addr):
                self.block    = block
                self.addr     = addr
                self.name     = None
            def tidy(self, sym):
                self.name     = sym.varfind(self.addr).name
                self.CPU_FRAC = sym.constfind("$profiler.CPU_FRACTION_FIELD").value
            def __repr__(self):
                if self.name is None:
                    raise Exception("Need to call the tidy method before using")
                return "%-50s - %2.1f %%" % (self.name, self.block[self.CPU_FRAC]/1000)


        # get the head of the list and a couple of constants
        head = self._core.sym.varfind("$profiler.last_addr").addr
        NULL = self._core.sym.constfind("$profiler.LAST_ENTRY").value
        SIZE = self._core.sym.constfind("$profiler.STRUC_SIZE").value
        NEXT = self._core.sym.constfind("$profiler.NEXT_ADDR_FIELD").value

        # get the first address
        curr = self._core.dm[head]

        # read all the structures off the chip as fast as we can
        tasks = []
        while curr != NULL:
            block = self._core.dm[curr:(curr+SIZE)]
            tasks.append(self.Task(block, curr))
            curr = block[NEXT]

        # now fill in the other bits
        for t in tasks:
            t.tidy(self._core.sym)

        # finally return
        return tasks

    def pcprofiler(self, num = 1000, modname = None):
        """Read the program 'num' times and return an array of program counter
           against module.

           The resulting output is an array of sorted values, most called first:
           res = [[count0, 'name0'],
                  [count1, 'name1'],
                  ...]
                  
           The 'modname' parameter can be used to specify a custom function
           for resolving module / function names from PC values. If None, the default
           function Sym.modname is used.
        """

        if modname is None:
            modname = self._core.sym.modname

        # read in the program counter locations
        locations = self._core.other.pcprofile(num)

        # map these to functions and sort
        locations = map(modname, locations)
        locations.sort()

        # now count up the results
        results = [[1, locations.pop(0)]]
        for lcn in locations:
            if results[-1][1] == lcn:
                results[-1][0] += 1
            else:
                results.append([1, lcn])

        # finally sort them to the most frequent first
        results.sort(reverse=True)

        return results

    def _lazyload_matplotlib(self):
        try:
            import matplotlib.pyplot as plt
        except ImportError:
            # we couldn't load this, so can't offer the plotting options
            self._plt = None
        else:
            self._plt = plt
            
    def plotpcprofile(self, num = 1000, modname = None, plotter = None, num_modules = 20, interactive = True):
        """Use pcprofiler to build a program trace, then plot it.

           This routine will try and import matplotlib plotting routines. If
           they fail to do so or if an alternative plot tool is preferred, this
           can be provided via the 'plotter' argument.
        
           'num_modules' can be used to specify the maximum number of modules to plot
           results for. The results for the least frequently hit modules are merged into 'others'.
           
           'interactive' controls whether matplotlib's interactive mode is turned on. If 'interactive'
           is not set, the mode is not altered. See the matplotlib documentation for details.
        """

        # check we have a plotter to use
        if plotter is None:
            self._lazyload_matplotlib()
            plotter = self._plt
        if plotter is None:
            raise Exception("No plotting method available")

        # get the trace
        results = self.pcprofiler(num=num, modname=modname)
        results.sort()

        # interactive mode on
        if interactive:
            plotter.ion()
            
        # check if we have too many results
        size = len(results)
        if size > num_modules:
            # merge the smallest and drop the rest
            others  = sum(map(lambda x: x[0], results[:num_modules-1]))
            # we want the last num_modules values, so the offset we want is negative
            results = results[1-num_modules:]
            results.insert(0, [others, 'others'])
            size    = num_modules

        # split into values and names
        values  = map(lambda x: x[0], results)
        names   = map(lambda x: x[1], results)
        # get the max value to find the middle
        maxVal = max(values)
        midVal = maxVal / 2
        # calculate the percentages
        pcnts = map(lambda x: "%5.1f %%" % (float(x) / num * 100), values)

        # plot these
        fig, ax = plotter.subplots()
        width   = 0.8
        bars    = ax.barh(range(size), values, width, color='b')
        # put the words on and stuff
        ax.set_title('PC profiling (%d samples)' % num)
        ax.set_xlabel('Cycles')
        ax.set_yticks(map(lambda x: x + width/2.0, range(size)))
        ax.set_yticklabels(pcnts)
        # now put the function names in the plot
        for i in xrange(size):
            b     = bars[i]
            name  = names[i]
            width = b.get_width()
            # if the bar is bigger than half width, pin left, otherwise go right
            xloc  = int(0.05 * maxVal)
            align = 'left'
            if (width < midVal):
                xloc  = maxVal
                align = 'right'
            # Center the text vertically in the bar
            yloc = b.get_y()+b.get_height()/2.0
            ax.text(xloc, yloc, name, horizontalalignment=align,
                    verticalalignment='center', weight='bold')
