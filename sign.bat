@echo off
setlocal

set SIGNTOOL=D:\WindowsKit\bin\10.0.26100.0\x64\signtool.exe
set PFX=certs\TestCert.pfx
set PASSWORD=none
set SYS=KernelBase.sys

if not exist "%SYS%" (
    echo [ERROR] KernelBase.sys not found. Run build.bat first.
    exit /b 1
)

if not exist "%PFX%" (
    echo [ERROR] Test certificate not found at %PFX%.
    echo [INFO] Please generate your own certificate, or place a .pfx file in the certs\ folder.
    exit /b 1
)

echo [*] Signing %SYS% with test certificate...
"%SIGNTOOL%" sign /v /fd sha256 /f "%PFX%" /p "%PASSWORD%" /tr http://timestamp.digicert.com /td sha256 "%SYS%"

if errorlevel 1 (
    echo [FAIL] Signing failed.
    exit /b 1
)

echo [OK] Signing succeeded.
endlocal
