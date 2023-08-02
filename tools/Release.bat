@echo off
echo --------------------------------------------------
echo Building Release
echo --------------------------------------------------
echo[
call "tools\Setup.bat"
MsBuild ModelExtras.sln /property:Configuration=Release 
del %OUT_DIR%"\ModelExtras.asi" /Q
xcopy /s "bin\ModelExtras.asi" %OUT_DIR% /K /D /H /Y 
