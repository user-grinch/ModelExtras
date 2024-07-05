@echo off
echo --------------------------------------------------
echo Building Release
echo --------------------------------------------------
echo[
call "tools\Setup.bat"
MsBuild %PLUGIN_NAME%.sln /property:Configuration=Release 
del "%GTA_SA_DIR%\%PLUGIN_NAME%.asi" /Q
xcopy /s "bin\%PLUGIN_NAME%.asi" "%GTA_SA_DIR%" /K /D /H /Y 
