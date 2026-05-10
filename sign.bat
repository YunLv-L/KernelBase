@echo off
set SIGNTOOL=
set PFX=
set PASSWORD=
set SYS=

echo [*] Signing %SYS% ...
"%SIGNTOOL%" sign /v /fd sha256 /f "%PFX%" /p "%PASSWORD%" /tr http://timestamp.digicert.com /td sha256 "%SYS%"
if errorlevel 1 (
    echo [FAIL] Signing failed.
) else (
    echo [OK] Signing succeeded.
)
pause
