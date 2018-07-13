@echo off
REM for make debug version use:  make_with_VC.bat DEBUG

REM First, we search for any installed Visual studio versions
for /F "tokens=* " %%i in ('reg query HKEY_LOCAL_MACHINE\SOFTWARE\Classes\WOW6432Node\CLSID /d /s /f vcbuild.dll  ') DO (
	REM Now we write all their Pathes into a file
 for /F "tokens=1,2* " %%j in ('@echo "%%i" ^| findstr vcbuild.dll ') DO (
	set dupeCheck=vcDllPath && set vcDllPath=%%l
	if [%dupeCheck%] neq [%vcDllPath%]	call :set_vcPath	
	)
)

REM ~~ found a Path, so call its path setup script. 
:set_vcPath
		set vcPath=%vcDllPath:vcpackages\vcbuild.dll" =%
		set vcPath=%vcPath%common7\tools
		call "%vcPath%vcvarsqueryregistry.bat"
		set vcDllPath=&& set dupeCheck=
		exit /b 0
:end_sub

call vcvarsall.bat x86

if "%1"=="DEBUG" set parameter1=DEBUG=1
REM set parameter1=DEBUG=1

cd scintilla\win32
nmake %parameter1% -f scintilla.mak
if errorlevel 1 goto :error

cd ..\..\scite\win32
nmake %parameter1% -f scite.mak
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

cd ..\..
PAUSE
EXIT
