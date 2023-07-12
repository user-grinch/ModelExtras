@echo off
echo --------------------------------------------------
echo Building Debug
echo --------------------------------------------------
echo[
call "tools\Setup.bat"
MsBuild FunctionalComponents.sln /property:Configuration=Debug
del %OUT_DIR%"\FunctionalComponents.asi" /Q
del %OUT_DIR%"\FunctionalComponents.pdb" /Q
xcopy /s "bin\FunctionalComponents.asi" %OUT_DIR% /K /D /H /Y 
xcopy /s "bin\FunctionalComponents.pdb" %OUT_DIR% /K /D /H /Y 