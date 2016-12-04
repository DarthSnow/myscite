@echo off
REM for make debug version use: 
REM >make_with_MinGW.bat DEBUG

setlocal
set PATH=c:\MinGW\bin;%PATH%;

if "%1"=="DEBUG" set DEBUG=1

cd scintilla\win32
 mingw32-make
if errorlevel 1 goto :error

cd ..\..\scite\win32
mingw32-make
if errorlevel 1 goto :error

copy /Y ..\bin\SciTE.exe ..\..\..\
copy /Y ..\bin\SciLexer.dll ..\..\..\
goto end

:error
pause

:end
pause
cd ..\..
