% *****************************************************************************
% Copyright (c) 2007 - 2016 Qualcomm Technologies International, Ltd.
% Part of ADK 4.1
%
% *****************************************************************************

% Rick and Gordon has a programmable digital filter integrated into each ADC 
% channel of the stereo CODEC.

% ADC supports the following sample rates:
%  8,000
% 11,025
% 16,000
% 22,050
% 24,000
% 32,000
% 44,100
% 48,000
% 96,000
%
% [Note] Usually, the sampling rate is setted when a SCO is opened with:
%     PcmRateAndRoute(0, PCM_NO_SYNC, 8000, 8000, VM_PCM_INTERNAL_A_AND_B)
Fs = 8000;   % Hz

% Filter Response
%    'low'     : Low pass 
%    'high'    : High Pass 
%    'bandpass': Band Pass 
%    'stop'    : Band Stop 
response = 'high';

% Wp is the pass band edge and Ws is the stop band edge
Wp = 300;     % Hz   Pass band corner frequency
Ws = 3900;    % Hz   Stop band corner frequency

disp(Wp)

Rp = 3;       % dB   Pass band Ripple
Rs = 60;      % dB   Stop band Ripple

% Desired Filter Gain
G = 1;

% DC Blocking 
%   0: disabled 
%   1: enabled
dc_block = 0;

% Rick IIR resolution mode
% 0: 12bit mode
% 1: 16bit mode
rick_mode = 0;

%Rici IIR exp0 factor
exp = 0;

% Filter type:
%    Butterworth
%    Chebyshev_I
%    Chebyshev_II
%    Elliptic
type = 'Butterworth';

Q_format_string = 'S1.10';

% Frequencies are normalized to [0,1], corresponding to the range [0,Fnyq]
Fnyq = Fs/2;  % Hz   Nyquist Frequency
Wp = Wp/Fnyq;       
Ws = Ws/Fnyq;

switch (type)
    case 'Butterworth'
        [order,Wn] = buttord(Wp,Ws,Rp,Rs);
    case 'Chebyshev_I'
        [order,Wn]=cheb1ord(Wp,Ws,Rp,Rs);
    case 'Chebyshev_II'
        [order,Wn]=cheb2ord(Wp,Ws,Rp,Rs);
    case 'Elliptic'
        [order,Wn]=ellipord(Wp,Ws,Rp,Rs);
    otherwise
        disp('ERROR: unsupported filter type')
        return
end

% The filter is a two stage, second order infinite impulse response(IIR):
%    - Two stage second order => 4th Order
if (order > 4)
    disp('It is not possible to design a two stage second order')
    disp('IIR filter with this design specifications.')
    return
else
    % It's Hardware: no need to economize
    order = 4;
end

switch (type)
    case 'Butterworth'
        [Z,P,K] = butter(order,Wp,response);
    case 'Chebyshev_I'
        [Z,P,K]=cheby1(order,Rp,Wp,response);
    case 'Chebyshev_II'
        [Z,P,K]=cheby2(order,Rs,Wp,response);
    case 'Elliptic'
        [Z,P,K]=ellip(order,Rp,Rs,Wp,response);
end

% converts a discrete-time zero-pole-gain representation 
% to an equivalent second-order section representation
[sos, gain] = zp2sos(Z,P,K);

disp(sprintf('\nMatlab second-order section representation'));
disp(sos)
disp(sprintf('\nGain of the sections'));
disp(gain)

sys_gain = G*gain;

%--------------------------------------------------------------------------
% Map the Matlab coefficients to Gordon IIR registers
% 0: Gain
% 1: b01
% 2: b02
% 3: a01
% 4: a02
% 5: b11
% 6: b12
% 7: a11
% 8: a12
% 9: DC Block (1 = enable, 0 = disable)

gordon_reg = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0];

gordon_reg(1) = sys_gain;

gordon_reg(2) = sos(1,2)/sos(1,1);
gordon_reg(3) = sos(1,3)/sos(1,1);
gordon_reg(4) = sos(1,5);
gordon_reg(5) = sos(1,6);

gordon_reg(6) = sos(2,2)/sos(2,1);
gordon_reg(7) = sos(2,3)/sos(2,1);
gordon_reg(8) = sos(2,5);
gordon_reg(9) = sos(2,6);

gordon_reg(10) = dc_block; 

% Display registers and check that they are representable in Q2.X format
disp(sprintf('\nIIR filter coefficients to be converted:'));
disp(gordon_reg)
for i=1:10
    if abs(gordon_reg(i)) > 2
       disp(sprintf('ERROR: The coefficient [%d: %f] is not representable in Q2.X format', (i - 1), gordon_reg(i)));
    end
end

% TODO: Gain information on the internal of the IIR to scale the
% coefficients to avoid OVERFLOW
% NOTE: To have all the coeffients correctly representable in Q2.10 format
% could be not sufficient

% Convert the coefficients in Q2.10 format
Qbits = 12;
Qformat = 10;
Qscale = 2^Qformat;

gordon_reg = gordon_reg * Qscale;
gordon_reg = round(gordon_reg);

% Saturate the coefficients to maximum positive value
Hex_Coeff = cell(size(gordon_reg));
for i=1:10
    % Saturate the coefficients to maximum positive value
    if gordon_reg(i) == 2^(Qbits-1)
        gordon_reg(i) = (2^(Qbits-1)) - 1;
    end

    % 2's complement for negative values
    if gordon_reg(i) < 0
        gordon_reg(i) = gordon_reg(i) + (2^Qbits);
    end
    Hex_Coeff{i} = dec2hex(gordon_reg(i));
end

disp(sprintf('\nGordon IIR registers:'));
disp(Hex_Coeff);

%--------------------------------------------------------------------------
% Write a simple header file with this coefficients
fid = fopen('iir_coeff_gordon.h','w');
  fprintf(fid, '//Q format for the coefficients is %s\n',Q_format_string);
  fprintf(fid, '#ifndef _IIR_COEFF_\n');
  fprintf(fid, '    #define _IIR_COEFF_\n\n');
  
  fprintf(fid, '    #define IIR_COEFF   {');
  for i=1:9
     fprintf(fid, '0x%04X, ', gordon_reg(i));
  end
  fprintf(fid, '0x%04X}\n\n', gordon_reg(10));  
  fprintf(fid,'#endif\n'); 
fclose(fid);
%--------------------------------------------------------------------------

%--------------------------------------------------------------------------
% Map the Matlab coefficients to Rick IIR registers
% 0: Gain0 and Exp0
% 1: b01
% 2: b02
% 3: a01
% 4: a02
% 5: Gain1
% 6: b11
% 7: b12
% 8: a11
% 9: a12
% a: DC Block (1 = enable, 0 = disable)

rick_reg = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];

rick_reg(1) = sys_gain;

rick_reg(2) = sos(1,2)/sos(1,1);
rick_reg(3) = sos(1,3)/sos(1,1);
rick_reg(4) = sos(1,5);
rick_reg(5) = sos(1,6);

rick_reg(6) = 1;

rick_reg(7) = sos(2,2)/sos(2,1);
rick_reg(8) = sos(2,3)/sos(2,1);
rick_reg(9) = sos(2,5);
rick_reg(10) = sos(2,6);

rick_reg(11) = dc_block; 

% Display registers and check that they are representable in Q2.X format
disp(sprintf('\nIIR filter coefficients to be converted:'));
disp(rick_reg)
for i=1:11
    if abs(rick_reg(i)) > 2
       disp(sprintf('ERROR: The coefficient [%d: %f] is not representable in Q2.X format', (i - 1), rick_reg(i)));
    end
end

if rick_mode == 1
    % Convert the coefficients in Q2.14 format
    Qbits = 16;
    Qformat = 14;
    Qscale = 2^Qformat;
    Q_format_string = 'S1.14';
end

rick_reg = rick_reg * Qscale;
rick_reg = round(rick_reg);

% Saturate the coefficients to maximum positive value
Hex_Coeff = cell(size(rick_reg));

for i=1:11
    % Saturate the coefficients to maximum positive value
    if rick_reg(i) == 2^(Qbits-1)
        rick_reg(i) = (2^(Qbits-1)) - 1;
    end

    % 2's complement for negative values
    if rick_reg(i) < 0
        rick_reg(i) = rick_reg(i) + (2^Qbits);
    end
    Hex_Coeff{i} = dec2hex(rick_reg(i));
end

if rick_mode == 1
      % if 16bit representation 0 the 4LSB's
      for i=1:4
        rick_reg(1) = bitset(rick_reg(1),i,0);
       end
else    
     rick_reg(1) = bitshift(rick_reg(1),4);
end 

%Exp is represented by the 3LSB's
rick_reg(1) = rick_reg(1) + exp;
 
Hex_Coeff{1} = dec2hex(rick_reg(1));
 
disp(sprintf('\nRick IIR registers:'));
disp(Hex_Coeff);

%--------------------------------------------------------------------------
% Write a simple header file with this coefficients
fid = fopen('iir_coeff_rick.h','w');
  fprintf(fid, '//Q format for the coefficients is %s\n',Q_format_string);
  fprintf(fid, '#ifndef _IIR_COEFF_\n');
  fprintf(fid, '    #define _IIR_COEFF_\n\n');
  
  fprintf(fid, '    #define IIR_COEFF   {');
  for i=1:10
     fprintf(fid, '0x%04X, ', rick_reg(i));
  end
  fprintf(fid, '0x%04X}\n\n', rick_reg(11));  
  fprintf(fid,'#endif\n'); 
fclose(fid);
%--------------------------------------------------------------------------

% System analysis
[B, A] = sos2tf(sos);
[H, f] = freqz(B, A, 512, Fs);
sys = tf(B, A, 1/Fs);

disp(sprintf('\nPoles distances from the unit circle:'));
disp(abs(eig(sys)));

S = allmargin(sys);
if (S.Stable)
  disp('THE FILTER IS STABLE');
else
  disp('THE FILTER IS UNSTABLE');
end

figure('Name','Filter Analysis')
  %Frequency Response
  subplot(3,1,1);
  plot(f, abs(H));
  grid;
  title('Frequency Response')
  xlabel('Hertz');
  ylabel('Magnitude Response');

  % Pole-Zero Map
  subplot(3,1,2);
  pzmap(sys);

  % Step Response
  subplot(3,1,3);
  step(sys);
  grid;