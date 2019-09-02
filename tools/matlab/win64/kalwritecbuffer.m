function wordsWritten = kalwritecbuffer( cbufferName, data, format )
% KALWRITECBUFFER writes data to a cbuffer, and updates its write pointer.
% Copyright (c) 2007 - 2016 Qualcomm Technologies International, Ltd.
% All Rights Reserved.
% Qualcomm Technologies International, Ltd. Confidential and Proprietary.
% Part of BlueLab-7.1-Release
%   N = KALWRITECBUFFER( NAME , DATA , FORMAT) writes the words in vector DATA to
%   the cbuffer structure specified in NAME. The format is given in FORMAT; valid values 
%   for FORMAT correspond to those for the kalwriteval function.
%   If FORMAT is not specified, the default format is the same as for kalwriteval.
%   If the amount of data is larger than the available space, KALWRITECBUFFER
%   only writes as much as possible. The number of words written to the cbuffer is
%   returned in N.
%     
%   See also kalreadcbuffer, kalcbuffers, kalwriteval, kalreadval, kalloadsym.
%

% Check number of input arguments; between two and three.
error(nargchk(2, 3, nargin));

% Grab the buffer details, leaning on kalcbuffers.
buffers         = kalcbuffers();
thisBufferProps = buffers(strcmp({buffers.name}, cbufferName));
if ~length(thisBufferProps)
    error('The buffer "%s" was not recognized as a cbuffer', cbufferName);
end

% Pot as many balls as you can (for the uninitiated: write as much data as there's space for).
% Allow for octet-addressable DM, where the number of words available is not equal to 
% thisBufferProps.availableSpace (which is always in addressable units).
kalArch               = kalprocessor('KAL_ARCH');
archInfo              = kaldspinfo.archInfoFromArchNumber(kalArch);
addrsPerWord          = kaldspinfo.addressesPerWordDM(archInfo);
availableSpaceInWords = thisBufferProps.availableSpace / addrsPerWord;
wordsToWrite          = min(length(data), availableSpaceInWords);
data                  = data(1:wordsToWrite);

% Do the write. May need to do it in two stages, if need to wrap.
bufferEndAddress     = thisBufferProps.startAddress + thisBufferProps.size - 1;
addressesBeforeWrap  = bufferEndAddress - thisBufferProps.writePointer + 1;
% Wrapping on a non-word-boundary (on octet-addressable chips) is not supported. TODO: does it ever need to be?
assert(mod(addressesBeforeWrap, addrsPerWord) == 0);
wordsBeforeWrap      = addressesBeforeWrap / addrsPerWord;
wordsLeft            = wordsToWrite;

if addressesBeforeWrap < (wordsToWrite*addrsPerWord)
    % We need to remove addresses not aligned on word boundaries for octet-addressable chips, or
    % kalaccess (and the lower levels of the Matlab tools) will reject the write. Does nothing if addrsPerWord == 1.
    addressRange = thisBufferProps.writePointer : bufferEndAddress;
    addressRange = addressRange(1 : addrsPerWord : end);
    
    if nargin == 3
        kalwriteval(addressRange, data(1:wordsBeforeWrap), format);
    else
        kalwriteval(addressRange, data(1:wordsBeforeWrap));
    end
    
    data      = data(wordsBeforeWrap + 1 : end);
    wordsLeft = length(data);
    thisBufferProps.writePointer = thisBufferProps.startAddress;
end

addressRange = thisBufferProps.writePointer : (thisBufferProps.writePointer + wordsLeft*addrsPerWord - addrsPerWord);
addressRange = addressRange(1 : addrsPerWord : end);
if nargin == 3
    kalwriteval(addressRange, data, format);
else
    kalwriteval(addressRange, data);
end

% Fix up the write pointer if we walked just past the end (longer walks are errors, and asserted on below)
thisBufferProps.writePointer = thisBufferProps.writePointer + wordsLeft*addrsPerWord;
if thisBufferProps.writePointer == bufferEndAddress + addrsPerWord;
    thisBufferProps.writePointer = thisBufferProps.startAddress;
end

assert(thisBufferProps.writePointer <= bufferEndAddress);

% Update the write pointer:
% Get the right field offset
field = kalsymfind('$cbuffer.WRITE_ADDR_FIELD', 'const');
WRITE_POINTER_FIELD_OFFSET = field{1, 3};
% ..And write back to Kalimba memory.
kalwriteval(cbufferName, thisBufferProps.writePointer, WRITE_POINTER_FIELD_OFFSET, 'UINT');

wordsWritten = wordsToWrite;

end % Top-level function
