::...::..:::...::..::.:.::
::    SciTE Prod   ::  
::...::..:::...::..::.:.::

@echo off
set PATH=E:\MinGW\bin;%PATH%;
setlocal enabledelayedexpansion enableextensions

:: ... use customized CMD Terminal
if "%1"=="" (
  reg import ...\contrib\TinyTonCMD\TinyTonCMD.reg
  start "TinyTonCMD" .make_release_with_MinGW.bat tiny
  EXIT
)

echo ::..::..:::..::..::.:.::
echo ::    SciTE Prod      ::
echo ::..::..:::..::..::.:.::

del Release\SciTE.exe 2>NUL
del Release\SciLexer.dll 2>NUL

cd scintilla\win32
mingw32-make -j %NUMBER_OF_PROCESSORS%
if errorlevel 1 goto :error

cd ..\..\scite\win32
mingw32-make -j %NUMBER_OF_PROCESSORS%
if errorlevel 1 goto :error

echo :--------------------------------------------------
echo .... done ....
echo :--------------------------------------------------
::--------------------------------------------------
:: This littl hack looks for a platform PE Signature at offset 120+
:: Should work compiler independent for uncompressed binaries.
set PLAT=""
set off32=""
set off64=""

for /f "delims=:" %%A in ('findstr /o "^.*PE..L.. " ..\bin\SciTE.exe') do ( set off32=%%A ) 
if %off32%==120 set PLAT=WIN32

for /f "delims=:" %%A in ('findstr /o "^.*PE..d.. " ..\bin\SciTE.exe') do ( set off64=%%A ) 
if %off64%==120 set PLAT=WIN64

echo .... Targets platform [%PLAT%] ......
If [%PLAT%]==[WIN32] (
move ..\bin\SciTE.exe ..\..\release
move ..\bin\SciLexer.dll ..\..\release
)

If [%PLAT%]==[WIN64] (
move ..\bin\SciTE.exe ..\..\release
move ..\bin\SciLexer.dll ..\..\release
)

goto end

:error
pause

:end
cd ..\..
PAUSE
EXIT
