@echo off
echo --------------------------------------------------
echo Building Release
echo --------------------------------------------------
echo[
call "tools\Setup.bat"
MsBuild VehExtras.sln /property:Configuration=Release 
del %OUT_DIR%"\VehExtras.asi" /Q
xcopy /s "bin\VehExtras.asi" %OUT_DIR% /K /D /H /Y 
