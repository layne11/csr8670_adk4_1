function [ varargout ] = kalports( )
%KALPORTS return MMU ports status.
% Copyright (c) 2006 - 2016 Qualcomm Technologies International, Ltd.
% All Rights Reserved.
% Qualcomm Technologies International, Ltd. Confidential and Proprietary.
% Part of BlueLab-7.1-Release
%   KALPORTS() returns the status - connected or disconnected - of each of
%   Kalimba's MMU ports.
%
%   [STATE] = KALPORTS() returns the status of each of Kalimba's MMU ports.
%
%   [STATE, OFFSETADDR] = KALPORTS() returns the status and offset addr of each
%   of Kalimba's MMU ports.
%
%   [STATE, OFFSETADDR, AMNT] = KALPORTS() returns the status, offset and amount
%   of space or data for each of Kalimba's MMU ports.
%
%   [STATE, OFFSETADDR, AMNT, CNFG] = KALPORTS() returns the status, offset,
%   amount of data/space and the value of the corresponding config register
%   of  each of Kalimba's MMU ports.
%
%   See also kalloadsym, kalsymfind, kalreadval, kalwriteval, kalrunning.
%

chip = kalprocessor('kal_arch');
if ~ismember(chip, [1,2,3,5])
    error('kalports does not support Kalimba architecture %d.', chip);
end

% get the offset addresses
readOffsetAddr = kalreadval('$cbuffer.read_port_offset_addr',  'uint');
wrteOffsetAddr = kalreadval('$cbuffer.write_port_offset_addr', 'uint');

% did we get something sensible?
if sum(isnan(readOffsetAddr))
    error('Could not read the Read Port Local offsets from Kalimba')
end
if sum(isnan(wrteOffsetAddr))
    error('Could not read the Write Port Local offsets from Kalimba')
end

% get the limit addresses
readLimit = kalreadval('$cbuffer.read_port_limit_addr',  'uint');
wrteLimit = kalreadval('$cbuffer.write_port_limit_addr', 'uint');
% get the limits
readLimit = kalreadval(readLimit, 'uint');
wrteLimit = kalreadval(wrteLimit, 'uint');

% read the config registers
readCnfg = zeros(length(readOffsetAddr), 1);
wrteCnfg = zeros(length(readOffsetAddr), 1);

num_ports = min(length(readOffsetAddr),11);

for i=1:num_ports
    readCnfg(i) = kalreadval(['$READ_PORT'  int2str(i-1) '_CONFIG'], 'uint');
    wrteCnfg(i) = kalreadval(['$WRITE_PORT' int2str(i-1) '_CONFIG'], 'uint');
end

% work out if this is a BC3 by looking for the local offset variable
switch chip
    case 1
        % its a BC3
        readOffset = kalreadval('$cbuffer.read_port_local_offset',  'uint');
        wrteOffset = kalreadval('$cbuffer.write_port_local_offset', 'uint');
        % remove the odd byte (if present)
        readLimit = bitand( readLimit, 65534 );
        wrteLimit = bitand( wrteLimit, 65534 );

    case {2,3,5}
        % its post BC3
        readOffset = kalreadval(readOffsetAddr, 'uint');
        wrteOffset = kalreadval(wrteOffsetAddr, 'uint');

end

% get the config strings and word sizes
[ readCnfgStr, readWord ] = readPortConfigLook( chip, readOffsetAddr, readCnfg );
[ wrteCnfgStr, wrteWord ] = wrtePortConfigLook( chip, wrteOffsetAddr, wrteCnfg );

% get the port sizes
readSize = kalreadval('$cbuffer.read_port_buffer_size', 'uint');
readMask = readSize - (readSize ~= 0);
wrteSize = kalreadval('$cbuffer.write_port_buffer_size', 'uint');
wrteMask = wrteSize - (wrteSize ~= 0);

% how much data is in the read ports
readData = bitand( readLimit, readMask ) - bitand(readOffset, readMask);
readData = readData + readSize .* (readData < 0);
% how much space is in the write ports
wrteSpace = bitand( wrteLimit, wrteMask ) - bitand( wrteOffset, wrteMask );
wrteSpace = wrteSpace + wrteSize .* (wrteSpace <= 0);
% we always say its one less in the DSP so the buffers don't fill up
wrteSpace = wrteSpace - 1;

% is this a BC5AMAP
temp = kalsymfind('$ARM_MCU_WIN2_SELECT','const');
if length(temp)
    % it is, where are the ports connected
    amapPorts = kalreadval(temp{3}, 'uint');
    if amapPorts
        fprintf('BC5AMAP - DSP ports connected to ARM processor.\n')
    else
        fprintf('BC5AMAP - DSP ports connected to XAP processor.\n')
    end
end

% now generate and appropriate output
switch (nargout)
 case 0
  % make a pretty display
  fprintf('\nRead ports:\n  Port    Status      Offset Address    Size(Bytes)     Data          Config\n')

  for i=1:num_ports
      if readOffsetAddr(i)
          fprintf('    %d     Enabled    %6d (0x%04X)  %5d (0x%04X)   %5d          %s\n', ...
                  i-1, readOffsetAddr(i), readOffsetAddr(i), ...
                  readSize(i), readSize(i), ...
                  floor(readData(i)/readWord(i)), readCnfgStr{i})
      else
          fprintf('    %d     Disabled\n',i-1)
      end
  end

  fprintf('\nWrite ports:\n  Port    Status      Offset Address    Size(Bytes)     Space         Config\n')

  for i=1:num_ports
      if wrteOffsetAddr(i)
          fprintf('    %d     Enabled    %6d (0x%04X)  %5d (0x%04X)   %5d          %s\n', ...
                  i-1, wrteOffsetAddr(i), wrteOffsetAddr(i), ...
                  wrteSize(i), wrteSize(i), ...
                  floor(wrteSpace(i)/wrteWord(i)), wrteCnfgStr{i})
      else
          fprintf('    %d     Disabled\n',i-1)
      end
  end
 case 1
  % status only
  varargout{1} = [(readOffsetAddr(:) ~= 0); (wrteOffsetAddr(:) ~= 0)];
 case 2
  % status and offset addresses
  varargout{1} = [(readOffsetAddr(:) ~= 0); (wrteOffsetAddr(:) ~= 0)];
  varargout{2} = [readOffsetAddr(:);         wrteOffsetAddr(:)      ];
 case 3
  % status, offset addresses and data/space
  varargout{1} = [(readOffsetAddr(:) ~= 0); (wrteOffsetAddr(:) ~= 0)];
  varargout{2} = [readOffsetAddr(:);         wrteOffsetAddr(:)      ];
  varargout{3} = [readData(:);               wrteSpace(:)           ];
 case 4
  % status, offset, addresses, data/space and config
  varargout{1} = [(readOffsetAddr(:) ~= 0); (wrteOffsetAddr(:) ~= 0)];
  varargout{2} = [readOffsetAddr(:);         wrteOffsetAddr(:)      ];
  varargout{3} = [readData(:);               wrteSpace(:)           ];
  varargout{4} = {readCnfgStr{:}             wrteCnfgStr{:}         };
end

return





%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                              %
%  Private functions to convert the config register values into something      %
% understandable.                                                              %
%                                                                              %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function [ str, siz ] = readPortConfigLook( chip, offsetAddr, cnfg )

switch upper(chip)
 case 1
  % make the look up table
  cnfgLook = { 'Not Connected',
               ' 8-bit, Little Endian, Sign Ext,    No pre-fetch',
               '16-bit, Little Endian, Sign Ext,    No pre-fetch',
               ' 8-bit, Big Endian,    Sign Ext,    No pre-fetch',
               '16-bit, Big Endian,    Sign Ext,    No pre-fetch',
               ' 8-bit, Little Endian, Sign Ext,    Pre-fetch',
               '16-bit, Little Endian, Sign Ext,    Pre-fetch',
               ' 8-bit, Big Endian,    Sign Ext,    Pre-fetch',
               '16-bit, Big Endian,    Sign Ext,    Pre-fetch',
               ' 8-bit, Little Endian, No Sign Ext, No pre-fetch',
               '16-bit, Little Endian, No Sign Ext, No pre-fetch',
               ' 8-bit, Big Endian,    No Sign Ext, No pre-fetch',
               '16-bit, Big Endian,    No Sign Ext, No pre-fetch',
               ' 8-bit, Little Endian, No Sign Ext, Pre-fetch',
               '16-bit, Little Endian, No Sign Ext, Pre-fetch',
               ' 8-bit, Big Endian,    No Sign Ext, Pre-fetch',
               '16-bit, Big Endian,    No Sign Ext, Pre-fetch'};
  % and the masks
  strMask = 15;
  sizMask = 1;
 otherwise
  % make the look up table
  cnfgLook = { 'Not Connected',
               ' 8-bit, Little Endian, Sign Ext',
               '16-bit, Little Endian, Sign Ext',
               '24-bit, Little Endian, Sign Ext',
               '*** Unknown ***, Little Endian, Sign Ext',
               ' 8-bit, Big Endian,    Sign Ext',
               '16-bit, Big Endian,    Sign Ext',
               '24-bit, Big Endian,    Sign Ext',
               '*** Unknown ***, Big Endian,    Sign Ext',
               ' 8-bit, Little Endian, No Sign Ext',
               '16-bit, Little Endian, No Sign Ext',
               '24-bit, Little Endian, No Sign Ext',
               '*** Unknown ***, Little Endian, No Sign Ext',
               ' 8-bit, Big Endian,    No Sign Ext',
               '16-bit, Big Endian,    No Sign Ext',
               '24-bit, Big Endian,    No Sign Ext',
               '*** Unknown ***, Big Endian,    No Sign Ext'};
  % and the masks
  strMask = 15;
  sizMask = 3;
end

% make up the config string
str = {cnfgLook{ ( (offsetAddr ~= 0) .* (bitand(cnfg, strMask) + 1) + 1 ) }};
% how big is a word
siz = bitand(cnfg, sizMask) + 1;

return


function [ str, siz ] = wrtePortConfigLook( chip, offsetAddr, cnfg )

switch upper(chip)
 case 1
  % make the look up table
  cnfgLook = { 'Not Connected',
               ' 8-bit, Little Endian',
               '16-bit, Little Endian',
               ' 8-bit, Big Endian',
               '16-bit, Big Endian'};
  % and the masks
  strMask = 3;
  sizMask = 1;
 otherwise
  % make the look up table
  cnfgLook = { 'Not Connected',
               ' 8-bit, Little Endian, No Saturate',
               '16-bit, Little Endian, No Saturate',
               '24-bit, Little Endian, No Saturate',
               '*** Unknown ***, Little Endian, No Saturate',
               ' 8-bit, Big Endian,    No Saturate',
               '16-bit, Big Endian,    No Saturate',
               '24-bit, Big Endian,    No Saturate',
               '*** Unknown ***, Big Endian,    No Saturate',
               ' 8-bit, Little Endian, Saturate',
               '16-bit, Little Endian, Saturate',
               '24-bit, Little Endian, Saturate',
               '*** Unknown ***, Little Endian, Saturate',
               ' 8-bit, Big Endian,    Saturate',
               '16-bit, Big Endian,    Saturate',
               '24-bit, Big Endian,    Saturate',
               '*** Unknown ***, Big Endian,    Saturate'};
  % and the masks
  strMask = 15;
  sizMask = 3;
end

% make up the config string
str = {cnfgLook{ ( (offsetAddr ~= 0) .* (bitand(cnfg, strMask) + 1) + 1 ) }};
% how big is a word
siz = bitand(cnfg, sizMask) + 1;

return