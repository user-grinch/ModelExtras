@echo off
echo --------------------------------------------------
echo Building Release
echo --------------------------------------------------
echo[
call "tools\Setup.bat"
MsBuild FunctionalComponents.sln /property:Configuration=Release 
del %OUT_DIR%"\FunctionalComponents.asi" /Q
xcopy /s "bin\FunctionalComponents.asi" %OUT_DIR% /K /D /H /Y 
