@echo off 
mode 80,10

:: -----------------------------------------
::  
::   - Scite startup portable -
::
::    (wrote for vanilla scite 3.6.1) 
::
::    Thorsten K (2015, marcedo_habmalnefrage_de)
::
::  - Handles arg1 containing spaces in filename
::  - Search path of  %command", extends system Path with it.
::  - Start %command% %1 at %2 Path 
::
:: --- seems Scite 3.6.2 with relative Pathes dont need it anymore -
:: ----------------------------------------

:: ---- use path from second param if been set
IF NOT [%2] == [] (cd %2)

:: ------- This batchfile must stay in the same directory as scite.exe 
 set command=scite_start_usb.cmd

:: ------- Search for this batch file, expand path to file scite.home  
if exist %command% (
 FOR /D  %%I IN (%command%) do echo %%~fI>.scite.tmp
 set /P scite_path=<.scite.tmp
:: ---- store the path we found to make this batchfile work from everywhere.  
 copy /Y .scite.tmp %userprofile%\.scite.path>NUL
 goto continue
) 

:: -- batchfile was  called indirectly.
:: -- eg fuser choose this batchfile from "open with" Dialog.
:: -- Neat we can just load the path we stored before ;)
if exist %userprofile%\.scite.path (
 set /P scite_path=<%userprofile%\.scite.path
) else ( goto fail_filename)

:continue

:: -- Got that shorthand strReplace from
:: -- http://www.dostips.com/DtTipsStringOperations.php
:: -- strip %command%\ from scite_path and extend systems PATH
  
 :: -- move last \  from scite_path
 set str=%scite_path%
 CALL set str=%str:\xscite_start_usb.cmd=%
 set scite_path=%str%

 set Path=%Path%;%scite_path%

:: -- Add systemroot to SciTE_home and scite_Path
 set SciTE_HOME=%scite_path%\
::  set scite_path=%scite_path%

:: -- escaping \ with \\ in first arg, so files in Dirs with spaces will work too.
  set word=\\
  set str=%1
  call set str=%%str:\=%word%%%
  set filename=%str%
 
:: --properly  escaping \ with \\  to scite_path
 set word=\\
 set str=%scite_path%
 call set str=%%str:\=%word%%%
 set scite_path=%str%

 :: -- properly escaping \\ with \\\\ to  for Scites -CWD comand"  
 set word=\\\\
 set str=%scite_path%
 CALL set str=%%str:\\=%word%%%
 set scite_path_ext=%str%

:: echo %scite_path_ext%
:: echo %scite_HOME%
:: echo %filename%

:: -- clean up and start SciTE
 IF exist .scite.tmp del .scite.tmp >NUL
 start /D "%scite_path_ext%" /B Scite.exe %filename% "-cwd:%scite_path_ext%" "-myDefaultHome=%scite_path%"
 
:: Note down how to call scite exe from anywhere on the system. 
 echo. > .scite.read.me.path.txt
 echo "Hint: Use this parameters to open scite from anywhere:" >> .scite.read.me.path.txt
 echo "%%1" "-cwd:%scite_path_ext%" >> .scite.read.me.path.txt
 
 goto end

:fail_filename
 echo.
 echo  * Please fix: cant find %command%
 echo.
 echo  * Tried to use path:  %scite_path%
 echo.
 echo  * Try to copy this batch to where scite.exe is.
 pause

:: -------- It would be more effective to patch SciTE
:: -------- to set scite_home to current directory, if it could not find config files in users home dir
:end