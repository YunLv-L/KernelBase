@echo off
setlocal EnableDelayedExpansion

rem ---------- Path Configuration ----------
set WDK_ROOT=D:\WindowsKit
set WDK_VERSION=10.0.26100.0
set ARCH=x64

set INC_KM=%WDK_ROOT%\Include\%WDK_VERSION%\km
set INC_SHARED=%WDK_ROOT%\Include\%WDK_VERSION%\shared
set INC_UCRT=%WDK_ROOT%\Include\%WDK_VERSION%\ucrt
set LIB_KM=%WDK_ROOT%\Lib\%WDK_VERSION%\km\%ARCH%

if not defined VSINSTALLDIR (
    echo [ERROR] Please run this script from "x64 Native Tools Command Prompt for VS".
    exit /b 1
)

rem Compiler options: /wd4996 silences ExAllocatePoolWithTag deprecation warning
set CL_OPTS=/nologo /Gz /GS- /EHa- /kernel /W4 /WX /wd4996 /wd4819 /O2 /D"_AMD64_" /D"_WIN64" /D"NDEBUG"
set CL_INCLUDES=/I"%INC_KM%" /I"%INC_SHARED%" /I"%INC_UCRT%"
set LINK_OPTS=/nologo /machine:%ARCH% /subsystem:native /driver /entry:DriverEntry

rem ---------- Compile ----------
echo [*] Compiling...
cl.exe %CL_OPTS% %CL_INCLUDES% /c driver.c kernelbase.c
if errorlevel 1 (
    echo [FAIL] Compilation failed.
    exit /b 1
)

rem ---------- Link ----------
echo [*] Linking...
link.exe %LINK_OPTS% /out:KernelBase.sys /DEF:exports.def driver.obj kernelbase.obj ntoskrnl.lib /LIBPATH:"%LIB_KM%"
if errorlevel 1 (
    echo [FAIL] Linking failed.
    exit /b 1
)

echo [OK] KernelBase.sys generated successfully.
endlocal