% Kalimba debugging tools.
% Copyright (c) 2005 - 2016 Qualcomm Technologies International, Ltd.
% All Rights Reserved.
% Qualcomm Technologies International, Ltd. Confidential and Proprietary.
%
% Core tools: These tools provide the basic debugging functionality.
%   kalspi              - Connects to a chip or simulator to start a debugging session.
%   kalloadsym          - Loads the debug symbols for a Kalimba program.
%   kalsymfind          - Searches through the symbol table for matching symbol names.
%   kalreadval          - Reads variables/registers from Kalimba data memory, program memory, and registers.
%   kalwriteval         - Writes values to Kalimba data memory, program memory, and registers.
%   kalrunning          - Starts, pauses, and checks the state of the DSP.
%   kalvarprs           - Stores and retrieves the Kalimba symbol table.
%   kalprocessor        - Returns the chip version.
%   kalpcprofiler       - Returns usage statistics by polling the program counter
%   kalports            - Returns the status of each of Kalimba's MMU ports
%
% Additional tools: These tools are dependent on the core tools and the Kalimba libraries.
%   kalcbuffers         - Returns the status of circular buffers in Kalimba.
%   kalreadcbuffer      - Reads from a circular buffer through a cbuffer structure.
%   kalwritecbuffer     - Writes to a circular buffer through a cbuffer structure.
%   kalmessages         - Displays the registered message handlers along with ID.
%   kalmodname          - Lists the module containing the supplied program location.
%   kalprofiler         - Displays Kalimba profiling information.
%   kalstacktrace       - Displays the function stack trace.
%   kaltimers           - Returns the list of timers currently set.


