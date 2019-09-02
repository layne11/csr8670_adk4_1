function [handList, intCpuFrac, corruptList] = kalprofiler( filename, varargin )
%KALPROFILER display Kalimba profiling information.
% Copyright (c) 2005 - 2016 Qualcomm Technologies International, Ltd.
% All Rights Reserved.
% Qualcomm Technologies International, Ltd. Confidential and Proprietary.
% Part of BlueLab-7.1-Release
%   KALPROFILER prints the profile data logged for the current Kalimba
%   application and libraries, IF in the compiled code.
%
%   KALPROFILER( FILENAME ) passes FILENAME to KALLOADSYM to load a KLO file and
%   then returns the profile data.
%
%   [HANDLIST, INTCPUFRAC, CORRUPTLIST] = KALPROFILER(...) returns the
%   profile data. HANDLIST is a 2-column array where its first column holds
%   the memory addresses to the profiler structures and its second column
%   the corresponding cpu fractions (as values between 0 and 1). INTCPUFRAC
%   is the cpu fraction used in interrupts. CORRUPTLIST is a flag which is
%   1 if there is an inconsistency  in the profilers' linked list on
%   Kalimba and 0 otherwise.
%
%   KALPROFILER in timer mode:
%   KALPROFILER can be set up to automatically read the profiler's data
%   from the DSP without keeping Matlab busy. This is done with an internal
%   timer. The profiler structures on Kalimba are read every second. The
%   user has control over this functionality through the following
%   commands: 
%   
%   KALPROFILER(CMD, ARG1, ARG2) % ARG1 and ARG2 only used in 'plot' and
%   'resetpk'
%
%   CMD        |  Description
%   -----------|----------------------------------------------------------
%   'start'    |  starts profiling Kalimba DSP
%   'stop'     |  stops profiling Kalimba DSP
%   'running'  |  prints the status of the profiler on screen
%   'get'      |  returns the data collected so far as a structure
%   'plot'     |  starts plotting the data continuously
%   'dontplot' |  stops plotting and releases/restores the figure
%   'reset'    |  stops collecting data, releases figures and resets/clears
%              |  all internall variables, including the collected data.
%   'detail'   |  attempts to toggle 'detail profiling'
%   'resetpk'  |  resets the peak cycles values on kalimba. Specify the
%              |  name of the section to reset in ARG1. If ARG1 is 'all' or
%              |  blank then all sections are reset
%
%   If you require extra profiling information about number of stalls etc
%   set 'extra_info' inside this function.
%
%   Plotting the data:
%   Using the 'plot' command creats a plot which is not accessible from
%   workspace. That is, you can easily continue to work without
%   accidentally clearing this figure. Furthermore you can select the
%   profiler structures that you want through the second argument, ARG1.
%   ARG1 is a vector with indices of the profilers of interest. These
%   indices are can be selected according to the name field of output data
%   obtained through 'get'. ARG2 determines the maximum number of samples
%   to be plotted. If set to [] all the samples of the selected profilers
%   will be plotted. If extra_info is set the plot will have 4 subplots.
%   These will display cpu fraction, average number of clocks, average number
%   of stalls and average, number of instructions for each profiled section.
%   Each subplot also displays a number for each section, this is the peak
%   value for that section.
%
%   See also kalloadsym, kalsymfind, kalreadval, kalwriteval, kalrunning,
%

% check if we've been passed an argument

persistent tmr        % internal timer
persistent pdat       % profiling data
persistent cnt        % profiling counter = one more than the number of samples
persistent h_fig;     % handle to axes
persistent pname;     % profiler names
persistent plist;     % list of variables to be plotted
persistent nmax;      % the number of samples for plotting
persistent extra_info;
persistent reset_peaks;

% set to 1 if require extra profiling information to be presented
if isempty(extra_info)
   extra_info = 1;
end

if extra_info
    test = kalsymfind('$profiler.RUN_CLKS_MS_MAX_FIELD','const');
    if isempty(test)
       disp ('!!!! Warning, detailed profiling not enabled !!!!');
       extra_info = 0;
    end
end


error(nargchk(0,3,nargin))

if nargin > 0
   if ~isempty(dir(filename))
      kalloadsym( filename )
   else
      % timer mode commands
      switch lower(filename)
         case 'detail'
            extra_info = ~extra_info;
            if (extra_info), disp('detailed profiling ENABLED.');
            else, disp('detailed profiling DISABLED.'); end
         case 'sample'
            [hl, icf, cl] = kalprofiler;
            if cl==1, return; end; % make sure the list is not corrupted
            [hl(:, 1), I] = sort(hl(:, 1));
            hl(:,2:end) = hl(I, 2:end);
            L = size(hl, 1);
            if isempty(pdat)
               pdat.addr = hl(:, 1);            % holds the address of profiler structures on Kalimba
               for i = 1:length(pdat.addr)
                  a = kalsymfind(pdat.addr(i));
                  pname{i} = a{1};
               end
               pdat.cpu_frac = nan(L, 1000);    % holds the cpu fractions
               if extra_info
                  pdat.clks_pk = nan(L, 1000);  % holds the peak number of clocks
                  pdat.clks_av = nan(L, 1000);  % holds the average number of clocks
                  pdat.stalls_pk = nan(L, 1000);  % holds the peak number of stalls
                  pdat.stalls_av = nan(L, 1000);  % holds the average number of stalls
                  pdat.instrs_pk = nan(L, 1000);  % holds the peak number of instructions
                  pdat.instrs_av = nan(L, 1000);  % holds the average number of instructions
               end
               pdat.int_cpu_frac = nan(1, 1000);% holds the interupt cpu fractions
               cnt = 1;
            elseif cnt>length(pdat.int_cpu_frac)
               % append the buffers
               pdat.cpu_frac = [pdat.cpu_frac nan(size(pdat.cpu_frac, 1), 1000)];
               if extra_info
                  pdat.clks_pk = [pdat.clks_pk nan(size(pdat.clks_pk, 1), 1000)];
                  pdat.clks_av = [pdat.clks_av nan(size(pdat.clks_av, 1), 1000)];
                  pdat.stalls_pk = [pdat.stalls_pk nan(size(pdat.stalls_pk, 1), 1000)];
                  pdat.stalls_av = [pdat.stalls_av nan(size(pdat.stalls_av, 1), 1000)];
                  pdat.instrs_pk = [pdat.instrs_pk nan(size(pdat.instrs_pk, 1), 1000)];
                  pdat.instrs_av = [pdat.instrs_av nan(size(pdat.instrs_av, 1), 1000)];
               end
               pdat.int_cpu_frac = [pdat.int_cpu_frac nan(1, 1000)];
            end
            if size(pdat.cpu_frac, 1)==L && (sum(abs(pdat.addr - hl(:, 1)))==0)
               pdat.cpu_frac(:, cnt) = hl(:, 2);
               if extra_info
                  % replace "incorrect" values with NaN
                  temp = hl(:,4:2:8);
                  temp(temp>64) = nan;
                  hl(:,4:2:8) = temp;
                  % store detailed data
                  pdat.clks_pk(:, cnt) = hl(:, 3);
                  pdat.clks_av(:, cnt) = hl(:, 4);
                  pdat.stalls_pk(:, cnt) = hl(:, 5);
                  pdat.stalls_av(:, cnt) = hl(:, 6);
                  pdat.instrs_pk(:, cnt) = hl(:, 7);
                  pdat.instrs_av(:, cnt) = hl(:, 8);
               end
               pdat.int_cpu_frac(cnt) = icf;
               cnt = cnt + 1;
            else
               % todo: is it required to deal with cases where a different
               % number of handles is returned?
               disp('The number/value of handles captured now is different to what is stored internally.')
               disp('This might be because chip has been re-loaded. Try resetting the profiler.')
               warning('Profiler stopped.')
               kalprofiler('stop');
            end
            % plot the results if required
            if ~isempty(h_fig)
               set(h_fig, 'HandleVisibility', 'on')
               set(0,'CurrentFigure',h_fig)               
               if isempty(nmax), xst = 1;
               else xst = max(1, cnt-nmax);
               end
               set(h_fig, 'Name', 'Kalimba DSP Profiler')
               if extra_info
                  subplot(2,2,1), 
               end
               plot(pdat.cpu_frac(plist,1:cnt-1)');
               axis([xst cnt-1 0 1])
               xlabel('sample (sec)')
               ylabel('cpu fraction')
               lh = legend(pname(plist));
               set(lh,'Interpreter','none', 'Location', 'Best',  'Box', 'off')               
               if extra_info
                  subplot(2,2,2), plot(pdat.clks_av(plist,1:cnt-1)');
                  ax = axis;
                  ax(1:2) = [xst cnt-1];
                  ax(3) = max(0, ax(3)); ax(4) = min(64, ax(4));
                  axis(ax);
                  xlabel('sample (sec)')
                  ylabel('clocks average (MIPS)')
                  lh = legend(cellstr(num2str(pdat.clks_pk(plist, cnt-1))));
                  set(lh,'Interpreter','none', 'Location', 'Best',  'Box', 'off')
                  
                  subplot(2,2,3), plot(pdat.stalls_av(plist,1:cnt-1)');
                  ax = axis;
                  ax(1:2) = [xst cnt-1];
                  ax(3) = max(0, ax(3)); ax(4) = min(64, ax(4));
                  axis(ax);
                  xlabel('sample (sec)')
                  ylabel('stalls average (MIPS)')
                  lh = legend(cellstr(num2str(pdat.stalls_pk(plist, cnt-1))));
                  set(lh,'Interpreter','none', 'Location', 'Best',  'Box', 'off')
                  
                  subplot(2,2,4), plot(pdat.instrs_av(plist,1:cnt-1)');
                  ax = axis;
                  ax(1:2) = [xst cnt-1];
                  ax(3) = max(0, ax(3)); ax(4) = min(64, ax(4));
                  axis(ax);
                  xlabel('sample (sec)')
                  ylabel('instructions average (MIPS)')
                  lh = legend(cellstr(num2str(pdat.instrs_pk(plist, cnt-1))));
                  set(lh,'Interpreter','none', 'Location', 'Best',  'Box', 'off')               
               end
               set(h_fig, 'HandleVisibility', 'callback')
            end
         case 'plot'
            if isempty(pdat)
               disp('kalprofiler has not been collecting data.')
            else
               if nargin<2
                  plist = 1:length(pdat.addr);
               else
                  plist = varargin{1};
               end
               if nargin==3
                  nmax = varargin{2};
               end
               plist = plist(plist==round(abs(plist)) & plist<=length(pdat.addr));
               if isempty(h_fig)
                  % create the plot and store its handle
                  figure;
                  h_fig = gcf;
                  set(h_fig, 'CloseRequestFcn', 'kalprofiler(''dontplot'');closereq')
                  set(h_fig, 'Toolbar', 'none')
                  set(h_fig, 'menu', 'none')
                  set(h_fig, 'HandleVisibility', 'callback') % to prevent workspace user plotting on it                  
               end
            end
         case 'dontplot'
            % releases the plot and restores its original properties
            if ~isempty(h_fig)
               set(h_fig, 'Toolbar', 'auto')
               set(h_fig, 'CloseRequestFcn', 'closereq')
               set(h_fig, 'menu', 'figure')
               set(h_fig, 'HandleVisibility', 'on') % to prevent workspace user plotting on it
               h_fig = [];
            end
         case 'get'
            if ~isempty(pdat)
               handList = pdat;
               handList.cpu_frac = handList.cpu_frac(:,1:cnt - 1);
               if extra_info
                  handList.clks_pk = handList.clks_pk(:,1:cnt - 1);
                  handList.clks_av = handList.clks_av(:,1:cnt - 1);
                  handList.stalls_pk = handList.stalls_pk(:,1:cnt - 1);
                  handList.stalls_av = handList.stalls_av(:,1:cnt - 1);
                  handList.instrs_pk = handList.instrs_pk(:,1:cnt - 1);
                  handList.instrs_av = handList.instrs_av(:,1:cnt - 1);
               end
               handList.int_cpu_frac = handList.int_cpu_frac(:,1:cnt - 1);
               handList.name = pname;
            else
               handList = [];
               disp('No data in profiler.')
            end
         case 'start'
            % start the timer after doing a few checks and possibly
            % creating the timer structure
            if isempty(tmr) || ~isvalid(tmr)
               tmr = timer('TimerFcn','kalprofiler(''sample'')', 'Period', 1.0,'ExecutionMode','fixedSpacing');
            elseif isequal(get(tmr, 'Running'), 'on')
               disp('Profiler is already running.')
               return;
            end
            start(tmr);          
         case 'stop'
            % stop the timer if it is running
            if ~isempty(tmr) && isvalid(tmr) && isequal(get(tmr, 'Running'), 'on')
               stop(tmr)
               delete(tmr)
               tmr = [];
            end
         case 'reset'
            kalprofiler('stop');
            kalprofiler('dontplot');
            pdat = [];
            cnt = [];
            h_fig = [];
            pname = [];
            plist = [];
            nmax = [];
         case 'running'
            % display if the timer is running
            if ~isempty(tmr) && isvalid(tmr) && isequal(get(tmr, 'Running'), 'on')
               disp('Profiler is running.')
               disp(['Number of samples so far: ' num2str(cnt - 1)])
            else
               disp('Profiler is NOT running.')
            end
         case 'resetpk'
            % reset all of the max values on kalimba
            if extra_info,
                if nargin > 1
                    reset_peaks = varargin(1);
                else
                    reset_peaks{1} = 'all';
                end
                kalprofiler;
            else
                disp('There are no peaks to reset - detailed profiling is switched off');
            end
         otherwise
            % unknown command
            fprintf(['''' filename ''' is not recognized as an internal command\n or KLO file.\n'])
      end
      return;
   end
end

% get the address of $profiler.NEXT_ADDR_FIELD
temp = kalsymfind('$profiler.NEXT_ADDR_FIELD','const');
% we could have used kalreadval but now we can check profiling is enabled
if isempty(temp)
    error('There are no profiler variables, is profiling enabled?')
end
nextAddrField = temp{1,3};

% get the rest of the symbols ------ % This does not seem to be used at all
% temp = kalsymfind('$profiler.STRUC_SIZE','const');
% profilerStrucSize = temp{1,3};

temp = kalsymfind('$profiler.CPU_FRACTION_FIELD','const');
cpuFracField = temp{1,3};
if extra_info
    temp = kalsymfind('$profiler.RUN_CLKS_MS_MAX_FIELD','const');
    RCMaxMSField = temp{1,3};
    temp = kalsymfind('$profiler.RUN_CLKS_LS_MAX_FIELD','const');
    RCMaxLSField = temp{1,3};
    temp = kalsymfind('$profiler.STALLS_MS_MAX_FIELD','const');
    SMaxMSField = temp{1,3};
    temp = kalsymfind('$profiler.STALLS_LS_MAX_FIELD','const');
    SMaxLSField = temp{1,3};
    temp = kalsymfind('$profiler.INSTRS_MS_MAX_FIELD','const');
    IMaxMSField = temp{1,3};
    temp = kalsymfind('$profiler.INSTRS_LS_MAX_FIELD','const');
    IMaxLSField = temp{1,3};
    temp = kalsymfind('$profiler.RUN_CLKS_AVERAGE_FIELD','const');
    RCAveField = temp{1,3};
    temp = kalsymfind('$profiler.STALLS_AVERAGE_FIELD','const');
    SAveField = temp{1,3};
    temp = kalsymfind('$profiler.INSTRS_AVERAGE_FIELD','const');
    IAveField = temp{1,3};
end

temp = kalsymfind('$profiler.LAST_ENTRY','const');
if (temp{1,3} == -1)
    % 0xFFFF = 65535
    lastEntryConst = 65535;
else
    lastEntryConst = temp{1,3};
end

% get the start of the list
temp = kalreadval('$profiler.last_addr','uint');

if isempty(reset_peaks)
    % set up some variables
    if extra_info
       handList = zeros(1,8);
    end
    handList(1,1:2) = temp; % a 2-column array where its first column points to ... 
                     % profiler structures on Kalimba and its second column
                     % holds the cpu fraction values.
    corruptList = 0;
    corruptIntCpuFrac = 0;
    while (1)
        % these values may occur consequtively, might be quicker as one call
        temp1 = kalreadval( temp(1) + [nextAddrField, cpuFracField], 'uint');
        if extra_info
           temp2 = kalreadval( temp(1) + [RCMaxMSField, RCMaxLSField, RCAveField, SMaxMSField, SMaxLSField, SAveField, IMaxMSField, IMaxLSField, IAveField], 'uint');
           temp2 = [temp2(1)*2^24+temp2(2); temp2(3); temp2(4)*2^24+temp2(5); temp2(6); temp2(7)*2^24+temp2(8); temp2(9)];
           temp1 = [temp1; temp2];
        end
        temp = temp1;

        % store the cpu fraction against the correct field
        handList(end,2:end) = temp(2:end);

        if sum(handList(:,1) == temp(1))
            corruptList = 1;
            break
        end

        if (temp(1) == lastEntryConst)
            break
        end

        handList = [handList; temp(:)'];
    end

    % now get the interrupt cpu fraction
    intCpuFrac = kalreadval('$interrupt.cpu_fraction','int');
    if (intCpuFrac < 0)
        corruptIntCpuFrac = 1;
    end

    if nargout==0
       % get the list of symbols
       symbs = kalsymfind('*profile*');

       if extra_info
          fprintf('Section                                                CPU        Clocks          Stalls       Instructions\n');
          fprintf('      pk - peak cycles     av - average MIPS                     pk      av      pk      av      pk      av\n');
       else
          fprintf('Section                                                CPU\n');
       end

       % print out the details
       for i = (1:size(handList,1))
          temp = find([symbs{:,3}]==handList(i,1));

          if (length(temp) == 1)
             fprintf('  %-50s%6.1f%%', symbs{temp,1}, handList(i,2)/10);
             if extra_info
                 fprintf(' %7.0f %7.3f %7.0f %7.3f %7.0f %7.3f', ...
                 handList(i,3), handList(i,4)/1000, ...
                 handList(i,5), handList(i,6)/1000, ...
                 handList(i,7), handList(i,8)/1000 );
             end
             fprintf('\n');
          else
             fprintf('  ** A unique match could not be found for this variable\n')
             fprintf('  Addr: %-64d%6.1f%%\n', handList(i,1), handList(i,2) )
          end
       end

       if corruptList
          temp = find([symbs{:,3}]==handList(end,i));

          if (length(temp)==1)
             error(['The linked list appears to be corrupt. The handler '...
                symbs{temp, 1} ' appears twice'])
          else
             error(['The linked list appears to be corrupt. The handler '...
                'at address ' int2str(handList(end,1)) ' appears twice'])
          end
       end

       fprintf('%-52s%6.1f%%\n','$interrupt.cpu_fraction', intCpuFrac/10 );

       if corruptIntCpuFrac
           error('$interrupt.cpu_fraction shouldn''t be negative');
       end

       % clear handList so that it is not printed on the screen
       clear handList;
    else
       if corruptIntCpuFrac
           corruptList = 1;
       end
       handList(:,2:2:end) = handList(:,2:2:end)./1000;
       intCpuFrac = intCpuFrac/1000;
    end
else
    match = 1;
    symbs = kalsymfind('*profile*');
    while (1)
        symbolname = symbs(find([symbs{:,3}]==temp), 1);
        if ~isequal(reset_peaks{1}, 'all')
            match = isequal(symbolname{1}, reset_peaks{1});
        end
        if match,
            result = kalwriteval( temp + [RCMaxMSField, RCMaxLSField, SMaxMSField, SMaxLSField, IMaxMSField, IMaxLSField], zeros(6,1) );
            fprintf('%s peaks reset\n', symbolname{1});
        end
        temp = kalreadval( temp + nextAddrField, 'uint');
        if (temp(1) == lastEntryConst)
            break
        end        
    end
    clear reset_peaks;
end