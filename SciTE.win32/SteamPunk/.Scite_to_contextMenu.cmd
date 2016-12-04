@echo off
mode 85,20
REM ---------------- 
REM
REM  Add Scite to Explorer "open with" Context Menu
REM  -Creates Regfile-.
REM
REM  *Outputfile has to be imported manally. Please check it beforehand*
REM
REM 
REM :: Created Nov 2015, Marcedo@HabmalneFrage.de
REM :: URL: https://sourceforge.net/projects/scite-webdev/?source=directory
REM
REM
REM Search for %cmd% in actual and up to 2 parent Directories
REM Use full qualified path. 
REM
REM ----------------

set cmd=SciTE.exe

REM ------- this batch can reside in a subdir to support a more clean directory structure
REM ------- write path of %cmd% in scite_cmd

:: ------- Check for and write path of %cmd% in scite_cmd
IF EXIST ..\%cmd% ( 
 set scite_cmd="..\%cmd%" 
 GOTO continue)

IF EXIST ..\..\%cmd% ( 
 set scite_cmd="..\..\%cmd%"
 GOTO continue)
 
IF NOT EXIST %cmd% (
 GOTO fail_filename)

:continue
 REM ------- Search for %scite_cmd%, expand its path to file scite.tmp
 FOR /D  %%I IN (%scite_cmd%) do echo %%~fI >scite.tmp
 set /P scite_path=<scite.tmp


REM -- Got that shorthand strReplace from
REM -- http://www.dostips.com/DtTipsStringOperations.php
REM -- Remove  \ %cmd%  from scite_path and extend systems PATH
 set str=%scite_path%
 call set str=%str:\scite.exe =%
 set scite_path=%str%

:: -- replace string \ with \\ 
 set word=\\
 set str=%scite_path%
 CALL set str=%%str:\=%word%%%
 set scite_path=%str%

 :: -- replace string \\ with \\\\ to properly escape two backslashes for Scites -CWD comand"  
 set word=\\\\
 set str=%scite_path%
 CALL set str=%%str:\\=%word%%%
 set scite_path_ext=%str%
:: echo %scite_path_ext%
 
 set RegFile="#add.scite.to.context.menu.reg"
 set scite_cmd="\"%scite_path%\\%cmd%\" \"%%1\" \"-CWD:%scite_path_ext%\""

REM ---- Finally, write the .reg file, \" escapes double quotes
 echo Windows Registry Editor Version 5.00 > %RegFile%
 echo [-HKEY_CLASSES_ROOT\*\shell\Open with SciTE] >> %RegFile%
 echo [HKEY_CLASSES_ROOT\*\shell\Open with SciTE] >>%RegFile%
 echo [HKEY_CLASSES_ROOT\*\shell\Open with SciTE\command] >> %RegFile%
REM  echo @="H:\\Playground\\SciTE_webdev\\SciTE.exe %%1" >> %RegFile%
echo @=%scite_cmd% >>%RegFile%
 

:: ----  Note down how to call scite exe from anywhere on the system. 
  set /P scite_path=<scite.tmp
 echo. > _scite.read.me.path.txt
 echo "Hint: Use this parameters to open scite from anywhere:" >> _scite.read.me.path.txt
 echo %scite_path% "%%1" "-cwd:%scite_path_ext%" >> _scite.read.me.path.txt
 
 
REM ------ Clean UP
 
 del scite.tmp
 echo.
 echo Finished writing to --%RegFile%--
 echo Now, please press your favorite key to be Done. HanD! 
pause >NUL
exit

:fail_filename
 echo.
 echo Please fix: %cmd% was'nt found or did'nt match %%cmd
 echo -- Try to copy this file to scites root dir --
pause
exit
