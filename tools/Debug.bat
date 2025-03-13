@echo off
echo --------------------------------------------------
echo Building Debug
echo --------------------------------------------------
echo[
call "tools\Setup.bat"
premake5 --config=Debug ecc
MsBuild %PLUGIN_NAME%.sln /property:Configuration=Debug
del "%GTA_SA_DIR%\%PLUGIN_NAME%.asi" /Q
del "%GTA_SA_DIR%\%PLUGIN_NAME%.pdb" /Q
xcopy /s "bin\%PLUGIN_NAME%.asi" "%GTA_SA_DIR%" /K /D /H /Y 
xcopy /s "bin\%PLUGIN_NAME%.pdb" "%GTA_SA_DIR%" /K /D /H /Y 