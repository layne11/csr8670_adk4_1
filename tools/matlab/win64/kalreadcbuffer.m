function data = kalreadcbuffer( cbufferName, varargin )
%KALREADCBUFFER reads data from a cbuffer in Kalimba memory.
% Copyright (c) 2007 - 2016 Qualcomm Technologies International, Ltd.
% All Rights Reserved.
% Qualcomm Technologies International, Ltd. Confidential and Proprietary.
% Part of BlueLab-7.1-Release
%   [DATA] = KALREADCBUFFER( NAME ) reads all the data available in the
%   cbuffer specified by NAME.
%
%   NOTE: this function does not pause the Kalimba, so the buffer contents may change during
%   the reading process. To obtain a 'static' buffer snapshot, the Kalimba may be paused 
%   prior to calling this function, using the kalrunning function.
%   
%   [DATA] = KALREADCBUFFER( NAME, N ) reads a maximum of N words from the
%   cbuffer specified by NAME. 
%   If fewer than N words of data are available, only the available data are read.
%
%   [DATA] = KALREADCBUFFER( NAME, [N], FORMAT ) applies the formatting given
%   in FORMAT to the data read from the cbuffer. Valid formats correspond to those
%   available for kalreadval; the default formatting is INT. N is optional.
%
%   [DATA] = KALREADCBUFFER( NAME, [N], [FORMAT], 'update') updates the circular
%   buffer read pointer in Kalimba memory after reading the data, thus consuming
%   the data read. N and FORMAT are optional.
%
%   See also kalwritecbuffer, kalreadval, kalwriteval, kalloadsym, kalrunning.
%

% Check number of input arguments; between one and four.
error(nargchk(1, 4, nargin));

% Grab the buffer details, leaning on kalcbuffers.
buffers         = kalcbuffers();
thisBufferProps = buffers(strcmp({buffers.name}, cbufferName));
if ~length(thisBufferProps)
    error('The buffer "%s" was not recognized as a cbuffer', cbufferName);
end

% Allow for octet-addressable DM, where the number of words available is not equal to 
% thisBufferProps.availableData (which is always in addressable units).
kalArch               = kalprocessor('KAL_ARCH');
archInfo              = kaldspinfo.archInfoFromArchNumber(kalArch);
addrsPerWord          = kaldspinfo.addressesPerWordDM(archInfo);
availableDataInWords  = thisBufferProps.availableData / addrsPerWord;

% Arg handling. The order is not enforced *fully* strictly (e.g. format / 'update' can appear in either order).
% Defaults:
format = 'INT';
numWords = availableDataInWords;
shouldUpdateReadPointer = false;

for i = 2:nargin
    arg = varargin{i - 1};
    
    if isstr(arg)
        % Is it the form ( NAME, N ), where N may be a stringified number?
        if ~isempty(str2num(arg))
            numWords = str2num(arg);
        elseif strcmp(lower(arg), 'update')
            shouldUpdateReadPointer = true;
        else
            % Any other string must be a format specifier.
            format = arg;
        end
    elseif isnumeric(arg)
        numWords = arg;
    else
        error('Invalid argument at position %d', i);
    end
end

% Adjust amount of data to read, if needed.
if numWords > availableDataInWords
    numWords = availableDataInWords;
    warning('The cbuffer does not have the requested amount of data. Returning the available data.');
end
numAddresses = numWords * addrsPerWord;

% Early return if no data.
if availableDataInWords == 0
    data = [];
    return;
end

% If there is data, but we were asked for something silly, complain.
if numWords <= 0
    error('Cannot read zero or a negative number of words from the cbuffer.');
end

bufferEndAddress = thisBufferProps.startAddress + thisBufferProps.size - 1;

% Actually read the data. Update the read pointer locally (not yet on Kalimba memory)
if (thisBufferProps.readPointer + numAddresses - 1) > bufferEndAddress   
    % Read the data available to the end.
    % Note that ranges (x : y) in Matlab are inclusive.
    addressesToRead = thisBufferProps.readPointer : bufferEndAddress;
    % We need to remove addresses not aligned on word boundaries for octet-addressable chips, or
    % kalaccess (and the lower levels of the Matlab tools) will reject the read. Does nothing if addrsPerWord == 1.
    addressesToRead = addressesToRead(1 : addrsPerWord : end);
    
    data = kalreadval(addressesToRead, format);
    wordsGotSoFar = length(data);
    % ..And then wrap around, and read the remaining data.
    addressesToRead = thisBufferProps.startAddress : (thisBufferProps.startAddress + numAddresses - (wordsGotSoFar * addrsPerWord) - addrsPerWord);
    addressesToRead = addressesToRead(1 : addrsPerWord : end);
    
    data(wordsGotSoFar + 1 : numWords) = kalreadval(addressesToRead, format);
    thisBufferProps.readPointer = thisBufferProps.readPointer + numAddresses - thisBufferProps.size;
else
    % All the data is available in one contiguous block.
    addressesToRead = thisBufferProps.readPointer : (thisBufferProps.readPointer + numAddresses - addrsPerWord);
    % See above comment (removing addresses which are not word aligned).
    addressesToRead = addressesToRead(1 : addrsPerWord : end);
    
    data = kalreadval(addressesToRead, format);
    thisBufferProps.readPointer = thisBufferProps.readPointer + numAddresses;
    % We may have walked to one-word-past-the-end, in which case we need to rewind.
    if thisBufferProps.readPointer == bufferEndAddress + addrsPerWord
        thisBufferProps.readPointer = thisBufferProps.startAddress;
    end
end

assert(thisBufferProps.readPointer <= bufferEndAddress);

if shouldUpdateReadPointer
    % Get the right field offset
    field = kalsymfind('$cbuffer.READ_ADDR_FIELD', 'const');
    READ_POINTER_FIELD_OFFSET = field{1, 3};
    kalwriteval(cbufferName, thisBufferProps.readPointer, READ_POINTER_FIELD_OFFSET, 'UINT');
end
    
end % Top-level function
