@echo off
path = C:\MinGW\bin

if %1X == installX goto install

mingw32-make -fMakefile.mingw %1
goto end

:install
if %2X == X goto error

mingw32-make -fMakefile.mingw %1 -e prefix=%2
goto end

:error
echo:
echo   You didn't supply the Install Path!
echo:
echo   example: make_mingw install C:\d2pack109
echo:
pause
goto end

:end
