@echo off
setlocal

:: Store the original directory
set "ORIGINAL_DIR=%CD%"

:: Define the path to your certificate
set "CERT_FILE=%CERT_DIR%\grinchcert.pfx"

:: Add signtool directory to PATH for this script
set "PATH=%PATH%;C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64"

:: Sign all files provided as arguments
echo --------------------------------------------------
echo Signing ModelExtras.asi with certificate
echo --------------------------------------------------
:loop
if "%~1"=="" goto end

signtool sign /f "%CERT_FILE%" /p "%CERT_PASSWORD%" /fd SHA256 "%~1"
shift
goto loop

:end
echo --------------------------------------------------
echo Signing complete
echo --------------------------------------------------
endlocal