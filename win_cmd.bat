@echo off
rem SPDX-License-Identifier: GPL-2.0+
rem
rem Copyright (C) 2023 ArtInChip Technology Co., Ltd

ver | find "Windows" > NUL || goto end

if "%CD%"=="C:\Windows\system32" (
    echo Current user is administrator, need change path to Luban-Lite ...
    cd /d %~dp0
)

set PYTHONIOENCODING=UTF-8
set SDK_PRJ_TOP_DIR=%cd%
set PATH=%SDK_PRJ_TOP_DIR%\tools\env\tools\bin;%PATH%
set PATH=%SDK_PRJ_TOP_DIR%\tools\env\tools\Python27\Scripts;%PATH%
set PATH=%SDK_PRJ_TOP_DIR%\tools\env\tools\Python39;%PATH%
set PATH=%SDK_PRJ_TOP_DIR%\tools\scripts\onestep;%PATH%

set ENV_ROOT=%SDK_PRJ_TOP_DIR%\tools\env
set PKGS_ROOT=%ENV_ROOT%\packages
set RTT_ROOT=%SDK_PRJ_TOP_DIR%\kernel\rt-thread

rem OneStep command
doskey list=scons --list-def -C %SDK_PRJ_TOP_DIR%
doskey m=scons -C %SDK_PRJ_TOP_DIR% -j 8
doskey c=scons -c -C %SDK_PRJ_TOP_DIR%
doskey i=scons --info -C %SDK_PRJ_TOP_DIR%
doskey croot=cd /d %SDK_PRJ_TOP_DIR%
doskey cr=cd /d %SDK_PRJ_TOP_DIR%
doskey cb=cbuild
doskey co=cout
doskey ct=ctarget

chcp 65001 > nul
echo Luban-Lite command for Windows
echo.
echo      _         _   ___        ___ _     _
echo     / \   _ __^| ^|_^|_ _^|_ __  / __^| ^|__ (_)_ __
echo    / _ \ ^| '__^| __^|^| ^|^| '_ \^| ^|  ^| '_ \^| ^| '_ \
echo   / ___ \^| ^|  ^| ^|_ ^| ^|^| ^| ^| ^| ^|__^| ^| ^| ^| ^| ^|_) ^|
echo  /_/   \_\_^|   \__^|___^|_^| ^|_^|\___^|_^| ^|_^|_^| .__/
echo                                          ^|_^|
echo.

cmd.exe

end:
echo "Current OS is not Windows!"
