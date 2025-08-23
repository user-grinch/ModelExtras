@echo off
@REM Packs all game files into their proper archives (Yes I'm that lazy)

echo ------------------------------------------------------
echo "ModelExtras Packaging Utility"
echo ------------------------------------------------------
rd /S /Q "archive" /Q
cd tools
echo Packing...
call :copyFiles "ModelExtras"
rd /S /Q "pack" /Q
cd ..
exit


:copyFiles
set "buildPath="..\build\bin\%~1""
set "srcPath="..\src\""
set "folderpath="..\resource\dist\ModelExtras\""
set "iniPath="..\resource\dist\ModelExtras.ini""
set "archivePath="..\archive\%~1.7z""

@REM Remove existing files
rd /S /Q "pack" /Q
rd /S /Q %archivePath% /Q

@REM Copy the files to a temp folder
xcopy /s "%srcPath%\ModelExtrasAPI.h" "pack\" /K /D /H /Y
xcopy /E "%buildPath%.asi" "pack\" /K /D /H /Y 
xcopy /E "%buildPath%.lib" "pack\" /K /D /H /Y 
xcopy /E "%buildPath%.exp" "pack\" /K /D /H /Y 
xcopy /E "%iniPath%" "pack\" /K /D /H /Y 
xcopy /E %folderpath% "pack\%~1\" /K /D /H /Y 

@REM Guessing we have 7zip installed already, well I have 
"C:\Program Files\7-Zip\7z.exe" a -t7z %archivePath% ".\pack\*"

@REM  ------------------------------------------------------