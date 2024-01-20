@echo off
echo --------------------------------------------------
echo Building Debug
echo --------------------------------------------------
echo[
call "tools\Setup.bat"
MsBuild ModelExtras.sln /property:Configuration=Debug
del %OUT_DIR%"\ModelExtras.asi" /Q
del %OUT_DIR%"\ModelExtras.pdb" /Q
xcopy /s "bin\ModelExtras.asi" %OUT_DIR% /K /D /H /Y 
xcopy /s "bin\ModelExtras.pdb" %OUT_DIR% /K /D /H /Y 