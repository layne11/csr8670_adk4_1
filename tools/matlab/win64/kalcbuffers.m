function [ buffers ] = kalcbuffers( varargin )
%KALCBUFFERS returns the status of cbuffers in Kalimba.
% Copyright (c) 2005 - 2016 Qualcomm Technologies International, Ltd.
% All Rights Reserved.
% Qualcomm Technologies International, Ltd. Confidential and Proprietary.
% Part of BlueLab-7.1-Release
%   KALCBUFFERS ( ) prints the status of the cbuffers in Kalimba. It requires the
%   structures follow the naming convention '*cbuffer_struc'.
%
%   [ BUFFERS ] = KALCBUFFERS ( ) returns the status of the cbuffers in Kalimba.
%   Each buffer structure has the following fields:
%       name                The name of the cbuffer
%       readPointer         The current read pointer value
%       writePointer        The current write pointer value
%       size                The size of the cbuffer, in addressable units.
%       startAddress        The start address of the cbuffer
%       availableSpace      The available space in the cbuffer, in addressable units.
%       availableData       The number of addressable units of data available to read from the cbuffer.
%
%   [ BUFFERS ] = KALCBUFFERS( FILENAME ) passes FILENAME to KALLOADSYM to load an ELF or 
%   KLO file and then prints or returns the status of the cbuffers in Kalimba.
%
%   See also kalloadsym, kalsymfind, kalreadval, kalwriteval, kalrunning.
%

% If we've been passed an argument, assume it's a filename from which to load symbols.
if nargin == 1
    kalloadsym(varargin{1})
end

% Find cbuffers which follow the prescribed naming convention.
cbuffers = kalsymfind('*cbuffer_struc');

longestNameLength = 0;
for i = 1:size(cbuffers, 1)
    if length(cbuffers{i, 1}) > longestNameLength
        longestNameLength = length(cbuffers{i, 1});
    end
end

% Print a header if not returning data; else prep the return value.
if ~nargout
    fprintf('%-*s Size     Read Address     Write Address     Start           Space     Data\n', longestNameLength, 'C-Buffer')
else
    buffers = struct();
end

for i = 1:size(cbuffers, 1)
    % Grab the field values
    thisBufferName    = cbuffers{i, 1};
    thisBufferAddress = cbuffers{i, 3};
    % Use the field offsets from the DSP libraries.
    field = kalsymfind('$cbuffer.SIZE_FIELD', 'const');
    SIZE_FIELD_OFFSET = field{1, 3};
    field = kalsymfind('$cbuffer.READ_ADDR_FIELD', 'const');
    READ_ADDR_OFFSET  = field{1, 3};
    field = kalsymfind('$cbuffer.WRITE_ADDR_FIELD', 'const');
    WRITE_ADDR_OFFSET = field{1, 3};
    fields = kalreadval([thisBufferAddress + SIZE_FIELD_OFFSET, thisBufferAddress + READ_ADDR_OFFSET, thisBufferAddress + WRITE_ADDR_OFFSET], 'UINT');

    cbufferSize  = fields(1 + SIZE_FIELD_OFFSET);
    readPointer  = fields(1 + READ_ADDR_OFFSET);
    writePointer = fields(1 + WRITE_ADDR_OFFSET);
    
    if cbufferSize == 0
        if ~nargout
            fprintf('%-*s <Size zero>\n', longestNameLength, thisBufferName);
        else
            buffers(i).name           = thisBufferName;
            buffers(i).readPointer    = 0;
            buffers(i).writePointer   = 0;
            buffers(i).size           = cbufferSize;
            buffers(i).startAddress   = 0;
            buffers(i).availableSpace = 0;
            buffers(i).availableData  = 0;
        end
        continue
    end
    
    % Work out the start address of the buffer. Valid start addresses have zeros at all the bits set in the offset mask.
    % Anding with the current current read pointer thus gives its displacement from the start; we then subtract this
    % displacement to obtain the start address. See, for example, Sec. 4.11 CS-101693-UGP9 (BC5 user guide).
    offsetMask = 2^ceil(log2(cbufferSize)) - 1;
    startAddr = readPointer - bitand(readPointer, offsetMask);
    if (startAddr ~= (writePointer - bitand(writePointer, offsetMask)))
        fprintf(['\n** Warning: for the next buffer, the start address calculated from the read and write pointers is different. **\n']);
    end

    availableSpace = readPointer - writePointer;
    % -1: Be consistent with the cbuffer library: space is always reported as one less than the theoretical maximum. 
    % See cbuffer.calc_amount_space.
    availableSpace = cbufferSize*(availableSpace <= 0) + availableSpace - 1;
    availableData  = writePointer - readPointer;
    availableData  = cbufferSize*(availableData < 0) + availableData;
    
    if ~nargout
        fprintf('%-*s%4d      0x%08X       0x%08X        0x%08X      %-6d    %-6d\n', ...
                longestNameLength, thisBufferName, cbufferSize, readPointer, writePointer, startAddr, availableSpace, availableData);
    else
        buffers(i).name           = thisBufferName;
        buffers(i).readPointer    = readPointer;
        buffers(i).writePointer   = writePointer;
        buffers(i).size           = cbufferSize;
        buffers(i).startAddress   = startAddr;
        buffers(i).availableSpace = availableSpace;
        buffers(i).availableData  = availableData;
    end
end
