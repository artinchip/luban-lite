@echo off
rem SPDX-License-Identifier: GPL-2.0+
rem
rem Copyright (C) 2023 ArtInChip Technology Co., Ltd

if "%CD%"=="C:\Windows\system32" (
    echo Current user is administrator, need change path to Luban-Lite ...
    cd /d %~dp0
)

if /i "%processor_architecture%"=="x86" (
        start  .\tools\env\tools\ConEmu\ConEmu.exe 
) else if /i "%processor_architecture%"=="amd64" (
    if defined processor_architew6432 (
        start  .\tools\env\tools\ConEmu\ConEmu.exe 
    ) else (
        start  .\tools\env\tools\ConEmu\ConEmu64.exe 
    )
)

@echo success
