@echo off
echo --------------------------------------------------
echo Building Debug
echo --------------------------------------------------
echo[
call "tools\Setup.bat"
MsBuild VehExtras.sln /property:Configuration=Debug
del %OUT_DIR%"\VehExtras.asi" /Q
del %OUT_DIR%"\VehExtras.pdb" /Q
xcopy /s "bin\VehExtras.asi" %OUT_DIR% /K /D /H /Y 
xcopy /s "bin\VehExtras.pdb" %OUT_DIR% /K /D /H /Y 