function [ ] = kaltimers( varargin )
%KALTIMERS returns the list of timers currently registered with the core DSP library.
% Copyright (c) 2005 - 2016 Qualcomm Technologies International, Ltd.
% All Rights Reserved.
% Qualcomm Technologies International, Ltd. Confidential and Proprietary.
% Part of BlueLab-7.1-Release
%   KALTIMERS returns the list of timers currently registered with the core DSP library
%   in the order they are due to fire, with details of what routine will be called 
%   and the offset from the previous timer (with the exception of the first timer, 
%   which will show an absolute timeout).
%
%   NOTE: if the DSP is running when this is called, it will be stopped, the
%   required data will be read and then the DSP will be restarted.
%
%   KALTIMERS( FILENAME ) passes FILENAME to KALLOADSYM to load an ELF or KLO file and
%   then returns the timer information.
%
%   See also kalloadsym, kalsymfind, kalreadval, kalwriteval, kalrunning
%

% If we've been passed an argument, assume it's a filename from which to load symbols.
if nargin == 1
    kalloadsym(varargin{1})
end

% Check if the DSP is running
dspState        = kalrunning;
restartDspAtEnd = 0;

% If the DSP is running, pause it, do all the reads, and restart it afterwards.
switch dspState
    case 1
        warning(['****** The DSP is running, it will now be paused. ******' char(10)])
        kalrunning(0);
        restartDspAtEnd = 1;
    case -1
        error('The DSP is not loaded.')
end

% Find the variable which points to the start of the linked list of timers.
% The terminology here is confusing; "lastAddr" always points to the START of the singly-linked list.
lastAddr = kalsymfind('$timer.last_addr');

% Either the app we're debugging doesn't use the core libraries / (or uses a timer library with a 
% different naming convention) or there are no symbols loaded.
if isempty(lastAddr)
    error('The variable $timer.last_addr could not be found. Please check if the symbols are loaded.');
end

% Get the field positions. {1, 3} selects only the constant value.
nextAddrField     = kalsymfind('$timer.NEXT_ADDR_FIELD', 'const');    % field offset of the linked list pointer
nextAddrField     = nextAddrField{1,3};
timeField         = kalsymfind('$timer.TIME_FIELD', 'const');         % field offset of the expiry time
timeField         = timeField{1,3};
handlerAddrField  = kalsymfind('$timer.HANDLER_ADDR_FIELD', 'const'); % field offset of the handler routine address
handlerAddrField  = handlerAddrField{1,3};
idField           = kalsymfind('$timer.ID_FIELD', 'const');           % field offset of the id for this timer
idField           = idField{1,3};
lastEntryMarker   = kalsymfind('$timer.LAST_ENTRY', 'const');         % This constant marks the end of the timer linked list.
lastEntryMarker   = lastEntryMarker{1,3};

% Get address of the timer at the beginning of the linked list. (lastAddr currently holds the address of it).
currentTimerAddress = kalreadval(lastAddr{1,3}, 'uint');

% If there are no timers defined, bail out.
if currentTimerAddress == lastEntryMarker
    fprintf('No timers are registered with the core DSP library.\n');
    return
end

% Grab all defined timer structures which conform to the naming convention.
timerStructureSymbols = kalsymfind('*timer_struc');

fprintf('Timer Structure                         Handler                            Offset(us)\n')

% This allows the first item to show the *absolute* expiry time; subsequent entries will show offwsts.
previousTimerExpiryTime = 0;

% Define a "soft" limit, as a rudimentary guard against list corruption.
timerCount = 0;
softLimit  = 15;

while (currentTimerAddress ~= lastEntryMarker)
    
    timerCount = timerCount + 1;
    
    if timerCount >= softLimit
        response = input([int2str(timerCount) ' registered timers have been found, continue (y/n)? '],'s');
        if isempty(response) || strcmpi(response(1), 'y')
            % Raise limit and continue
            softLimit = softLimit + 15;
        else
            break
        end
    end

    % Trawl through the timer structures to find a match.
    timerStructIndex = find([timerStructureSymbols{:,3}] == currentTimerAddress);

    % If we didn't find a suitable timer structure, it's assumed to be
    % embedded in another variable.
    if (isempty(timerStructIndex))
        structureName = '';
    else
        structureName = timerStructureSymbols{timerStructIndex,1};
    end

    % Compute the offset from the last timer, using the "offset" form of kalreadval.
    timerExpiryTime         = kalreadval(currentTimerAddress, timeField, 'uint');
    offsetFromLastTimer     = timerExpiryTime - previousTimerExpiryTime;
    previousTimerExpiryTime = timerExpiryTime;

    handlerRoutineAddress = kalreadval(currentTimerAddress, handlerAddrField, 'uint');
    handlerRoutineName    = kalmodname(handlerRoutineAddress);

    fprintf('%-40s%-35s%6d\n', structureName, handlerRoutineName.mod, offsetFromLastTimer)

    % Walk the linked list
    currentTimerAddress = kalreadval(currentTimerAddress, nextAddrField, 'uint');
end

if restartDspAtEnd
    warning(['****** Restarting the DSP again. ******' char(10)])
    kalrunning(1);
end
