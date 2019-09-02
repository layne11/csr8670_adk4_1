set bluelab_dir=%cd%
set start_dir=%bluelab_dir%\src\lib
cd %start_dir%
%bluelab_dir%\tools\bin\make.exe -R BLUELAB=%bluelab_dir%\tools install doxygen

pause