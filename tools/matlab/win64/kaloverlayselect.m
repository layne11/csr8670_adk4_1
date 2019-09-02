function OB = kaloverlayselect(x)
%KALOVERLAYSELECT selects overlay used by Matlab tools.
% Copyright (c) 2005 - 2016 Qualcomm Technologies International, Ltd.
% All Rights Reserved.
% Qualcomm Technologies International, Ltd. Confidential and Proprietary.
% Part of BlueLab-7.1-Release
%   KALOVERLAYSELECT initialises its internal state and selects a fefault
%   set of overlays.
%
%   KALOVERLAYSELECT(X) selects overlay X, where X is the Matlab Index of the
%   overlay as displayed by calling KALOVERLAYSELECT. 
%
%   S = KALOVERLAYSELECT(..) returns a structure containing overlay
%   information.
%
%   NOTE: KALOVERLAYSELECT does not load the overlay in Kalimba nor does it
%         detect which overlay is loaded.
%
%   See also kalstacktrace, kalsymfind, kalmodname

% Persistent Variables
persistent MODULE_SYMB
persistent OVERLAY_STRUCTURE

if nargin==1 && isequal(x, 'reset')
   MODULE_SYMB = [];
   OVERLAY_STRUCTURE = [];
   return;
end

if isempty(MODULE_SYMB)
%    disp('Analysing overlay structure ...')
   % initialise
   MODULE_SYMB = kalvarprs('MODULE');
   m = MODULE_SYMB;
   
   % find blocks and overlays
   c = 1;
   start_index = 1;
%    mod_list = {m{start_index, 1}};
   for i = 2:size(m,1)
      if (m{i, 3} - 1)~=m{i-1, 3};
         OVERLAY_STRUCTURE(c).start_pc = m{start_index, 3};
         OVERLAY_STRUCTURE(c).end_pc = m{i-1, 3};
         OVERLAY_STRUCTURE(c).name = m{start_index, 1};
         OVERLAY_STRUCTURE(c).start_index = start_index;
         OVERLAY_STRUCTURE(c).end_index = i - 1;
%          OVERLAY_STRUCTURE(c).modules = mod_list;
         OVERLAY_STRUCTURE(c).selected = 1;
         c = c + 1;
         start_index = i;
%          mod_list = {m{start_index, 1}};
      end
%       if ~ismember(mod_list, m{i,1})
%          mod_list{end + 1} = m{i, 1};
%       end
   end
   OVERLAY_STRUCTURE(c).start_pc = m{start_index, 3};
   OVERLAY_STRUCTURE(c).end_pc = m{i, 3};
   OVERLAY_STRUCTURE(c).name = m{start_index, 1};
   OVERLAY_STRUCTURE(c).start_index = start_index;
   OVERLAY_STRUCTURE(c).end_index = i;
%    OVERLAY_STRUCTURE(c).modules = mod_list;
   OVERLAY_STRUCTURE(c).selected = 1;
   
   % Initial selection of overlays
   a = OVERLAY_STRUCTURE(1).start_pc:OVERLAY_STRUCTURE(1).end_pc;
   for i = 2:length(OVERLAY_STRUCTURE)
      b = OVERLAY_STRUCTURE(i).start_pc:OVERLAY_STRUCTURE(i).end_pc;
      if sum(ismember(b, a)) == 0;
         a = [a b];
      else
         OVERLAY_STRUCTURE(i).selected = 0;
      end
   end
end

% if an overlay is selected, de-select overlapping overlays
if nargin == 1
   if isscalar(x) && x == round(abs(x)) && x>0 && x<=length(OVERLAY_STRUCTURE)      
      a = OVERLAY_STRUCTURE(x).start_pc:OVERLAY_STRUCTURE(x).end_pc;
      OVERLAY_STRUCTURE(x).selected = 1;
      for i =1:length(OVERLAY_STRUCTURE)
         if (OVERLAY_STRUCTURE(i).selected == 1) && i~=x
            b = OVERLAY_STRUCTURE(i).start_pc:OVERLAY_STRUCTURE(i).end_pc;
            if sum(ismember(b, a)) == 0;
               a = [a b];
            else
               OVERLAY_STRUCTURE(i).selected = 0;
            end
         end
      end      
   else
      error('Invalid input.');
   end
end

% Adjust symbols to reflect selected overlays
m = MODULE_SYMB;
a = [];
for i = 1:length(OVERLAY_STRUCTURE)
   if (OVERLAY_STRUCTURE(i).selected == 1)
      a = [a OVERLAY_STRUCTURE(i).start_index:OVERLAY_STRUCTURE(i).end_index];
   end
end
m = m(a, :);
kalvarprs(m, 'MODULE');

% Display Overlays
fprintf('%-70s%-20s%-10s\n','First module in overlay', 'Matlab Index', 'Slected')
for i = 1:length(OVERLAY_STRUCTURE)
   if OVERLAY_STRUCTURE(i).selected, sel = 'Yes'; else sel = 'No'; end
   fprintf('%-70s%-20d%s\n',OVERLAY_STRUCTURE(i).name, i, sel)
end
if nargout>0
   OB = OVERLAY_STRUCTURE;
end





















