function [ ] = kalmessages( varargin )
%KALMESSAGES displays the registered core DSP library message handlers.
% Copyright (c) 2005 - 2016 Qualcomm Technologies International, Ltd.
% All Rights Reserved.
% Qualcomm Technologies International, Ltd. Confidential and Proprietary.
% Part of BlueLab-7.1-Release
%   KALMESSAGES displays the message handlers currently registered with the core DSP library.
%   Only message structures which follow the usual naming convention '*message_struc' will be found.
%
%   KALMESSAGES( FILEPATH ) passes FILEPATH (which should be a valid path to an ELF file) 
%   to kalloadsym to load debugging symbols, and then proceeds as above.
%
%   See also kalloadsym, kalsymfind, kalreadval, kalwriteval, kalrunning.
%

% If we've been passed an argument, assume it's a filename from which to load symbols.
if nargin == 1
    kalloadsym( varargin{1} )
end

% Find message structures conforming to the DSP library naming convention
messageData = kalsymfind('*message_struc');

if isempty(messageData)
    error(['No message structures were found, have any handlers been' ...
	   ' declared?'])
end

% Get the field positions. {1, 3} selects only the constant value.
nextAddrField = kalsymfind('$message.NEXT_ADDR_FIELD','const');
nextAddrField = nextAddrField{1,3};
idField = kalsymfind('$message.ID_FIELD','const');
idField = idField{1,3};
handlerAddrField = kalsymfind('$message.HANDLER_ADDR_FIELD','const');
handlerAddrField = handlerAddrField{1,3};

% find out how many handlers there are
numMessages = size( messageData, 1 );

% find the address of the first handler
lastAddr = kalreadval('$message.last_addr','uint');

% make a header row
fprintf('Message structure                                                Message Handler                                   Message ID\n')

% in case the list is corrupt, make a counter to stop us
count = 0;
limit = 15;

% work through the structures
while (lastAddr ~= 2^24-1)

    % check the count
    if (count > limit)
        n=input([int2str(count) ' handlers have been found, continue (y/n)? '],'s');
        if (isempty(n))
            % continue
            limit = limit + 15;
        elseif (strcmpi( n(1), 'y'))
            % continue
            limit = limit + 15;
        else
            %stop
            break
        end
    end
    
    count = count + 1;

    % find out which structure to use
    i = find([messageData{:,3}]==lastAddr);

    % did we find anything
    if (isempty(i))
        % the message handler must be buried in a structure
        name = '';
    else
        name = messageData{i,1};
    end

    % get the address of the handler
    hand = kalreadval(lastAddr,handlerAddrField,'uint');
    hand = kalmodname( hand );

    id = kalreadval(lastAddr,idField,'uint');

    fprintf('%-65s%-50s%d-(0x%04X)\n',name,hand.mod,id,id)

    % get the next handler address
    lastAddr = kalreadval(lastAddr,nextAddrField,'uint');

end
