function [ ] = kalstacktrace( varargin )
%KALSTACKTRACE displays the function stack trace.
% Copyright (c) 2005 - 2016 Qualcomm Technologies International, Ltd.
% All Rights Reserved.
% Qualcomm Technologies International, Ltd. Confidential and Proprietary.
% Part of BlueLab-7.1-Release
%   KALSTACKTRACE displays the stacktrace for the current location in Kalimba.
%   NOTE if the DSP is switched on it momentarily pauses the DSP reads the
%   necessary data and then restarts the DSP.
%
%   KALSTACKTRACE( FILENAME ) passes FILENAME to KALLOADSYM to load an ELF or KLO file
%   and then returns the stack trace.
%
%   See also kalloadsym, kalsymfind, kalreadval, kalwriteval, kalrunning,
%

% If we've been passed anything, assume it's a filename, which is forwarded to kalloadsym.
if nargin == 1
    kalloadsym(varargin{1})
end

kalArch = kalprocessor('KAL_ARCH');

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

% Get the registers we need.
pcVal       = kalreadval('PC',       'uint');
rLinkVal    = kalreadval('rLink',    'uint');
% Note that kalaccess will properly resolve rIntLink to a memory-mapped
% register on relevant architectures, so we don't need extra logic here.
rIntLinkVal = kalreadval('rIntLink', 'uint');

% Work out which processor we've got. The actions we need to take to determine the stack
% information depend on the kalArch.
if kalArch == 1
    % must be a BC3 get the details on the variables
    stackBufSym  = kalsymfind(        '$stack.buffer');
    dontUseR9Sym = kalsymfind(   '$stack.dont_use_r9');
    stackPntrSym = kalsymfind(           '$stack.ptr');
    savedRegSym  = kalsymfind('$interrupt.saved_regs');

    % read r9 - if it points into the stack buffer assume its good
    r9 = kalreadval('r9','uint');

    % are we using r9
    dontUseR9   = kalreadval( dontUseR9Sym{1,3}, 'uint');
    % read the stack pointer variable
    stackPntr   = kalreadval( stackPntrSym{1,3}, 'uint');

    if ~or((stackBufSym{1,3} > r9), ((stackBufSym{1,3} + stackBufSym{1,2}) < r9))
        % assume r9 is the stack pointer
        stackPntr = r9;
    elseif ~dontUseR9
        % something wrong nothing seems to be a valid stack pointer
        error(['r9 does not point into the stack buffer and' ...
               ' ''dont_use_r9'' is not set.'])
    end

    % read in the stack
    stackBuffer = kalreadval( stackBufSym{1,3}:(stackPntr-1), 'uint');

    % read in the saved register buffer if they are not on the stack
    if ~isempty(savedRegSym)
        savedRegRLink = kalreadval(savedRegSym,11,'uint');
    end

    % store the processor type
    proc = 'BC3';
else
    % Grab the values of stack-related memory-mapped registers
    stackBufAddr = kalsymfind('$STACK_START_ADDR','const');
    stackBufEnd  = kalsymfind('$STACK_END_ADDR','const');
    stackPntrSym = kalsymfind('$STACK_POINTER','const');
        
    if kalArch == 2 % arch. 2: mask off bits above 16.
        stackBufAddr{3} = bitand( stackBufAddr{3}, 65535);
        stackBufEnd{3}  = bitand( stackBufEnd{3},  65535);
        stackPntrSym{3} = bitand( stackPntrSym{3}, 65535);
    end

    % read the addresses
    stackBufStartAddr = kalreadval( stackBufAddr{3}, 'uint');
    stackBufEndAddr   = kalreadval(  stackBufEnd{3}, 'uint');
    stackPntr         = kalreadval( stackPntrSym{3}, 'uint');

    % error if the stack pointer is outside the buffer
    if or( (stackBufStartAddr > stackPntr), (stackBufEndAddr < stackPntr) )
        error('The stack pointer is outside the stack buffer.')
    end

    % read the buffer - don't mask to 16 bits, may help differentiate calls
    % from registers pushed onto the stack
    if stackPntr == stackBufStartAddr %Don't try to read a zero-length block if the stack is empty.
        stackBuffer = [];
    else
        stackBuffer = kalreadval(stackBufStartAddr:(stackPntr-1), 'uint');
    end
        
    % store the processor type
    proc = 'BC5';
end

if restartDspAtEnd
    warning(['****** Restarting the DSP again. ******' char(10)])
    kalrunning(1);
end

% get the module information for the locations
pc = kalmodname(pcVal);
archInfo = kaldspinfo.archInfoFromArchNumber(kalArch);
addrsPerWordPM = kaldspinfo.addressesPerWordPM(archInfo);
rLink = kalmodname(rLinkVal - addrsPerWordPM); % subtract, because rLink contains the *next* location.

% work out if the registers are being saved onto the stack in interrupts
nestedInterruptState = kalsymfind('$interrupt.NESTED_INTERRUPTS_ENABLE','const');
% We might not have got anything back, so check.
if length(nestedInterruptState) > 0
    nestedInterruptState = nestedInterruptState{3};
else
    nestedInterruptState = 0;
end
intStackStateSize = kalsymfind('$interrupt.STORE_STATE_SIZE','const');

if isempty(intStackStateSize)
    % the registers are not saved onto the stack by the ISR
    intStackStateSize = 0;
    intType = 2;
else
    % how much is pushed onto the stack
    intStackStateSize = intStackStateSize{1,3};

    % work out where to get rLink and rIntLink from the stack
    if (strcmpi(proc,'BC3'))
        rLinkOffset = 24;
        rIntLinkOffset = 0;
    else
        rLinkOffset = 25;
        rIntLinkOffset = 0;
    end

    if (nestedInterruptState)
        % registers saved onto stack with nested interrupts allowed
        rLinkOffset = rLinkOffset + 3;
        rIntLinkOffset = rIntLinkOffset + 2;
        intType = 0;
    else
        % registers saved onto stack w/o  nested interrupts allowed
        intType = 1;
    end
end

% work out which is the last element on the stack
offsetIntoBuffer = length(stackBuffer);

% initialise the interrupt count
intpt = 0;

% check if there is any data on the stack
if offsetIntoBuffer
    % there is data on the stack, parse the stack remove any saved registers and
    % note locations of interrupt hanlders

    % get the first value from the stack
    m = kalmodname(stackBuffer(offsetIntoBuffer));

    if strcmpi(rLink.mod, pc.mod)
        % if rLink is the same as pc then assume we are back from a call
        stackBuffer(end+1) = pcVal;
        offsetIntoBuffer = offsetIntoBuffer + 1;
    elseif strcmpi(rLink.mod, m.mod)
        % if rLink is the same as the stack assume we have just pushed
        stackBuffer(end+1) = pcVal;
        offsetIntoBuffer = offsetIntoBuffer + 1;
    else
        % rLink is valid
        stackBuffer(end+1) = rLinkVal;
        stackBuffer(end+1) = pcVal;
        offsetIntoBuffer = offsetIntoBuffer + 2;
    end

    % initialise the remaining parameters
    i = offsetIntoBuffer;
    s = {};

    while i
        if (i < 1)
            fprintf('stack-walk loop got confused');
            break
        end
        m = kalmodname(stackBuffer(i));
        s = {s{:} m};

        if strcmpi(m.mod, '$M.interrupt.handler')
            % make a note of where the interrupt is in the stack for later
            intpt = [intpt length(s)];

            % there are three possible interrupt types:
            %                        registers on stack with nested interrupts
            %                        registers on stack w/o  nested interrupts
            %                        registers not on stack
            switch intType
             case 0                % registers on stack with nested interrupts
              % rIntLink will be the PC at the point the interrupt occured
              rIntLink = kalmodname(stackBuffer( i - rIntLinkOffset ));
              % get the saved rLink
              rLink = kalmodname(stackBuffer( i - rLinkOffset ));

              % is there anything else on the stack other than registers
              if (( i - intStackStateSize - 1 ) > 0)
                  % temorarily have a look at the next location on the stack
                  t = kalmodname(stackBuffer( i - intStackStateSize - 1 ));
              else
                  t.mod = '';
              end

              % again we need to try and work out what needs to go in the list
              if strcmpi(rIntLink.mod, rLink.mod)
                  % rLink is the same as rIntLink assume we are back from a call
                  s = {s{:} rIntLink};
              elseif strcmpi(rLink.mod, t.mod)
                  % rLink is the same as the value off the stack assume we have just pushed
                  s = {s{:} rIntLink};
              else
                  % rLink is valid
                  s = {s{:} rIntLink rLink};
              end

             case 1                % registers on stack w/o  nested interrupts
              % rIntLink will be the PC at the point the interrupt occured
              rIntLink = kalmodname(rIntLinkVal);
              % get the saved rLink
              rLink = kalmodname(stackBuffer( i - rLinkOffset ));

              % is there anything else on the stack other than registers
              if (( i - intStackStateSize - 1 ) > 0)
                  % temorarily have a look at the next location on the stack
                  t = kalmodname(stackBuffer( i - intStackStateSize - 1 ));
              else
                  t.mod = '';
              end

              % again we need to try and work out what needs to go in the list
              if strcmpi(rIntLink.mod, rLink.mod)
                  % rLink is the same as rIntLink assume we are back from a call
                  s = {s{:} rIntLink};
              elseif strcmpi(rLink.mod, t.mod)
                  % rLink is the same as the value off the stack assume we have just pushed
                  s = {s{:} rIntLink};
              else
                  % rLink is valid
                  s = {s{:} rIntLink rLink};
              end

             case 2                % registers not on stack
              % rIntLink will be the PC at the point the interrupt occured
              rIntLink = kalmodname(rIntLinkVal);
              % get the saved rLink
              rLink = kalmodname(savedRegRLink);

              % is there anything else on the stack other than registers
              if (( i - intStackStateSize - 1 ) > 0)
                  % temorarily have a look at the next location on the stack
                  t = kalmodname(stackBuffer( i - intStackStateSize - 1 ));
              else
                  t.mod = '';
              end

              % again we need to try and work out what needs to go in the list
              if strcmpi(rIntLink.mod, rLink.mod)
                  % rLink is the same as rIntLink assume we are back from a call
                  s = {s{:} rIntLink};
              elseif strcmpi(rLink.mod, t.mod)
                  % rLink is the same as the value off the stack assume we have just pushed
                  s = {s{:} rIntLink};
              else
                  % rLink is valid
                  s = {s{:} rIntLink rLink};
              end

            end

            % move the index over the registers
            i = i - intStackStateSize;

        end

        i = i - 1;
    end
else
    % there is no data on the stack its just PC and rLink
    if strcmpi(rLink.mod, pc.mod)
        % if rLink is the same as pc then assume we are back from a call
        s = {pc};
    else
        % rLink is valid
        s = {pc rLink};
    end
end

% finish intpt
intpt = [intpt length(s)];

if strcmpi(pc.mod,'$M.error')
    % we must be in the error routine and the value in rLink is the line after the error
    errData = kalmodname( (rLinkVal - 1) );

    % open the file
    fid = fopen( errData.file );
    % did we manage to open the file?
    if (fid == -1)
        fprintf( ['The DSP is in the error routine and Matlab is unable to ' ...
                  'open the file which called the error.'] )
    else

        for i=1:(errData.line-5)
            errline = fgetl( fid );
        end

        fprintf( '\nThe code has called the error routine. The code had reached line %d\n', errData.line )
        fprintf( 'In module %s\n', errData.mod )
        fprintf( 'In file %s\n', errData.file )
        fprintf( 'The section of code is:\n')
        for i = i:errData.line
            fprintf( '%s\n', fgetl( fid ));
        end
        fprintf( '\n\n')
        fclose(fid);
    end
end

for j = (length(intpt)-1):-1:1
    % set up some strings and a counter
    prettyIndent = '  -> ';
    spaces = '                                                 ';
    offset = 0;

    for i = intpt(j+1):-1:(intpt(j)+1)
        % check if we got a valid entry
        if (s{i}.mod == -1)
            % its not valid
            s{i}.mod = '*** Corrupt ***';
            s{i}.line = [];
            s{i}.file = '';
            % kalmodname will have returned the duff PC value so might as well display it
        end

        strlen = length( s{i}.mod );

        fprintf('%s%s%s\tPC: %5d\t  line: %4d\tfile: %s\n',prettyIndent,...
                s{i}.mod, spaces(strlen:end-offset), s{i}.pc, s{i}.line,s{i}.file);
        prettyIndent = ['  ' prettyIndent];
        offset = offset + 2;
    end

    if (j ~= 1)
        % this is not the last thread
        fprintf('\n     --- An Interrupt Occured ---\n')
    else
        fprintf('\n')
    end
end

